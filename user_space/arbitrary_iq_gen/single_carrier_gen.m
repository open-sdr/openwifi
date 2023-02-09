% Author: Xianjun Jiao (xianjun.jiao@imec.be; putaoshu@msn.com)
% SPDX-FileCopyrightText: 2023 UGent
% SPDX-License-Identifier: AGPL-3.0-or-later

function single_carrier_gen(carrier_freq, num_iq)
if exist('carrier_freq', 'var')==0 || isempty(carrier_freq)
  carrier_freq = 1e6;
end

if exist('num_iq', 'var')==0 || isempty(num_iq)
  num_iq = 512;
end

sampling_rate = 20e6;
sampling_time = 1/sampling_rate;
t = (0.3+(0:(num_iq-1))).*sampling_time;
s = exp(2.*pi.*carrier_freq.*t.*1i);

%let's use 14 bits 
real_part = round(real(s).*(2^14));
imag_part = round(imag(s).*(2^14));

filename = ['iq_single_carrier_' num2str(carrier_freq) 'Hz_' num2str(num_iq) '.txt'];
fid = fopen(filename,'w');
if fid == -1
    disp('fopen failed');
    return;
end
len = length(s);
for j=1:len
    fprintf(fid, '%d,%d\n', real_part(j), imag_part(j));
end
fclose(fid);
disp(['Saved to ' filename]); 

filename = ['iq_single_carrier_' num2str(carrier_freq) 'Hz_' num2str(num_iq) '.bin'];
fid = fopen(filename,'w');
if fid == -1
    disp('fopen error');
    return;
end

iq_int16 = [real_part; imag_part];
iq_int16 = iq_int16(:);

fwrite(fid, iq_int16, 'int16');
fclose(fid);
disp(['Saved to ' filename]); 
