#ifndef MINIDB_STORAGE_TABLE_HPP
#define MINIDB_STORAGE_TABLE_HPP

#include "../core/string.hpp"
#include "../core/vector.hpp"
#include "column.hpp"
#include "row.hpp"

namespace minidb {
namespace storage {

// File-backed table. Rows are persisted to disk as text.
// File format (plain text):
//   Line 1: column_count
//   Line 2..N: name type is_primary  (one per column)
//   Line N+1: row_count
//   Remaining lines: value1|value2|...  (one per row)
//
// String values are stored as-is (no escaping for now).
// Int values are stored as decimal.
class Table {
public:
    explicit Table(core::String name);
    Table(core::String name, core::String data_dir);

    const core::String& name() const { return _name; }

    // Schema
    void add_column(const Column& col);
    const core::Vector<Column>& columns() const { return _columns; }
    std::size_t column_count() const { return _columns.size(); }

    // Find column index by name; returns -1 if not found
    int find_column(const char* col_name) const;

    // Find primary key column index; returns -1 if none
    int primary_key_index() const;

    // Data
    void insert_row(const Row& row);
    const core::Vector<Row>& rows() const { return _rows; }
    Row& row_at(std::size_t i) { return _rows[i]; }
    const Row& row_at(std::size_t i) const { return _rows[i]; }
    std::size_t row_count() const { return _rows.size(); }

    void remove_row(std::size_t index);
    void update_row(std::size_t index, const Row& new_row);

    // Persistence
    bool save_to_file();
    bool load_from_file();

private:
    core::String       _name;
    core::String       _data_path;    // full path to .dat file
    core::Vector<Column> _columns;
    core::Vector<Row>    _rows;
};

} // namespace storage
} // namespace minidb

#endif // MINIDB_STORAGE_TABLE_HPP
