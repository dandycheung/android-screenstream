#pragma once
#include <unistd.h>
#include <setjmp.h>
#include <jpeglib.h>

typedef struct BGR
{
    unsigned char blue, green, red, alpha;
} BGR_t;

struct my_error_mgr
{
    struct jpeg_error_mgr pub; /* "public" fields */
    jmp_buf setjmp_buffer;     /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

// typedef struct image_size_s {
//     uint width;
//     uint height;
//     uint8_t depth;
// } image_size_t;

// image_size_t image_size;

extern void *xdata;
extern struct jpeg_decompress_struct cinfo;
int decode_jpeg_init(const unsigned char *buff, size_t *size);
int decode_jpeg_run(const unsigned char *buff, size_t *size);