#pragma once
#include "service.h"  // Only depends on the interface
#include <map>
#include <memory>
#include <functional>
#include <future>

namespace bpmn {
    namespace services {
        class ServiceFactory {
        public:
            static ServiceFactory& ServiceFactory::instance() {
                static ServiceFactory instance;
                return instance;
            }

            // Template method must be in header
            template<typename T>
            void registerService(const std::string& name) {
                services_[name] = []() -> std::unique_ptr<IService> {
                    return std::make_unique<T>();
                    };
            }

            std::future<json> ServiceFactory::executeService(
                const std::string& name,
                const ExecutionState& state)
            {
                auto it = services_.find(name);
                if (it == services_.end()) {
                    throw std::runtime_error("Service not registered: " + name);
                }

                auto service = it->second();
                //return std::async(std::launch::async,
                //    [service = std::move(service), state]() mutable {
                //        return service->execute(state);
                //    });
            }

        private:
            std::map<std::string, std::function<std::unique_ptr<IService>()>> services_;
            ServiceFactory() = default;  // Private constructor for singleton
        };
    }
}