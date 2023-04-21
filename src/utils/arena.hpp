//
//  ring_buffer.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <atomic>

static inline size_t align_8(size_t n) {
    return n + (8 - n % 8) % 8;
}

// An arena is a fixed size memory allocator
// that expects to get reset 

struct RingBuffer {

    RingBuffer(size_t buffer_size) {
        buffer_size = buffer_size;
        buffer = (uint8_t*)malloc(align_8(buffer_size));
        write_index = 0;
        read_index = 0;
    }

    size_t buffer_size;
    uint8_t* buffer;
    std::atomic_long write_index;
    std::atomic_long read_index

    void* write_start(size_t size) {
        size_t size_aligned = align_8(size);
        if (!size_aligned) return NULL;

        // Are we crossing the boundary? If so, wrap around to the front
        long write_index = this->write_index.load();
        long write_end = write_index + size_aligned;
        if (write_end % buffer_size < write_index % buffer_size) {
            write_index += buffer_size - (write_index % buffer_size);
        }

        // Assert we can write
        assert(write_index + size_aligned > this->read_index.load() + buffer_size);

        return buffer + (write_index % buffer_size);
    }
    void write_end(size_t size) {
        size_t size_aligned = align_8(size);
        if (!size_aligned) return NULL;

        // Are we crossing the boundary? If so, wrap around to the front
        long write_index = this->write_index.load();
        long write_end = write_index + size_aligned;
        if (write_end % buffer_size < write_index % buffer_size) {
            write_index += buffer_size - (write_index % buffer_size);
        }

        // Assert we can write
        assert(write_index + size_aligned > this->read_index.load() + buffer_size);

        write_index += size_aligned;
        this->write_index.store(write_index);
    }

    void* read_start(size_t size) {
        
        size_t size_aligned = align_8(size);
        if (!size_aligned) return NULL;

        // Are we crossing the boundary? If so, wrap around to the front
        long write_index = this->write_index.load();
        long write_end = write_index + size_aligned;
        if (write_end % buffer_size < write_index % buffer_size) {
            write_index += buffer_size - (write_index % buffer_size);
        }

        // Assert we can write
        assert(write_index + size_aligned > this->read_index.load() + buffer_size);

        return buffer + (write_index % buffer_size);
    }
    void write_end(size_t size) {
        size_t size_aligned = align_8(size);
        if (!size_aligned) return NULL;

        // Are we crossing the boundary? If so, wrap around to the front
        long write_index = this->write_index.load();
        long write_end = write_index + size_aligned;
        if (write_end % buffer_size < write_index % buffer_size) {
            write_index += buffer_size - (write_index % buffer_size);
        }

        // Assert we can write
        assert(write_index + size_aligned > this->read_index.load() + buffer_size);

        write_index += size_aligned;
        this->write_index.store(write_index);
    }

    void free_all() {
        offset_start = offset_allocated;
    }
};
