CC = arm-linux-gcc

#[Definitions]
#FPS : performance measurement. It doesn't make the output file
#ROTATE_ENABLE : Rotation mode enable when testing line buffer and ring buffer.
#                and if you want to test rotation, you must modify definition as "#define MFC_ROTATE_ENABLE 1" in 
#                MfcConfig.h file 
#RGB24BPP : display as 24bpp. default is 16bpp
#DIVX_ENABLE : test for hybrid divx decoder

CFLAGS = -Wall -Os 

CSRCS = ./encoder_test.c	\
		./SsbSipH264Encode.c	\
		./SsbSipLogMsg.c 		\
		./performance.c 		\
		./test.c 
OBJS = $(CSRCS:.c=.o)

.SUFFIXES:.c.o

all:	mfc

mfc: $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $(OBJS) -lpthread
	
.c.o:
	$(CC) $(CFLAGS) -g -c -o $@ $<

clean:
	rm -f mfc $(OBJS) 

