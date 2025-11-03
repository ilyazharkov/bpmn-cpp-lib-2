#ifndef FLOW_ABSTRACT_H
#define FLOW_ABSTRACT_H

#include <string>
namespace bpmn {
    class FlowElement {
    public:
        FlowElement(const std::string& id, const std::string& name)
            : id(id), name(name) {
        }

        virtual ~FlowElement() = default;

        std::string getId() const { return id; }
        std::string getName() const { return name; }

    private:
        std::string id;
        std::string name;
    };
}
#endif