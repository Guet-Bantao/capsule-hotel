
/* MJPEG : 实质上每一帧数据都是一个完整的JPEG文件 */


#include <convert_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>




typedef struct my_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
}my_jpeg_mgr;

extern void jpeg_mem_src_tj(j_decompress_ptr, unsigned char *, unsigned long);


static int support_mjpeg2rgb(int pixel_format_in, int pixel_format_out)
{
    if (pixel_format_in != V4L2_PIX_FMT_MJPEG)
        return 0;
    if ((pixel_format_out != V4L2_PIX_FMT_RGB565) && (pixel_format_out != V4L2_PIX_FMT_RGB32))
    {
        return 0;
    }
    return 1;
}

/*自定义的libjpeg库出错处理函数*/
static void MyErrorExit(j_common_ptr info)
{
	static char err_str[JMSG_LENGTH_MAX];
	my_jpeg_mgr* error = (my_jpeg_mgr*)info->err;

	/* Create the message */
    (*info->err->format_message) (info, err_str);
	//printf("%s\n", err_str);

	longjmp(error->setjmp_buffer, 1);
}
/*
* 输入参数： width		 	- 宽度,即多少个象素
*			 src_bpp	 - 已经从JPG文件取出的一行象素数据里面,一个象素用多少位来表示
*			 dst_bpp	 - 显示设备上一个象素用多少位来表示
*			 src_datas	 - 已经从JPG文件取出的一行象素数所存储的位置
*			 put_datas	 - 转换所得数据存储的位置
* 把已经从JPG文件取出的一行象素数据,转换为能在显示设备上使用的格式 */
static int covert_one_line(int width, int src_bpp, int dst_bpp, unsigned char *src_datas, unsigned char *put_datas)
{
	int i;
	int pos = 0;
	unsigned int red, green, blue, color;	

	unsigned short *dst_datas_16bpp = (unsigned short *)put_datas;
	unsigned int   *dst_datas_32bpp = (unsigned int *)put_datas;

	if (src_bpp != 24)
	{
		return -1;
	}
	if (dst_bpp == 24)
	{
		memcpy(put_datas, src_datas, width*3);
	}
	else
	{
		for (i = 0; i < width; i++)
		{
			red   = src_datas[pos++];
			green = src_datas[pos++];
			blue  = src_datas[pos++];
			if (dst_bpp == 32)
			{
				color = (red<< 16) | (green << 8) | blue;
				*dst_datas_32bpp = color;
				dst_datas_32bpp++;
			}
			else if (dst_bpp == 16)
			{
				/* 565 */
				red   = red >> 3;
				green = green >> 2;
				blue  = blue >> 3;
				color = (red << 11) | (green << 5) | (blue);
				*dst_datas_16bpp = color;
				dst_datas_16bpp++;
			}
		}
	}
	return 0;
}


static int mjpeg2rgb(video_buf* video_buf_in, video_buf* video_buf_out)
{
	struct jpeg_decompress_struct info;
	my_jpeg_mgr jerr;
	int ret;
	int row_stride;
	unsigned char *line_buffer = NULL;
	unsigned char *put_datas;
	pixel_datas* pixeldatas = &video_buf_out->video_pixel_datas;

	info.err			= jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit	= MyErrorExit;

	if(setjmp(jerr.setjmp_buffer))
	{
		/* 如果程序能运行到这里, 表示JPEG解码出错 */
        jpeg_destroy_decompress(&info);
        if (line_buffer)
        {
            free(line_buffer);
        }
		return -1;//add
        if (pixeldatas->pixel_datas)
        {
            free(pixeldatas->pixel_datas);
        }
		return -1;
	}

	jpeg_create_decompress(&info);
	/* 把数据设为内存中的数据 */
	jpeg_mem_src_tj (&info, video_buf_in->video_pixel_datas.pixel_datas, video_buf_in->video_pixel_datas.total_bytes);

	ret = jpeg_read_header(&info, TRUE);

	// 设置解压参数,比如放大、缩小
	info.scale_num = info.scale_denom = 1;

	// 启动解压：jpeg_start_decompress	
	jpeg_start_decompress(&info);

	// 一行的数据长度
	row_stride = info.output_width * info.output_components;
	line_buffer = malloc(row_stride);
	if (NULL == line_buffer)
    {
        return -1;
    }

	pixeldatas->width  		= info.output_width;
	pixeldatas->height 		= info.output_height;
	pixeldatas->line_bytes	= pixeldatas->width * pixeldatas->bpp / 8;
    pixeldatas->total_bytes	= pixeldatas->height * pixeldatas->line_bytes;
	if (NULL == pixeldatas->pixel_datas)
	{
	    pixeldatas->pixel_datas = malloc(pixeldatas->total_bytes);
	}
	put_datas = pixeldatas->pixel_datas;

	//循环调用jpeg_read_scanlines来一行一行地获得解压的数据
	while (info.output_scanline < info.output_height) 
	{
		/* 得到一行数据,里面的颜色格式为0xRR, 0xGG, 0xBB */
		(void) jpeg_read_scanlines(&info, &line_buffer, 1);

		// 转到pixeldatas去
		covert_one_line(pixeldatas->width, 24, pixeldatas->bpp, line_buffer, put_datas);
		put_datas += pixeldatas->line_bytes;
	}

	free(line_buffer);
	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	if(pixeldatas->pixel_datas[pixeldatas->line_bytes*240-1]==132)//如果一桢的最后一个字节是132，代表文件不完整
		return -1;//会出现：Premature end of JPEG file，所以抛弃掉，以免卡顿黑屏

    return 0;
    
}

static int mjpeg2rgb_exit(video_buf* video_buf_out)
{
    if (video_buf_out->video_pixel_datas.pixel_datas)
    {
        free(video_buf_out->video_pixel_datas.pixel_datas);
        video_buf_out->video_pixel_datas.pixel_datas = NULL;
    }
    return 0;
}

static convert_opr mjpeg_device_opr = {
    .name		= "mjpeg2rgb",
	.is_support	= support_mjpeg2rgb,
	.convert	= mjpeg2rgb,
	.exit		= mjpeg2rgb_exit,
};


int mjpeg_register(void)
{
	return register_convert_opr(&mjpeg_device_opr);
}



