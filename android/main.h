#pragma once
#include <stdio.h>


#define DIE(str, args...)             \
    do                                \
    {                                 \
        fprintf(stderr, "%s\n", str); \
        exit(1);           \
    } while (0)

extern int fd;