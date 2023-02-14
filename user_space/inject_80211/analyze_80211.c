
// Author: Michael Mehari
// SPDX-FileCopyrightText: 2020 UGent
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "inject_80211.h"
#include "radiotap.h"
#include "uthash.h"

#define HEADERLEN_80211   24

struct RECORD_t
{
	char		id[16];		// hw_mode-rate-sgi_flag-packet_size
	uint16_t	pkt_cnt;	// number of packets received
	uint64_t	ts_begin;	// beginning timestamp
	uint64_t	ts_end;		// ending timestamp

	UT_hash_handle	hh;		// hash function handler
};

/* 802.11n bitrates x 2 */
static const uint8_t rates_11n[] = {13, 26, 39, 52, 78, 104, 117, 130};

int main(int argc, char **argv)
{ 
	struct pcap_pkthdr pcap_hdr; 
	const u_char *packet;
	char hw_mode, id[16];
	int rate, sgi_flag, packet_size;

	int n, hdr_len;
	struct ieee80211_radiotap_iterator rti;

	struct RECORD_t *RECORD_ptr, *tmp_ptr, *hash_ptr = NULL;

	if (argc < 2)
	{ 
		fprintf(stderr, "Usage: %s <pcap>\n", argv[0]); 
		exit(1); 
	} 

	pcap_t *handle; 
	char errbuf[PCAP_ERRBUF_SIZE];  
	handle = pcap_open_offline(argv[1], errbuf); 

	if (handle == NULL)
	{
		fprintf(stderr,"Couldn't open pcap file %s: %s\n", argv[1], errbuf); 
		return(2); 
	} 

	while ((packet = pcap_next(handle, &pcap_hdr)))
	{
		hdr_len = (packet[2] + (packet[3] << 8));
		if (pcap_hdr.len < (hdr_len + HEADERLEN_80211))
			continue;

		packet_size = pcap_hdr.len - (hdr_len + HEADERLEN_80211);
		if (packet_size < 0)
			continue;

		if (ieee80211_radiotap_iterator_init(&rti, (struct ieee80211_radiotap_header *)packet, packet_size, NULL) < 0)
			continue;

		while ((n = ieee80211_radiotap_iterator_next(&rti)) == 0)
		{
			switch (rti.this_arg_index)
			{
				case IEEE80211_RADIOTAP_RATE:
					rate = (rti.this_arg)[0];
					sgi_flag = 0;
					hw_mode = 'a';
					break;

				case IEEE80211_RADIOTAP_MCS:
					rate = rates_11n[((rti.this_arg)[2])];
					sgi_flag = (rti.this_arg)[1] & 0x40;
					hw_mode = 'n';
					break;
			}
		}

		// create hash table index
		sprintf(id, "%c-%d-%d-%d", hw_mode, rate, sgi_flag, packet_size);

		// Hash table implementation for c : https://github.com/troydhanson/uthash
		HASH_FIND_STR(hash_ptr, id, RECORD_ptr);
		if(RECORD_ptr == NULL)
		{
			RECORD_ptr = (struct RECORD_t*)malloc(sizeof(struct RECORD_t));
			if(RECORD_ptr == NULL)
			{
				fprintf(stderr, "Unable to create record!\n");
				return 1;
			}

			strcpy(RECORD_ptr->id, id);
			RECORD_ptr->pkt_cnt = 1;
			RECORD_ptr->ts_begin = 1e6*pcap_hdr.ts.tv_sec + pcap_hdr.ts.tv_usec;

			// Add the new record to the hash table
			HASH_ADD_STR(hash_ptr, id, RECORD_ptr);
		}
		else
		{
			RECORD_ptr->pkt_cnt++;
			RECORD_ptr->ts_end = 1e6*pcap_hdr.ts.tv_sec + pcap_hdr.ts.tv_usec;
		}
	} 
	pcap_close(handle);


	// Iterate through the hash table
	printf("HW MODE\tRATE(Mbps)\tSGI\tSIZE(bytes)\tCOUNT\tDELAY(sec)\n");
	printf("=======\t==========\t===\t===========\t=====\t=========\n");
	HASH_ITER(hh, hash_ptr, RECORD_ptr, tmp_ptr)
	{
		sscanf(RECORD_ptr->id, "%c-%d-%d-%d", &hw_mode, &rate, &sgi_flag, &packet_size);
		printf("802.11%c\t%.1f\t\t%s\t%d\t\t%d\t%.5f\n", hw_mode, rate/2.0, (sgi_flag == 0 ? "OFF" : "ON"), packet_size, RECORD_ptr->pkt_cnt, 1e-6*(RECORD_ptr->ts_end - RECORD_ptr->ts_begin));
	}
	fflush(stdout);

	return 0;
}
