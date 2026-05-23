// ============================================================
// Unit tests for the MiniDB Parser
// ============================================================
#include "parser/parser.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace minidb::parser;

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
    if (std::strcmp((a), (b)) != 0) { \
        printf("FAILED: %s (expected '%s', got '%s')\n", msg, b, a); \
        tests_failed++; return; \
    }

#define ASSERT_STMT_KIND(stmt, expected) \
    if (stmt.kind != expected) { \
        printf("FAILED: expected stmt kind '%s', got '%s'\n", \
               stmt_kind_name(expected), stmt_kind_name(stmt.kind)); \
        tests_failed++; return; \
    }

static Parser parser;

// ============================================================
// CREATE DATABASE
// ============================================================
static void test_create_database() {
    TEST("CREATE DATABASE")
    {
        auto stmt = parser.parse("create database mydb");
        ASSERT_STMT_KIND(stmt, StmtKind::CREATE_DATABASE);
        ASSERT_EQ_STR(stmt.create_database.database_name.c_str(), "mydb", "db name");
    }
    END_TEST();

    TEST("CREATE DATABASE with semicolon")
    {
        auto stmt = parser.parse("create database testdb;");
        ASSERT_STMT_KIND(stmt, StmtKind::CREATE_DATABASE);
        ASSERT_EQ_STR(stmt.create_database.database_name.c_str(), "testdb", "db name");
    }
    END_TEST();
}

// ============================================================
// DROP DATABASE
// ============================================================
static void test_drop_database() {
    TEST("DROP DATABASE")
    {
        auto stmt = parser.parse("drop database mydb");
        ASSERT_STMT_KIND(stmt, StmtKind::DROP_DATABASE);
        ASSERT_EQ_STR(stmt.drop_database.database_name.c_str(), "mydb", "db name");
    }
    END_TEST();
}

// ============================================================
// USE
// ============================================================
static void test_use() {
    TEST("USE")
    {
        auto stmt = parser.parse("use person");
        ASSERT_STMT_KIND(stmt, StmtKind::USE);
        ASSERT_EQ_STR(stmt.use_stmt.database_name.c_str(), "person", "db name");
    }
    END_TEST();
}

// ============================================================
// CREATE TABLE
// ============================================================
static void test_create_table_single_col() {
    TEST("CREATE TABLE - single column")
    {
        auto stmt = parser.parse("create table t (id int)");
        ASSERT_STMT_KIND(stmt, StmtKind::CREATE_TABLE);
        ASSERT_EQ_STR(stmt.create_table.table_name.c_str(), "t", "table name");
        ASSERT_TRUE(stmt.create_table.columns.size() == 1, "column count");
        ASSERT_EQ_STR(stmt.create_table.columns[0].name.c_str(), "id", "col name");
        ASSERT_EQ_STR(stmt.create_table.columns[0].type.c_str(), "int", "col type");
        ASSERT_TRUE(!stmt.create_table.columns[0].is_primary, "not primary");
    }
    END_TEST();
}

static void test_create_table_with_primary() {
    TEST("CREATE TABLE - with primary key")
    {
        auto stmt = parser.parse("create table person (id int primary, name string)");
        ASSERT_STMT_KIND(stmt, StmtKind::CREATE_TABLE);
        ASSERT_EQ_STR(stmt.create_table.table_name.c_str(), "person", "table name");
        ASSERT_TRUE(stmt.create_table.columns.size() == 2, "column count");
        ASSERT_EQ_STR(stmt.create_table.columns[0].name.c_str(), "id", "first col");
        ASSERT_EQ_STR(stmt.create_table.columns[0].type.c_str(), "int", "first type");
        ASSERT_TRUE(stmt.create_table.columns[0].is_primary, "is primary");
        ASSERT_EQ_STR(stmt.create_table.columns[1].name.c_str(), "name", "second col");
        ASSERT_EQ_STR(stmt.create_table.columns[1].type.c_str(), "string", "second type");
        ASSERT_TRUE(!stmt.create_table.columns[1].is_primary, "not primary");
    }
    END_TEST();
}

static void test_create_table_error_duplicate_pk() {
    TEST("CREATE TABLE - duplicate primary keys (error)")
    {
        auto stmt = parser.parse("create table t (a int primary, b int primary)");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
        ASSERT_TRUE(stmt.error_msg.length() > 0, "should have error");
    }
    END_TEST();
}

static void test_create_table_error_no_cols() {
    TEST("CREATE TABLE - no columns (error)")
    {
        auto stmt = parser.parse("create table t ()");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
    }
    END_TEST();
}

static void test_create_table_error_no_paren() {
    TEST("CREATE TABLE - missing parens (error)")
    {
        auto stmt = parser.parse("create table t id int");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
    }
    END_TEST();
}

// ============================================================
// DROP TABLE
// ============================================================
static void test_drop_table() {
    TEST("DROP TABLE")
    {
        auto stmt = parser.parse("drop table person");
        ASSERT_STMT_KIND(stmt, StmtKind::DROP_TABLE);
        ASSERT_EQ_STR(stmt.drop_table.table_name.c_str(), "person", "table name");
    }
    END_TEST();
}

// ============================================================
// SELECT
// ============================================================
static void test_select_star() {
    TEST("SELECT *")
    {
        auto stmt = parser.parse("select * from person");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_EQ_STR(stmt.select_stmt.column_name.c_str(), "*", "column");
        ASSERT_EQ_STR(stmt.select_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(!stmt.select_stmt.has_where, "no where");
    }
    END_TEST();
}

static void test_select_single_col() {
    TEST("SELECT single column")
    {
        auto stmt = parser.parse("select name from person");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_EQ_STR(stmt.select_stmt.column_name.c_str(), "name", "column");
        ASSERT_EQ_STR(stmt.select_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(!stmt.select_stmt.has_where, "no where");
    }
    END_TEST();
}

static void test_select_with_where_eq() {
    TEST("SELECT with WHERE =")
    {
        auto stmt = parser.parse("select name from person where id = 1001");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_EQ_STR(stmt.select_stmt.column_name.c_str(), "name", "column");
        ASSERT_EQ_STR(stmt.select_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(stmt.select_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.select_stmt.where.column.c_str(), "id", "where col");
        ASSERT_EQ_STR(stmt.select_stmt.where.op.c_str(), "=", "where op");
        ASSERT_EQ_STR(stmt.select_stmt.where.const_value.c_str(), "1001", "where val");
        ASSERT_TRUE(stmt.select_stmt.where.is_int_literal, "is int");
    }
    END_TEST();
}

static void test_select_with_where_lt() {
    TEST("SELECT with WHERE <")
    {
        auto stmt = parser.parse("select name from person where age < 30");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_TRUE(stmt.select_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.select_stmt.where.op.c_str(), "<", "where op");
        ASSERT_EQ_STR(stmt.select_stmt.where.const_value.c_str(), "30", "where val");
    }
    END_TEST();
}

static void test_select_with_where_gt() {
    TEST("SELECT with WHERE >")
    {
        auto stmt = parser.parse("select * from person where id > 0");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_TRUE(stmt.select_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.select_stmt.where.op.c_str(), ">", "where op");
        ASSERT_EQ_STR(stmt.select_stmt.where.const_value.c_str(), "0", "where val");
    }
    END_TEST();
}

static void test_select_with_where_string() {
    TEST("SELECT with WHERE string literal")
    {
        auto stmt = parser.parse("select id from person where name = \"peter\"");
        ASSERT_STMT_KIND(stmt, StmtKind::SELECT);
        ASSERT_TRUE(stmt.select_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.select_stmt.where.const_value.c_str(), "peter", "where val");
        ASSERT_TRUE(!stmt.select_stmt.where.is_int_literal, "is string");
    }
    END_TEST();
}

static void test_select_error_no_from() {
    TEST("SELECT - missing FROM (error)")
    {
        auto stmt = parser.parse("select name person");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
    }
    END_TEST();
}

// ============================================================
// DELETE
// ============================================================
static void test_delete_all() {
    TEST("DELETE (no where)")
    {
        auto stmt = parser.parse("delete from person");
        ASSERT_STMT_KIND(stmt, StmtKind::DELETE);
        ASSERT_EQ_STR(stmt.delete_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(!stmt.delete_stmt.has_where, "no where");
    }
    END_TEST();
}

static void test_delete_with_where() {
    TEST("DELETE with WHERE")
    {
        auto stmt = parser.parse("delete from person where id = 1001");
        ASSERT_STMT_KIND(stmt, StmtKind::DELETE);
        ASSERT_TRUE(stmt.delete_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.delete_stmt.where.column.c_str(), "id", "where col");
        ASSERT_EQ_STR(stmt.delete_stmt.where.op.c_str(), "=", "where op");
    }
    END_TEST();
}

// ============================================================
// INSERT
// ============================================================
static void test_insert_single_value() {
    TEST("INSERT single value")
    {
        auto stmt = parser.parse("insert into person values (1001)");
        ASSERT_STMT_KIND(stmt, StmtKind::INSERT);
        ASSERT_EQ_STR(stmt.insert_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(stmt.insert_stmt.values.size() == 1, "value count");
        ASSERT_EQ_STR(stmt.insert_stmt.values[0].c_str(), "1001", "value");
        ASSERT_TRUE(stmt.insert_stmt.is_int[0], "is int");
    }
    END_TEST();
}

static void test_insert_multi_values() {
    TEST("INSERT multiple values")
    {
        auto stmt = parser.parse("insert into person values (1001, \"peter\")");
        ASSERT_STMT_KIND(stmt, StmtKind::INSERT);
        ASSERT_EQ_STR(stmt.insert_stmt.table_name.c_str(), "person", "table");
        ASSERT_TRUE(stmt.insert_stmt.values.size() == 2, "value count");
        ASSERT_EQ_STR(stmt.insert_stmt.values[0].c_str(), "1001", "first value");
        ASSERT_TRUE(stmt.insert_stmt.is_int[0], "first is int");
        ASSERT_EQ_STR(stmt.insert_stmt.values[1].c_str(), "peter", "second value");
        ASSERT_TRUE(!stmt.insert_stmt.is_int[1], "second is string");
    }
    END_TEST();
}

static void test_insert_error_no_values() {
    TEST("INSERT - empty values (error)")
    {
        auto stmt = parser.parse("insert into person values ()");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
    }
    END_TEST();
}

// ============================================================
// UPDATE
// ============================================================
static void test_update_all() {
    TEST("UPDATE (no where)")
    {
        auto stmt = parser.parse("update person set name = \"john\"");
        ASSERT_STMT_KIND(stmt, StmtKind::UPDATE);
        ASSERT_EQ_STR(stmt.update_stmt.table_name.c_str(), "person", "table");
        ASSERT_EQ_STR(stmt.update_stmt.set_column.c_str(), "name", "set col");
        ASSERT_EQ_STR(stmt.update_stmt.set_value.c_str(), "john", "set val");
        ASSERT_TRUE(!stmt.update_stmt.set_is_int, "set is string");
        ASSERT_TRUE(!stmt.update_stmt.has_where, "no where");
    }
    END_TEST();
}

static void test_update_with_where() {
    TEST("UPDATE with WHERE")
    {
        auto stmt = parser.parse("update person set name = \"alice\" where id = 1001");
        ASSERT_STMT_KIND(stmt, StmtKind::UPDATE);
        ASSERT_EQ_STR(stmt.update_stmt.set_column.c_str(), "name", "set col");
        ASSERT_TRUE(stmt.update_stmt.has_where, "has where");
        ASSERT_EQ_STR(stmt.update_stmt.where.column.c_str(), "id", "where col");
    }
    END_TEST();
}

static void test_update_int_value() {
    TEST("UPDATE with int value")
    {
        auto stmt = parser.parse("update person set age = 25 where id = 1");
        ASSERT_STMT_KIND(stmt, StmtKind::UPDATE);
        ASSERT_EQ_STR(stmt.update_stmt.set_value.c_str(), "25", "set val");
        ASSERT_TRUE(stmt.update_stmt.set_is_int, "set is int");
    }
    END_TEST();
}

// ============================================================
// Error cases
// ============================================================
static void test_invalid_statement() {
    TEST("invalid statement")
    {
        auto stmt = parser.parse("invalid sql here");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
        ASSERT_TRUE(stmt.error_msg.length() > 0, "should have error message");
    }
    END_TEST();
}

static void test_empty_input() {
    TEST("empty input (error)")
    {
        auto stmt = parser.parse("");
        ASSERT_STMT_KIND(stmt, StmtKind::INVALID);
    }
    END_TEST();
}

// ============================================================
// Main
// ============================================================
int main() {
    printf("=== MiniDB Parser Tests ===\n\n");

    test_create_database();
    test_drop_database();
    test_use();
    test_create_table_single_col();
    test_create_table_with_primary();
    test_create_table_error_duplicate_pk();
    test_create_table_error_no_cols();
    test_create_table_error_no_paren();
    test_drop_table();
    test_select_star();
    test_select_single_col();
    test_select_with_where_eq();
    test_select_with_where_lt();
    test_select_with_where_gt();
    test_select_with_where_string();
    test_select_error_no_from();
    test_delete_all();
    test_delete_with_where();
    test_insert_single_value();
    test_insert_multi_values();
    test_insert_error_no_values();
    test_update_all();
    test_update_with_where();
    test_update_int_value();
    test_invalid_statement();
    test_empty_input();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
