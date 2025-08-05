#
# openwifi side info receive and display program
# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
#
import os
import sys
import socket
import numpy as np
import matplotlib.pyplot as plt

def display_iq(iq0_capture, iq1_capture):
    fig_iq_capture = plt.figure(0)
    fig_iq_capture.clf()

    ax_iq0 = fig_iq_capture.add_subplot(211)
    # ax_iq0.set_xlabel("sample")
    ax_iq0.set_ylabel("I/Q")
    ax_iq0.set_title("rx0 I/Q")
    plt.plot(iq0_capture.real)
    plt.plot(iq0_capture.imag)
    plt.ylim(-32767, 32767)

    ax_iq1 = fig_iq_capture.add_subplot(212)
    ax_iq1.set_xlabel("sample")
    ax_iq1.set_ylabel("I/Q")
    ax_iq1.set_title("rx1 I/Q")
    plt.plot(iq1_capture.real)
    plt.plot(iq1_capture.imag)
    plt.ylim(-32767, 32767)
    fig_iq_capture.canvas.flush_events()

def parse_iq(iq, iq_len):
    # print(len(iq), iq_len)
    num_dma_symbol_per_trans = 1 + iq_len
    num_int16_per_trans = num_dma_symbol_per_trans*4 # 64bit per dma symbol
    num_trans = round(len(iq)/num_int16_per_trans)
    # print(len(iq), iq.dtype, num_trans)
    iq = iq.reshape([num_trans, num_int16_per_trans])
    
    timestamp = iq[:,0] + pow(2,16)*iq[:,1] + pow(2,32)*iq[:,2] + pow(2,48)*iq[:,3]
    iq0_capture = np.int16(iq[:,4::4]) + np.int16(iq[:,5::4])*1j
    iq1_capture = np.int16(iq[:,6::4]) + np.int16(iq[:,7::4])*1j
    # print(num_trans, iq_len, iq0_capture.shape, iq1_capture.shape)

    # iq0_capture = iq0_capture.reshape([num_trans*iq_len,])
    # iq1_capture = iq1_capture.reshape([num_trans*iq_len,])
    iq0_capture = np.transpose(iq0_capture)
    iq1_capture = np.transpose(iq1_capture)

    return timestamp, iq0_capture, iq1_capture

UDP_IP = "192.168.10.1" #Local IP to listen
UDP_PORT = 4000         #Local port to listen

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))
sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 464) # for low latency. 464 is the minimum udp length in our case (CSI only)

# align with side_ch_control.v and all related user space, remote files
MAX_NUM_DMA_SYMBOL = 8192

if len(sys.argv)<2:
    print("Assume iq_len = 8187! (Max UDP 65507 bytes; (65507/8)-1 = 8187)")
    iq_len = 8187
else:
    iq_len = int(sys.argv[1])
    print(iq_len)
    # print(type(num_eq))

if iq_len>8187:
    iq_len = 8187
    print('Limit iq_len to 8187! (Max UDP 65507 bytes; (65507/8)-1 = 8187)')

num_dma_symbol_per_trans = 1 + iq_len
num_byte_per_trans = 8*num_dma_symbol_per_trans

if os.path.exists("iq_2ant.txt"):
    os.remove("iq_2ant.txt")
iq_fd=open('iq_2ant.txt','a')

plt.ion()

while True:
    try:
        data, addr = sock.recvfrom(MAX_NUM_DMA_SYMBOL*8) # buffer size
        # print(addr)
        test_residual = len(data)%num_byte_per_trans
        # print(len(data)/8, num_dma_symbol_per_trans, test_residual)
        if (test_residual != 0):
            print("Abnormal length")

        iq = np.frombuffer(data, dtype='uint16')
        np.savetxt(iq_fd, iq)

        timestamp, iq0_capture, iq1_capture = parse_iq(iq, iq_len)
        display_iq(iq0_capture, iq1_capture)
        # print(timestamp, max(max(iq0_capture.real)), max(max(iq1_capture.real)))
        tmp0 = np.asmatrix(iq0_capture)
        tmp1 = np.asmatrix(iq1_capture)
        print(timestamp, tmp0.max(), tmp1.max())

    except KeyboardInterrupt:
        print('User quit')
        break

print('close()')
iq_fd.close()
sock.close()
