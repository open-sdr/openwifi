#
# openwifi iq capture and frequency offset calculation program
# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
#
# ATTENTION! If you can see the packet matched by addr1&addr2, most probably fpga and ltf FO estimation will be the same
# For those cases where fpga FO estimation is wrong, you won't see them.
# So, the value of this script is to show the correct/same FO estimation for overriding it into the receiver for performance check
# By script receiver_phase_offset_override.sh
# 
# Need these commands on board, after setup communication and know MAC addr of both sides
# insmod side_ch.ko iq_len_init=1000
# ./side_ch_ctl wh11d997
# ./side_ch_ctl wh7h635c982f
# ./side_ch_ctl wh6h44332236
# ./side_ch_ctl wh1h6001
# ./side_ch_ctl wh8d25
# ./side_ch_ctl g0
# # (In the case of totally clean/non-standard channel, long preamble detected can also be used as trigger ./side_ch_ctl wh8d8)

# On host PC
# python3 iq_capture_freq_offset.py 1000
# It will print phase_offset value: FPGA VS python
# You can override this correct vlaue to receiver via receiver_phase_offset_override.sh on board.

import os
import sys
import socket
import numpy as np
# import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

metric_plot_enable = True

# Decided by ATAN_LUT_SCALE_SHIFT in common_defs.v
# Different for master(11a/g/n) and 80211ax!!!
LUT_SIZE = 512.0 # master(11a/g/n)
# LUT_SIZE = 4096.0 # 80211ax

freq_offset_fpga_store = np.zeros(64,)
freq_offset_ltf_store = np.zeros(64,)

def phase_offset_to_freq_offset(phase_offset):
    freq_offset = (20.0e6*phase_offset/LUT_SIZE)/(2.0*3.14159265358979323846)
    return freq_offset

def plot_agc_gain(agc_gain):
    num_trans = np.shape(agc_gain)[0]

    fig_agc_gain = plt.figure(2)
    fig_agc_gain.clf()
    plt.xlabel("sample")
    plt.ylabel("gain/lock")
    # plt.title("AGC gain (blue) and lock status (red)")
    
    for i in range(num_trans):
      agc_gain_tmp = agc_gain[i,:]
      agc_gain_lock = np.copy(agc_gain_tmp)
      agc_gain_lock[agc_gain_tmp>127] = 80 # agc lock
      agc_gain_lock[agc_gain_tmp<=127] = 0 # agc not lock

      agc_gain_value = np.copy(agc_gain_tmp)
      agc_gain_value[agc_gain_tmp>127] = agc_gain_tmp[agc_gain_tmp>127] - 128
      plt.plot(agc_gain_value, label='gain')
      plt.plot(agc_gain_lock, label='lock')
      plt.legend(loc='upper right')

    plt.ylim(0, 82)
    fig_agc_gain.canvas.flush_events()

def plot_phase_offset(phase_offset_fpga, phase_offset_ltf):
    freq_offset_fpga = phase_offset_to_freq_offset(phase_offset_fpga)
    freq_offset_ltf = phase_offset_to_freq_offset(phase_offset_ltf)

    for i in range(len(phase_offset_fpga)):
      freq_offset_fpga_store[:(64-1)] = freq_offset_fpga_store[1:]
      freq_offset_fpga_store[(64-1):] = freq_offset_fpga[i]
      freq_offset_ltf_store[:(64-1)] = freq_offset_ltf_store[1:]
      freq_offset_ltf_store[(64-1):] = freq_offset_ltf[i]

    fig_fo_log = plt.figure(1)
    fig_fo_log.clf()
    plt.xlabel("capture idx")
    plt.plot(freq_offset_fpga_store, 'b.-', label='FPGA')
    plt.plot(freq_offset_ltf_store, 'r.-', label='python LTF')
    plt.legend(loc='upper right')
    fig_fo_log.canvas.flush_events()

def ltf_freq_offset_estimation(iq_capture, start_idx_demod_is_ongoing):
    num_trans = np.shape(iq_capture)[0]
    phase_offset_ltf = np.zeros(num_trans,)

    if metric_plot_enable:
      fig_metric = plt.figure(0)
      fig_metric.clf()
      plt.xlabel("sample")
    
    for i in range(num_trans):
      iq = iq_capture[i, 0:-65]
      iq_delay = iq_capture[i, 64:-1]
      # iq_delay_conj_prod = np.multiply(iq_delay, np.conj(iq))
      iq_delay_conj_prod = np.multiply(np.conj(iq_delay), iq)
      iq_delay_conj_prod_mv_sum = np.convolve(iq_delay_conj_prod, np.ones(32,), 'valid')
      iq_delay_conj_prod_mv_sum_power = np.real(np.multiply(np.conj(iq_delay_conj_prod_mv_sum),iq_delay_conj_prod_mv_sum))

      # # --------------- old method
      # iq_delay_conj_prod_power = np.real(np.multiply(np.conj(iq_delay_conj_prod),iq_delay_conj_prod))
      # iq_delay_conj_prod_power_mv_sum = np.convolve(iq_delay_conj_prod_power, np.ones(32,), 'valid')
      # metric_normalized = iq_delay_conj_prod_mv_sum_power/iq_delay_conj_prod_power_mv_sum

      # max_idx_metric_normalized = np.argmax(metric_normalized)
      # phase_offset_ltf[i] = np.angle(iq_delay_conj_prod_mv_sum[max_idx_metric_normalized])*8.0

      # # --------------- new method
      if start_idx_demod_is_ongoing[i] > 100:
        base_idx = start_idx_demod_is_ongoing[i]-100
        max_idx = np.argmax(iq_delay_conj_prod_mv_sum_power[base_idx:start_idx_demod_is_ongoing[i]])
        phase_offset_ltf[i] = np.angle(iq_delay_conj_prod_mv_sum[base_idx+max_idx])*LUT_SIZE/64.0
      else:
        phase_offset_ltf[i] = np.inf
        
      if metric_plot_enable:
        # plt.plot(metric_normalized)
        plt.plot(iq_delay_conj_prod_mv_sum_power, label='metric')

        iq_power = np.real(np.multiply(np.conj(iq),iq))
        iq_total_power = np.sum(iq_power)
        iq_power_mv_sum = np.convolve(iq_power, np.ones(32,), 'valid')
        iq_power_normalized = 500.0*iq_power_mv_sum/iq_total_power
        # plt.plot(iq_power_normalized)
        plt.plot(iq_power*1e10, label='iq power')
        plt.legend(loc='upper right')

    if metric_plot_enable:
      fig_metric.canvas.flush_events()

    return phase_offset_ltf

    # fig_iq_capture = plt.figure(0)
    # fig_iq_capture.clf()
    # plt.xlabel("sample")
    # plt.ylabel("I/Q")
    # plt.title("I (blue) and Q (red) capture")
    # plt.plot(iq_capture.real, 'b')
    # plt.plot(iq_capture.imag, 'r')
    # plt.ylim(-32767, 32767)
    # fig_iq_capture.canvas.flush_events()

    # agc_gain_lock = np.copy(agc_gain)
    # agc_gain_lock[agc_gain>127] = 80 # agc lock
    # agc_gain_lock[agc_gain<=127] = 0 # agc not lock

    # agc_gain_value = np.copy(agc_gain)
    # agc_gain_value[agc_gain>127] = agc_gain[agc_gain>127] - 128

    # fig_agc_gain = plt.figure(1)
    # fig_agc_gain.clf()
    # plt.xlabel("sample")
    # plt.ylabel("gain/lock")
    # plt.title("AGC gain (blue) and lock status (red)")
    # plt.plot(agc_gain_value, 'b')
    # plt.plot(agc_gain_lock, 'r')
    # plt.ylim(0, 82)
    # fig_agc_gain.canvas.flush_events()

    # fig_rssi_half_db = plt.figure(2)
    # fig_rssi_half_db.clf()
    # plt.xlabel("sample")
    # plt.ylabel("dB")
    # plt.title("RSSI half dB (uncalibrated)")
    # plt.plot(rssi_half_db)
    # plt.ylim(100, 270)
    # fig_rssi_half_db.canvas.flush_events()

def parse_iq(iq, iq_len):
    # print(len(iq), iq_len)
    num_dma_symbol_per_trans = 1 + iq_len
    num_int16_per_trans = num_dma_symbol_per_trans*4 # 64bit per dma symbol
    num_trans = round(len(iq)/num_int16_per_trans)
    # print(len(iq), iq.dtype, num_trans)
    iq = iq.reshape([num_trans, num_int16_per_trans])
    phase_offset_fpga = np.zeros(num_trans, dtype=np.int16)
    start_idx_demod_is_ongoing = np.zeros(num_trans, dtype=np.uint16)
    
    timestamp = iq[:,0] + pow(2,16)*iq[:,1] + pow(2,32)*iq[:,2] + pow(2,48)*iq[:,3]
    iq_capture = np.int16(iq[:,4::4]) + np.int16(iq[:,5::4])*1j
    agc_gain = np.bitwise_and(iq[:,6::4], np.uint16(0xFF))
    phase_offset_fpga_high2_mat = iq[:,7::4]
    demod_is_ongoing_mat = np.bitwise_and(phase_offset_fpga_high2_mat, np.uint16(0x8000))
    phase_offset_fpga_low7_mat = iq[:,6::4]
    for i in range(num_trans):
      # print(demod_is_ongoing_mat[i,:])
      start_idx_demod_is_ongoing_tmp = np.nonzero(demod_is_ongoing_mat[i,:])
      # print(start_idx_demod_is_ongoing_tmp[0][0])
      start_idx_demod_is_ongoing[i] = start_idx_demod_is_ongoing_tmp[0][0]
      # print(type(start_idx_demod_is_ongoing_tmp[0][0]))
      phase_offset_fpga_low7 = np.right_shift(np.bitwise_and(phase_offset_fpga_low7_mat[i,start_idx_demod_is_ongoing[i]], np.uint16(0x7F00)), 8)
      phase_offset_fpga_high2 = np.right_shift(np.bitwise_and(phase_offset_fpga_high2_mat[i,start_idx_demod_is_ongoing[i]], np.uint16(0x1800)), 4)

      sign_bit = np.bitwise_and(phase_offset_fpga_high2, np.uint16(0x100))
      phase_offset_fpga_tmp = phase_offset_fpga_high2 + phase_offset_fpga_low7 + sign_bit*2 + sign_bit*4 + sign_bit*8 + sign_bit*16 + sign_bit*32 + sign_bit*64 + sign_bit*128
      phase_offset_fpga[i] = np.int16(phase_offset_fpga_tmp)

      # # to avoid crash
      # if start_idx_demod_is_ongoing[i] < 100:
      #    start_idx_demod_is_ongoing[i] = 100

    # iq_capture = iq_capture.reshape([num_trans*iq_len,])

    return timestamp, iq_capture, phase_offset_fpga, agc_gain, start_idx_demod_is_ongoing

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

# if os.path.exists("iq.txt"):
#     os.remove("iq.txt")
# iq_fd=open('iq.txt','a')

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
        # np.savetxt(iq_fd, iq)

        timestamp, iq_capture, phase_offset_fpga, agc_gain, start_idx_demod_is_ongoing = parse_iq(iq, iq_len)
        # print(timestamp)
        phase_offset_ltf = ltf_freq_offset_estimation(iq_capture, start_idx_demod_is_ongoing)

        plot_agc_gain(agc_gain)
        plot_phase_offset(phase_offset_fpga, phase_offset_ltf)

        # freq_offset_fpga = phase_offset_to_freq_offset(phase_offset_fpga)
        print(start_idx_demod_is_ongoing)
        print(phase_offset_fpga, phase_offset_ltf)
        
        # input("Press Enter to continue...")

    except KeyboardInterrupt:
        print('User quit')
        break

print('close()')
# iq_fd.close()
sock.close()
