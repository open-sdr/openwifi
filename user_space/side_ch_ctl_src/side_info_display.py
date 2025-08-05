#
# openwifi side info receive and display program
# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
#
import os
import sys
import socket
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt

def display_side_info(freq_offset, csi, equalizer, waterfall_flag):
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
    fig_freq_offset.canvas.flush_events()

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
        unwrap_phase = np.zeros(csi_for_plot.shape)
        mid_phase = np.zeros(csi_for_plot.shape[1])
        for ci in range(csi_for_plot.shape[1]):
            unwrap_phase[:,ci] = np.unwrap(np.angle(csi_for_plot[:,ci]))
            mid_phase[ci] = unwrap_phase[csi_for_plot.shape[0]//2-1,ci]
        plt.plot(unwrap_phase-mid_phase)
        fig_csi.canvas.flush_events()

        if waterfall_flag == 1:          
            display_side_info.csi_abs_for_waterfall = np.roll(display_side_info.csi_abs_for_waterfall, 1, axis=0)
            display_side_info.csi_phase_for_waterfall = np.roll(display_side_info.csi_phase_for_waterfall, 1, axis=0)
            
            display_side_info.csi_abs_for_waterfall[0,:] = np.abs(csi[0,:])
            unwrap_phase = np.unwrap(np.angle(csi[0,:]))
            mid_phase = unwrap_phase[len(unwrap_phase)//2-1]
            display_side_info.csi_phase_for_waterfall[0,:] = unwrap_phase-mid_phase
            fig_waterfall = plt.figure(3)
            fig_waterfall.clf()
	
            ax_abs_csi_waterfall = fig_waterfall.add_subplot(121)
            ax_abs_csi_waterfall.set_title('CSI amplitude')
            ax_abs_csi_waterfall.set_xlabel("subcarrier idx")
            ax_abs_csi_waterfall.set_ylabel("time")
            ax_abs_csi_waterfall_shw = ax_abs_csi_waterfall.imshow(display_side_info.csi_abs_for_waterfall)      
            plt.colorbar(ax_abs_csi_waterfall_shw)

            ax_phase_csi_waterfall = fig_waterfall.add_subplot(122)
            ax_phase_csi_waterfall.set_title('CSI phase')
            ax_phase_csi_waterfall.set_xlabel("subcarrier idx")
            ax_phase_csi_waterfall.set_ylabel("time")
            ax_phase_csi_waterfall_shw = ax_phase_csi_waterfall.imshow(display_side_info.csi_phase_for_waterfall)
            plt.colorbar(ax_phase_csi_waterfall_shw)
            
            fig_waterfall.canvas.flush_events()

    if ( (len(equalizer)>0) and (good_row_idx>0) ):
        fig_equalizer = plt.figure(2)
        fig_equalizer.clf()
        plt.xlabel("I")
        plt.ylabel("Q")
        plt.title("equalizer")
        plt.scatter(equalizer_for_plot.real, equalizer_for_plot.imag)
        fig_freq_offset.canvas.flush_events()

def parse_side_info(side_info, num_eq):
    # print(len(side_info), num_eq, CSI_LEN, EQUALIZER_LEN, HEADER_LEN)
    CSI_LEN_HALF = round(CSI_LEN/2)
    num_dma_symbol_per_trans = HEADER_LEN + CSI_LEN + num_eq*EQUALIZER_LEN
    num_int16_per_trans = num_dma_symbol_per_trans*4 # 64bit per dma symbol
    num_trans = round(len(side_info)/num_int16_per_trans)
    # print(len(side_info), side_info.dtype, num_trans)
    side_info = side_info.reshape([num_trans, num_int16_per_trans])
    
    timestamp = side_info[:,0] + pow(2,16)*side_info[:,1] + pow(2,32)*side_info[:,2] + pow(2,48)*side_info[:,3]
    
    freq_offset = (20e6*np.int16(side_info[:,4])/512)/(2*3.14159265358979323846)

    csi = np.zeros((num_trans, CSI_LEN), dtype='int16')
    csi = csi + csi*1j
    
    equalizer = np.zeros((0,0), dtype='int16')
    if num_eq>0:
        equalizer = np.zeros((num_trans, num_eq*EQUALIZER_LEN), dtype='int16')
        equalizer = equalizer + equalizer*1j
    
    for i in range(num_trans):
        tmp_vec_i = np.int16(side_info[i,8:(num_int16_per_trans-1):4])
        tmp_vec_q = np.int16(side_info[i,9:(num_int16_per_trans-1):4])
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
sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 464) # for low latency. 464 is the minimum udp length in our case (CSI only)

# align with side_ch_control.v and all related user space, remote files
MAX_NUM_DMA_SYMBOL = 8192
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

waterfall_flag = 0
if len(sys.argv)>2:
    print("Will plot CSI in waterfall!")
    display_side_info.csi_abs_for_waterfall = np.zeros((64, CSI_LEN))
    display_side_info.csi_phase_for_waterfall = np.zeros((64, CSI_LEN))
    waterfall_flag = 1

num_dma_symbol_per_trans = HEADER_LEN + CSI_LEN + num_eq*EQUALIZER_LEN
num_byte_per_trans = 8*num_dma_symbol_per_trans

if os.path.exists("side_info.txt"):
    os.remove("side_info.txt")
side_info_fd=open('side_info.txt','a')

plt.ion()

while True:
    try:
        data, addr = sock.recvfrom(MAX_NUM_DMA_SYMBOL*8) # buffer size
        # print(addr)
        # print(len(data), num_byte_per_trans)
        test_residual = len(data)%num_byte_per_trans
        if (test_residual != 0):
            print("Abnormal length")

        side_info = np.frombuffer(data, dtype='uint16')
        np.savetxt(side_info_fd, side_info)

        timestamp, freq_offset, csi, equalizer = parse_side_info(side_info, num_eq)
        # print(timestamp)
        # print(freq_offset)
        # print(csi[0,0:10])
        # print(equalizer[0,0:10])
        display_side_info(freq_offset, csi, equalizer, waterfall_flag)

    except KeyboardInterrupt:
        print('User quit')
        break

print('close()')
side_info_fd.close()
sock.close()
