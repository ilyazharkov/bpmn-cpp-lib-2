#ifndef BPMN_MODEL_H
#define BPMN_MODEL_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../bpmn/services/abstractService.h"

namespace bpmn {

    // Forward declarations
    class SequenceFlow;
    class StartEvent;
    class EndEvent;
    class UserTask;
    class ParallelGateway;
    class ExclusiveGateway;

    class Process {
    public:
        // Конструкторы
        Process() = default;
        Process(const std::string& id, const std::string& name)
            : id(id), name(name) {
        }

        // Helper methods
        FlowElement* getElement(const std::string& id) const;
        void addElement(const std::unique_ptr<FlowElement> element);
        void addElement(const std::shared_ptr<FlowElement> element);
        void addSequenceFlow(const std::string& id, const std::string& name, const std::string& sourceRef, const std::string& targetRef);
        StartEvent* getStartEvent() const;
        std::vector<services::ServiceTask*> getServiceTasks() const;
        std::vector<UserTask*> getUserTasks() const;
        bool validate() const;
        const std::vector<std::shared_ptr<SequenceFlow>> getOutgoingFlows(const std::string& element_id) const;

        //Getters методы
        const std::string getId() const;
        const std::string getStartEventId() const;

        // Сеттеры
        void setId(const std::string& newId) { id = newId; }
        void setName(const std::string& newName) { name = newName; }
        void setStartEventId(const std::string& startId) { start_event_id = startId; }

    private:
        std::string id;
        std::string name;
        std::vector<std::unique_ptr<FlowElement>> elements;
        std::unordered_map<std::string, std::shared_ptr<FlowElement>> elementsById;
        std::vector<std::unique_ptr<SequenceFlow>> flows;
        std::unordered_map<std::string, std::shared_ptr<SequenceFlow>> flowsById;
        std::unordered_map<std::string, std::vector<std::shared_ptr<SequenceFlow>>> outgoingFlows;
        std::unordered_map<std::string, std::vector<std::shared_ptr<SequenceFlow>>> incomingFlows;
        std::string start_event_id;
        std::map<std::string, FlowElement*> element_map;
    };

    class SequenceFlow : public FlowElement {
    public:
        std::string source_ref;
        std::string target_ref;
        std::string condition_expression;

        // Конструктор
        SequenceFlow(const std::string& id, const std::string& name,
            const std::string& source_ref, const std::string& target_ref)
            : FlowElement(id, name), source_ref(source_ref), target_ref(target_ref) {
        }
    };

    class StartEvent : public FlowElement {
    public:
        using FlowElement::FlowElement; // Наследуем конструктор
        ~StartEvent() override = default;
        // Start event specific properties
    };

    class EndEvent : public FlowElement {
    public:
        using FlowElement::FlowElement;
        ~EndEvent() override = default;
        // End event specific properties
    };

    class UserTask : public FlowElement {
    public:
        using FlowElement::FlowElement;

        std::string form_key;
        std::string assignee;
        std::map<std::string, std::string> form_fields;

        ~UserTask() override = default;
    };

    class ParallelGateway : public FlowElement {
    public:
        using FlowElement::FlowElement;
        ~ParallelGateway() override = default;
        // Parallel gateway specific properties
    };

    class ExclusiveGateway : public FlowElement {
    public:
        using FlowElement::FlowElement;

        std::string default_flow;  // ID of default sequence flow

        ~ExclusiveGateway() override = default;
    };

} // namespace bpmn

#endif // BPMN_MODEL_H