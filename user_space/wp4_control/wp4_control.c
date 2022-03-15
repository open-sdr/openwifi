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
#include <math.h>
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

#define FTPAGES     24
#define PBPAGES     24 
#define PAGE_SIZE   4096

int current_time;
struct classifier *flow_table;
struct pbuffer *pk_buffer;
int inst_count, tCount, vCount;
int min_rule, min_correct_match, max_class_rules, minmax_offset, min_del_exp, min_match_exp, debug_level, val_method, ul_inst;
int totalTruePos, totalFalseNeg, totalTrueNeg, totalFalsePos, totalTests, inExp;
float min_del_acc;
float min_del_fs;
float min_correct_acc;
bool fCapture, fRead, vOnly;
struct Headers_t headers;
time_t t;
FILE *capfile;
char line[256];


// Internal Functions
void clear_flowtable(void);
void fill_headers(void);
void train(void);
void cover(void);
void genetic(void);
void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope, int source);
void delete_rule(int rule);
void print_table(void);
void print_rule(int rule);

static inline uint64_t (_htonll)(uint64_t n)
{
    return htonl(1) == 1 ? n : ((uint64_t) htonl(n) << 32) | htonl(n >> 32);
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
    
    // Load config file
    FILE *file = fopen( "wp4_control.cfg", "r" );
    
    if ( file == 0 )
    {
        printf("No config file found!\n\n");
    } else {
        while(fgets(line, sizeof(line), file) != NULL)
        {
            if(line[0] == '#' || line[0] == '\n') continue;     // Ignore comments

            if (strstr(line, "debug") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        debug_level = atoi(token);
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

            if (strstr(line, "max_class_rules") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        max_class_rules = atoi(token);
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

            if (strstr(line, "min_del_acc") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_del_acc = atof(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }

            if (strstr(line, "min_match_exp") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_match_exp = atof(token);
                    }
                    token = strtok(NULL, " '\t\n");
                    token_cnt++;
                }
                token_cnt = 0;
            }
            
            if (strstr(line, "min_del_fs") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_del_fs = atof(token);
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
            
            if (strstr(line, "min_correct_acc") != NULL){
                token = strtok(line, " '\t\n");
                while( token != NULL )
                {
                    if (token_cnt == 1)
                    {
                        min_correct_acc = atof(token);
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
        while((opt=getopt(argc,argv,"cCRpv"))!=-1)
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
                case 'p':
                    // Print out the rule set
                    print_table();
                    return;
            }
        }
    }


    // print startup values
    
    printf("*****************************************************\n");
    if (fCapture == true) printf("Capturing frames to file\n");
    if (fRead == true) printf("Reading frames from file\n");  
    printf("Max rule count = %d\n", TABLE_SIZE);
    printf("Training frame count = %d\n", tCount);
    printf("Max number of rules per class = %d\n", max_class_rules);  
    printf("Min number of rules for covering = %d\n", min_rule);
    printf("Min number of correct matches for covering = %d\n", min_correct_match);
    printf("Min number of matches for covering = %d\n", min_match_exp);
    printf("Min / Max offset % = %d\n", minmax_offset);
    printf("Experience threshold for deleting rules = %d\n", min_del_exp);
    printf("Experience threshold for applying rules = %d\n", min_match_exp);
    printf("Min Accuracy for rule deletion = %f\n", min_del_acc);
    printf("Min F-score for rule deletion = %.3f\n", min_del_fs);
    printf("Min F-score percentage for a correct match = %.2f\n", min_correct_acc);
    printf("*****************************************************\n");
    printf("\n");

    if (fCapture == true)
    {
        capfile = fopen( "capture.csv", "a" );
        if (capfile == 0)
        {
            printf("Unable to open capture file!\n\n");
            return;
        } else {
            fprintf(capfile,"Time, MAC, Rate_IDX, RSSI, Phase Offset, Pilot Offset, MagSQ\n");
        }
    }

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
            headers.rfFeatures.rate_idx = (headers.rfFeatures.rate_idx&0xDF);
            if (debug_level > 0) printf("(%d) buffer %d loaded - Time: %lld, MAC: %llX, Rate_IDX: %d, RSSI: %d, Phase Offset: %d, Pilot Offset: %d, MagSQ: %d \n", inst_count, i, headers.rfFeatures.timestamp, headers.mac80211.Addr2, headers.rfFeatures.rate_idx, headers.rfFeatures.rssi, headers.rfFeatures.phaseOffset, headers.rfFeatures.pilotOffset, headers.rfFeatures.magSq);
            if (fCapture == true)
            {
                fprintf(capfile,"%lld,%llX,%d,%d,%d,%d,%d\n", headers.rfFeatures.timestamp, headers.mac80211.Addr2, headers.rfFeatures.rate_idx, headers.rfFeatures.rssi, headers.rfFeatures.phaseOffset, headers.rfFeatures.pilotOffset, headers.rfFeatures.magSq);
                fflush(capfile);
            } else {
                train();
            }
            pk_buffer->buffer[i].type = PB_EMPTY;
            inst_count++;
        }
    }
    return;
}

/*
 *  Evaluate an instance from the training set
 */
void train(void)
{
    int j;
    int mCount = 0;
    int cCount = 0;
    float tpScore = 0;
    float fnScore = 0;
    float total_acc = 0;

    if (flow_table->last_entry < min_rule) // ==0
    {
        printf("No rules so automatically applying covering\n");
        cover();
        return;
    } else {
        for(j=0;j<flow_table->last_entry;j++)
        {
            if (flow_table->key[j].headers_mac80211_Addr2_class == headers.mac80211.Addr2)
            {
                mCount++;
                if (((flow_table->key[j].headers_rfFeatures_rate_idx_exact == headers.rfFeatures.rate_idx) || (flow_table->scope[j].headers_rfFeatures_rate_idx_exact == false)) &&
                    ((headers.rfFeatures.rssi < flow_table->key[j].headers_rfFeatures_rssi_max) || (flow_table->scope[j].headers_rfFeatures_rssi_max == false)) &&
                    ((headers.rfFeatures.rssi > flow_table->key[j].headers_rfFeatures_rssi_min) || (flow_table->scope[j].headers_rfFeatures_rssi_min == false)) &&
                    ((headers.rfFeatures.phaseOffset < flow_table->key[j].headers_rfFeatures_phaseOffset_max) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_max == false)) &&
                    ((headers.rfFeatures.phaseOffset > flow_table->key[j].headers_rfFeatures_phaseOffset_min) || (flow_table->scope[j].headers_rfFeatures_phaseOffset_min == false)) &&
                    ((headers.rfFeatures.pilotOffset < flow_table->key[j].headers_rfFeatures_pilotOffset_max) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_max == false)) &&
                    ((headers.rfFeatures.pilotOffset > flow_table->key[j].headers_rfFeatures_pilotOffset_min) || (flow_table->scope[j].headers_rfFeatures_pilotOffset_min == false)) &&
                    ((headers.rfFeatures.magSq < flow_table->key[j].headers_rfFeatures_magSq_max) || (flow_table->scope[j].headers_rfFeatures_magSq_max == false)) &&
                    ((headers.rfFeatures.magSq > flow_table->key[j].headers_rfFeatures_magSq_min) || (flow_table->scope[j].headers_rfFeatures_magSq_min == false)))
                {
                    cCount++;
                    if (debug_level > 1) printf("!!! True Positive !!! ");
                    flow_table->rule_data[j].true_pos++;
                    tpScore += flow_table->rule_data[j].f_score;
                    if (debug_level > 1) print_rule(j);
                } else {
                    if (debug_level > 1) printf("!!! False Negative !!! ");
                    flow_table->rule_data[j].false_neg++;
                    fnScore += flow_table->rule_data[j].f_score;
                    if (debug_level > 1 ) print_rule(j);
                }
                // Update metadata
                flow_table->rule_data[j].experience++;
                if (flow_table->rule_data[j].true_pos > 0 || flow_table->rule_data[j].false_neg > 0) flow_table->rule_data[j].sensitivity = (float)flow_table->rule_data[j].true_pos / ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg);
                if (flow_table->rule_data[j].true_neg > 0 || flow_table->rule_data[j].false_pos > 0) flow_table->rule_data[j].specificity = (float)flow_table->rule_data[j].true_neg / ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_pos);
                flow_table->rule_data[j].accuracy = ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].true_neg) / (float)flow_table->rule_data[j].experience;         // (TP+TN) / EXP
                flow_table->rule_data[j].f_score = (float)flow_table->rule_data[j].true_pos / (((float)flow_table->rule_data[j].true_pos + (0.5*((float)flow_table->rule_data[j].false_pos + (float)flow_table->rule_data[j].false_neg))));   // TP / TP + 0.5(FP+FN)
                flow_table->rule_data[j].mcc = (((float)flow_table->rule_data[j].true_pos * (float)flow_table->rule_data[j].true_neg) - ((float)flow_table->rule_data[j].false_pos * (float)flow_table->rule_data[j].false_neg)) / (sqrtf(((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_pos) * ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_neg) * ((float)flow_table->rule_data[j].true_pos + (float)flow_table->rule_data[j].false_pos) * ((float)flow_table->rule_data[j].true_neg + (float)flow_table->rule_data[j].false_neg)));          // ((TP * TN) - (FP * FN)) / SQRT((TP + FP) * (TP + FN) * (TN + FP) * (TN + FN))

                 // Rule deletion
                 if ((flow_table->rule_data[j].experience > min_del_exp) && (flow_table->rule_data[j].accuracy < min_del_acc)) delete_rule(j);
                 if ((flow_table->rule_data[j].experience > min_del_exp) && ((flow_table->rule_data[j].f_score < min_del_fs) || (flow_table->rule_data[j].f_score !=flow_table->rule_data[j].f_score))) delete_rule(j);  // If F-score is below min_del_fs or equals NaN
            }
        }
        if ((cCount < min_correct_match) && (mCount < max_class_rules) && (headers.mac80211.Addr2 != 0)) cover();
        if ((mCount > 10) && (mCount < max_class_rules)) genetic();
        total_acc = (((float)tpScore + 0.01 / ((float)fnScore + (float)tpScore + 0.01)*100));
        printf(" %012llX,%d,%d,%.2f,%.2f\n", headers.mac80211.Addr2, cCount, mCount,((float)cCount/(float)mCount)*100, total_acc);
        return;
    }
}

/*
 *  Covering add new rules when the number of rules for a class is below max_class_rules AND number of correct matches is below min_correct_match
 */
void cover(void)
{
    struct swtch_lookup_tbl_key key;
    struct rule_scope scope;
    int rand_offset = minmax_offset;
    // Populate a rule and call add_rule function
    key.headers_mac80211_Addr2_class = headers.mac80211.Addr2;
    key.headers_rfFeatures_rate_idx_exact = headers.rfFeatures.rate_idx;
    key.headers_rfFeatures_phaseOffset_max = headers.rfFeatures.phaseOffset + abs((headers.rfFeatures.phaseOffset * rand_offset)/100);
    key.headers_rfFeatures_phaseOffset_min = headers.rfFeatures.phaseOffset - abs((headers.rfFeatures.phaseOffset * rand_offset)/100);
    key.headers_rfFeatures_rssi_max = headers.rfFeatures.rssi + abs((headers.rfFeatures.rssi * rand_offset)/100);
    key.headers_rfFeatures_rssi_min = headers.rfFeatures.rssi - abs((headers.rfFeatures.rssi * rand_offset)/100);
    key.headers_rfFeatures_pilotOffset_max = headers.rfFeatures.pilotOffset + abs((headers.rfFeatures.pilotOffset * rand_offset)/100);
    key.headers_rfFeatures_pilotOffset_min = headers.rfFeatures.pilotOffset - abs((headers.rfFeatures.pilotOffset * rand_offset)/100);
    key.headers_rfFeatures_magSq_max = headers.rfFeatures.magSq + abs((headers.rfFeatures.magSq * rand_offset)/100);
    key.headers_rfFeatures_magSq_min = headers.rfFeatures.magSq - abs((headers.rfFeatures.magSq * rand_offset)/100);

    scope.headers_mac80211_Addr2_class = 1;    // Class is always in scope
    scope.headers_rfFeatures_rate_idx_exact = 0;    // Rate IDX is always out of scope as it is unreliable.
    //scope.headers_rfFeatures_rate_idx_exact = rand() & 1;


    scope.headers_rfFeatures_rssi_max = rand() & 1;
    scope.headers_rfFeatures_rssi_min = rand() & 1;
    //scope.headers_rfFeatures_rssi_max = 0;
    //scope.headers_rfFeatures_rssi_min = 0;
    

    scope.headers_rfFeatures_phaseOffset_max = rand() & 1;
    scope.headers_rfFeatures_phaseOffset_min = rand() & 1;
    //scope.headers_rfFeatures_phaseOffset_max = 0;
    //scope.headers_rfFeatures_phaseOffset_min = 0;
    
    
    //scope.headers_rfFeatures_pilotOffset_max = rand() & 1;
    //scope.headers_rfFeatures_pilotOffset_min = rand() & 1;
    scope.headers_rfFeatures_pilotOffset_max = 0;
    scope.headers_rfFeatures_pilotOffset_min = 0;
    
    scope.headers_rfFeatures_magSq_max = rand() & 1;
    scope.headers_rfFeatures_magSq_min = rand() & 1;
    //scope.headers_rfFeatures_magSq_max = 0;
    //scope.headers_rfFeatures_magSq_min = 0;

    if ((scope.headers_rfFeatures_rate_idx_exact == 0) && (scope.headers_rfFeatures_phaseOffset_max == 0) && (scope.headers_rfFeatures_phaseOffset_min == 0) && (scope.headers_rfFeatures_rssi_max == 0) && (scope.headers_rfFeatures_rssi_min == 0) && (scope.headers_rfFeatures_pilotOffset_max == 0) && (scope.headers_rfFeatures_pilotOffset_min == 0) && (scope.headers_rfFeatures_magSq_max == 0) && (scope.headers_rfFeatures_magSq_min == 0))
    {
        if (debug_level > 0) printf("Full wild, no point adding this rule!\n");
        return; // Rule is all wildcards to no point adding to population
    }
    
    add_rule(&key, &scope, 0);
    return;
}

/*
 *  Add new rules using Genetic Algorithm
 */
void genetic(void)
{
    int p1 = -1;
    int p2 = -1;
    float acc = 0;
    uint64_t class = 0;
    int j;
    
    for(j=0;j<flow_table->last_entry;j++)
    {
        if ((rand() & 1) == true  && flow_table->rule_data[j].accuracy > acc && flow_table->rule_data[j].experience > min_del_exp)
        {
            if (class == 0)
            {
                if (headers.mac80211.Addr2 == flow_table->key[j].headers_mac80211_Addr2_class) class = flow_table->key[j].headers_mac80211_Addr2_class;
                p1 = j;
            } else if (class == flow_table->key[j].headers_mac80211_Addr2_class)
            {
                p2 = p1;
                p1 = j;
                acc = flow_table->rule_data[j].accuracy;
                class = flow_table->key[j].headers_mac80211_Addr2_class;
            }
        }
    }
    
    if (p1 == -1 || p2 == -1) return;
    
    if (debug_level > 0) printf("Parent 1 ");
    if (debug_level > 0) print_rule(p1);
    if (debug_level > 0) printf("Parent 2 ");
    if (debug_level > 0) print_rule(p2);
    
    // Create offspring 1
    flow_table->key[flow_table->last_entry].headers_mac80211_Addr2_class = flow_table->key[p1].headers_mac80211_Addr2_class;
    flow_table->scope[flow_table->last_entry].headers_mac80211_Addr2_class = flow_table->scope[p1].headers_mac80211_Addr2_class;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = flow_table->key[p1].headers_rfFeatures_rate_idx_exact;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = flow_table->scope[p1].headers_rfFeatures_rate_idx_exact;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = rand() & 1;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_max = flow_table->key[p1].headers_rfFeatures_rssi_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_min = flow_table->key[p1].headers_rfFeatures_rssi_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = flow_table->key[p1].headers_rfFeatures_phaseOffset_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = flow_table->key[p1].headers_rfFeatures_phaseOffset_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = flow_table->key[p2].headers_rfFeatures_pilotOffset_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = flow_table->key[p2].headers_rfFeatures_pilotOffset_min;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = 0;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_max = flow_table->key[p2].headers_rfFeatures_magSq_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_min = flow_table->key[p2].headers_rfFeatures_magSq_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min = 0;
    
    if ((flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min == 0))
    {
        return; // Rule is all wildcards to no point adding to population
    }
    
    flow_table->rule_data[flow_table->last_entry].time_added = time(NULL);
    flow_table->rule_data[flow_table->last_entry].source = 1;
    flow_table->rule_data[flow_table->last_entry].experience = 0;
    flow_table->rule_data[flow_table->last_entry].true_pos = 0;
    flow_table->rule_data[flow_table->last_entry].true_neg = 0;
    flow_table->rule_data[flow_table->last_entry].false_pos = 0;
    flow_table->rule_data[flow_table->last_entry].false_neg = 0;
    flow_table->rule_data[flow_table->last_entry].sensitivity = 0;
    flow_table->rule_data[flow_table->last_entry].specificity = 0;
    flow_table->rule_data[flow_table->last_entry].accuracy = 0;
    flow_table->rule_data[flow_table->last_entry].f_score = 0;
    flow_table->rule_data[flow_table->last_entry].mcc = 0;
    
    if (debug_level > 0) printf("1 (%d-%d) - New rule added at", p1,p2);
    if (debug_level > 0) print_rule(flow_table->last_entry);
    flow_table->last_entry++;

    // Create offspring 2
    flow_table->key[flow_table->last_entry].headers_mac80211_Addr2_class = flow_table->key[p2].headers_mac80211_Addr2_class;
    flow_table->scope[flow_table->last_entry].headers_mac80211_Addr2_class = flow_table->scope[p2].headers_mac80211_Addr2_class;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = flow_table->key[p2].headers_rfFeatures_rate_idx_exact;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = flow_table->scope[p1].headers_rfFeatures_rate_idx_exact;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact = rand() & 1;
 
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_max = flow_table->key[p2].headers_rfFeatures_rssi_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_rssi_min = flow_table->key[p2].headers_rfFeatures_rssi_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = flow_table->key[p2].headers_rfFeatures_phaseOffset_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = flow_table->key[p2].headers_rfFeatures_phaseOffset_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = flow_table->key[p1].headers_rfFeatures_pilotOffset_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = flow_table->key[p1].headers_rfFeatures_pilotOffset_min;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max = 0;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min = 0;
    
    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_max = flow_table->key[p1].headers_rfFeatures_magSq_max;
    flow_table->key[flow_table->last_entry].headers_rfFeatures_magSq_min = flow_table->key[p1].headers_rfFeatures_magSq_min;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min = rand() & 1;
    flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max = rand() & 1;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min = 0;
    //flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max = 0;
    
    if ((flow_table->scope[flow_table->last_entry].headers_rfFeatures_rate_idx_exact == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_phaseOffset_min ==0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_rssi_min == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_pilotOffset_min == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_max == 0) && (flow_table->scope[flow_table->last_entry].headers_rfFeatures_magSq_min == 0))
    {
        return; // Rule is all wildcards to no point adding to population
    }
    
    flow_table->rule_data[flow_table->last_entry].time_added = time(NULL);
    flow_table->rule_data[flow_table->last_entry].source = 1;
    flow_table->rule_data[flow_table->last_entry].experience = 0;
    flow_table->rule_data[flow_table->last_entry].true_pos = 0;
    flow_table->rule_data[flow_table->last_entry].true_neg = 0;
    flow_table->rule_data[flow_table->last_entry].false_pos = 0;
    flow_table->rule_data[flow_table->last_entry].false_neg = 0;
    flow_table->rule_data[flow_table->last_entry].sensitivity = 0;
    flow_table->rule_data[flow_table->last_entry].specificity = 0;
    flow_table->rule_data[flow_table->last_entry].accuracy = 0;
    flow_table->rule_data[flow_table->last_entry].f_score = 0;
    flow_table->rule_data[flow_table->last_entry].mcc = 0;
    
    if (debug_level > 0) printf("2 (%d-%d) - New rule added at", p1,p2);
    if (debug_level > 0) print_rule(flow_table->last_entry);
    flow_table->last_entry++;
    
    return;
}


void add_rule(struct swtch_lookup_tbl_key *key, struct rule_scope *scope, int source)
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
    flow_table->rule_data[flow_table->last_entry].source = source;
    flow_table->rule_data[flow_table->last_entry].experience = 0;
    flow_table->rule_data[flow_table->last_entry].true_pos = 0;
    flow_table->rule_data[flow_table->last_entry].true_neg = 0;
    flow_table->rule_data[flow_table->last_entry].false_pos = 0;
    flow_table->rule_data[flow_table->last_entry].false_neg = 0;
    flow_table->rule_data[flow_table->last_entry].sensitivity = 0;
    flow_table->rule_data[flow_table->last_entry].specificity = 0;
    flow_table->rule_data[flow_table->last_entry].accuracy = 0;
    flow_table->rule_data[flow_table->last_entry].f_score = 0;
    flow_table->rule_data[flow_table->last_entry].mcc = 0;

    if (debug_level > 0) printf("New rule added at");
    if (debug_level > 0) print_rule(flow_table->last_entry);
    flow_table->last_entry++;
    return;
}

void clear_flowtable(void)
{
    printf("Rule table and buffer cleared!\n\n");
    memset(flow_table, 0, FTPAGES * PAGE_SIZE);
    memset(pk_buffer, 0, PBPAGES * PAGE_SIZE);

}

void delete_rule(int rule)
{
    if (debug_level > 0) printf("Rule deleted at");
    if (debug_level > 0) print_rule(rule);
    memcpy(&flow_table->key[rule], &flow_table->key[flow_table->last_entry-1], sizeof(struct swtch_lookup_tbl_key));
    memcpy(&flow_table->scope[rule], &flow_table->scope[flow_table->last_entry-1], sizeof(struct rule_scope));
    memcpy(&flow_table->rule_data[rule], &flow_table->rule_data[flow_table->last_entry-1], sizeof(struct rule_metadata));
    if (debug_level > 1) print_rule(flow_table->last_entry-1);
    memset(&flow_table->key[flow_table->last_entry-1], 0, sizeof(struct swtch_lookup_tbl_key));
    memset(&flow_table->scope[flow_table->last_entry-1], 0, sizeof(struct rule_scope));
    memset(&flow_table->rule_data[flow_table->last_entry-1], 0, sizeof(struct rule_metadata));
    flow_table->last_entry--;
    if (debug_level > 1) print_rule(flow_table->last_entry);
    if (debug_level > 1) print_rule(rule);
    return;
}

void print_table(void)
{
    int j;
    printf("\nRule table\n");
    printf("----------\n");
    for(j=0;j<flow_table->last_entry;j++)
    {
        print_rule(j);
    }
    printf("\n");
}

void print_rule(int rule)
{
    printf("%4d:", rule+1);
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
    printf(" - E: %6d", flow_table->rule_data[rule].experience);
    printf(" TP: %6d", flow_table->rule_data[rule].true_pos);
    printf(" TN: %6d", flow_table->rule_data[rule].true_neg);
    printf(" FP: %6d", flow_table->rule_data[rule].false_pos);
    printf(" FN: %6d", flow_table->rule_data[rule].false_neg);
    printf(" SN: %.3f", flow_table->rule_data[rule].sensitivity);
    printf(" SP: %.3f", flow_table->rule_data[rule].specificity);
    printf(" AC: %.3f", flow_table->rule_data[rule].accuracy);
    printf(" FS: %6.3f", flow_table->rule_data[rule].f_score);
    printf(" MCC: %.3f", flow_table->rule_data[rule].mcc);
    if (flow_table->rule_data[rule].source == 0) printf(" C");
    if (flow_table->rule_data[rule].source == 1) printf(" G");
    printf("\n");
    return;
}