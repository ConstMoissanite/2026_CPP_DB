#ifndef MINIDB_PARSER_AST_HPP
#define MINIDB_PARSER_AST_HPP

#include "../core/string.hpp"
#include "../core/vector.hpp"

namespace minidb {
namespace parser {

// ============================================================
// AST node types for all supported SQL statements
// ============================================================

// A column definition in CREATE TABLE
struct ColumnDef {
    core::String name;
    core::String type;   // "int" or "string"
    bool is_primary;

    ColumnDef() : is_primary(false) {}
};

// A WHERE clause condition: column OP const_value
struct WhereClause {
    core::String column;
    core::String op;           // "=", "<", ">"
    core::String const_value;  // the literal value as string (for comparison)
    bool is_int_literal;       // true if const_value is an integer literal

    WhereClause() : is_int_literal(false) {}
};

// ============================================================
// Statement types
// ============================================================

// CREATE DATABASE <dbname>
struct CreateDatabaseStmt {
    core::String database_name;
};

// DROP DATABASE <dbname>
struct DropDatabaseStmt {
    core::String database_name;
};

// USE <dbname>
struct UseStmt {
    core::String database_name;
};

// CREATE TABLE <table-name> ( <col> <type> [primary], ... )
struct CreateTableStmt {
    core::String table_name;
    core::Vector<ColumnDef> columns;
};

// DROP TABLE <table-name>
struct DropTableStmt {
    core::String table_name;
};

// SELECT <column> FROM <table> [WHERE <cond>]
struct SelectStmt {
    core::String column_name;   // column name or "*"
    core::String table_name;
    bool has_where;
    WhereClause where;

    SelectStmt() : has_where(false) {}
};

// DELETE FROM <table> [WHERE <cond>]
struct DeleteStmt {
    core::String table_name;
    bool has_where;
    WhereClause where;

    DeleteStmt() : has_where(false) {}
};

// INSERT INTO <table> VALUES (<val1> [, <val2> ...])
struct InsertStmt {
    core::String table_name;
    core::Vector<core::String> values;  // raw literal strings
    core::Vector<bool> is_int;         // parallel: true = int literal, false = string
};

// UPDATE <table> SET <col> = <val> [WHERE <cond>]
struct UpdateStmt {
    core::String table_name;
    core::String set_column;
    core::String set_value;
    bool set_is_int;    // true if the value is an integer
    bool has_where;
    WhereClause where;

    UpdateStmt() : set_is_int(false), has_where(false) {}
};

// ============================================================
// Statement kind enum
// ============================================================

enum class StmtKind {
    CREATE_DATABASE,
    DROP_DATABASE,
    USE,
    CREATE_TABLE,
    DROP_TABLE,
    SELECT,
    DELETE,
    INSERT,
    UPDATE,
    INVALID
};

// ============================================================
// Unified statement wrapper
// ============================================================

struct SQLStatement {
    StmtKind kind;

    // Exactly one of these is valid depending on 'kind'
    CreateDatabaseStmt create_database;
    DropDatabaseStmt   drop_database;
    UseStmt            use_stmt;
    CreateTableStmt    create_table;
    DropTableStmt      drop_table;
    SelectStmt         select_stmt;
    DeleteStmt         delete_stmt;
    InsertStmt         insert_stmt;
    UpdateStmt         update_stmt;

    core::String error_msg;  // populated if kind == INVALID

    SQLStatement() : kind(StmtKind::INVALID) {}
};

// Helper to get statement name for debugging
const char* stmt_kind_name(StmtKind kind);

} // namespace parser
} // namespace minidb

#endif // MINIDB_PARSER_AST_HPP
