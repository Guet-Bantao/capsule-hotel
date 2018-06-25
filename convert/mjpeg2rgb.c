
/* MJPEG : ʵ����ÿһ֡���ݶ���һ��������JPEG�ļ� */


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

/*�Զ����libjpeg���������*/
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
* ��������� width		 	- ���,�����ٸ�����
*			 src_bpp	 - �Ѿ���JPG�ļ�ȡ����һ��������������,һ�������ö���λ����ʾ
*			 dst_bpp	 - ��ʾ�豸��һ�������ö���λ����ʾ
*			 src_datas	 - �Ѿ���JPG�ļ�ȡ����һ�����������洢��λ��
*			 put_datas	 - ת���������ݴ洢��λ��
* ���Ѿ���JPG�ļ�ȡ����һ����������,ת��Ϊ������ʾ�豸��ʹ�õĸ�ʽ */
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
		/* ������������е�����, ��ʾJPEG������� */
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
	/* ��������Ϊ�ڴ��е����� */
	jpeg_mem_src_tj (&info, video_buf_in->video_pixel_datas.pixel_datas, video_buf_in->video_pixel_datas.total_bytes);

	ret = jpeg_read_header(&info, TRUE);

	// ���ý�ѹ����,����Ŵ���С
	info.scale_num = info.scale_denom = 1;

	// ������ѹ��jpeg_start_decompress	
	jpeg_start_decompress(&info);

	// һ�е����ݳ���
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

	//ѭ������jpeg_read_scanlines��һ��һ�еػ�ý�ѹ������
	while (info.output_scanline < info.output_height) 
	{
		/* �õ�һ������,�������ɫ��ʽΪ0xRR, 0xGG, 0xBB */
		(void) jpeg_read_scanlines(&info, &line_buffer, 1);

		// ת��pixeldatasȥ
		covert_one_line(pixeldatas->width, 24, pixeldatas->bpp, line_buffer, put_datas);
		put_datas += pixeldatas->line_bytes;
	}

	free(line_buffer);
	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	if(pixeldatas->pixel_datas[pixeldatas->line_bytes*240-1]==132)//���һ������һ���ֽ���132�������ļ�������
		return -1;//����֣�Premature end of JPEG file�����������������⿨�ٺ���

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



