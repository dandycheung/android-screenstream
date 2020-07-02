#pragma once

#define INPUT_BINARY_SIZE 6

#define DIE(str, args...)             \
    do                                \
    {                                 \
        fprintf(stderr, "%s\n", str); \
        exit(1);                      \
    } while (0)

#ifndef __SOCK_SIZE__
// sizeof(struct sockaddr_in)
#define __SOCK_SIZE__ 16U
#endif