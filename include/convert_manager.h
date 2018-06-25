
#ifndef _CONVERT_MANAGER_H
#define _CONVERT_MANAGER_H

#include "list_core.h"
#include <lcd_manager.h>
#include <linux/videodev2.h>


typedef struct convert_info {
	char *name;
	int (*is_support)(int pixel_format_in, int pixel_format_out);
	int (*convert)(video_buf* video_buf_in, video_buf* video_buf_out);
	int (*exit)(video_buf* video_buf_out);
	struct list_head node;
}convert_opr;


int convert_init(void);
int register_convert_opr(convert_opr* opr);
convert_opr* get_convert_formats(int pixel_format_in, int Pixel_format_out);



int mjpeg_register(void);
int yuyv_register(void);
int yuyv_new_register(void);


#endif /* _CONVERT_MANAGER_H */

