// db/orm.cpp
#include "db/orm.h"
#include <stdexcept>
#include <iostream>

namespace db {

    Database::Database() {
        auto dbConfig = DatabaseConfig::fromJson("./config.json");
        std::string connString = dbConfig.getConnectionString();
        conn_ = PQconnectdb(connString.c_str());

        if (PQstatus(conn_) != CONNECTION_OK) {
            last_error_ = PQerrorMessage(conn_);
            throw std::runtime_error("Database connection failed: " + last_error_);
        }

        initializeSchema();
    }

    Database::Database(const std::string& connection_string) {
        conn_ = PQconnectdb(connection_string.c_str());

        if (PQstatus(conn_) != CONNECTION_OK) {
            last_error_ = PQerrorMessage(conn_);
            throw std::runtime_error("Database connection failed: " + last_error_);
        }

        initializeSchema();
    }

    Database::~Database() {
        if (conn_) {
            PQfinish(conn_);
        }
    }

    void Database::checkConnection() {
        if (PQstatus(conn_) != CONNECTION_OK) {
            throw std::runtime_error("Database connection is broken");
        }
    }

    void Database::executeQuery(const std::string& query) {
        checkConnection();

        PGresult* res = PQexec(conn_, query.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
            last_error_ = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Query failed: " + last_error_ + "\nQuery: " + query);
        }
        PQclear(res);
    }

    void Database::executeQueryWithParams(const std::string& query, const std::vector<const char*>& params) {
        checkConnection();

        PGresult* res = PQexecParams(conn_, query.c_str(),
            params.size(), NULL, params.data(),
            NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
            last_error_ = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Parameterized query failed: " + last_error_);
        }
        PQclear(res);
    }

    std::vector<std::vector<std::string>> Database::executeQueryWithResults(const std::string& query,
        const std::vector<const char*>& params) {
        checkConnection();

        PGresult* res = PQexecParams(conn_, query.c_str(),
            params.size(), NULL, params.data(),
            NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            last_error_ = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Query failed: " + last_error_);
        }

        std::vector<std::vector<std::string>> results;
        int rows = PQntuples(res);
        int cols = PQnfields(res);

        for (int i = 0; i < rows; i++) {
            std::vector<std::string> row;
            for (int j = 0; j < cols; j++) {
                row.push_back(PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "");
            }
            results.push_back(row);
        }

        PQclear(res);
        return results;
    }

    bool Database::tableExists(const std::string& table_name) {
        try {
            std::vector<const char*> params = { table_name.c_str() };
            auto result = executeQueryWithResults(
                "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = $1)",
                params
            );
            return !result.empty() && result[0][0] == "t";
        }
        catch (...) {
            return false;
        }
    }

    void Database::initializeSchema() {
        // Process instances table
        executeQuery(R"(
            CREATE TABLE IF NOT EXISTS process_instances (
                id VARCHAR(36) PRIMARY KEY,
                process_id VARCHAR(255) NOT NULL,
                current_element VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                completed_at TIMESTAMP,
                status VARCHAR(20) DEFAULT 'RUNNING'
            )
        )");

        // Process variables table
        executeQuery(R"(
            CREATE TABLE IF NOT EXISTS process_variables (
                id SERIAL PRIMARY KEY,
                instance_id VARCHAR(36) NOT NULL REFERENCES process_instances(id) ON DELETE CASCADE,
                var_key VARCHAR(255) NOT NULL,
                var_value TEXT,
                UNIQUE(instance_id, var_key)
            )
        )");

        // User tasks table
        executeQuery(R"(
            CREATE TABLE IF NOT EXISTS user_tasks (
                id SERIAL PRIMARY KEY,
                instance_id VARCHAR(36) NOT NULL REFERENCES process_instances(id),
                task_id VARCHAR(255) NOT NULL,
                form_key VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                completed_at TIMESTAMP,
                status VARCHAR(20) DEFAULT 'PENDING'
            )
        )");

        // Process errors table
        executeQuery(R"(
            CREATE TABLE IF NOT EXISTS process_errors (
                id SERIAL PRIMARY KEY,
                instance_id VARCHAR(36) NOT NULL REFERENCES process_instances(id),
                error_message TEXT NOT NULL,
                occurred_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )");

        // Process definitions table
        executeQuery(R"(
            CREATE TABLE IF NOT EXISTS process_definitions (
                id VARCHAR(255) PRIMARY KEY,
                bpmn_xml TEXT NOT NULL,
                version INTEGER NOT NULL,
                deployed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )");
    }

    void Database::saveProcessInstance(
        const std::string& instance_id,
        const std::string& process_id,
        const std::string& current_element,
        const std::map<std::string, std::string>& variables
    ) {
        // Начинаем транзакцию
        executeQuery("BEGIN");

        try {
            // Insert or update process instance
            std::vector<const char*> instance_params = {
                instance_id.c_str(),
                process_id.c_str(),
                current_element.c_str()
            };
            executeQueryWithParams(
                "INSERT INTO process_instances (id, process_id, current_element) "
                "VALUES ($1, $2, $3) "
                "ON CONFLICT (id) DO UPDATE SET "
                "current_element = $3, status = 'RUNNING', completed_at = NULL",
                instance_params
            );

            // Delete existing variables
            std::vector<const char*> delete_params = { instance_id.c_str() };
            executeQueryWithParams(
                "DELETE FROM process_variables WHERE instance_id = $1",
                delete_params
            );

            // Insert new variables
            for (const auto& [key, value] : variables) {
                std::vector<const char*> var_params = {
                    instance_id.c_str(),
                    key.c_str(),
                    value.c_str()
                };
                executeQueryWithParams(
                    "INSERT INTO process_variables (instance_id, var_key, var_value) "
                    "VALUES ($1, $2, $3)",
                    var_params
                );
            }

            executeQuery("COMMIT");
        }
        catch (const std::exception& e) {
            executeQuery("ROLLBACK");
            throw;
        }
    }

    Database::ProcessInstance Database::loadProcessInstance(const std::string& instance_id) {
        ProcessInstance result;

        // Load instance metadata
        std::vector<const char*> params = { instance_id.c_str() };
        auto instance_result = executeQueryWithResults(
            "SELECT process_id, current_element FROM process_instances "
            "WHERE id = $1 AND status = 'RUNNING'",
            params
        );

        if (instance_result.empty()) {
            throw std::runtime_error("Process instance not found or completed: " + instance_id);
        }

        result.process_id = instance_result[0][0];
        result.current_element = instance_result[0][1];
        result.variables = loadVariables(instance_id);

        return result;
    }

    void Database::completeProcessInstance(const std::string& instance_id) {
        std::vector<const char*> params = { instance_id.c_str() };
        executeQueryWithParams(
            "UPDATE process_instances SET "
            "status = 'COMPLETED', "
            "completed_at = CURRENT_TIMESTAMP "
            "WHERE id = $1",
            params
        );
    }

    void Database::saveUserTask(
        const std::string& instance_id,
        const std::string& task_id,
        const std::string& form_key,
        const std::map<std::string, std::string>& variables
    ) {
        executeQuery("BEGIN");

        try {
            // Save task metadata
            std::vector<const char*> task_params = {
                instance_id.c_str(),
                task_id.c_str(),
                form_key.c_str()
            };
            executeQueryWithParams(
                "INSERT INTO user_tasks (instance_id, task_id, form_key) "
                "VALUES ($1, $2, $3)",
                task_params
            );

            // Save task-specific variables
            for (const auto& [key, value] : variables) {
                std::string prefixed_key = "task_" + task_id + "_" + key;
                std::vector<const char*> var_params = {
                    instance_id.c_str(),
                    prefixed_key.c_str(),
                    value.c_str()
                };
                executeQueryWithParams(
                    "INSERT INTO process_variables (instance_id, var_key, var_value) "
                    "VALUES ($1, $2, $3)",
                    var_params
                );
            }

            executeQuery("COMMIT");
        }
        catch (const std::exception& e) {
            executeQuery("ROLLBACK");
            throw;
        }
    }

    void Database::saveError(const std::string& instance_id, const std::string& error_message) {
        std::vector<const char*> params = {
            instance_id.c_str(),
            error_message.c_str()
        };
        executeQueryWithParams(
            "INSERT INTO process_errors (instance_id, error_message) "
            "VALUES ($1, $2)",
            params
        );
    }

    PGconn* Database::getConnection() const {
        return conn_;
    }

    std::string Database::loadProcessDefinition(const std::string& process_id) {
        std::vector<const char*> params = { process_id.c_str() };
        auto result = executeQueryWithResults(
            "SELECT bpmn_xml FROM process_definitions "
            "WHERE id = $1 ORDER BY version DESC LIMIT 1",
            params
        );

        if (result.empty()) {
            throw std::runtime_error("Process definition not found: " + process_id);
        }

        return result[0][0];
    }

    std::map<std::string, std::string> Database::loadVariables(const std::string& instance_id) {
        std::map<std::string, std::string> variables;

        std::vector<const char*> params = { instance_id.c_str() };
        auto result = executeQueryWithResults(
            "SELECT var_key, var_value FROM process_variables "
            "WHERE instance_id = $1",
            params
        );

        for (const auto& row : result) {
            variables[row[0]] = row[1];
        }

        return variables;
    }

    nlohmann::json Database::getFormById(const std::string formId) const {
        return "none";
    }

} // namespace db