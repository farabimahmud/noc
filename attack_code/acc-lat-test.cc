#include <stdint.h>
#include <x86intrin.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>


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

int main()
{

    unsigned long t[512];
    uint8_t x;
    unsigned int junk;
    string filename = "latency.log";
    ofstream output_file;
    output_file.open(filename);

    output_file << "first loop\n";
    for (int i = 0; i < 512; i++) {
        fprintf("&arr2[%d*64] = %hhn\n", i, &(arr2[i * 64]));
        junk ^= arr2[i * 64];
    }

    _mm_mfence();
    fprintf("begin second loop\n");

    // 125, 126
    for (int i = 0; i < 512; i++) {
        unsigned long time1 = __rdtscp(&junk);
        x ^= arr2[i * 64];
        unsigned long time2 = __rdtscp(&junk);
        t[i] = time2 - time1;
    }
    fprintf("end second loop\n");

    for (int i = 0; i < 512; i++) {
        fprintf("%d: %ld, %s\n", i, t[i], (t[i] < 45)? "": "");
    }
    output_file.close();
}

