#define _GNU_SOURCE
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <sys/mman.h>

/**
 * L1D Cache 32kB per core (total number of cache lines = 32KB/64B = 512)
 * L1D Assoc 8-way
 * L2  Cache 1MB per core x36 core
 * L2  Assoc 16-way 
 */

#define L1D_SIZE (32*1024)
#define CACHE_LINE_SIZE (64)
#define NUM_L1D_CACHE_LINES (L1D_SIZE/CACHE_LINE_SIZE) // 512
#define NUM_ARR_ELEMENTS ((NUM_L1D_CACHE_LINES)*(CACHE_LINE_SIZE)*(4)) // 32 * 4 = 128KB // why not work with doubling the l1d size?
#define HUGEPAGE_SIZE ((1) * (1024) * (1024) * (1024)) // 1GB

uint8_t* arr = NULL; //[NUM_ARR_ELEMENTS];
uint8_t secret = 123;
void victim(int idx)
{
    uint8_t s = secret & idx;
    if (s == 0) {
        s ^= arr[16384 * 64];
    } else {
        s ^= arr[32768 * 64];
    }
}

void* allocate_hugepage(uint64_t len){
    void * ptr = NULL;
    int ret = posix_memalign(&ptr, HUGEPAGE_SIZE, len);
    if (ret){
        perror("posix memalign could not allocate\n");
    }
    ret = madvise(ptr, len, MADV_HUGEPAGE);
    memset(ptr, -1, len);

    //printf("Allocated %ld memory at address %p\n", len, ptr);
    return ptr;
}

int main(int argc, char * argv[])
{
    int cpuid = -1;
    if (argc == 2) {
        cpuid = atoi(argv[1]);
        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(cpuid, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    } else {
        printf("Usage: %s cpuid\n", argv[0]); 
        return -1;
    }

    arr = allocate_hugepage(HUGEPAGE_SIZE);

    system("cat /proc/meminfo");
    printf("num_l1d_cache_lines: %d\n", NUM_L1D_CACHE_LINES);
    printf("num_arr_elements:    %d\n", NUM_ARR_ELEMENTS);

    unsigned int junk;
    for (unsigned int i = 0; i < NUM_ARR_ELEMENTS; i += (64)) {
        junk ^= arr[i];
    }
    _mm_mfence();

    char filename[] = "attack-example.csv";
    FILE* output_file = fopen(filename,"w");
    fprintf(output_file, "try,secret-bit,latency\n");

    int mask;
    unsigned long t1, t2;
    char str[512];
    char tmp[16];
    for (unsigned int try = 0; try < 1; try++) {
        mask = 1;
        memset(str, 0, 512);
        for (unsigned int i = 0; i < 8; i++) {
            t1 = __rdtscp(&junk);
            //victim(mask);
            junk ^= arr[16384 * 64];
            t2 = __rdtscp(&junk) - t1;
            sprintf(tmp, "%d,", t2);
            strcat(str, tmp);

            mask <<= 1;
            for (unsigned int i = 0; i < NUM_ARR_ELEMENTS; i += (64)) {
                junk ^= arr[i];
            }
            _mm_mfence();

        }
        fprintf(output_file, "%d,%s\n", try, str);
    }

    fclose(output_file);
    printf("program finished\n");
    return 0;

}

