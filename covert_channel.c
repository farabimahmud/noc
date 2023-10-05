#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>
#include <pthread.h>

//------------------------------------------------------------------
// Important Baseline Architecture Configuration
// L1: 64KB, 2-way,
// 8x8 MESH Topology
//------------------------------------------------------------------

//------------------------------------------------------------------
// Mapping to L1D Set and LLC Slice ID 
// 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
//          <----llc id-----> <------set idx-------> <-blk off->
//------------------------------------------------------------------

pthread_barrier_t   barrier; // the barrier synchronization object


#define NTHREADS 1
#define NUM_CACHE_LINES 1024 * 128
#define LINE_SIZE 64

//-------------------------------------------
// Note: In my (sungkeun) experiment,
// base address of array was 0x4040 and virtual and physical address was the same.
// If you have different base address and physical address mapping,
// four sets used by receiver and transmitter should be updated.
//-------------------------------------------
uint8_t array[NUM_CACHE_LINES * LINE_SIZE];
uint64_t latency[NUM_CACHE_LINES];


void* receiver(void* args) {
    // Receiver is running in Tile-1
    //uint64_t monitor_set[2] = {0x210000,0x410000};  // L1 set0, LLC-2
    //uint64_t eviction_set[2] = {0x208000,0x408000}; // L1 set0, LLC-1
    int monitor_set[2] = {2146240,4243392};  // L1 set0, LLC-2
    int eviction_set[2] = {2113472,4210624}; // L1 set0, LLC-1
    unsigned int junk;
    unsigned long time, threshold;
    threshold = 100;

    uint8_t secret[8] = {0,};

    // preparation: access to monitor_set
    //junk ^= *((uint8_t*)monitor_set[0]);
    //junk ^= *((uint8_t*)monitor_set[1]);
    junk ^= array[monitor_set[0]];
    junk ^= array[monitor_set[1]];
    _mm_mfence();
    for (int i = 0; i < 4; i++) {
        pthread_barrier_wait (&barrier);
        //printf("receiver: %d\n", i);

        int cnt = 0;
        for (int j = 0; j < 5; j++) {
            junk ^= array[eviction_set[0]];
            junk ^= array[eviction_set[1]];
            //junk ^= *((uint8_t*)eviction_set[0]);
            //junk ^= *((uint8_t*)eviction_set[1]);
            _mm_mfence();

            time = __rdtscp(&junk);
            junk ^= array[monitor_set[0]];
            junk ^= array[monitor_set[1]];
            //junk ^= *((uint8_t*)monitor_set[0]);
            //junk ^= *((uint8_t*)monitor_set[1]);
            time = __rdtscp(&junk) - time;
            printf("time: %d\n", time);
            if (time > threshold) cnt++;
            _mm_mfence();
        }

        if (cnt >= 5) secret[i] = 1;
        pthread_barrier_wait (&barrier);
    }

    printf("received data: ");
    for (int i = 3; i >= 0; i--) {
        printf("%d", secret[i]);
    }
    printf("\n");
    pthread_barrier_wait (&barrier);
    return NULL;
}

int main()
{
    pthread_t thread_id[NTHREADS];
    unsigned int junk;
    pthread_barrier_init (&barrier, NULL, 2);
    pthread_create(&thread_id[0], NULL, receiver, (void *)NULL);

    printf("base addr: %#x\n", array);
    // Transmitter is running in Tile-0
    //uint64_t target_ev[2] = {0x218000,0x418000}; // L1 Set0, LLC-3
    //uint64_t local_ev[2] = {0x200000,0x400000};  // L1 Set0, LLC-0
    int target_ev[2] = {2179008,4276160}; // L1 Set0, LLC-3
    int local_ev[2] = {2080704,4177856};  // L1 Set0, LLC-0

    for (int i = 0; i < 4; i++) {
        pthread_barrier_wait (&barrier);
        //printf("trasmitter: %d\n", i);
        if ((i&1) == 1) {
            printf("sending 1 (contention) ...\n");
            for (int j = 0; j < 1000; j++) {
                junk ^= array[target_ev[0]];
                junk ^= array[target_ev[1]];
                junk ^= array[local_ev[0]];
                junk ^= array[local_ev[1]];
                //junk ^= *((uint8_t*)target_ev[0]);
                //junk ^= *((uint8_t*)target_ev[1]);
                //junk ^= *((uint8_t*)local_ev[0]);
                //junk ^= *((uint8_t*)local_ev[1]);
            }
        } else {
            printf("sending 0 (no contention) ...\n");
        }
        pthread_barrier_wait (&barrier);

    }
    pthread_barrier_wait (&barrier);
       
    return 0;
}
