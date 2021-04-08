//
// Created by gunavaran on 4/6/21.
//

#include <bits/types/FILE.h>
#include "fileIO.h"
#include "frigate.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <inttypes.h>

void write_meta_data(int thread_count, int k_value, uint64_t write_length, char *binary_output_location) {
    FILE *meta_file;
    char meta_file_location[200];
    strcpy(meta_file_location, binary_output_location);
    strcat(meta_file_location, "_meta");

    if((meta_file = fopen(meta_file_location, "w")) == NULL){
        fprintf(stderr, "Can't open file %s\n", meta_file_location);
        exit(1);
    }

    fprintf(meta_file,"%d\n", k_value);
    fprintf(meta_file,"%d\n", thread_count);
    fprintf(meta_file,"%"PRIu64"\n", write_length);
    fclose(meta_file);
}

void write_binary_output(char *binary_output_location, counter_t *array_main, uint64_t array_length, int k_value) {
    FILE *binary_output_file;

    if ((binary_output_file = fopen(binary_output_location, "wb")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", binary_output_location);
        exit(1);
    }

    size_t kmer_byte_size = (k_value + 3) / 4;
//    int compression_size = (kmer_byte_size + 1) / 2;

    uint64_t n = 0;

    while (n < array_length) {
        if (array_main[n] >= COUNT_MIN) {
            fwrite(&n, kmer_byte_size, 1, binary_output_file);
            fwrite(&array_main[n], COUNTER_BYTE_SIZE, 1, binary_output_file);
        }
        n++;
    }

    fclose(binary_output_file);
}

void
parallel_write_binary_output(char *binary_output_location, counter_t *array_main, int thread_id, int k_value,
                             uint64_t write_length, uint64_t permutations) {
    uint64_t start_index, n;
    uint64_t i;
    FILE *binary_output_file;

    if ((binary_output_file = fopen(binary_output_location, "wb")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", binary_output_location);
        exit(1);
    }

    size_t kmer_byte_size = (k_value + 3) / 4;
//    int compression_size = (kmer_byte_size + 1) / 2;

    start_index = thread_id * write_length;
    n = start_index;

//    uint64_t temp_count = 0;
    while (n < permutations) {

        for (i = 0; (i < write_length) && (n < permutations); i++) {
            if (array_main[n] >= COUNT_MIN) {
//                temp_count ++;
                fwrite(&n, kmer_byte_size, 1, binary_output_file);
                fwrite(&array_main[n], COUNTER_BYTE_SIZE, 1, binary_output_file);
            }
            n++;
        }

        start_index += GLOBAL_THREAD_COUNT * write_length;
        n = start_index;

    }

//    printf("====thread %d : %ld \n", thread_id, temp_count);

    fclose(binary_output_file);
}

void write_readable_output(char *output_file, char *readable_file) {
    FILE *input_binary_file, *dump_file, *meta_file;
    uint64_t kmer;
    counter_t kmer_count;
    int k_value, num_files;
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

    char decoded_string[k_value];
    char my_binary_output_location[200];
    char num_string[5];
    uint8_t temp, mask = 3;
    int i, j;

    size_t kmer_byte_size = (k_value + 3) / 4;

    if ((dump_file = fopen(readable_file, "w")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", readable_file);
        exit(1);
    }

    for (i = 0; i < num_files; i++) {
        sprintf(num_string, "%d", i);
        strcpy(my_binary_output_location, "./output/");
        strcat(my_binary_output_location, output_file);
        strcat(my_binary_output_location, num_string);
//        printf("%s \n", my_binary_output_location);

        if ((input_binary_file = fopen(my_binary_output_location, "r")) == NULL) {
            fprintf(stderr, "Can't open file %s\n", my_binary_output_location);
            exit(1);
        }

        while (fread(&kmer, kmer_byte_size, 1, input_binary_file) == 1) {
            fread(&kmer_count, COUNTER_BYTE_SIZE, 1, input_binary_file);

            for (j = 0; j < k_value; j++) {

                temp = kmer & mask;

                switch (temp) {
                    case 0:
                        decoded_string[k_value - 1 - j] = 'A';
                        break;
                    case 1:
                        decoded_string[k_value - 1 - j] = 'C';
                        break;
                    case 2:
                        decoded_string[k_value - 1 - j] = 'G';
                        break;
                    case 3:
                        decoded_string[k_value - 1 - j] = 'T';
                        break;
                    default:  //not required
                        printf("Invalid Input \n");
                        break;
                }

                kmer = kmer >> 2;

            }

//            printf("%s : %u \n", decoded_string, kmer_count);
            fprintf(dump_file, "%s : %u\n", decoded_string, kmer_count);

        }

        fclose(input_binary_file);
    }
    fclose(dump_file);
}