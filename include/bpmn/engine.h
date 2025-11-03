#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "db/config.h"
#include "parser.h"
#include "process.h"
#include "executor.h"

namespace bpmn {

    class BpmnEngine {
    public:
        // Фабричные методы для создания движка
        static std::unique_ptr<BpmnEngine> create(const db::DatabaseConfig& config);
        static std::unique_ptr<BpmnEngine> createFromConfig(const std::string& configPath);
        static std::unique_ptr<BpmnEngine> createFromEnvironment();

        // Основной API движка
        std::string startProcess(const std::string& processDefinition, const std::string& initData = "{}");
        std::string startProcessFromFile(const std::string& filePath, const std::string& initData = "{}");

        void completeTask(const std::string& instanceId, const std::string& taskId, const std::string& data = "{}");
        void signalEvent(const std::string& instanceId, const std::string& eventId, const std::string& data = "{}");

        std::string getProcessState(const std::string& instanceId) const;
        std::string getActiveTasks(const std::string& instanceId) const;

        void suspendProcess(const std::string& instanceId);
        void resumeProcess(const std::string& instanceId);
        void terminateProcess(const std::string& instanceId);

        // Управление процессами
        std::vector<std::string> getActiveInstances() const;
        bool isProcessActive(const std::string& instanceId) const;

        // Деструктор
        virtual ~BpmnEngine() = default;

    private:
        BpmnEngine(const db::DatabaseConfig& config);

        // Внутренние методы
        void initializeDatabase();
        void validateProcessDefinition(const std::string& processDefinition) const;
        std::string generateInstanceId() const;

        // Компоненты движка
        db::DatabaseConfig config_;
        std::unique_ptr<BpmnParser> parser_;
        std::unique_ptr<ProcessExecutor> executor_;
        std::unique_ptr<db::Database> database_;
        std::unordered_map<std::string, std::shared_ptr<Process>> processCache_;

        // Состояние движка
        mutable std::mutex engineMutex_;
    };

} // namespace bpmn