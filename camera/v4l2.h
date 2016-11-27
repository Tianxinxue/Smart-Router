#ifndef _V4L2_H
#define _V4L2_H

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define frame_width   320
#define frame_height  240

struct buffer { 
    void * start;
    size_t length;  
};

typedef struct v4l2_device{
    int                   fd;         //�豸�ļ�������
    char                 name[32];    //�豸����
    struct buffer     *buffers;  //������	
    unsigned int      n_buffers;  //����������  
}v4l2_device_t;	

extern int open_device(const char *name,v4l2_device_t *dev);
extern int init_device(v4l2_device_t *dev);
extern int start_capturing(v4l2_device_t *dev);
extern int stop_capturing(v4l2_device_t *dev);
extern int uninit_device(v4l2_device_t *dev);
extern int close_device(v4l2_device_t *dev);
extern int cam_get_frame(v4l2_device_t *dev);

#endif  /*_V4L2_H*/
