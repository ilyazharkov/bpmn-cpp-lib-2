#pragma once

#include <string>
#include <map>
#include <vector>
#include <future>

namespace bpmn {
    class ProcessExecutor; // Forward declaration

    struct ExecutionState {
        std::string process_id;
        std::string current_element;
        std::map<std::string, std::string> variables;
        std::vector<std::future<void>> parallel_tasks;
        bool isPaused = false;
        bool isCompleted = false;
        bool isStarted = false;

        // Rule of Five implementation:
        ExecutionState() = default;
        ~ExecutionState() = default;

        // Delete copy operations (critical because of std::future members)
        ExecutionState(const ExecutionState&) = delete;
        ExecutionState& operator=(const ExecutionState&) = delete;

        // Default move operations (sufficient in this case)
        ExecutionState(ExecutionState&&) noexcept = default;
        ExecutionState& operator=(ExecutionState&&) noexcept = default;
    };
}