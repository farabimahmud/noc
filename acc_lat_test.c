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
#define COUNT 512
uint8_t arr1[256 * 64];
uint8_t arr2[ COUNT * 64];  // idx 0 * 64 -> node 2
                         // idx 1 * 64 -> node 3
                         // idx 2 * 64 -> node 4
                         // idx 61 * 64-> node 63
                         // idx 62 * 64-> node 0
                         // ...
                         // idx 125 * 64-> node 63
                         // idx 126 * 64-> node 0
uint8_t secret = 123;

int main()
{

    unsigned long t[COUNT];
    uint8_t x;
    unsigned int junk;

    printf("first loop\n");
    for (int i = 0; i < COUNT; i++) {
        printf("&arr2[%d*64] = %#x\n", i, &(arr2[i * 64]));
        junk ^= arr2[i * 64];
    }

    _mm_mfence();
    printf("begin second loop\n");

    // 125, 126
    for (int i = 0; i < COUNT; i++) {
        unsigned long time1 = __rdtscp(&junk);
        x ^= arr2[i * 64];
        unsigned long time2 = __rdtscp(&junk);
        t[i] = time2 - time1;
    }
    printf("end second loop\n");

    for (int i = 0; i < COUNT; i++) {
        printf("%d: %d, %s\n", i, t[i], (t[i] < 45)? "": "");
    }
}

