//
// Created by gunavaran on 4/6/21.
//

#ifndef FRIGATE_FILEIO_H
#define FRIGATE_FILEIO_H

#include <stdint.h>
#include "frigate.h"

void write_meta_data(int thread_count, int k_value, uint64_t write_length, char *binary_output_location);

void write_binary_output(char *binary_output_location, counter_t *array_main, uint64_t array_length, int k_value);

void
parallel_write_binary_output(char *binary_output_location, counter_t *array_main, int thread_id, int k_value,
                             uint64_t write_length, uint64_t permutations);

void write_readable_output(char *output_file, char *readable_file);

#endif //FRIGATE_FILEIO_H
