#include "minidb/parser.h"
#include "parser.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "ast.hpp"
#include "../core/string.hpp"
#include "../core/vector.hpp"
#include <cstdlib>
#include <cstring>

/* ============================================================
 * Internal struct holding the parsed C++ AST
 * ============================================================ */

/* Forward decl needed by struct methods */
static char* strdup_own(const minidb::core::String& s);

struct MiniDB_Stmt {
    minidb::parser::SQLStatement ast;

    /* Flattened arrays for C accessors */
    MiniDB_ColumnDef*  columns;
    size_t             column_count;
    const char**       insert_values;
    int*               insert_is_int;
    size_t             value_count;
    const char*        error_buf;
    const char*        kind_name_buf;

    MiniDB_Stmt() : columns(nullptr), column_count(0),
                    insert_values(nullptr), insert_is_int(nullptr),
                    value_count(0), error_buf(nullptr), kind_name_buf(nullptr) {}

    ~MiniDB_Stmt() {
        delete[] columns;
        delete[] insert_values;
        delete[] insert_is_int;
        delete[] error_buf;
        delete[] kind_name_buf;
    }

    /* Flatten CREATE TABLE columns for C access */
    void flatten_columns() {
        const auto& cols = ast.create_table.columns;
        column_count = cols.size();
        if (column_count == 0) return;
        columns = new MiniDB_ColumnDef[column_count];
        for (size_t i = 0; i < column_count; ++i) {
            columns[i].name       = strdup_own(cols[i].name);
            columns[i].type       = strdup_own(cols[i].type);
            columns[i].is_primary = cols[i].is_primary ? 1 : 0;
        }
    }

    /* Flatten INSERT values for C access */
    void flatten_insert() {
        const auto& vals = ast.insert_stmt.values;
        value_count = vals.size();
        if (value_count == 0) return;
        insert_values = new const char*[value_count];
        insert_is_int = new int[value_count];
        for (size_t i = 0; i < value_count; ++i) {
            insert_values[i] = strdup_own(vals[i]);
            insert_is_int[i] = ast.insert_stmt.is_int[i] ? 1 : 0;
        }
    }
};

/* Helper: own a C++ core::String as a heap C string (allocated with new[]) */
static char* strdup_own(const minidb::core::String& s) {
    size_t len = s.length();
    char* p = new char[len + 1];
    if (len > 0) {
        std::memcpy(p, s.c_str(), len);
    }
    p[len] = '\0';
    return p;
}

/* ============================================================
 * Public C API
 * ============================================================ */

MiniDB_Stmt* minidb_parse(const char* sql) {
    MiniDB_Stmt* stmt = new MiniDB_Stmt();
    stmt->ast = minidb::parser::Parser().parse(sql);
    /* Flatten data for C accessors */
    if (stmt->ast.kind == minidb::parser::StmtKind::CREATE_TABLE) {
        stmt->flatten_columns();
    } else if (stmt->ast.kind == minidb::parser::StmtKind::INSERT) {
        stmt->flatten_insert();
    }
    return stmt;
}

void minidb_stmt_free(MiniDB_Stmt* stmt) {
    /* Free new[]'d strings inside column defs */
    if (stmt->columns) {
        for (size_t i = 0; i < stmt->column_count; ++i) {
            delete[] stmt->columns[i].name;
            delete[] stmt->columns[i].type;
        }
    }
    if (stmt->insert_values) {
        for (size_t i = 0; i < stmt->value_count; ++i) {
            delete[] stmt->insert_values[i];
        }
    }
    delete stmt;
}

MiniDB_StmtKind minidb_stmt_kind(const MiniDB_Stmt* stmt) {
    return static_cast<MiniDB_StmtKind>(stmt->ast.kind);
}

const char* minidb_stmt_error(const MiniDB_Stmt* stmt) {
    if (stmt->ast.kind != minidb::parser::StmtKind::INVALID) return "";
    /* Cache the C string; free previous buffer if already set */
    MiniDB_Stmt* mstmt = const_cast<MiniDB_Stmt*>(stmt);
    delete[] mstmt->error_buf;
    mstmt->error_buf = strdup_own(stmt->ast.error_msg);
    return mstmt->error_buf;
}

const char* minidb_stmt_kind_name(MiniDB_StmtKind kind) {
    switch (kind) {
    case MINIDB_STMT_CREATE_DATABASE: return "CREATE DATABASE";
    case MINIDB_STMT_DROP_DATABASE:   return "DROP DATABASE";
    case MINIDB_STMT_USE:             return "USE";
    case MINIDB_STMT_CREATE_TABLE:    return "CREATE TABLE";
    case MINIDB_STMT_DROP_TABLE:      return "DROP TABLE";
    case MINIDB_STMT_SELECT:          return "SELECT";
    case MINIDB_STMT_DELETE:          return "DELETE";
    case MINIDB_STMT_INSERT:          return "INSERT";
    case MINIDB_STMT_UPDATE:          return "UPDATE";
    case MINIDB_STMT_INVALID:         return "INVALID";
    default:                          return "UNKNOWN";
    }
}

/* -------------------------------------------------------
 * Statement-specific accessors
 * ------------------------------------------------------- */

int minidb_get_database_name(const MiniDB_Stmt* stmt, const char** name) {
    switch (stmt->ast.kind) {
    case minidb::parser::StmtKind::CREATE_DATABASE:
        if (name) *name = stmt->ast.create_database.database_name.c_str();
        return 1;
    case minidb::parser::StmtKind::DROP_DATABASE:
        if (name) *name = stmt->ast.drop_database.database_name.c_str();
        return 1;
    case minidb::parser::StmtKind::USE:
        if (name) *name = stmt->ast.use_stmt.database_name.c_str();
        return 1;
    default:
        return 0;
    }
}

int minidb_get_create_table(const MiniDB_Stmt* stmt,
    const char** table_name,
    const MiniDB_ColumnDef** columns,
    size_t* column_count)
{
    if (stmt->ast.kind != minidb::parser::StmtKind::CREATE_TABLE) return 0;
    if (table_name)  *table_name  = stmt->ast.create_table.table_name.c_str();
    if (columns)     *columns     = stmt->columns;
    if (column_count) *column_count = stmt->column_count;
    return 1;
}

int minidb_get_table_name(const MiniDB_Stmt* stmt, const char** name) {
    if (stmt->ast.kind != minidb::parser::StmtKind::DROP_TABLE) return 0;
    if (name) *name = stmt->ast.drop_table.table_name.c_str();
    return 1;
}

int minidb_get_select(const MiniDB_Stmt* stmt,
    const char** column_name,
    const char** table_name,
    int* has_where,
    MiniDB_WhereClause* where)
{
    if (stmt->ast.kind != minidb::parser::StmtKind::SELECT) return 0;
    if (column_name) *column_name = stmt->ast.select_stmt.column_name.c_str();
    if (table_name)  *table_name  = stmt->ast.select_stmt.table_name.c_str();
    if (has_where)   *has_where   = stmt->ast.select_stmt.has_where ? 1 : 0;
    if (stmt->ast.select_stmt.has_where && where) {
        where->column         = stmt->ast.select_stmt.where.column.c_str();
        where->op             = stmt->ast.select_stmt.where.op.c_str();
        where->const_value    = stmt->ast.select_stmt.where.const_value.c_str();
        where->is_int_literal = stmt->ast.select_stmt.where.is_int_literal ? 1 : 0;
    }
    return 1;
}

int minidb_get_delete(const MiniDB_Stmt* stmt,
    const char** table_name,
    int* has_where,
    MiniDB_WhereClause* where)
{
    if (stmt->ast.kind != minidb::parser::StmtKind::DELETE) return 0;
    if (table_name) *table_name = stmt->ast.delete_stmt.table_name.c_str();
    if (has_where)  *has_where  = stmt->ast.delete_stmt.has_where ? 1 : 0;
    if (stmt->ast.delete_stmt.has_where && where) {
        where->column         = stmt->ast.delete_stmt.where.column.c_str();
        where->op             = stmt->ast.delete_stmt.where.op.c_str();
        where->const_value    = stmt->ast.delete_stmt.where.const_value.c_str();
        where->is_int_literal = stmt->ast.delete_stmt.where.is_int_literal ? 1 : 0;
    }
    return 1;
}

int minidb_get_insert(const MiniDB_Stmt* stmt,
    const char** table_name,
    const char* const** values,
    const int** is_int,
    size_t* value_count)
{
    if (stmt->ast.kind != minidb::parser::StmtKind::INSERT) return 0;
    if (table_name)  *table_name  = stmt->ast.insert_stmt.table_name.c_str();
    if (values)      *values      = stmt->insert_values;
    if (is_int)      *is_int      = stmt->insert_is_int;
    if (value_count) *value_count = stmt->value_count;
    return 1;
}

int minidb_get_update(const MiniDB_Stmt* stmt,
    const char** table_name,
    const char** set_column,
    const char** set_value,
    int* set_is_int,
    int* has_where,
    MiniDB_WhereClause* where)
{
    if (stmt->ast.kind != minidb::parser::StmtKind::UPDATE) return 0;
    if (table_name)  *table_name  = stmt->ast.update_stmt.table_name.c_str();
    if (set_column)  *set_column  = stmt->ast.update_stmt.set_column.c_str();
    if (set_value)   *set_value   = stmt->ast.update_stmt.set_value.c_str();
    if (set_is_int)  *set_is_int  = stmt->ast.update_stmt.set_is_int ? 1 : 0;
    if (has_where)   *has_where   = stmt->ast.update_stmt.has_where ? 1 : 0;
    if (stmt->ast.update_stmt.has_where && where) {
        where->column         = stmt->ast.update_stmt.where.column.c_str();
        where->op             = stmt->ast.update_stmt.where.op.c_str();
        where->const_value    = stmt->ast.update_stmt.where.const_value.c_str();
        where->is_int_literal = stmt->ast.update_stmt.where.is_int_literal ? 1 : 0;
    }
    return 1;
}
