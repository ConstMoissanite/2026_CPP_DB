// ============================================================
// Unit tests for the MiniDB Parser C API
// ============================================================
#include "minidb/parser.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { tests_run++; printf("  TEST %s ... ", name);
#define END_TEST() \
        printf("PASSED\n"); tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { printf("FAILED: %s\n", msg); tests_failed++; return; } while(0)

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { FAIL(msg); }

#define ASSERT_EQ_STR(a, b, msg) \
    if (!(a) || std::strcmp((a), (b)) != 0) { \
        printf("FAILED: %s (expected '%s', got '%s')\n", msg, b, (a) ? (a) : "null"); \
        tests_failed++; return; \
    }

#define ASSERT_EQ_INT(a, b, msg) \
    if ((a) != (b)) { \
        printf("FAILED: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
        tests_failed++; return; \
    }

// ============================================================
// CREATE DATABASE
// ============================================================
static void test_create_database() {
    TEST("CREATE DATABASE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("create database mydb");
        ASSERT_TRUE(stmt != nullptr, "stmt not null");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_CREATE_DATABASE, "kind");
        ASSERT_EQ_STR(minidb_stmt_kind_name(minidb_stmt_kind(stmt)),
                      "CREATE DATABASE", "kind name");

        const char* name = nullptr;
        ASSERT_TRUE(minidb_get_database_name(stmt, &name) == 1, "get db name");
        ASSERT_EQ_STR(name, "mydb", "db name");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// DROP DATABASE
// ============================================================
static void test_drop_database() {
    TEST("DROP DATABASE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("drop database testdb");
        ASSERT_TRUE(stmt != nullptr, "stmt not null");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_DROP_DATABASE, "kind");

        const char* name = nullptr;
        ASSERT_TRUE(minidb_get_database_name(stmt, &name) == 1, "get db name");
        ASSERT_EQ_STR(name, "testdb", "db name");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// USE
// ============================================================
static void test_use() {
    TEST("USE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("use person");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_USE, "kind");

        const char* name = nullptr;
        ASSERT_TRUE(minidb_get_database_name(stmt, &name) == 1, "get db name");
        ASSERT_EQ_STR(name, "person", "db name");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// CREATE TABLE
// ============================================================
static void test_create_table() {
    TEST("CREATE TABLE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("create table person (id int primary, name string)");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_CREATE_TABLE, "kind");

        const char* table_name = nullptr;
        const MiniDB_ColumnDef* cols = nullptr;
        size_t col_count = 0;
        ASSERT_TRUE(minidb_get_create_table(stmt, &table_name, &cols, &col_count) == 1,
                    "get create table");
        ASSERT_EQ_STR(table_name, "person", "table name");
        ASSERT_EQ_INT(col_count, 2, "column count");

        ASSERT_EQ_STR(cols[0].name, "id", "col 0 name");
        ASSERT_EQ_STR(cols[0].type, "int", "col 0 type");
        ASSERT_EQ_INT(cols[0].is_primary, 1, "col 0 primary");
        ASSERT_EQ_STR(cols[1].name, "name", "col 1 name");
        ASSERT_EQ_STR(cols[1].type, "string", "col 1 type");
        ASSERT_EQ_INT(cols[1].is_primary, 0, "col 1 not primary");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// DROP TABLE
// ============================================================
static void test_drop_table() {
    TEST("DROP TABLE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("drop table person");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_DROP_TABLE, "kind");

        const char* name = nullptr;
        ASSERT_TRUE(minidb_get_table_name(stmt, &name) == 1, "get table name");
        ASSERT_EQ_STR(name, "person", "table name");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// SELECT
// ============================================================
static void test_select() {
    TEST("SELECT via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("select name from person where id = 1001");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_SELECT, "kind");

        const char* col = nullptr;
        const char* tbl = nullptr;
        int has_where = 0;
        MiniDB_WhereClause where;
        std::memset(&where, 0, sizeof(where));

        ASSERT_TRUE(minidb_get_select(stmt, &col, &tbl, &has_where, &where) == 1,
                    "get select");
        ASSERT_EQ_STR(col, "name", "column");
        ASSERT_EQ_STR(tbl, "person", "table");
        ASSERT_EQ_INT(has_where, 1, "has where");
        ASSERT_EQ_STR(where.column, "id", "where col");
        ASSERT_EQ_STR(where.op, "=", "where op");
        ASSERT_EQ_STR(where.const_value, "1001", "where val");
        ASSERT_EQ_INT(where.is_int_literal, 1, "is int literal");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_select_star_no_where() {
    TEST("SELECT * without WHERE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("select * from person");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_SELECT, "kind");

        const char* col = nullptr;
        const char* tbl = nullptr;
        int has_where = 0;
        MiniDB_WhereClause where;

        ASSERT_TRUE(minidb_get_select(stmt, &col, &tbl, &has_where, &where) == 1,
                    "get select");
        ASSERT_EQ_STR(col, "*", "column");
        ASSERT_EQ_STR(tbl, "person", "table");
        ASSERT_EQ_INT(has_where, 0, "no where");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_select_with_string_where() {
    TEST("SELECT with WHERE string via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("select id from person where name = \"peter\"");
        const char* col = nullptr;
        const char* tbl = nullptr;
        int has_where = 0;
        MiniDB_WhereClause where;

        ASSERT_TRUE(minidb_get_select(stmt, &col, &tbl, &has_where, &where) == 1, "get select");
        ASSERT_EQ_INT(has_where, 1, "has where");
        ASSERT_EQ_STR(where.const_value, "peter", "where val");
        ASSERT_EQ_INT(where.is_int_literal, 0, "is string literal");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_select_ops_lt_gt() {
    TEST("SELECT with < > via C API")
    {
        // <
        {
            MiniDB_Stmt* stmt = minidb_parse("select a from b where x < 10");
            MiniDB_WhereClause where;
            int has_where;
            ASSERT_TRUE(minidb_get_select(stmt, nullptr, nullptr, &has_where, &where) == 1, "get");
            ASSERT_EQ_STR(where.op, "<", "op <");
            minidb_stmt_free(stmt);
        }
        // >
        {
            MiniDB_Stmt* stmt = minidb_parse("select a from b where x > 20");
            MiniDB_WhereClause where;
            int has_where;
            ASSERT_TRUE(minidb_get_select(stmt, nullptr, nullptr, &has_where, &where) == 1, "get");
            ASSERT_EQ_STR(where.op, ">", "op >");
            minidb_stmt_free(stmt);
        }
    }
    END_TEST();
}

// ============================================================
// DELETE
// ============================================================
static void test_delete() {
    TEST("DELETE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("delete from person where id = 1001");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_DELETE, "kind");

        const char* tbl = nullptr;
        int has_where = 0;
        MiniDB_WhereClause where;
        std::memset(&where, 0, sizeof(where));

        ASSERT_TRUE(minidb_get_delete(stmt, &tbl, &has_where, &where) == 1, "get delete");
        ASSERT_EQ_STR(tbl, "person", "table");
        ASSERT_EQ_INT(has_where, 1, "has where");
        ASSERT_EQ_STR(where.column, "id", "where col");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_delete_no_where() {
    TEST("DELETE (no where) via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("delete from person");
        const char* tbl = nullptr;
        int has_where = -1;
        MiniDB_WhereClause where;

        ASSERT_TRUE(minidb_get_delete(stmt, &tbl, &has_where, &where) == 1, "get delete");
        ASSERT_EQ_INT(has_where, 0, "no where");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// INSERT
// ============================================================
static void test_insert() {
    TEST("INSERT via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("insert into person values (1001, \"peter\")");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_INSERT, "kind");

        const char* tbl = nullptr;
        const char* const* vals = nullptr;
        const int* is_int_arr = nullptr;
        size_t val_count = 0;

        ASSERT_TRUE(minidb_get_insert(stmt, &tbl, &vals, &is_int_arr, &val_count) == 1,
                    "get insert");
        ASSERT_EQ_STR(tbl, "person", "table");
        ASSERT_EQ_INT(val_count, 2, "value count");
        ASSERT_EQ_STR(vals[0], "1001", "first val");
        ASSERT_EQ_INT(is_int_arr[0], 1, "first is int");
        ASSERT_EQ_STR(vals[1], "peter", "second val");
        ASSERT_EQ_INT(is_int_arr[1], 0, "second is string");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_insert_single() {
    TEST("INSERT single value via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("insert into t values (42)");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_INSERT, "kind");

        const char* tbl = nullptr;
        const char* const* vals = nullptr;
        const int* is_int_arr = nullptr;
        size_t val_count = 0;

        ASSERT_TRUE(minidb_get_insert(stmt, &tbl, &vals, &is_int_arr, &val_count) == 1,
                    "get insert");
        ASSERT_EQ_INT(val_count, 1, "value count");
        ASSERT_EQ_STR(vals[0], "42", "value");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// UPDATE
// ============================================================
static void test_update() {
    TEST("UPDATE via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("update person set name = \"alice\" where id = 1001");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_UPDATE, "kind");

        const char* tbl = nullptr;
        const char* set_col = nullptr;
        const char* set_val = nullptr;
        int set_is_int = 0;
        int has_where = 0;
        MiniDB_WhereClause where;

        ASSERT_TRUE(minidb_get_update(stmt, &tbl, &set_col, &set_val,
                    &set_is_int, &has_where, &where) == 1, "get update");
        ASSERT_EQ_STR(tbl, "person", "table");
        ASSERT_EQ_STR(set_col, "name", "set col");
        ASSERT_EQ_STR(set_val, "alice", "set val");
        ASSERT_EQ_INT(set_is_int, 0, "set is string");
        ASSERT_EQ_INT(has_where, 1, "has where");
        ASSERT_EQ_STR(where.column, "id", "where col");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_update_no_where() {
    TEST("UPDATE (no where) via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("update person set age = 25");
        const char* tbl;
        const char* set_col;
        const char* set_val;
        int set_is_int;
        int has_where;
        MiniDB_WhereClause where;

        ASSERT_TRUE(minidb_get_update(stmt, &tbl, &set_col, &set_val,
                    &set_is_int, &has_where, &where) == 1, "get update");
        ASSERT_EQ_INT(has_where, 0, "no where");
        ASSERT_EQ_INT(set_is_int, 1, "set is int");
        ASSERT_EQ_STR(set_val, "25", "set val");

        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// Error handling
// ============================================================
static void test_error_handling() {
    TEST("error message via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("invalid sql here");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_INVALID, "kind");
        ASSERT_EQ_STR(minidb_stmt_kind_name(MINIDB_STMT_INVALID), "INVALID", "kind name");

        const char* err = minidb_stmt_error(stmt);
        ASSERT_TRUE(err != nullptr, "error not null");
        ASSERT_TRUE(std::strlen(err) > 0, "error non-empty");

        // Calling again should not crash or leak
        const char* err2 = minidb_stmt_error(stmt);
        ASSERT_TRUE(err2 != nullptr, "error2 not null");

        minidb_stmt_free(stmt);
    }
    END_TEST();

    TEST("no error for valid stmt")
    {
        MiniDB_Stmt* stmt = minidb_parse("use db");
        ASSERT_TRUE(minidb_stmt_kind(stmt) != MINIDB_STMT_INVALID, "valid stmt");
        const char* err = minidb_stmt_error(stmt);
        ASSERT_EQ_STR(err, "", "empty error for valid");
        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// Edge cases
// ============================================================
static void test_case_insensitivity_c() {
    TEST("case insensitivity via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("CREATE DATABASE Mydb");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_CREATE_DATABASE, "kind");
        const char* name;
        minidb_get_database_name(stmt, &name);
        ASSERT_EQ_STR(name, "Mydb", "preserves case in identifiers");
        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_semicolons() {
    TEST("statements with semicolons via C API")
    {
        MiniDB_Stmt* stmt = minidb_parse("use mydb;");
        ASSERT_EQ_INT(minidb_stmt_kind(stmt), MINIDB_STMT_USE, "kind");

        const char* name;
        ASSERT_TRUE(minidb_get_database_name(stmt, &name) == 1, "get name");
        ASSERT_EQ_STR(name, "mydb", "name");
        minidb_stmt_free(stmt);
    }
    END_TEST();
}

static void test_accessor_mismatch() {
    TEST("wrong accessor returns 0")
    {
        // Try to get INSERT data from a SELECT stmt
        MiniDB_Stmt* stmt = minidb_parse("select a from b");
        const char* tbl = nullptr;
        const char* const* vals = nullptr;
        const int* is_int_arr = nullptr;
        size_t vc = 0;
        ASSERT_TRUE(minidb_get_insert(stmt, &tbl, &vals, &is_int_arr, &vc) == 0,
                    "should return 0 for wrong type");
        minidb_stmt_free(stmt);
    }
    END_TEST();
}

// ============================================================
// Main
// ============================================================
int main() {
    printf("=== MiniDB C API Tests ===\n\n");

    test_create_database();
    test_drop_database();
    test_use();
    test_create_table();
    test_drop_table();
    test_select();
    test_select_star_no_where();
    test_select_with_string_where();
    test_select_ops_lt_gt();
    test_delete();
    test_delete_no_where();
    test_insert();
    test_insert_single();
    test_update();
    test_update_no_where();
    test_error_handling();
    test_case_insensitivity_c();
    test_semicolons();
    test_accessor_mismatch();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
