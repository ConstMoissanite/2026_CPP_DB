#ifndef MINIDB_STORAGE_COLUMN_HPP
#define MINIDB_STORAGE_COLUMN_HPP

#include "../core/string.hpp"

namespace minidb {
namespace storage {

enum class ColumnType {
    INT,
    STRING
};

class Column {
public:
    Column() : _is_primary(false), _type(ColumnType::INT) {}

    Column(core::String name, ColumnType type, bool is_primary = false)
        : _name(std::move(name)), _type(type), _is_primary(is_primary) {}

    const core::String& name() const { return _name; }
    ColumnType type() const { return _type; }
    bool is_primary() const { return _is_primary; }

    static ColumnType type_from_string(const char* s);

private:
    core::String _name;
    ColumnType   _type;
    bool         _is_primary;
};

} // namespace storage
} // namespace minidb

#endif // MINIDB_STORAGE_COLUMN_HPP
