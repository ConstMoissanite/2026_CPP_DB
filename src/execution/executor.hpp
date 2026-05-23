#ifndef MINIDB_EXECUTION_EXECUTOR_HPP
#define MINIDB_EXECUTION_EXECUTOR_HPP

#include "../core/string.hpp"
#include "../parser/ast.hpp"
#include "../storage/database.hpp"
#include "../storage/table.hpp"

namespace minidb {
namespace execution {

// Result of executing a SQL statement.
struct ExecResult {
    enum Status { OK, ERROR };

    Status status;
    core::String message;           // human-readable message
    const storage::Table* result_table; // non-null for SELECT results

    ExecResult() : status(OK), result_table(nullptr) {}

    static ExecResult ok(const char* msg) {
        ExecResult r;
        r.status = OK;
        r.message = core::String(msg);
        return r;
    }

    static ExecResult error(const char* msg) {
        ExecResult r;
        r.status = ERROR;
        r.message = core::String(msg);
        return r;
    }
};

// Execution engine: interprets parsed SQL and calls the storage layer.
class Executor {
public:
    explicit Executor(const char* data_root = "data");
    ~Executor();

    // Execute a parsed SQL statement
    ExecResult execute(const parser::SQLStatement& stmt);

    // Get current database name (empty if none selected)
    const char* current_db_name() const;

    // Shutdown: save all data
    void shutdown();

private:
    core::String _data_root;
    core::String _current_db_name;
    storage::Database* _current_db;   // owned by this executor

    // DDL handlers
    ExecResult exec_create_database(const parser::CreateDatabaseStmt& stmt);
    ExecResult exec_drop_database(const parser::DropDatabaseStmt& stmt);
    ExecResult exec_use(const parser::UseStmt& stmt);
    ExecResult exec_create_table(const parser::CreateTableStmt& stmt);
    ExecResult exec_drop_table(const parser::DropTableStmt& stmt);

    // DML handlers
    ExecResult exec_select(const parser::SelectStmt& stmt);
    ExecResult exec_delete(const parser::DeleteStmt& stmt);
    ExecResult exec_insert(const parser::InsertStmt& stmt);
    ExecResult exec_update(const parser::UpdateStmt& stmt);

    // Helper: require a database to be selected
    bool require_db(ExecResult& err);

    // Helper: parse a WHERE clause value to a storage::Value
    storage::Value parse_where_value(const parser::WhereClause& where) const;

    // Helper: match a row against a WHERE clause
    bool matches_where(const storage::Row& row, int col_idx,
                       const parser::WhereClause& where) const;
};

} // namespace execution
} // namespace minidb

#endif // MINIDB_EXECUTION_EXECUTOR_HPP
