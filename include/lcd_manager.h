#ifndef _LCD_MANAGER_H
#define _LCD_MANAGER_H

#include "list_core.h"


struct lcd_info;

typedef struct lcd_devices {
	int xres;               /* X分辨率 */
	int yres;               /* Y分辨率 */
	int bpp;                /* 一个象素用多少位来表示 */
	int line_width;          /* 一行数据占据多少字节 */
	unsigned char *puc_mem;   /* 显存地址 */
	unsigned int screen_size;
	//struct fb_var_screeninfo fb_var;

	unsigned int dw_line_width;
	unsigned int dw_pixel_width;

	struct lcd_info* opr;
}lcd_device_opr;

/* 图片的象素数据 */
typedef struct pixel_data {
	int width;   /* 宽度: 一行有多少个象素 */
	int height;  /* 高度: 一列有多少个象素 */
	int bpp;     /* 一个象素用多少位来表示 */
	int line_bytes;  /* 一行数据有多少字节 */
	int total_bytes; /* 所有字节数 */ 
	unsigned char *pixel_datas;  /* 象素数据存储的地方 */
}pixel_datas;

typedef struct lcd_info {
	char *name;
	int (*init)(char * name, lcd_device_opr* lcd_device);
	int (*clean)(lcd_device_opr* lcd_device, unsigned int bcack_color);
	int (*show_page)(pixel_datas* lcd_pixel_datas, lcd_device_opr* lcd_device);
	struct list_head node;
}lcd_opr;

typedef struct video_bufs {
    pixel_datas video_pixel_datas;
    int pixel_format;
}video_buf;



int lcd_init();
int lcd_device_init(char * name, lcd_device_opr * lcd);
int get_lcd_resolution(int *xres, int *yres, int *bpp);
int get_lcd_buf_display(video_buf* frame_buf);

int lcd_register(void);





#endif /* _DISP_MANAGER_H */

