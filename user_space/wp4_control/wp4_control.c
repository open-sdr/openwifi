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
struct flow_table *flow_table;
struct pbuffer *pk_buffer;
int enabled;

// Internal Functions
void clear_flowtable(void);

static inline uint64_t (_htonll)(uint64_t n)
{
    return htonl(1) == 1 ? n : ((uint64_t) htonl(n) << 32) | htonl(n >> 32);
}

void clear_flowtable(void)
{

    memset(flow_table, 0, FTPAGES * PAGE_SIZE);
    memset(pk_buffer, 0, PBPAGES * PAGE_SIZE);
    
    // Set flags
    flow_table->enabled = enabled;

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
    FILE *file = fopen( "/etc/config/nnofagent", "r" );
    
    if ( file == 0 )
    {
        // No file found
    }
    else
    {
        while(fgets(line, sizeof(line), file) != NULL)
        {
            if(line[0] == '#' || line[0] == '\n') continue;     // Ignore comments

            if (strstr(line, "enabled") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 2)
                    {
                        enabled = atoi(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }
        }
        fclose( file );
    }


    //configfd = open("/sys/kernel/debug/wp4/data", O_RDWR);
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
        while((opt=getopt(argc,argv,"fgmc"))!=-1)
        {
            switch(opt)
            {
                case 'f':
                    // call function
                    if(munmap(flow_table, FTPAGES*PAGE_SIZE) !=0)
                    {
                        printf("munmap operation failed for flow_table!\n");
                    }
                    if(munmap(pk_buffer, PBPAGES*PAGE_SIZE) !=0)
                    {
                        printf("munmap operation failed for pk_buffer!\n");
                    }                       
                    return 0;
                case 'c':
                    // clear flowtable
                    clear_flowtable();
                    return 0;
            }
        }
    }

    clear_flowtable();
    
    flow_table->iLastFlow = 9;

    // Main processing loop
    while(1)
    {
        usleep(100000);    // Allow CPU to sleep (1s)
        printf("iLastFlow %d\n", flow_table->iLastFlow);
        
    }
    return 0;
}
