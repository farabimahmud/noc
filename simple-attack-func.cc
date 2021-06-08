#include <stdint.h>
#include <x86intrin.h>

#include <cstdio>
#include <cstring>

#define LLC_SIZE (2 << 20)
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
uint8_t secret = 123;

void victim(int idx)
{
    uint8_t s = secret & idx;
    if (s == 0) {
        s ^= arr2[125 * 64];
    } else {
        s ^= arr2[126 * 64];
    }
}

int main()
{
    unsigned long t[512];
    uint8_t x;
    unsigned int junk;
    //printf("first loop\n");
    for (int i = 0; i < 512; i++) {
        //printf("&arr2[%d*64] = %#x\n", i, &(arr2[i * 64]));
        junk ^= arr2[i * 64];
    }

    _mm_mfence();

    int mask = 1;
    unsigned long t1, t2;
    for (int i = 0; i < 8; i++) {
        t1 = __rdtscp(&junk);
        victim(mask);
        t2 = __rdtscp(&junk) - t1;
        if (t2 > 100) {
            printf("lat: %d --> BIT[%d]: %d\n", t2, i, 0);
        } else {
            printf("lat: %d --> BIT[%d]: %d\n", t2, i, 1);
        }

        for (int i = 0; i < 512; i++) {
            junk ^= arr2[i * 64];
        }

        mask <<= 1;
    }
}

