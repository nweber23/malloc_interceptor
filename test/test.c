#include <stdio.h>
#include <stdlib.h>

int main(void) {
    for (int i = 1; i <= 50; ++i) {
        void *p = malloc(10);
        if (!p) {
            printf("malloc #%d failed!\n", i);
            return 1;
        }
        printf("malloc #%d succeeded\n", i);
        free(p);
    }
    return 0;
}
