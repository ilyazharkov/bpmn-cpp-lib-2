#ifndef BPMN_EXECUTOR_H
#define BPMN_EXECUTOR_H

#include "bpmn/model.h"
#include "./db/orm.h"
#include "./bpmn/parser.h"
#include "./bpmn/services/abstractService.h"
#include <string>
#include <map>
#include <vector>
#include <future>
#include <memory>
#include <random>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace bpmn {

    class ProcessExecutor {
    public:
        // Reuse the ExecutionState from forward header
        // using ExecutionState = bpmn::ExecutionState;

        explicit ProcessExecutor(db::Database& db);

        // Starts a new process instance
        std::string startProcess(const Process& process, const std::string& init_data, std::function<bool(const std::string&)> user_task_callback);
        // Starts a new process by Id
        std::string startProcessById(const std::string& process_id, const std::string& init_data, std::function<bool(const std::string&)> user_task_callback);

        // Resumes a process paused at a user task
        std::string resumeProcess(const std::string& instance_id, const std::string& user_task_result, std::function<bool(const std::string&)> user_task_callback);
        
        // Get output form by Id
        nlohmann::json getFormById(const std::string formId) const;
        const ExecutionState& getExecutionState(const std::string & instanceId) const;
        void completeTask(const std::string& instance_id, const std::string& user_task, const std::string& user_task_callback);

    private:
        std::random_device random_device_;
        std::mt19937 random_engine_;
        boost::uuids::random_generator uuid_generator_;
        std::unique_ptr<ExecutionState> lastState_;
        db::Database& db_;

        // Element type handlers
        void executeElement(const std::string& instance_id, const std::string& element_id, ExecutionState& state);
        void handleStartEvent(const std::string& instance_id, const StartEvent& start_event, ExecutionState& state);
        void handleUserTask(const std::string& instance_id, const UserTask& user_task, ExecutionState& state);
        void handleServiceTask(const std::string& instance_id, const services::ServiceTask& service_task, ExecutionState& state);
        void handleParallelGateway(const std::string& instance_id, const ParallelGateway& gateway, ExecutionState& state);
        void handleExclusiveGateway(const std::string& instance_id, const ExclusiveGateway& gateway, ExecutionState& state);
        void handleEndEvent(const std::string& instance_id, const EndEvent& end_event, ExecutionState& state);
        void handleGatewayCompletion(const std::string& instance_id, const std::string& gateway_id, ExecutionState& state);

        // State management
        void saveState(const std::string& instance_id, ExecutionState& state);
        ExecutionState loadState(const std::string& instance_id);

        // Helper methods
        void log(const std::string& message);
        void handleError(const std::string& instance_id, const std::string& error_message, ExecutionState& state);
        std::vector<FlowElement*> getOutgoingElements(const std::string& element_id, const std::string& process_id);
        std::shared_ptr<SequenceFlow> getDefaultFlow(const ExclusiveGateway& gateway, const std::vector<std::shared_ptr<SequenceFlow>>& flows);

        // Process definition cache
        const Process* getProcessDefinition(const std::string& process_id);

        // UUID generation
        std::string generate_uuid();
    };

} // namespace bpmn

#endif // BPMN_EXECUTOR_H