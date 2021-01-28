/* Copyright (c) 2017 Northbound Networks
 *
 * Written By Paul Zanna (paul@northboundnetworks.com)
 *
 */

#define BUFLEN 65536

#define MAX_FLOWS    512
#define MAX_TABLES      16
#define SHARED_BUFFER_LEN 16384
#define PACKET_BUFFER 32
#define PACKET_BUFFER_SIZE 256
#define PB_EMPTY 0
#define PB_PACKETIN 1
#define PB_PACKETOUT 2
#define PB_PENDING 3

#define VERSION 0.3

struct flows_counter
{
    uint64_t hitCount;
    uint64_t bytes;
    uint32_t duration;
    uint8_t active;
    int lastmatch;
};

struct flow_table
{
    int iLastFlow;
    int enabled;
    struct flows_counter    flow_counters[MAX_FLOWS];
};

struct packet_buffer
{
    uint8_t type;
    uint8_t age;
    uint16_t size;
    uint32_t inport;
    uint8_t reason;
    uint8_t flow;
    uint8_t table_id;
    uint32_t outport;
    //struct sk_buff *skb;
    uint8_t buffer[PACKET_BUFFER_SIZE];
};

struct packet_out
{
    uint32_t inport;
    uint32_t outport;
    //uint32_t sk_buff *skb; 
};

struct pbuffer
{
    struct packet_buffer buffer[PACKET_BUFFER];
};
