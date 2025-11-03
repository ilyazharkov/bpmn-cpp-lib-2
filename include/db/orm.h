// db/orm.h
#ifndef DB_ORM_H
#define DB_ORM_H

#include <libpq-fe.h>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include "config.h"

namespace db {

    class Database {
    public:
        Database();
        explicit Database(const std::string& connection_string);
        ~Database();

        // Запрещаем копирование
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

        // Process instance management
        void saveProcessInstance(
            const std::string& instance_id,
            const std::string& process_id,
            const std::string& current_element,
            const std::map<std::string, std::string>& variables
        );

        struct ProcessInstance {
            std::string process_id;
            std::string current_element;
            std::map<std::string, std::string> variables;
        };

        ProcessInstance loadProcessInstance(const std::string& instance_id);
        void completeProcessInstance(const std::string& instance_id);

        // User task management
        void saveUserTask(
            const std::string& instance_id,
            const std::string& task_id,
            const std::string& form_key,
            const std::map<std::string, std::string>& variables
        );

        // Error handling
        void saveError(const std::string& instance_id, const std::string& error_message);

        // Process definition storage
        std::string loadProcessDefinition(const std::string& process_id);
        PGconn* getConnection() const;
        nlohmann::json getFormById(const std::string formId) const;

    private:
        PGconn* conn_;
        std::string last_error_;

        void initializeSchema();
        std::map<std::string, std::string> loadVariables(const std::string& instance_id);

        // Вспомогательные методы
        void executeQuery(const std::string& query);
        void executeQueryWithParams(const std::string& query, const std::vector<const char*>& params);
        std::vector<std::vector<std::string>> executeQueryWithResults(const std::string& query,
            const std::vector<const char*>& params = {});
        bool tableExists(const std::string& table_name);
        void checkConnection();
    };

} // namespace db

#endif // DB_ORM_H