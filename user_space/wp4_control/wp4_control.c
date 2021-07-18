/* Copyright (c) 2017 Northbound Networks
 *
 * Written By Paul Zanna (paul@northboundnetworks.com)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <float.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ifaddrs.h>

#include "wp4_control.h"

extern int errno;

#define FTPAGES      16
#define PBPAGES      16 
#define PAGE_SIZE   4096

int current_time;
int sock;
struct ofmsgbuf *inbuf, *outbuf;    // input,output buffers
struct wp4_map_def  *flow_table;
struct pbuffer *pk_buffer;
int tCount;
bool fCapture;
struct Headers_t headers;

// Internal Functions
void clear_flowtable(void);
void fill_headers(void);

static inline uint64_t (_htonll)(uint64_t n)
{
    return htonl(1) == 1 ? n : ((uint64_t) htonl(n) << 32) | htonl(n >> 32);
}

void clear_flowtable(void)
{
    printf("Rule table and buffer cleared!\n\n");
    memset(flow_table, 0, FTPAGES * PAGE_SIZE);
    memset(pk_buffer, 0, PBPAGES * PAGE_SIZE);

}

int main(int argc, const char * argv[]) {

    int x;
    int errnum;
    int configfd;
    char line[128];
    char *token;
    int token_cnt = 0;
    int opt = 0;        // command line options
    
    // Load UCI config file
    FILE *file = fopen( "wp4_control.cfg", "r" );
    
    if ( file == 0 )
    {
        printf("No config file found!\n\n");
    }
    else
    {
        while(fgets(line, sizeof(line), file) != NULL)
        {
            if(line[0] == '#' || line[0] == '\n') continue;     // Ignore comments

            if (strstr(line, "training_count") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    //printf("%d %s\n", token_cnt, token);
                    if (token_cnt == 1)
                    {
                        tCount = atoi(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }


        }
        fclose( file );
    }

    configfd = open("/proc/wp4_data", O_RDWR);
    if(configfd < 0)
    {
        printf("Unable to open mmap file!\n");
        return -1;
    }
    
    flow_table = mmap(NULL, FTPAGES*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, configfd, 0);

    if (flow_table == MAP_FAILED)
    {
        errnum = errno;
        printf("flow_table mmap operation failed: %s(%d)\n",strerror(errnum),errnum);
        return -1;
    }

    pk_buffer = mmap(NULL, PBPAGES*PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, configfd, FTPAGES*PAGE_SIZE);
    if (pk_buffer == MAP_FAILED)
    {
        printf("pk_buffer mmap operation failed: %s(%d)\n",strerror(errnum), errnum);
        return -1;
    }
    // Initialise time
    current_time = (int)time(NULL);
    
    // Check for arguments
    if(argc > 1)
    {
        while((opt=getopt(argc,argv,"cC"))!=-1)
        {
            switch(opt)
            {
                case 'C':
                    // Capture frames to a CSV file
                    fCapture = true;
                    break;                  
                case 'c':
                    // clear flowtable
                    clear_flowtable();
                    break;
            }
        }
    }
    
    flow_table->last_entry = 4;

    // print startup values
    
    printf("*****************************************************\n");
    if (fCapture == true) printf("Capturing frames to file\n"); 
    printf("Max rule count = %d\n", TABLE_SIZE);
    printf("Training frame count = %d\n", tCount);
    printf("*****************************************************\n");
    // Main processing loop
    while(1)
    {
        usleep(100000);    // Allow CPU to sleep (1s)
        fill_headers();
        
    }
    return 0;
}

void fill_headers(void)
{
    int i;

    for(i=0;i<(PACKET_BUFFER);i++)
    {
        if(pk_buffer->buffer[i].type == PB_PENDING)
        {
            memcpy(&headers, &pk_buffer->buffer[i].buffer, sizeof(struct Headers_t));
            headers.rfFeatures.rssi = (headers.rfFeatures.rssi>>1);     // Shift RSSI Value
            printf("buffer %d loaded - Time: %lld, MAC: %llX, RSSI: %d, Phase Offset: %d\n", i, headers.rfFeatures.timestamp, headers.mac80211.Addr2, headers.rfFeatures.rssi, headers.rfFeatures.phaseOffset);
            pk_buffer->buffer[i].type = PB_EMPTY;
        }
    }

    return;
}