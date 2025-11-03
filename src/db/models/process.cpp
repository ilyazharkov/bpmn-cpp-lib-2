// src/db/models/process.cpp
#include "db/models/Process.h"
#include "db/models/Form.h"
#include <libpq-fe.h>
#include <stdexcept>
#include <vector>
#include <memory>

namespace db {

    // Constructor implementation
    ProcessDb::ProcessDb(std::string id, std::string description, std::string xmlContent)
        : id(std::move(id)),
        description(std::move(description)),
        xmlContent(std::move(xmlContent)) {
    }

    // CRUD Operations implementation

    std::unique_ptr<ProcessDb> ProcessDb::findById(const std::string& id, Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[1] = { id.c_str() };

        PGresult* res = PQexecParams(conn,
            "SELECT id, description, xmlContent FROM process_definitions WHERE id = $1",
            1, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to find process: " + error);
        }

        if (PQntuples(res) == 0) {
            PQclear(res);
            throw std::runtime_error("Process not found with id: " + id);
        }

        // Extract data from result
        std::unique_ptr<ProcessDb> process = std::make_unique<ProcessDb>(
            PQgetvalue(res, 0, 0) ? PQgetvalue(res, 0, 0) : "",
            PQgetvalue(res, 0, 1) ? PQgetvalue(res, 0, 1) : "",
            PQgetvalue(res, 0, 2) ? PQgetvalue(res, 0, 2) : ""
        );

        PQclear(res);
        return process;
    }

    std::unique_ptr<ProcessDb> ProcessDb::create(const ProcessDb& record, Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[3] = {
            record.id.c_str(),
            record.description.c_str(),
            record.xmlContent.c_str()
        };

        PGresult* res = PQexecParams(conn,
            "INSERT INTO process_definitions(id, description, xmlContent) "
            "VALUES($1, $2, $3) RETURNING id",
            3, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to create process: " + error);
        }

        // Return the created process
        std::unique_ptr<ProcessDb> created_process = std::make_unique<ProcessDb>(
            PQgetvalue(res, 0, 0) ? PQgetvalue(res, 0, 0) : "",
            record.description,
            record.xmlContent
        );

        PQclear(res);
        return created_process;
    }

    std::vector<std::unique_ptr<ProcessDb>> ProcessDb::batchCreate(const std::vector<ProcessDb>& records, Database& db) {
        if (records.empty()) return {};

        PGconn* conn = db.getConnection();

        // Begin transaction
        PGresult* begin_res = PQexec(conn, "BEGIN");
        if (PQresultStatus(begin_res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(begin_res);
            PQclear(begin_res);
            throw std::runtime_error("Failed to begin transaction: " + error);
        }
        PQclear(begin_res);

        std::vector<std::unique_ptr<ProcessDb>> created_processes;

        try {
            for (const auto& record : records) {
                const char* params[3] = {
                    record.id.c_str(),
                    record.description.c_str(),
                    record.xmlContent.c_str()
                };

                PGresult* res = PQexecParams(conn,
                    "INSERT INTO process_definitions(id, description, xmlContent) "
                    "VALUES($1, $2, $3)",
                    3, NULL, params, NULL, NULL, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    std::string error = PQresultErrorMessage(res);
                    PQclear(res);
                    throw std::runtime_error("Batch create processes failed: " + error);
                }

                created_processes.push_back(std::make_unique<ProcessDb>(
                    record.id, record.description, record.xmlContent
                ));

                PQclear(res);
            }

            // Commit transaction
            PGresult* commit_res = PQexec(conn, "COMMIT");
            if (PQresultStatus(commit_res) != PGRES_COMMAND_OK) {
                std::string error = PQresultErrorMessage(commit_res);
                PQclear(commit_res);
                throw std::runtime_error("Failed to commit transaction: " + error);
            }
            PQclear(commit_res);

            return created_processes;
        }
        catch (const std::exception&) {
            // Rollback transaction on error
            PGresult* rollback_res = PQexec(conn, "ROLLBACK");
            PQclear(rollback_res);
            throw;
        }
    }

    int ProcessDb::update(const ProcessDb& record, Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[3] = {
            record.id.c_str(),
            record.description.c_str(),
            record.xmlContent.c_str()
        };

        PGresult* res = PQexecParams(conn,
            "UPDATE process_definitions SET "
            "description = $2, xmlContent = $3 "
            "WHERE id = $1",
            3, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to update process: " + error);
        }

        int affected_rows = atoi(PQcmdTuples(res));
        PQclear(res);
        return affected_rows;
    }

    int ProcessDb::batchUpdate(const std::vector<ProcessDb>& records, Database& db) {
        if (records.empty()) {
            return 0;
        }

        PGconn* conn = db.getConnection();

        // Begin transaction
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
                const char* params[3] = {
                    record.id.c_str(),
                    record.description.c_str(),
                    record.xmlContent.c_str()
                };

                PGresult* res = PQexecParams(conn,
                    "UPDATE process_definitions SET "
                    "description = $2, xmlContent = $3 "
                    "WHERE id = $1",
                    3, NULL, params, NULL, NULL, 0);

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    std::string error = PQresultErrorMessage(res);
                    PQclear(res);
                    throw std::runtime_error("Batch update failed: " + error);
                }

                updated_count += atoi(PQcmdTuples(res));
                PQclear(res);
            }

            // Commit transaction
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
            // Rollback transaction on error
            PGresult* rollback_res = PQexec(conn, "ROLLBACK");
            PQclear(rollback_res);
            throw;
        }
    }

    int ProcessDb::deleteById(const std::string& id, Database& db) {
        PGconn* conn = db.getConnection();
        const char* params[1] = { id.c_str() };

        PGresult* res = PQexecParams(conn,
            "DELETE FROM process_definitions WHERE id = $1",
            1, NULL, params, NULL, NULL, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string error = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("Failed to delete process: " + error);
        }

        int affected_rows = atoi(PQcmdTuples(res));
        PQclear(res);
        return affected_rows;
    }

    int ProcessDb::loadForms(Database& db) {
        // Implementation for loading related forms
        // This would typically query the forms table and populate the forms vector
        try {
            //// Example implementation - adjust based on your actual form relationship
            //auto form_list = ProcessDb::findByProcessId(this->id, db);
            //forms.clear();
            //for (auto& form : form_list) {
            //    forms.push_back(std::move(form));
            //}
            //return forms.size();
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to load forms: " + std::string(e.what()));
        }
    }

} // namespace db