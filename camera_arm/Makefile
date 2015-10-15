OBJ=camera.o v4l2.o rtp.o mfc/mfc.o 
CC=arm-linux-gcc
all: server
server: $(OBJ)
	$(CC) $^ -o $@ -lpthread 
mfc/mfc.o :
	make -C mfc all
clean:
	$(RM) *.o server
	make -C mfc clean
