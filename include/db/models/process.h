#ifndef PROCESS_DB_H
#define PROCESS_DB_H

#include <memory>
#include <string>
#include <vector>
#include "../orm.h"

namespace db {

    class FormDb;  // Forward declaration

    class ProcessDb {
    public:
        ProcessDb(std::string id, std::string description = "", std::string xmlContent = "");

        // CRUD operations
        static std::unique_ptr<ProcessDb> findById(const std::string& id, Database& db);
        static std::unique_ptr<ProcessDb> create(const ProcessDb& record, Database& db);
        static std::vector<std::unique_ptr<ProcessDb>> batchCreate(const std::vector<ProcessDb>& records, Database& db);
        static int update(const ProcessDb& record, Database& db);
        static int batchUpdate(const std::vector<ProcessDb>& records, Database& db);
        static int deleteById(const std::string& id, Database& db);

        // Relations
        int loadForms(Database& db);

        // Getters
        std::string getId() const { return id; }
        std::string getDescription() const { return description; }
        std::string getXmlContent() const { return xmlContent; }
        const std::vector<std::unique_ptr<FormDb>>& getForms() const { return forms; }

        // Setters
        void setDescription(const std::string& desc) { description = desc; }
        void setXmlContent(const std::string& xml) { xmlContent = xml; }

    private:
        std::string id;
        std::string description;
        std::string xmlContent;
        std::vector<std::unique_ptr<FormDb>> forms;

    };

} // namespace db

#endif // PROCESS_DB_H