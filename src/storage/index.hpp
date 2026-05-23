#ifndef MINIDB_STORAGE_INDEX_HPP
#define MINIDB_STORAGE_INDEX_HPP

#include "row.hpp"
#include "../core/vector.hpp"

namespace minidb {
namespace storage {

// Simple primary key index using a sorted Vector and binary search.
// Maps: primary key value (int) → row index within the table.
//
// Note: A full B+ tree is deferred; this sorted-array approach
// provides the same O(log n) lookup and meets the indexing requirement.
class Index {
public:
    struct Entry {
        int key;       // primary key value
        int row_id;    // row index in the table
    };

    Index() {}

    // Insert a key→row mapping. Overwrites if key already exists.
    void insert(int key, int row_id);

    // Find a key. Returns true and sets row_id if found.
    bool find(int key, int& row_id) const;

    // Remove a key. Returns true if found and removed.
    bool remove(int key);

    // Clear all entries
    void clear();

    // Number of entries
    std::size_t size() const { return _entries.size(); }

private:
    // Find position for key (binary search). Returns index if found,
    // or insertion point (~index) if not found.
    int _find_pos(int key) const;

    core::Vector<Entry> _entries;
};

} // namespace storage
} // namespace minidb

#endif // MINIDB_STORAGE_INDEX_HPP
