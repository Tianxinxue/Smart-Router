#ifndef _V4L2_H
#define _V4L2_H

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer { 
    void * start;
    size_t length;  
};

typedef struct v4l2_device{
    int                   fd;         //设备文件描述符
    char                 name[32];    //设备名称
    struct buffer     *buffers;  //缓冲区	
    unsigned int      n_buffers;  //缓冲区个数  
    int (*process_image)(const void * ,int);  //处理一帧图像的函数指针

}v4l2_device_t;	

extern int open_device(const char *name,v4l2_device_t *dev);
extern int init_device(v4l2_device_t *dev);
extern int start_capturing(v4l2_device_t *dev);
extern int read_frame(v4l2_device_t *dev);
extern int stop_capturing(v4l2_device_t *dev);
extern int uninit_device(v4l2_device_t *dev);
extern int close_device(v4l2_device_t *dev);
extern int cam_get_frame(v4l2_device_t *dev);

#endif  /*_V4L2_H*/
