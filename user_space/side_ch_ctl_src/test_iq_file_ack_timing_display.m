% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

% clear all;
% close all;

function timestamp = test_iq_file_ack_timing_display(varargin)
close all;
idx_ack_show = 1;
if nargin == 0
    iq_len = 8187;
elseif nargin == 1
    iq_len = varargin{1};
elseif nargin == 2
    iq_len = varargin{1};
    idx_ack_show = varargin{2};
end

if iq_len < 4096 % small FPGA
  event_base_idx_reduce = 2048;
else
  event_base_idx_reduce = 0;
end

a = load('iq.txt');
len_a = floor(length(a)/4)*4;
a = a(1:len_a);

b = reshape(a, [4, length(a)/4])';
num_data_in_each_iq_capture = 1 + iq_len;
num_iq_capture = floor(size(b,1)/num_data_in_each_iq_capture);

iq_capture     = zeros(iq_len, num_iq_capture);
timestamp      = zeros(1, num_iq_capture);
agc_gain_lock  = zeros(iq_len, num_iq_capture);
agc_gain_value = zeros(iq_len, num_iq_capture);
rssi_half_db   = zeros(iq_len, num_iq_capture);

% new added for ack timing test, etc.
tx_rf_is_ongoing     = zeros(iq_len, num_iq_capture);
tx_control_state     = zeros(iq_len, num_iq_capture);
pkt_header_and_fcs_strobe = zeros(iq_len, num_iq_capture);
pkt_header_and_fcs_ok     = zeros(iq_len, num_iq_capture);
frame_type       = zeros(iq_len, num_iq_capture);
frame_subtype    = zeros(iq_len, num_iq_capture);

tx_ack_gap_us = -ones(1, num_iq_capture);
rx_ack_gap_us = -ones(1, num_iq_capture);

tx_ack_gap_sp = ones(1, num_iq_capture);
rx_ack_gap_sp = ones(1, num_iq_capture);

tx_ack_gap_ep = ones(1, num_iq_capture);
rx_ack_gap_ep = ones(1, num_iq_capture);

b = uint16(b);
for i=1:num_iq_capture
    sp = (i-1)*num_data_in_each_iq_capture + 1;
    ep = i*num_data_in_each_iq_capture;
    timestamp(i) = double(b(sp,1)) + (2^16)*double(b(sp,2)) + (2^32)*double(b(sp,3)) + (2^48)*double(b(sp,4));
    iq_capture(:,i) = double(typecast(b((sp+1):ep,1),'int16')) + 1i.*double(typecast(b((sp+1):ep,2),'int16'));
%     agc_gain(:,i) = b((sp+1):ep,3);
    agc_gain_lock(:,i)  = double(bitand(bitshift(b((sp+1):ep,3),-7), uint16(1  )));
    agc_gain_value(:,i) = double(bitand(         b((sp+1):ep,3)    , uint16(127)));
%     rssi_half_db(:,i) = b((sp+1):ep,4);
    rssi_half_db(:,i)   = double(bitand(b((sp+1):ep,4), uint16(2047)));
    
    % new added for ack timing test, etc.
    tx_rf_is_ongoing(:,i)  = double(bitand(bitshift(b((sp+1):ep,4),-15), uint16(1 )));
    tx_control_state(:,i)  = double(bitand(bitshift(b((sp+1):ep,4),-11), uint16(15)));
    
    pkt_header_and_fcs_strobe(:,i)  = double(bitand(bitshift(b((sp+1):ep,3),-15), uint16(1 )));
    pkt_header_and_fcs_ok(:,i)      = double(bitand(bitshift(b((sp+1):ep,3),-14), uint16(1 )));
    frame_subtype(:,i)     = double(bitand(bitshift(b((sp+1):ep,3),-10), uint16(15)));
    frame_type(:,i)        = double(bitand(bitshift(b((sp+1):ep,3), -8), uint16(3)));
    
    % %------------ detect if there is an rx ack event
    event_base_idx = 4096 - event_base_idx_reduce; %when the trigger is 6 -- RECV_ACK
    idx_rx_ack_event = find(tx_control_state((event_base_idx-60):(event_base_idx+60),i)==6, 1, 'first');%RECV_ACK
    if isempty(idx_rx_ack_event)
        event_base_idx = 5074 - event_base_idx_reduce; %when the trigger is 5 -- RECV_ACK_WAIT_SIG_VALID
        idx_rx_ack_event = find(tx_control_state((event_base_idx-60):(event_base_idx+60),i)==5, 1, 'first');%sRECV_ACK_WAIT_SIG_VALID
    end
    if ~isempty(idx_rx_ack_event)
%         if idx_rx_ack_event >= 5074-60 && idx <= 5074+60
            iq_sp = (event_base_idx-60-1) + idx_rx_ack_event - 585 - 320 - 585;
            iq_ep = (event_base_idx-60-1) + idx_rx_ack_event;
            [rx_ack_gap_us(i), rx_ack_gap_sp(i), rx_ack_gap_ep(i)] = extract_gap_from_iq_of_two_pkts(iq_capture(iq_sp:iq_ep,i));
            if rx_ack_gap_sp(i) ~= 1
                rx_ack_gap_sp(i) = rx_ack_gap_sp(i) + iq_sp-1;
            end
            if rx_ack_gap_ep(i) ~= 1
                rx_ack_gap_ep(i) = rx_ack_gap_ep(i) + iq_sp-1;
            end
%         end
    end
    
    % % ----------- detect if there is an tx ack event
    event_base_idx = 4096 - event_base_idx_reduce; %when the trigger is 2/3 -- SEND_DFL_ACK/SEND_BLK_ACK
    idx_tx_ack_event = find(tx_control_state((event_base_idx-60):(event_base_idx+60),i)==2, 1, 'first');%sending normal ACK
    if isempty(idx_tx_ack_event)
        idx_tx_ack_event = find(tx_control_state((event_base_idx-60):(event_base_idx+60),i)==3, 1, 'first');%sending block ACK
    end
    if ~isempty(idx_tx_ack_event)
%         if idx >= 4097-60 && idx <= 4097+60
            iq_sp = (event_base_idx-60-1) + idx_tx_ack_event - (320-70) - 585;
            iq_ep = (event_base_idx-60-1) + idx_tx_ack_event + 70 + 585;
            [tx_ack_gap_us(i), tx_ack_gap_sp(i), tx_ack_gap_ep(i)] = extract_gap_from_iq_of_two_pkts(iq_capture(iq_sp:iq_ep,i));
            if tx_ack_gap_sp(i) ~= 1
                tx_ack_gap_sp(i) = tx_ack_gap_sp(i) + iq_sp-1;
            end
            if tx_ack_gap_ep(i) ~= 1
                tx_ack_gap_ep(i) = tx_ack_gap_ep(i) + iq_sp-1;
            end
%         end
    end
    
    if isempty(idx_rx_ack_event) && isempty(idx_tx_ack_event)
        disp(['WARNING! CAN NOT find valid Rx/Tx ACK event in capture' num2str(i)]);
    end
end

figure; plot(timestamp,'b+-'); title('time stamp (TSF value)'); ylabel('us'); xlabel('packet idx');  grid on;
figure; plot(rssi_half_db(:,idx_ack_show)); title(['RSSI half dB (uncalibrated). Cap idx' num2str(idx_ack_show)]); xlabel('sample idx'); ylabel('dB'); grid on;

figure;
subplot(3,1,1); plot(real(iq_capture(:,idx_ack_show))); hold on; plot(imag(iq_capture(:,idx_ack_show)),'r'); title(['I (blue) Q (red) sample. Cap idx' num2str(idx_ack_show)]); xlabel('sample idx'); ylabel('I/Q'); grid on;
plot([rx_ack_gap_sp(idx_ack_show) rx_ack_gap_sp(idx_ack_show)],[-40000 40000], 'k'); plot([rx_ack_gap_ep(idx_ack_show) rx_ack_gap_ep(idx_ack_show)],[-40000 40000], 'k'); xlabel('sample idx');
plot([tx_ack_gap_sp(idx_ack_show) tx_ack_gap_sp(idx_ack_show)],[-40000 40000], 'k'); plot([tx_ack_gap_ep(idx_ack_show) tx_ack_gap_ep(idx_ack_show)],[-40000 40000], 'k'); xlabel('sample idx');
subplot(3,1,2);
plot(tx_control_state(:,idx_ack_show),'k.-'); hold on; grid on;
plot(7+tx_rf_is_ongoing(:,idx_ack_show),'r.-');
plot(9+pkt_header_and_fcs_strobe(:,idx_ack_show),'g.-');
plot(11+pkt_header_and_fcs_ok(:,idx_ack_show), 'b.-');
legend('tx control state','tx rf is ongoing','pkt header and fcs strobe','pkt header and fcs ok');
title(['Internal control signal. Cap idx' num2str(idx_ack_show)]);
subplot(3,1,3);
plot(frame_type(:,idx_ack_show),'k.-'); hold on; grid on;
plot(4+frame_subtype(:,idx_ack_show), 'r.-');
legend('frame type','frame subtype');
title(['Frame type, subtype. Cap idx' num2str(idx_ack_show)]);

figure; 
subplot(2,1,1); plot(agc_gain_lock(:,idx_ack_show)); title(['AGC lock status from AD9361. Cap idx' num2str(idx_ack_show)]); xlabel('sample idx'); ylabel('status'); grid on;
subplot(2,1,2); plot(agc_gain_value(:,idx_ack_show)); title(['AGC gain from AD9361. Cap idx' num2str(idx_ack_show)]); xlabel('sample idx'); ylabel('gain'); grid on;

figure; plot(rx_ack_gap_us, 'bo'); grid on; hold on; 
plot(ones(1,num_iq_capture).*mean(rx_ack_gap_us(rx_ack_gap_us~=-1)),'b'); title('Rx ACK GAP'); xlabel('Cap idx'); ylabel('us');
figure; plot(tx_ack_gap_us, 'ro'); grid on; hold on; 
plot(ones(1,num_iq_capture).*mean(tx_ack_gap_us(tx_ack_gap_us~=-1)),'r'); title('Tx ACK GAP'); xlabel('Cap idx'); ylabel('us');

% save(['iq_' num2str(iq_len) '_ack_timing.mat'], 'iq_capture');
save(['iq_' num2str(iq_len) '.mat'], 'iq_capture');

function [gap, gap_sp, gap_ep] = extract_gap_from_iq_of_two_pkts(iq)
iq = iq - mean(iq);

num_sample_power_window = 17;
power_ratio = 100;
power_sum_2us_before = zeros(1, length(iq));
iq_extend = [iq(1:num_sample_power_window); iq];
for i = 1 : length(iq)
    power_iq_tmp = iq_extend(i:(i+num_sample_power_window)) - mean(iq_extend(i:(i+num_sample_power_window)));% DC removal -- sometimes at the beginning phase of AGC, there is DC, which has strong power. But it is not signal!
    power_sum_2us_before(i) = real(power_iq_tmp'*power_iq_tmp);
end

power_sum_2us_after = zeros(1, length(iq));
iq_extend = [iq; iq((end-39):end)];
for i = 1 : length(iq)
    power_iq_tmp = iq_extend(i:(i+num_sample_power_window)) - mean(iq_extend(i:(i+num_sample_power_window)));
    power_sum_2us_after(i) = real(power_iq_tmp'*power_iq_tmp);
end

power_sum_diff = power_sum_2us_before./power_sum_2us_after;

gap = -1;
gap_ep = 1;
gap_sp = find(power_sum_diff>=power_ratio, 1, 'first');
if ~isempty(gap_sp)
    gap_ep = find(power_sum_diff<=(1/power_ratio), 1, 'last');
    if ~isempty(gap_ep)
        gap = (gap_ep - gap_sp)./20;
    else
        gap_ep = 1;
        disp('WARNING! CAN NOT find the end of ACK GAP!');
    end
else
    gap_sp = 1;
    disp('WARNING! CAN NOT find the beginning of ACK GAP!');
end

