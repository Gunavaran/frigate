//
// Created by gunavaran on 9/16/20.
//

#include <inttypes.h>
#include <time.h>
#include "frigate.h"
#include "fileIO.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "systemProperties.h"

//only if memory cam hold the entire data structure

/**
 * THIS VERSION := IMPROVING THE CANONICAL CALCULATION.
 * Rather thean calculationg the canonical kmer everytime from scratch
 * we can keep the old one and do a left shift and append the next kmer
 */

void usage() {
    printf("-----------------------------------------------------------------------------------------\n");
    printf("                                      Frigate %d.%d \n", MAJOR_VERSION, MINOR_VERSION);
    printf("-----------------------------------------------------------------------------------------\n");
    printf("Frigate can perform FOUR operations: \n");
    printf("    1. count:       counting k-mers \n");
    printf("    2. histogram:   generating histogram for k-mer counting results \n");
    printf("    3. dump:        output k-mer counting results in readable format \n");
    printf("    4. query:       query k-mers \n");
    printf("-----------------------------------------------------------------------------------------\n\n");
    printf("1. COUNTING K-MERS ======================================================================\n");
    printf("    Usage: ./frigate count [options] <input_file> <output_file> \n");
    printf("    <input_file>        single FASTQ file\n");
    printf("    <output_file>       preferred name for the output file(s)\n");
    printf("    [options] : \n");
    printf("        -k <size>           size of k-mers (default: %d) \n", DEF_K_VALUE);
    printf("        -t <value>          number of threads (default: no. of CPU cores) \n");
    printf("        -m <size>           max amount of RAM in GB (default: size of RAM) \n");
    printf("        -l <value>          exclude k-mers with count less than <value> (default: %d)\n", THRESHOLD_MIN);
    printf("        -d                  disable canonical counting \n");
    printf("        -v                  verbose mode; displays the parameter settings (default: %d) \n\n", DEF_VERBOSE);
    printf("2. HISTOGRAM =============================================================================\n");
    printf("    Usage: ./frigate histogram <output_file> <histo_file> \n");
    printf("    <output_file>           same file name used when counting\n");
    printf("    <histo_file>            preferred file name to generate the histogram\n\n");
    printf("3. READABLE K-MER COUNTING OUTPUT ========================================================\n");
    printf("    Usage: ./frigate dump <output_file> <dump_file>\n");
    printf("    <output_file>           same file name used when counting\n");
    printf("    <dump_file>             preferred file name to generate the readable outputs\n\n");
    printf("4. QUERYING K-MERS =======================================================================\n");
    printf("    Usage: ./frigate query <output_file> <query_file>\n");
    printf("    <output_file>           same file name used when counting\n");
    printf("    <query_file>            file containing the k-mers to be queried (k-mers should be separated using \\n) \n");

}

void initialize_params() {
    kmer_length = DEF_K_VALUE;
    MAX_MEMORY = 0;
    IS_CANONICAL = true;
    IS_VERBOSE = false;
    GLOBAL_THREAD_COUNT = 0;
    COUNT_MIN = THRESHOLD_MIN;
}

void set_default_params() {

    if (MAX_MEMORY == 0) {
        if ((MAX_MEMORY = (int) (get_RAM_size() / (1024 * 1024 * 1024))) == 0) {
            printf("Couldn't access RAM size. Using default value of %d \n", DEF_MEMORY_SIZE);
            MAX_MEMORY = DEF_MEMORY_SIZE;
        }
    }

    if (GLOBAL_THREAD_COUNT == 0) {
        GLOBAL_THREAD_COUNT = (int) get_cpu_cores();
    }

}

void
kmer_counter_sequential(int k_value, char *fastq_location, char *binary_output_file) {

    struct timespec start_time, end_time, binary_write_start, binary_write_end;
    unsigned long sec, nsec;
    float elapsed_msec;GET_TIME(start_time);

    uint64_t window_64bit, canonical, permutations, minimum;
    int m, encode_index, i;
    bool is_over, is_repeat;
    long array_size_in_bytes, node_size_in_bits;

    const uint64_t CANONICAL_C_SHIFT = 1lu << ((k_value - 1) * 2);
    const uint64_t CANONICAL_G_SHIFT = 2lu << ((k_value - 1) * 2);
    const uint64_t CANONICAL_T_SHIFT = 3lu << ((k_value - 1) * 2);

    counter_max = (uint64_t) pow(2, COUNTER_BIT_SIZE) - 1;

    //create array
    permutations = (uint64_t) pow(4, k_value);
//    printf("No. of permutations: %ld \n", permutations);

    if (MAX_MEMORY < permutations * sizeof(counter_t) / (1024 * 1024 * 1024)) {
        printf("ERROR: System does not have sufficient memory \n");
        exit(EXIT_FAILURE);
    }

    counter_t *array_main = calloc(permutations, sizeof(counter_t));
    if (array_main == NULL) {
        printf("Error! memory not allocated.");
        exit(0);
    }


    //=============================READ THE FASTQ FILE=======================
    FILE *lfp;
    char *line = malloc(sizeof(char) * MAXLINELEN + 1);

    if ((lfp = fopen(fastq_location, "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", fastq_location);
        exit(1);
    }

    char *aline; /* this variable is not used, it suppresses a compiler warning */

    //TO REMOVE THE FIRST TWO BITS
    uint64_t first_2_bits = 3;
    for (m = 2; m < k_value; m++) {
        first_2_bits = (first_2_bits << 2u) | 3u;
    }

    while ((aline = fgets(line, MAXLINELEN, lfp)) != NULL) {
        fgets(line, MAXLINELEN, lfp);
        encode_index = 0;
        is_over = 0;

        while (!is_over) {
            is_repeat = 0;
            window_64bit = 0;
            canonical = 0;
            for (i = encode_index; i < encode_index + k_value; i++) {
                window_64bit = window_64bit << 2u;
                canonical >>= 2u;
                switch (line[i]) {
                    case 'A':
                        canonical |= CANONICAL_T_SHIFT;
                        break;
                    case 'C':
                        window_64bit |= C_SHIFT;
                        canonical |= CANONICAL_G_SHIFT;
                        break;
                    case 'G':
                        window_64bit |= G_SHIFT;
                        canonical |= CANONICAL_C_SHIFT;
                        break;
                    case 'T':
                        window_64bit |= T_SHIFT;
                        break;
                    case '\n':
                        is_over = 1;
                        break;
                    default:
                        encode_index = i + 1;
                        is_repeat = 1;
                        break;
                }

                if (is_repeat || is_over) {
                    break;
                }
            }

            if (is_repeat || is_over) {
                continue;
            }

            if (IS_CANONICAL) {
                minimum = window_64bit;

                if (canonical < window_64bit) {
                    minimum = canonical;
                }

                if (array_main[minimum] < counter_max) {
                    array_main[minimum] += 1;
                }
            } else {
                if (array_main[window_64bit] < counter_max) {
                    array_main[window_64bit] += 1;
                }
            }

            //REMOVE FIRST TWO BITS
            window_64bit &= first_2_bits;

            for (i = encode_index + k_value;; i++) {
                window_64bit <<= 2u;
                canonical >>= 2u;
                switch (line[i]) {
                    case 'A':
                        canonical |= CANONICAL_T_SHIFT;
                        break;
                    case 'C':
                        window_64bit |= C_SHIFT;
                        canonical |= CANONICAL_G_SHIFT;
                        break;
                    case 'G':
                        window_64bit |= G_SHIFT;
                        canonical |= CANONICAL_C_SHIFT;
                        break;
                    case 'T':
                        window_64bit |= T_SHIFT;
                        break;
                    case '\n':
                        is_over = 1;
                        break;
                    default:
                        encode_index = i + 1;
                        is_repeat = 1;
                        break;
                }
                if (is_repeat || is_over) {
                    break;
                }

                if (IS_CANONICAL) {
                    minimum = window_64bit;

                    if (canonical < window_64bit) {
                        minimum = canonical;
                    }

                    if (array_main[minimum] < counter_max) {
                        array_main[minimum] += 1;
                    }
                } else {
                    if (array_main[window_64bit] < counter_max) {
                        array_main[window_64bit] += 1;
                    }
                }

                window_64bit &= first_2_bits;

            }

            if (is_repeat) {
                continue;
            }

            is_over = 1;
        }
        fgets(line, MAXLINELEN, lfp);
        fgets(line, MAXLINELEN, lfp);


    }
    fclose(lfp);

    char binary_output_location[200];
    strcpy(binary_output_location, "./output/");
    strcat(binary_output_location, binary_output_file);


    //WRITE BINARY OUTPUT FILE
    write_meta_data(GLOBAL_THREAD_COUNT, k_value, permutations, binary_output_location);

    strcat(binary_output_location, "0");

    GET_TIME(binary_write_start);
    write_binary_output(binary_output_location, array_main, permutations, k_value);GET_TIME(binary_write_end);
    elapsed_msec = elapsedTime(&binary_write_start, &binary_write_end, &sec, &nsec);
    printf("BINARY FILE WRITING TIME IN SECONDS: %f \n", elapsed_msec / 1000);

    free(array_main);

    GET_TIME(end_time);
    elapsed_msec = elapsedTime(&start_time, &end_time, &sec, &nsec);
    printf("time elapsed in seconds: %f \n", elapsed_msec / 1000);

#ifdef RF
    //WRITE THE RESULT TO FILE IN HUMAN READABLE FORM
//    write_readable_output(binary_output_location, readable_output_location, kmer_length, permutations);
#endif
}

void compare_histogram(char *file1, char *file2, int range, char *algo_name) {
    FILE *file_ptr;
    int key, value, i;
    int array1[range], array2[range];
    char str[5];

    if ((file_ptr = fopen(file1, "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", file1);
        exit(1);
    }

    for (i = 0; i < range; i++) {
        fscanf(file_ptr, "%u %s %u", &key, str, &array1[i]);
    }
    fclose(file_ptr);

    if ((file_ptr = fopen(file2, "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", file2);
        exit(1);
    }

    if (!strcmp(algo_name, "kmc2")) {
        for (i = 0; i < range; i++) {
            fscanf(file_ptr, "%d %d", &key, &array2[i]);
        }
    }


    fclose(file_ptr);

    for (i = 0; i < range; i++) {
        if (array1[i] != array2[i]) {
            printf("different index: %d , values: %d, %d \n", i, array1[i], array2[i]);
        }
    }


}


float elapsedTime(struct timespec *begin, struct timespec *end, long *sec, long *nsec) {
    if (end->tv_nsec < begin->tv_nsec) {
        *nsec = 1000000000 - (begin->tv_nsec - end->tv_nsec);
        *sec = end->tv_sec - begin->tv_sec - 1;
    } else {
        *nsec = end->tv_nsec - begin->tv_nsec;
        *sec = end->tv_sec - begin->tv_sec;
    }
    return (float) (*sec) * 1000 + ((float) (*nsec)) / 1000000;

}
