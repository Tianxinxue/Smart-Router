#ifndef _V4L2_H
#define _V4L2_H

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define frame_width   176
#define frame_height  144

struct buffer { 
    void * start;
    size_t length;  
};

typedef struct v4l2_device{
    int                   fd;         //设备文件描述符
    char                 name[32];    //设备名称
    struct buffer     *buffers;  //缓冲区	
    unsigned int      n_buffers;  //缓冲区个数  
}v4l2_device_t;	

extern int open_device(const char *name,v4l2_device_t *dev);
extern int init_device(v4l2_device_t *dev);
extern int start_capturing(v4l2_device_t *dev);
extern int stop_capturing(v4l2_device_t *dev);
extern int uninit_device(v4l2_device_t *dev);
extern int close_device(v4l2_device_t *dev);
extern int cam_get_frame(v4l2_device_t *dev);
extern int yuy2_yuv420(unsigned char *src_buf,unsigned char *out_buf, int src_width, int src_height);

#endif  /*_V4L2_H*/
