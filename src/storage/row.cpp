#include "row.hpp"
#include <cstring>

namespace minidb {
namespace storage {

int Value::compare(const Value& a, const Value& b) {
    // NONE sorts before everything
    if (a._type == Value::NONE && b._type == Value::NONE) return 0;
    if (a._type == Value::NONE) return -1;
    if (b._type == Value::NONE) return 1;

    // INT vs INT
    if (a._type == Value::INT && b._type == Value::INT) {
        if (a._int_val < b._int_val) return -1;
        if (a._int_val > b._int_val) return 1;
        return 0;
    }

    // STRING vs STRING
    if (a._type == Value::STRING && b._type == Value::STRING) {
        return std::strcmp(a.string_value().c_str(), b.string_value().c_str());
    }

    // Mixed types: INT < STRING
    if (a._type == Value::INT) return -1;
    return 1;
}

} // namespace storage
} // namespace minidb
