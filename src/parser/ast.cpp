#include "ast.hpp"

namespace minidb {
namespace parser {

const char* stmt_kind_name(StmtKind kind) {
    switch (kind) {
    case StmtKind::CREATE_DATABASE: return "CREATE DATABASE";
    case StmtKind::DROP_DATABASE:   return "DROP DATABASE";
    case StmtKind::USE:             return "USE";
    case StmtKind::CREATE_TABLE:    return "CREATE TABLE";
    case StmtKind::DROP_TABLE:      return "DROP TABLE";
    case StmtKind::SELECT:          return "SELECT";
    case StmtKind::DELETE:          return "DELETE";
    case StmtKind::INSERT:          return "INSERT";
    case StmtKind::UPDATE:          return "UPDATE";
    case StmtKind::INVALID:         return "INVALID";
    default:                        return "UNKNOWN";
    }
}

} // namespace parser
} // namespace minidb
