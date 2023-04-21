//
//  relative.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-10.
//

#pragma once
#include <stdlib.h>

// relative.hpp defines the following types:
//  
//   RelPointer<T>    a relative pointer to type T
//   Array<T>         a very simple array (one field size, one pointer to T)
//   RelArray<T>      a relative array (one field size, one field RelPointer<T>)
//   RelString        a relative string RelArray<char>
//
// These relative structs are used inside data structures
// who are built to be trivially copyable and serializable.
// As such they usually keep all associated data in a dynamic
// buffer just off the end of the struct.

template <typename T>
struct RelPointer {
    uint32_t offset;

    RelPointer() = default;
    RelPointer(uint32_t offset) : offset(offset) {}
    T* get(void* base) const {
        return (T*)((uint8_t*)base + offset);
    }
    const T* get(const void* base) const {
        return (const T*)((const uint8_t*)base + offset);
    }
    RelPointer& operator+=(const int rhs) {
        offset += sizeof(T) * rhs;
        return *this;
    }
    RelPointer& operator-=(const int rhs) {
        offset -= sizeof(T) * rhs;
        return *this;
    }
};

template <typename T>
struct Array {
    uint32_t size;
    T* data;

    Array() = default;
    Array(uint32_t size, T* data) : size(size), data(data) {}
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
    T* begin() { return &data[0]; }
    T* end() { return &data[size]; }
};

template <typename T>
struct RelArray {
    uint32_t size;
    RelPointer<T> data;

    RelArray() = default;
    RelArray(uint32_t size, uint32_t data) : size(size), data(data) {}
    Array<T> get(void* base) const {
        return Array<T>(size, data.get(base));
    }
    const Array<T> get(const void* base) const {
        return Array<T>(size, const_cast<T*>(data.get(base)));
    }
    T& get(void* base, size_t index) const {
        return data.get(base)[index];
    }
    const T& get(const void* base, size_t index) const {
        return data.get(base)[index];
    }
};

typedef RelArray<char> RelString;
