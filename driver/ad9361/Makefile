# by Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be

ad9361_drv-y := ad9361.o ad9361_conv.o
obj-m += ad9361_drv.o

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
