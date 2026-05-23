#include "lexer.hpp"
#include <cstring>
#include <cctype>

namespace minidb {
namespace parser {

Lexer::Lexer()
    : _src(nullptr), _pos(0), _line(1), _col(1), _has_error(false) {}

core::Vector<Token> Lexer::tokenize(const char* sql) {
    core::Vector<Token> tokens;
    _src = sql;
    _pos = 0;
    _line = 1;
    _col = 1;
    _has_error = false;
    _error_msg = "";

    if (!sql || *sql == '\0') {
        Token eof;
        eof.kind = TokenKind::END_OF_FILE;
        eof.line = _line;
        eof.column = _col;
        tokens.push_back(std::move(eof));
        return tokens;
    }

    while (_src[_pos] != '\0' && !_has_error) {
        skip_whitespace();
        if (_src[_pos] == '\0') break;

        Token tok = scan_token();
        tokens.push_back(std::move(tok));
        if (_has_error) break;
    }

    Token eof;
    eof.kind = TokenKind::END_OF_FILE;
    eof.line = _line;
    eof.column = _col;
    tokens.push_back(std::move(eof));

    return tokens;
}

void Lexer::skip_whitespace() {
    while (_src[_pos] != '\0') {
        char c = _src[_pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++_pos;
            ++_col;
        } else if (c == '\n') {
            ++_pos;
            ++_line;
            _col = 1;
        } else {
            break;
        }
    }
}

Token Lexer::scan_token() {
    char c = _src[_pos];

    // Punctuation and operators
    switch (c) {
    case '(': {
        Token tok;
        tok.kind = TokenKind::LPAREN;
        tok.lexeme = core::String("(");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case ')': {
        Token tok;
        tok.kind = TokenKind::RPAREN;
        tok.lexeme = core::String(")");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case ',': {
        Token tok;
        tok.kind = TokenKind::COMMA;
        tok.lexeme = core::String(",");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case ';': {
        Token tok;
        tok.kind = TokenKind::SEMICOLON;
        tok.lexeme = core::String(";");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case '=': {
        Token tok;
        tok.kind = TokenKind::OP_EQ;
        tok.lexeme = core::String("=");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case '<': {
        Token tok;
        tok.kind = TokenKind::OP_LT;
        tok.lexeme = core::String("<");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case '>': {
        Token tok;
        tok.kind = TokenKind::OP_GT;
        tok.lexeme = core::String(">");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case '*': {
        Token tok;
        tok.kind = TokenKind::OP_STAR;
        tok.lexeme = core::String("*");
        tok.line = _line;
        tok.column = _col;
        ++_pos; ++_col;
        return tok;
    }
    case '\"':
        return scan_string();
    default:
        break;
    }

    // Numbers
    if (c >= '0' && c <= '9') {
        return scan_number();
    }

    // Identifiers / keywords (start with letter or underscore)
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        return scan_identifier_or_keyword();
    }

    // Invalid character
    _has_error = true;
    _error_msg = core::String("Unexpected character: '");
    _error_msg = core::String(_error_msg);  // copy
    // Build error message manually (no operator+)
    {
        char buf[64];
        int n = 0;
        const char* prefix = "Unexpected character: '";
        while (prefix[n]) { buf[n] = prefix[n]; ++n; }
        buf[n++] = c;
        buf[n++] = '\'';
        buf[n] = '\0';
        _error_msg = core::String(buf);
    }
    Token tok;
    tok.kind = TokenKind::INVALID;
    tok.line = _line;
    tok.column = _col;
    return tok;
}

Token Lexer::scan_identifier_or_keyword() {
    std::size_t start = _pos;
    std::size_t start_col = _col;

    char c = _src[_pos];
    while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
        ++_pos; ++_col;
        c = _src[_pos];
    }

    std::size_t len = _pos - start;

    Token tok;
    tok.lexeme = core::String(_src + start, len);
    tok.line = _line;
    tok.column = start_col;

    TokenKind kw = match_keyword(_src + start, len);
    if (kw != TokenKind::IDENTIFIER) {
        tok.kind = kw;
    } else {
        tok.kind = TokenKind::IDENTIFIER;
    }
    return tok;
}

Token Lexer::scan_number() {
    std::size_t start = _pos;
    std::size_t start_col = _col;
    int value = 0;

    while (_src[_pos] >= '0' && _src[_pos] <= '9') {
        value = value * 10 + (_src[_pos] - '0');
        ++_pos; ++_col;
    }

    Token tok;
    tok.kind = TokenKind::INT_LITERAL;
    tok.lexeme = core::String(_src + start, _pos - start);
    tok.int_value = value;
    tok.line = _line;
    tok.column = start_col;
    return tok;
}

Token Lexer::scan_string() {
    // Skip opening quote
    std::size_t start_col = _col;
    ++_pos; ++_col; // skip first "

    std::size_t content_start = _pos;

    while (_src[_pos] != '\0' && _src[_pos] != '"') {
        ++_pos; ++_col;
    }

    std::size_t content_len = _pos - content_start;

    if (_src[_pos] == '\0') {
        _has_error = true;
        _error_msg = "Unterminated string literal";
        Token tok;
        tok.kind = TokenKind::INVALID;
        tok.line = _line;
        tok.column = start_col;
        return tok;
    }

    // Skip closing quote
    ++_pos; ++_col;

    Token tok;
    tok.kind = TokenKind::STRING_LITERAL;
    tok.lexeme = core::String(_src + content_start, content_len);
    tok.line = _line;
    tok.column = start_col;
    return tok;
}

TokenKind Lexer::match_keyword(const char* s, std::size_t len) {
    // Simple length-prefixed keyword matching (case-insensitive comparison)
    // We convert to uppercase for comparison
    auto char_eq_ci = [](char a, char b) -> bool {
        if (a >= 'a' && a <= 'z') a = a - 'a' + 'A';
        if (b >= 'a' && b <= 'z') b = b - 'a' + 'A';
        return a == b;
    };

    auto str_eq_ci = [&](const char* kw, std::size_t kw_len) -> bool {
        if (len != kw_len) return false;
        for (std::size_t i = 0; i < len; ++i) {
            if (!char_eq_ci(s[i], kw[i])) return false;
        }
        return true;
    };

    if (str_eq_ci("create", 6))   return TokenKind::KW_CREATE;
    if (str_eq_ci("database", 8)) return TokenKind::KW_DATABASE;
    if (str_eq_ci("drop", 4))     return TokenKind::KW_DROP;
    if (str_eq_ci("use", 3))      return TokenKind::KW_USE;
    if (str_eq_ci("table", 5))    return TokenKind::KW_TABLE;
    if (str_eq_ci("select", 6))   return TokenKind::KW_SELECT;
    if (str_eq_ci("from", 4))     return TokenKind::KW_FROM;
    if (str_eq_ci("where", 5))    return TokenKind::KW_WHERE;
    if (str_eq_ci("delete", 6))   return TokenKind::KW_DELETE;
    if (str_eq_ci("insert", 6))   return TokenKind::KW_INSERT;
    if (str_eq_ci("into", 4))     return TokenKind::KW_INTO;
    if (str_eq_ci("values", 6))   return TokenKind::KW_VALUES;
    if (str_eq_ci("update", 6))   return TokenKind::KW_UPDATE;
    if (str_eq_ci("set", 3))      return TokenKind::KW_SET;
    if (str_eq_ci("primary", 7))  return TokenKind::KW_PRIMARY;
    if (str_eq_ci("int", 3))      return TokenKind::KW_INT;
    if (str_eq_ci("string", 6))   return TokenKind::KW_STRING;

    return TokenKind::IDENTIFIER;
}

} // namespace parser
} // namespace minidb
