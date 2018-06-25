#include <convert_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "color.h"



typedef struct {
  struct jpeg_destination_mgr pub;
  JOCTET * buffer; 
  unsigned char *outbuffer;
  int outbuffer_size;
  unsigned char *outbuffer_cursor;
  int *written; 
}mjpg_destination_mgr;
typedef mjpg_destination_mgr *mjpg_dest_ptr;

#define OUTPUT_BUF_SIZE  4096*2

METHODDEF(void) init_destination(j_compress_ptr cinfo) {
  mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
  dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
  *(dest->written) = 0;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}
METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo) {
  mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
  memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
  dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
  *(dest->written) += OUTPUT_BUF_SIZE;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
  return TRUE;
}
METHODDEF(void) term_destination(j_compress_ptr cinfo) {
  mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
 
  memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
  dest->outbuffer_cursor += datacount;
  *(dest->written) += datacount;
}

void dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written) {
  mjpg_dest_ptr dest;
  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(mjpg_destination_mgr));
  }
  dest = (mjpg_dest_ptr)cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outbuffer = buffer;
  dest->outbuffer_size = size;
  dest->outbuffer_cursor = buffer;
  dest->written = written;
}

static int yuyv2jpg_new(video_buf* video_buf_in, video_buf* video_buf_out)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];

	pixel_datas* pixeldatas_in  = &video_buf_in->video_pixel_datas;
	pixel_datas* pixeldatas_out = &video_buf_out->video_pixel_datas;

	unsigned char *line_buffer, *yuyv;
	int z;
	static int written;
	int size = pixeldatas_in->width * pixeldatas_in->height;

	int WIDTH = pixeldatas_in->width;
	int HEIGHT = pixeldatas_in->height;

	line_buffer = calloc (WIDTH * 3, 1);
	yuyv = pixeldatas_in->pixel_datas;//将YUYV格式的图片数据赋给YUYV指针
	
	pixeldatas_out->bpp = 16;
	pixeldatas_out->line_bytes  = pixeldatas_in->width * pixeldatas_out->bpp / 8;
	pixeldatas_out->total_bytes = pixeldatas_in->line_bytes * pixeldatas_in->height;
	if (!pixeldatas_out->pixel_datas)
	{
		pixeldatas_out->pixel_datas = malloc(pixeldatas_out->total_bytes);
	}
	printf("compress start...\n");
	
	cinfo.err = jpeg_std_error (&jerr);
	jpeg_create_compress (&cinfo);
	/* jpeg_stdio_dest (&cinfo, file); */
	dest_buffer(&cinfo, pixeldatas_out->pixel_datas, size, &written);
	
	cinfo.image_width = pixeldatas_in->width;
	cinfo.image_height = pixeldatas_in->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, 80, TRUE);
	jpeg_start_compress (&cinfo, TRUE);

	z = 0;
	while (cinfo.next_scanline < HEIGHT) {
    int x;
    unsigned char *ptr = line_buffer;

    for (x = 0; x < WIDTH; x++) {
      int r, g, b;
      int y, u, v;

      if (!z)
        y = yuyv[0] << 8;
      else
        y = yuyv[2] << 8;
	  
      u = yuyv[1] - 128;
      v = yuyv[3] - 128;

      r = (y + (359 * v)) >> 8;
      g = (y - (88 * u) - (183 * v)) >> 8;
      b = (y + (454 * u)) >> 8;

      *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
      *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
      *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);
 
      if (z++) {
        z = 0;
        yuyv += 4;
      }
    }

    row_pointer[0] = line_buffer;

    jpeg_write_scanlines (&cinfo, row_pointer, 1);

  }
  	free (line_buffer);

	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

  return (written);
}

static int support_yuyv2jpg(int pixel_format_in, int pixel_format_out)
{
    if (pixel_format_in != V4L2_PIX_FMT_YUYV)
        return 0;
    if (pixel_format_out != V4L2_PIX_FMT_JPEG)
    {
        return 0;
    }
    return 1;
}

static int yuyv2jpg_exit(video_buf* video_buf_out)
{
    if (video_buf_out->video_pixel_datas.pixel_datas)
    {
        free(video_buf_out->video_pixel_datas.pixel_datas);
        video_buf_out->video_pixel_datas.pixel_datas = NULL;
    }
    return 0;
}

static convert_opr yuyv_new_device_opr = {
    .name		= "yuyv2jpg",
	.is_support	= support_yuyv2jpg,
	.convert	= yuyv2jpg_new,
	.exit		= yuyv2jpg_exit,
};

extern void initLut(void);

int yuyv_new_register(void)
{
	initLut();
	return register_convert_opr(&yuyv_new_device_opr);
}

