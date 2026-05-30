#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif

#include "../src/storage/column.hpp"
#include "../src/storage/row.hpp"
#include "../src/storage/table.hpp"
#include "../src/storage/index.hpp"
#include "../src/storage/database.hpp"

using namespace minidb;
using namespace minidb::storage;

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

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::printf("  FAIL %s:%d — %s\n", __FILE__, __LINE__, msg); \
            ++g_failures; \
        } else { ++g_passed; } \
    } while (0)

#define ASSERT_EQ_INT(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::printf("  FAIL %s:%d — %s: expected %d, got %d\n", \
                __FILE__, __LINE__, msg, (int)(b), (int)(a)); \
            ++g_failures; \
        } else { ++g_passed; } \
    } while (0)

// ============================================================
// Column tests
// ============================================================
TEST(column_basics) {
    Column col(core::String("id"), ColumnType::INT, true);
    ASSERT_TRUE(col.name() == core::String("id"), "column name mismatch");
    ASSERT_TRUE(col.type() == ColumnType::INT, "column type should be INT");
    ASSERT_TRUE(col.is_primary(), "column should be primary");

    Column col2(core::String("name"), ColumnType::STRING);
    ASSERT_TRUE(col2.name() == core::String("name"), "column name mismatch");
    ASSERT_TRUE(col2.type() == ColumnType::STRING, "column type should be STRING");
    ASSERT_TRUE(!col2.is_primary(), "column should not be primary");
}

TEST(column_type_from_string) {
    ASSERT_TRUE(Column::type_from_string("int") == ColumnType::INT, "int parse");
    ASSERT_TRUE(Column::type_from_string("string") == ColumnType::STRING, "string parse");
    ASSERT_TRUE(Column::type_from_string("x") == ColumnType::STRING, "unknown -> string");
}

// ============================================================
// Row / Value tests
// ============================================================
TEST(value_basics) {
    Value vi(42);
    ASSERT_TRUE(vi.is_int(), "should be int");
    ASSERT_TRUE(!vi.is_string(), "should not be string");
    ASSERT_EQ_INT(vi.int_value(), 42, "int value");

    Value vs(core::String("hello"));
    ASSERT_TRUE(vs.is_string(), "should be string");
    ASSERT_TRUE(vs.string_value() == core::String("hello"), "string value");

    Value vn;
    ASSERT_TRUE(vn.is_none(), "default should be none");
}

TEST(value_compare) {
    Value a(10);
    Value b(20);
    Value c(core::String("abc"));
    Value d(core::String("xyz"));
    Value e(10);

    ASSERT_TRUE(Value::compare(a, b) < 0, "10 < 20");
    ASSERT_TRUE(Value::compare(b, a) > 0, "20 > 10");
    ASSERT_TRUE(Value::compare(a, e) == 0, "10 == 10");
    ASSERT_TRUE(Value::compare(c, d) < 0, "'abc' < 'xyz'");
    ASSERT_TRUE(Value::compare(a, c) < 0, "int < string");
}

TEST(row_basics) {
    Row row;
    row.add_value(Value(1001));
    row.add_value(Value(core::String("zhangsan")));
    ASSERT_EQ_INT(static_cast<int>(row.count()), 2, "row count");
    ASSERT_TRUE(row.get_value(0).int_value() == 1001, "first value");
    ASSERT_TRUE(row.get_value(1).string_value() == core::String("zhangsan"), "second value");
}

// ============================================================
// Table tests
// ============================================================
TEST(table_basics) {
    Table t(core::String("student"));
    ASSERT_TRUE(t.name() == core::String("student"), "table name");

    t.add_column(Column(core::String("id"), ColumnType::INT, true));
    t.add_column(Column(core::String("name"), ColumnType::STRING));
    ASSERT_EQ_INT(static_cast<int>(t.column_count()), 2, "column count");
    ASSERT_EQ_INT(t.primary_key_index(), 0, "pk index");

    ASSERT_EQ_INT(t.find_column("id"), 0, "find id");
    ASSERT_EQ_INT(t.find_column("name"), 1, "find name");
    ASSERT_EQ_INT(t.find_column("x"), -1, "find missing");
}

TEST(table_insert_and_query) {
    Table t(core::String("test"));
    t.add_column(Column(core::String("id"), ColumnType::INT, true));
    t.add_column(Column(core::String("name"), ColumnType::STRING));

    Row r1;
    r1.add_value(Value(1001));
    r1.add_value(Value(core::String("peter")));
    t.insert_row(r1);

    Row r2;
    r2.add_value(Value(1002));
    r2.add_value(Value(core::String("mary")));
    t.insert_row(r2);

    ASSERT_EQ_INT(static_cast<int>(t.row_count()), 2, "row count");
    ASSERT_TRUE(t.row_at(0).get_value(0).int_value() == 1001, "row0 id");
    ASSERT_TRUE(t.row_at(0).get_value(1).string_value() == core::String("peter"), "row0 name");
}

TEST(table_save_load) {
    Table t(core::String("test_save"));
    t.add_column(Column(core::String("id"), ColumnType::INT, true));
    t.add_column(Column(core::String("name"), ColumnType::STRING));

    Row r;
    r.add_value(Value(1));
    r.add_value(Value(core::String("hello")));
    t.insert_row(r);

    ASSERT_TRUE(t.save_to_file(), "save");

    Table t2(core::String("test_save"));
    ASSERT_TRUE(t2.load_from_file(), "load");
    ASSERT_EQ_INT(static_cast<int>(t2.column_count()), 2, "loaded columns");
    ASSERT_EQ_INT(static_cast<int>(t2.row_count()), 1, "loaded rows");
    ASSERT_TRUE(t2.row_at(0).get_value(0).int_value() == 1, "loaded id");
    ASSERT_TRUE(t2.row_at(0).get_value(1).string_value() == core::String("hello"), "loaded name");

    // Cleanup
    std::remove("test_save.dat");
}

TEST(table_remove_row) {
    Table t(core::String("test"));
    t.add_column(Column(core::String("id"), ColumnType::INT, true));

    Row r1; r1.add_value(Value(1)); t.insert_row(r1);
    Row r2; r2.add_value(Value(2)); t.insert_row(r2);
    Row r3; r3.add_value(Value(3)); t.insert_row(r3);

    ASSERT_EQ_INT(static_cast<int>(t.row_count()), 3, "before remove");
    t.remove_row(1);
    ASSERT_EQ_INT(static_cast<int>(t.row_count()), 2, "after remove");
    ASSERT_TRUE(t.row_at(0).get_value(0).int_value() == 1, "kept row0");
    ASSERT_TRUE(t.row_at(1).get_value(0).int_value() == 3, "kept row1 shifted");
}

// ============================================================
// Index tests
// ============================================================
TEST(index_basics) {
    Index idx;
    idx.insert(1001, 0);
    idx.insert(1002, 1);
    idx.insert(1000, 2);

    ASSERT_EQ_INT(static_cast<int>(idx.size()), 3, "index size");

    int row_id;
    ASSERT_TRUE(idx.find(1001, row_id), "find 1001");
    ASSERT_EQ_INT(row_id, 0, "row_id for 1001");
    ASSERT_TRUE(idx.find(1000, row_id), "find 1000");
    ASSERT_EQ_INT(row_id, 2, "row_id for 1000");
    ASSERT_TRUE(!idx.find(999, row_id), "not find 999");

    ASSERT_TRUE(idx.remove(1001), "remove 1001");
    ASSERT_EQ_INT(static_cast<int>(idx.size()), 2, "size after remove");
    ASSERT_TRUE(!idx.find(1001, row_id), "should not find removed");
}

TEST(index_update) {
    Index idx;
    idx.insert(10, 5);
    idx.insert(10, 99); // overwrite

    int row_id;
    ASSERT_TRUE(idx.find(10, row_id), "find after overwrite");
    ASSERT_EQ_INT(row_id, 99, "overwritten row_id");
}

// ============================================================
// Database tests
// ============================================================
TEST(database_basics) {
    Database db(core::String("testdb"));
    ASSERT_TRUE(db.name() == core::String("testdb"), "db name");

    core::Vector<Column> cols;
    cols.push_back(Column(core::String("id"), ColumnType::INT, true));
    cols.push_back(Column(core::String("val"), ColumnType::STRING));

    ASSERT_TRUE(db.create_table(core::String("t1"), cols), "create t1");
    ASSERT_TRUE(!db.create_table(core::String("t1"), cols), "duplicate t1");

    Table* t = db.get_table("t1");
    ASSERT_TRUE(t != nullptr, "get t1");
    ASSERT_TRUE(t->name() == core::String("t1"), "table name check");

    ASSERT_TRUE(db.get_table("missing") == nullptr, "missing table");

    ASSERT_TRUE(db.drop_table("t1"), "drop t1");
    ASSERT_TRUE(db.get_table("t1") == nullptr, "t1 gone");
    ASSERT_TRUE(!db.drop_table("t1"), "drop nonexistent");
}

// ============================================================
// Global registry tests
// ============================================================
TEST(global_create_drop_db) {
    ASSERT_TRUE(create_database("test_data", "db1"), "create db1");
    // Creating again should fail (directory exists)
    ASSERT_TRUE(!create_database("test_data", "db1"), "duplicate db1");

    Database* db = use_database("test_data", "db1");
    ASSERT_TRUE(db != nullptr, "use db1");

    ASSERT_TRUE(current_db() == db, "current_db matches");

    ASSERT_TRUE(drop_database("test_data", "db1"), "drop db1");
}

int main() {
#ifdef _WIN32
    _rmdir("test_data\\db1");
    _rmdir("test_data");
#else
    rmdir("test_data/db1");
    rmdir("test_data");
#endif
    std::remove("test_save.dat");
    std::printf("=== Storage Engine Tests ===\n\n");
    // Tests are auto-registered via constructors
    if (g_failures > 0) {
        std::printf("\nFAILED: %d failures, %d passed\n", g_failures, g_passed);
        return 1;
    }
    std::printf("\nOK: %d tests passed\n", g_passed);
    return 0;
}
