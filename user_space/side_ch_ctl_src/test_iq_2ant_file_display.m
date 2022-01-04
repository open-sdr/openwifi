% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

% clear all;
% close all;
function test_iq_2ant_file_display(varargin)

if nargin == 0
    iq_len = 8187; % default for big fpga
    % iq_len = 4095; % for small fpga
else
    iq_len = varargin{1};
end

a = load('iq_2ant.txt');
len_a = floor(length(a)/4)*4;
a = a(1:len_a);

b = reshape(a, [4, length(a)/4])';
num_data_in_each_iq_capture = 1 + iq_len;
num_iq_capture = floor(size(b,1)/num_data_in_each_iq_capture);

iq0_capture =   zeros(iq_len, num_iq_capture);
iq1_capture =   zeros(iq_len, num_iq_capture);
timestamp   =   zeros(1, num_iq_capture);

for i=1:num_iq_capture
    sp = (i-1)*num_data_in_each_iq_capture + 1;
    ep = i*num_data_in_each_iq_capture;
    timestamp(i) = b(sp,1) + (2^16)*b(sp,2) + (2^32)*b(sp,3) + (2^48)*b(sp,4);
    iq0_capture(:,i) = 1i.*b((sp+1):ep,1) + b((sp+1):ep,2);
    iq1_capture(:,i) = 1i.*b((sp+1):ep,3) + b((sp+1):ep,4);
end
save(['iq_2ant_' num2str(iq_len) '.mat'], 'iq0_capture', 'iq1_capture');

iq0_capture = iq0_capture(:);
iq1_capture = iq1_capture(:);

figure; plot(timestamp); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet');  grid on;

figure;
subplot(2,1,1); 
plot(real(iq0_capture)); hold on; plot(imag(iq0_capture),'r'); title('rx0 I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;
subplot(2,1,2); 
plot(real(iq1_capture)); hold on; plot(imag(iq1_capture),'r'); title('rx1 I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;

figure;
a = abs(iq0_capture);
b = abs(iq1_capture);
% a(a==0) = max(b);
plot(a); hold on;
plot(b,'r'); title('rx0 and rx1 abs'); xlabel('sample'); ylabel('abs'); grid on;
legend('rx0','rx1');
