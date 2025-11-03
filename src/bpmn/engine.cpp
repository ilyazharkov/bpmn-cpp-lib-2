#include "bpmn/engine.h"
#include "bpmn/parser.h"
#include "bpmn/executor.h"
#include "db/config.h"
#include "db/orm.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>

namespace bpmn {

    // Фабричные методы
    std::unique_ptr<BpmnEngine> BpmnEngine::create(const db::DatabaseConfig& config) {
        return std::unique_ptr<BpmnEngine>(new BpmnEngine(config));
    }

    std::unique_ptr<BpmnEngine> BpmnEngine::createFromConfig(const std::string& configPath) {
        auto config = db::DatabaseConfig::fromJson(configPath);
        return std::unique_ptr<BpmnEngine>(new BpmnEngine(config));
    }

    std::unique_ptr<BpmnEngine> BpmnEngine::createFromEnvironment() {
        auto config = db::DatabaseConfig::fromEnvironment();
        return std::unique_ptr<BpmnEngine>(new BpmnEngine(config));
    }

    // Конструктор
    BpmnEngine::BpmnEngine(const db::DatabaseConfig& config)
        : config_(config) {

        initializeDatabase();
        parser_ = std::make_unique<BpmnParser>();
        // Создаем Database и передаем в ProcessExecutor
        database_ = std::make_unique<db::Database>(config_.getConnectionString());
        executor_ = std::make_unique<ProcessExecutor>(*database_);
    }

    void BpmnEngine::initializeDatabase() {
        try {
            // Создаем соединение с БД для инициализации
            db::Database db(config_.getConnectionString());
            // Схема уже создается в конструкторе Database
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to initialize database: " + std::string(e.what()));
        }
    }

    std::string BpmnEngine::startProcess(const std::string& processDefinition, const std::string& initData) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            // Парсим процесс из XML
            auto process = parser_->parseFromString(processDefinition);

            // Генерируем ID экземпляра
            std::string instanceId = generateInstanceId();

            // Запускаем процесс
            // executor_->startProcess(*process, initData, instanceId);

            // Кэшируем процесс для будущего использования
            processCache_[instanceId] = std::move(process);

            return instanceId;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to start process: " + std::string(e.what()));
        }
    }

    std::string BpmnEngine::startProcessFromFile(const std::string& filePath, const std::string& initData) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            // Парсим процесс из файла
            auto process = parser_->parse(filePath);

            // Генерируем ID экземпляра
            std::string instanceId = generateInstanceId();

            // Запускаем процесс
            // executor_->startProcess(*process, initData, instanceId);

            // Кэшируем процесс
            processCache_[instanceId] = std::move(process);

            return instanceId;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to start process from file: " + std::string(e.what()));
        }
    }

    void BpmnEngine::completeTask(const std::string& instanceId, const std::string& taskId, const std::string& data) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            // Находим процесс в кэше
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Завершаем задачу
            // executor_->completeTask(instanceId, taskId, data);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to complete task: " + std::string(e.what()));
        }
    }

    void BpmnEngine::signalEvent(const std::string& instanceId, const std::string& eventId, const std::string& data) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Сигнализируем событие
            // executor_->signalEvent(instanceId, eventId, data);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to signal event: " + std::string(e.what()));
        }
    }

    std::string BpmnEngine::getProcessState(const std::string& instanceId) const {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Получаем состояние процесса
            // auto state = executor_->getProcessState(instanceId);

            // Пока заглушка - возвращаем JSON с базовой информацией
            nlohmann::json state = {
                {"instance_id", instanceId},
                {"status", "active"},
                {"current_element", "unknown"},
                {"variables", nlohmann::json::object()}
            };

            return state.dump(4);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to get process state: " + std::string(e.what()));
        }
    }

    std::string BpmnEngine::getActiveTasks(const std::string& instanceId) const {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Получаем активные задачи
            // auto tasks = executor_->getActiveTasks(instanceId);

            // Заглушка
            nlohmann::json tasks = nlohmann::json::array();
            // tasks.push_back({{"task_id", "task1"}, {"name", "Approval Task"}});

            return tasks.dump(4);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to get active tasks: " + std::string(e.what()));
        }
    }

    void BpmnEngine::suspendProcess(const std::string& instanceId) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Приостанавливаем процесс
            // executor_->suspendProcess(instanceId);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to suspend process: " + std::string(e.what()));
        }
    }

    void BpmnEngine::resumeProcess(const std::string& instanceId) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Возобновляем процесс
            // executor_->resumeProcess(instanceId);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to resume process: " + std::string(e.what()));
        }
    }

    void BpmnEngine::terminateProcess(const std::string& instanceId) {
        std::lock_guard<std::mutex> lock(engineMutex_);

        try {
            auto it = processCache_.find(instanceId);
            if (it == processCache_.end()) {
                throw std::runtime_error("Process instance not found: " + instanceId);
            }

            // Завершаем процесс
            // executor_->terminateProcess(instanceId);

            // Удаляем из кэша
            processCache_.erase(it);
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to terminate process: " + std::string(e.what()));
        }
    }

    std::vector<std::string> BpmnEngine::getActiveInstances() const {
        std::lock_guard<std::mutex> lock(engineMutex_);

        std::vector<std::string> instances;
        for (const auto& [instanceId, process] : processCache_) {
            instances.push_back(instanceId);
        }

        return instances;
    }

    bool BpmnEngine::isProcessActive(const std::string& instanceId) const {
        std::lock_guard<std::mutex> lock(engineMutex_);
        return processCache_.find(instanceId) != processCache_.end();
    }

    // Вспомогательные методы
    std::string BpmnEngine::generateInstanceId() const {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << std::hex;

        for (int i = 0; i < 32; i++) {
            if (i == 8 || i == 12 || i == 16 || i == 20) {
                ss << "-";
            }
            ss << dis(gen);
        }

        return ss.str();
    }

    void BpmnEngine::validateProcessDefinition(const std::string& processDefinition) const {
        // Базовая валидация XML
        if (processDefinition.empty()) {
            throw std::invalid_argument("Process definition cannot be empty");
        }

        if (processDefinition.find("bpmn2:process") == std::string::npos &&
            processDefinition.find("bpmn:process") == std::string::npos) {
            throw std::invalid_argument("Invalid BPMN process definition");
        }
    }

} // namespace bpmn