# Paul Zanna

obj-m += wp4-p4.o

all:
	make -C $(KDIR) M=$(PWD) modules
	# ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
