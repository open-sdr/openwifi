# Paul Zanna
ccflags-y := -O0 -std=gnu99 -Wno-declaration-after-statement

wp4_p4_drv-y := wp4-p4.o wp4_runtime.o
obj-m += wp4_p4_drv.o

all:
	make -C $(KDIR) M=$(PWD) modules
	# ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
