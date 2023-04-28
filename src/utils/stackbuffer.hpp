#include <stdlib.h>

// StackBuffer is a simple elastic buffer that starts out managing a certain amount of
// pre-allocated memory that exists on the stack.
// In the event that the memory needed fits in pre-allocated memory, then nothing will be
// allocated.
// In the event that you go past the pre-allocated amount it will allocate and manage data
// on the heap

// There is also a StackBufferFixed with its pre-allocated memory size fixed at compile-time.

struct StackBuffer {
    void* data;
    size_t size;
    bool data_is_on_stack;

    StackBuffer(void* data, size_t size) {
        this->data = data;
        this->size = size;
        this->data_is_on_stack = true;
    }
    ~StackBuffer() {
        if (!data_is_on_stack) {
            free(data);
            data = NULL;
            size = 0;
        }
    }
    StackBuffer(StackBuffer&&) = delete;
    StackBuffer(const StackBuffer&) = delete;

    void reserve(size_t size_wanted) {
        if (size_wanted <= size) return;

        // Grow size by 2x each time
        size_t size_new = size ? size << 1 : 8;
        while (size_new < size_wanted) {
            size_new <<= 1;
        }

        if (data_is_on_stack) {
            void* data_heap = malloc(size_new);
            memcpy(data_heap, data, size);
            data = data_heap;
            size = size_new;
            data_is_on_stack = false;
        } else {
            data = realloc(data, size_new);
            size = size_new;
        }
    }
};

template <size_t N>
struct StackBufferFixed : StackBuffer {
    StackBufferFixed() : StackBuffer(data_fixed, N) {}

    uint8_t data_fixed[N];
};


template <typename T>
struct StackArray {
    StackBuffer buffer;
    size_t size;

    StackArray() : size(0) {}
    StackArray(T* data, size_t initial_capacity) : buffer(data, initial_capacity * sizeof(T)), size(0) {}

    T& operator[](size_t index) {
        return ((T*)buffer.data)[index];
    }
    const T& operator[](size_t index) const {
        return ((T*)buffer.data)[index];
    }
    T* begin() {
        return &((T*)buffer.data)[0];
    }
    T* end() {
        return &((T*)buffer.data)[size];
    }
    const T* begin() const {
        return &((T*)buffer.data)[0];
    }
    const T* end() const {
        return &((T*)buffer.data)[size];
    }
    T& front() const {
        return ((T*)buffer.data)[0];
    }
    T& back() const {
        return ((T*)buffer.data)[size - 1];
    }

    void reserve(size_t size) {
        buffer.reserve(size * sizeof(T));
    }
    void push_back(T& item) {
        if (size * sizeof(T) >= buffer.size) {
            reserve(size + 1);
        }
        ((T*)buffer.data)[size++] = std::move(item);
    }
    void push_back(const T& item) {
        if (size * sizeof(T) >= buffer.size) {
            reserve(size + 1);
        }
        ((T*)buffer.data)[size++] = item;
    }
    T& push_back() {
        if (size * sizeof(T) >= buffer.size) {
            reserve(size + 1);
        }
        return ((T*)buffer.data)[size++];
    }
};

template <typename T, size_t N>
struct StackArrayFixed : StackArray<T> {
    StackArrayFixed() : StackArray<T>(data_fixed, N) {}

    T data_fixed[N];
};
