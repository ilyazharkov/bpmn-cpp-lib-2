#include "./bpmn/model.h"
#include "./bpmn/services/abstractService.h"
#include <algorithm>

namespace bpmn {

    FlowElement* Process::getElement(const std::string& id) const {
        auto it = element_map.find(id);
        if (it != element_map.end()) {
            return it->second;
        }
        return nullptr;
    }
    // Версия с unique_ptr (предпочтительная)
    void Process::addElement(std::unique_ptr<FlowElement> element) {
        if (!element) {
            throw std::invalid_argument("Cannot add null element to process");
        }

        std::string elementId = element->getId();

        // Проверяем дубликаты
        if (elementsById.find(elementId) != elementsById.end()) {
            throw std::runtime_error("Element with id " + elementId + " already exists in process");
        }

        // Создаем shared_ptr из unique_ptr для хранения в карте
        std::shared_ptr<FlowElement> sharedElement = std::move(element);

        // Сохраняем unique_ptr в вектор
        elements.push_back(std::unique_ptr<FlowElement>(sharedElement.get()));

        // Сохраняем shared_ptr в карту для быстрого доступа
        elementsById[elementId] = sharedElement;
    }
    // Версия с shared_ptr (предпочтительная)
    void Process::addElement(std::shared_ptr<FlowElement> element) {
        if (!element) {
            throw std::invalid_argument("Cannot add null element to process");
        }

        std::string elementId = element->getId();

        // Проверяем дубликаты
        if (elementsById.find(elementId) != elementsById.end()) {
            throw std::runtime_error("Element with id " + elementId + " already exists in process");
        }

        // Создаем unique_ptr из shared_ptr
        std::unique_ptr<FlowElement> uniqueElement = std::unique_ptr<FlowElement>(element.get());

        // Сохраняем unique_ptr в вектор
        elements.push_back(std::move(uniqueElement));

        // Сохраняем shared_ptr в карту
        elementsById[elementId] = element;
    }
    
    void Process::addSequenceFlow(const std::string& id, const std::string& name, const std::string& sourceRef, const std::string& targetRef) {
        // Проверяем существование элементов
        auto sourceElement = getElement(sourceRef);
        auto targetElement = getElement(targetRef);

        if (!sourceElement) {
            throw std::runtime_error("Source element not found: " + sourceRef);
        }
        if (!targetElement) {
            throw std::runtime_error("Target element not found: " + targetRef);
        }

        // Проверяем дубликаты потоков
        if (flowsById.find(id) != flowsById.end()) {
            throw std::runtime_error("Sequence flow with id " + id + " already exists");
        }

        // Создаем unique_ptr для потока
        auto sequenceFlow = std::make_unique<SequenceFlow>(id, name, sourceRef, targetRef);

        // Создаем shared_ptr для карт доступа
        std::shared_ptr<SequenceFlow> sharedFlow = std::move(sequenceFlow);

        // Сохраняем unique_ptr в вектор (передаем владение обратно)
        flows.push_back(std::unique_ptr<SequenceFlow>(sharedFlow.get()));

        // Сохраняем shared_ptr в карту для быстрого доступа
        flowsById[id] = sharedFlow;

        // Добавляем в карты для быстрого поиска по элементам
        outgoingFlows[sourceRef].push_back(sharedFlow);
        incomingFlows[targetRef].push_back(sharedFlow);
    }


    const std::vector<std::shared_ptr<SequenceFlow>> Process::getOutgoingFlows(const std::string& elementId) const {
        auto it = outgoingFlows.find(elementId);
        return (it != outgoingFlows.end()) ? it->second : std::vector<std::shared_ptr<SequenceFlow>>{};
    }
    const std::string Process::getId() const {
        return id;
    }
    const std::string Process::getStartEventId() const {
        return start_event_id;
    }
    // Helper functions
    namespace {

        template<typename T>
        T* findElementById(const std::vector<std::unique_ptr<FlowElement>>& elements, const std::string& id) {
            auto it = std::find_if(elements.begin(), elements.end(),
                [&id](const std::unique_ptr<FlowElement>& elem) {
                    return elem->id == id && dynamic_cast<T*>(elem.get()) != nullptr;
                });
            return it != elements.end() ? dynamic_cast<T*>(it->get()) : nullptr;
        }

    } // anonymous namespace

    StartEvent* Process::getStartEvent() const {
        return start_event_id.empty() ? nullptr : dynamic_cast<StartEvent*>(getElement(start_event_id));
    }

    std::vector<UserTask*> Process::getUserTasks() const {
        std::vector<UserTask*> tasks;
        for (const auto& element : elements) {
            if (auto task = dynamic_cast<UserTask*>(element.get())) {
                tasks.push_back(task);
            }
        }
        return tasks;
    }

    std::vector<services::ServiceTask*> Process::getServiceTasks() const {
        std::vector<services::ServiceTask*> tasks;
        for (const auto& element : elements) {
            if (auto task = dynamic_cast<services::ServiceTask*>(element.get())) {
                tasks.push_back(task);
            }
        }
        return tasks;
    }

    bool Process::validate() const {
        // Basic validation checks
        if (start_event_id.empty()) {
            return false;
        }

        if (getStartEvent() == nullptr) {
            return false;
        }

        // Check all sequence flows reference existing elements
        for (const auto& flow : flows) {
            if (getElement(flow->source_ref) == nullptr ||
                getElement(flow->target_ref) == nullptr) {
                return false;
            }
        }

        return true;
    }

} // namespace bpmn