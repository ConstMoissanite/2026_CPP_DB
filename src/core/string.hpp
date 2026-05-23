#ifndef MINIDB_CORE_STRING_HPP
#define MINIDB_CORE_STRING_HPP

#include <cstring>
#include <cstddef>
#include <utility>

namespace minidb {
namespace core {

// Fixed-capacity string for SQL identifiers and short values.
// For general use; the SQL string type (max 256) is handled separately.
class String {
public:
    String() : _data(nullptr), _len(0) {}

    explicit String(const char* s) {
        if (s) {
            _len = std::strlen(s);
            _data = new char[_len + 1];
            std::memcpy(_data, s, _len + 1);
        } else {
            _data = nullptr;
            _len = 0;
        }
    }

    String(const char* s, std::size_t len) : _len(len) {
        _data = new char[_len + 1];
        std::memcpy(_data, s, _len);
        _data[_len] = '\0';
    }

    // Copy
    String(const String& other) : _len(other._len) {
        if (other._data) {
            _data = new char[_len + 1];
            std::memcpy(_data, other._data, _len + 1);
        } else {
            _data = nullptr;
        }
    }

    // Move
    String(String&& other) noexcept : _data(other._data), _len(other._len) {
        other._data = nullptr;
        other._len = 0;
    }

    ~String() { delete[] _data; }

    String& operator=(const String& other) {
        if (this != &other) {
            delete[] _data;
            _len = other._len;
            if (other._data) {
                _data = new char[_len + 1];
                std::memcpy(_data, other._data, _len + 1);
            } else {
                _data = nullptr;
            }
        }
        return *this;
    }

    String& operator=(String&& other) noexcept {
        if (this != &other) {
            delete[] _data;
            _data = other._data;
            _len = other._len;
            other._data = nullptr;
            other._len = 0;
        }
        return *this;
    }

    String& operator=(const char* s) {
        delete[] _data;
        if (s) {
            _len = std::strlen(s);
            _data = new char[_len + 1];
            std::memcpy(_data, s, _len + 1);
        } else {
            _data = nullptr;
            _len = 0;
        }
        return *this;
    }

    const char* c_str() const { return _data ? _data : ""; }
    std::size_t length() const { return _len; }
    bool empty() const { return _len == 0; }

    char operator[](std::size_t i) const { return _data[i]; }
    char& operator[](std::size_t i) { return _data[i]; }

    bool operator==(const String& other) const {
        if (_len != other._len) return false;
        if (_len == 0) return true;
        return std::memcmp(_data, other._data, _len) == 0;
    }
    bool operator!=(const String& other) const { return !(*this == other); }

    bool operator==(const char* s) const {
        if (!s) return _len == 0;
        std::size_t slen = std::strlen(s);
        if (_len != slen) return false;
        if (_len == 0) return true;
        return std::memcmp(_data, s, _len) == 0;
    }
    bool operator!=(const char* s) const { return !(*this == s); }

private:
    char* _data;
    std::size_t _len;
};

} // namespace core
} // namespace minidb

#endif // MINIDB_CORE_STRING_HPP
