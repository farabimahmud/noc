#include <stdint.h>
#include <x86intrin.h>

#include <cstdio>
#include <cstring>

#define LLC_SIZE (2 << 20)

#ifdef GEM5_SE
#include <gem5/m5ops.h>
#endif

// 8 x 8
// prog. is runing core 0
// A is mapped to l2 bank 0
// B is mapped to l2 back 63
//if (secret == 1) {
//   LD A
//} else {
//   LD B
//}
size_t size = 4;
uint8_t arr1[256 * 64];
uint8_t arr2[512 * 64];  // idx 0 * 64 -> node 2
// idx 1 * 64 -> node 3
// idx 2 * 64 -> node 4
// idx 61 * 64-> node 63
// idx 62 * 64-> node 0
// ...
// idx 125 * 64-> node 63
// idx 126 * 64-> node 0
uint8_t secret = 0b00001111;// 0b1111011

void static inline victim(int idx)
{
    uint8_t s = secret & idx;
    if (s == 0) {
        s ^= arr2[117 * 64];
    } else {
        s ^= arr2[118 * 64];
    }
}

int main()
{
    unsigned long t[512];
    uint8_t x;
    unsigned int junk;
    //printf("first loop\n");

    int mask = 1;
    unsigned long t1, t2;    
    int iteration = 1;
    while( iteration -- ){
        mask = 1;
        for (int i = 0; i< 8; i++) {

            for (int j = 0; j < 512; j++) {
                junk ^= arr2[j * 64];
            }
            _mm_mfence();

            junk ^= arr2[0 * 64];

            t1 = __rdtscp(&junk);
            victim(mask);
            t2 = __rdtscp(&junk);
            _mm_mfence();

            printf("%lu\t",t2-t1);
            mask <<= 1;

        }
        printf("\n");
    }

}

