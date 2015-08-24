#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "udp.h"
#include "v4l2.h"

char  *cur_frame = NULL;
int  cur_frame_len;
char server_thread_wait_flag = 0;
char camera_thread_wait_flag = 0;
/*互斥锁条件变量*/
pthread_mutex_t	cam_mutex;
pthread_cond_t 	cam_cond;
//FILE *fp;  
int sock;   
char buff[1024];
//char *filename = "test.jpg";
struct sockaddr_in clientAddr;
int len = sizeof(clientAddr);
int num = 48 ;
v4l2_device_t device;
int server_count = 0;
int camera_count = 0;
char *name = "/dev/video0";
  /*
void process(const void * p, int size)           
{
    num++;
    int h;
    char filename[5] =".jpg";
    for(h=4;h>=0;h--)
        filename[h] = filename[h-1];
    filename[0] = num;
    printf("num: %d name :%s\n",num,filename);
    FILE *fp;
    fp = fopen(filename, "wa+");
	
    int n ,i;
    int pic_nu = size/1024;
    int pic_yu = size%1024;
    for(i=0;i<pic_nu;i++)
    {    
        n = sendto(sock, p+1024*i, 1024, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        if (n < 0)
        {
            perror("sendto");
            
        }
        usleep(10000);
    }
    if(pic_yu != 0)
        sendto(sock, p+1024*pic_nu, pic_yu, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    printf("pic_nu:%d pic_yu:%d\n",pic_nu,pic_yu);
   
    fwrite(p, size, 1, fp);
    fclose(fp);

}
 */
//新添加的处理当前帧的函数
int get_cur_frame(const void *ptr, int siz)
{
	cur_frame     = (char *)ptr;
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
    for(;; )
    {
        cam_get_frame(&device); 
        while (1)
        {
            pthread_mutex_lock(&cam_mutex);
            if (1 == server_thread_wait_flag)
            {
                pthread_cond_signal(&cam_cond);
                pthread_mutex_unlock(&cam_mutex);
                break;
            }
            pthread_mutex_unlock(&cam_mutex);
        }
        pthread_mutex_lock(&cam_mutex);
        camera_thread_wait_flag = 1;
        pthread_cond_wait(&cam_cond, &cam_mutex); 
	 camera_thread_wait_flag = 0;
        pthread_mutex_unlock(&cam_mutex);
	 ++camera_count;
        if (camera_count >= 2)
        {
            fprintf(stdout, "subthread loop 2 times\n");
            break;
        }
     
    }
}
int frm_nu = 0;
void *server_thread_fnc()
{
    char *flg = "q";
    for(;;)
    {
        pthread_mutex_lock(&cam_mutex);
        server_thread_wait_flag = 1;
        pthread_cond_wait(&cam_cond, &cam_mutex);
        server_thread_wait_flag = 0;
	 pthread_mutex_unlock(&cam_mutex);
    /*
        num++;
        int h;
        char filename[5] =".jpg";
        for(h=4;h>=0;h--)
            filename[h] = filename[h-1];
        filename[0] = num;
        printf("num: %d name :%s\n",num,filename);
        FILE *fp;
        fp = fopen(filename, "wa+");
        fwrite(cur_frame, cur_frame_len, 1, fp);
        fclose(fp);*/
        
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
	 while (1)
        {
            pthread_mutex_lock(&cam_mutex);
            if (1 == camera_thread_wait_flag)
            {
                pthread_cond_signal(&cam_cond);
                pthread_mutex_unlock(&cam_mutex);
                break;
            }
            pthread_mutex_unlock(&cam_mutex);
        }
	 ++server_count;
        if (server_count >= 2)
        {
            send(sock,flg,2,0);
            fprintf(stdout, "server thread loop 2 times\n");
            break;
        }
        
    }
}

int main(int argc, char ** argv) 
{
    pthread_t camera_thread;/*摄像头采集线程*/
    pthread_t server_thread; /*服务器处理当前帧线程*/
    int n;
   // sock = udp_server_init(8888);
    sock = tcp_server_init(8888);
    if(sock < 0)
    {
        printf("init tcp server error\n");
	  return -1;	
    }
    printf("init tcp server ok sock is %d\n",sock);
    device.process_image = get_cur_frame;
    if ( -1 == camera_run(&device,name))
        return -1;
    /*初始化互斥锁和条件变量*/
    pthread_mutex_init(&cam_mutex, NULL);  
    pthread_cond_init(&cam_cond, NULL);
    /*启动采集线程和服务器线程*/
    pthread_create(&camera_thread,NULL,camera_thread_fnc,NULL);
    pthread_create(&server_thread,NULL,server_thread_fnc,NULL);
    /*等待采集线程和服务器线程结束*/
    pthread_join(camera_thread,NULL);
    pthread_join(server_thread,NULL);
    camera_stop(&device);
    pthread_mutex_destroy(&cam_mutex);
    pthread_cond_destroy(&cam_cond);
   // fp = fopen(filename, "wa+");
	//接收客户端开始请求(待修改)
   // n = recvfrom(sock, buff, 3, 0, (struct sockaddr*)&clientAddr, &len);
   
	
    //fclose(fp);
    close(sock);
   
	
    return 0;
}
