#include "parser.hpp"
#include "lexer.hpp"
#include <cstdio>

namespace minidb {
namespace parser {

// ============================================================
// Parser implementation
// ============================================================

Parser::Parser() : _tokens(nullptr), _pos(0) {}

SQLStatement Parser::parse(const core::Vector<Token>& tokens) {
    _tokens = &tokens;
    _pos = 0;
    return parse_statement();
}

SQLStatement Parser::parse(const char* sql) {
    Lexer lexer;
    auto tokens = lexer.tokenize(sql);
    if (lexer.has_error()) {
        SQLStatement stmt;
        stmt.error_msg = lexer.error_msg();
        return stmt;
    }
    return parse(tokens);
}

// ============================================================
// Helpers
// ============================================================

const Token& Parser::peek() const {
    return (*_tokens)[_pos];
}

const Token& Parser::advance() {
    return (*_tokens)[_pos++];
}

bool Parser::check(TokenKind kind) const {
    if (is_at_end()) return false;
    return peek().kind == kind;
}

Token Parser::expect(TokenKind kind) {
    if (check(kind)) {
        return advance();
    }
    // Error: build message
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "Line %zu:%zu: expected '%s' but got '%s'",
        peek().line, peek().column,
        token_kind_name(kind), token_kind_name(peek().kind));
    SQLStatement stmt;
    stmt.error_msg = core::String(buf);
    return Token(); // caller checks error_msg
}

bool Parser::is_at_end() const {
    return peek().kind == TokenKind::END_OF_FILE;
}

SQLStatement Parser::make_error(const char* msg) const {
    SQLStatement stmt;
    char buf[512];
    const Token& tok = peek();
    std::snprintf(buf, sizeof(buf), "Line %zu:%zu: %s", tok.line, tok.column, msg);
    stmt.error_msg = core::String(buf);
    return stmt;
}

// ============================================================
// Statement dispatcher
// ============================================================

SQLStatement Parser::parse_statement() {
    if (is_at_end()) {
        return make_error("Empty input, expected an SQL statement");
    }

    TokenKind kind = peek().kind;
    switch (kind) {
    case TokenKind::KW_CREATE:
        return parse_create();
    case TokenKind::KW_DROP:
        return parse_drop();
    case TokenKind::KW_USE:
        return parse_use();
    case TokenKind::KW_SELECT:
        return parse_select();
    case TokenKind::KW_DELETE:
        return parse_delete();
    case TokenKind::KW_INSERT:
        return parse_insert();
    case TokenKind::KW_UPDATE:
        return parse_update();
    default:
        return make_error("Unexpected token, expected a SQL statement keyword");
    }
}

// ============================================================
// CREATE DATABASE / CREATE TABLE
// ============================================================

SQLStatement Parser::parse_create() {
    advance(); // consume KW_CREATE

    if (!check(TokenKind::KW_DATABASE) && !check(TokenKind::KW_TABLE)) {
        return make_error("Expected 'DATABASE' or 'TABLE' after 'CREATE'");
    }

    if (check(TokenKind::KW_DATABASE)) {
        advance(); // consume KW_DATABASE

        if (!check(TokenKind::IDENTIFIER)) {
            return make_error("Expected database name after 'CREATE DATABASE'");
        }
        Token name_tok = advance();

        // Optional semicolon
        if (check(TokenKind::SEMICOLON)) advance();

        SQLStatement stmt;
        stmt.kind = StmtKind::CREATE_DATABASE;
        stmt.create_database.database_name = name_tok.lexeme;
        return stmt;
    }

    // CREATE TABLE
    advance(); // consume KW_TABLE

    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'CREATE TABLE'");
    }
    Token table_tok = advance();

    if (!check(TokenKind::LPAREN)) {
        return make_error("Expected '(' after table name in CREATE TABLE");
    }
    advance(); // consume '('

    SQLStatement stmt;
    stmt.kind = StmtKind::CREATE_TABLE;
    stmt.create_table.table_name = table_tok.lexeme;

    // Parse column definitions
    if (check(TokenKind::RPAREN)) {
        return make_error("Expected at least one column definition in CREATE TABLE");
    }

    while (true) {
        ColumnDef col = parse_column_def();
        if (col.name.empty()) {
            return make_error("Expected column name in CREATE TABLE");
        }
        stmt.create_table.columns.push_back(std::move(col));

        if (check(TokenKind::RPAREN)) {
            advance(); // consume ')'
            break;
        }
        if (check(TokenKind::COMMA)) {
            advance(); // consume ','
            continue;
        }
        return make_error("Expected ',' or ')' in column list");
    }

    // Check for duplicate primary keys
    int pk_count = 0;
    for (std::size_t i = 0; i < stmt.create_table.columns.size(); ++i) {
        if (stmt.create_table.columns[i].is_primary) pk_count++;
    }
    if (pk_count > 1) {
        return make_error("Only one primary key column is allowed per table");
    }

    // Optional semicolon
    if (check(TokenKind::SEMICOLON)) advance();

    return stmt;
}

// ============================================================
// DROP DATABASE / DROP TABLE
// ============================================================

SQLStatement Parser::parse_drop() {
    advance(); // consume KW_DROP

    if (!check(TokenKind::KW_DATABASE) && !check(TokenKind::KW_TABLE)) {
        return make_error("Expected 'DATABASE' or 'TABLE' after 'DROP'");
    }

    if (check(TokenKind::KW_DATABASE)) {
        advance(); // consume KW_DATABASE

        if (!check(TokenKind::IDENTIFIER)) {
            return make_error("Expected database name after 'DROP DATABASE'");
        }
        Token name_tok = advance();

        if (check(TokenKind::SEMICOLON)) advance();

        SQLStatement stmt;
        stmt.kind = StmtKind::DROP_DATABASE;
        stmt.drop_database.database_name = name_tok.lexeme;
        return stmt;
    }

    // DROP TABLE
    advance(); // consume KW_TABLE

    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'DROP TABLE'");
    }
    Token table_tok = advance();

    if (check(TokenKind::SEMICOLON)) advance();

    SQLStatement stmt;
    stmt.kind = StmtKind::DROP_TABLE;
    stmt.drop_table.table_name = table_tok.lexeme;
    return stmt;
}

// ============================================================
// USE
// ============================================================

SQLStatement Parser::parse_use() {
    advance(); // consume KW_USE

    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected database name after 'USE'");
    }
    Token name_tok = advance();

    if (check(TokenKind::SEMICOLON)) advance();

    SQLStatement stmt;
    stmt.kind = StmtKind::USE;
    stmt.use_stmt.database_name = name_tok.lexeme;
    return stmt;
}

// ============================================================
// SELECT
// ============================================================

SQLStatement Parser::parse_select() {
    advance(); // consume KW_SELECT

    // <column> : <column-name> | '*'
    Token col_tok;
    if (check(TokenKind::IDENTIFIER)) {
        col_tok = advance();
    } else if (check(TokenKind::OP_STAR)) {
        col_tok = advance();
    } else {
        return make_error("Expected column name or '*' after SELECT");
    }

    // FROM
    if (!check(TokenKind::KW_FROM)) {
        return make_error("Expected 'FROM' after column in SELECT");
    }
    advance();

    // table name
    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'FROM'");
    }
    Token table_tok = advance();

    SQLStatement stmt;
    stmt.kind = StmtKind::SELECT;
    stmt.select_stmt.column_name = col_tok.lexeme;
    stmt.select_stmt.table_name = table_tok.lexeme;
    stmt.select_stmt.has_where = false;

    // Optional WHERE
    if (check(TokenKind::KW_WHERE)) {
        WhereClause wc = parse_where_clause();
        if (!wc.column.empty()) {
            stmt.select_stmt.has_where = true;
            stmt.select_stmt.where = std::move(wc);
        } else {
            stmt.error_msg = "Error parsing WHERE clause";
            stmt.kind = StmtKind::INVALID;
            return stmt;
        }
    }

    if (check(TokenKind::SEMICOLON)) advance();

    return stmt;
}

// ============================================================
// DELETE
// ============================================================

SQLStatement Parser::parse_delete() {
    advance(); // consume KW_DELETE

    // FROM
    if (!check(TokenKind::KW_FROM)) {
        return make_error("Expected 'FROM' after 'DELETE'");
    }
    advance();

    // table name
    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'DELETE FROM'");
    }
    Token table_tok = advance();

    SQLStatement stmt;
    stmt.kind = StmtKind::DELETE;
    stmt.delete_stmt.table_name = table_tok.lexeme;
    stmt.delete_stmt.has_where = false;

    if (check(TokenKind::KW_WHERE)) {
        WhereClause wc = parse_where_clause();
        if (!wc.column.empty()) {
            stmt.delete_stmt.has_where = true;
            stmt.delete_stmt.where = std::move(wc);
        } else {
            stmt.error_msg = "Error parsing WHERE clause";
            stmt.kind = StmtKind::INVALID;
            return stmt;
        }
    }

    if (check(TokenKind::SEMICOLON)) advance();

    return stmt;
}

// ============================================================
// INSERT
// ============================================================

SQLStatement Parser::parse_insert() {
    advance(); // consume KW_INSERT

    // INTO
    if (!check(TokenKind::KW_INTO)) {
        return make_error("Expected 'INTO' after 'INSERT'");
    }
    advance();

    // table name
    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'INSERT INTO'");
    }
    Token table_tok = advance();

    // VALUES
    if (!check(TokenKind::KW_VALUES)) {
        return make_error("Expected 'VALUES' after table name in INSERT");
    }
    advance();

    // (
    if (!check(TokenKind::LPAREN)) {
        return make_error("Expected '(' after 'VALUES'");
    }
    advance();

    SQLStatement stmt;
    stmt.kind = StmtKind::INSERT;
    stmt.insert_stmt.table_name = table_tok.lexeme;

    // Parse value list
    if (check(TokenKind::RPAREN)) {
        return make_error("Expected at least one value in INSERT VALUES");
    }

    while (true) {
        bool is_int = false;
        core::String val = parse_value_string(is_int);
        stmt.insert_stmt.values.push_back(std::move(val));
        stmt.insert_stmt.is_int.push_back(is_int);

        if (check(TokenKind::RPAREN)) {
            advance(); // consume ')'
            break;
        }
        if (check(TokenKind::COMMA)) {
            advance();
            continue;
        }
        return make_error("Expected ',' or ')' in VALUES list");
    }

    if (check(TokenKind::SEMICOLON)) advance();

    return stmt;
}

// ============================================================
// UPDATE
// ============================================================

SQLStatement Parser::parse_update() {
    advance(); // consume KW_UPDATE

    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected table name after 'UPDATE'");
    }
    Token table_tok = advance();

    // SET
    if (!check(TokenKind::KW_SET)) {
        return make_error("Expected 'SET' after table name in UPDATE");
    }
    advance();

    // column name
    if (!check(TokenKind::IDENTIFIER)) {
        return make_error("Expected column name after 'SET'");
    }
    Token col_tok = advance();

    // =
    if (!check(TokenKind::OP_EQ)) {
        return make_error("Expected '=' after column name in SET clause");
    }
    advance();

    // value
    bool set_is_int = false;
    core::String set_val = parse_value_string(set_is_int);

    SQLStatement stmt;
    stmt.kind = StmtKind::UPDATE;
    stmt.update_stmt.table_name = table_tok.lexeme;
    stmt.update_stmt.set_column = col_tok.lexeme;
    stmt.update_stmt.set_value = std::move(set_val);
    stmt.update_stmt.set_is_int = set_is_int;
    stmt.update_stmt.has_where = false;

    if (check(TokenKind::KW_WHERE)) {
        WhereClause wc = parse_where_clause();
        if (!wc.column.empty()) {
            stmt.update_stmt.has_where = true;
            stmt.update_stmt.where = std::move(wc);
        } else {
            stmt.error_msg = "Error parsing WHERE clause";
            stmt.kind = StmtKind::INVALID;
            return stmt;
        }
    }

    if (check(TokenKind::SEMICOLON)) advance();

    return stmt;
}

// ============================================================
// Column definition parser
// ============================================================

ColumnDef Parser::parse_column_def() {
    // column name
    if (!check(TokenKind::IDENTIFIER)) {
        return ColumnDef(); // caller should handle error
    }
    Token name_tok = advance();

    // type: int or string
    if (!check(TokenKind::KW_INT) && !check(TokenKind::KW_STRING)) {
        return ColumnDef();
    }
    Token type_tok = advance();

    ColumnDef col;
    col.name = name_tok.lexeme;
    col.type = type_tok.lexeme;
    col.is_primary = false;

    // Optional PRIMARY
    if (check(TokenKind::KW_PRIMARY)) {
        advance();
        col.is_primary = true;
    }

    return col;
}

// ============================================================
// WHERE clause parser
// ============================================================

WhereClause Parser::parse_where_clause() {
    advance(); // consume KW_WHERE

    WhereClause wc;

    // column name
    if (!check(TokenKind::IDENTIFIER)) {
        return WhereClause(); // empty = error
    }
    Token col_tok = advance();
    wc.column = col_tok.lexeme;

    // operator: =, <, >
    if (!check(TokenKind::OP_EQ) && !check(TokenKind::OP_LT) && !check(TokenKind::OP_GT)) {
        return WhereClause();
    }
    Token op_tok = advance();
    wc.op = op_tok.lexeme;

    // const-value
    bool is_int_lit = false;
    wc.const_value = parse_value_string(is_int_lit);
    wc.is_int_literal = is_int_lit;

    return wc;
}

// ============================================================
// Value parser (int literal or string literal)
// ============================================================

core::String Parser::parse_value_string(bool& is_int) {
    if (check(TokenKind::INT_LITERAL)) {
        Token tok = advance();
        is_int = true;
        // The lexeme for INT_LITERAL is the numeric string
        // We also have int_value; we return the lexeme
        return tok.lexeme;
    }

    if (check(TokenKind::STRING_LITERAL)) {
        Token tok = advance();
        is_int = false;
        // lexeme already has the content without quotes
        return tok.lexeme;
    }

    // Invalid value
    is_int = false;
    return core::String();
}

} // namespace parser
} // namespace minidb
