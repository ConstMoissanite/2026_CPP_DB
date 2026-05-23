#include "executor.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

namespace minidb {
namespace execution {

// Temporary result table storage (for SELECT results to be serialized)
static storage::Table g_result_table(core::String("__result__"));

Executor::Executor(const char* data_root) 
    : _data_root(data_root), _current_db(nullptr) {
    MKDIR(data_root);
}

Executor::~Executor() {
    delete _current_db;
}

const char* Executor::current_db_name() const {
    return _current_db_name.empty() ? "" : _current_db_name.c_str();
}

// ============================================================
// Main dispatch
// ============================================================
ExecResult Executor::execute(const parser::SQLStatement& stmt) {
    switch (stmt.kind) {
    case parser::StmtKind::CREATE_DATABASE:
        return exec_create_database(stmt.create_database);
    case parser::StmtKind::DROP_DATABASE:
        return exec_drop_database(stmt.drop_database);
    case parser::StmtKind::USE:
        return exec_use(stmt.use_stmt);
    case parser::StmtKind::CREATE_TABLE:
        return exec_create_table(stmt.create_table);
    case parser::StmtKind::DROP_TABLE:
        return exec_drop_table(stmt.drop_table);
    case parser::StmtKind::SELECT:
        return exec_select(stmt.select_stmt);
    case parser::StmtKind::DELETE:
        return exec_delete(stmt.delete_stmt);
    case parser::StmtKind::INSERT:
        return exec_insert(stmt.insert_stmt);
    case parser::StmtKind::UPDATE:
        return exec_update(stmt.update_stmt);
    default:
        return ExecResult::error(stmt.error_msg.empty()
            ? "Unknown statement" : stmt.error_msg.c_str());
    }
}

// ============================================================
// DDL: CREATE DATABASE
// ============================================================
ExecResult Executor::exec_create_database(const parser::CreateDatabaseStmt& stmt) {
    bool ok = storage::create_database(_data_root.c_str(), stmt.database_name.c_str());
    if (!ok) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Failed to create database '%s'",
            stmt.database_name.c_str());
        return ExecResult::error(buf);
    }
    char buf[512];
    std::snprintf(buf, sizeof(buf), "Database '%s' created", stmt.database_name.c_str());
    return ExecResult::ok(buf);
}

// ============================================================
// DDL: DROP DATABASE
// ============================================================
ExecResult Executor::exec_drop_database(const parser::DropDatabaseStmt& stmt) {
    // If currently using this database, clear it
    if (_current_db_name == stmt.database_name.c_str()) {
        _current_db->save_all();
        delete _current_db;
        _current_db = nullptr;
        _current_db_name = core::String();
    }

    // Build path and remove
    const char* r = _data_root.c_str();
    const char* n = stmt.database_name.c_str();
    std::size_t total = std::strlen(r) + 1 + std::strlen(n);
    char* buf = new char[total + 1];
    std::memcpy(buf, r, std::strlen(r));
    buf[std::strlen(r)] = '/';
    std::memcpy(buf + std::strlen(r) + 1, n, std::strlen(n));
    buf[total] = '\0';

#ifdef _WIN32
    int rc = _rmdir(buf);
#else
    int rc = rmdir(buf);
#endif
    delete[] buf;

    if (rc != 0) {
        char msg[512];
        std::snprintf(msg, sizeof(msg), "Failed to drop database '%s'",
            stmt.database_name.c_str());
        return ExecResult::error(msg);
    }

    char msg[512];
    std::snprintf(msg, sizeof(msg), "Database '%s' dropped", stmt.database_name.c_str());
    return ExecResult::ok(msg);
}

// ============================================================
// DDL: USE
// ============================================================
ExecResult Executor::exec_use(const parser::UseStmt& stmt) {
    // Clean up previous database
    if (_current_db) {
        _current_db->save_all();
        delete _current_db;
    }

    _current_db_name = stmt.database_name;

    // Build data dir path
    const char* r = _data_root.c_str();
    const char* n = _current_db_name.c_str();
    std::size_t total = std::strlen(r) + 1 + std::strlen(n);
    char* buf = new char[total + 1];
    std::memcpy(buf, r, std::strlen(r));
    buf[std::strlen(r)] = '/';
    std::memcpy(buf + std::strlen(r) + 1, n, std::strlen(n));
    buf[total] = '\0';
    core::String db_path(buf);
    delete[] buf;

    _current_db = new storage::Database(_current_db_name);
    _current_db->set_data_dir(std::move(db_path));

    char msg[512];
    std::snprintf(msg, sizeof(msg), "Using database '%s'", stmt.database_name.c_str());
    return ExecResult::ok(msg);
}

// ============================================================
// DDL: CREATE TABLE
// ============================================================
ExecResult Executor::exec_create_table(const parser::CreateTableStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    // Convert AST ColumnDef to storage::Column
    core::Vector<storage::Column> columns;
    for (std::size_t i = 0; i < stmt.columns.size(); ++i) {
        const auto& cd = stmt.columns[i];
        storage::ColumnType ct = storage::Column::type_from_string(cd.type.c_str());
        columns.push_back(storage::Column(cd.name, ct, cd.is_primary));
    }

    bool ok = db->create_table(stmt.table_name, columns);
    if (!ok) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' already exists",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    char buf[512];
    std::snprintf(buf, sizeof(buf), "Table '%s' created", stmt.table_name.c_str());
    return ExecResult::ok(buf);
}

// ============================================================
// DDL: DROP TABLE
// ============================================================
ExecResult Executor::exec_drop_table(const parser::DropTableStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    bool ok = db->drop_table(stmt.table_name.c_str());
    if (!ok) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' does not exist",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    char buf[512];
    std::snprintf(buf, sizeof(buf), "Table '%s' dropped", stmt.table_name.c_str());
    return ExecResult::ok(buf);
}

// ============================================================
// Helper: require a database
// ============================================================
bool Executor::require_db(ExecResult& err) {
    if (_current_db_name.empty()) {
        err = ExecResult::error("No database selected. Use 'use <dbname>' first.");
        return false;
    }
    return true;
}

// ============================================================
// Helper: parse WHERE value
// ============================================================
storage::Value Executor::parse_where_value(const parser::WhereClause& where) const {
    if (where.is_int_literal) {
        return storage::Value(std::atoi(where.const_value.c_str()));
    }
    return storage::Value(where.const_value);
}

// ============================================================
// Helper: match row against WHERE clause
// ============================================================
bool Executor::matches_where(const storage::Row& row, int col_idx,
                              const parser::WhereClause& where) const {
    if (col_idx < 0 || static_cast<std::size_t>(col_idx) >= row.count()) {
        return false;
    }

    const storage::Value& cell = row.get_value(static_cast<std::size_t>(col_idx));
    storage::Value target = parse_where_value(where);

    int cmp = storage::Value::compare(cell, target);

    if (where.op == core::String("="))  return cmp == 0;
    if (where.op == core::String("<"))  return cmp < 0;
    if (where.op == core::String(">"))  return cmp > 0;
    return false;
}

// ============================================================
// DML: SELECT
// ============================================================
ExecResult Executor::exec_select(const parser::SelectStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    storage::Table* table = db->get_table(stmt.table_name.c_str());
    if (!table) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' does not exist",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    // Find the target column index
    bool select_all = (stmt.column_name == core::String("*"));
    int target_col = select_all ? -1 : table->find_column(stmt.column_name.c_str());

    if (!select_all && target_col < 0) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Column '%s' not found in table '%s'",
            stmt.column_name.c_str(), stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    // Find WHERE column index (if applicable)
    int where_col = -1;
    if (stmt.has_where) {
        where_col = table->find_column(stmt.where.column.c_str());
        if (where_col < 0) {
            char buf[512];
            std::snprintf(buf, sizeof(buf), "Column '%s' not found in table '%s'",
                stmt.where.column.c_str(), stmt.table_name.c_str());
            return ExecResult::error(buf);
        }
    }

    // Build result table with schema subset
    static storage::Table result(core::String("__result__"));
    // Clear and rebuild schema
    result = storage::Table(core::String("__result__"));

    if (select_all) {
        // Copy all columns
        for (std::size_t i = 0; i < table->columns().size(); ++i) {
            result.add_column(table->columns()[i]);
        }
    } else {
        // Single column
        result.add_column(table->columns()[static_cast<std::size_t>(target_col)]);
    }

    // Filter and project rows
    for (std::size_t i = 0; i < table->row_count(); ++i) {
        const storage::Row& row = table->row_at(i);

        if (stmt.has_where) {
            if (!matches_where(row, where_col, stmt.where)) continue;
        }

        storage::Row new_row;
        if (select_all) {
            for (std::size_t j = 0; j < row.count(); ++j) {
                new_row.add_value(row.get_value(j));
            }
        } else {
            new_row.add_value(row.get_value(static_cast<std::size_t>(target_col)));
        }
        result.insert_row(std::move(new_row));
    }

    g_result_table = std::move(result);
    ExecResult r = ExecResult::ok("Query OK");
    r.result_table = &g_result_table;
    return r;
}

// ============================================================
// DML: DELETE
// ============================================================
ExecResult Executor::exec_delete(const parser::DeleteStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    storage::Table* table = db->get_table(stmt.table_name.c_str());
    if (!table) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' does not exist",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    storage::Index* idx = db->get_index(stmt.table_name.c_str());
    int where_col = -1;
    int pk_col = table->primary_key_index();

    if (stmt.has_where) {
        where_col = table->find_column(stmt.where.column.c_str());
        if (where_col < 0) {
            char buf[512];
            std::snprintf(buf, sizeof(buf), "Column '%s' not found in table '%s'",
                stmt.where.column.c_str(), stmt.table_name.c_str());
            return ExecResult::error(buf);
        }
    }

    // Delete matching rows (iterate backwards for safe removal)
    int deleted = 0;
    for (int i = static_cast<int>(table->row_count()) - 1; i >= 0; --i) {
        if (stmt.has_where) {
            if (!matches_where(table->row_at(static_cast<std::size_t>(i)),
                               where_col, stmt.where)) continue;
        }

        // Update index if primary key exists
        if (idx && pk_col >= 0) {
            const storage::Value& pk_val = table->row_at(
                static_cast<std::size_t>(i)).get_value(static_cast<std::size_t>(pk_col));
            if (pk_val.is_int()) {
                idx->remove(pk_val.int_value());
            }
        }

        table->remove_row(static_cast<std::size_t>(i));
        ++deleted;
    }

    char buf[256];
    std::snprintf(buf, sizeof(buf), "%d row(s) deleted", deleted);
    return ExecResult::ok(buf);
}

// ============================================================
// DML: INSERT
// ============================================================
ExecResult Executor::exec_insert(const parser::InsertStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    storage::Table* table = db->get_table(stmt.table_name.c_str());
    if (!table) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' does not exist",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    // Validate value count matches column count
    if (stmt.values.size() != table->column_count()) {
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "Column count mismatch: expected %zu, got %zu",
            table->column_count(), stmt.values.size());
        return ExecResult::error(buf);
    }

    // Build row from values
    storage::Row row;
    int pk_col = table->primary_key_index();
    int pk_val = 0;

    for (std::size_t i = 0; i < stmt.values.size(); ++i) {
        if (stmt.is_int[i]) {
            int ival = std::atoi(stmt.values[i].c_str());
            row.add_value(storage::Value(ival));
        } else {
            row.add_value(storage::Value(stmt.values[i]));
        }
    }

    // Check primary key uniqueness
    storage::Index* idx = db->get_index(stmt.table_name.c_str());
    if (pk_col >= 0 && idx) {
        const storage::Value& pk = row.get_value(static_cast<std::size_t>(pk_col));
        if (pk.is_int()) {
            pk_val = pk.int_value();
            int existing;
            if (idx->find(pk_val, existing)) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "Duplicate primary key: %d", pk_val);
                return ExecResult::error(buf);
            }
        }
    }

    std::size_t row_idx = table->row_count();
    table->insert_row(row);

    // Update index
    if (pk_col >= 0) {
        if (idx) {
            idx->insert(pk_val, static_cast<int>(row_idx));
        }
    }

    return ExecResult::ok("1 row inserted");
}

// ============================================================
// DML: UPDATE
// ============================================================
ExecResult Executor::exec_update(const parser::UpdateStmt& stmt) {
    ExecResult err;
    if (!require_db(err)) return err;

    storage::Database* db = _current_db;
    if (!db) return ExecResult::error("No database selected");

    storage::Table* table = db->get_table(stmt.table_name.c_str());
    if (!table) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Table '%s' does not exist",
            stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    // Find the column to update
    int set_col = table->find_column(stmt.set_column.c_str());
    if (set_col < 0) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Column '%s' not found in table '%s'",
            stmt.set_column.c_str(), stmt.table_name.c_str());
        return ExecResult::error(buf);
    }

    // Find WHERE column index
    int where_col = -1;
    if (stmt.has_where) {
        where_col = table->find_column(stmt.where.column.c_str());
        if (where_col < 0) {
            char buf[512];
            std::snprintf(buf, sizeof(buf), "Column '%s' not found in table '%s'",
                stmt.where.column.c_str(), stmt.table_name.c_str());
            return ExecResult::error(buf);
        }
    }

    // Build the new value
    storage::Value new_val;
    if (stmt.set_is_int) {
        new_val = storage::Value(std::atoi(stmt.set_value.c_str()));
    } else {
        new_val = storage::Value(stmt.set_value);
    }

    int updated = 0;
    int pk_col = table->primary_key_index();
    storage::Index* idx = db->get_index(stmt.table_name.c_str());

    for (std::size_t i = 0; i < table->row_count(); ++i) {
        if (stmt.has_where) {
            if (!matches_where(table->row_at(i), where_col, stmt.where)) continue;
        }

        // Update the cell
        table->row_at(i).get_value(static_cast<std::size_t>(set_col)) = new_val;

        // If updating primary key, update index
        if (set_col == pk_col && idx && new_val.is_int()) {
            // Remove old entry... we don't track old value easily.
            // For simplicity: rebuild index (not efficient but correct)
            // Actually let's just update the entry
            idx->insert(new_val.int_value(), static_cast<int>(i));
        }

        ++updated;
    }

    char buf[256];
    std::snprintf(buf, sizeof(buf), "%d row(s) updated", updated);
    return ExecResult::ok(buf);
}

// ============================================================
// Shutdown
// ============================================================
void Executor::shutdown() {
    if (_current_db) {
        _current_db->save_all();
    }
}

} // namespace execution
} // namespace minidb
