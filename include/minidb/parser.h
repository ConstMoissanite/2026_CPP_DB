#ifndef MINIDB_PARSER_C_INTERFACE_H
#define MINIDB_PARSER_C_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* ============================================================
 * C API for the MiniDB SQL Parser
 *
 * This header provides a stable C ABI for parsing SQL statements.
 * The implementation wraps the C++ parser internally.
 * ============================================================ */

/* Opaque handle to a parsed SQL statement */
typedef struct MiniDB_Stmt MiniDB_Stmt;

/* Statement kind */
typedef enum {
    MINIDB_STMT_CREATE_DATABASE = 0,
    MINIDB_STMT_DROP_DATABASE   = 1,
    MINIDB_STMT_USE             = 2,
    MINIDB_STMT_CREATE_TABLE    = 3,
    MINIDB_STMT_DROP_TABLE      = 4,
    MINIDB_STMT_SELECT          = 5,
    MINIDB_STMT_DELETE          = 6,
    MINIDB_STMT_INSERT          = 7,
    MINIDB_STMT_UPDATE          = 8,
    MINIDB_STMT_INVALID         = 9
} MiniDB_StmtKind;

/* Column definition (for CREATE TABLE) */
typedef struct {
    const char* name;
    const char* type;
    int         is_primary;
} MiniDB_ColumnDef;

/* WHERE clause */
typedef struct {
    const char* column;
    const char* op;
    const char* const_value;
    int         is_int_literal;
} MiniDB_WhereClause;

/* -------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------- */

/* Parse an SQL string. Returns a handle; NULL on allocation failure.
 * Call minidb_stmt_kind() to check if parsing succeeded. */
MiniDB_Stmt* minidb_parse(const char* sql);

/* Free a statement handle */
void minidb_stmt_free(MiniDB_Stmt* stmt);

/* -------------------------------------------------------
 * General accessors
 * ------------------------------------------------------- */

MiniDB_StmtKind minidb_stmt_kind(const MiniDB_Stmt* stmt);
const char* minidb_stmt_error(const MiniDB_Stmt* stmt);
const char* minidb_stmt_kind_name(MiniDB_StmtKind kind);

/* -------------------------------------------------------
 * Statement-specific accessors
 * Return 1 on success, 0 if kind doesn't match.
 * ------------------------------------------------------- */

/* CREATE DATABASE / DROP DATABASE / USE */
int minidb_get_database_name(const MiniDB_Stmt* stmt, const char** name);

/* CREATE TABLE */
int minidb_get_create_table(const MiniDB_Stmt* stmt,
    const char** table_name,
    const MiniDB_ColumnDef** columns,
    size_t* column_count);

/* DROP TABLE */
int minidb_get_table_name(const MiniDB_Stmt* stmt, const char** name);

/* SELECT */
int minidb_get_select(const MiniDB_Stmt* stmt,
    const char** column_name,
    const char** table_name,
    int* has_where,
    MiniDB_WhereClause* where);

/* DELETE */
int minidb_get_delete(const MiniDB_Stmt* stmt,
    const char** table_name,
    int* has_where,
    MiniDB_WhereClause* where);

/* INSERT */
int minidb_get_insert(const MiniDB_Stmt* stmt,
    const char** table_name,
    const char* const** values,
    const int** is_int,
    size_t* value_count);

/* UPDATE */
int minidb_get_update(const MiniDB_Stmt* stmt,
    const char** table_name,
    const char** set_column,
    const char** set_value,
    int* set_is_int,
    int* has_where,
    MiniDB_WhereClause* where);

#ifdef __cplusplus
}
#endif

#endif /* MINIDB_PARSER_C_INTERFACE_H */
