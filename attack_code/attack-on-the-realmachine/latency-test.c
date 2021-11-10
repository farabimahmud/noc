#define _GNU_SOURCE
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

/**
 * L1D Cache Info
 *     size                     : 32 KB per core 
 *     cache line size          : 64 B
 *     number of cache lines    : 32KB/64B = 512
 *     associativity            : 8 way
 *     number of sets           : 512 / 8 = 64
 *
 * L2  Cache Info
 *     size                     : 1MB per tile (36 tiles)
 *     cache line size          : 64 B (check again)
 *     number of cache lines    : 1MB / 64B = 16384
 *     associativity            : 16 way
 *     number of sets           : 16384 / 16 = 1024
 */
#define HUGEPAGE_SIZE       (2<<29) //((1) * (1024) * (1024) * (1024))// 1GB
#define STRIDE              (4096)                         // access to different pages to fool cache prefetcher
#define MAX_NUM_ARR_IDX     (16)

int num_arr_idx = MAX_NUM_ARR_IDX;
int idx_array[MAX_NUM_ARR_IDX];
void prepare_idx(int* idx_array, uint64_t stride, uint64_t num_idx, uint64_t set, uint8_t shuffle)
{
    for (int i = 0; i < num_idx; i++) {
        idx_array[i] = i * stride + set;
    }

    if (shuffle) {
        for (int i = 0; i < num_idx; i++) {
            int tmp = idx_array[i];
            int randIdx = rand() % num_idx;
            idx_array[i] = idx_array[randIdx];
            idx_array[randIdx] = tmp;
        }
    }
}

uint8_t* arr = NULL;
void* allocate_hugepage(uint64_t len){
    void * ptr = NULL;
    int ret = posix_memalign(&ptr, HUGEPAGE_SIZE, len);
    if (ret){
        perror("posix memalign could not allocate\n");
    }
    ret = madvise(ptr, len, MADV_HUGEPAGE);
    memset(ptr, -1, len);

    printf("Allocated %ld memory at address %p\n", len, ptr);
    return ptr;
}

#define ARR_IDX_FAR_L2      (28672)
#define ARR_IDX_CLOSE_L2    (61440)
#define NUM_SECRET_BITS     (8)
int secret = 170;  //b10101010
void victim(uint64_t idx)
{
    uint8_t s = secret & idx;
    if (s == 0) {
        s ^= arr[ARR_IDX_CLOSE_L2]; // access latency < thresold
    } else {
        s ^= arr[ARR_IDX_FAR_L2];   // access latency > thresold
    }
}

#define MAX_LATENCY (500)
uint64_t latency_list[MAX_LATENCY][MAX_NUM_ARR_IDX];

unsigned long read_addr(char *adrs) {
    volatile unsigned long time;

    asm __volatile__(
            "    mfence             \n"
            "    lfence             \n"
            "    rdtsc              \n"
            "    lfence             \n"
            "    movl %%eax, %%esi  \n"
            "    movl (%1), %%eax   \n"
            "    lfence             \n"
            "    rdtsc              \n"
            "    subl %%esi, %%eax  \n"
            : "=a" (time)
            : "c" (adrs)
            : "%esi", "%edx"
            );
    return time;
}

int main(int argc, char * argv[])
{
    int cpuid = -1;
    int target = -1;
    if (argc == 3) {
        cpuid = atoi(argv[1]);
        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(cpuid, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

        target = atoi(argv[2]);
        if (target == 1) {
            num_arr_idx = 8;
        } else if (target == 2) {
            num_arr_idx = 16;
        } else if (target == 3) {
            num_arr_idx = 16;
        } else {
            printf("unknown target : %d\n", target); 
            return -1;
        }
    } else {
        printf("Usage: %s cpuid target-latency\n", argv[0]); 
        return -1;
    }

    // Use huge page (1GB) to allocate the array in the contiguous physical memory space
    arr = allocate_hugepage(HUGEPAGE_SIZE);

    //system("cat /proc/meminfo");
    //printf("num_l1d_cache_lines: %d\n", NUM_L1D_CACHE_LINES);
    //printf("num_idx:             %d\n", num_idx);

    // Use seed value 0 to make the sequence of random numbers deterministic
    srand(0);
    // Use the same access pattern but it might not detected by the cache prefetcher
    // due to the 4 KB stride
    prepare_idx(idx_array, STRIDE, num_arr_idx, 0x00/*set idx*/, 1/* shuffle*/);


    // Initialize latency_list with all zeros
    for (uint64_t lat = 0; lat < MAX_LATENCY; lat++) {
        for (uint64_t idx = 0; idx < num_arr_idx; idx++) {
            latency_list[lat][idx] = 0;
        }
    }

    #define NUM_TRY (1000)
    for (uint64_t try = 0; try < NUM_TRY; try++) {

        // TODO: Is the cache flush necessary?
        for (uint64_t i = 0; i < num_arr_idx; i++) {
            //_mm_clflush(&(arr[idx_array[i]]));
            _mm_clflush(arr + (i << 14));
        }
        _mm_mfence();

        uint32_t junk;

        if (target == 1 || target == 2) {
            for (uint64_t i = 0; i < num_arr_idx; i++) {
                //int idx  = idx_array[i];
                //junk ^= arr[idx];
                junk ^= *(arr + (i << 14));
            }
            _mm_mfence();
        }

        for (uint64_t i = 0; i < num_arr_idx; i++) {
            //int idx = idx_array[i];
            //_mm_mfence();
            uint64_t time = __rdtscp(&junk);
            //junk ^= arr[idx];
            //junk ^= idx_array[i];
            junk ^= *(arr + (i << 14));
            time = __rdtscp(&junk) - time;
            //uint64_t time = read_addr(arr + (i << 14));
            if (time < MAX_LATENCY) {
                latency_list[time][i]++;
            }
        }
    }

    char line[512];
    char tmp_str[64];
    sprintf(tmp_str, "latency-%d.csv", target);
    FILE* output_file = fopen(tmp_str, "w");

    sprintf(line, "latency");
    for (uint64_t i = 0; i < num_arr_idx; i++) {
        sprintf(tmp_str, ",%#x", idx_array[i]);
        strcat(line, tmp_str);
    }
    printf("%s\n", line);
    fprintf(output_file, "%s\n", line);

    int max_latency = MAX_LATENCY;
    //if (target == 1 || target == 2) {
    //    max_latency = 100;
    //}

    for (uint64_t lat = 0; lat < max_latency; lat++) {
        sprintf(line, "%d", lat);
        for (uint64_t idx = 0; idx < num_arr_idx; idx++) {
            sprintf(tmp_str, ",%d", latency_list[lat][idx]);
            strcat(line, tmp_str);
        }
        fprintf(output_file, "%s\n", line);
    }
    fclose(output_file);
    printf("program finished\n");
    return 0;
}

