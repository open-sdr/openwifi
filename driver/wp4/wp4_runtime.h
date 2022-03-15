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
#include "wp4-p4.h"

#define PACKET_BUFFER 64
#define PB_EMPTY 0
#define PB_PENDING 1

void dump_rx_packet(u8 *ptr, u16 wp4_ul_size);
int table_init(void);
void table_exit(void);
void to_cpu(struct Headers_t headers);
int state_update(int state_input);
int dev_ident(struct swtch_lookup_tbl_key *key);
int ssid_check(u8 *p_uc_data, u16 wp4_ul_size);

struct packet_buffer
{
    u8 flow;
    u8 type;
    struct Headers_t buffer;
};

struct pbuffer
{
    struct packet_buffer buffer[PACKET_BUFFER];
};

