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
	  //����RTP HEADER��  
	  rtp_hdr->payload     = H264;  //�������ͺţ�  
	  rtp_hdr->version     = 2;  //�汾�ţ��˰汾�̶�Ϊ2  
	  rtp_hdr->marker    = 0;   //��־λ���ɾ���Э��涨��ֵ��  
	  rtp_hdr->ssrc        = htonl(10);    //���ָ��Ϊ10�������ڱ�RTP�Ự��ȫ��Ψһ 	
	
	  nalu_hdr.F = buf[4] &  0x80; //1 bit  
	  nalu_hdr.NRI = buf[4] &  0x60; // 2 bit 
	  nalu_hdr.TYPE = buf[4] &  0x1f;// 5 bit   
	
	  if(len <= 1400) 
	  {
		    //����rtp M λ��  
		    rtp_hdr->marker=1;  
		    rtp_hdr->seq_no     = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1 
		
		    ts_current=ts_current+timestamp_increase;  
		    rtp_hdr->timestamp=htonl(ts_current);  
        
		    memcpy(&sendbuf[12],&buf[4],len);//ȥ��naluͷ��naluʣ������д��sendbuf[13]��ʼ���ַ�����
		
	      send( socketfd, sendbuf, len+12, 0 );//����rtp��	 
		
	  }
	  else if(len>1400)
	  {
		    int k=0,l=0;  
        k=len/1400;//��Ҫk��1400�ֽڵ�RTP��  
        l=len%1400;//���һ��RTP������Ҫװ�ص��ֽ���  
        int t=0;//����ָʾ��ǰ���͵��ǵڼ�����ƬRTP��  
        ts_current=ts_current+timestamp_increase;  
        rtp_hdr->timestamp=htonl(ts_current);  // There are same timestamp in one NALU 
        while(t<=k)  //there are k + l RTP packages,because index = 0 1...k
        {  
            rtp_hdr->seq_no = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1  
            if(!t)//����һ����Ҫ��Ƭ��NALU�ĵ�һ����Ƭ����FU HEADER��Sλ  
            {  
                //����rtp M λ��  
                rtp_hdr->marker=0;  
                //����FU INDICATOR,�������HEADER����sendbuf[12]  
                //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�  
                fu_ind.F=nalu_hdr.F;  
                fu_ind.NRI=nalu_hdr.NRI;  
					      /*Values equal to 28 and 29 in the Type field of the FU indicator octet
                  identify an FU-A and an FU-B, respectively
					      */
                fu_ind.TYPE=28;  
                sendbuf[12] = fu_ind.F | fu_ind.NRI | fu_ind.TYPE;
				     
                //����FU HEADER,�������HEADER����sendbuf[13]  
                //   fu_hdr =(FU_HEADER*)&sendbuf[13];  
                fu_hdr.E=0;  
                fu_hdr.R=0;  
                fu_hdr.S=1;  
                fu_hdr.TYPE=nalu_hdr.TYPE;  
                     
		            sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE; 
                nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]����nalu_payload  
                memcpy(nalu_payload,&buf[5],1400);//ȥ��NALUͷ    
                bytes=1400+14;                      //���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥ��ʼǰ׺��NALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�  
                send( socketfd, sendbuf, bytes, 0 );//����rtp��   
                t++;  
                      
                }
                else if(k==t)
                {  
                    //����һ����Ҫ��Ƭ��NALU�ķǵ�һ����Ƭ������FU HEADER��Sλ������÷�Ƭ�Ǹ�NALU�����һ����Ƭ����FU HEADER��Eλ  
                    //���͵������һ����Ƭ��ע�����һ����Ƭ�ĳ��ȿ��ܳ���1400�ֽڣ���l>1386ʱ����  
                   
                    //����rtp M λ����ǰ����������һ����Ƭʱ��λ��1  
                    rtp_hdr->marker=1;  
                    //����FU INDICATOR,�������HEADER����sendbuf[12]  
                    //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�  
                    fu_ind.F=nalu_hdr.F;    
                    fu_ind.NRI=nalu_hdr.NRI; 
                    fu_ind.TYPE=28;  
                      
				            sendbuf[12] = fu_ind.F  | fu_ind.NRI | fu_ind.TYPE; 	  
                    //����FU HEADER,�������HEADER����sendbuf[13]  
                    //   fu_hdr =(FU_HEADER*)&sendbuf[13];  
                    fu_hdr.R=0;  
                    fu_hdr.S=0;  
                    fu_hdr.TYPE=nalu_hdr.TYPE;
                    fu_hdr.E=1;  
                    sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE;
					 
					 
                    nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload  
                    memcpy(nalu_payload,&buf[t*1400+5],l);//��nalu���ʣ���l-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����  
                    bytes=l-1+14;       //���sendbuf�ĳ���,Ϊʣ��nalu�ĳ���l-1����rtp_header��FU_INDICATOR,FU_HEADER������ͷ��14�ֽ�  
                    send( socketfd, sendbuf, bytes, 0 );//����rtp��  
                    t++;  
                    //sleep(1);  
                }
                else if(t<k&&0!=t)
                {  
                    //����rtp M λ��  
                    rtp_hdr->marker=0;  
                    //����FU INDICATOR,�������HEADER����sendbuf[12]  
                    //   fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�  
                    fu_ind.F=nalu_hdr.F ; 
                    fu_ind.NRI=nalu_hdr.NRI;  
                    fu_ind.TYPE=28;  
                    sendbuf[12] = fu_ind.F  | fu_ind.NRI  | fu_ind.TYPE; 	     
                    //����FU HEADER,�������HEADER����sendbuf[13]  
                    //    fu_hdr =(FU_HEADER*)&sendbuf[13];  
                    //fu_hdr.E=0;  
                    fu_hdr.R=0;  
                    fu_hdr.S=0;  
                    fu_hdr.E=0;  
                    fu_hdr.TYPE=nalu_hdr.TYPE;  
                    sendbuf[13] = fu_hdr.S <<7 | fu_hdr.E <<6 | fu_hdr.R <<5 | fu_hdr.TYPE;
				  
                    nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload  
                    memcpy(nalu_payload,&buf[t*1400+5],1400);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����  
                    bytes=1400+14;                      //���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥԭNALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�  
                    send( socketfd, sendbuf, bytes, 0 );//����rtp��  
                                        //sleep(1);  
                    t++;  
                }  	
	      }
    }
    return 0;
}


