#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "udp.h"
#include "v4l2.h"

#define MAX 8 //需要生产的数量

unsigned char  *cur_frame = NULL;
unsigned char  *yuv420_frame = NULL;
int  cur_frame_len;
/*互斥锁条件变量*/
pthread_mutex_t	cam_mutex;
pthread_cond_t 	cam_condc,cam_condp;
int cam_cond_flag = 0;
int sock;   
char buff[1024];
struct sockaddr_in clientAddr;
int len = sizeof(clientAddr);
int num = 48 ;
v4l2_device_t device;
int server_count = 0;
int camera_count = 0;
char *name = "/dev/video0";

int get_cur_frame(const void *ptr, int siz)
{
	cur_frame     = (unsigned char *)ptr;
	cur_frame_len = siz;	
	printf("get_cur_frame\n");
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
        printf("procucer produce item %d\n",i);
        cam_cond_flag = 1; 
        pthread_cond_signal(&cam_condc);//唤醒消费者
        pthread_mutex_unlock(&cam_mutex);//释放缓冲区
    }
    
    pthread_exit(0);
    
}


int frm_nu = 0;
void *server_thread_fnc()
{
    int yuv420_size = frame_width*frame_height * 1.5;
    char *flg = "q";
      
#if 0
    int n ,i;
    int pic_nu = cur_frame_len/1024;
    int pic_yu = cur_frame_len%1024;
    for(i=0;i<pic_nu;i++)
    {  
        frm_nu++;
        //n = sendto(sock, cur_frame+1024*i, 1024, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        send(sock,cur_frame+1024*i,1024,0);
       printf("fram:%d\n",frm_nu);
	 if (n < 0)
        {
            perror("sendto");
            
        }
    }
    if(pic_yu != 0)
       // sendto(sock, cur_frame+1024*pic_nu, pic_yu, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    printf("pic_nu:%d pic_yu:%d\n",pic_nu,pic_yu);
#endif

    int i;
    for(i=1; i<=MAX; i++)
    {
        pthread_mutex_lock(&cam_mutex);//互斥使用缓冲区
        while(cam_cond_flag ==0) pthread_cond_wait(&cam_condc, &cam_mutex);
        printf("consumer consume item %d\n",i);
        num++;
        int h;
        char filename[5] =".yuv";
        for(h=4;h>=0;h--)
            filename[h] = filename[h-1];
        filename[0] = num;
        yuy2_yuv420(cur_frame,yuv420_frame,frame_width,frame_height);
        printf("num: %d name :%s\n",num,filename);
        FILE *fp;
        fp = fopen(filename, "wa+");
        //fwrite(cur_frame, cur_frame_len, 1, fp);
        fwrite(yuv420_frame, yuv420_size, 1, fp);
        fclose(fp);
        cam_cond_flag = 0;
        pthread_cond_signal(&cam_condp);//唤醒生产者
        pthread_mutex_unlock(&cam_mutex);//释放缓冲区
    }
    pthread_exit(0);

}


int main(int argc, char ** argv) 
{
    int yuv420_size = frame_width*frame_height * 1.5;
    pthread_t camera_thread;/*摄像头采集线程*/
    pthread_t server_thread; /*服务器处理当前帧线程*/
    int n;
   // sock = udp_server_init(8888);
    //sock = tcp_server_init(8888);
    if(sock < 0)
    {
        printf("init tcp server error\n");
	  return -1;	
    }
    printf("init tcp server ok sock is %d\n",sock);
    device.process_image = get_cur_frame;
    yuv420_frame = (unsigned char *)malloc(yuv420_size*sizeof(char));
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
   free(yuv420_frame); 
   yuv420_frame=NULL;
	
    return 0;
}
