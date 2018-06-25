#ifndef _LCD_MANAGER_H
#define _LCD_MANAGER_H

#include "list_core.h"


struct lcd_info;

typedef struct lcd_devices {
	int xres;               /* X�ֱ��� */
	int yres;               /* Y�ֱ��� */
	int bpp;                /* һ�������ö���λ����ʾ */
	int line_width;          /* һ������ռ�ݶ����ֽ� */
	unsigned char *puc_mem;   /* �Դ��ַ */
	unsigned int screen_size;
	//struct fb_var_screeninfo fb_var;

	unsigned int dw_line_width;
	unsigned int dw_pixel_width;

	struct lcd_info* opr;
}lcd_device_opr;

/* ͼƬ���������� */
typedef struct pixel_data {
	int width;   /* ���: һ���ж��ٸ����� */
	int height;  /* �߶�: һ���ж��ٸ����� */
	int bpp;     /* һ�������ö���λ����ʾ */
	int line_bytes;  /* һ�������ж����ֽ� */
	int total_bytes; /* �����ֽ��� */ 
	unsigned char *pixel_datas;  /* �������ݴ洢�ĵط� */
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

