/*
TCP通讯的client端；

*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#define BUFFER_SIZE 1024


int main(int argc,char *argv[])
{
	  int i = 0; 
	  FILE *fp; 
	  fp = fopen("test.jpg","a");
    int sockfd,client_fd;
    char buf[BUFFER_SIZE];
    struct sockaddr_in servaddr;

    if(argc<3)
    {
        printf("USAGE=%s <serv_in> <serv_port>\n",argv[0]);
        exit(-1);
    }

   // memset(buf,0,sizeof(buf));

/*创建socket*/
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("socket");
        exit(-1);
    }
    

/*创建sockaddr_in结构体中相关参数*/
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
/*调用connect函数主动发起对服务端的链接*/

    if(connect(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr))==-1)
    {
        perror("connect");
        exit(-1);
    }
/*发送消息给服务端*/
while(1)
{
	  i++;
    memset(buf,0,sizeof(buf));
    
    if(recv(sockfd,buf,BUFFER_SIZE,0)==-1)
    {
        perror("recv");
        exit(0);
    }
    if(buf[0] =='q' )
        break;
 
    fwrite(buf, 1024, 1, fp);
    printf("i:%d\n",i);
}
    fclose(fp);
    close(sockfd);
    exit(0);
}
