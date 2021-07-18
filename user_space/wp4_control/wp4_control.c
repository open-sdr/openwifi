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
int tCount, min_rule, min_match, minmax_offset;
bool fCapture;
struct Headers_t headers;
time_t t;

// Internal Functions
void clear_flowtable(void);
void fill_headers(void);
void matching(void);
void covering(void);
void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope);

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

    srand((unsigned) time(&t));
    
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
                    if (token_cnt == 1)
                    {
                        tCount = atoi(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }

            if (strstr(line, "min_rule") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_rule = atoi(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }

            if (strstr(line, "min_match") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_match = atoi(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }

            if (strstr(line, "minmax_offset") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        minmax_offset = atoi(token);
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

    // print startup values
    
    printf("*****************************************************\n");
    if (fCapture == true) printf("Capturing frames to file\n"); 
    printf("Max rule count = %d\n", TABLE_SIZE);
    printf("Training frame count = %d\n", tCount);
    printf("Min number of rules for covering = %d\n", min_rule);
    printf("Min number of matches for covering = %d\n", min_match);
    printf("Min / Max offset % = %d\n", minmax_offset);
    printf("*****************************************************\n");
    // Main processing loop
    while(1)
    {
        usleep(100000);    // Allow CPU to sleep (0.1s)
        fill_headers();
        
    }
    return 0;
}

/*
 *  Populate the local headers struct from each buffer entry in turn
 */
void fill_headers(void)
{
    int i;

    for(i=0;i<(PACKET_BUFFER);i++)
    {
        if(pk_buffer->buffer[i].type == PB_PENDING)
        {
            memcpy(&headers, &pk_buffer->buffer[i].buffer, sizeof(struct Headers_t));
            headers.rfFeatures.rssi = (headers.rfFeatures.rssi>>1);     // Shift RSSI Value
            printf("buffer %d loaded - Time: %lld, MAC: %llX, Rate_IDX: %d, RSSI: %d, Phase Offset: %d, Pilot Offset: %d, MagSQ: %d \n", i, headers.rfFeatures.timestamp, headers.mac80211.Addr2, headers.rfFeatures.rate_idx, headers.rfFeatures.rssi, headers.rfFeatures.phaseOffset, headers.rfFeatures.pilotOffset, headers.rfFeatures.magSq);
            matching();
            pk_buffer->buffer[i].type = PB_EMPTY;
        }
    }

    return;
}

/*
 *  Calculate the match set from existing rules
 */
void matching(void)
{
    int j;
    //printf("Matching: last rule = %d\n", flow_table->last_entry);

    if (flow_table->last_entry < min_rule) // ==0
    {
        printf("No rules so automatically applying covering\n");
        covering();
    } else
    {
        printf("******\n");
        for(j=0;j<flow_table->last_entry;j++)
        {
            printf("#%d - MAC: %llX, Rate_IDX: %d, RSSI: %d (%d) - %d(%d), Phase Offset: %d(%d) - %d(%d), Pilot Offset: %d(%d) - %d(%d), MagSQ: %d(%d) - %d(%d) \n", j+1, flow_table->key[j].headers_mac80211_Addr2_class, flow_table->key[j].headers_rfFeatures_rate_idx_exact, flow_table->key[j].headers_rfFeatures_rssi_min, flow_table->scope[j].headers_rfFeatures_rssi_min, flow_table->key[j].headers_rfFeatures_rssi_max, flow_table->scope[j].headers_rfFeatures_rssi_max, flow_table->key[j].headers_rfFeatures_phaseOffset_min, flow_table->scope[j].headers_rfFeatures_phaseOffset_min, flow_table->key[j].headers_rfFeatures_phaseOffset_max, flow_table->scope[j].headers_rfFeatures_phaseOffset_max, flow_table->key[j].headers_rfFeatures_pilotOffset_min, flow_table->scope[j].headers_rfFeatures_pilotOffset_min, flow_table->key[j].headers_rfFeatures_pilotOffset_max, flow_table->scope[j].headers_rfFeatures_pilotOffset_max, flow_table->key[j].headers_rfFeatures_magSq_min, flow_table->scope[j].headers_rfFeatures_magSq_min, flow_table->key[j].headers_rfFeatures_magSq_max, flow_table->scope[j].headers_rfFeatures_magSq_max); 
        }
        printf("******\n\n");
    }
    return;
}

/*
 *  Covering add new rules when the rule table is empty OR the number of matches is below the min_match level
 */
void covering(void)
{
    struct swtch_lookup_tbl_key key;
    struct rule_scope scope;
    // Populate a rule and call add_rule function
    key.headers_mac80211_Addr2_class = headers.mac80211.Addr2;
    key.headers_rfFeatures_rate_idx_exact = headers.rfFeatures.rate_idx;
    key.headers_rfFeatures_phaseOffset_max = headers.rfFeatures.phaseOffset + ((headers.rfFeatures.phaseOffset * minmax_offset)/100);
    key.headers_rfFeatures_phaseOffset_min = headers.rfFeatures.phaseOffset - ((headers.rfFeatures.phaseOffset * minmax_offset)/100);
    key.headers_rfFeatures_rssi_max = headers.rfFeatures.rssi + ((headers.rfFeatures.rssi * minmax_offset)/100);
    key.headers_rfFeatures_rssi_min = headers.rfFeatures.rssi - ((headers.rfFeatures.rssi * minmax_offset)/100);
    key.headers_rfFeatures_pilotOffset_max = headers.rfFeatures.pilotOffset + ((headers.rfFeatures.pilotOffset * minmax_offset)/100);
    key.headers_rfFeatures_pilotOffset_min = headers.rfFeatures.pilotOffset - ((headers.rfFeatures.pilotOffset * minmax_offset)/100);
    key.headers_rfFeatures_magSq_max = headers.rfFeatures.magSq + ((headers.rfFeatures.magSq * minmax_offset)/100);
    key.headers_rfFeatures_magSq_min = headers.rfFeatures.magSq - ((headers.rfFeatures.magSq * minmax_offset)/100);

    scope.headers_mac80211_Addr2_class = 1;    // Class is always in scope
    scope.headers_rfFeatures_rate_idx_exact = rand() & 1;
    scope.headers_rfFeatures_phaseOffset_max = rand() & 1;
    scope.headers_rfFeatures_phaseOffset_min = rand() & 1;
    scope.headers_rfFeatures_rssi_max = rand() & 1;
    scope.headers_rfFeatures_rssi_min = rand() & 1;
    scope.headers_rfFeatures_pilotOffset_max = rand() & 1;
    scope.headers_rfFeatures_pilotOffset_min = rand() & 1;
    scope.headers_rfFeatures_magSq_max = rand() & 1;
    scope.headers_rfFeatures_magSq_min = rand() & 1;

    if ((scope.headers_rfFeatures_rate_idx_exact == 0) && (scope.headers_rfFeatures_phaseOffset_max == 0) && (scope.headers_rfFeatures_phaseOffset_min ==0) && (scope.headers_rfFeatures_rssi_max == 0) && (scope.headers_rfFeatures_rssi_min == 0) && (scope.headers_rfFeatures_pilotOffset_max == 0) && (scope.headers_rfFeatures_pilotOffset_min == 0) && (scope.headers_rfFeatures_magSq_max == 0) && (scope.headers_rfFeatures_magSq_min == 0))
    {
        printf("Full wild, not adding rule!\n");
        return; // Rule is all wildcards to no point adding to population
    }

    add_rule(&key, &scope);
    return;
}

void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope)
{
    //printf("New rule added at %d - MAC: %llX, RSSI: %d (%d) - %d(%d), Phase Offset: %d(%d) - %d(%d) \n\n", flow_table->last_entry, key->headers_mac80211_Addr2_class, key->headers_rfFeatures_rssi_min, scope->headers_rfFeatures_rssi_min, key->headers_rfFeatures_rssi_max, scope->headers_rfFeatures_rssi_max, key->headers_rfFeatures_phaseOffset_min, scope->headers_rfFeatures_phaseOffset_min, key->headers_rfFeatures_phaseOffset_max, scope->headers_rfFeatures_phaseOffset_max); 
    flow_table->key[flow_table->last_entry].headers_mac80211_Addr2_class = key->headers_mac80211_Addr2_class;
    flow_table->scope[flow_table->last_entry].headers_mac80211_Addr2_class = scope->headers_mac80211_Addr2_class;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = key->headers_rfFeatures_rate_idx_exact;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = scope->headers_rfFeatures_rate_idx_exact;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = key->headers_rfFeatures_phaseOffset_max;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = scope->headers_rfFeatures_phaseOffset_max;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = key->headers_rfFeatures_phaseOffset_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = scope->headers_rfFeatures_phaseOffset_min;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_max = key->headers_rfFeatures_rssi_max;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max = scope->headers_rfFeatures_rssi_max;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_min = key->headers_rfFeatures_rssi_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min = scope->headers_rfFeatures_rssi_min;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = key->headers_rfFeatures_pilotOffset_max;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = scope->headers_rfFeatures_pilotOffset_max;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = key->headers_rfFeatures_pilotOffset_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = scope->headers_rfFeatures_pilotOffset_min;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_max = key->headers_rfFeatures_magSq_max;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max = scope->headers_rfFeatures_magSq_max;

    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_min = key->headers_rfFeatures_magSq_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min = scope->headers_rfFeatures_magSq_min;

    printf("New rule added at %d - MAC: %llX, Rate_IDX: %d(%d), RSSI: %d (%d) - %d(%d), Phase Offset: %d(%d) - %d(%d), Pilot Offset: %d(%d) - %d(%d), MagSQ: %d(%d) - %d(%d) \n\n", flow_table->last_entry, flow_table->key[flow_table->last_entry].headers_mac80211_Addr2_class, flow_table->key[flow_table->last_entry].headers_rfFeatures_rate_idx_exact, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact, flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max); 
    flow_table->last_entry++;
    return;
}