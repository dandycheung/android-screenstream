#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

#include "decode.h"

struct jpeg_decompress_struct cinfo;
static struct my_error_mgr jerr;
char *xdata;
JSAMPROW *prow[2];

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
    printf("jpeg ERROR");
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

int decode_jpeg_decompress()
{
    int offset_w;
    // printf("JPEG: %ux%ux%d\n", cinfo.image_height, cinfo.image_width, cinfo.num_components);
    /* Step 5: Start decompressor */
    cinfo.out_color_space = JCS_EXT_BGRX;
    if (!jpeg_start_decompress(&cinfo))
    {
        printf("JPEG decompress failed\n");
        jpeg_finish_decompress(&cinfo);
        return 0;
    }
    printf("jpeg decompress %p %d\n", xdata, cinfo.output_scanline);
    /* Step 6: while (scan lines remain to be read) */
    while (cinfo.output_scanline < cinfo.output_height)
    {
        prow[0] = xdata + (cinfo.output_scanline * cinfo.output_width * 4);
        prow[1] = xdata + ((cinfo.output_scanline+1) * cinfo.output_width * 4);
        jpeg_read_scanlines(&cinfo, prow, 2);
    }
    /* Step 7: Finish decompression */
    jpeg_finish_decompress(&cinfo);
    return 1;
}

int decode_jpeg_init(const unsigned char *buff, ssize_t size)
{
    JDIMENSION max_size;
    /* Step 1: allocate and initialize JPEG decompression object */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, buff, size);
    /* Step 3: read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, TRUE);
    max_size = (cinfo.image_height > cinfo.image_width ? cinfo.image_height : cinfo.image_width);
    printf("JPEG: init %d %d %d == %d\n", cinfo.image_height, cinfo.image_width, cinfo.num_components, max_size);

    // bitmap bgr0 data
    xdata = malloc(max_size * max_size * 4);

    return decode_jpeg_decompress();
}

int decode_jpeg_run(const unsigned char *buff, ssize_t size)
{

    /* Step 2: specify data source (eg, a file) */
    jpeg_mem_src(&cinfo, buff, size);
    /* Step 3: read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, TRUE);
    return decode_jpeg_decompress();
}

int decode_jpeg_clear()
{
    // /* Step 8: Release JPEG decompression object */
    jpeg_destroy_decompress(&cinfo);
    return 1;
}