cmake_minimum_required(VERSION 3.5)
project(frigate C)

set(CMAKE_C_STANDARD 11)

add_definitions("-O3 -D_FILE_OFFSET_BITS=64 -pthread")

add_executable(frigate src/main.c src/systemProperties.h src/systemProperties.c src/frigate.c src/frigate.h src/frigate_parallel.c src/histogram.c src/histogram.h src/fileIO.c src/fileIO.h src/query.c src/query.h)

find_package(Threads)
target_link_libraries(frigate m ${CMAKE_THREAD_LIBS_INIT})
