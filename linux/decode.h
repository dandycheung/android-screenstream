#pragma once
#include <unistd.h>
#include <setjmp.h>
#include <jpeglib.h>

struct my_error_mgr
{
    struct jpeg_error_mgr pub; /* "public" fields */
    jmp_buf setjmp_buffer;     /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

extern char *xdata;
extern struct jpeg_decompress_struct cinfo;
int decode_jpeg_init(const unsigned char *buff, ssize_t size);
int decode_jpeg_run(const unsigned char *buff, ssize_t size);