#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <poll.h>

#include <camera_manager.h>

static int support_fmts[] = {V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565};
static camera_opr v4l2_device_opr;


/*query for support which format*/
static int query_support_fmt(int pixelformat)
{
    int i;
    for (i = 0; i < sizeof(support_fmts)/sizeof(support_fmts[0]); i++)
    {
        if (support_fmts[i] == pixelformat)
            return 1;
    }
    return 0;
}

static int v4l2_init(char * name, camera_device_opr* v4l2_device)
{
	int fd;
	int error;
	int lcd_width=640,lcd_heigt=480,lcd_bpp=0;
	int i;
	struct v4l2_capability v4l2_cap;
	struct v4l2_fmtdesc v4l2_fmt_desc;
	struct v4l2_format  v4l2_fmt;
	struct v4l2_requestbuffers v4l2_reqbuffs;
	struct v4l2_buffer v4l2_buf;

	fd = open(name, O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		printf("can't open %s\n", name);
		return -1;
	}
	v4l2_device->fd = fd;

	/*use VIDIOC_QUERYCAP,What is device?*/
	error = ioctl(fd, VIDIOC_QUERYCAP, &v4l2_cap);
	memset(&v4l2_cap, 0, sizeof(struct v4l2_capability));
    error = ioctl(fd, VIDIOC_QUERYCAP, &v4l2_cap);
	 if (error)
	 {
			printf("Error opening device %s: unable to query device.\n", name);
			goto err_exit;
	 }
	 
	 if (!(v4l2_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
    	printf("%s is not a video capture device\n", name);
        goto err_exit;
    }
	if (v4l2_cap.capabilities & V4L2_CAP_STREAMING)
	{
	    printf("%s supports streaming i/o\n", name);
	}
	else//if this device is not support streaming i/o
	{
		printf("sorry,this drive not support others device\n");
        goto err_exit;
	}

	/*use VIDIOC_ENUM_FMT, list device's format */
	memset(&v4l2_fmt_desc, 0, sizeof(v4l2_fmt_desc));
	v4l2_fmt_desc.index = 0;
	v4l2_fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while ((error = ioctl(fd, VIDIOC_ENUM_FMT, &v4l2_fmt_desc)) == 0) {
        if (query_support_fmt(v4l2_fmt_desc.pixelformat))
        {
            v4l2_device->pixelformat = v4l2_fmt_desc.pixelformat;
            break;
        }
		v4l2_fmt_desc.index++;//list next device's format
	}
	if (!v4l2_device->pixelformat)
    {
    	printf("can not support the format of this device\n");
        goto err_exit;        
    }

	/* set format */
    //get_lcd_resolution(&lcd_width, &lcd_heigt, &lcd_bpp);
    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt.fmt.pix.pixelformat = v4l2_device->pixelformat;
    v4l2_fmt.fmt.pix.width       = lcd_width;
    v4l2_fmt.fmt.pix.height      = lcd_heigt;
    v4l2_fmt.fmt.pix.field       = V4L2_FIELD_ANY;
	printf("lcd's width:%d, height:%d, bpp:%d \n", lcd_width, lcd_heigt, lcd_bpp);
	/*use VIDIOC_S_FMT,Set format,but it will automatically adjust */
	error = ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt); 
    if (error) 
    {
    	printf("Unable to set format!\n");
        goto err_exit;        
    }
	/*This is the result of automatic adjustment*/
	v4l2_device->width	 = v4l2_fmt.fmt.pix.width;
    v4l2_device->height	= v4l2_fmt.fmt.pix.height;
	printf("adjustment width:%d, height:%d\n", v4l2_device->width, v4l2_device->height);

    memset(&v4l2_reqbuffs, 0, sizeof(struct v4l2_requestbuffers));
    v4l2_reqbuffs.count		= REGURY_BUFFER_NUMBERS;
    v4l2_reqbuffs.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_reqbuffs.memory	= V4L2_MEMORY_MMAP;
	/* use VIDIOC_REQBUFS, request buffers */
	error = ioctl(fd, VIDIOC_REQBUFS, &v4l2_reqbuffs);
    if (error) 
    {
    	printf("Unable to allocate buffers.\n");
        goto err_exit;        
    }
	v4l2_device->camera_buf_cnt = v4l2_reqbuffs.count;

	/* map the buffers */
	for (i = 0; i < v4l2_device->camera_buf_cnt; i++)
	{
		memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
		v4l2_buf.index = i;
        v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       	v4l2_buf.memory = V4L2_MEMORY_MMAP;
		/* use VIDIOC_QUERYBUF, query buffers */
		error = ioctl(fd, VIDIOC_QUERYBUF, &v4l2_buf);
    	if (error) 
        {
    	    printf("Unable to query buffer.\n");
    	    goto err_exit;
    	}

		v4l2_device->camera_buf_max_len = v4l2_buf.length;
		v4l2_device->puc_buf[i] = mmap(0 /* start anywhere */ ,
        			  v4l2_buf.length, PROT_READ, MAP_SHARED, fd,
        			  v4l2_buf.m.offset);
		if (v4l2_device->puc_buf[i] == MAP_FAILED)
		{
        	    printf("Unable to map buffer\n");
        	    goto err_exit;
		}
	}

	/* Queue the buffers. */
    for (i = 0; i < v4l2_device->camera_buf_cnt; i++) 
    {
    	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
    	v4l2_buf.index 	= i;
    	v4l2_buf.type  	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	v4l2_buf.memory = V4L2_MEMORY_MMAP;
    	error = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
    	if (error)
        {
    	    printf("Unable to queue buffer.\n");
    	    goto err_exit;
    	}
    }

	//v4l2_device->opr = &v4l2_device_opr;
    return 0;

 err_exit:	  
	 close(fd);
	 return -1;

}

static int v4l2_exit(camera_device_opr* v4l2_device)
{
    int i;
	printf("exit \n");

    for (i = 0; i < v4l2_device->camera_buf_cnt; i++)
    {
        if (v4l2_device->puc_buf[i])
        {
            munmap(v4l2_device->puc_buf[i], v4l2_device->camera_buf_max_len);
            v4l2_device->puc_buf[i] = NULL;
        }
    }
        
    close(v4l2_device->fd);
    return 0;
}

static int v4l2_get_format(camera_device_opr* v4l2_device)
{
    return v4l2_device->pixelformat;
}

static int v4l2_get_frame_streaming(camera_device_opr* v4l2_device, video_buf* camera_buf)
{
	struct pollfd fds[1];
	struct v4l2_buffer v4l2_buf;
	int ret;

	/* poll */
    fds[0].fd     = v4l2_device->fd;
    fds[0].events = POLLIN;

	ret = poll(fds, 1, -1);
    if (ret <= 0)
    {
        printf("poll error!\n");
        return -1;
    }

	/* VIDIOC_DQBUF */
    memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(v4l2_device->fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) 
    {
    	printf("Unable to dequeue buffer.\n");
    	return -1;
    }
	v4l2_device->camera_buf_cur_index		= v4l2_buf.index;
	
	camera_buf->pixel_format				= v4l2_device->pixelformat;
    camera_buf->video_pixel_datas.width		= v4l2_device->width;
    camera_buf->video_pixel_datas.height	= v4l2_device->height;
	camera_buf->video_pixel_datas.bpp   	= (v4l2_device->pixelformat == V4L2_PIX_FMT_YUYV) ? 16 : \
                                        		(v4l2_device->pixelformat == V4L2_PIX_FMT_MJPEG) ? 0 :  \
                                        		(v4l2_device->pixelformat == V4L2_PIX_FMT_RGB565) ? 16 :  \
                                        		0;
	camera_buf->video_pixel_datas.line_bytes	= v4l2_device->width * camera_buf->video_pixel_datas.bpp / 8;
    camera_buf->video_pixel_datas.total_bytes	= v4l2_buf.bytesused;
    camera_buf->video_pixel_datas.pixel_datas	= v4l2_device->puc_buf[v4l2_buf.index]; 
	
    return 0;

	
}

static int v4l2_put_frame_streaming(camera_device_opr* v4l2_device, video_buf* camera_buf)
{
	/* VIDIOC_QBUF */
    struct v4l2_buffer v4l2_buf;
	int error;
    
	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.index  = v4l2_device->camera_buf_cur_index;
	v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	error = ioctl(v4l2_device->fd, VIDIOC_QBUF, &v4l2_buf);
	if (error) 
    {
	    printf("Unable to queue buffer.\n");
	    return -1;
	}
    return 0;
}

static int v4l2_start(camera_device_opr* v4l2_device)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int error;

    error = ioctl(v4l2_device->fd , VIDIOC_STREAMON, &type);
    if (error) 
    {
    	printf("Unable to start capture.\n");
    	return -1;
    }
    return 0;
}




static camera_opr v4l2_device_opr = {
    .name		= "v4l2",
	.init		= v4l2_init,
	.exit		= v4l2_exit,
	.get_fmat	= v4l2_get_format,
	.get_frame  = v4l2_get_frame_streaming,
	.put_frame	= v4l2_put_frame_streaming,
	.start		= v4l2_start,
};


int v4l2_register(void)
{
	return register_camera_opr(&v4l2_device_opr);
}




