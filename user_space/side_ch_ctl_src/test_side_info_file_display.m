% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

function [timestamp, csi, freq_offset, equalizer_out] = test_side_info_file_display(num_eq, side_info_filename, idx_to_check)
close all;

if exist('num_eq', 'var')==0 || isempty(num_eq)
    num_eq = 8;
end

if exist('side_info_filename', 'var')==0 || isempty(side_info_filename)
    side_info_filename = 'side_info.txt';
end

if exist('idx_to_check', 'var')==0 || isempty(idx_to_check)
    idx_to_check = 1;
end

a = load(side_info_filename);
len_a = floor(length(a)/4)*4;
a = a(1:len_a);

b = reshape(a, [4, length(a)/4])';
num_data_in_each_side_info = 2+56+num_eq*52;
num_side_info = floor(size(b,1)/num_data_in_each_side_info);

side_info = zeros(num_data_in_each_side_info, num_side_info);
timestamp = uint64(zeros(1, num_side_info));
freq_offset = zeros(1, num_side_info);
csi = zeros(56, num_side_info);
equalizer = zeros(num_eq*52, num_side_info);

b = uint16(b);
for i=1:num_side_info
    sp = (i-1)*num_data_in_each_side_info + 1;
    ep = i*num_data_in_each_side_info;
    timestamp(i) = uint64(b(sp,1)) + (2^16)*uint64(b(sp,2)) + (2^32)*uint64(b(sp,3)) + (2^48)*uint64(b(sp,4));
    freq_offset(i) = (20e6*double(typecast(b(sp+1,1),'int16'))/512)/(2*pi);
    side_info(:,i) = double(typecast(b(sp:ep,1),'int16')) + 1i.*double(typecast(b(sp:ep,2),'int16'));
    csi(:,i) = side_info(3:58,i);
    equalizer(:,i) = side_info(59:end,i);
end
equalizer_out = equalizer;

csi = [csi(29:end,:); csi(1:28,:)];
equalizer = equalizer(:);
equalizer(equalizer == 32767+1i*32767) = NaN;

subplot(2,1,1); plot(abs(csi)); title('CSI'); ylabel('abs'); grid on;
subplot(2,1,2); plot(angle(csi)); ylabel('phase'); xlabel('subcarrier'); grid on;

if ~isempty(equalizer)
    scatterplot(equalizer); grid on;
end

figure; plot(timestamp,'b+-'); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet');  grid on;
figure; plot(freq_offset); title('freq offset (Hz)'); ylabel('Hz'); xlabel('packet'); grid on;

figure;
subplot(2,1,1); plot(abs(csi(:,idx_to_check))); title('CSI'); ylabel('abs'); grid on; title(['Capture idx ' num2str(idx_to_check) ' timestamp ' num2str(timestamp(idx_to_check))]);
subplot(2,1,2); plot(angle(csi(:,idx_to_check))); ylabel('phase'); xlabel('subcarrier'); grid on;
