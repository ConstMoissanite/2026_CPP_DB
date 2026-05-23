#include "index.hpp"

namespace minidb {
namespace storage {

int Index::_find_pos(int key) const {
    if (_entries.empty()) return -1; // ~0

    int lo = 0;
    int hi = static_cast<int>(_entries.size()) - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (_entries[static_cast<std::size_t>(mid)].key == key) return mid;
        if (_entries[static_cast<std::size_t>(mid)].key < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ~lo; // insertion point
}

void Index::insert(int key, int row_id) {
    int pos = _find_pos(key);
    if (pos >= 0) {
        // Key exists, update
        _entries[static_cast<std::size_t>(pos)].row_id = row_id;
        return;
    }
    // Insert at ~pos
    int insert_at = ~pos;
    Entry e{key, row_id};
    if (insert_at >= static_cast<int>(_entries.size())) {
        _entries.push_back(e);
    } else {
        // Shift and insert
        _entries.push_back(Entry{}); // placeholder
        for (int i = static_cast<int>(_entries.size()) - 1; i > insert_at; --i) {
            _entries[static_cast<std::size_t>(i)] = _entries[static_cast<std::size_t>(i - 1)];
        }
        _entries[static_cast<std::size_t>(insert_at)] = e;
    }
}

bool Index::find(int key, int& row_id) const {
    int pos = _find_pos(key);
    if (pos < 0) return false;
    row_id = _entries[static_cast<std::size_t>(pos)].row_id;
    return true;
}

bool Index::remove(int key) {
    int pos = _find_pos(key);
    if (pos < 0) return false;
    // Shift left
    for (std::size_t i = static_cast<std::size_t>(pos);
         i + 1 < _entries.size(); ++i) {
        _entries[i] = _entries[i + 1];
    }
    _entries.pop_back();
    return true;
}

void Index::clear() {
    _entries.clear();
}

} // namespace storage
} // namespace minidb
