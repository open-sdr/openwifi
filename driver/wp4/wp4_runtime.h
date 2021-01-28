/*
Copyright 2020 Paul Zanna.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <linux/types.h>

#define MAX_FLOWS    512
#define SHARED_BUFFER_LEN 16384
#define PACKET_BUFFER 32
#define PACKET_BUFFER_SIZE 256
#define PB_EMPTY 0
#define PB_PACKETIN 1
#define PB_PACKETOUT 2
#define PB_PENDING 3

void dump_rx_packet(u8 *ptr);
int table_init(void);
void table_exit(void);
struct packet_out CPU_Port(int buffer_id);

struct flows_counter
{
    u64 hitCount;
    u64 bytes;
    u32 duration;
    u8 active;
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
    u8 type;
    u8 age;
    u16 size;
    u32 inport;
    u8 reason;
    u8 flow;
    u8 table_id;
    u32 outport;
    struct sk_buff *skb;
    u8 buffer[PACKET_BUFFER_SIZE];
};

struct packet_out
{
    u32 inport;
    u32 outport;
    struct sk_buff *skb; 
};

struct pbuffer
{
    struct packet_buffer buffer[PACKET_BUFFER];
};

