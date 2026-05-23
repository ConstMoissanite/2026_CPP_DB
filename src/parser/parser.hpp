#ifndef MINIDB_PARSER_PARSER_HPP
#define MINIDB_PARSER_PARSER_HPP

#include "ast.hpp"
#include "token.hpp"
#include "../core/vector.hpp"
#include "../core/string.hpp"

namespace minidb {
namespace parser {

// Recursive-descent SQL parser.
// Consumes tokens produced by the Lexer and builds a SQLStatement AST.
class Parser {
public:
    Parser();

    // Parse a sequence of tokens and return a SQLStatement.
    // On success, kind != INVALID and error_msg is empty.
    // On failure, kind == INVALID and error_msg describes the error.
    SQLStatement parse(const core::Vector<Token>& tokens);

    // Convenience: lex + parse in one call.
    // The Lexer is included via the parser header for convenience.
    SQLStatement parse(const char* sql);

private:
    // Core helpers
    const Token& peek() const;
    const Token& advance();
    bool check(TokenKind kind) const;
    Token expect(TokenKind kind);
    bool is_at_end() const;

    // Error
    SQLStatement make_error(const char* msg) const;

    // Statement parsers
    SQLStatement parse_statement();
    SQLStatement parse_create();
    SQLStatement parse_drop();
    SQLStatement parse_use();
    SQLStatement parse_select();
    SQLStatement parse_delete();
    SQLStatement parse_insert();
    SQLStatement parse_update();

    // Sub-parsers
    ColumnDef parse_column_def();
    WhereClause parse_where_clause();
    core::String parse_value_string(bool& is_int);

    const core::Vector<Token>* _tokens;
    std::size_t _pos;
};

} // namespace parser
} // namespace minidb

#endif // MINIDB_PARSER_PARSER_HPP
