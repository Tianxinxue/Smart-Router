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
#include "v4l2.h"  


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
    fmt.fmt.pix.width = frame_width ;
    fmt.fmt.pix.height = frame_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == ioctl(dev->fd, VIDIOC_S_FMT, &fmt))
    {
         printf("set video format error\n");
         return -1;
    }
    if(ioctl(dev->fd, VIDIOC_G_FMT, &fmt) == -1)                                      //检查格式
	  {
        printf("Unable to get format\n");                                         
		    return -1;
	  } 
    printf("current format:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) &
     	    0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("current height:\t\t%d\n",fmt.fmt.pix.height);
    printf("current width:\t\t%d\n\n",fmt.fmt.pix.width);
	

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;
	
    if ( -1 == init_mmap(dev))
        return -1;
    printf("init_device\n");
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
     printf("6\n");
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //运行到这里的时候会导致系统重启！！！！！
    if (-1 == ioctl(dev->fd, VIDIOC_STREAMON, &type))
    {
        printf("vidioc_streamon error\n");
	      return -1;
    }
    printf("7\n");
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

int yuy2_yuv420(unsigned char *src_buf,unsigned char *out_buf, int src_width, int src_height)
{
    int src_size = 0, out_size = 0, tem_size = 0;
    unsigned char *Y, *U, *V, *Y2, *U2, *V2;
    unsigned char *tem_buf;
    unsigned char *p=NULL;
    
    src_size = (src_width * src_height) << 1 ;//对于YUY2 4:2:2   
    tem_size = (src_width * src_height) << 1; ///对于YUV 4:2:2
    tem_buf = (unsigned char *)malloc(tem_size*sizeof(char));
    memset(tem_buf, 0, tem_size);   
    out_size = src_width*src_height * 1.5;//对于YUV 4:2:0
  
    p = src_buf;
    Y = tem_buf;
    U = Y + src_width*src_height;
    V = U + (src_width*src_height>>1);
    
    Y2 = out_buf;
    U2 = Y2 + src_width*src_height;
    V2 = U2 + (src_width*src_height>>2);
    
    //由打包YUYV变成平板YUV
    int k, j;
    for( k=0; k<src_height; ++k)
    { 
        for( j=0; j<(src_width>>1); ++j)
        {    
            Y[j*2] = p[4*j];    
            U[j] = p[4*j+1];
            Y[j*2+1] = p[4*j+2];
            V[j] = p[4*j+3];
        }
        p = p + src_width*2;
        Y = Y + src_width;
        U = U + (src_width>>1);
        V = V + (src_width>>1);
    }
    
    //复位
    Y = tem_buf;
    U = Y + src_width*src_height;
    V = U + (src_width*src_height>>1); 
    int l;
    for( l=0; l<src_height/2; ++l)
    {
        memcpy(U2, U, src_width>>1);
        memcpy(V2, V, src_width>>1);
   
        U2 = U2 + (src_width>>1);
        V2 = V2 + (src_width>>1);
   
        U = U + (src_width);
        V = V + (src_width);
    }
    memcpy(Y2, Y, src_width*src_height);
    free(tem_buf); 
    tem_buf=NULL;
    return 0;
  
} 
