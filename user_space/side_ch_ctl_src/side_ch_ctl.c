/*
 * openwifi side channel user space program
 * Author: Xianjun Jiao
 * SPDX-FileCopyrightText: 2019 UGent
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

// #define NETLINK_USER 31
#define MAX_NUM_DMA_SYMBOL 8192   //align with side_ch.v side_ch.h

#define MAX_PAYLOAD (8*MAX_NUM_DMA_SYMBOL) /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

//align with side_ch_control.v and all related user space, remote files
#define CSI_LEN 56 // length of single CSI
#define EQUALIZER_LEN (56-4) // for non HT, four {32767,32767} will be padded to achieve 52 (non HT should have 48)
#define HEADER_LEN 2 //timestamp and frequency offset

#define ACTION_INVALID       0
#define ACTION_REG_WRITE     1
#define ACTION_REG_READ      2
#define ACTION_SIDE_INFO_GET 3

#define REG_TYPE_INVALID     0
#define REG_TYPE_HARDWARE    1
#define REG_TYPE_SOFTWARE    2

#define MAX_PARA_STRING_LEN  31
char tmp_str[MAX_PARA_STRING_LEN+1];
int take_reg_idx_string_for_write(char *para) { // return into tmp_str till 'd' 'D' 'h' 'H' or 0
    int i = 0;

    // while (para[i] != 'd' && para[i] != 'D' && para[i] != 'h' && para[i] != 'H' && para[i] != 0) {
    while (para[i] != 'd' && para[i] != 'h' && para[i] != 0) {
        tmp_str[i] = para[i];
        i++;
    }

    if (i==0)
        return(-1);
    
    if (para[i-1] == 0) // we expect d D h H, not 0!
        return(-2);
    
    tmp_str[i] = 0;

    return(i);
}

int take_reg_val_string_for_write(char *para) {
    int i = 0;

    while (para[i] != 0) {
        tmp_str[i] = para[i];
        i++;
    }

    if (i==0)
        return(-1);
        
    tmp_str[i] = 0;

    return(i);
}

int all_zero_in_string(char *para) {
    int i;
    int check_len = strlen(para);

    if (check_len == 0)
        return(-1);

    i = 0;
    while (para[i] == '0')
        i++;
    
    if (i == check_len)
        return(1);

    return(0);
}

long int atoi_my(char *para) {
    long int ret = all_zero_in_string(para);

    if (ret<0)
        return(-1);

    if (ret==1)
        return(0);

    ret = atol(para);

    if (ret==0)
        return(-1);
    
    return(ret);
}

long int hextoi_my(char *para) {
    long int ret = all_zero_in_string(para);

    if (ret<0)
        return(-1);

    if (ret==1)
        return(0);

    ret = strtoul(para, NULL, 16);

    if (ret==0)
        return(-1);
    
    return(ret);
}

// parameter_string format:
// write 987   to hardware register  3: wh3d987  (w--write; h--hardware; 3 --register idx; d--decimal; 987--value)
// write 0x3db to software register 19: ws19h3db (w--write; s--software; 19--register idx; h--hex;     3db--value 0x3db)
//           read software register 23: rs23     (r-- read; s--software; 23--register idx)
//        get csi and equalizer output: g400     (g--  get; 400--every 400ms; no/wrong input means default 100ms)
int parse_para_string(char *para, int *action_flag, int *reg_type, int *reg_idx, unsigned int *reg_val, int *interval_ms) {
    int i, para_string_len, num_char_reg_idx, num_char_reg_val, hex_flag;

    para_string_len = strlen(para);

    if (para_string_len == 0 || para_string_len>MAX_PARA_STRING_LEN) {
        printf("Parameter string is too short/long!\n");
        return(-1);
    }
    
    // process the csi/equalizer get command
    if ( para[0] == 'g'){// || para[0] == 'G' ) {
        (*action_flag) = ACTION_SIDE_INFO_GET;
        
        if (para_string_len == 1) { // no explicit input
            (*interval_ms) = 100;
            printf("The default 100ms side info getting period is taken!\n");
            return(0);
        }

        // there is something input
        (*interval_ms) = atoi_my(para+1);
        if ( (*interval_ms)<0 ) { // for invalid input, we set it to the default 100ms
            (*interval_ms) = 100;
            printf("Invalid side info getting period!\n");
            printf("The default 100ms side info getting period is taken!\n");
        }
        
        return(0);
    }

    if (para_string_len == 2) {// this is invalid, for read and write, the length should be > 2
        printf("Lack of input (register index/value) for read/write action\n");
        return(-2);
    }

    // process the register read command
    if ( para[0] == 'r'){// || para[0] == 'R' ) {
        (*action_flag) = ACTION_REG_READ;
        
        if ( para[1] == 'h')// || para[1] == 'H' )
            (*reg_type) = REG_TYPE_HARDWARE;
        else if ( para[1] == 's')// || para[1] == 'S' )
            (*reg_type) = REG_TYPE_SOFTWARE;
        else {
            (*reg_type) = REG_TYPE_INVALID;
            printf("Invalid register type (s/h is expected)!\n");
            return(-3);
        }

        (*reg_idx) = atoi_my(para+2);
        if ( (*reg_idx)<0 || (*reg_idx)>31) {
            printf("Invalid register index (should be 0~31)!\n");
            return(-4);
        }

        return(0);
    }

    if (para_string_len < 5) { // this is invalid, for write, the length should be >= 5. example wh3d9
        printf("Lack of input (register value/etc) for write action\n");
        return(-5);
    }

    // process the register write command
    if ( para[0] == 'w'){// || para[0] == 'W' ) {
        (*action_flag) = ACTION_REG_WRITE;
        
        if ( para[1] == 'h')// || para[1] == 'H' )
            (*reg_type) = REG_TYPE_HARDWARE;
        else if ( para[1] == 's')// || para[1] == 'S' )
            (*reg_type) = REG_TYPE_SOFTWARE;
        else {
            (*reg_type) = REG_TYPE_INVALID;
            printf("Invalid register type (s/h is expected)!\n");
            return(-6);
        }

        num_char_reg_idx = take_reg_idx_string_for_write(para+2);
        if ( num_char_reg_idx<0 ) {
            printf("Invalid register index input!\n");
            return(-7);
        }
        
        // if ((num_char_reg_idx+2)==para_string_len) //consume all string already
        //     return(-8);

        (*reg_idx) = atoi_my(tmp_str);
        if ( (*reg_idx)<0 || (*reg_idx)>31 ) {
            printf("Invalid register index (should be 0~31)!\n");
            return(-9);
        }

        if (para[2+num_char_reg_idx] == 'd')// || para[2+num_char_reg_idx] == 'D')
            hex_flag=0;
        else if (para[2+num_char_reg_idx] == 'h')// || para[2+num_char_reg_idx] == 'H')
            hex_flag=1;
        else {
            printf("Invalid hex/decimal flag (d/h is expected)!\n");
            return(-10);
        }

        num_char_reg_val = take_reg_val_string_for_write(para+2+num_char_reg_idx+1);
        if ( num_char_reg_val<0 ) {
            printf("Invalid register value input!\n");
            return(-11);
        }

        if (hex_flag==0) {
            (*reg_val) = atoi_my(tmp_str);
            if ( (*reg_val)<0 ) {
                printf("Invalid register value input of decimal number!\n");
                return(-12);
            }
        } else {
            (*reg_val) = hextoi_my(tmp_str);
            // printf("%u %s\n", (*reg_val), tmp_str);
            if ( (*reg_val)<0 ) {
                printf("Invalid register value input of hex number!\n");
                return(-13);
            }
        }
        return(0);
    }

    return(-14);
}

void print_usage(void) {
    printf("Usage: side_ch_ctl parameter_string\n");
    printf("Example:\n");
    printf("write 987   to hardware register  3: wh3d987  (w--write; h--hardware; 3 --register idx; d--decimal; 987--value)\n");
    printf("write 0x3db to software register 19: ws19h3db (w--write; s--software; 19--register idx; h--hex;     3db--value 0x3db)\n");
    printf("          read software register 23: rs23     (r-- read; s--software; 23--register idx)\n");
    printf("       get csi and equalizer output: g400     (g--  get; 400--every 400ms; no/wrong input means default 100ms)\n");
}

volatile bool do_exit = false;

void sigint_callback_handler(int signum)
{
	fprintf(stdout, "Caught signal %d\n", signum);
	do_exit = true;
}

int main(const int argc, char * const argv[])
{
    int action_flag, reg_type, reg_idx, interval_ms, s, side_info_size, socket_ok = 1, loop_count=0, side_info_count=0;
    unsigned int reg_val, *cmd_buf;
    unsigned short port;
    struct sockaddr_in server;
    int value_only_flag = 0;
    int ret = 0;

    if (argc<2) {
        printf("1 argument is needed!\n");
        print_usage();
        return(ret);
    }

    value_only_flag = (argc>2?1:0);

    ret = parse_para_string(argv[1], &action_flag, &reg_type, &reg_idx, &reg_val, &interval_ms);
    if (value_only_flag==0) {
        printf("parse: ret %d\n", ret);
        printf("   tx: action_flag %d reg_type %d reg_idx %d reg_val %u interval_ms %d\n", action_flag, reg_type, reg_idx, reg_val, interval_ms);
    }
    if (ret<0) {
        printf("Wrong input!\n");
        print_usage();
        return(ret);
    }

    // if (signal(SIGINT, &sigint_callback_handler)==SIG_ERR ||
    //     signal(SIGILL, &sigint_callback_handler)==SIG_ERR ||
    //     signal(SIGFPE, &sigint_callback_handler)==SIG_ERR ||
    //     signal(SIGSEGV, &sigint_callback_handler)==SIG_ERR ||
    //     signal(SIGTERM, &sigint_callback_handler)==SIG_ERR ||
    //     signal(SIGABRT, &sigint_callback_handler)==SIG_ERR) {
    if (signal(SIGINT, &sigint_callback_handler)==SIG_ERR) {
        printf("SIG_ERR!\n");
        return(ret);
    }

    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
    if(sock_fd<0) {
        printf("sock_fd %d\n", sock_fd);
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    // memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

    // nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);

    // strcpy(NLMSG_DATA(nlh), "Hello");

    // udp socket setup
    port = htons(4000); // port 4000 at remote server
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("socket() error! Will not send info to remote.\n");
        socket_ok = 0;
    }
    server.sin_family      = AF_INET;            /* Internet Domain    */
    server.sin_port        = port;               /* Server Port        */
    server.sin_addr.s_addr = inet_addr("192.168.10.1"); /* Server's Address   */

    while(do_exit==false) {
        nlh->nlmsg_len = NLMSG_SPACE(4*4);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;

        cmd_buf = (unsigned int*)NLMSG_DATA(nlh);
        cmd_buf[0] = action_flag;
        cmd_buf[1] = reg_type;
        cmd_buf[2] = reg_idx;
        cmd_buf[3] = reg_val;

        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        // printf("Sending message to kernel\n");
        sendmsg(sock_fd,&msg,0);
        // printf("Waiting for message from kernel\n");

        nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        iov.iov_len = nlh->nlmsg_len;
        /* Read message from kernel */
        recvmsg(sock_fd, &msg, 0);
        // printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));

        side_info_size = nlh->nlmsg_len-NLMSG_HDRLEN;
        // printf("num_dma_symbol %d\n", side_info_size/8);

        if (action_flag!=ACTION_SIDE_INFO_GET) {
            if (value_only_flag==0)
                printf("   rx: size %d val %d 0x%08x\n", side_info_size, cmd_buf[0], cmd_buf[0]);
            else
                printf("%u\n", cmd_buf[0]);
            break;
        }
        
        if (socket_ok && (side_info_size >= ((CSI_LEN+0*EQUALIZER_LEN+HEADER_LEN)*8)))
            if (sendto(s, cmd_buf, side_info_size, 0, (struct sockaddr *)&server, sizeof(server)) < 0)
                printf("sendto() error!\n");

        side_info_count = side_info_count + (side_info_size>4);
        loop_count++;
        if ((loop_count%64) == 0)
            printf("loop %d side info count %d\n", loop_count, side_info_count);

        usleep(interval_ms*1000);
    }
    
    close(s);
    close(sock_fd);
    return(ret);
}
