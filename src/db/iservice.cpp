#include "bpmn/services/service.h"

namespace bpmn {
    namespace services {
        // Реализация конструктора
        IService::IService(const std::string& id, const std::string& name)
            : FlowElement(id, name) {
        }
    }
}