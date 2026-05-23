#ifndef MINIDB_CORE_VECTOR_HPP
#define MINIDB_CORE_VECTOR_HPP

#include <cstddef>
#include <utility>
#include <new>

namespace minidb {
namespace core {

template <typename T>
class Vector {
public:
    using value_type = T;
    using size_type = std::size_t;

    Vector() : _data(nullptr), _size(0), _capacity(0) {}

    explicit Vector(size_type n) : _size(n), _capacity(n) {
        _data = static_cast<T*>(::operator new(sizeof(T) * n));
        for (size_type i = 0; i < n; ++i) {
            new (_data + i) T();
        }
    }

    ~Vector() { clear(); ::operator delete(_data); }

    // Copy
    Vector(const Vector& other) : _size(other._size), _capacity(other._capacity) {
        _data = static_cast<T*>(::operator new(sizeof(T) * _capacity));
        for (size_type i = 0; i < _size; ++i) {
            new (_data + i) T(other._data[i]);
        }
    }

    // Move
    Vector(Vector&& other) noexcept : _data(other._data), _size(other._size), _capacity(other._capacity) {
        other._data = nullptr;
        other._size = 0;
        other._capacity = 0;
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            clear();
            ::operator delete(_data);
            _size = other._size;
            _capacity = other._capacity;
            _data = static_cast<T*>(::operator new(sizeof(T) * _capacity));
            for (size_type i = 0; i < _size; ++i) {
                new (_data + i) T(other._data[i]);
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clear();
            ::operator delete(_data);
            _data = other._data;
            _size = other._size;
            _capacity = other._capacity;
            other._data = nullptr;
            other._size = 0;
            other._capacity = 0;
        }
        return *this;
    }

    void push_back(const T& value) {
        if (_size >= _capacity) {
            reserve(_capacity == 0 ? 8 : _capacity * 2);
        }
        new (_data + _size) T(value);
        ++_size;
    }

    void push_back(T&& value) {
        if (_size >= _capacity) {
            reserve(_capacity == 0 ? 8 : _capacity * 2);
        }
        new (_data + _size) T(std::move(value));
        ++_size;
    }

    void pop_back() {
        if (_size > 0) {
            --_size;
            _data[_size].~T();
        }
    }

    void clear() {
        for (size_type i = 0; i < _size; ++i) {
            _data[i].~T();
        }
        _size = 0;
    }

    void reserve(size_type new_cap) {
        if (new_cap > _capacity) {
            T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_cap));
            for (size_type i = 0; i < _size; ++i) {
                new (new_data + i) T(std::move(_data[i]));
                _data[i].~T();
            }
            ::operator delete(_data);
            _data = new_data;
            _capacity = new_cap;
        }
    }

    T& operator[](size_type i) { return _data[i]; }
    const T& operator[](size_type i) const { return _data[i]; }

    T* data() { return _data; }
    const T* data() const { return _data; }

    size_type size() const { return _size; }
    size_type capacity() const { return _capacity; }
    bool empty() const { return _size == 0; }

    T& back() { return _data[_size - 1]; }
    const T& back() const { return _data[_size - 1]; }

    // Iterator support (minimal, pointer-based)
    T* begin() { return _data; }
    const T* begin() const { return _data; }
    T* end() { return _data + _size; }
    const T* end() const { return _data + _size; }

private:
    T* _data;
    size_type _size;
    size_type _capacity;
};

} // namespace core
} // namespace minidb

#endif // MINIDB_CORE_VECTOR_HPP
