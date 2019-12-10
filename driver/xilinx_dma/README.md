Currently used driver xilinx_dma-orig.c is based on 552d3f11e374ca0d435aa93a571507819eabdda2 of https://github.com/Xilinx/linux-xlnx )

instruction to generate our customized xilinx dma driver:

./make_xilinx_dma.sh

instruction to generate our customized xilinx dma test program:

./make_xilinx_dma_test.sh

test dma driver on board: login to zc706, then:

    rm axidmatest.ko
    wget ftp://192.168.10.1/driver/xilinx_dma/axidmatest.ko
    rm ddc.ko
    wget ftp://192.168.10.1/driver/ddc/ddc.ko
    rm xilinx_dma.ko
    wget ftp://192.168.10.1/driver/xilinx_dma/xilinx_dma.ko
    rmmod axidmatest
    rmmod ddc
    rmmod xilinx_dma
    insmod xilinx_dma.ko
    insmod ddc.ko
    insmod axidmatest.ko
    dmesg -c

dmesg will show test result printed by "insmod axidmatest.ko". Like this:

    root@analog:~#     dmesg -c
    xilinx_dmatest: dropped channel dma5chan0
    xilinx_dmatest: dropped channel dma5chan1

    sdr,ddc dev_remove base_addr 0xf14e0000
    sdr,ddc dev_remove ddc_driver_api_inst 0xbf032284
    sdr,ddc dev_remove             ddc_api 0xbf032284
    sdr,ddc dev_remove succeed!
    xilinx-vdma 43000000.axivdma: Xilinx AXI VDMA Engine Driver Probed!!
    xilinx-vdma 80400000.dma: Xilinx AXI DMA Engine Driver Probed!!
    xilinx-vdma 80410000.dma: Xilinx AXI DMA Engine Driver Probed!!

    sdr,ddc dev_probe match!
    sdr,ddc dev_probe io start 0x83c20000 end 0x83c2ffff name /fpga-axi@0/rx_intf@83c20000 flags 0x00000200 desc 0x00000000
    sdr,ddc dev_probe base_addr 0xf18e0000
    sdr,ddc dev_probe ddc_driver_api_inst 0xbf0e1284
    sdr,ddc dev_probe             ddc_api 0xbf0e1284
    sdr,ddc dev_probe reset tsf timer
    sdr,ddc dev_probe tsf timer runtime read 1 33007 100015us
    sdr,ddc dev_probe succeed!
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    dmatest: Started 1 threads using dma5chan0 dma5chan1
    align 3
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    tx_tmo 99 status 0 len 6448 DMA_COMPLETE 0
    dma5chan0-dma5c: verifying source buffer...
    dma5chan0-dma5c: verifying dest buffer...
    dma5chan0-dma5c: #0: No errors with 
    src_off=0x448 dst_off=0x568 len=0x1930
    align 3
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    tx_tmo 100 status 0 len 3248 DMA_COMPLETE 0
    dma5chan0-dma5c: verifying source buffer...
    dma5chan0-dma5c: verifying dest buffer...
    dma5chan0-dma5c: #1: No errors with 
    src_off=0x458 dst_off=0xf08 len=0xcb0
    align 3
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    tx_tmo 100 status 0 len 8112 DMA_COMPLETE 0
    dma5chan0-dma5c: verifying source buffer...
    dma5chan0-dma5c: verifying dest buffer...
    dma5chan0-dma5c: #2: No errors with 
    src_off=0x10 dst_off=0x20 len=0x1fb0
    align 3
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    tx_tmo 100 status 0 len 840 DMA_COMPLETE 0
    dma5chan0-dma5c: verifying source buffer...
    dma5chan0-dma5c: verifying dest buffer...
    dma5chan0-dma5c: #3: No errors with 
    src_off=0x1890 dst_off=0x1268 len=0x348
    align 3
    sdr,ddc hw_init mode 0
    sdr,ddc hw_init mode DDC_AXIS_LOOP_BACK
    sdr,ddc hw_init err 0
    tx_tmo 100 status 0 len 7816 DMA_COMPLETE 0
    dma5chan0-dma5c: verifying source buffer...
    dma5chan0-dma5c: verifying dest buffer...
    dma5chan0-dma5c: #4: No errors with 
    src_off=0x80 dst_off=0x168 len=0x1e88
    dma5chan0-dma5c: terminating after 5 tests, 0 failures (status 0)

