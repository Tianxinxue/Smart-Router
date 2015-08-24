#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#define PORT 4321
#define BUFFER_SIZE 1024
#define MAX 5
#include <pthread.h>
int main()
{
    
    struct sockaddr_in servaddr;
    int sockfd,client_fd;
    char buf[BUFFER_SIZE];

/*建立socket连接*/
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("socket");
        exit(1);
    }

    printf("socket id=%d\n",sockfd);
/*设置sockaddr_in结构体中相关参数*/
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(PORT);
    servaddr.sin_addr.s_addr=INADDR_ANY;
    int i=1;  /*允许重复使用本地址与套接字进行绑定*/
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i));
/*绑定函数bind()*/

    if(bind(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr))==-1)
    {
        perror("bind");
        exit(1);
    }

    printf("Bind success!\n");
/*调用listen函数，创建未处理请求的队列*/
    if(listen(sockfd,MAX)==-1)
    {
        perror("listen");
        exit(1);
    }

    printf("Listen...\n");

/*调用accept函数，等待客户端连接*/
    if((client_fd=accept(sockfd,NULL,NULL))==-1)
    {
        perror("accept");
        exit(0);
    }
/*调用recv()函数接收客户端的请求*/
    memset(buf,0,sizeof(buf));
    
    if(recv(client_fd,buf,BUFFER_SIZE,0)==-1)
    {
        perror("recv");
        exit(0);
    }

    printf("Received a message:%s\n",buf);
    close(sockfd);
    exit(0);
}
