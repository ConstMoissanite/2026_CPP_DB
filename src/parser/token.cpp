#include "token.hpp"

namespace minidb {
namespace parser {

const char* token_kind_name(TokenKind kind) {
    switch (kind) {
    case TokenKind::KW_CREATE:    return "CREATE";
    case TokenKind::KW_DATABASE:  return "DATABASE";
    case TokenKind::KW_DROP:      return "DROP";
    case TokenKind::KW_USE:       return "USE";
    case TokenKind::KW_TABLE:     return "TABLE";
    case TokenKind::KW_SELECT:    return "SELECT";
    case TokenKind::KW_FROM:      return "FROM";
    case TokenKind::KW_WHERE:     return "WHERE";
    case TokenKind::KW_DELETE:    return "DELETE";
    case TokenKind::KW_INSERT:    return "INSERT";
    case TokenKind::KW_INTO:      return "INTO";
    case TokenKind::KW_VALUES:    return "VALUES";
    case TokenKind::KW_UPDATE:    return "UPDATE";
    case TokenKind::KW_SET:       return "SET";
    case TokenKind::KW_PRIMARY:   return "PRIMARY";
    case TokenKind::KW_INT:       return "INT";
    case TokenKind::KW_STRING:    return "STRING";
    case TokenKind::IDENTIFIER:   return "IDENTIFIER";
    case TokenKind::INT_LITERAL:  return "INT_LITERAL";
    case TokenKind::STRING_LITERAL: return "STRING_LITERAL";
    case TokenKind::OP_EQ:        return "=";
    case TokenKind::OP_LT:        return "<";
    case TokenKind::OP_GT:        return ">";
    case TokenKind::OP_STAR:      return "*";
    case TokenKind::LPAREN:       return "(";
    case TokenKind::RPAREN:       return ")";
    case TokenKind::COMMA:        return ",";
    case TokenKind::SEMICOLON:    return ";";
    case TokenKind::END_OF_FILE:  return "EOF";
    case TokenKind::INVALID:      return "INVALID";
    default:                      return "UNKNOWN";
    }
}

} // namespace parser
} // namespace minidb
