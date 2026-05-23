#include "column.hpp"
#include <cstring>

namespace minidb {
namespace storage {

ColumnType Column::type_from_string(const char* s) {
    if (std::strcmp(s, "int") == 0) return ColumnType::INT;
    return ColumnType::STRING;
}

} // namespace storage
} // namespace minidb
