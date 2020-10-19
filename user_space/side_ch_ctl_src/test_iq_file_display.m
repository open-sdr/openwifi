% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

clear all;
close all;

iq_len = 8187;

a = load('iq.txt');
len_a = floor(length(a)/4)*4;
a = a(1:len_a);

b = reshape(a, [4, length(a)/4])';
num_data_in_each_iq_capture = 1 + iq_len;
num_iq_capture = floor(size(b,1)/num_data_in_each_iq_capture);

iq_capture =   zeros(iq_len, num_iq_capture);
timestamp =    zeros(1, num_iq_capture);
agc_gain =     zeros(iq_len, num_iq_capture);
rssi_half_db = zeros(iq_len, num_iq_capture);

for i=1:num_iq_capture
    sp = (i-1)*num_data_in_each_iq_capture + 1;
    ep = i*num_data_in_each_iq_capture;
    timestamp(i) = b(sp,1) + (2^16)*b(sp,2) + (2^32)*b(sp,3) + (2^48)*b(sp,4);
    iq_capture(:,i) = b((sp+1):ep,1) + 1i.*b((sp+1):ep,2);
    agc_gain(:,i) = b((sp+1):ep,3);
    rssi_half_db(:,i) = b((sp+1):ep,4);
end
iq_capture = iq_capture(:);
agc_gain = agc_gain(:);
rssi_half_db = rssi_half_db(:);

agc_gain_lock = zeros(iq_len*num_iq_capture,1);
agc_gain_lock(agc_gain>127) = 1;

agc_gain_value = agc_gain;
agc_gain_value(agc_gain_value>127) = agc_gain_value(agc_gain_value>127) - 128;

figure; plot(timestamp); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet');  grid on;
figure; plot(rssi_half_db); title('RSSI half dB (uncalibrated)'); xlabel('sample'); ylabel('dB'); grid on;

figure;
plot(real(iq_capture)); hold on; plot(imag(iq_capture),'r'); title('I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;

figure; 
subplot(2,1,1); plot(agc_gain_lock); title('AGC lock status from AD9361'); xlabel('sample'); ylabel('status'); grid on;
subplot(2,1,2); plot(agc_gain_value); title('AGC gain from AD9361'); xlabel('sample'); ylabel('gain'); grid on;
