#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <memory.h>
#include <errno.h>  
 
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h> 
#include "rtp.h"

int TCP_init(int port)
{
    int sockfd;
	  int clientfd;
	
    struct sockaddr_in server_addr; 
    struct sockaddr_in client_addr;
    int sin_size;
    
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)  
    {
        printf("Socket error:%s\n\a",strerror(errno));
        exit(1);
     }
    bzero(&server_addr,sizeof(struct sockaddr_in));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(port);

    if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
    {
        printf("Bind error:%s\n\a",strerror(errno));
        exit(1);
    }
 
    if(listen(sockfd, 10)==-1)
    {
        fprintf(stderr,"Listen error:%s\n\a",strerror(errno));
        exit(1);
     }
    sin_size=sizeof(struct sockaddr_in);

    if((clientfd=accept(sockfd,(struct sockaddr *)(&client_addr),(socklen_t *)&sin_size))==-1)
    {
        fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
        exit(1);
    } 
	
    return clientfd;	
}

int UDP_init() 
{
    int    udpsocket; 
    struct sockaddr_in server;  
    int len =sizeof(server);  
    float framerate=20;  
    unsigned int timestamp_increase=0,ts_current=0;  
    timestamp_increase=(unsigned int)(90000.0 / framerate); //+0.5);  
  
    server.sin_family=AF_INET;  
    server.sin_port=htons(UDP_DEST_PORT);            
    server.sin_addr.s_addr=inet_addr(UDP_DEST_IP);   
    udpsocket=socket(AF_INET,SOCK_DGRAM,0);  
    if(udpsocket==-1)
    {
        fprintf(stderr,"UDP create socket error:%s\n\a",strerror(errno));
        exit(1);
    } 
    if(connect(udpsocket, (const struct sockaddr *)&server, len) == -1)//none three handshake
    {
	   printf("UDP connect error\n"); 
	   exit(1);
	}
	 return udpsocket; 
}

int RTP_send(int socketfd,char *buf,unsigned int len) 
{

    static unsigned short seq_num =0;
	  static unsigned int ts_current=0;	
	  FU_INDICATOR    fu_ind;  
	  FU_HEADER       fu_hdr;  
	  NALU_HEADER     nalu_hdr;
	  char* nalu_payload;
	
	  int bytes=0; 
    float framerate=25;  
    unsigned int timestamp_increase=0;  
    timestamp_increase=(unsigned int)(90000.0 / framerate); //+0.5); 	
	
	  len = len -4; //delete the 00 00 00 01
		
	  RTP_FIXED_HEADER  *rtp_hdr;
	  char sendbuf[1500];  
	  memset(sendbuf,0,1500);

	  rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];   
	  //设置RTP HEADER，  
	  rtp_hdr->payload     = H264;  //负载类型号，  
	  rtp_hdr->version     = 2;  //版本号，此版本固定为2  
	  rtp_hdr->marker    = 0;   //标志位，由具体协议规定其值。  
	  rtp_hdr->ssrc        = htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一 	
	
	  nalu_hdr.F = buf[4] &  0x80; //1 bit  
	  nalu_hdr.NRI = buf[4] &  0x60; // 2 bit 
	  nalu_hdr.TYPE = buf[4] &  0x1f;// 5 bit   
	
	  if(len <= 1400) 
	  {
		    //设置rtp M 位；  
		    rtp_hdr->marker=1;  
		    rtp_hdr->seq_no     = htons(seq_num ++); //序列号，每发送一个RTP包增1 
		
		    ts_current=ts_current+timestamp_increase;  
		    rtp_hdr->timestamp=htonl(ts_current);  
        
		    memcpy(&sendbuf[12],&buf[4],len);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。
		
	      send( socketfd, sendbuf, len+12, 0 );//发送rtp包	 
		
	  }
	  else if(len>1400)
	  {
		    int k=0,l=0;  
        k=len/1400;//需要k个1400字节的RTP包  
        l=len%1400;//最后一个RTP包的需要装载的字节数  
        int t=0;//用于指示当前发送的是第几个分片RTP包  
        ts_current=ts_current+timestamp_increase;  
        rtp_hdr->timestamp=htonl(ts_current);  // There are same timestamp in one NALU 
        while(t<=k)  //there are k + l RTP packages,because index = 0 1...k
        {  
            rtp_hdr->seq_no = htons(seq_num ++); //序列号，每发送一个RTP包增1  
            if(!t)//发送一个需要分片的NALU的第一个分片，置FU HEADER的S位  
            {  
                //设置rtp M 位；  
                rtp_hdr->marker=0;  
                //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                fu_ind.F=nalu_hdr.F;  
                fu_ind.NRI=nalu_hdr.NRI;  
					      /*Values equal to 28 and 29 in the Type field of the FU indicator octet
                  identify an FU-A and an FU-B, respectively
					      */
                fu_ind.TYPE=28;  
                sendbuf[12] = fu_ind.F | fu_ind.NRI | fu_ind.TYPE;
				     
                //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                //   fu_hdr =(FU_HEADER*)&sendbuf[13];  
                fu_hdr.E=0;  
                fu_hdr.R=0;  
                fu_hdr.S=1;  
                fu_hdr.TYPE=nalu_hdr.TYPE;  
                     
		            sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE; 
                nalu_payload=&sendbuf[14];//同理将sendbuf[14]赋给nalu_payload  
                memcpy(nalu_payload,&buf[5],1400);//去掉NALU头    
                bytes=1400+14;                      //获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节  
                send( socketfd, sendbuf, bytes, 0 );//发送rtp包   
                t++;  
                      
                }
                else if(k==t)
                {  
                    //发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位  
                    //发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。  
                   
                    //设置rtp M 位；当前传输的是最后一个分片时该位置1  
                    rtp_hdr->marker=1;  
                    //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                    //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                    fu_ind.F=nalu_hdr.F;    
                    fu_ind.NRI=nalu_hdr.NRI; 
                    fu_ind.TYPE=28;  
                      
				            sendbuf[12] = fu_ind.F  | fu_ind.NRI | fu_ind.TYPE; 	  
                    //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                    //   fu_hdr =(FU_HEADER*)&sendbuf[13];  
                    fu_hdr.R=0;  
                    fu_hdr.S=0;  
                    fu_hdr.TYPE=nalu_hdr.TYPE;
                    fu_hdr.E=1;  
                    sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE;
					 
					 
                    nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload  
                    memcpy(nalu_payload,&buf[t*1400+5],l);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。  
                    bytes=l-1+14;       //获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节  
                    send( socketfd, sendbuf, bytes, 0 );//发送rtp包  
                    t++;  
                    //sleep(1);  
                }
                else if(t<k&&0!=t)
                {  
                    //设置rtp M 位；  
                    rtp_hdr->marker=0;  
                    //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]  
                    //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；  
                    fu_ind.F=nalu_hdr.F ; 
                    fu_ind.NRI=nalu_hdr.NRI;  
                    fu_ind.TYPE=28;  
                    sendbuf[12] = fu_ind.F  | fu_ind.NRI  | fu_ind.TYPE; 	     
                    //设置FU HEADER,并将这个HEADER填入sendbuf[13]  
                    //    fu_hdr =(FU_HEADER*)&sendbuf[13];  
                    //fu_hdr.E=0;  
                    fu_hdr.R=0;  
                    fu_hdr.S=0;  
                    fu_hdr.E=0;  
                    fu_hdr.TYPE=nalu_hdr.TYPE;  
                    sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE;
				  
                    nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload  
                    memcpy(nalu_payload,&buf[t*1400+5],1400);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。  
                    bytes=1400+14;                      //获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节  
                    send( socketfd, sendbuf, bytes, 0 );//发送rtp包  
                                        //sleep(1);  
                    t++;  
                }  	
	      }
    }
    return 0;
}


