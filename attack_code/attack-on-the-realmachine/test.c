#include <stdio.h>
#include <stdlib.h>
int main()
{
    int array[10];

    for (int i = 0; i < 10; i++) {
        array[i] = i;
    }
    for (int i = 0; i < 10; i++) {
        int temp = array[i];
        int randomIndex = rand() % 10;
        array[i] = array[randomIndex];
        array[randomIndex] = temp;
    }

    for (int i = 0; i < 10; i++) {
        printf("arr[%d] = %d\n", i, array[i]);
    }
    return 0;
}
