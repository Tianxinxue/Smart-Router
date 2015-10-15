#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "v4l2.h"
#include "mfc/SsbSipH264Encode.h"
#include "mfc/SsbSipLogMsg.h"
#include "rtp.h"

#define MAX 1000//需要生产的数量

FILE *fp;
unsigned char  *cur_frame    = NULL;
unsigned char  *yuv420_frame = NULL;
unsigned char  *h264_frame   = NULL;
unsigned char	 *p_inbuf;
unsigned char	*p_outbuf;
int  cur_frame_len;
int yuv420_size = frame_width*frame_height * 1.5;
/*互斥锁条件变量*/
pthread_mutex_t	cam_mutex;
pthread_cond_t 	cam_condc,cam_condp;
int cam_cond_flag = 0;   
v4l2_device_t device;
int server_count = 0;
int camera_count = 0;
char *name = "/dev/video2";
void *mfc_handle;
int    udp_socket;
long size;

int get_cur_frame(const void *ptr, int siz)
{
	cur_frame     = (unsigned char *)ptr;
	cur_frame_len = siz;	
	return 0;
}

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

void *camera_thread_fnc()
{
    int i;
    for(i=1; i<=MAX; i++)
    {
        pthread_mutex_lock(&cam_mutex); //互斥使用缓冲区
        while(cam_cond_flag !=0) pthread_cond_wait(&cam_condp, &cam_mutex);
        cam_get_frame(&device);
        yuy2_yuv420(cur_frame,yuv420_frame,frame_width,frame_height);
        // copy YUV data into input buffer
		    memcpy(p_inbuf, yuv420_frame, yuv420_size);
		    SsbSipH264EncodeExe(mfc_handle);
		    p_outbuf = SsbSipH264EncodeGetOutBuf(mfc_handle, &size);
        printf("procucer produce item %d\n",i);
        cam_cond_flag = 1; 
        pthread_cond_signal(&cam_condc);//唤醒消费者
        pthread_mutex_unlock(&cam_mutex);//释放缓冲区
    }
    
    pthread_exit(0);
    
}


void *server_thread_fnc()
{             
    int i;

    for(i=1; i<=MAX; i++)
    {
        pthread_mutex_lock(&cam_mutex);//互斥使用缓冲区
        while(cam_cond_flag ==0) pthread_cond_wait(&cam_condc, &cam_mutex);
        printf("consumer consume item %d\n",i);
        //fwrite(cur_frame, cur_frame_len, 1, fp);
        //fwrite(yuv420_frame, yuv420_size, 1, fp);
        RTP_send(udp_socket,p_outbuf,size); 
       // fwrite(p_outbuf,size,1,fp);
        cam_cond_flag = 0;
        pthread_cond_signal(&cam_condp);//唤醒生产者
        pthread_mutex_unlock(&cam_mutex);//释放缓冲区
    }
    pthread_exit(0);

}


int main(int argc, char ** argv) 
{
	  int slices[2];
    pthread_t camera_thread;/*摄像头采集线程*/
    pthread_t server_thread; /*服务器处理当前帧线程*/
    device.process_image = get_cur_frame;
    yuv420_frame = (unsigned char *)malloc(yuv420_size*sizeof(char));
   // fp = fopen("test.h264", "wa+");
    udp_socket = UDP_init();
    mfc_handle = SsbSipH264EncodeInit(frame_width, frame_height, 20, 1000, 1);
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
    if ( -1 == camera_run(&device,name))
        return -1;
    /*初始化互斥锁和条件变量*/
    pthread_mutex_init(&cam_mutex, NULL);  
    pthread_cond_init(&cam_condc, NULL);
    pthread_cond_init(&cam_condp, NULL);
    /*启动采集线程和服务器线程*/
    pthread_create(&camera_thread,NULL,camera_thread_fnc,NULL);
    pthread_create(&server_thread,NULL,server_thread_fnc,NULL);
    /*等待采集线程和服务器线程结束*/
    pthread_join(camera_thread,NULL);
    pthread_join(server_thread,NULL);
    camera_stop(&device);
    pthread_cond_destroy(&cam_condc);
    pthread_cond_destroy(&cam_condp);
    pthread_mutex_destroy(&cam_mutex);
   // close(sock);
   SsbSipH264EncodeDeInit(mfc_handle);
   //fclose(fp);
   free(yuv420_frame); 
   yuv420_frame=NULL;
	
    return 0;
}
