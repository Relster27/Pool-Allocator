#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>

#include "pool_allocator.h"

int main(void) {

    int *ptr = p_alloc(0x1000);
    if (ptr == NULL) {
        perror("ERRNO ");
        return 1;
    }

    p_free(ptr);
    p_destroy();
    return 0;
}