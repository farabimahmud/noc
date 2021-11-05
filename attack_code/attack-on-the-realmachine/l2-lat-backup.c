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

#define L1D_SIZE (64*1024)
#define CACHE_LINE_SIZE (64)
#define NUM_L1D_CACHE_LINES (L1D_SIZE/CACHE_LINE_SIZE) // 512
#define NUM_ARR_ELEMENTS ((NUM_L1D_CACHE_LINES)*(CACHE_LINE_SIZE)*(4)) // 32 * 2 = 64KB 
#define HUGEPAGE_SIZE ((1) * (1024) * (1024) * (1024)) // 1GB

int idx_array[NUM_ARR_ELEMENTS];
void shuffle_idx(int* idx_array, uint64_t stride, uint64_t num_idx)
{
    for (int i = 0; i < num_idx; i++) {
        idx_array[i] = i * stride;
    }

    //for (int i = 0; i < num_idx; i++) {
    //    int temp = idx_array[i];
    //    int randomIndex = rand() % num_idx;
    //    idx_array[i] = idx_array[randomIndex];
    //    idx_array[randomIndex] = temp;
    //}

    //printf("num_idx = %d\n", num_idx);
    //for (int i = 0; i < num_idx; i++) {
    //    printf("idx_arry[%d] = %d\n", i, idx_array[i]);
    //}
}

uint8_t* arr = NULL; //[NUM_ARR_ELEMENTS];
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

    uint64_t stride = 4096;
    uint64_t num_idx = NUM_ARR_ELEMENTS / stride;
    shuffle_idx(idx_array, stride, num_idx);

    for (int try = 0; try < 1; try++) {
        unsigned int junk;
        for (unsigned int i = 0; i < NUM_ARR_ELEMENTS; i += 4096) {
            junk ^= arr[i];
        }
        _mm_mfence();

        uint64_t time;
        uint64_t latency_list[NUM_ARR_ELEMENTS];
        //for (uint64_t i = 0; i < num_idx; i++) {
        for (uint64_t i = 0; i < NUM_ARR_ELEMENTS; i += 4096) {
            time = __rdtscp(&junk);
            //junk ^= arr[idx_array[i]];
            junk ^= arr[i];
            latency_list[i] = __rdtscp(&junk) - time;
        }

        char filename[] = "l2-lat.csv";
        FILE* output_file = fopen(filename,"a");
        fprintf(output_file, "try,offset,latency\n");
        //for (uint64_t i = 0; i < num_idx; i++) {
        for (uint64_t i = 0; i < NUM_ARR_ELEMENTS; i += 4096) {
            //fprintf(output_file, "%d,%d,%ld\n", try, idx_array[i], latency_list[i]);
            fprintf(output_file, "%d,%d,%ld\n", try, i, latency_list[i]);
        }
        fclose(output_file);

        //for (int64_t i = NUM_ARR_ELEMENTS - 64; i >= 0; i -= (64)) {
        //    _mm_clflush(&(arr[i]));
        //    //junk ^= arr[i];
        //}
        //_mm_mfence();
    }
    printf("program finished\n");
    return 0;
}

