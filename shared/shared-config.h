#pragma once

#define INPUT_BINARY_SIZE 6
#define INPUT_BINARY_MASK_KEYBOARD 0b10000000
#define INPUT_BINARY_SET_KEYBOARD(b) (b | INPUT_BINARY_MASK_KEYBOARD)
#define INPUT_BINARY_CHECK_KEYBOARD(b) (b & INPUT_BINARY_MASK_KEYBOARD)
#define INPUT_BINARY_NO_MASK(b) (b & ~INPUT_BINARY_MASK_KEYBOARD)

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