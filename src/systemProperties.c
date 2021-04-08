//
// Created by gunavaran on 6/2/20.
//

#include "systemProperties.h"
#include <unistd.h>
#include <sys/sysinfo.h>

/**
 *
 * @return Cacheline size in bytes
 */
long get_cacheline_size(void) {
    //TODO: should implement it for other platforms as well
    return sysconf(_SC_LEVEL1_ICACHE_LINESIZE);

}

/**
 *
 * @return total RAM size in Bytes
 */
unsigned long long get_RAM_size(void) {
    struct sysinfo info;
    if(sysinfo(&info) < 0)
        return 0;
    return info.totalram;
}

/**
 *
 * @return number of logical cores in the system
 */
long get_cpu_cores(){
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    return number_of_processors;
}