//
// Created by gunavaran on 4/6/21.
//

#include <stdlib.h>
#include "histogram.h"
#include "frigate.h"
#include <stdio.h>
#include <memory.h>

void histogram(char *output_file, char *histo_file) {
    FILE *input_binary_file, *histogram_file, *meta_file;
    uint64_t kmer, count = 0;
    counter_t kmer_count;
    const int ARRAY_SIZE = 256;
    int k_value = 0, num_files = 0;

    char meta_file_location[200];
    strcpy(meta_file_location, "./output/");
    strcat(meta_file_location, output_file);
    strcat(meta_file_location, "_meta");

    if((meta_file = fopen(meta_file_location, "r")) == NULL){
        fprintf(stderr, "Can't open file %s\n", meta_file_location);
        exit(1);
    }

    fscanf(meta_file, "%d %d", &k_value, &num_files);
    fclose(meta_file);

    char binary_output_location[200];
    strcpy(binary_output_location, "./output/");
    strcat(binary_output_location, output_file);

    uint64_t *histo_array = calloc(ARRAY_SIZE, sizeof(uint64_t));
    char my_binary_output_location[200];
    char num_string[5];

    size_t kmer_byte_size = (k_value + 3) / 4;

    for (int i = 0; i < num_files; i++) {
        sprintf(num_string, "%d", i);
        strcpy(my_binary_output_location, binary_output_location);
        strcat(my_binary_output_location, num_string);
//        printf("%s \n", my_binary_output_location);

        if ((input_binary_file = fopen(my_binary_output_location, "r")) == NULL) {
            fprintf(stderr, "Can't open file %s\n", my_binary_output_location);
            exit(1);
        }

        while (fread(&kmer, kmer_byte_size, 1, input_binary_file) == 1) {
            fread(&kmer_count, COUNTER_BYTE_SIZE, 1, input_binary_file);
            if (kmer_count < ARRAY_SIZE) {
                histo_array[kmer_count]++;
                count++;
            }
        }

        fclose(input_binary_file);

    }

    if ((histogram_file = fopen(histo_file, "w")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", histo_file);
        exit(1);
    }


    for (int i = 1; i < ARRAY_SIZE; i++) {
        fprintf(histogram_file, "%d : %lu\n", i, histo_array[i]);
    }
    fclose(histogram_file);

    free(histo_array);
}
