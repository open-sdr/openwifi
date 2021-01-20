// SPDX-FileCopyrightText: 2020 Michael Tetemke Mehari <michael.mehari@ugent.be>
// SPDX-FileCopyrightText: 2007 Andy Green <andy@warmcat.com>
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

// Thanks for contributions:
// 2007-03-15 fixes to getopt_long code by Matteo Croce rootkit85@yahoo.it

#include "inject_80211.h"
#include "radiotap.h"

/* wifi bitrate to use in 500kHz units */
static const u8 u8aRatesToUse[] = {
	6*2,
	9*2,
	12*2,
	18*2,
	24*2,
	36*2,
	48*2,
	54*2
};

/* this is the template radiotap header we send packets out with */
static const u8 u8aRadiotapHeader[] = 
{
	0x00, 0x00, // <-- radiotap version
	0x1c, 0x00, // <- radiotap header length
	0x6f, 0x08, 0x08, 0x00, // <-- bitmap
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <-- timestamp
	0x00, // <-- flags (Offset +0x10)
	0x6c, // <-- rate (0ffset +0x11)
	0x71, 0x09, 0xc0, 0x00, // <-- channel
	0xde, // <-- antsignal
	0x00, // <-- antnoise
	0x01, // <-- antenna
	0x02, 0x00, 0x0f,  // <-- MCS
};

#define	OFFSET_RATE 0x11
#define MCS_OFFSET 0x19
#define GI_OFFSET 0x1a
#define MCS_RATE_OFFSET 0x1b

/* IEEE80211 header */
static const u8 ieee_hdr[] =
{
	0x08, 0x01, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x10, 0x86,
};

// Generate random string
void gen_rand_str(int size, char *rand_char)
{
	int i, randNum = 0;

	// Seed the random number generator with packet size
	srand(size);
	for (i = 0; i < size; i++)
	{
		// First, pick a number between 0 and 25.
		randNum = 255 * (rand() / (RAND_MAX + 1.0));

		if(randNum == 0)
		{
			i--;
			continue;
		}

		// Type cast to character
		rand_char[i] = (char) randNum;
	}
	rand_char[i] = '\0';
}

int flagHelp = 0;

void usage(void)
{
	printf(
	    "(c)2006-2007 Andy Green <andy@warmcat.com>  Licensed under GPL2\n"
		"(r)2020 Michael Tetemke Mehari <michael.mehari@ugent.be>"
	    "\n"
	    "Usage: inject_80211 [options] <interface>\n\nOptions\n"
	    "-m/--hw_mode <hardware operation mode> (a,g,n)\n"
	    "-r/--rate_index <rate/MCS index> (0,1,2,3,4,5,6,7)\n"
	    "-i/--sgi_flag (0,1)\n"
	    "-n/--num_packets <number of packets>\n"
	    "-s/--payload_size <payload size in bytes>\n"
	    "-d/--delay <delay between packets in usec>\n"
	    "-h   this menu\n\n"

	    "Example:\n"
	    "  iw dev wlan0 interface add mon0 type monitor && ifconfig mon0 up\n"
	    "  inject_80211 mon0\n"
	    "\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	u8 buffer[1536];
	char szErrbuf[PCAP_ERRBUF_SIZE], rand_char[1536], hw_mode = 'n';
	int i, nLinkEncap = 0, r, rate_index = 0, sgi_flag = 0, num_packets = 10, payload_size = 64, packet_size, nDelay = 100000;
	pcap_t *ppcap = NULL;

	while (1)
	{
		int nOptionIndex;
		static const struct option optiona[] =
		{
			{ "hw_mode", required_argument, NULL, 'm' },
			{ "rate_index", required_argument, NULL, 'r' },
			{ "sgi_flag", no_argument, NULL, 'i' },
			{ "num_packets", required_argument, NULL, 'n' },
			{ "payload_size", required_argument, NULL, 's' },
			{ "delay", required_argument, NULL, 'd' },
			{ "help", no_argument, &flagHelp, 1 },
			{ 0, 0, 0, 0 }
		};
		int c = getopt_long(argc, argv, "m:r:i:n:s:d:h", optiona, &nOptionIndex);

		if (c == -1)
			break;
		switch (c)
		{
			case 0: // long option
				break;

			case 'h':
				usage();

			case 'm':
				hw_mode = optarg[0];
				break;

			case 'r':
				rate_index = atoi(optarg);
				break;

			case 'i':
				sgi_flag = atoi(optarg);
				break;

			case 'n':
				num_packets = atoi(optarg);
				break;

			case 's':
				payload_size = atoi(optarg);
				break;

			case 'd':
				nDelay = atoi(optarg);
				break;

			default:
				printf("unknown switch %c\n", c);
				usage();
				break;
		}
	}

	if (optind >= argc)
		usage();

	// open the interface in pcap
	szErrbuf[0] = '\0';
	ppcap = pcap_open_live(argv[optind], 800, 1, 20, szErrbuf);
	if (ppcap == NULL)
	{
		printf("Unable to open interface %s in pcap: %s\n", argv[optind], szErrbuf);
		return (1);
	}

	nLinkEncap = pcap_datalink(ppcap);
	switch (nLinkEncap)
	{
		case DLT_PRISM_HEADER:
			printf("DLT_PRISM_HEADER Encap\n");
			break;

		case DLT_IEEE802_11_RADIO:
			printf("DLT_IEEE802_11_RADIO Encap\n");
			break;

		default:
			printf("!!! unknown encapsulation on %s !\n", argv[1]);
			return (1);
	}

	pcap_setnonblock(ppcap, 1, szErrbuf);

	// Generate random string
	gen_rand_str(payload_size, rand_char);
	packet_size = sizeof(u8aRadiotapHeader) + sizeof(ieee_hdr) + strlen(rand_char);
	printf("mode = 802.11%c, rate index = %d, SHORT GI = %d, number of packets = %d and packet size = %d bytes, delay = %d usec\n", hw_mode, rate_index, sgi_flag, num_packets, packet_size, nDelay);

	// Clear storage buffer
	memset(buffer, 0, sizeof (buffer));

	// Insert default radiotap header
	memcpy(buffer, u8aRadiotapHeader, sizeof (u8aRadiotapHeader));
	// Update radiotap header (i.e. hw_mode, rate, GI)
	if(hw_mode == 'g' || hw_mode == 'a')
	{
		buffer[OFFSET_RATE] = u8aRatesToUse[rate_index];
		buffer[MCS_OFFSET] = 0x00;
	}
	else
	{
		buffer[MCS_OFFSET] = 0x07;
		if(sgi_flag)
			buffer[GI_OFFSET] = IEEE80211_RADIOTAP_MCS_SGI;
		buffer[MCS_RATE_OFFSET] = rate_index;
	}
	// Insert IEEE DATA header
	memcpy(buffer + sizeof(u8aRadiotapHeader), ieee_hdr, sizeof (ieee_hdr));
	// Insert IEEE DATA payload
	sprintf((char *)(buffer + sizeof(u8aRadiotapHeader) + sizeof(ieee_hdr)), "%s", rand_char);

	// Inject packets
	for(i = 1; i <= num_packets; i++)
	{
		r = pcap_inject(ppcap, buffer, packet_size);
		if (r != packet_size) {
			perror("Trouble injecting packet");
			return (1);
		}

		printf("number of packets sent = %d\r", i);
		fflush(stdout);

		if (nDelay)
			usleep(nDelay);
	}

	printf("\n");

	return (0);
}
