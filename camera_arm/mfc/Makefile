objs := performance.o SsbSipH264Encode.o SsbSipLogMsg.o 
all : $(objs)
	arm-linux-ld -r -o mfc.o $^	
%.o : %.c
	arm-linux-gcc -c $^	
clean:
	rm -f *.o 	