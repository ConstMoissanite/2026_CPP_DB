// ============================================================
// Unit tests for the MiniDB Lexer
// ============================================================
#include "parser/lexer.hpp"
#include "parser/token.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace minidb::parser;
using namespace minidb::core;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  TEST %s ... ", name);
#define END_TEST() \
        printf("PASSED\n"); \
        tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { \
        printf("FAILED: %s\n", msg); \
        tests_failed++; \
    } while(0)

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { FAIL(msg); return; }
#define ASSERT_EQ_STR(a, b, msg) \
    if (std::strcmp((a), (b)) != 0) { \
        printf("FAILED: %s (expected '%s', got '%s')\n", msg, b, a); \
        tests_failed++; return; \
    }

// ============================================================
// Helper: tokenize and return token list
// ============================================================
static Vector<Token> tokenize(const char* sql) {
    Lexer lexer;
    auto tokens = lexer.tokenize(sql);
    if (lexer.has_error()) {
        printf("  LEXER ERROR: %s\n", lexer.error_msg().c_str());
    }
    return tokens;
}

// ============================================================
// Test: Keywords
// ============================================================
static void test_keywords() {
    TEST("keywords")
    {
        auto tokens = tokenize("create drop use table select delete insert update");
        ASSERT_TRUE(tokens.size() >= 8, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_CREATE, "create");
        ASSERT_TRUE(tokens[1].kind == TokenKind::KW_DROP, "drop");
        ASSERT_TRUE(tokens[2].kind == TokenKind::KW_USE, "use");
        ASSERT_TRUE(tokens[3].kind == TokenKind::KW_TABLE, "table");
        ASSERT_TRUE(tokens[4].kind == TokenKind::KW_SELECT, "select");
        ASSERT_TRUE(tokens[5].kind == TokenKind::KW_DELETE, "delete");
        ASSERT_TRUE(tokens[6].kind == TokenKind::KW_INSERT, "insert");
        ASSERT_TRUE(tokens[7].kind == TokenKind::KW_UPDATE, "update");
    }
    END_TEST();
}

// ============================================================
// Test: Case insensitivity
// ============================================================
static void test_case_insensitivity() {
    TEST("case insensitivity")
    {
        auto tokens = tokenize("CREATE DATABASE");
        ASSERT_TRUE(tokens.size() >= 3, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_CREATE, "CREATE");
        ASSERT_TRUE(tokens[1].kind == TokenKind::KW_DATABASE, "DATABASE");
    }
    END_TEST();

    TEST("mixed case")
    {
        auto tokens = tokenize("Select From Where");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_SELECT, "Select");
        ASSERT_TRUE(tokens[1].kind == TokenKind::KW_FROM, "From");
        ASSERT_TRUE(tokens[2].kind == TokenKind::KW_WHERE, "Where");
    }
    END_TEST();
}

// ============================================================
// Test: Identifiers
// ============================================================
static void test_identifiers() {
    TEST("identifiers")
    {
        auto tokens = tokenize("person id name");
        ASSERT_TRUE(tokens.size() >= 4, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::IDENTIFIER, "person");
        ASSERT_EQ_STR(tokens[0].lexeme.c_str(), "person", "person lexeme");
        ASSERT_TRUE(tokens[1].kind == TokenKind::IDENTIFIER, "id");
        ASSERT_EQ_STR(tokens[1].lexeme.c_str(), "id", "id lexeme");
    }
    END_TEST();
}

// ============================================================
// Test: Integer literals
// ============================================================
static void test_integer_literals() {
    TEST("integer literals")
    {
        auto tokens = tokenize("1001 0 42");
        ASSERT_TRUE(tokens.size() >= 4, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::INT_LITERAL, "1001 kind");
        ASSERT_TRUE(tokens[0].int_value == 1001, "1001 value");
        ASSERT_TRUE(tokens[1].kind == TokenKind::INT_LITERAL, "0 kind");
        ASSERT_TRUE(tokens[1].int_value == 0, "0 value");
        ASSERT_TRUE(tokens[2].kind == TokenKind::INT_LITERAL, "42 kind");
        ASSERT_TRUE(tokens[2].int_value == 42, "42 value");
    }
    END_TEST();
}

// ============================================================
// Test: String literals
// ============================================================
static void test_string_literals() {
    TEST("string literals")
    {
        auto tokens = tokenize("\"peter\" \"hello world\"");
        ASSERT_TRUE(tokens.size() >= 3, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::STRING_LITERAL, "first string kind");
        ASSERT_EQ_STR(tokens[0].lexeme.c_str(), "peter", "first string value");
        ASSERT_TRUE(tokens[1].kind == TokenKind::STRING_LITERAL, "second string kind");
        ASSERT_EQ_STR(tokens[1].lexeme.c_str(), "hello world", "second string value");
    }
    END_TEST();
}

// ============================================================
// Test: Operators
// ============================================================
static void test_operators() {
    TEST("operators = < > *")
    {
        auto tokens = tokenize("= < > *");
        ASSERT_TRUE(tokens.size() >= 5, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::OP_EQ, "=");
        ASSERT_TRUE(tokens[1].kind == TokenKind::OP_LT, "<");
        ASSERT_TRUE(tokens[2].kind == TokenKind::OP_GT, ">");
        ASSERT_TRUE(tokens[3].kind == TokenKind::OP_STAR, "*");
    }
    END_TEST();
}

// ============================================================
// Test: Punctuation
// ============================================================
static void test_punctuation() {
    TEST("punctuation")
    {
        auto tokens = tokenize("( ) , ;");
        ASSERT_TRUE(tokens.size() >= 5, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::LPAREN, "(");
        ASSERT_TRUE(tokens[1].kind == TokenKind::RPAREN, ")");
        ASSERT_TRUE(tokens[2].kind == TokenKind::COMMA, ",");
        ASSERT_TRUE(tokens[3].kind == TokenKind::SEMICOLON, ";");
    }
    END_TEST();
}

// ============================================================
// Test: Complete SQL statements
// ============================================================
static void test_create_table_tokens() {
    TEST("create table tokens")
    {
        auto tokens = tokenize("create table person (id int primary, name string)");
        ASSERT_TRUE(tokens.size() >= 12, "token count");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_CREATE, "create");
        ASSERT_TRUE(tokens[1].kind == TokenKind::KW_TABLE, "table");
        ASSERT_TRUE(tokens[2].kind == TokenKind::IDENTIFIER, "person");
        ASSERT_TRUE(tokens[3].kind == TokenKind::LPAREN, "(");
        ASSERT_TRUE(tokens[4].kind == TokenKind::IDENTIFIER, "id");
        ASSERT_TRUE(tokens[5].kind == TokenKind::KW_INT, "int");
        ASSERT_TRUE(tokens[6].kind == TokenKind::KW_PRIMARY, "primary");
        ASSERT_TRUE(tokens[7].kind == TokenKind::COMMA, ",");
        ASSERT_TRUE(tokens[8].kind == TokenKind::IDENTIFIER, "name");
        ASSERT_TRUE(tokens[9].kind == TokenKind::KW_STRING, "string");
        ASSERT_TRUE(tokens[10].kind == TokenKind::RPAREN, ")");
    }
    END_TEST();
}

static void test_select_tokens() {
    TEST("select tokens")
    {
        auto tokens = tokenize("select name from person where id = 1001");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_SELECT, "select");
        ASSERT_TRUE(tokens[1].kind == TokenKind::IDENTIFIER, "name");
        ASSERT_TRUE(tokens[2].kind == TokenKind::KW_FROM, "from");
        ASSERT_TRUE(tokens[3].kind == TokenKind::IDENTIFIER, "person");
        ASSERT_TRUE(tokens[4].kind == TokenKind::KW_WHERE, "where");
        ASSERT_TRUE(tokens[5].kind == TokenKind::IDENTIFIER, "id");
        ASSERT_TRUE(tokens[6].kind == TokenKind::OP_EQ, "=");
        ASSERT_TRUE(tokens[7].kind == TokenKind::INT_LITERAL, "1001");
        ASSERT_TRUE(tokens[7].int_value == 1001, "1001 value");
    }
    END_TEST();
}

static void test_insert_tokens() {
    TEST("insert tokens")
    {
        auto tokens = tokenize("insert into person values (1001, \"peter\")");
        ASSERT_TRUE(tokens[0].kind == TokenKind::KW_INSERT, "insert");
        ASSERT_TRUE(tokens[1].kind == TokenKind::KW_INTO, "into");
        ASSERT_TRUE(tokens[2].kind == TokenKind::IDENTIFIER, "person");
        ASSERT_TRUE(tokens[3].kind == TokenKind::KW_VALUES, "values");
        ASSERT_TRUE(tokens[4].kind == TokenKind::LPAREN, "(");
        ASSERT_TRUE(tokens[5].kind == TokenKind::INT_LITERAL, "1001");
        ASSERT_TRUE(tokens[6].kind == TokenKind::COMMA, ",");
        ASSERT_TRUE(tokens[7].kind == TokenKind::STRING_LITERAL, "peter");
        ASSERT_TRUE(tokens[8].kind == TokenKind::RPAREN, ")");
    }
    END_TEST();
}

// ============================================================
// Test: Empty input
// ============================================================
static void test_empty_input() {
    TEST("empty input")
    {
        auto tokens = tokenize("");
        ASSERT_TRUE(tokens.size() == 1, "should have one token (EOF)");
        ASSERT_TRUE(tokens[0].kind == TokenKind::END_OF_FILE, "EOF");
    }
    END_TEST();
}

// ============================================================
// Test: Unterminated string
// ============================================================
static void test_unterminated_string() {
    TEST("unterminated string")
    {
        Lexer lexer;
        auto tokens = lexer.tokenize("\"hello");
        ASSERT_TRUE(lexer.has_error(), "should have error");
        ASSERT_TRUE(tokens.size() > 0, "should have tokens");
        // The INVALID token should be present
        bool has_invalid = false;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].kind == TokenKind::INVALID) { has_invalid = true; break; }
        }
        ASSERT_TRUE(has_invalid, "should have INVALID token");
    }
    END_TEST();
}

// ============================================================
// Test: Line/column tracking
// ============================================================
static void test_line_column() {
    TEST("line/column tracking")
    {
        auto tokens = tokenize("select\n*\nfrom t");
        ASSERT_TRUE(tokens[0].line == 1, "select line");
        ASSERT_TRUE(tokens[0].column == 1, "select col");
        ASSERT_TRUE(tokens[1].line == 2, "* line");
        ASSERT_TRUE(tokens[1].column == 1, "* col");
        ASSERT_TRUE(tokens[2].line == 3, "from line");
        ASSERT_TRUE(tokens[2].column == 1, "from col");
    }
    END_TEST();
}

// ============================================================
// Main
// ============================================================
int main() {
    printf("=== MiniDB Lexer Tests ===\n\n");

    test_keywords();
    test_case_insensitivity();
    test_identifiers();
    test_integer_literals();
    test_string_literals();
    test_operators();
    test_punctuation();
    test_create_table_tokens();
    test_select_tokens();
    test_insert_tokens();
    test_empty_input();
    test_unterminated_string();
    test_line_column();

    printf("\n=== Results: %d run, %d passed, %d failed ===\n",
           tests_run, tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
