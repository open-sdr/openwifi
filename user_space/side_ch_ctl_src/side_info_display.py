#
# openwifi side info receive and display program
# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
#
import os
import sys
import socket
import numpy as np
import matplotlib.pyplot as plt

def display_side_info(freq_offset, csi, equalizer, CSI_LEN, EQUALIZER_LEN):
    if not hasattr(display_side_info, 'freq_offset_store'):
        display_side_info.freq_offset_store = np.zeros((256,))

    len_freq_offset = len(freq_offset)
    display_side_info.freq_offset_store[:(256-len_freq_offset)] = display_side_info.freq_offset_store[len_freq_offset:]
    display_side_info.freq_offset_store[(256-len_freq_offset):] = freq_offset
    
    fig_freq_offset = plt.figure(0)
    fig_freq_offset.clf()
    plt.xlabel("packet idx")
    plt.ylabel("Hz")
    plt.title("freq offset")
    plt.plot(display_side_info.freq_offset_store)
    fig_freq_offset.show()
    plt.pause(0.0001)

    good_row_idx = 0
    if ( len(equalizer)==0 ):
        csi_for_plot = csi.T
    else:
        equalizer[equalizer == 32767+32767*1j] = 0
        num_row_equalizer, num_col_equalizer = equalizer.shape
        equalizer_for_plot = np.zeros((num_row_equalizer, num_col_equalizer)) + 1j*np.zeros((num_row_equalizer, num_col_equalizer))

        num_row_csi, num_col_csi = csi.shape
        csi_for_plot = np.zeros((num_row_csi, num_col_csi)) + 1j*np.zeros((num_row_csi, num_col_csi))

        # only take out the good equalizer result, when output > 2000, it is not good
        for i in range(num_row_equalizer):
            if (not (np.any(abs(equalizer[i,:].real)>2000) or np.any(abs(equalizer[i,:].imag)>2000)) ):
                equalizer_for_plot[good_row_idx,:] = equalizer[i,:]
                csi_for_plot[good_row_idx,:] = csi[i,:]
                good_row_idx = good_row_idx + 1

        csi_for_plot = csi_for_plot[0:good_row_idx,:]
        equalizer_for_plot = equalizer_for_plot[0:good_row_idx,:]
        csi_for_plot = csi_for_plot.T
        equalizer_for_plot = equalizer_for_plot.T

    if ( (len(equalizer)==0) or ((len(equalizer)>0)and(good_row_idx>0)) ):
        fig_csi = plt.figure(1)
        fig_csi.clf()
        ax_abs_csi = fig_csi.add_subplot(211)
        ax_abs_csi.set_xlabel("subcarrier idx")
        ax_abs_csi.set_ylabel("abs")
        ax_abs_csi.set_title("CSI")
        plt.plot(np.abs(csi_for_plot))
        ax_phase_csi = fig_csi.add_subplot(212)
        ax_phase_csi.set_xlabel("subcarrier idx")
        ax_phase_csi.set_ylabel("phase")
        plt.plot(np.angle(csi_for_plot))
        fig_csi.show()
        plt.pause(0.0001)

    if ( (len(equalizer)>0) and (good_row_idx>0) ):
        fig_equalizer = plt.figure(2)
        fig_equalizer.clf()
        plt.xlabel("I")
        plt.ylabel("Q")
        plt.title("equalizer")
        plt.scatter(equalizer_for_plot.real, equalizer_for_plot.imag)
        fig_freq_offset.show()
        plt.pause(0.0001)

def parse_side_info(side_info, num_eq, CSI_LEN, EQUALIZER_LEN, HEADER_LEN):
    # print(len(side_info), num_eq, CSI_LEN, EQUALIZER_LEN, HEADER_LEN)
    CSI_LEN_HALF = round(CSI_LEN/2)
    num_dma_symbol_per_trans = HEADER_LEN + CSI_LEN + num_eq*EQUALIZER_LEN
    num_int16_per_trans = num_dma_symbol_per_trans*4 # 64bit per dma symbol
    num_trans = round(len(side_info)/num_int16_per_trans)
    # print(len(side_info), side_info.dtype, num_trans)
    side_info = side_info.reshape([num_trans, num_int16_per_trans])
    
    timestamp = side_info[:,0] + pow(2,16)*side_info[:,1] + pow(2,32)*side_info[:,2] + pow(2,48)*side_info[:,3]
    
    freq_offset = (20e6*side_info[:,4]/512)/(2*3.14159265358979323846)

    csi = np.zeros((num_trans, CSI_LEN), dtype='int16')
    csi = csi + csi*1j
    
    equalizer = np.zeros((0,0), dtype='int16')
    if num_eq>0:
        equalizer = np.zeros((num_trans, num_eq*EQUALIZER_LEN), dtype='int16')
        equalizer = equalizer + equalizer*1j
    
    for i in range(num_trans):
        tmp_vec_i = side_info[i,8:(num_int16_per_trans-1):4]
        tmp_vec_q = side_info[i,9:(num_int16_per_trans-1):4]
        tmp_vec = tmp_vec_i + tmp_vec_q*1j
        # csi[i,:] = tmp_vec[0:CSI_LEN]
        csi[i,:CSI_LEN_HALF] = tmp_vec[CSI_LEN_HALF:CSI_LEN]
        csi[i,CSI_LEN_HALF:] = tmp_vec[0:CSI_LEN_HALF]
        if num_eq>0:
            equalizer[i,:] = tmp_vec[CSI_LEN:(CSI_LEN+num_eq*EQUALIZER_LEN)]
        # print(i, len(tmp_vec), len(tmp_vec[0:CSI_LEN]), len(tmp_vec[CSI_LEN:(CSI_LEN+num_eq*EQUALIZER_LEN)]))

    return timestamp, freq_offset, csi, equalizer

UDP_IP = "192.168.10.1" #Local IP to listen
UDP_PORT = 4000         #Local port to listen

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

# align with side_ch_control.v and all related user space, remote files
CSI_LEN = 56 # length of single CSI
EQUALIZER_LEN = (56-4) # for non HT, four {32767,32767} will be padded to achieve 52 (non HT should have 48)
HEADER_LEN = 2 # timestamp and frequency offset

if len(sys.argv)<2:
    print("Assume num_eq = 8!")
    num_eq = 8
else:
    num_eq = int(sys.argv[1])
    print(num_eq)
    # print(type(num_eq))

num_dma_symbol_per_trans = HEADER_LEN + CSI_LEN + num_eq*EQUALIZER_LEN
num_byte_per_trans = 8*num_dma_symbol_per_trans

if os.path.exists("side_info.txt"):
    os.remove("side_info.txt")
side_info_fd=open('side_info.txt','a')

while True:
    try:
        data, addr = sock.recvfrom(32768) # buffer size
        # print(addr)
        print(len(data), num_byte_per_trans)
        test_residual = len(data)%num_byte_per_trans
        if (test_residual != 0):
            print("Abnormal length")

        side_info = np.frombuffer(data, dtype='int16')
        np.savetxt(side_info_fd, side_info)

        timestamp, freq_offset, csi, equalizer = parse_side_info(side_info, num_eq, CSI_LEN, EQUALIZER_LEN, HEADER_LEN)
        print(timestamp)
        # print(freq_offset)
        # print(csi[0,0:10])
        # print(equalizer[0,0:10])
        display_side_info(freq_offset, csi, equalizer, CSI_LEN, EQUALIZER_LEN)

    except KeyboardInterrupt:
        print('User quit')
        break

print('close()')
side_info_fd.close()
sock.close()
