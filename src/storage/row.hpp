#ifndef MINIDB_STORAGE_ROW_HPP
#define MINIDB_STORAGE_ROW_HPP

#include "../core/string.hpp"
#include "../core/vector.hpp"

namespace minidb {
namespace storage {

// Discriminated union: holds either an int or a string (no std::variant).
class Value {
public:
    enum Type { INT, STRING, NONE };

    Value() : _type(NONE), _int_val(0) {}

    explicit Value(int val) : _type(INT), _int_val(val) {}
    explicit Value(core::String val) : _type(STRING), _int_val(0) {
        new (&_str_buf) core::String(std::move(val));
    }

    Value(const Value& other) : _type(other._type), _int_val(other._int_val) {
        if (other._type == STRING) {
            new (&_str_buf) core::String(other.str_ref());
        }
    }

    Value(Value&& other) noexcept : _type(other._type), _int_val(other._int_val) {
        if (other._type == STRING) {
            new (&_str_buf) core::String(std::move(other.str_ref()));
        }
        other._type = NONE;
    }

    ~Value() {
        if (_type == STRING) {
            str_ref().~String();
        }
    }

    Value& operator=(const Value& other) {
        if (this != &other) {
            if (_type == STRING) str_ref().~String();
            _type = other._type;
            _int_val = other._int_val;
            if (other._type == STRING) {
                new (&_str_buf) core::String(other.str_ref());
            }
        }
        return *this;
    }

    Value& operator=(Value&& other) noexcept {
        if (this != &other) {
            if (_type == STRING) str_ref().~String();
            _type = other._type;
            _int_val = other._int_val;
            if (other._type == STRING) {
                new (&_str_buf) core::String(std::move(other.str_ref()));
            }
            other._type = NONE;
        }
        return *this;
    }

    Type type() const { return _type; }
    bool is_int() const { return _type == INT; }
    bool is_string() const { return _type == STRING; }
    bool is_none() const { return _type == NONE; }

    int int_value() const { return _int_val; }
    const core::String& string_value() const { return str_ref(); }

    // Compare two values. Returns -1 (less), 0 (equal), 1 (greater).
    // Strings are compared lexicographically.
    static int compare(const Value& a, const Value& b);

private:
    core::String& str_ref() {
        return *reinterpret_cast<core::String*>(&_str_buf);
    }
    const core::String& str_ref() const {
        return *reinterpret_cast<const core::String*>(&_str_buf);
    }

    Type                      _type;
    int                       _int_val;
    alignas(core::String) char _str_buf[sizeof(core::String)];
};

// A row is a collection of Value objects.
class Row {
public:
    void add_value(const Value& val) { _values.push_back(val); }
    void add_value(Value&& val) { _values.push_back(std::move(val)); }

    const Value& get_value(std::size_t index) const { return _values[index]; }
    Value& get_value(std::size_t index) { return _values[index]; }

    std::size_t count() const { return _values.size(); }
    bool empty() const { return _values.empty(); }

    const core::Vector<Value>& values() const { return _values; }

private:
    core::Vector<Value> _values;
};

} // namespace storage
} // namespace minidb

#endif // MINIDB_STORAGE_ROW_HPP
