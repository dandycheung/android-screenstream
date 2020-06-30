#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

#include "decode.h"

struct jpeg_decompress_struct cinfo;
static struct my_error_mgr jerr;
static BGR_t *px;
static JSAMPARRAY row_buffer;
void *xdata;

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
    printf("jpeg ERROR");
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

//image_size_t image_size;

int decode_jpeg_init(const unsigned char *buff, size_t *size)
{
    int max_size;
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
    jpeg_mem_src(&cinfo, buff, *size);
    /* Step 3: read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, TRUE);
    // image_size.height = cinfo.image_height;
    // image_size.width = cinfo.image_width;
    // image_size.depth = cinfo.num_components;
    max_size = (cinfo.image_height > cinfo.image_width ? cinfo.image_height : cinfo.image_width);

    // Allocate to fit all possible rotation
    row_buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, max_size * cinfo.output_components, 1);
    // bitmap bgr0 data
    xdata = malloc(max_size * max_size * cinfo.output_components);

    return 1;
}

int decode_jpeg_run(const unsigned char *buff, size_t *size)
{
    int offset_w;
    /* Step 2: specify data source (eg, a file) */
    jpeg_mem_src(&cinfo, buff, *size);
    /* Step 3: read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, TRUE);

    printf("JPEG: %ux%ux%d\n", cinfo.image_height, cinfo.image_width, cinfo.output_components);
    /* Step 5: Start decompressor */
    if (!jpeg_start_decompress(&cinfo))
    {
        printf("JPEG decompress failed\n");
        jpeg_destroy_decompress(&cinfo);
        return 0;
    }

    /* Step 6: while (scan lines remain to be read) */
    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, row_buffer, 1);
        // 4 is padding. 24 bit / 32 bit image using 4 bytes in X display
        px = xdata + (cinfo.output_scanline * cinfo.output_width * 4);
        for (offset_w = 0; offset_w < cinfo.output_width; offset_w++)
        {
            px->red = row_buffer[0][offset_w * cinfo.output_components];
            px->green = row_buffer[0][offset_w * cinfo.output_components + 1];
            px->blue = row_buffer[0][offset_w * cinfo.output_components + 2];
            px++;
        }
    }
    /* Step 7: Finish decompression */
    jpeg_finish_decompress(&cinfo);

    return 1;
}

int decode_jpeg_clear()
{
    // /* Step 8: Release JPEG decompression object */
    jpeg_destroy_decompress(&cinfo);
    return 1;
}