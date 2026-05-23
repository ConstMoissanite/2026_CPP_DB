#ifndef MINIDB_PARSER_LEXER_HPP
#define MINIDB_PARSER_LEXER_HPP

#include "token.hpp"
#include "../core/vector.hpp"
#include "../core/string.hpp"

namespace minidb {
namespace parser {

// Converts an SQL string into a sequence of tokens.
class Lexer {
public:
    Lexer();

    // Tokenize the input SQL string. Returns the list of tokens.
    // The last token is always END_OF_FILE.
    // On error, sets _error flag and _error_msg.
    core::Vector<Token> tokenize(const char* sql);

    bool has_error() const { return _has_error; }
    const core::String& error_msg() const { return _error_msg; }

private:
    void skip_whitespace();
    Token scan_token();
    Token scan_identifier_or_keyword();
    Token scan_number();
    Token scan_string();

    // Check if an identifier string matches a keyword
    static TokenKind match_keyword(const char* s, std::size_t len);

    const char* _src;
    std::size_t _pos;
    std::size_t _line;
    std::size_t _col;

    bool _has_error;
    core::String _error_msg;
};

} // namespace parser
} // namespace minidb

#endif // MINIDB_PARSER_LEXER_HPP
