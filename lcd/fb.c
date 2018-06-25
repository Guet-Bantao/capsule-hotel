#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <linux/fb.h>


#include <lcd_manager.h>

static int fd;

static struct fb_fix_screeninfo fb_fix;	





static int fb_init(char * name, lcd_device_opr* lcd_device)
{
	int ret;
	struct fb_var_screeninfo fb_var;
	unsigned int screen_size;
	unsigned char *puc_fb_mem;
	
	fd = open(name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		printf("can't open %s\n", name);
		return -1;
	}

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret < 0)
	{
		printf("can't get fb's var\n");
		goto err_exit;
	}
	ret = ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret < 0)
	{
		printf("can't get fb's fix\n");
		goto err_exit;
	}
	
	screen_size = fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;
	puc_fb_mem = (unsigned char *)mmap(NULL , screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (0 > puc_fb_mem)	
	{
		printf("can't mmap\n");
		goto err_exit;
	}

	lcd_device->xres      	= fb_var.xres;
	lcd_device->yres		= fb_var.yres;
	lcd_device->bpp      	= fb_var.bits_per_pixel;
	lcd_device->line_width	= fb_var.xres * lcd_device->bpp / 8;
	lcd_device->puc_mem 	= puc_fb_mem;
	lcd_device->screen_size= screen_size;

	lcd_device->dw_line_width  = fb_var.xres * fb_var.bits_per_pixel / 8;
	lcd_device->dw_pixel_width = fb_var.bits_per_pixel / 8;

	return 0;

err_exit:	 
	close(fd);
	return -1;

}

static int fb_clean_screen(lcd_device_opr* lcd_device, unsigned int bcack_color)
{
	int i=0;
	int red, green, blue;
	unsigned short color16bpp; /* 565 */
	unsigned short *fb16bpp;
	unsigned int *fb32bpp;
	unsigned char *pucfb;

	pucfb      = lcd_device->puc_mem;
	fb16bpp  = (unsigned short *)pucfb;
	fb32bpp = (unsigned int *)pucfb;
	
	switch (lcd_device->bpp)
	{
		case 8:
		{
			memset(lcd_device->puc_mem, bcack_color, lcd_device->screen_size);
			break;
		}
		case 16:
		{
			/* 从dwBackColor中取出红绿蓝三原色,
			 * 构造为16Bpp的颜色
			 */
			red   = (bcack_color >> (16+3)) & 0x1f;
			green = (bcack_color >> (8+2)) & 0x3f;
			blue  = (bcack_color >> 3) & 0x1f;
			color16bpp = (red << 11) | (green << 5) | blue;
			while (i < lcd_device->screen_size)
			{
				*fb16bpp	= color16bpp;
				fb16bpp++;
				i += 2;
			}
			break;
		}
		case 32:
		{
			while (i < lcd_device->screen_size)
			{
				*fb32bpp	= bcack_color;
				fb32bpp++;
				i += 4;
			}
			break;
		}
		default :
		{
			printf("can't support %d bpp\n", lcd_device->bpp);
			return -1;
		}
	}
	return 0;
}

//颜色数据在FrameBuffer上显示出来
static int fb_show_page(pixel_datas* lcd_pixel_datas, lcd_device_opr* lcd_device)
{
    if (lcd_device->puc_mem != lcd_pixel_datas->pixel_datas)
    {
    	memcpy(lcd_device->puc_mem, lcd_pixel_datas->pixel_datas, lcd_pixel_datas->total_bytes);
    }
	return 0;
}


static lcd_opr fb_device_opr = {
    .name		= "fb",
	.init		= fb_init,
	.clean		= fb_clean_screen,
	.show_page	= fb_show_page,
};


int lcd_register(void)
{
	return register_lcd_opr(&fb_device_opr);
}




