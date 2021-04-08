//
// Created by gunavaran on 4/7/21.
//

#ifndef FRIGATE_QUERY_H
#define FRIGATE_QUERY_H

#include <stdint.h>

void
query(char *output_file, char *search_file_location);

char *bit_decode_small_k(uint64_t binaryString, int length);

#endif //FRIGATE_QUERY_H
