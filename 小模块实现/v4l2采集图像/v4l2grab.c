#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <stdio.h>
 
 
#define  TRUE	1
#define  FALSE	0

#define FILE_VIDEO 	"/dev/video0"
#define YUV		"/home/tian/Desktop/image_yuv.yuv"

#define  IMAGEWIDTH    640
#define  IMAGEHEIGHT   480

static   int      fd;
static   struct   v4l2_capability   cap;                  //设备能力结构
struct v4l2_fmtdesc fmtdesc;
struct v4l2_format fmt,fmtack;
struct v4l2_streamparm setfps;  
struct v4l2_requestbuffers req;
struct v4l2_buffer buf;
enum v4l2_buf_type type;
unsigned char frame_buffer[IMAGEWIDTH*IMAGEHEIGHT*3];


struct buffer
{
	void * start;
	unsigned int length;
} * buffers;
//工作流程：打开设备－> 检查和设置设备属性－> 设置帧格式－> 设置一种输入输出方法（缓冲区管理）－> 循环获取数据－> 关闭设备
 
int init_v4l2(void)
{
	int i;
	int ret = 0;
	
	//打开设备
	if ((fd = open(FILE_VIDEO, O_RDWR)) == -1) 
	{
		printf("Error opening V4L interface\n");
		return (FALSE);
	}

	//查询驱动功能
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) 
	{
		printf("Error opening device %s: unable to query device.\n",FILE_VIDEO);
		return (FALSE);
	}
	else
	{
     	printf("driver:\t\t%s\n",cap.driver);                      //驱动名称
     	printf("card:\t\t%s\n",cap.card);                          //设备名称
     	printf("bus_info:\t%s\n",cap.bus_info);                    //设备在系统中的位置
     	printf("version:\t%d\n",cap.version);                      //驱动版本号
     	printf("capabilities:\t%x\n",cap.capabilities);            //设备支持的操作
     	
     	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)   //设备是否支持图像获取
     	{
			printf("Device %s: supports capture.\n",FILE_VIDEO);
		}

		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING) 
		{
			printf("Device %s: supports streaming.\n",FILE_VIDEO);
		}
	} 
	
	//列举设备支持的格式
	fmtdesc.index=0;
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
	{
		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}
	
    //设置格式
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;                                         //设置传输流类型
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;                                   //设置采样类型
	fmt.fmt.pix.height = IMAGEHEIGHT;                                              //设置帧宽高 单位像素
	fmt.fmt.pix.width = IMAGEWIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;                                    //设置采样区域
	
	if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)                                      //设置格式
	{
		printf("Unable to set format\n");
		return FALSE;
	} 	
	if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)                                      //检查格式
	{
		printf("Unable to get format\n");                                         
		return FALSE;
	} 
	{
     	printf("fmt.type:\t\t%d\n",fmt.type);
     	printf("pix.pixelformat:\t%c%c%c%c\n",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) &
     	 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
     	printf("pix.height:\t\t%d\n",fmt.fmt.pix.height);
     	printf("pix.width:\t\t%d\n",fmt.fmt.pix.width);
     	printf("pix.field:\t\t%d\n",fmt.fmt.pix.field);
	}
	//设置fps
	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	setfps.parm.capture.timeperframe.numerator = 10;
	setfps.parm.capture.timeperframe.denominator = 10;
	
	printf("init %s \t[OK]\n",FILE_VIDEO);
	    
	return TRUE;
}

int v4l2_grab(void)
{
	unsigned int n_buffers;
	
	//需要 4 个buffers 
	req.count=4;
	req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory=V4L2_MEMORY_MMAP;
	if(ioctl(fd,VIDIOC_REQBUFS,&req)==-1)
	{
		printf("request for buffers error\n");
	}

	//mmap for buffers
	buffers = malloc(req.count*sizeof (*buffers));
	if (!buffers) 
	{
		printf ("Out of memory\n");
		return(FALSE);
	}
	
	for (n_buffers = 0; n_buffers < req.count; n_buffers++) 
	{
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		//query buffers
		if (ioctl (fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			printf("query buffer error\n");
			return(FALSE);
		}

		buffers[n_buffers].length = buf.length;
		//map
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ |PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (buffers[n_buffers].start == MAP_FAILED)
		{
			printf("buffer map error\n");
			return(FALSE);
		}
	}
		
	//queue
	for (n_buffers = 0; n_buffers < req.count; n_buffers++)
	{
		buf.index = n_buffers;
		ioctl(fd, VIDIOC_QBUF, &buf);
	} 
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (fd, VIDIOC_STREAMON, &type);
	
	ioctl(fd, VIDIOC_DQBUF, &buf);

    printf("grab yuyv OK\n");
	return(TRUE);
}




int close_v4l2(void)
{
     if(fd != -1) 
     {
         close(fd);
         return (TRUE);
     }
     return (FALSE);
}


int main(void)
{

    FILE * fp;
   
   
	fp = fopen(YUV, "wb");
    if(!fp)
	{
		printf("open "YUV"error\n");
		return(FALSE);
	}

	if(init_v4l2() == FALSE) 
	{
     	return(FALSE);
	}
	

    
    v4l2_grab();
    fwrite(buffers[0].start, 640*480*2, 1, fp);
    printf("save "YUV"OK\n");
    
    
    
    fclose(fp);
    close_v4l2();
    
    return(TRUE);
}
