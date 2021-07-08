% Xianjun Jiao. xianjun.jiao@imec.be; putaoshu@msn.com

function save_iq_to_txt_for_verilog_sim(mat_filename, varargin)
a = load(mat_filename);
var_names = fieldnames(a);
var_cells = struct2cell(a);
[len_iq, num_frame] = size(var_cells{1});

if nargin>=2
    idx_set = varargin{1};
else
    idx_set = 1:num_frame;
end

if nargin >= 3
    sp = varargin{2}(1);
    ep = varargin{2}(2);
else
    sp = 1;
    ep = len_iq;
end

for name_idx = 1 : length(var_names)
    filename_txt = [var_names{name_idx} '.txt'];
    fid = fopen(filename_txt,'w');
    if fid == -1
        disp('fopen failed');
        return;
    end
    var_tmp = var_cells{name_idx};
    for j=1:length(idx_set)
        idx = idx_set(j);
        iq = var_tmp(:,idx);
        for i=sp:ep
            fprintf(fid, '%d %d\n', round(real(iq(i))), round(imag(iq(i))));
        end
    end
    fclose(fid);
end
