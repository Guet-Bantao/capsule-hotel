#include <convert_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "color.h"

/*static int convert_yuv_to_rgb_pixel(int y, int u, int v)//花屏
{	
	unsigned int pixel32 = 0;
	unsigned char *pixel = (unsigned char *)&pixel32;
	int r, g, b;
	r = y + (1.370705 * (v-128));
	g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
	b = y + (1.732446 * (u-128));
	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;
	if(r < 0) r = 0;
	if(g < 0) g = 0;
	if(b < 0) b = 0;
	pixel[0] = r * 220 / 256;
	pixel[1] = g * 220 / 256;
	pixel[2] = b * 220 / 256;
	
	return pixel32;
}*/
/*yuv格式转换为rgb格式*/

/* translate YUV422Packed to rgb24 */

static unsigned int
Pyuv422torgb565(unsigned char * input_ptr, unsigned char * output_ptr, unsigned int image_width, unsigned int image_height)
{
	unsigned int i, size;
	unsigned char Y, Y1, U, V;
	unsigned char *buff = input_ptr;
	unsigned char *output_pt = output_ptr;

    unsigned int r, g, b;
    unsigned int color;
    
	size = image_width * image_height /2;
	for (i = size; i > 0; i--) {
		/* bgr instead rgb ?? */
		Y = buff[0] ;
		U = buff[1] ;
		Y1 = buff[2];
		V = buff[3];
		buff += 4;
		r = R_FROMYV(Y,V);
		g = G_FROMYUV(Y,U,V); //b
		b = B_FROMYU(Y,U); //v

        /* 把r,g,b三色构造为rgb565的16位值 */
        r = r >> 3;
        g = g >> 2;
        b = b >> 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;
			
		r = R_FROMYV(Y1,V);
		g = G_FROMYUV(Y1,U,V); //b
		b = B_FROMYU(Y1,U); //v
		
        /* 把r,g,b三色构造为rgb565的16位值 */
        r = r >> 3;
        g = g >> 2;
        b = b >> 3;
        color = (r << 11) | (g << 5) | b;
        *output_pt++ = color & 0xff;
        *output_pt++ = (color >> 8) & 0xff;
	}

	return 0;
} 

/* translate YUV422Packed to rgb24 */

static unsigned int
Pyuv422torgb32(unsigned char * input_ptr, unsigned char * output_ptr, unsigned int image_width, unsigned int image_height)
{
	unsigned int i, size;
	unsigned char Y, Y1, U, V;
	unsigned char *buff = input_ptr;
	unsigned int *output_pt = (unsigned int *)output_ptr;

    unsigned int r, g, b;
    unsigned int color;

	size = image_width * image_height /2;
	for (i = size; i > 0; i--) {
		/* bgr instead rgb ?? */
		Y = buff[0] ;
		U = buff[1] ;
		Y1 = buff[2];
		V = buff[3];
		buff += 4;

        r = R_FROMYV(Y,V);
		g = G_FROMYUV(Y,U,V); //b
		b = B_FROMYU(Y,U); //v
		/* rgb888 */
		color = (r << 16) | (g << 8) | b;
        *output_pt++ = color;
			
		r = R_FROMYV(Y1,V);
		g = G_FROMYUV(Y1,U,V); //b
		b = B_FROMYU(Y1,U); //v
		color = (r << 16) | (g << 8) | b;
        *output_pt++ = color;
	}
	
	return 0;
} 

static int yuyv2rgb(video_buf* video_buf_in, video_buf* video_buf_out)
{
	
	pixel_datas* pixeldatas_in  = &video_buf_in->video_pixel_datas;
	pixel_datas* pixeldatas_out = &video_buf_out->video_pixel_datas;

	pixeldatas_out->width  = pixeldatas_in->width;
	pixeldatas_out->height = pixeldatas_in->height;

	if (video_buf_out->pixel_format == V4L2_PIX_FMT_RGB565)
	{
		pixeldatas_out->bpp = 16;
		pixeldatas_out->line_bytes  = pixeldatas_out->width * pixeldatas_out->bpp / 8;
		pixeldatas_out->total_bytes = pixeldatas_out->line_bytes * pixeldatas_out->height;

		if (!pixeldatas_out->pixel_datas)
		{
			pixeldatas_out->pixel_datas = malloc(pixeldatas_out->total_bytes);
		}
		Pyuv422torgb565(pixeldatas_in->pixel_datas, pixeldatas_out->pixel_datas, pixeldatas_out->width, pixeldatas_out->height);
		return 0;
	}
	else if (video_buf_out->pixel_format == V4L2_PIX_FMT_RGB32)
	{
		pixeldatas_out->bpp = 32;
		pixeldatas_out->line_bytes  = pixeldatas_out->width * pixeldatas_out->bpp / 8;
		pixeldatas_out->total_bytes = pixeldatas_out->line_bytes * pixeldatas_out->height;

		if (!pixeldatas_out->pixel_datas)
		{
			pixeldatas_out->pixel_datas = malloc(pixeldatas_out->total_bytes);
		}
		
		Pyuv422torgb32(pixeldatas_in->pixel_datas, pixeldatas_out->pixel_datas, pixeldatas_out->width, pixeldatas_out->height);
		return 0;
	}
	
	return -1;
/*	unsigned char *yuv = video_buf_in->video_pixel_datas.pixel_datas;//花屏
	int yuyv_width = 320;
	int yuyv_height = 240;
	pixel_datas* pixeldatas = &video_buf_out->video_pixel_datas;
	unsigned char *rgb = NULL;
	
	unsigned int in, out = 0;
	unsigned int pixel_16;
	unsigned char pixel_24[3];
	unsigned int pixel32;
	int y0, u, y1, v;

	pixeldatas->width  		= yuyv_width;
	pixeldatas->height 		= yuyv_height;
	pixeldatas->line_bytes	= yuyv_width * pixeldatas->bpp / 8;
    pixeldatas->total_bytes	= pixeldatas->height * pixeldatas->line_bytes;
	if (NULL == pixeldatas->pixel_datas)
	{
	    pixeldatas->pixel_datas = malloc(pixeldatas->total_bytes);
	}
	rgb = pixeldatas->pixel_datas;

	for(in = 0; in < yuyv_width * yuyv_height; in += 4)//yuyv_width * yuyv_height * 2
	{
		pixel_16 =  yuv[in + 3] << 24 |
					yuv[in + 2] << 16 |
					yuv[in + 1] <<  8 |
					yuv[in + 0];
		y0 = (pixel_16 & 0x000000ff);
		u  = (pixel_16 & 0x0000ff00) >>  8;
		y1 = (pixel_16 & 0x00ff0000) >> 16;
		v  = (pixel_16 & 0xff000000) >> 24;
		pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
		pixel_24[0] = (pixel32 & 0x000000ff);
		pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
		pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];
		pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
		pixel_24[0] = (pixel32 & 0x000000ff);
		pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
		pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];
	}

	return 0;*/
}

static int support_yuyv2rgb(int pixel_format_in, int pixel_format_out)
{
    if (pixel_format_in != V4L2_PIX_FMT_YUYV)
        return 0;
    if ((pixel_format_out != V4L2_PIX_FMT_RGB565) && (pixel_format_out != V4L2_PIX_FMT_RGB32))
    {
        return 0;
    }
    return 1;
}

static int yuyv2rgb_exit(video_buf* video_buf_out)
{
    if (video_buf_out->video_pixel_datas.pixel_datas)
    {
        free(video_buf_out->video_pixel_datas.pixel_datas);
        video_buf_out->video_pixel_datas.pixel_datas = NULL;
    }
    return 0;
}


static convert_opr yuyv_device_opr = {
    .name		= "yuyv2rgb",
	.is_support	= support_yuyv2rgb,
	.convert	= yuyv2rgb,
	//.convert	= yuyv2jpg,
	.exit		= yuyv2rgb_exit,
};

extern void initLut(void);

int yuyv_register(void)
{
	initLut();
	return register_convert_opr(&yuyv_device_opr);
}


