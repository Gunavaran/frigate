//
// Created by gunavaran on 9/16/20.
//

#include <inttypes.h>
#include "frigate.h"
#include <pthread.h>
#include "systemProperties.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "fileIO.h"

typedef struct data_queue {
    bool flag[DATA_QUEUE_SIZE];
    char queue[DATA_QUEUE_SIZE][MAXLINELEN + 1];
} data_queue;

int kmer_length, number_of_threads;
counter_t *array_main;
volatile data_queue *thread_kmer_queue;
volatile bool is_end_of_file, start_binary_write;
volatile int thread_complete_count;
uint64_t CANONICAL_C_SHIFT, CANONICAL_G_SHIFT, CANONICAL_T_SHIFT, first_2_bits, permutations, write_length;
char binary_output_location[200];
//only if memory can hold the entire data structure
void kmer_counter_parallel(int k_value, char *fastq_location, char *binary_output_file) {

    struct timespec start_time, end_time, binary_write_start, binary_write_end;
    unsigned long sec, nsec;
    float elapsed_msec;GET_TIME(start_time);
    FILE *lfp;
    long array_size_in_bytes, node_size_in_bits, cacheline_size;
    int rc, m; //for the thread
    bool is_set;

    counter_max = (uint64_t) pow(2, COUNTER_BIT_SIZE) - 1;
//    printf("counter max =  %ld \n", counter_max);
    number_of_threads = GLOBAL_THREAD_COUNT - 1;
    kmer_length = k_value;

    start_binary_write = 0;
    thread_complete_count = 0;

    permutations = (uint64_t) pow(4, k_value);

    if (MAX_MEMORY < permutations * sizeof(counter_t) / (1024 * 1024 * 1024)) {
        printf("ERROR: System does not have sufficient memory \n");
        exit(EXIT_FAILURE);
    }

    array_main = calloc(permutations, sizeof(counter_t));
    thread_kmer_queue = calloc((size_t) number_of_threads, sizeof(data_queue));

    cacheline_size = get_cacheline_size();
    write_length = (cacheline_size * 5) / sizeof(counter_t);

//    printf("write length: %lu \n", write_length);
    strcpy(binary_output_location, "./output/");
    strcat(binary_output_location, binary_output_file);

    CANONICAL_C_SHIFT = 1lu << ((kmer_length - 1) * 2);
    CANONICAL_G_SHIFT = 2lu << ((kmer_length - 1) * 2);
    CANONICAL_T_SHIFT = 3lu << ((kmer_length - 1) * 2);

    //TO REMOVE THE FIRST TWO BITS
    first_2_bits = 3;
    for (m = 2; m < kmer_length; m++) {
        first_2_bits = (first_2_bits << 2u) | 3u;
    }

    char *line = malloc(sizeof(char) * MAXLINELEN + 1);

    if ((lfp = fopen(fastq_location, "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", fastq_location);
        exit(1);
    }

    //INITIALIZE THREADS
    pthread_t threads[number_of_threads];
    int threadids[number_of_threads];
    //FILL THE DATA QUEUE FIRST BEFORE STARTING THE THREADS

    is_end_of_file = 0;

    for (int i = 0; i < number_of_threads; i++) {
        while (fgets(line, MAXLINELEN, lfp) != NULL) {
            fgets(thread_kmer_queue[i].queue[0], MAXLINELEN, lfp);
            thread_kmer_queue[i].flag[0] = 1;
            threadids[i] = i;
            rc = pthread_create(&threads[i], NULL, parallel_direct_count, (void *) threadids[i]);
            if (rc) {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
            break;
        }
        fgets(line, MAXLINELEN, lfp);
        fgets(line, MAXLINELEN, lfp);
    }

    uint64_t queue_count = 0;
    int full_count = 0;
    int temp_count;
    uint64_t index;

    while (fgets(line, MAXLINELEN, lfp) != NULL) {
        is_set = 1;
        full_count = 0;
        while (is_set) {
            for (int j = 0; j < DATA_QUEUE_SIZE; j++) {
                index = queue_count % number_of_threads;
                if (thread_kmer_queue[index].flag[j] == 0) {
                    fgets(thread_kmer_queue[index].queue[j], MAXLINELEN, lfp);
                    thread_kmer_queue[index].flag[j] = 1;
                    is_set = 0;
                    fgets(line, MAXLINELEN, lfp);
                    fgets(line, MAXLINELEN, lfp);
                    break;
                }
            }
            queue_count++;

            if (is_set) {
                full_count++;
            }

            if (full_count == number_of_threads) {
                fgets(line, MAXLINELEN, lfp);
                single_read_count(line);
                fgets(line, MAXLINELEN, lfp);
                fgets(line, MAXLINELEN, lfp);
                temp_count = 1;
                while ((temp_count < DATA_QUEUE_SIZE / 2) && (fgets(line, MAXLINELEN, lfp) != NULL)) {
                    fgets(line, MAXLINELEN, lfp);
                    single_read_count(line);
                    temp_count++;
                    fgets(line, MAXLINELEN, lfp);
                    fgets(line, MAXLINELEN, lfp);
                }

                is_set = 0;
            }
        }

    }
//    printf("is end of file reached \n");
    is_end_of_file = 1;
    fclose(lfp);

    while (thread_complete_count < number_of_threads) {
//        printf("thread complete count: %d \n", thread_complete_count);
    }

    start_binary_write = 1;
//    printf("start binary write set to 1 \n");

    char my_binary_output_location[200];
    strcpy(my_binary_output_location, binary_output_location);
    strcat(my_binary_output_location, "0");

    //WRITE BINARY OUTPUT
    write_meta_data(GLOBAL_THREAD_COUNT, kmer_length, write_length, binary_output_location);
    GET_TIME(binary_write_start);
    parallel_write_binary_output(my_binary_output_location, array_main, 0, k_value, write_length, permutations);
    GET_TIME(binary_write_end);
    elapsed_msec = elapsedTime(&binary_write_start, &binary_write_end, &sec, &nsec);
    printf("BINARY FILE WRITING TIME IN SECONDS: %f \n", elapsed_msec / 1000);

    for (int i = 0; i < number_of_threads; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }

    //====================================================================================
//    uint64_t singleton_count = 0;
//    uint64_t total_kmer_count = 0;
//    uint64_t greater_255_count = 0;
//
//    for (uint64_t t = 0; t < permutations; t++) {
//        if (array_main[t] == 1) {
//            singleton_count++;
//        }
//
//        if (array_main[t] > 0) {
//            total_kmer_count++;
//        }
//
//        if (array_main[t] > 255) {
//            greater_255_count++;
//        }
//    }
//    printf("k value: %d \n", k_value);
//    printf("singleton count: %" PRIu64 "\n", singleton_count);
//    printf("total kmer count: %" PRIu64 "\n", total_kmer_count);
//    printf("greater than 255 count: %" PRIu64 "\n", greater_255_count);
    //=====================================================================================

    free(array_main);
    free(thread_kmer_queue);

    GET_TIME(end_time);
    elapsed_msec = elapsedTime(&start_time, &end_time, &sec, &nsec);
    printf("time elapsed in seconds: %f \n", elapsed_msec / 1000);

}

void *parallel_direct_count(void *threadarg) {
    int thread_id = (int) threadarg;
    char *line;
    bool is_terminate = 1, is_over, is_repeat, is_last_iter = 0;
    int data_queue_index, encode_index;
    uint64_t window_64bit, canonical, minimum;
    struct timespec binary_write_start, binary_write_end;
    unsigned long sec, nsec;
    float elapsed_msec;
    counter_t old_val;

    while (is_terminate) {

        for (data_queue_index = 0; data_queue_index < DATA_QUEUE_SIZE; data_queue_index++) {
            if (thread_kmer_queue[thread_id].flag[data_queue_index] == 1) {
                break;
            }
        }

        //THE DATA QUEUE IS EMPTY
        if (data_queue_index == DATA_QUEUE_SIZE) {
            if (is_last_iter) {
                is_terminate = 0;
                continue;
            }
            if (is_end_of_file == 0) {
                continue;
            } else {
                is_last_iter = 1;
                continue;
            }

        }

        line = thread_kmer_queue[thread_id].queue[data_queue_index];
//        printf("%s \n", line);
        single_read_count(line);

        thread_kmer_queue[thread_id].flag[data_queue_index] = 0;
    }

    __sync_fetch_and_add(&thread_complete_count, 1);
//    printf("thread %d incremented the counter \n", thread_id);
    while (!start_binary_write) {

    }
//    printf("thread %d starting binary write \n", thread_id);
    //WRITE BINARY OUTPUT
    char my_binary_output_location[200];
    char num_string[5];

    sprintf(num_string, "%d", thread_id + 1);
    strcpy(my_binary_output_location, binary_output_location);
    strcat(my_binary_output_location, num_string);

//    GET_TIME(binary_write_start);
    parallel_write_binary_output(my_binary_output_location, array_main, thread_id + 1, kmer_length, write_length, permutations);
//    GET_TIME(binary_write_end);
//    elapsed_msec = elapsedTime(&binary_write_start, &binary_write_end, &sec, &nsec);
//    printf("BINARY FILE WRITING TIME IN SECONDS: %f \n", elapsed_msec / 1000);
//    printf("thread %d completed binary write \n", thread_id);
    pthread_exit(NULL);

}

static inline void *single_read_count(char *line) {

    int encode_index;
    bool is_over, is_repeat;
    uint64_t window_64bit, canonical, minimum;
    counter_t old_val;

    encode_index = 0;
    is_over = 0;

    while (!is_over) {
        is_repeat = 0;
        window_64bit = 0;
        canonical = 0;
        for (int i = encode_index; i < encode_index + kmer_length; i++) {
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

            do {
                old_val = array_main[minimum];
                if (old_val == counter_max) {
                    break;
                }
            } while (!__sync_bool_compare_and_swap(&array_main[minimum], old_val, old_val + 1));
        } else {
            do {
                old_val = array_main[window_64bit];
                if (old_val == counter_max) {
                    break;
                }
            } while (!__sync_bool_compare_and_swap(&array_main[window_64bit], old_val, old_val + 1));

        }

        //REMOVE FIRST TWO BITS
        window_64bit &= first_2_bits;

        for (int i = encode_index + kmer_length;; i++) {
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

                do {
                    old_val = array_main[minimum];
                    if (old_val == counter_max) {
                        break;
                    }
                } while (!__sync_bool_compare_and_swap(&array_main[minimum], old_val, old_val + 1));
            } else {
                do {
                    old_val = array_main[window_64bit];
                    if (old_val == counter_max) {
                        break;
                    }
                } while (!__sync_bool_compare_and_swap(&array_main[window_64bit], old_val, old_val + 1));

            }

            window_64bit &= first_2_bits;

        }

        if (is_repeat) {
            continue;
        }

        is_over = 1;
    }

}



//    uint64_t count_array[number_of_threads + 1];
//    memset(count_array, 0, sizeof(uint64_t) * (number_of_threads + 1));
//    uint64_t temp_length = permutations / (number_of_threads + 1);
//    uint64_t n;
//    for (int c = 0; c < number_of_threads + 1; c++) {
//        for (n = 0; n < temp_length; n++) {
//            if (array_main[c * temp_length + n] >= COUNT_MIN) {
//                count_array[c]++;
//            }
//        }
//    }
//
//    for (n = (number_of_threads + 1) * temp_length; n < permutations; n++) {
//        if (array_main[n] >= COUNT_MIN) {
//            count_array[number_of_threads]++;
//        }
//    }
//
//    for (int c = 0; c < number_of_threads + 1; c++) {
//        printf("thread %d : %ld \n", c, count_array[c]);
//    }