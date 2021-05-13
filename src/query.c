//
// Created by gunavaran on 4/7/21.
//

#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <inttypes.h>
#include "frigate.h"

char *bit_decode_small_k(uint64_t binaryString, int length) {

    char *decodedString = malloc(sizeof(char) * length);
    uint64_t frame;
    for (int i = 0; i < length; i++) {
        frame = 3;
        frame = (frame << (length * 2 - (i * 2) - 2)) & binaryString;
        frame = frame >> (length * 2 - (i * 2) - 2);
        switch (frame) {
            case 0:
                decodedString[i] = 'A';
                break;
            case 1:
                decodedString[i] = 'C';
                break;
            case 2:
                decodedString[i] = 'G';
                break;
            case 3:
                decodedString[i] = 'T';
                break;
            default:
                printf("Invalid Input");
                break;
        }

    }
    printf("Decoded String: %s \n", decodedString);
//    return decodedString;
}

void
query(char *output_file, char *search_file_location) {
    FILE *search_file, *meta_file;
    char *kmer;
    int k_value, thread_id, i, num_files, temp_kvalue;
    uint64_t binary_kmer, read_kmer, read_count, write_length;
    char my_binary_output_location[200], num_string[5];

    char meta_file_location[200];
    strcpy(meta_file_location, "./output/");
    strcat(meta_file_location, output_file);
    strcat(meta_file_location, "_meta");

    if((meta_file = fopen(meta_file_location, "r")) == NULL){
        fprintf(stderr, "Can't open file %s\n", meta_file_location);
        exit(1);
    }

    fscanf(meta_file, "%d %d %" PRIu64, &k_value, &num_files, &write_length);
    fclose(meta_file);

    FILE *file_array[num_files];
    if ((search_file = fopen(search_file_location, "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", search_file_location);
        exit(1);
    }

    kmer = malloc(MAXLINELEN * sizeof(char));
    temp_kvalue = 0;

    //TO GET THE K VALUE
    if (fgets(kmer, MAXLINELEN, search_file) != NULL) {
        for (i = 0; i < MAXLINELEN; i++) {
            switch (kmer[i]) {
                case 'A':
                    temp_kvalue++;
                    break;
                case 'C':
                    temp_kvalue++;
                    break;
                case 'G':
                    temp_kvalue++;
                    break;
                case 'T':
                    temp_kvalue++;
                    break;
                case '\n':
                    i = MAXLINELEN;
                    break;
                default:
                    fprintf(stderr, "INVALID INPUT \n");
                    exit(EXIT_FAILURE);
            }
        }
    } else {
        fprintf(stderr, "Empty search file %s\n", search_file_location);
        exit(1);
    }

    if (k_value != temp_kvalue){
        printf("ERROR: kmer length mismatch in output file and seach file \n");
        exit(EXIT_FAILURE);
    }

    char binary_output_location[200];
    strcpy(binary_output_location, "./output/");
    strcat(binary_output_location, output_file);

    //OPEN FILES AND STORE THE POINTERS IN FILE ARRAY
    for (i = 0; i < num_files; i++) {
        sprintf(num_string, "%d", i);
        strcpy(my_binary_output_location, binary_output_location);
        strcat(my_binary_output_location, num_string);

//        printf("%s \n", my_binary_output_location);

        if ((file_array[i] = fopen(my_binary_output_location, "r")) == NULL) {
            fprintf(stderr, "Can't open file %s\n", my_binary_output_location);
            exit(EXIT_FAILURE);
        }

    }

    size_t kmer_byte_size = (k_value + 3) / 4;
    bool is_found;
    read_kmer = 0, read_count = 0;

    do {
        binary_kmer = 0;
        is_found = 0;
        for (i = 0; i < k_value; i++) {
            binary_kmer <<= 2u;
            switch (kmer[i]) {
                case 'A':
                    break;
                case 'C':
                    binary_kmer |= C_SHIFT;
                    break;
                case 'G':
                    binary_kmer |= G_SHIFT;
                    break;
                case 'T':
                    binary_kmer |= T_SHIFT;
                    break;
                default:
                    fprintf(stderr, "INVALID INPUT \n");
                    exit(10);
            }
        }

//        printf("binary encoded kmer %" PRIu64 "\n", binary_kmer);
//        bit_decode_small_k(binary_kmer, 15);

        thread_id = (int) (binary_kmer / write_length) % num_files;
//        printf("thread id: %d \n", thread_id);

        if (fread(&read_kmer, kmer_byte_size, 1, file_array[thread_id]) == 1) {
//            bit_decode_small_k(read_kmer, kmer_length);
            if (read_kmer > binary_kmer) {
                fseeko(file_array[thread_id], 0, SEEK_SET);
            } else if (read_kmer == binary_kmer) {
                fread(&read_count, COUNTER_BYTE_SIZE, 1, file_array[thread_id]);
                printf("%s : %lu \n", kmer, read_count);
                continue;
            } else {
                fread(&read_count, COUNTER_BYTE_SIZE, 1, file_array[thread_id]);
            }
        }

        while (fread(&read_kmer, kmer_byte_size, 1, file_array[thread_id]) == 1) {
//            bit_decode_small_k(read_kmer, kmer_length);
            if (read_kmer > binary_kmer) {
                printf("%s : %d \n", kmer, 0);
                is_found = 1;
                break;
            } else if (read_kmer == binary_kmer) {
                fread(&read_count, COUNTER_BYTE_SIZE, 1, file_array[thread_id]);
                printf("%s : %lu \n", kmer, read_count);
                is_found = 1;
                break;
            } else {
                fread(&read_count, COUNTER_BYTE_SIZE, 1, file_array[thread_id]);
            }
        }

        if (!is_found) {
            printf("%s : %d \n", kmer, 0);
        }


    } while (fgets(kmer, MAXLINELEN, search_file) != NULL);

}
