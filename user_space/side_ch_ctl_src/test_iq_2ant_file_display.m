% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

% clear all;
% close all;
function timestamp = test_iq_2ant_file_display(iq_len, iq_cap_filename, idx_to_check)
close all;

if exist('iq_len', 'var')==0 || isempty(iq_len)
    iq_len = 8187; % default for big fpga
    % iq_len = 4095; % for small fpga
end

if exist('iq_cap_filename', 'var')==0 || isempty(iq_cap_filename)
    iq_cap_filename = 'iq_2ant.txt';
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

iq0_capture =   zeros(iq_len, num_iq_capture);
iq1_capture =   zeros(iq_len, num_iq_capture);
timestamp   =   zeros(1, num_iq_capture);

b = uint16(b);
for i=1:num_iq_capture
    sp = (i-1)*num_data_in_each_iq_capture + 1;
    ep = i*num_data_in_each_iq_capture;
    timestamp(i) = double(b(sp,1)) + (2^16)*double(b(sp,2)) + (2^32)*double(b(sp,3)) + (2^48)*double(b(sp,4));
    iq0_capture(:,i) = double(typecast(b((sp+1):ep,1),'int16')) + 1i.*double(typecast(b((sp+1):ep,2),'int16'));
    iq1_capture(:,i) = double(typecast(b((sp+1):ep,3),'int16')) + 1i.*double(typecast(b((sp+1):ep,4),'int16'));
end

mat_filename = [iq_cap_filename(1:end-4) '_' num2str(iq_len) '.mat'];
save(mat_filename, 'iq0_capture', 'iq1_capture');
disp(mat_filename);

figure; plot(timestamp,'b+-'); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet');  grid on;

figure;
subplot(2,1,1); 
plot(real(iq0_capture(:))); hold on; plot(imag(iq0_capture(:)),'r'); title('rx0 I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;
subplot(2,1,2); 
plot(real(iq1_capture(:))); hold on; plot(imag(iq1_capture(:)),'r'); title('rx1 I (blue) Q (red) sample'); xlabel('sample'); ylabel('I/Q'); grid on;

figure;
a = abs(iq0_capture(:));
b = abs(iq1_capture(:));
% a(a==0) = max(b);
plot(a); hold on;
plot(b,'r'); title('rx0 and rx1 abs'); xlabel('sample'); ylabel('abs'); grid on;
legend('rx0','rx1');

save_iq_complex_to_txt(iq0_capture(:), [mat_filename(1:end-4) '_iq0.txt']);
save_iq_complex_to_txt(iq1_capture(:), [mat_filename(1:end-4) '_iq1.txt']);

figure;
subplot(2,1,1); 
plot(real(iq0_capture(:,idx_to_check))); hold on; plot(imag(iq0_capture(:,idx_to_check)),'r'); title(['Capture idx ' num2str(idx_to_check) ' timestamp ' num2str(timestamp(idx_to_check))]); xlabel('sample'); ylabel('amplitude'); legend('I', 'Q'); grid on;
subplot(2,1,2); 
plot(real(iq1_capture(:,idx_to_check))); hold on; plot(imag(iq1_capture(:,idx_to_check)),'r'); title(['Capture idx ' num2str(idx_to_check) ' timestamp ' num2str(timestamp(idx_to_check))]); xlabel('sample'); ylabel('amplitude'); legend('I', 'Q'); grid on;

function save_iq_complex_to_txt(iq, filename)
fid = fopen(filename,'w');
if fid == -1
    disp('fopen failed');
    return;
end

for i=1:length(iq)
    fprintf(fid, '%d %d\n', round(real(iq(i))), round(imag(iq(i))));
end

fclose(fid);
