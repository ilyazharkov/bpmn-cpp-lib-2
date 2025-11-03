#include <gtest/gtest.h>
#include <bpmn/executor.h>
#include <bpmn/model.h>

using namespace bpmn;

class TestExecutor : public ::testing::Test {
protected:
    void SetUp() override {
        // Создаем простой процесс для тестирования
        process = std::make_unique<Process>("test_process", "Test Process");

        // Создаем элементы процесса
        auto startEvent = std::make_shared<StartEvent>("start", "Start Event");
        auto userTask = std::make_shared<UserTask>("user_task", "User Task");
        auto endEvent = std::make_shared<EndEvent>("end", "End Event");

        // Добавляем элементы в процесс
        process->addElement(startEvent);
        process->addElement(userTask);
        process->addElement(endEvent);

        // Создаем последовательные потоки
        process->addSequenceFlow("flow1", "flow1", "start", "user_task");
        process->addSequenceFlow("flow2", "flow2", "user_task", "end");

        // Создаем экземпляр исполнителя
        database = std::make_unique<db::Database>("test_connection_string");
        executor = std::make_unique<ProcessExecutor>(*database);
    }

    std::unique_ptr<Process> process;
    std::unique_ptr<db::Database> database;
    std::unique_ptr<ProcessExecutor> executor;

};

TEST_F(TestExecutor, StartProcess) {
    std::string instanceId = executor->startProcess(*process, "{}",
        [](const std::string& taskId) -> bool {
            return true;
        });
    EXPECT_FALSE(instanceId.empty());
    EXPECT_EQ(instanceId.length(), 36); // UUID length
}

TEST_F(TestExecutor, ExecuteStartEvent) {
    std::string instanceId = executor->startProcess(*process, "{}",
        [](const std::string& taskId) -> bool {
            return true;
        });

    const ExecutionState& state = executor->getExecutionState(instanceId);

    // Проверяем состояние
    EXPECT_EQ(state.process_id, process->getId());
    EXPECT_EQ(state.current_element, "some_expected_element");
    EXPECT_FALSE(state.variables.empty());
}

TEST_F(TestExecutor, CompleteUserTask) {
    std::string instanceId = executor->startProcess(*process, "{}",
        [](const std::string& taskId) -> bool {
            return true;
        });

    // Завершаем user task
    EXPECT_NO_THROW({
        executor->completeTask(instanceId, "user_task", R"({"approved": true})");
        });

    const ExecutionState& state = executor->getExecutionState(instanceId);
    EXPECT_TRUE(state.isCompleted); // Процесс должен быть завершен
}

TEST_F(TestExecutor, ProcessVariables) {
    std::string initData = R"({"days": 5, "reason": "vacation"})";
    std::string instanceId = executor->startProcess(*process, "{}",
        [](const std::string& taskId) -> bool {
            return true;
        });

    const ExecutionState& state = executor->getExecutionState(instanceId);
    
    EXPECT_EQ(state.variables.at("days"), "5");
    EXPECT_EQ(state.variables.at("reason"), "vacation");
}

TEST_F(TestExecutor, InvalidProcess) {
    Process emptyProcess; 
    EXPECT_THROW({
        executor->startProcess(emptyProcess, "{}", [](auto) { return true; });
        }, std::runtime_error);
}

TEST_F(TestExecutor, NonExistentInstance) {
    EXPECT_THROW({
        executor->getExecutionState("non_existent_id");
        }, std::runtime_error);
}