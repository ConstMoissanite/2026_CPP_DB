#include "database.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(p) mkdir(p, 0755)
#endif

namespace minidb {
namespace storage {

// ============================================================
// Database
// ============================================================

Database::Database(core::String name) : _name(std::move(name)) {}

bool Database::create_table(const core::String& name,
                            const core::Vector<Column>& columns) {
    // Check if table already exists
    if (get_table(name.c_str()) != nullptr) return false;

    Table table(name, _data_dir);
    for (std::size_t i = 0; i < columns.size(); ++i) {
        table.add_column(columns[i]);
    }

    // Create index if there's a primary key
    int pk_idx = table.primary_key_index();
    if (pk_idx >= 0) {
        Index idx;
        _indexes.push_back(std::move(idx));
    } else {
        _indexes.push_back(Index()); // placeholder
    }

    _table_names.push_back(name);
    _tables.push_back(std::move(table));
    return true;
}

bool Database::drop_table(const char* name) {
    for (std::size_t i = 0; i < _table_names.size(); ++i) {
        if (_table_names[i] == name) {
            // Remove table file
            core::String path = _tables[i].name();
            // We need to get the data path; reconstruct it
            if (!_data_dir.empty()) {
                const char* d = _data_dir.c_str();
                const char* n = name;
                std::size_t total = std::strlen(d) + 1 + std::strlen(n) + 4;
                char* buf = new char[total + 1];
                std::snprintf(buf, total + 1, "%s/%s.dat", d, n);
                std::remove(buf);
                delete[] buf;
            } else {
                char buf[512];
                std::snprintf(buf, sizeof(buf), "%s.dat", name);
                std::remove(buf);
            }

            // Remove index file
            if (!_data_dir.empty()) {
                const char* d = _data_dir.c_str();
                const char* n = name;
                std::size_t total = std::strlen(d) + 1 + std::strlen(n) + 4;
                char* buf = new char[total + 1];
                std::snprintf(buf, total + 1, "%s/%s.idx", d, n);
                std::remove(buf);
                delete[] buf;
            } else {
                char buf[512];
                std::snprintf(buf, sizeof(buf), "%s.idx", name);
                std::remove(buf);
            }

            // Remove from vectors
            for (std::size_t j = i; j + 1 < _table_names.size(); ++j) {
                _table_names[j] = std::move(_table_names[j + 1]);
                _tables[j] = std::move(_tables[j + 1]);
                _indexes[j] = std::move(_indexes[j + 1]);
            }
            _table_names.pop_back();
            _tables.pop_back();
            _indexes.pop_back();
            return true;
        }
    }
    return false;
}

Table* Database::get_table(const char* name) {
    for (std::size_t i = 0; i < _table_names.size(); ++i) {
        if (_table_names[i] == name) {
            return &_tables[i];
        }
    }
    return nullptr;
}

Index* Database::get_index(const char* name) {
    for (std::size_t i = 0; i < _table_names.size(); ++i) {
        if (_table_names[i] == name) {
            return &_indexes[i];
        }
    }
    return nullptr;
}

bool Database::save_all() {
    if (_data_dir.empty()) return false;

    for (std::size_t i = 0; i < _tables.size(); ++i) {
        if (!_tables[i].save_to_file()) return false;
    }
    return true;
}

bool Database::load_all() {
    if (_data_dir.empty()) return false;

    // Tables are loaded on demand via get_table/create_table
    return true;
}

// ============================================================
// Global database registry
// ============================================================

static Database* g_current_db = nullptr;

Database* current_db() { return g_current_db; }

static core::String make_db_path(const char* data_root, const char* dbname) {
    std::size_t rlen = std::strlen(data_root);
    std::size_t dlen = std::strlen(dbname);
    std::size_t total = rlen + 1 + dlen;
    char* buf = new char[total + 1];
    std::memcpy(buf, data_root, rlen);
    buf[rlen] = '/';
    std::memcpy(buf + rlen + 1, dbname, dlen);
    buf[total] = '\0';
    core::String result(buf);
    delete[] buf;
    return result;
}

bool create_database(const char* data_root, const char* dbname) {
    // Ensure data_root exists
    MKDIR(data_root);
    core::String path = make_db_path(data_root, dbname);
    return MKDIR(path.c_str()) == 0;
}

bool drop_database(const char* data_root, const char* dbname) {
    core::String path = make_db_path(data_root, dbname);
    // Remove all .dat and .idx files first, then the directory
    // For simplicity, we just try rmdir (which fails if not empty)
    // A full recursive delete would need directory iteration.

    // Simple approach: just try rmdir
#ifdef _WIN32
    return _rmdir(path.c_str()) == 0;
#else
    return rmdir(path.c_str()) == 0;
#endif
}

Database* use_database(const char* data_root, const char* dbname) {
    static Database db{core::String(dbname)};
    db.set_data_dir(make_db_path(data_root, dbname));
    g_current_db = &db;
    return g_current_db;
}

} // namespace storage
} // namespace minidb
