#include "./bpmn/services/abstractService.h"
#include "./bpmn/executor.h"
#include <stdexcept>
#include <utility>
#include <future>
#include <mutex>

namespace bpmn {
    namespace services {

        ServiceTask::~ServiceTask() = default;

        std::future<json> ServiceTask::execute(ExecutionState& state) {
            // Capture state by move (assumes ExecutionState is movable)
            ExecutionState stateCopy = std::move(state);

            // Use a mutex if class_name, expression, or topic can be modified concurrently
            static std::mutex mtx;

            return std::async(std::launch::async,
                [this, stateCopy = std::move(stateCopy)]() -> json {
                    try {
                        json result;

                        // Lock if needed (uncomment if thread safety is required)
                        // std::lock_guard<std::mutex> lock(mtx);

                        if (!class_name.empty()) {
                            // Java delegate logic
                            result["output"] = "Java delegate executed";
                        }
                        else if (!expression.empty()) {
                            // Expression evaluation logic
                            result["output"] = "Expression evaluated";
                        }
                        else if (!topic.empty()) {
                            // External service call logic
                            result["output"] = "External service called";
                        }
                        else {
                            throw std::runtime_error("No execution method specified (class_name, expression, or topic missing)");
                        }

                        return result;
                    }
                    catch (const std::exception& e) {
                        return json{ {"error", e.what()} };
                    }
                });
        }
    }
}