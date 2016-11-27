#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>

#include "v4l2.h"
#include "rtp.h"
#include "h264encoder.h"

Encoder en;

char *name = "/dev/video0";
int    udp_socket;
char	*p_outbuf;
/*add for save yuv files start*/
//FILE *fp;
/*add for save yuv files end*/
//FILE *fp_h264;

int camera_run(v4l2_device_t *dev,const char *devname)
{
    if (-1 == open_device(devname,dev))
        return -1;
    if ( -1 == init_device(dev))
        return -1;
    if ( -1 == start_capturing(dev))
        return -1;
    return 0;
}

int camera_stop(v4l2_device_t *dev)
{
    if ( -1 == stop_capturing(dev))
        return -1;
    if ( -1 == uninit_device(dev))
        return -1;
    if ( -1 == close_device(dev))
        return -1;
    return 0;
}


int read_frame(v4l2_device_t *dev) 
{
	  //struct timeval tv1,tv2,tv3,tv4,tv5;
    struct v4l2_buffer buf; 
    CLEAR(buf);
	
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
	  
	  //gettimeofday(&tv1,NULL);
	  
    if (-1 == ioctl(dev->fd, VIDIOC_DQBUF, &buf)) 
    {
        printf("vidioc_dqbuf error\n");
	      return -1;
    }
    char *yuv_frame=dev->buffers[buf.index].start;//
    /*add for save yuv files start*/
   // fwrite(yuv_frame, buf.length, 1, fp);
    /*add for save yuv files end*/
	  if (yuv_frame[0] == '\0')
	  {
        printf("nothing in this frame\n");
	      return 0;
	  }
		//gettimeofday(&tv4,NULL); 
		long size = compress_frame(&en, -1, yuv_frame, p_outbuf);
		//fwrite(p_outbuf, size, 1, fp_h264);
		printf("size:%ld\n",size);
		if(size)
		    RTP_send(udp_socket,p_outbuf,size); 
		//gettimeofday(&tv5,NULL);
		//printf("dqbuf= %d, 422to420= %d,  mfc= %d,  rtp= %d\n",tv2.tv_usec - tv1.tv_usec,
		           //tv3.tv_usec - tv2.tv_usec,tv4.tv_usec - tv3.tv_usec,tv5.tv_usec - tv4.tv_usec);
		 
    if (-1 == ioctl(dev->fd, VIDIOC_QBUF, &buf))  
    {
        printf("vidioc_buf error\n");
	      return -1;
    }
    return 0;
}

int main(int argc, char ** argv) 
{
	  /*add for save yuv files start*/
	 // fp = fopen("test.yuv", "wa+");
	  /*add for save yuv files end*/
	  
	  //fp_h264 = fopen("test.h264", "wa+");
	  v4l2_device_t device;
    udp_socket = UDP_init();
    p_outbuf = (char *) malloc(sizeof(char) * frame_width * frame_height * 3);
    compress_begin(&en, frame_width, frame_height);
    if ( -1 == camera_run(&device,name))
        return -1;
    while(1)
    {
        read_frame(&device)	;
    }
                      
   camera_stop(&device);
   close(udp_socket);
   
   compress_end(&en);
   free(p_outbuf);
   p_outbuf = NULL;
	 /*add for save yuv files start*/
	// fclose(fp);
	 /*add for save yuv files end*/
	 
	 //fclose(fp_h264);
	 
    return 0;
}
