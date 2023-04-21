//
//  worker.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once
#include "arena.hpp"

// A worker defines a worker thread off the main
// thread that can do a specific kind of work.

struct Worker {
    void run();

    // To send input data to the worker you can use post_input()
    void post_input(uint8_t* data, size_t data_len);

    // To process response 
    size_t process_output_start();
    void process_output_get(uint8_t** data, size_t data_len*);
    void process_output_end(size_t num_outputs_processed);

private:
    void post_output(uint8_t* data, size_t data_len);
    virtual void process_input(uint8_t* data, size_t data_len) = 0;
};
