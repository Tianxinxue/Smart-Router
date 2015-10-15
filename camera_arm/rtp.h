#ifndef _RTP_H_
#define _RTP_H_

#define UDP_DEST_IP                "192.168.1.102"
#define UDP_DEST_PORT            1234
#define H264                    96

typedef struct   
{  
    /**//* byte 0 */  
    unsigned char csrc_len:4;        /**//* expect 0 */  
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */  
    unsigned char padding:1;        /**//* expect 0 */  
    unsigned char version:2;        /**//* expect 2 */  
    /**//* byte 1 */  
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */  
    unsigned char marker:1;        /**//* expect 1 */  
    /**//* bytes 2, 3 */  
    unsigned short seq_no;              
    /**//* bytes 4-7 */  
    unsigned  long timestamp;          
    /**//* bytes 8-11 */  
    unsigned long ssrc;            /**//* stream number is used here. */  
} RTP_FIXED_HEADER;  
  
typedef struct {  
    //byte 0  
    unsigned char TYPE;// 5bit  
    unsigned char NRI;//2 bit  
    unsigned char F;//1 bit           
} NALU_HEADER; /**//* 1 BYTES */  

//Fragmentation  
typedef struct {  
    //byte 0  
    unsigned char TYPE;//:5;  
    unsigned char NRI;//:2;   
    unsigned char F;//;//:1;                  
} FU_INDICATOR; /**//* 1 BYTES */  
  
typedef struct {  
    //byte 0  
    unsigned char TYPE;//:5;  
    unsigned char R;//:1;  
    unsigned char E;//:1;  
    unsigned char S;//:1;      
} FU_HEADER; /**//* 1 BYTES */  

int UDP_init();
int RTP_send(int socketfd,char *buf,unsigned int len);
#endif
