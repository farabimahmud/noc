#define _GNU_SOURCE
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>

/**
 * L1D Cache 32kB per core (total number of cache lines = 32KB/64B = 512)
 * L1D Assoc 8-way
 * L2  Cache 1MB per core x36 core
 * L2  Assoc 16-way 
 */

#define LLC_SIZE (2 << 20)
#define L1D_SIZE (32*1024)
#define CACHE_LINE_SIZE (64)
#define NUM_L1D_CACHE_LINES (L1D_SIZE/CACHE_LINE_SIZE)
#define NUM_ARR_ELEMENTS ((NUM_L1D_CACHE_LINES)*(CACHE_LINE_SIZE)*(2))

//size_t size = 4;
uint8_t arr2[NUM_ARR_ELEMENTS];
			// idx 0 * 64 -> node 2
                         // idx 1 * 64 -> node 3
                         // idx 2 * 64 -> node 4
                         // idx 61 * 64-> node 63
                         // idx 62 * 64-> node 0
                         // ...
                         // idx 125 * 64-> node 63
                         // idx 126 * 64-> node 0
uint8_t secret = 123;

int main(int argc, char * argv[])
{
    srand(time(0));
    int cpuid = -1;

    if (argc == 2){
        cpuid  = atoi(argv[1]);
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(cpuid, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    unsigned long t[NUM_ARR_ELEMENTS];
    uint8_t x;
    unsigned int junk;
    char filename[] = "mem-lat.log";
    FILE* output_file = fopen(filename,"w");

    //printf("num_l1d_cache_lines: %d\n", NUM_L1D_CACHE_LINES);
    //printf("num_arr_elements:    %d\n", NUM_ARR_ELEMENTS);

    for (int i = 0; i < NUM_L1D_CACHE_LINES * 2; i++) {
	int idx = rand() % (NUM_L1D_CACHE_LINES *2);
        //_mm_clflush(&(arr2[i * CACHE_LINE_SIZE]));
        _mm_clflush(&(arr2[32 * CACHE_LINE_SIZE]));
	_mm_mfence();
        unsigned long time1 = __rdtscp(&junk);
        //x ^= arr2[i * CACHE_LINE_SIZE];
        x ^= arr2[32 * CACHE_LINE_SIZE];
        unsigned long time2 = __rdtscp(&junk);
        t[i] = time2 - time1;
    }

    for (int i = 0; i < NUM_L1D_CACHE_LINES * 2; i++) {
        fprintf(output_file, "%d, %d, %ld\n", i, cpuid, t[i]);
        //fprintf(output_file, "%d, %d, %ld\n", i, cpuid, t[i]>50?50:t[i]);
    }
    fclose(output_file);

}

