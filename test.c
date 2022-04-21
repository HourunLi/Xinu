#include <stdio.h>
#include <malloc.h>
int main() {
    int argc = 4;
    int argv[4];
    int *ptr = argv;
    for(;argc >0; argc--) {
        *ptr++ = 1;
    }
    return 0;
}


