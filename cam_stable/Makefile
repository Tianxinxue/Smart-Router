OBJ=camera.o v4l2.o udp.o SsbSipH264Encode.o SsbSipLogMsg.o 
CC=arm-linux-gcc
all: server
server: $(OBJ)
	$(CC) $^ -o $@ -lpthread 
clean:
	$(RM) *.o server
