#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    char buf[64];
    for (int i = 1; i <= 50; ++i) {
        void *p = malloc(10);
        int len = snprintf(buf, sizeof(buf), "malloc #%d %s\n", i, p ? "succeeded" : "failed!");
        write(1, buf, len);
        if (p) free(p);
    }
    return 0;
}
