# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be

obj-m += sdr.o openofdm_rx/openofdm_rx.o openofdm_tx/openofdm_tx.o tx_intf/tx_intf.o rx_intf/rx_intf.o xpu/xpu.o

all:
	make -C $(KDIR) M=$(PWD) modules
	# ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

clean:
	rm -f *.ko *.o *.mod.o *.mod.c *.symvers *.order
