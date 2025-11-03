#include <bpmn/engine.h>
#include <iostream>

int main() {
    try {
        // Создание движка из конфига
        auto engine = bpmn::BpmnEngine::createFromConfig("config.json");

        // Запуск процесса
        std::string bpmnXml = R"(
            <?xml version="1.0" encoding="UTF-8"?>
            <definitions>
                <process id="vacation_request">
                    <startEvent id="start"/>
                    <userTask id="approve_request" name="Approve Vacation Request"/>
                    <endEvent id="end"/>
                    <sequenceFlow sourceRef="start" targetRef="approve_request"/>
                    <sequenceFlow sourceRef="approve_request" targetRef="end"/>
                </process>
            </definitions>
        )";

        std::string instanceId = engine->startProcess(bpmnXml, R"({"days": 5})");
        std::cout << "Process started: " << instanceId << std::endl;

        // Получение состояния
        std::string state = engine->getProcessState(instanceId);
        std::cout << "Process state: " << state << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}