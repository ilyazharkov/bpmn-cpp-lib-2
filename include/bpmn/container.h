#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>

namespace bpmn {

    class Container {
    public:
        // Define pointer and array types for recursive use
        using Ptr = std::unique_ptr<Container>;
        using Array = std::vector<Ptr>;

        // Value variant now uses Ptr and Array to handle incomplete type
        using Value = std::variant<
            int,
            double,
            bool,
            std::string,
            Ptr,        // Single nested container
            Array       // Array of containers
        >;

        // Check if field exists
        bool hasField(const std::string& name) const {
            return fields_.find(name) != fields_.end();
        }

        // Generic setter for basic types
        template <typename T>
        void setField(const std::string& name, T&& value) {
            fields_[name] = std::forward<T>(value);
        }

        // Set nested container (moves Container into unique_ptr)
        void setContainerField(const std::string& name, Container container) {
            fields_[name] = std::make_unique<Container>(std::move(container));
        }

        // Set array from vector of Containers (converts to vector<unique_ptr>)
        void setContainerArrayField(const std::string& name, std::vector<Container> array) {
            Array ptrArray;
            for (auto& cont : array) {
                ptrArray.push_back(std::make_unique<Container>(std::move(cont)));
            }
            fields_[name] = std::move(ptrArray);
        }

        // Set array directly from vector of unique_ptr
        void setContainerArrayField(const std::string& name, Array array) {
            fields_[name] = std::move(array);
        }

        // Get field by exact type
        template <typename T>
        T& getField(const std::string& name) {
            auto it = fields_.find(name);
            if (it == fields_.end()) {
                throw std::runtime_error("Field not found: " + name);
            }
            return std::get<T>(it->second);
        }

        template <typename T>
        const T& getField(const std::string& name) const {
            auto it = fields_.find(name);
            if (it == fields_.end()) {
                throw std::runtime_error("Field not found: " + name);
            }
            return std::get<T>(it->second);
        }

        // Helper to get nested container (dereferences pointer)
        Container& getContainerField(const std::string& name) {
            return *getField<Ptr>(name);
        }

        const Container& getContainerField(const std::string& name) const {
            return *getField<Ptr>(name);
        }

        // Helper to get array of containers
        Array& getContainerArrayField(const std::string& name) {
            return getField<Array>(name);
        }

        const Array& getContainerArrayField(const std::string& name) const {
            return getField<Array>(name);
        }

    private:
        std::unordered_map<std::string, Value> fields_;
    };
}