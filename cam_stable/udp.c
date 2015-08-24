#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "udp.h"

int udp_server_init(int port)
{
    int sock;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return -1;
    }
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        return -1;
    }
    return sock;
}

int tcp_server_init(int port)
{
    int skfd,cnfd,addr_len;
    struct sockaddr_in srv_addr,clt_addr;
	/* 创建IPv4的流式套接字描述符 */
    if(-1 == (skfd=socket(AF_INET,SOCK_STREAM,0)))
    {
         perror("Socket Error:");
         return -1;
    }
	/* 填充服务器端sockaddr地址结构 */
    bzero(&srv_addr,sizeof(struct sockaddr_in));
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    srv_addr.sin_port=htons(port);
	/* 将套接字描述符skfd和地址信息结构体绑定起来 */
    if(-1 == bind(skfd,(struct sockaddr *)(&srv_addr),sizeof(struct sockaddr)))
    {
         perror("Bind error:");
         return -1;
    }
	/* 将skfd转换为被动建通模式 */
    printf("Listen .....\n");
    if(-1 == listen(skfd,4))
    {
         perror("Listen error:");
         return -1;
    }
    addr_len=sizeof(struct sockaddr_in);
    if(-1 == (cnfd=accept(skfd,(struct sockaddr *)(&clt_addr),&addr_len)))
    {
         perror("Accept error:");
         return -1;
    }
    return cnfd;
}
    