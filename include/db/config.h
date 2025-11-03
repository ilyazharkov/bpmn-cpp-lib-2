// include/bpmn/config.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace db {

    class DatabaseConfig {
    public:
        static DatabaseConfig fromJson(const std::string& configPath);
        static DatabaseConfig fromEnvironment();

        std::string getConnectionString() const;

        // Геттеры
        std::string getHost() const { return host; }
        std::string getPort() const { return port; }
        std::string getDatabase() const { return database; }
        std::string getUsername() const { return username; }
        std::string getPassword() const { return password; }

        // Сеттеры
        void setHost(const std::string& h) { host = h; }
        void setPort(const std::string& p) { port = p; }
        void setDatabase(const std::string& db) { database = db; }
        void setUsername(const std::string& user) { username = user; }
        void setPassword(const std::string& pass) { password = pass; }

    private:
        std::string host = "localhost";
        std::string port = "5432";
        std::string database = "bpmn_engine";
        std::string username = "postgres";
        std::string password = "password";
    };

} // namespace bpmns