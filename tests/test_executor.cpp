#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif

#include "../src/execution/executor.hpp"
#include "../src/parser/parser.hpp"

using namespace minidb;
using namespace minidb::parser;
using namespace minidb::execution;

static int g_failures = 0;
static int g_passed = 0;

#define TEST(name) \
    static void test_##name(); \
    struct _register_##name { \
        _register_##name() { \
            std::printf("  RUN  %s\n", #name); \
            test_##name(); \
        } \
    } _inst_##name; \
    static void test_##name()

#define ASSERT_EQ_INT(a, b, msg) \
    do { \
        int _a = (a); int _b = (b); \
        if (_a != _b) { \
            std::printf("  FAIL %s:%d — %s: expected %d, got %d\n", \
                __FILE__, __LINE__, msg, _b, _a); \
            ++g_failures; \
        } else { ++g_passed; } \
    } while (0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::printf("  FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); \
            ++g_failures; \
        } else { ++g_passed; } \
    } while (0)

#define ASSERT_STR_EQ(a, b, msg) \
    do { \
        if (std::strcmp((a), (b)) != 0) { \
            std::printf("  FAIL %s:%d — %s: expected '%s', got '%s'\n", \
                __FILE__, __LINE__, msg, (b), (a)); \
            ++g_failures; \
        } else { ++g_passed; } \
    } while (0)

// Helper: parse and execute (shared executor to preserve state)
static ExecResult exec(const char* sql) {
    static Executor g_executor("test_exec_data");
    Parser parser;
    SQLStatement stmt = parser.parse(sql);
    if (stmt.kind == StmtKind::INVALID) {
        ExecResult r;
        r.status = ExecResult::ERROR;
        r.message = stmt.error_msg.empty()
            ? core::String("Parse error") : core::String(stmt.error_msg.c_str());
        return r;
    }
    return g_executor.execute(stmt);
}

// ============================================================
TEST(create_database) {
    ExecResult r = exec("create database testdb");
    ASSERT_TRUE(r.status == ExecResult::OK, "create database ok");
}

TEST(drop_database) {
    exec("create database tmpdb");
    ExecResult r = exec("drop database tmpdb");
    ASSERT_TRUE(r.status == ExecResult::OK, "drop database ok");
}

TEST(use_database) {
    exec("create database mydb");
    ExecResult r = exec("use mydb");
    ASSERT_TRUE(r.status == ExecResult::OK, "use database ok");
}

TEST(create_table_no_db) {
    Executor executor("test_exec_data_no_db");
    Parser parser;
    SQLStatement stmt = parser.parse("create table t1 (id int primary, name string)");
    ExecResult r = executor.execute(stmt);
    ASSERT_TRUE(r.status == ExecResult::ERROR, "create table without db should fail");
}

TEST(create_table_with_db) {
    exec("create database testdb");
    exec("use testdb");
    ExecResult r = exec("create table person (id int primary, name string)");
    ASSERT_TRUE(r.status == ExecResult::OK, "create table ok");

    ExecResult r2 = exec("create table person (id int primary, name string)");
    ASSERT_TRUE(r2.status == ExecResult::ERROR, "duplicate table should fail");
}

TEST(drop_table) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table tmp (x int primary)");
    ExecResult r = exec("drop table tmp");
    ASSERT_TRUE(r.status == ExecResult::OK, "drop table ok");

    ExecResult r2 = exec("drop table tmp");
    ASSERT_TRUE(r2.status == ExecResult::ERROR, "drop nonexistent table");
}

TEST(insert) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");

    ExecResult r = exec("insert into person values (1001, \"peter\")");
    ASSERT_TRUE(r.status == ExecResult::OK, "insert ok");
}

TEST(insert_duplicate_pk) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");

    ExecResult r = exec("insert into person values (1001, \"mary\")");
    ASSERT_TRUE(r.status == ExecResult::ERROR, "duplicate pk should fail");
}

TEST(select_all) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");

    ExecResult r = exec("select * from person");
    ASSERT_TRUE(r.status == ExecResult::OK, "select * ok");
    ASSERT_TRUE(r.result_table != nullptr, "result table not null");
    ASSERT_EQ_INT(static_cast<int>(r.result_table->column_count()), 2, "result columns");
    ASSERT_EQ_INT(static_cast<int>(r.result_table->row_count()), 2, "result rows");
}

TEST(select_single_column) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");

    ExecResult r = exec("select name from person");
    ASSERT_TRUE(r.status == ExecResult::OK, "select name ok");
    ASSERT_EQ_INT(static_cast<int>(r.result_table->column_count()), 1, "1 column");
    ASSERT_EQ_INT(static_cast<int>(r.result_table->row_count()), 2, "2 rows");
    ASSERT_TRUE(r.result_table->row_at(0).get_value(0).string_value()
        == core::String("peter"), "first row name");
}

TEST(select_where) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");
    exec("insert into person values (1003, \"john\")");

    ExecResult r = exec("select name from person where id = 1002");
    ASSERT_TRUE(r.status == ExecResult::OK, "select where ok");
    ASSERT_EQ_INT(static_cast<int>(r.result_table->row_count()), 1, "1 matched row");
}

TEST(delete_all) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");

    ExecResult r = exec("delete from person");
    ASSERT_TRUE(r.status == ExecResult::OK, "delete all ok");
}

TEST(delete_where) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");
    exec("insert into person values (1003, \"john\")");

    ExecResult r = exec("delete from person where id = 1002");
    ASSERT_TRUE(r.status == ExecResult::OK, "delete where ok");

    // Verify: only 2 rows left
    ExecResult r2 = exec("select * from person");
    ASSERT_EQ_INT(static_cast<int>(r2.result_table->row_count()), 2, "2 rows after delete");
}

TEST(update) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");

    ExecResult r = exec("update person set name = \"peter2\" where id = 1001");
    ASSERT_TRUE(r.status == ExecResult::OK, "update ok");

    ExecResult r2 = exec("select name from person where id = 1001");
    ASSERT_TRUE(r2.result_table->row_at(0).get_value(0).string_value()
        == core::String("peter2"), "updated value");
}

TEST(update_all) {
    exec("create database testdb");
    exec("use testdb");
    exec("create table person (id int primary, name string)");
    exec("insert into person values (1001, \"peter\")");
    exec("insert into person values (1002, \"mary\")");

    ExecResult r = exec("update person set name = \"anonymous\"");
    ASSERT_TRUE(r.status == ExecResult::OK, "update all ok");

    ExecResult r2 = exec("select name from person");
    ASSERT_TRUE(r2.result_table->row_at(0).get_value(0).string_value()
        == core::String("anonymous"), "row0 updated");
    ASSERT_TRUE(r2.result_table->row_at(1).get_value(0).string_value()
        == core::String("anonymous"), "row1 updated");
}

int main() {
    // Clean up test data from previous runs
#ifdef _WIN32
    _rmdir("test_exec_data\\testdb");
    _rmdir("test_exec_data\\mydb");
    _rmdir("test_exec_data");
    _rmdir("test_exec_data_no_db");
#else
    rmdir("test_exec_data/testdb");
    rmdir("test_exec_data/mydb");
    rmdir("test_exec_data");
    rmdir("test_exec_data_no_db");
#endif
    std::printf("=== Executor Tests ===\n\n");
    // Tests are auto-registered via constructors
    if (g_failures > 0) {
        std::printf("\nFAILED: %d failures, %d passed\n", g_failures, g_passed);
        return 1;
    }
    std::printf("\nOK: %d tests passed\n", g_passed);
    return 0;
}
