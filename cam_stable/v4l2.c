#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>              
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>          
#include <sys/ioctl.h>
#include <asm/types.h>    
#include <linux/videodev2.h>
#include "v4l2.h"  

int frame_nu = 0;


int open_device(const char *name,v4l2_device_t *dev) 
{
#define DEFAULT_DEVICE "/dev/video0"
    if (!name)
        name = DEFAULT_DEVICE;
    if ((dev->fd=open(name, O_RDWR))<0)  
    {
         printf("Cannot open device error in open_device\n");
         return -1;
     }	  
#undef DEFAULT_DEVICE
    return 0;
}

int init_mmap(v4l2_device_t *dev) 
{
    struct v4l2_requestbuffers req;
    struct buffer * buffers;
    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(dev->fd, VIDIOC_REQBUFS, &req)) 
    {
        printf("device does not support memory mapping\n");
	 return -1;
     }

     if (req.count < 2) 
     {
         printf("Insufficient buffer memory on device\n");
	  return -1;
      }

     dev->buffers = calloc(req.count, sizeof(*buffers));

    if (!dev->buffers) 
     {
          printf("Out of memory\n");
	   return -1;
      }

     for (dev->n_buffers = 0; dev->n_buffers < req.count; ++dev->n_buffers) 
     {

         struct v4l2_buffer buf;
         CLEAR(buf);

         buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
         buf.memory = V4L2_MEMORY_MMAP;
         buf.index = dev->n_buffers;
		 
         if (-1 == ioctl(dev->fd, VIDIOC_QUERYBUF, &buf))
         {
              printf("error querybuf\n");
	       return -1;
	   }

         dev->buffers[dev->n_buffers].length = buf.length;
         dev->buffers[dev->n_buffers].start = mmap(NULL , buf.length,PROT_READ | PROT_WRITE ,
		             MAP_SHARED , dev->fd, buf.m.offset);
         if (MAP_FAILED == dev->buffers[dev->n_buffers].start)
         {
              printf("mmap error\n");
	       return -1;
          }
     }
    return 0;
}

int init_device(v4l2_device_t *dev) 
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc fmtdesc;
    unsigned int min;

    if (-1 == ioctl(dev->fd, VIDIOC_QUERYCAP, &cap))
    {
         printf("qureycap error\n");
         return -1;
    }
    printf("\n***camera information***\n");
    printf("driver:\t\t%s\n",cap.driver);                      /*驱动名称*/
    printf("card:\t\t%s\n",cap.card);                          /*设备名称*/
    printf("bus_info:\t%s\n",cap.bus_info);             /*设备在系统中的位置*/

    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Camera Support formats:\n");
    while(ioctl(dev->fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        printf("%d.%s\n",fmtdesc.index+1,fmtdesc.description);
	 fmtdesc.index++;
    }
    printf("\n");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
    {
        printf("device is not video capture device\n");
        return -1;
     }

    /* Select video input, video standard and tune here. */
   CLEAR(cropcap);
   cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   if (0 == ioctl(dev->fd, VIDIOC_CROPCAP, &cropcap)) 
   {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

     }
	   
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == ioctl(dev->fd, VIDIOC_S_FMT, &fmt))
    {
         printf("set video format error\n");
         return -1;
     }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;
	
    if ( -1 == init_mmap(dev))
        return -1;
    return 0;
	  
}


int start_capturing(v4l2_device_t *dev) 
{
    unsigned int i;
    enum v4l2_buf_type type;
    for (i = 0; i < dev->n_buffers; ++i) 
    {
        struct v4l2_buffer buf;
	 CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	 buf.memory = V4L2_MEMORY_MMAP;
	 buf.index = i;
	 if (-1 == ioctl(dev->fd, VIDIOC_QBUF, &buf))
	 {
            printf("vidioc_qbuf error\n");
	     return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(dev->fd, VIDIOC_STREAMON, &type))
    {
        printf("vidioc_streamon error\n");
	  return -1;
    }
    return 0;
	  
}

int uninit_device(v4l2_device_t *dev) 
{
    unsigned int i;
    for (i = 0; i < dev->n_buffers; ++i)
    {
        if (-1 == munmap(dev->buffers[i].start, dev->buffers[i].length))
        {
            printf("munmap error\n");
	     return -1;
	  }
    }		              
    free(dev->buffers);
    return 0;
}


int read_frame(v4l2_device_t *dev) 
{
     frame_nu++;
    struct v4l2_buffer buf; 
    CLEAR(buf);
	
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
	
  /*这里为什么不在取出视频缓冲区以后再处理数据？*/
  /*投放一个空的视频缓冲区到视频缓冲区输入队列中*/
    if (-1 == ioctl(dev->fd, VIDIOC_DQBUF, &buf)) 
    {
        printf("vidioc_dqbuf error\n");
	 return -1;
     }
			          
    dev->process_image(dev->buffers[buf.index].start, buf.length);   /*处理当前帧*/
    printf("buff.index %d buff.length %d frame_num:%d\n",buf.index,buf.length,frame_nu);
     /*从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区*/
    if (-1 == ioctl(dev->fd, VIDIOC_QBUF, &buf))  
    {
        printf("vidioc_buf error\n");
	 return -1;
     }
    return 0;
}
int cam_get_frame(v4l2_device_t *dev)
{
    fd_set fds;
    struct timeval tv;
    int ret;
    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);
    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;   
    ret= select(dev->fd + 1, &fds, NULL, NULL, &tv);
    if (-1 == ret) 
    {
        printf("select error\n");
	 return -1;
    }

    else if (0 == ret) 
    {
        printf("select timeout/n");
	 return -1;
    }
    else
        read_frame(dev);	
       // sleep(1);
}
    
int stop_capturing(v4l2_device_t *dev) 
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(dev->fd, VIDIOC_STREAMOFF, &type))
    {
        printf("vidioc_streamoff error\n");
	 return -1;
     }
	return 0;

}

int close_device(v4l2_device_t *dev)
{
    if (-1 == close(dev->fd))
    {
        printf("close device error\n");
	 return -1;
     }
	return 0;
	  
}
