// src/config.cpp
#include "db/config.h"
#include <fstream>

namespace db {

    DatabaseConfig DatabaseConfig::fromJson(const std::string& configPath) {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + configPath);
        }

        nlohmann::json config;
        file >> config;

        DatabaseConfig dbConfig;
        dbConfig.host = config.value("database_host", "localhost");
        dbConfig.port = config.value("database_port", "5432");
        dbConfig.database = config.value("database_name", "bpmn_engine");
        dbConfig.username = config.value("database_user", "postgres");
        dbConfig.password = config.value("database_password", "password");

        return dbConfig;
    }

    DatabaseConfig DatabaseConfig::fromEnvironment() {
        DatabaseConfig config;
        // Чтение из переменных окружения
        if (const char* host = std::getenv("BPMN_DB_HOST")) config.host = host;
        if (const char* port = std::getenv("BPMN_DB_PORT")) config.port = port;
        if (const char* db = std::getenv("BPMN_DB_NAME")) config.database = db;
        if (const char* user = std::getenv("BPMN_DB_USER")) config.username = user;
        if (const char* pass = std::getenv("BPMN_DB_PASS")) config.password = pass;

        return config;
    }

    std::string DatabaseConfig::getConnectionString() const {
        return "host=" + host +
            " port=" + port +
            " dbname=" + database +
            " user=" + username +
            " password=" + password;
    }

} // namespace bpmn