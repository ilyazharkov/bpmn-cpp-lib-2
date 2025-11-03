#include "db/models/Form.h"
#include <libpq-fe.h>
#include <utility>
#include <vector>
#include <stdexcept>
#include <sstream>

namespace db {

    // Constructor implementation
    FormDb::FormDb(std::string id, std::string processId, std::string description,
        std::string schema, std::string uischema)
        : id(std::move(id)),
        processId(std::move(processId)),
        description(std::move(description)),
        schema(std::move(schema)),
        uischema(std::move(uischema)) {
    }

    // CRUD Operations implementation

    std::unique_ptr<FormDb> FormDb::findById(const std::string& id, const Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[1] = { id.c_str() };

        PGresult* res = PQexecParams(conn,
            "SELECT id, \"processId\", description, schema, uischema "
            "FROM process_forms WHERE id = $1",
            1, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to find form: " + error);
        }

        if (PQntuples(res) == 0) {
            PQclear(res);
            throw std::runtime_error("Form not found with id: " + id);
        }

        // Extract data from result
        std::unique_ptr<FormDb> form = std::make_unique<FormDb>(
            PQgetvalue(res, 0, 0) ? PQgetvalue(res, 0, 0) : "",
            PQgetvalue(res, 0, 1) ? PQgetvalue(res, 0, 1) : "",
            PQgetvalue(res, 0, 2) ? PQgetvalue(res, 0, 2) : "",
            PQgetvalue(res, 0, 3) ? PQgetvalue(res, 0, 3) : "",
            PQgetvalue(res, 0, 4) ? PQgetvalue(res, 0, 4) : ""
        );

        PQclear(res);
        return form;
    }

    int FormDb::create(const FormDb& record, const Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[5] = {
            record.id.c_str(),
            record.processId.c_str(),
            record.description.c_str(),
            record.schema.c_str(),
            record.uischema.c_str()
        };

        PGresult* res = PQexecParams(conn,
            "INSERT INTO process_forms(id, \"processId\", description, schema, uischema) "
            "VALUES($1, $2, $3, $4, $5)",
            5, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to create form: " + error);
        }

        int affected_rows = atoi(PQcmdTuples(res));
        PQclear(res);
        return affected_rows;
    }

    int FormDb::batchCreate(const std::vector<FormDb>& records, const Database& db) {
        if (records.empty()) return 0;

        PGconn* conn = db.getConnection();

        // Начинаем транзакцию
        PGresult* begin_res = PQexec(conn, "BEGIN");
        if (PQresultStatus(begin_res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(begin_res);
            PQclear(begin_res);
            throw std::runtime_error("Failed to begin transaction: " + error);
        }
        PQclear(begin_res);

        try {
            int created_count = 0;
            for (const auto& record : records) {
                const char* params[5] = {
                    record.id.c_str(),
                    record.processId.c_str(),
                    record.description.c_str(),
                    record.schema.c_str(),
                    record.uischema.c_str()
                };

                PGresult* res = PQexecParams(conn,
                    "INSERT INTO process_forms(id, \"processId\", description, schema, uischema) "
                    "VALUES($1, $2, $3, $4, $5)",
                    5, NULL, params, NULL, NULL, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    std::string error = PQresultErrorMessage(res);
                    PQclear(res);
                    throw std::runtime_error("Batch create forms failed: " + error);
                }

                created_count += atoi(PQcmdTuples(res));
                PQclear(res);
            }

            // Коммитим транзакцию
            PGresult* commit_res = PQexec(conn, "COMMIT");
            if (PQresultStatus(commit_res) != PGRES_COMMAND_OK) {
                std::string error = PQresultErrorMessage(commit_res);
                PQclear(commit_res);
                throw std::runtime_error("Failed to commit transaction: " + error);
            }
            PQclear(commit_res);

            return created_count;
        }
        catch (const std::exception&) {
            // Откатываем транзакцию при ошибке
            PGresult* rollback_res = PQexec(conn, "ROLLBACK");
            PQclear(rollback_res);
            throw;
        }
    }

    int FormDb::update(const FormDb& record, const Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[5] = {
            record.id.c_str(),
            record.processId.c_str(),
            record.description.c_str(),
            record.schema.c_str(),
            record.uischema.c_str()
        };

        PGresult* res = PQexecParams(conn,
            "UPDATE process_forms SET "
            "\"processId\" = $2, description = $3, schema = $4, uischema = $5 "
            "WHERE id = $1",
            5, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to update form: " + error);
        }

        int affected_rows = atoi(PQcmdTuples(res));
        PQclear(res);
        return affected_rows;
    }

    int FormDb::batchUpdate(const std::vector<FormDb>& records, const Database& db) {
        if (records.empty()) {
            return 0;
        }

        PGconn* conn = db.getConnection();

        // Начинаем транзакцию
        PGresult* begin_res = PQexec(conn, "BEGIN");
        if (PQresultStatus(begin_res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(begin_res);
            PQclear(begin_res);
            throw std::runtime_error("Failed to begin transaction: " + error);
        }
        PQclear(begin_res);

        try {
            int updated_count = 0;
            for (const auto& record : records) {
                const char* params[5] = {
                    record.id.c_str(),
                    record.processId.c_str(),
                    record.description.c_str(),
                    record.schema.c_str(),
                    record.uischema.c_str()
                };

                PGresult* res = PQexecParams(conn,
                    "UPDATE process_forms SET "
                    "\"processId\" = $2, description = $3, schema = $4, uischema = $5 "
                    "WHERE id = $1",
                    5, NULL, params, NULL, NULL, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    std::string error = PQresultErrorMessage(res);
                    PQclear(res);
                    throw std::runtime_error("Batch update failed: " + error);
                }

                updated_count += atoi(PQcmdTuples(res));
                PQclear(res);
            }

            // Коммитим транзакцию
            PGresult* commit_res = PQexec(conn, "COMMIT");
            if (PQresultStatus(commit_res) != PGRES_COMMAND_OK) {
                std::string error = PQresultErrorMessage(commit_res);
                PQclear(commit_res);
                throw std::runtime_error("Failed to commit transaction: " + error);
            }
            PQclear(commit_res);

            return updated_count;
        }
        catch (const std::exception&) {
            // Откатываем транзакцию при ошибке
            PGresult* rollback_res = PQexec(conn, "ROLLBACK");
            PQclear(rollback_res);
            throw;
        }
    }

    int FormDb::deleteById(const std::string& id, const Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[1] = { id.c_str() };

        PGresult* res = PQexecParams(conn,
            "DELETE FROM process_forms WHERE id = $1",
            1, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to delete form: " + error);
        }

        int affected_rows = atoi(PQcmdTuples(res));
        PQclear(res);
        return affected_rows;
    }

} // namespace db