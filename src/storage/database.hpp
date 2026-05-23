#ifndef MINIDB_STORAGE_DATABASE_HPP
#define MINIDB_STORAGE_DATABASE_HPP

#include "../core/string.hpp"
#include "../core/vector.hpp"
#include "table.hpp"
#include "index.hpp"

namespace minidb {
namespace storage {

// Manages a single database: a collection of tables and their indexes.
class Database {
public:
    explicit Database(core::String name);

    const core::String& name() const { return _name; }

    // Table operations
    bool create_table(const core::String& name,
                      const core::Vector<Column>& columns);
    bool drop_table(const char* name);

    Table* get_table(const char* name);
    Index* get_index(const char* name);

    // Data directory for persistence
    void set_data_dir(core::String dir) { _data_dir = std::move(dir); }
    const core::String& data_dir() const { return _data_dir; }

    // Persist all tables
    bool save_all();
    bool load_all();

    // List table names
    const core::Vector<core::String>& table_names() const { return _table_names; }

private:
    core::String             _name;
    core::String             _data_dir;
    core::Vector<Table>      _tables;
    core::Vector<Index>      _indexes;
    core::Vector<core::String> _table_names;
};

// ============================================================
// Global database registry
// ============================================================

// Create a database directory in the data root.
// data_root is typically "data/" under the working directory.
bool create_database(const char* data_root, const char* dbname);

// Drop a database directory
bool drop_database(const char* data_root, const char* dbname);

// Set the current database; returns the new Database*
Database* use_database(const char* data_root, const char* dbname);

// Get the current database; returns nullptr if none selected
Database* current_db();

} // namespace storage
} // namespace minidb

#endif // MINIDB_STORAGE_DATABASE_HPP
