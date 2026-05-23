#ifndef MINIDB_PARSER_TOKEN_HPP
#define MINIDB_PARSER_TOKEN_HPP

#include "../core/string.hpp"
#include <cstddef>

namespace minidb {
namespace parser {

enum class TokenKind {
    // Keywords
    KW_CREATE,
    KW_DATABASE,
    KW_DROP,
    KW_USE,
    KW_TABLE,
    KW_SELECT,
    KW_FROM,
    KW_WHERE,
    KW_DELETE,
    KW_INSERT,
    KW_INTO,
    KW_VALUES,
    KW_UPDATE,
    KW_SET,
    KW_PRIMARY,
    KW_INT,
    KW_STRING,

    // Identifiers and literals
    IDENTIFIER,
    INT_LITERAL,
    STRING_LITERAL,

    // Operators
    OP_EQ,      // =
    OP_LT,      // <
    OP_GT,      // >
    OP_STAR,    // *

    // Punctuation
    LPAREN,     // (
    RPAREN,     // )
    COMMA,      // ,
    SEMICOLON,  // ;

    // Special
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenKind kind;
    core::String lexeme;  // the raw text of the token
    std::size_t line;
    std::size_t column;

    // For INT_LITERAL
    int int_value;

    Token() : kind(TokenKind::INVALID), line(0), column(0), int_value(0) {}
};

// Return a human-readable name for a token kind
const char* token_kind_name(TokenKind kind);

} // namespace parser
} // namespace minidb

#endif // MINIDB_PARSER_TOKEN_HPP
