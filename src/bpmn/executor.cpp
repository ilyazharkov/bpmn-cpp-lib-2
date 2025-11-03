#include "./bpmn/executor.h"
#include "./bpmn/model.h"


#include <chrono>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace bpmn {

    ProcessExecutor::ProcessExecutor(db::Database& db) : db_(db), random_engine_(random_device_()), uuid_generator_() {}

    std::string ProcessExecutor::startProcessById(const std::string& process_id, const std::string& init_data, std::function<bool(const std::string&)> user_task_callback) {
        const std::string processXml = db_.loadProcessDefinition(process_id);
        BpmnParser parser = BpmnParser();
        const auto processDefinition = parser.parse(processXml);
        return startProcess(*processDefinition, init_data, user_task_callback);
    }
    
    std::string ProcessExecutor::startProcess(const Process& process, const std::string& init_data, std::function<bool(const std::string&)> user_task_callback) {
        // Generate unique process instance ID

        std::string instance_id = generate_uuid();

        ExecutionState state;
        state.current_element = process.getStartEventId();
        state.variables["init_data"] = init_data;
        state.process_id = process.getId();

        saveState(instance_id, state);
        executeElement(instance_id, state.current_element, state);
        if (state.isPaused) {
            if (!user_task_callback(state.current_element)) {
                return "";
            }
        }
        return instance_id;
    }
    nlohmann::json ProcessExecutor::getFormById(const std::string formId) const {
        return db_.getFormById(formId);
    };

    const ExecutionState& ProcessExecutor::getExecutionState(const std::string& instanceId) const {
        if (!lastState_) {
            throw std::runtime_error("No execution state available");
        }
        return *lastState_;
    };

    void ProcessExecutor::completeTask(const std::string& instance_id, const std::string& user_task, const std::string& user_task_callback) {
    
    };


    std::string ProcessExecutor::resumeProcess(const std::string& instance_id, const std::string& user_task_result, std::function<bool(const std::string&)> user_task_callback) {
        ExecutionState state = std::move(loadState(instance_id));
        state.variables["user_task_result"] = user_task_result;

        saveState(instance_id, state);
        executeElement(instance_id, state.current_element, state);
        if (state.isPaused) {
            if (!user_task_callback(state.current_element)) {
                return "";
            }
        }
        return instance_id;
    }

    void ProcessExecutor::executeElement(const std::string& instance_id, const std::string& element_id, ExecutionState& state) {
        const Process* process = getProcessDefinition(state.process_id);
        if (!process) {
            throw std::runtime_error("Process definition not found: " + state.process_id);
        }

        FlowElement* element = process->getElement(element_id);
        if (!element) {
            throw std::runtime_error("Element not found: " + element_id);
        }

        // Handle different element types
        if (auto start_event = dynamic_cast<StartEvent*>(element)) {
            handleStartEvent(instance_id, *start_event, state);
        }
        else if (auto user_task = dynamic_cast<UserTask*>(element)) {
            handleUserTask(instance_id, *user_task, state);
        }
        else if (auto service_task = dynamic_cast<services::ServiceTask*>(element)) {
            handleServiceTask(instance_id, *service_task, state);
        }
        else if (auto parallel_gateway = dynamic_cast<ParallelGateway*>(element)) {
            handleParallelGateway(instance_id, *parallel_gateway, state);
        }
        else if (auto exclusive_gateway = dynamic_cast<ExclusiveGateway*>(element)) {
            handleExclusiveGateway(instance_id, *exclusive_gateway, state);
        }
        else if (auto end_event = dynamic_cast<EndEvent*>(element)) {
            handleEndEvent(instance_id, *end_event, state);
        }
        else {
            throw std::runtime_error("Unsupported element type: " + std::string(typeid(*element).name()));
        }
    }

    void ProcessExecutor::handleStartEvent(const std::string& instance_id, const StartEvent& start_event, ExecutionState& state) {
        log("Process instance " + instance_id + " started");

        // Move to next element
        auto next_elements = getOutgoingElements(start_event.getId(), state.process_id);
        if (next_elements.empty()) {
            throw std::runtime_error("No outgoing sequence flows from start event");
        }

        state.current_element = next_elements[0]->getId();
        saveState(instance_id, state);
        executeElement(instance_id, state.current_element, state);
    }

    void ProcessExecutor::handleUserTask(const std::string& instance_id, const UserTask& user_task, ExecutionState& state) {
        log("User task reached: " + user_task.getId());

        // Save task to database for human completion
        db_.saveUserTask(instance_id, user_task.getId(), user_task.form_key, state.variables);

        // Process pauses here until resumed via REST API
    }

    void ProcessExecutor::handleServiceTask(const std::string& instance_id, const services::ServiceTask& service_task, ExecutionState& state) {
        log("Executing service task: " + service_task.getId());

        try {
            // Execute the service (implementation would be in a separate service layer)
            //service_executor_.execute(service_task, state.variables);

            // Move to next element
            auto next_elements = getOutgoingElements(service_task.getId(), state.process_id);
            if (!next_elements.empty()) {
                state.current_element = next_elements[0]->getId();
                saveState(instance_id, state);
                executeElement(instance_id, state.current_element, state);
            }
        }
        catch (const std::exception& e) {
            handleError(instance_id, "Service task failed: " + std::string(e.what()), state);
        }
    }

    void ProcessExecutor::handleParallelGateway(const std::string& instance_id,
        const ParallelGateway& gateway,
        ExecutionState& state) {
        log("Processing parallel gateway: " + gateway.getId());

        auto outgoing_flows = getProcessDefinition(state.process_id)->getOutgoingFlows(gateway.getId());
        if (outgoing_flows.empty()) {
            throw std::runtime_error("No outgoing flows from parallel gateway");
        }
        // Execute all outgoing paths in parallel
        std::vector<std::future<void>> futures;
        for (const auto& flow : outgoing_flows) {
            futures.emplace_back(std::async(std::launch::async,
                [this, instance_id, target = flow->target_ref,
                proc_id = state.process_id,  // Copy only primitives
                vars = state.variables]() {  // Copy only needed data
                    // Create fresh state without futures
                    ExecutionState branch_state;
                    branch_state.process_id = proc_id;
                    branch_state.current_element = target;
                    branch_state.variables = vars;  // Copy is safe (no futures)

                    executeElement(instance_id, target, branch_state);
                }));
        }

        // Store only if needed (otherwise they'll block in destructor)
        state.parallel_tasks = std::move(futures);

        // Wait for all branches to complete
        for (auto& future : futures) {
            future.wait();
        }

        handleGatewayCompletion(instance_id, gateway.getId(), state);
    }
    void ProcessExecutor::handleGatewayCompletion(const std::string& instance_id, const std::string& gateway_id, ExecutionState& state) {
    
    }
    
    
    void ProcessExecutor::handleExclusiveGateway(const std::string& instance_id, const ExclusiveGateway& gateway, ExecutionState& state) {
        log("Processing exclusive gateway: " + gateway.getId());

        auto outgoing_flows = getProcessDefinition(state.process_id)->getOutgoingFlows(gateway.getId());
        if (outgoing_flows.empty()) {
            throw std::runtime_error("No outgoing flows from exclusive gateway");
        }

        // Evaluate conditions to choose the right path
        std::shared_ptr<SequenceFlow> selected_flow;
        //for (const auto& flow : outgoing_flows) {
        //    if (flow->condition_expression.empty() ||
        //        condition_evaluator_.evaluate(flow->condition_expression, state.variables)) {
        //        selected_flow = flow;
        //        break;
        //    }
        //}

        if (!selected_flow) {
            selected_flow = getDefaultFlow(gateway, outgoing_flows);
        }

        if (selected_flow) {
            state.current_element = selected_flow->target_ref;
            saveState(instance_id, state);
            executeElement(instance_id, state.current_element, state);
        }
        else {
            throw std::runtime_error("No valid outgoing sequence flow from exclusive gateway");
        }
    }

    void ProcessExecutor::handleEndEvent(const std::string& instance_id, const EndEvent& end_event, ExecutionState& state) {
        log("Process instance " + instance_id + " completed");
        db_.completeProcessInstance(instance_id);
    }

    void ProcessExecutor::saveState(const std::string& instance_id, ExecutionState& state) {
        //Копируем состояние
        std::string process_id = state.process_id;
        std::string current_element = state.current_element;
        std::map<std::string, std::string> variables = state.variables;
        //Состояние сохраняем
        lastState_ = std::make_unique<ExecutionState>(std::move(state));
        //Сохраняем в БД
        db_.saveProcessInstance(instance_id, process_id, current_element, variables);
    }

    ExecutionState ProcessExecutor::loadState(const std::string& instance_id) {
        ExecutionState result;
        const db::Database::ProcessInstance processData = db_.loadProcessInstance(instance_id);
        result.variables = processData.variables;
        result.process_id = processData.process_id;
        result.current_element = processData.current_element;
        return result;
    }

    void ProcessExecutor::log(const std::string& message) {
        // Implementation would use a proper logging library
        std::cout << "[BPMN Engine] " << message << std::endl;
    }

    void ProcessExecutor::handleError(const std::string& instance_id, const std::string& error_message, ExecutionState& state) {
        log("ERROR: " + error_message);
        state.variables["last_error"] = error_message;
        db_.saveError(instance_id, error_message);

        // Could implement error handling flow here
    }

    // Helper methods
    std::vector<FlowElement*> ProcessExecutor::getOutgoingElements(const std::string& element_id, const std::string& process_id) {
        const Process* process = getProcessDefinition(process_id);
        if (!process) return {};

        auto flows = process->getOutgoingFlows(element_id);
        std::vector<FlowElement*> elements;
        for (const auto& flow : flows) {
            if (auto element = process->getElement(flow->target_ref)) {
                elements.push_back(element);
            }
        }
        return elements;
    }

    std::shared_ptr<SequenceFlow> ProcessExecutor::getDefaultFlow(const ExclusiveGateway& gateway, const std::vector<std::shared_ptr<SequenceFlow>>& flows) {
        if (!gateway.default_flow.empty()) {
            for (const auto& flow : flows) {
                if (flow->getId() == gateway.default_flow) {
                    return flow;
                }
            }
        }
    }

    const Process* ProcessExecutor::getProcessDefinition(const std::string& process_id) {
        // In a real implementation, you'd want to:
        // 1. Check a cache of loaded processes
        // 2. Load from database/file system if not cached
        // 3. Parse and validate the process

        // Simple implementation - would need proper process storage in real system
        static std::map<std::string, std::unique_ptr<Process>> process_cache_;

        auto it = process_cache_.find(process_id);
        if (it != process_cache_.end()) {
            return it->second.get();
        }

        // In a real system, you'd load this from persistent storage
        // For now we'll return nullptr to indicate not found
        return nullptr;
    }

    std::string ProcessExecutor::generate_uuid() {
        return boost::uuids::to_string(uuid_generator_());
    }

} // namespace bpmn