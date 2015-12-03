#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>

#include "v4l2.h"
#include "mfc/SsbSipH264Encode.h"
#include "mfc/SsbSipLogMsg.h"
#include "rtp.h"

unsigned char  *yuv420_frame = NULL;
unsigned char	 *p_inbuf;
int yuv420_size = frame_width*frame_height * 1.5;

char *name = "/dev/video2";
void *mfc_handle;
int    udp_socket;


int camera_run(v4l2_device_t *dev,const char *devname)
{
    if (-1 == open_device(devname,dev))
        return -1;
    printf("4\n");
    if ( -1 == init_device(dev))
        return -1;
    printf("5\n");
    if ( -1 == start_capturing(dev))
        return -1;
    printf("8\n");
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
	  struct timeval tv1,tv2,tv3,tv4,tv5;
	  char	*p_outbuf;
	  long size;
    struct v4l2_buffer buf; 
    CLEAR(buf);
	
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
	  
	  gettimeofday(&tv1,NULL);
	  
    if (-1 == ioctl(dev->fd, VIDIOC_DQBUF, &buf)) 
    {
        printf("vidioc_dqbuf error\n");
	      return -1;
    }
    gettimeofday(&tv2,NULL);
		yuy2_yuv420(dev->buffers[buf.index].start,yuv420_frame,frame_width,frame_height); 
		gettimeofday(&tv3,NULL);
		memcpy(p_inbuf, yuv420_frame, yuv420_size);    
		SsbSipH264EncodeExe(mfc_handle);  
		p_outbuf = SsbSipH264EncodeGetOutBuf(mfc_handle, &size); 
		gettimeofday(&tv4,NULL); 
		RTP_send(udp_socket,p_outbuf,size); 
		gettimeofday(&tv5,NULL);
		printf("dqbuf= %d, 422to420= %d,  mfc= %d,  rtp= %d\n",tv2.tv_usec - tv1.tv_usec,
		           tv3.tv_usec - tv2.tv_usec,tv4.tv_usec - tv3.tv_usec,tv5.tv_usec - tv4.tv_usec);
		 
    if (-1 == ioctl(dev->fd, VIDIOC_QBUF, &buf))  
    {
        printf("vidioc_buf error\n");
	      return -1;
     }
    return 0;
}

int main(int argc, char ** argv) 
{
	  v4l2_device_t device;
    int slices[2];
    yuv420_frame = (unsigned char *)malloc(yuv420_size*sizeof(char));
    printf("1\n");
    udp_socket = UDP_init();
    mfc_handle = SsbSipH264EncodeInit(frame_width, frame_height, 20, 1000, 1);
    printf("2\n");
    if (mfc_handle == NULL) 
    {
		    LOG_MSG(LOG_ERROR, "Test_Encoder", "SsbSipH264EncodeInit Failed\n");
		    return -1;
	  }
    // Setting multiple slices
    // This setting must be called before SsbSipH264EncodeExe()
    slices[0] = 0;	// 0 for single, 1 for multiple
	  slices[1] = 4;	// count of slices
	  SsbSipH264EncodeSetConfig(mfc_handle, H264_ENC_SETCONF_NUM_SLICES, slices);
	  SsbSipH264EncodeExe(mfc_handle);
	  p_inbuf = SsbSipH264EncodeGetInBuf(mfc_handle, 0);
	  printf("3\n");
    if ( -1 == camera_run(&device,name))
        return -1;
    
    while(1)
    {
        read_frame(&device)	;
    }
                      
   camera_stop(&device);
   close(udp_socket);
   SsbSipH264EncodeDeInit(mfc_handle);
   free(yuv420_frame); 
   yuv420_frame=NULL;
	
    return 0;
}
