#include <gtest/gtest.h>
#include <bpmn/engine.h>
#include <db/config.h>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace bpmn;

class TestEngine : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем временный конфиг для тестов
        createTestConfig();

        // Создаем движок
        engine = BpmnEngine::createFromConfig("test_config.json");
    }

    void TearDown() override {
        // Удаляем временный конфиг
        std::filesystem::remove("test_config.json");
    }

    void createTestConfig() {
        nlohmann::json config = {
            {"database_host", "localhost"},
            {"database_port", "5432"},
            {"database_name", "bpmn_engine_test"},
            {"database_user", "postgres"},
            {"database_password", "password"}
        };

        std::ofstream file("test_config.json");
        file << config.dump(4);
        file.close();
    }

    std::string getSimpleProcess() {
        return R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
                <process id="vacation_request" name="Vacation Request Process">
                    <startEvent id="start"/>
                    <userTask id="approve_request" name="Approve Vacation Request"/>
                    <endEvent id="end"/>
                    <sequenceFlow id="flow1" sourceRef="start" targetRef="approve_request"/>
                    <sequenceFlow id="flow2" sourceRef="approve_request" targetRef="end"/>
                </process>
            </definitions>
        )";
    }

    std::string getParallelProcess() {
        return R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
                <process id="parallel_process" name="Parallel Process">
                    <startEvent id="start"/>
                    <parallelGateway id="fork"/>
                    <userTask id="task1" name="Task 1"/>
                    <userTask id="task2" name="Task 2"/>
                    <parallelGateway id="join"/>
                    <endEvent id="end"/>
                    <sequenceFlow id="flow1" sourceRef="start" targetRef="fork"/>
                    <sequenceFlow id="flow2" sourceRef="fork" targetRef="task1"/>
                    <sequenceFlow id="flow3" sourceRef="fork" targetRef="task2"/>
                    <sequenceFlow id="flow4" sourceRef="task1" targetRef="join"/>
                    <sequenceFlow id="flow5" sourceRef="task2" targetRef="join"/>
                    <sequenceFlow id="flow6" sourceRef="join" targetRef="end"/>
                </process>
            </definitions>
        )";
    }

    std::unique_ptr<BpmnEngine> engine;
};

TEST_F(TestEngine, CreateEngineFromConfig) {
    EXPECT_NE(engine, nullptr);
}

TEST_F(TestEngine, StartSimpleProcess) {
    std::string instanceId = engine->startProcess(getSimpleProcess(), R"({"days": 5})");

    EXPECT_FALSE(instanceId.empty());
    EXPECT_EQ(instanceId.length(), 36); // UUID
}

TEST_F(TestEngine, GetProcessState) {
    std::string instanceId = engine->startProcess(getSimpleProcess(), R"({"days": 3})");

    std::string state = engine->getProcessState(instanceId);

    EXPECT_FALSE(state.empty());
    // Проверяем что состояние содержит ожидаемые поля
    EXPECT_TRUE(state.find("current_element") != std::string::npos);
    EXPECT_TRUE(state.find("variables") != std::string::npos);
}

TEST_F(TestEngine, CompleteUserTask) {
    std::string instanceId = engine->startProcess(getSimpleProcess(), R"({"days": 2})");

    // Должен быть активный user task
    std::string state = engine->getProcessState(instanceId);
    EXPECT_TRUE(state.find("approve_request") != std::string::npos);

    // Завершаем задачу
    EXPECT_NO_THROW({
        engine->completeTask(instanceId, "approve_request", R"({"approved": true})");
        });

    // Процесс должен быть завершен
    std::string finalState = engine->getProcessState(instanceId);
    EXPECT_TRUE(finalState.find("completed") != std::string::npos);
}

TEST_F(TestEngine, ProcessWithVariables) {
    std::string initData = R"({
        "employee": "John Doe",
        "department": "Engineering",
        "vacation_days": 10,
        "start_date": "2024-01-15"
    })";

    std::string instanceId = engine->startProcess(getSimpleProcess(), initData);

    std::string state = engine->getProcessState(instanceId);

    // Проверяем что переменные сохранились
    EXPECT_TRUE(state.find("John Doe") != std::string::npos);
    EXPECT_TRUE(state.find("Engineering") != std::string::npos);
    EXPECT_TRUE(state.find("10") != std::string::npos);
}

TEST_F(TestEngine, InvalidBPMN) {
    std::string invalidBpmn = "invalid xml content";

    EXPECT_THROW({
        engine->startProcess(invalidBpmn, "{}");
        }, std::runtime_error);
}

TEST_F(TestEngine, NonExistentInstance) {
    EXPECT_THROW({
        engine->getProcessState("non_existent_uuid");
        }, std::runtime_error);
}

TEST_F(TestEngine, CompleteNonExistentTask) {
    std::string instanceId = engine->startProcess(getSimpleProcess(), "{}");

    EXPECT_THROW({
        engine->completeTask(instanceId, "non_existent_task", "{}");
        }, std::runtime_error);
}