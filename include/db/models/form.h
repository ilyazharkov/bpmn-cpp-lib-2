#ifndef FORM_DB_H
#define FORM_DB_H

#include <string>
#include <memory>
#include "../orm.h"

namespace db {

    class FormDb {
    public:
        FormDb(std::string id, std::string processId, std::string description = "",
            std::string schema = "", std::string uischema = "");

        // Getters
        std::string getId() const { return id; }
        std::string getProcessId() const { return processId; }
        std::string getDescription() const { return description; }
        std::string getSchema() const { return schema; }
        std::string getUISchema() const { return uischema; }

        // Setters
        void setDescription(const std::string& desc) { description = desc; }
        void setSchema(const std::string& sch) { schema = sch; }
        void setUISchema(const std::string& ui) { uischema = ui; }

        // CRUD operations
        static std::unique_ptr<FormDb> findById(const std::string& id, const Database& db);
        static int create(const FormDb& record, const Database& db);
        static int batchCreate(const std::vector<FormDb>& records, const Database& db);
        static int update(const FormDb& record, const Database& db);
        static int batchUpdate(const std::vector<FormDb>& records, const Database& db);
        static int deleteById(const std::string& id, const Database& db);

    private:
        std::string id;
        std::string processId;
        std::string description;
        std::string schema;
        std::string uischema;
    };

} // namespace db

#endif // FORM_DB_H