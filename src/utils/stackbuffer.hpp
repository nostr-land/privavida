#pragma once

#include <stdlib.h>
#include <assert.h>
#include "../models/relative.hpp"
#include <string.h>

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

struct StackAllocator {
    struct StackNode {
        uint32_t size;
        RelPointer<StackNode> next;
        uint8_t ptr[];
    };
    struct HeapNode {
        HeapNode* next;
        uint8_t ptr[];
    };

    uint8_t* stack;
    uint32_t stack_size;
    RelPointer<StackNode> stack_head;
    RelPointer<StackNode> stack_tail;

    HeapNode* heap_head;
    HeapNode* heap_tail;

    StackAllocator(void* data, uint32_t size) {
        assert(data && size >= 2 * sizeof(StackNode));
        stack = (uint8_t*)data;
        stack_size = size;
        stack_head = RelPointer<StackNode>(0);
        stack_tail = RelPointer<StackNode>(0);
        heap_head = NULL;
        heap_tail = NULL;
        stack_head.get(stack)->size = 0;
    }
    StackAllocator(StackAllocator&&) = delete;
    StackAllocator(const StackAllocator&) = delete;
    ~StackAllocator() {
        if (heap_head) {
            auto node = heap_head;
            while (node) {
                auto next = node->next;
                free(node);
                node = next;
            }
        }
    }

    void* alloc(size_t size_wanted) {
        if (!size_wanted) return NULL;
        size_wanted = align_8(size_wanted);

        // Can we allocate on the stack?
        bool empty = (stack_tail.offset == 0 && stack_tail.get(stack)->size == 0);
        uint32_t bytes_available = (
            empty ? stack_size :
            (stack_size - stack_tail.offset + stack_tail.get(stack)->size)
        );
        if (bytes_available >= sizeof(StackNode) + size_wanted) {
            if (empty) {
                auto head = stack_head.get(stack);
                head->size = sizeof(StackNode) + (int)size_wanted;
                return (void*)head->ptr;
            } else {
                auto tail = stack_tail.get(stack);
                tail->next.offset = stack_tail.offset + tail->size;

                auto next = tail->next.get(stack);
                stack_tail = tail->next;
                return (void*)next->ptr;
            }
        }

        // No space on the stack! Allocate in heap
        auto node = (HeapNode*)malloc(sizeof(HeapNode) + size_wanted);
        node->next = NULL;
        if (heap_tail) {
            heap_tail->next = node;
            heap_tail = node;
        } else {
            heap_head = node;
            heap_tail = node;
        }
        return node->ptr;
    }
    void* realloc(void* ptr, size_t size_wanted) {
        assert(0);
        // printf("Calling StackAllocator.realloc() (not implemented yet!)");
        // if (!size_wanted) return NULL;
        // if (!ptr) alloc(size_wanted);
        // size_wanted = align_8(size_wanted);

        // // Are we realloc-ing the head?
        // if (heap_head && heap_head->ptr == ptr) {
        //     auto next = heap_head->next;
        //     heap_head = (HeapNode*)realloc(heap_head, sizeof(HeapNode) + size_wanted);
        //     heap_head->next = next;
        //     return heap_head->ptr;
        // }

        // // Find the non-head node and realloc
        // auto node = heap_head;
        // while (node && node->next) {
        //     if (node->next->ptr == ptr) {
        //         auto next_next = node->next->next;
        //         node->next = (HeapNode*)realloc(node->next, sizeof(HeapNode) + size_wanted);
        //         node->next->next = next_next;
        //         return node->next->ptr;
        //     }
        // }

        // // Didn't find the pointer? Look for it on the stack
        // bool empty = (stack_tail.offset == 0 && stack_tail.get(stack).size == 0);
        // assert(!empty);

        // auto node2 = stack_head;
        // while () {

        // }

        // void* new_ptr = alloc(size_wanted);
    }

private:
    static size_t align_8(size_t N) {
        return N + (8 - N % 8) % 8;
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
    void push_back(T item) {
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

template <typename T>
struct HeapArray {
    T* data;
    size_t size;
    size_t capacity;

    HeapArray() : data(NULL), size(0), capacity(0) {}
    HeapArray(size_t initial_capacity) {
        data = (T*)malloc(initial_capacity * sizeof(T));
        size = 0;
        capacity = initial_capacity;
    }
    ~HeapArray() {
        free(data);
    }

    T& operator[](size_t index) {
        return data[index];
    }
    const T& operator[](size_t index) const {
        return data[index];
    }
    T* begin() {
        return &data[0];
    }
    T* end() {
        return &data[size];
    }
    const T* begin() const {
        return &data[0];
    }
    const T* end() const {
        return &data[size];
    }
    T& front() const {
        return data[0];
    }
    T& back() const {
        return data[size - 1];
    }

    void reserve(size_t new_capacity) {
        if (new_capacity > capacity) {
            data = realloc(data, new_capacity * sizeof(T));
            capacity = new_capacity;
        }
    }
    void push_back(T item) {
        if (size * sizeof(T) >= capacity) {
            size_t grow = (capacity * 3) / 2;
            reserve(grow < 4 ? 4 : grow);
        }
        data[size++] = item;
    }
    T& push_back() {
        if (size * sizeof(T) >= capacity) {
            size_t grow = (capacity * 3) / 2;
            reserve(grow < 4 ? 4 : grow);
        }
        return data[size++];
    }
};
