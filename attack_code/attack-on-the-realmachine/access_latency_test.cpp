#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <random>
#include <ctime>
#include <x86intrin.h>
#include <unordered_set>
#include <iostream>

#define HUGEPAGE_SIZE 2<<29
#define PAGECOUNT 1
#define N_TESTCASES 100

#define CACHE_LINESIZE      64
#define LOG_CACHE_LINESIZE  6
#define LOG_CACHE_SETS_L1   6
#define CACHE_SETS_L1       64
#define CACHE_WAYS_L1       8


#define CACHE_WAYS_L2       8

#define LOG_CACHE_SETS_L3   11
#define CACHE_SETS_L3       (2 << LOG_CACHE_SETS_L3)
#define CACHE_SETS_L3_MASK  (CACHE_SETS_L3-1)
#define CACHE_WAYS_L3       16

#define LLC_ENTRIES 2048


using namespace std;

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

inline __attribute__((always_inline))
uint64_t time_access(uint64_t addr) {
    uint64_t cycles;

    asm volatile("mov %1, %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "mov %%eax, %%edi\n\t"
            "mov (%%r8), %%r8\n\t"
            "rdtsc\n\t"
            "lfence\n\t"
            "sub %%edi, %%eax\n\t"
    : "=a"(cycles) /*output*/
    : "r"(addr)
    : "r8", "edi");

    return cycles;
}


inline __attribute__((always_inline))
void flush(uint64_t addr) {
    asm volatile(
            "mfence     \n"
            "clflush 0(%0)   \n"
            :
            :"r"(addr)
            :
            );
}


void free_ptr(char * ptr){
    if (ptr != NULL){
        free(ptr);
    }
}


int main(int argc, char * argv[]){
    std::srand(std::time(nullptr));
    int cpuid = -1;
    uint8_t cache_set_target = 1; 

    if (argc ==2){
        cpuid = atoi(argv[1]);

    }

    if (argc == 3){
        cpuid = atoi(argv[1]);
        cache_set_target = atoi(argv[2]);
    }

    char * ptr = NULL;
    uint64_t len = PAGECOUNT * HUGEPAGE_SIZE;
    uint8_t x;
    ptr =(char*) allocate_hugepage(len);
    FILE *fp;
    fp = fopen("latency.csv", "a");
    uint64_t k,t;
    for (k = 0 ; k < 16*CACHE_SETS_L3; k++){
        for (int j=0; j<N_TESTCASES; j++){
            
            for (int i=0; i< CACHE_SETS_L3; i++){
                ptr[i] ^= k ; 
            }
            unsigned junk;
            uint64_t t1 = __rdtscp(&junk);
            x ^= ptr[k]  ;
            uint64_t t2 = __rdtscp(&junk);            
            fprintf(fp, "%d,%ld,%lu\n",cpuid,k, t2-t1);
        }
    }
    fclose(fp);       
    free_ptr(ptr);
    return 0;
}
