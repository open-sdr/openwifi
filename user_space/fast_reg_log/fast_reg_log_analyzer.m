% Author: Xianjun Jiao (xianjun.jiao@imec.be; putaoshu@msn.com)
% SPDX-FileCopyrightText: 2023 UGent
% SPDX-License-Identifier: AGPL-3.0-or-later

function fast_reg_log_analyzer(filename_bin, start_idx, end_idx)
close all;

% if exist('start_idx', 'var')==0 || isempty(start_idx)
%   start_idx = 1;
% end
% 
% if exist('end_idx', 'var')==0 || isempty(end_idx)
%   end_idx = 65536;
% end

filename_csv = [filename_bin(1:(end-3)) 'csv'];
disp(['Human readable fast reg log will be in ' filename_csv]);

fid = fopen(filename_bin);
if fid == -1
    disp('fopen failed!');
    return;
end

a = fread(fid, inf, 'uint32');
fclose(fid);
% a = bitand(uint32(a), uint32(268435455));
% plot(a(1:2:end)); hold on;
% plot(a(2:2:end));
% legend('1', '2');

a = uint32(a);
tsf = a(2:2:end);
% plot(tsf);
state = a(1:2:end);

% find out overflow idx
overflow_idx = find(diff([0; double(tsf)])<0, 1, 'first');
% overflow_idx
if ~isempty(overflow_idx)
    tsf(overflow_idx:end) = tsf(overflow_idx:end) + (2^32);
    disp(num2str(overflow_idx));
end

rssi_correction = 145;
rssi_half_db = double(bitand(bitshift(state, 0), uint32((2^11)-1)));
agc_lock = 1 - double(bitand(bitshift(state, -11), uint32(1)));
demod_is_ongoing = double(bitand(bitshift(state, -12), uint32(1)));
tx_is_ongoing = double(bitand(bitshift(state, -13), uint32(1)));
ch_idle = 1 - double(bitand(bitshift(state, -14), uint32(1)));
iq_rssi_half_db = double(bitand(bitshift(state, -16), uint32((2^9)-1)));
agc_gain = double(bitand(bitshift(state, -25), uint32((2^7)-1)));

rssi_dbm = (rssi_half_db./2) - rssi_correction;

figure;
subplot(2,1,1);
plot(tsf, -rssi_dbm, 'r+-'); hold on;
plot(tsf, iq_rssi_half_db, 'bo-');
plot(tsf, agc_gain, 'ks-');
legend('rssi dbm', 'iq rssi half db', 'agc gain');
subplot(2,1,2);
plot(tsf, agc_lock+0); hold on;
plot(tsf, demod_is_ongoing+2);
plot(tsf, tx_is_ongoing+4);
plot(tsf, ch_idle+6);

legend('agc lock', 'demod is ongoing', 'tx is ongoing', 'ch idle');

a=table(tsf, rssi_half_db, rssi_dbm, iq_rssi_half_db, agc_gain, agc_lock, demod_is_ongoing, tx_is_ongoing, ch_idle);
writetable(a, filename_csv);
