/* Copyright (c) 2017 Northbound Networks
 *
 * Written By Paul Zanna (paul@northboundnetworks.com)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <float.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

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
struct classifier  *flow_table;
struct pbuffer *pk_buffer;
int inst_count, tCount, vCount;
int min_rule, min_correct_match, min_match, minmax_offset, min_del_exp;
float min_del_sen;
bool fCapture, vOnly;
struct Headers_t headers;
time_t t;

// Internal Functions
void clear_flowtable(void);
void fill_headers(void);
void matching(void);
void covering(void);
void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope);
void delete_rule(int rule);
void print_rule(int rule);

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

    int x, j;
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
    } else {
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

            if (strstr(line, "validation_count") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        vCount = atoi(token);
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

            if (strstr(line, "min_correct_match") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_correct_match = atoi(token);
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

            if (strstr(line, "min_del_sen") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_del_sen = atof(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }

            if (strstr(line, "min_del_exp") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_del_exp = atoi(token);
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
        while((opt=getopt(argc,argv,"cCpv"))!=-1)
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
                case 'v':
                    // Varification only
                    vOnly = true;
                    inst_count = tCount;
                    break;                    
                case 'p':
                    // Print out the rule set
                    for(j=0;j<flow_table->last_entry;j++)
                    {
                        print_rule(j);
                    }
                    return;
            }
        }
    }

    // print startup values
    
    printf("*****************************************************\n");
    if (fCapture == true) printf("Capturing frames to file\n"); 
    printf("Max rule count = %d\n", TABLE_SIZE);
    printf("Training frame count = %d\n", tCount);
    printf("Validation frame count = %d\n", vCount);    
    printf("Min number of rules for covering = %d\n", min_rule);
    printf("Min number of correct matches for covering = %d\n", min_correct_match);
    printf("Min number of matches for covering = %d\n", min_match);
    printf("Min / Max offset % = %d\n", minmax_offset);
    printf("Experience threshold for deleting rules = %d\n", min_del_exp);
    printf("Min Sensitivity for rule deletion = %f\n", min_del_sen);
    printf("*****************************************************\n");
    // Main processing loop
    while(1)
    {
        usleep(100000);    // Allow CPU to sleep (0.1s)
        fill_headers();
        if (inst_count > (vCount + tCount)) return 0;
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
            headers.rfFeatures.rate_idx = (headers.rfFeatures.rate_idx&0xDF);
            printf("(%d) buffer %d loaded - Time: %lld, MAC: %llX, Rate_IDX: %d, RSSI: %d, Phase Offset: %d, Pilot Offset: %d, MagSQ: %d \n", inst_count, i, headers.rfFeatures.timestamp, headers.mac80211.Addr2, headers.rfFeatures.rate_idx, headers.rfFeatures.rssi, headers.rfFeatures.phaseOffset, headers.rfFeatures.pilotOffset, headers.rfFeatures.magSq);
            matching();
            pk_buffer->buffer[i].type = PB_EMPTY;
            inst_count++;
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
    int mCount = 0;
    int cCount = 0;
    int tpCount = 0;
    int fpCount = 0;
    int fnCount = 0;
    int tnCount = 0;

    if ((inst_count < tCount) && vOnly == false)
    {
        // Trainign Algorithm
        if (flow_table->last_entry < min_rule) // ==0
        {
            printf("No rules so automatically applying covering\n");
            covering();
            return;
        } else {
            for(j=0;j<flow_table->last_entry;j++)
            {
                if (((flow_table->key[j].headers_rfFeatures_rate_idx_exact == headers.rfFeatures.rate_idx) || (flow_table->scope[j].headers_rfFeatures_rate_idx_exact == false)) &&
                    ((headers.rfFeatures.rssi < flow_table->key[j].headers_rfFeatures_rssi_max) || (flow_table->scope[j].headers_rfFeatures_rssi_max == false)) &&
                    ((headers.rfFeatures.rssi > flow_table->key[j].headers_rfFeatures_rssi_min) || (flow_table->scope[j].headers_rfFeatures_rssi_min == false)) &&
                    ((headers.rfFeatures.phaseOffset < flow_table->key[j].headers_rfFeatures_phaseOffset_max) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_max == false)) &&
                    ((headers.rfFeatures.phaseOffset > flow_table->key[j].headers_rfFeatures_phaseOffset_min) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_min == false)) &&
                    ((headers.rfFeatures.pilotOffset < flow_table->key[j].headers_rfFeatures_pilotOffset_max) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_max == false)) &&
                    ((headers.rfFeatures.pilotOffset >flow_table->key[j].headers_rfFeatures_pilotOffset_min) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_min == false)) &&
                    ((flow_table->key[j].headers_rfFeatures_magSq_max > headers.rfFeatures.magSq) || (flow_table->scope[j].headers_rfFeatures_magSq_max == false)) &&
                    ((headers.rfFeatures.magSq > flow_table->key[j].headers_rfFeatures_magSq_min) || (flow_table->scope[j].headers_rfFeatures_magSq_min == false)))
                {
                    mCount++;
                    if (flow_table->key[j].headers_mac80211_Addr2_class == headers.mac80211.Addr2)
                    {
                        // Add to correct & match list
                        cCount++;
                        printf("!!! True Positive !!! ");
                        // Update rule metadata
                        flow_table->rule_data[j].true_pos++;
                        if (flow_table->rule_data[j].true_pos > 0 || flow_table->rule_data[j].false_neg > 0) flow_table->rule_data[j].sensitivity = (float)flow_table->rule_data[j].true_pos / ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg);
                        if (flow_table->rule_data[j].true_neg > 0 || flow_table->rule_data[j].false_pos > 0) flow_table->rule_data[j].specificity = (float)flow_table->rule_data[j].true_neg / ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_pos);
                        print_rule(j);
                    } else {
                        // Add to match list
                        printf("!!! False Positive !!! ");
                        // Update rule metadata
                        flow_table->rule_data[j].false_pos++;
                        if (flow_table->rule_data[j].true_pos > 0 || flow_table->rule_data[j].false_neg > 0) flow_table->rule_data[j].sensitivity = (float)flow_table->rule_data[j].true_pos / ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg);
                        if (flow_table->rule_data[j].true_neg > 0 || flow_table->rule_data[j].false_pos > 0) flow_table->rule_data[j].specificity = (float)flow_table->rule_data[j].true_neg / ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_pos);
                        print_rule(j);
                    }
                } else {
                    if (flow_table->key[j].headers_mac80211_Addr2_class == headers.mac80211.Addr2)
                    {
                        // Incorrectly missed prediction - False Negitive
                        printf("!!! False Negitive !!! ");
                        flow_table->rule_data[j].false_neg++;
                        if (flow_table->rule_data[j].true_pos > 0 || flow_table->rule_data[j].false_neg > 0) flow_table->rule_data[j].sensitivity = (float)flow_table->rule_data[j].true_pos / ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg);
                        if (flow_table->rule_data[j].true_neg > 0 || flow_table->rule_data[j].false_pos > 0) flow_table->rule_data[j].specificity = (float)flow_table->rule_data[j].true_neg / ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_pos);
                        print_rule(j);
                    } else {
                        // Correct missed prediction - True Negitive
                        printf("!!! True Negitive !!! ");
                        flow_table->rule_data[j].true_neg++;
                        if (flow_table->rule_data[j].true_pos > 0 || flow_table->rule_data[j].false_neg > 0) flow_table->rule_data[j].sensitivity = (float)flow_table->rule_data[j].true_pos / ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg);
                        if (flow_table->rule_data[j].true_neg > 0 || flow_table->rule_data[j].false_pos > 0) flow_table->rule_data[j].specificity = (float)flow_table->rule_data[j].true_neg / ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_pos);
                        print_rule(j);
                    }
                }
                flow_table->rule_data[j].experience++;
                flow_table->rule_data[j].accuracy = ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].true_neg) / (float)flow_table->rule_data[j].experience;

                // Rule deletion
                if ((flow_table->rule_data[j].experience > min_del_exp) && (flow_table->rule_data[j].sensitivity < min_del_sen)) delete_rule(j);
            }
            printf(" Total correct/match count for training instance = %d/%d\n", cCount, mCount);
            if ((cCount < min_correct_match) || (mCount < min_match)) covering();
            return;
        }
    } else {
        // Prediction Algorithm
        for(j=0;j<flow_table->last_entry;j++)
        {
            if (((flow_table->key[j].headers_rfFeatures_rate_idx_exact == headers.rfFeatures.rate_idx) || (flow_table->scope[j].headers_rfFeatures_rate_idx_exact == false)) &&
                ((headers.rfFeatures.rssi < flow_table->key[j].headers_rfFeatures_rssi_max) || (flow_table->scope[j].headers_rfFeatures_rssi_max == false)) &&
                ((headers.rfFeatures.rssi > flow_table->key[j].headers_rfFeatures_rssi_min) || (flow_table->scope[j].headers_rfFeatures_rssi_min == false)) &&
                ((headers.rfFeatures.phaseOffset < flow_table->key[j].headers_rfFeatures_phaseOffset_max) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_max == false)) &&
                ((headers.rfFeatures.phaseOffset > flow_table->key[j].headers_rfFeatures_phaseOffset_min) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_min == false)) &&
                ((headers.rfFeatures.pilotOffset < flow_table->key[j].headers_rfFeatures_pilotOffset_max) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_max == false)) &&
                ((headers.rfFeatures.pilotOffset >flow_table->key[j].headers_rfFeatures_pilotOffset_min) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_min == false)) &&
                ((flow_table->key[j].headers_rfFeatures_magSq_max > headers.rfFeatures.magSq) || (flow_table->scope[j].headers_rfFeatures_magSq_max == false)) &&
                ((headers.rfFeatures.magSq > flow_table->key[j].headers_rfFeatures_magSq_min) || (flow_table->scope[j].headers_rfFeatures_magSq_min == false)))
            {
                if (flow_table->key[j].headers_mac80211_Addr2_class == headers.mac80211.Addr2)
                {
                    // Correct prediction - True Positive
                    tpCount++;
                } else {
                    // Incorrect prediction - False Positive
                    fpCount++;
                }
            } else {
                if (flow_table->key[j].headers_mac80211_Addr2_class == headers.mac80211.Addr2)
                {
                    // Incorrectly missed prediction - False Negitive
                    fnCount++;
                } else {
                    // Correct missed prediction - True Negitive
                    tnCount++;
                }
            }
        }
        // Outcome after each rule is evaluated
        printf("*** (TP: %d, FP:%d, TN: %d, FN: %d) ***\n", tpCount, fpCount, tnCount, fnCount);
    }
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
        printf("Full wild, no point adding this rule!\n");
        return; // Rule is all wildcards to no point adding to population
    }

    add_rule(&key, &scope);
    return;
}

void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope)
{
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

    flow_table->rule_data[flow_table->last_entry].time_added = time(NULL);
    flow_table->rule_data[flow_table->last_entry].experience = 0;
    flow_table->rule_data[flow_table->last_entry].true_pos = 0;
    flow_table->rule_data[flow_table->last_entry].true_neg = 0;
    flow_table->rule_data[flow_table->last_entry].false_pos = 0;
    flow_table->rule_data[flow_table->last_entry].false_neg = 0;
    flow_table->rule_data[flow_table->last_entry].sensitivity = 0;
    flow_table->rule_data[flow_table->last_entry].specificity = 0;
    flow_table->rule_data[flow_table->last_entry].accuracy = 0;

    //printf("New rule added at %d - MAC: %llX, Rate_IDX: %d(%d), RSSI: %d (%d) - %d(%d), Phase Offset: %d(%d) - %d(%d), Pilot Offset: %d(%d) - %d(%d), MagSQ: %d(%d) - %d(%d) \n\n", flow_table->last_entry, flow_table->key[flow_table->last_entry].headers_mac80211_Addr2_class, flow_table->key[flow_table->last_entry].headers_rfFeatures_rate_idx_exact, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact, flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max, flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_min, flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min, flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_max, flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max); 
    printf("New rule added at");
    print_rule(flow_table->last_entry);
    flow_table->last_entry++;
    return;
}

void delete_rule(int rule)
{
    printf("Rule deleted at");
    print_rule(rule);
    memcpy(&flow_table->key[rule], &flow_table->key[flow_table->last_entry-1], sizeof(struct swtch_lookup_tbl_key));
    memcpy(&flow_table->scope[rule], &flow_table->scope[flow_table->last_entry-1], sizeof(struct rule_scope));
    memcpy(&flow_table->rule_data[rule], &flow_table->rule_data[flow_table->last_entry-1], sizeof(struct rule_metadata));
    print_rule(flow_table->last_entry-1);
    memset(&flow_table->key[flow_table->last_entry-1], 0, sizeof(struct swtch_lookup_tbl_key));
    memset(&flow_table->scope[flow_table->last_entry-1], 0, sizeof(struct rule_scope));
    memset(&flow_table->rule_data[flow_table->last_entry-1], 0, sizeof(struct rule_metadata));
    flow_table->last_entry--;
    print_rule(flow_table->last_entry);
    print_rule(rule);
    return;
}

void print_rule(int rule)
{
    printf("%4d:", rule+1);
    printf(" (%d)", flow_table->rule_data[rule].time_added);
    printf(" %012llX", flow_table->key[rule].headers_mac80211_Addr2_class);
    flow_table->scope[rule].headers_rfFeatures_rate_idx_exact ? printf(" %2d", flow_table->key[rule].headers_rfFeatures_rate_idx_exact) : printf("  #");
    flow_table->scope[rule].headers_rfFeatures_rssi_min ? printf(" %3d", flow_table->key[rule].headers_rfFeatures_rssi_min) : printf("   #");
    flow_table->scope[rule].headers_rfFeatures_rssi_max ? printf(" %3d", flow_table->key[rule].headers_rfFeatures_rssi_max) : printf("   #");
    flow_table->scope[rule].headers_rfFeatures_phaseOffset_min ? printf(" %4d", flow_table->key[rule].headers_rfFeatures_phaseOffset_min) : printf("    #");
    flow_table->scope[rule].headers_rfFeatures_phaseOffset_max ? printf(" %4d", flow_table->key[rule].headers_rfFeatures_phaseOffset_max) : printf("    #");
    flow_table->scope[rule].headers_rfFeatures_pilotOffset_min ? printf(" %4d", flow_table->key[rule].headers_rfFeatures_pilotOffset_min) : printf("    #");
    flow_table->scope[rule].headers_rfFeatures_pilotOffset_max ? printf(" %4d", flow_table->key[rule].headers_rfFeatures_pilotOffset_max) : printf("    #");
    flow_table->scope[rule].headers_rfFeatures_magSq_min ? printf(" %6d", flow_table->key[rule].headers_rfFeatures_magSq_min) : printf("      #");
    flow_table->scope[rule].headers_rfFeatures_magSq_max ? printf(" %6d", flow_table->key[rule].headers_rfFeatures_magSq_max) : printf("      #");
    printf(" - E: %4d", flow_table->rule_data[rule].experience);
    printf(" TP: %4d", flow_table->rule_data[rule].true_pos);
    printf(" TN: %4d", flow_table->rule_data[rule].true_neg);
    printf(" FP: %4d", flow_table->rule_data[rule].false_pos);
    printf(" FN: %4d", flow_table->rule_data[rule].false_neg);
    printf(" SN: %.4f", flow_table->rule_data[rule].sensitivity);
    printf(" SP: %.4f", flow_table->rule_data[rule].specificity);
    printf(" AC: %.4f", flow_table->rule_data[rule].accuracy);
    printf("\n");
    return;
}