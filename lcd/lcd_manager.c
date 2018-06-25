#include <lcd_manager.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list_core.h>
#include <string.h>

#include <camera_manager.h>


static struct list_head * head;

static lcd_device_opr* default_lcd_device;


/*	头结点的存在使得空链表与非空链表处理一致，
*	也方便对链表的开始结点的插入或删除操作
*/
static int Create_list(void)
{
	//list_info * head = (list_info *)malloc( sizeof(list_info) );  
	 head = (struct list_head *)malloc( sizeof(struct list_head) );
	if(head == NULL)  
		return -1;
	else  
	//	INIT_LIST_HEAD(&head->list_node);
		INIT_LIST_HEAD(head);
	return 0;
}

int register_lcd_opr(lcd_opr* opr)
{
	list_add(&opr->node,head);

	return 0;
}

int get_lcd_buf_display(video_buf* frame_buf)
{
    frame_buf->pixel_format = (default_lcd_device->bpp == 16) ? V4L2_PIX_FMT_RGB565 : \
                                   (default_lcd_device->bpp == 32) ?  V4L2_PIX_FMT_RGB32 : \
                                           0;
    frame_buf->video_pixel_datas.width	= default_lcd_device->xres;
    frame_buf->video_pixel_datas.height= default_lcd_device->yres;
    frame_buf->video_pixel_datas.bpp   	 = default_lcd_device->bpp;
    frame_buf->video_pixel_datas.line_bytes    = default_lcd_device->line_width;
    frame_buf->video_pixel_datas.total_bytes   = frame_buf->video_pixel_datas.line_bytes * frame_buf->video_pixel_datas.height;
    frame_buf->video_pixel_datas.pixel_datas = default_lcd_device->puc_mem;
    return 0;
}

/*get lcd's format*/
int get_lcd_resolution(int *xres, int *yres, int *bpp)
{
	if (default_lcd_device)
	{
		*xres = default_lcd_device->xres;
		*yres = default_lcd_device->yres;
		*bpp  = default_lcd_device->bpp;
		return 0;
	}
	else
	{
		return -1;
	}
}

/*
*success return 0; faild return -1
*/
int lcd_device_init(char * name, lcd_device_opr * lcd)
{
	int error;
	lcd_opr * pos;
	
	list_for_each_entry(pos, head, node)
	{
		if((pos->init)!=NULL)//if have init
		{
			error=pos->init(name, lcd);
			if(!error)
			{
				lcd->opr=pos;//success find operation
				lcd->opr->clean(lcd, 0);
				default_lcd_device = lcd;
				return 0;
			}
		}
	}
	return -1;
}

int lcd_init()
{
	Create_list();
	lcd_register();

	return 0;
}





