#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	  FILE *fp; 
	  fp = fopen("test.jpg","a");
    if (argc != 3)
    {
        printf("Usage: %s ip port", argv[0]);
        exit(1);
    }
    printf("This is a UDP client\n");
    struct sockaddr_in addr;
    int sock;

    if ( (sock=socket(AF_INET, SOCK_DGRAM, 0)) <0)
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Incorrect ip address!");
        close(sock);
        exit(1);
    }

    char buff[1024];
    int len = sizeof(addr);
    int i = 0;
    gets(buff);
        int n;
        n = sendto(sock, buff, strlen(buff), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (n < 0)
        {
            perror("sendto");
            close(sock);
            return -1;
        }
    while (1)
    {        
        n = recvfrom(sock, buff, 1024, 0, (struct sockaddr *)&addr, &len);
        if (n > 0)
        {
        	/*
        	  if(buff[0] == 'q')
        	  {
            printf("end\n");
            close(sock);
            fclose(fp);
            break;

            }
            */
            printf("received:");
            //fwrite(buff, 1024, 1, fp);
            //memset(buff,0,1024);
            i++;
            printf("i:%d\n",i);
        }
        
        else
        {
            printf("server closed\n");
            printf("i:%d\n",i);
            close(sock);
            fclose(fp);
            break;

        }
        
    }
    
    return 0;
}
    
