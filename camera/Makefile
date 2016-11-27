OBJ=camera.o v4l2.o rtp.o h264encoder.o
CC=gcc
all: server
server: $(OBJ)
	$(CC) $^ -o $@ -lpthread -lm -L./x264_lib -lx264
	
clean:
	$(RM) *.o server
