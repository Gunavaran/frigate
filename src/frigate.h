//
// Created by gunavaran on 9/16/20.
//

#include <stdint.h>
#include <stdbool.h>

#ifndef KMER_COUNTING_DIRECTACCESS_H
#define KMER_COUNTING_DIRECTACCESS_H

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define THRESHOLD_MIN 2
#define DEF_MEMORY_SIZE 4
#define DEF_K_VALUE 10
#define DEF_VERBOSE false

#define COUNTER_BIT_SIZE 8
#define COUNTER_BYTE_SIZE (COUNTER_BIT_SIZE/8)
#define DATA_QUEUE_SIZE 5
#define GET_TIME(x);    if (clock_gettime(CLOCK_MONOTONIC, &(x)) < 0) \
                    { perror("gettime( ):"); exit(EXIT_FAILURE); }

#define A_SHIFT 0                     //00
#define C_SHIFT 1u                    //01
#define G_SHIFT 2u                    //10
#define T_SHIFT 3u                    //11

#define MAXLINELEN 6400

#if COUNTER_BIT_SIZE == 8
typedef uint8_t counter_t;
#elif COUNTER_BIT_SIZE == 16
typedef uint16_t counter_t;
#endif

uint64_t counter_max;
int GLOBAL_THREAD_COUNT, COUNT_MIN, kmer_length, MAX_MEMORY;
bool IS_CANONICAL, IS_VERBOSE;

void kmer_counter_sequential(int k_value, char *fastq_location, char *binary_output_file);

void kmer_counter_parallel(int k_value, char *fastq_location, char *binary_output_file);

void *parallel_direct_count(void *threadarg);

void compare_histogram(char *file1, char *file2, int range, char *algo_name);

float elapsedTime(struct timespec *begin, struct timespec *end, long *sec, long *nsec);

static inline void *single_read_count(char *line);

void usage();

void set_default_params();

void initialize_params();


#endif //KMER_COUNTING_DIRECTACCESS_H
