% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

% clear all;
% close all;

function timestamp = test_iq_file_display(iq_len, iq_cap_filename, idx_to_check)
close all;

if exist('iq_len', 'var')==0 || isempty(iq_len)
    iq_len = 8187;
end

if exist('iq_cap_filename', 'var')==0 || isempty(iq_cap_filename)
    iq_cap_filename = 'iq.txt';
end

if exist('idx_to_check', 'var')==0 || isempty(idx_to_check)
    idx_to_check = 1;
end

a = load(iq_cap_filename);
len_a = floor(length(a)/4)*4;
a = a(1:len_a);

b = reshape(a, [4, length(a)/4])';
num_data_in_each_iq_capture = 1 + iq_len;
num_iq_capture = floor(size(b,1)/num_data_in_each_iq_capture);

iq_capture =   zeros(iq_len, num_iq_capture);
timestamp =    zeros(1, num_iq_capture);
agc_gain =     zeros(iq_len, num_iq_capture);
rssi_half_db = zeros(iq_len, num_iq_capture);

b = uint16(b);
for i=1:num_iq_capture
    sp = (i-1)*num_data_in_each_iq_capture + 1;
    ep = i*num_data_in_each_iq_capture;
    timestamp(i) = double(b(sp,1)) + (2^16)*double(b(sp,2)) + (2^32)*double(b(sp,3)) + (2^48)*double(b(sp,4));
    iq_capture(:,i) = double(typecast(b((sp+1):ep,1),'int16')) + 1i.*double(typecast(b((sp+1):ep,2),'int16'));
    // agc_gain(:,i) = double(b((sp+1):ep,3));
    agc_gain(:,i) = double(bitand(b((sp+1):ep,3), uint16(255)));
    // rssi_half_db(:,i) = double(b((sp+1):ep,4));
    rssi_half_db(:,i) = double(bitand(b((sp+1):ep,4), uint16(2047)));
end
save(['iq_' num2str(iq_len) '.mat'], 'iq_capture');

agc_gain_lock = zeros(iq_len*num_iq_capture,1);
agc_gain_lock(agc_gain(:)>127) = 1;

agc_gain_value = agc_gain(:);
agc_gain_value(agc_gain_value>127) = agc_gain_value(agc_gain_value>127) - 128;

figure; plot(timestamp,'b+-'); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet');  grid on;
figure; plot(rssi_half_db(:)); title('RSSI half dB (uncalibrated)'); xlabel('sample'); ylabel('dB'); grid on;

figure;
plot(real(iq_capture(:))); hold on; plot(imag(iq_capture(:)),'r'); title('I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;

figure; 
subplot(2,1,1); plot(agc_gain_lock); title('AGC lock status from AD9361'); xlabel('sample'); ylabel('status'); grid on;
subplot(2,1,2); plot(agc_gain_value); title('AGC gain from AD9361'); xlabel('sample'); ylabel('gain'); grid on;

figure;
agc_gain_value = reshape(agc_gain_value, [iq_len, num_iq_capture]);
agc_gain_lock  = reshape(agc_gain_lock, [iq_len, num_iq_capture]);
subplot(4,1,1); plot(real(iq_capture(:,idx_to_check))); hold on; plot(imag(iq_capture(:,idx_to_check)),'r'); title(['Capture idx ' num2str(idx_to_check) ' timestamp ' num2str(timestamp(idx_to_check))]); xlabel('sample'); ylabel('amplitude'); legend('I', 'Q'); grid on;
subplot(4,1,2); plot(rssi_half_db(:,idx_to_check)); title('RSSI half dB (uncalibrated)'); xlabel('sample'); ylabel('dB'); grid on;
subplot(4,1,3); plot(agc_gain_lock(:,idx_to_check)); title('AGC lock status from AD9361'); xlabel('sample'); ylabel('status'); grid on;
subplot(4,1,4); plot(agc_gain_value(:,idx_to_check)); title('AGC gain from AD9361'); xlabel('sample'); ylabel('gain'); grid on;
