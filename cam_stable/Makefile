OBJ=camera.o v4l2.o udp.o
CC=gcc
all: server
server: $(OBJ)
	$(CC) $^ -o $@ -lpthread 
clean:
	$(RM) *.o server
