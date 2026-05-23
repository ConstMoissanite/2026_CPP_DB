#include "table.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

namespace minidb {
namespace storage {

Table::Table(core::String name)
    : _name(std::move(name)), _data_path(_name) {
    // Append ".dat"
    std::size_t old_len = _data_path.length();
    char* new_buf = new char[old_len + 5];
    if (old_len > 0) std::memcpy(new_buf, _data_path.c_str(), old_len);
    std::memcpy(new_buf + old_len, ".dat", 5);
    _data_path = core::String(new_buf);
    delete[] new_buf;
}

Table::Table(core::String name, core::String data_dir)
    : _name(std::move(name)) {
    // Build path: data_dir/name.dat
    const char* d = data_dir.c_str();
    const char* n = _name.c_str();
    std::size_t dlen = std::strlen(d);
    std::size_t nlen = std::strlen(n);
    std::size_t total = dlen + 1 + nlen + 4; // dir/name.dat
    char* buf = new char[total + 1];
    std::memcpy(buf, d, dlen);
    buf[dlen] = '/';
    std::memcpy(buf + dlen + 1, n, nlen);
    std::memcpy(buf + dlen + 1 + nlen, ".dat", 5);
    buf[total] = '\0';
    _data_path = core::String(buf);
    delete[] buf;
}

void Table::add_column(const Column& col) {
    _columns.push_back(col);
}

int Table::find_column(const char* col_name) const {
    for (std::size_t i = 0; i < _columns.size(); ++i) {
        if (_columns[i].name() == col_name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Table::primary_key_index() const {
    for (std::size_t i = 0; i < _columns.size(); ++i) {
        if (_columns[i].is_primary()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Table::insert_row(const Row& row) {
    _rows.push_back(row);
}

void Table::remove_row(std::size_t index) {
    if (index >= _rows.size()) return;
    // Shift elements left
    for (std::size_t i = index; i + 1 < _rows.size(); ++i) {
        _rows[i] = std::move(_rows[i + 1]);
    }
    _rows.pop_back();
}

void Table::update_row(std::size_t index, const Row& new_row) {
    if (index < _rows.size()) {
        _rows[index] = new_row;
    }
}

// ============================================================
// Save to file
// ============================================================
bool Table::save_to_file() {
    FILE* fp = std::fopen(_data_path.c_str(), "w");
    if (!fp) return false;

    // Column count
    std::fprintf(fp, "%zu\n", _columns.size());

    // Columns: name type is_primary
    for (std::size_t i = 0; i < _columns.size(); ++i) {
        std::fprintf(fp, "%s %d %d\n",
            _columns[i].name().c_str(),
            static_cast<int>(_columns[i].type()),
            _columns[i].is_primary() ? 1 : 0);
    }

    // Row count
    std::fprintf(fp, "%zu\n", _rows.size());

    // Rows
    for (std::size_t i = 0; i < _rows.size(); ++i) {
        const Row& row = _rows[i];
        for (std::size_t j = 0; j < row.count(); ++j) {
            if (j > 0) std::fputc('|', fp);
            const Value& v = row.get_value(j);
            if (v.is_int()) {
                std::fprintf(fp, "%d", v.int_value());
            } else if (v.is_string()) {
                std::fprintf(fp, "%s", v.string_value().c_str());
            }
        }
        std::fputc('\n', fp);
    }

    std::fclose(fp);
    return true;
}

// ============================================================
// Load from file
// ============================================================
bool Table::load_from_file() {
    FILE* fp = std::fopen(_data_path.c_str(), "r");
    if (!fp) return false;

    char line_buf[8192];

    // Column count
    if (!std::fgets(line_buf, sizeof(line_buf), fp)) { std::fclose(fp); return false; }
    int col_cnt = std::atoi(line_buf);

    _columns.clear();
    for (int i = 0; i < col_cnt; ++i) {
        if (!std::fgets(line_buf, sizeof(line_buf), fp)) { std::fclose(fp); return false; }
        // Parse: name type is_primary
        char* name = std::strtok(line_buf, " \t\n\r");
        char* type_str = std::strtok(nullptr, " \t\n\r");
        char* pk_str = std::strtok(nullptr, " \t\n\r");
        if (!name || !type_str || !pk_str) { std::fclose(fp); return false; }

        ColumnType ct = Column::type_from_string(
            (std::strcmp(type_str, "0") == 0) ? "int" : "string");
        // Actually: we stored type as int (0=INT, 1=STRING)
        int type_int = std::atoi(type_str);
        ColumnType ct2 = (type_int == 0) ? ColumnType::INT : ColumnType::STRING;
        bool is_pk = (std::atoi(pk_str) != 0);

        _columns.push_back(Column(core::String(name), ct2, is_pk));
    }

    // Row count
    if (!std::fgets(line_buf, sizeof(line_buf), fp)) { std::fclose(fp); return false; }
    int row_cnt = std::atoi(line_buf);

    _rows.clear();
    for (int i = 0; i < row_cnt; ++i) {
        if (!std::fgets(line_buf, sizeof(line_buf), fp)) { std::fclose(fp); return false; }

        Row row;
        char* saveptr = nullptr;
#ifdef _MSC_VER
        char* token = strtok_s(line_buf, "|\n\r", &saveptr);
#else
        char* token = strtok_r(line_buf, "|\n\r", &saveptr);
#endif
        int col_idx = 0;
        while (token && col_idx < col_cnt) {
            if (col_idx < static_cast<int>(_columns.size())) {
                if (_columns[static_cast<std::size_t>(col_idx)].type() == ColumnType::INT) {
                    row.add_value(Value(std::atoi(token)));
                } else {
                    row.add_value(Value(core::String(token)));
                }
            }
            ++col_idx;
#ifdef _MSC_VER
            token = strtok_s(nullptr, "|\n\r", &saveptr);
#else
            token = strtok_r(nullptr, "|\n\r", &saveptr);
#endif
        }
        _rows.push_back(std::move(row));
    }

    std::fclose(fp);
    return true;
}

} // namespace storage
} // namespace minidb
