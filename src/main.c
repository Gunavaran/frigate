
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <memory.h>
#include "frigate.h"
#include "histogram.h"
#include "fileIO.h"
#include "query.h"

#define _FILE_OFFSET_BITS 64

int main(int argc, char **argv) {

    int i;
    char *input_file_location, *binary_output_file, *readable_output_location, *histogram_file, *compare_location;

    if (argc == 1 || !strcmp(argv[1], "-h")) {
        usage();
        exit(EXIT_SUCCESS);
    }

    if (argc < 4) {
        printf("ERROR: Missing parameters. (-h for help) \n");
        exit(EXIT_FAILURE);
    }

    initialize_params();

    if (!strcmp(argv[1], "count")) {
        for (i = 2; i < argc && argv[i][0] == '-'; i++) {
            switch (argv[i][1]) {
                case 'k':
                    kmer_length = atoi(argv[++i]);
                    break;
                case 't':
                    GLOBAL_THREAD_COUNT = atoi(argv[++i]);
                    break;
                case 'l':
                    COUNT_MIN = atoi(argv[++i]);
                    break;
                case 'd':
                    IS_CANONICAL = false;
                    break;
                case 'm':
                    MAX_MEMORY = atoi(argv[++i]);
                    break;
                case 'v':
                    IS_VERBOSE = true;
                    break;
                default:
                    usage();
                    exit(EXIT_FAILURE);
            }
        }

        set_default_params();

        if (argc - i < 2) {
            printf("ERROR: Missing parameters. (-h for help) \n");
            exit(EXIT_FAILURE);
        }

        input_file_location = argv[i++];
        binary_output_file = argv[i];

        if (IS_VERBOSE) {
            printf("================= USED PARAMETERS =================\n");
            printf("    k-mer length:           %d \n", kmer_length);
            printf("    # threads:              %d \n", GLOBAL_THREAD_COUNT);
            printf("    min count threshold:    %d \n", COUNT_MIN);
            printf("    usable max memory:      %d GB\n", MAX_MEMORY);
            printf("    canonical counting:     %s\n", (IS_CANONICAL) ? "ENABLED" : "DISABLED");
            printf("    input file:             %s \n", input_file_location);
//            printf("    intput file format:     %s\n", "FASTQ");
            printf("    output file:            %s \n ", binary_output_file);
        }


        if (GLOBAL_THREAD_COUNT == 1) {
            kmer_counter_sequential(kmer_length, input_file_location, binary_output_file);
        } else {
            kmer_counter_parallel(kmer_length, input_file_location, binary_output_file);
        }

    } else if (!strcmp(argv[1], "histogram")) {

        histogram(argv[2], argv[3]);

    } else if (!strcmp(argv[1], "dump")) {

        write_readable_output(argv[2], argv[3]);

    } else if (!strcmp(argv[1], "query")) {

        query(argv[2], argv[3]);

    } else {
        printf("ERROR: Missing/Incorrect command. (-h for help) \n");
        exit(EXIT_FAILURE);
    }

    pthread_exit(NULL);

}
