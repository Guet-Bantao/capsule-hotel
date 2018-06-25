#ifndef _CAMERA_MANAGER_H
#define _CAMERA_MANAGER_H

#include "list_core.h"
#include <lcd_manager.h>
#include <linux/videodev2.h>


#define REGURY_BUFFER_NUMBERS 4 
struct camera_info;

typedef struct camera_device {
	int fd;
	int pixelformat;
	int width;
    int height;
	int camera_buf_cnt;
	int camera_buf_max_len;
	int camera_buf_cur_index;
	unsigned char *puc_buf[REGURY_BUFFER_NUMBERS];

	struct camera_info* opr;
}camera_device_opr;

typedef struct camera_info {
	char *name;
	int (*init)(char * name, camera_device_opr* v4l2_device);
	int (*exit)(camera_device_opr* v4l2_device);
	int (*get_fmat)(camera_device_opr* v4l2_device);
	int (*get_frame)(camera_device_opr* v4l2_device, video_buf* camera_buf);
	int (*put_frame)(camera_device_opr* v4l2_device, video_buf* camera_buf);
	int (*start)(camera_device_opr* v4l2_device);
	struct list_head node;
}camera_opr;


int register_camera_opr(camera_opr* opr);
int camera_init(void);
int camera_device_init(char * name, camera_device_opr * camera);

int v4l2_register(void);





#endif /* _CAMERA_MANAGER_H */

