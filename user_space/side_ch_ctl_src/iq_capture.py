#
# openwifi side info receive and display program
# Xianjun jiao. putaoshu@msn.com; xianjun.jiao@imec.be
#
import os
import sys
import socket
import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

def display_iq(iq_capture, agc_gain, rssi_half_db, ch_idle, demod, tx_rf, fcs_ok):
   
    fig_iq_capture = plt.figure(0)
    fig_iq_capture.clf()
    plt.xlabel("sample")
    # plt.ylabel("I/Q")
    # plt.title("I (blue) and Q (red) capture")
    plt.plot(iq_capture.real, 'b')
    plt.plot(iq_capture.imag, 'r')
    plt.plot(30000+fcs_ok*3000, 'k-', label='fcs_ok')
    plt.plot(27000+demod*3000, 'r--', label='demod')
    plt.plot(-30000+tx_rf*3000, 'g-', label='tx_rf')
    plt.plot(-33000+ch_idle*3000, 'b--', label='ch_idle')
    plt.ylim(-34000, 34000)
    plt.legend(loc='upper right')
    plt.grid()
    fig_iq_capture.canvas.flush_events()

    agc_gain_lock = np.copy(agc_gain)
    agc_gain_lock[agc_gain>127] = 80 # agc lock
    agc_gain_lock[agc_gain<=127] = 0 # agc not lock

    agc_gain_value = np.copy(agc_gain)
    agc_gain_value[agc_gain>127] = agc_gain[agc_gain>127] - 128

    fig_agc_gain = plt.figure(1)
    fig_agc_gain.clf()
    plt.xlabel("sample")
    plt.ylabel("gain/lock")
    # plt.title("AGC gain (blue) and lock status (red)")
    plt.plot(agc_gain_value, 'b', label='gain')
    plt.plot(agc_gain_lock, 'r', label='lock')
    plt.ylim(0, 82)
    plt.legend(loc='upper right')
    fig_agc_gain.canvas.flush_events()

    fig_rssi_half_db = plt.figure(2)
    fig_rssi_half_db.clf()
    plt.xlabel("sample")
    plt.ylabel("dB")
    plt.title("RSSI half dB (uncalibrated)")
    plt.plot(rssi_half_db)
    plt.ylim(100, 270)
    fig_rssi_half_db.canvas.flush_events()

def parse_iq(iq, iq_len):
    # print(len(iq), iq_len)
    num_dma_symbol_per_trans = 1 + iq_len
    num_int16_per_trans = num_dma_symbol_per_trans*4 # 64bit per dma symbol
    num_trans = round(len(iq)/num_int16_per_trans)
    # print(len(iq), iq.dtype, num_trans)
    iq = iq.reshape([num_trans, num_int16_per_trans])
    
    timestamp = iq[:,0] + pow(2,16)*iq[:,1] + pow(2,32)*iq[:,2] + pow(2,48)*iq[:,3]
    iq_capture = np.int16(iq[:,4::4]) + np.int16(iq[:,5::4])*1j
    agc_gain = np.bitwise_and(iq[:,6::4], np.uint16(0xFF))
    rssi_half_db = np.bitwise_and(iq[:,7::4], np.uint16(0x7FF))
    # print(num_trans, iq_len, iq_capture.shape, agc_gain.shape, rssi_half_db.shape)

    ch_idle = np.right_shift(np.bitwise_and(iq[:,6::4], np.uint16(0x8000)), 15)
    demod = np.right_shift(np.bitwise_and(iq[:,7::4], np.uint16(0x8000)), 15)
    tx_rf = np.right_shift(np.bitwise_and(iq[:,7::4], np.uint16(0x4000)), 14)
    fcs_ok = np.right_shift(np.bitwise_and(iq[:,7::4], np.uint16(0x2000)), 13)

    # iq_capture = iq_capture.reshape([num_trans*iq_len,])
    # agc_gain = agc_gain.reshape([num_trans*iq_len,])
    # rssi_half_db = rssi_half_db.reshape([num_trans*iq_len,])

    return timestamp, iq_capture, agc_gain, rssi_half_db, ch_idle, demod, tx_rf, fcs_ok

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

if os.path.exists("iq.txt"):
    os.remove("iq.txt")
iq_fd=open('iq.txt','a')

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

        timestamp, iq_capture, agc_gain, rssi_half_db, ch_idle, demod, tx_rf, fcs_ok = parse_iq(iq, iq_len)
        print(timestamp)
        display_iq(iq_capture[0,:], agc_gain[0,:], rssi_half_db[0,:], ch_idle[0,:], demod[0,:], tx_rf[0,:], fcs_ok[0,:])
        # plt.waitforbuttonpress()

    except KeyboardInterrupt:
        print('User quit')
        break

print('close()')
iq_fd.close()
sock.close()
