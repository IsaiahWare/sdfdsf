#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cachelab.h"

char * file = NULL;

int misses = 0;
int hits = 0;
int evictions = 0;
int dirty_evicted = 0;
int dirty_active = 0;
int double_accesses = 0;

int b = 0;
int s = 0;
int E = 0;

int B;
int S;

unsigned long long int timestamp = 0;

typedef struct line {
    char valid;
    unsigned long long int tag;
    unsigned long timestamp;
} line_t;

typedef line_t * set_t;
typedef set_t * cache_t;

unsigned long long int set_index_mask;

cache_t cache;

void initCache() {
    int i, j;
    cache = (set_t*)malloc(sizeof(set_t) * S);
    for (i = 0; i < S; i++) {
        cache[i] = (line_t*)malloc(sizeof(line_t) * E);
        for (j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].timestamp = 0;
        }
    }

    /* Computes set index mask */
    set_index_mask = (unsigned long long int )((1 << s)- 1);
}

void freeCache() {
    int i;
    for (i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

void accessData(unsigned long long int addr) {
    int i;
    unsigned int eviction_line = 0;
    unsigned long long int set_index = (addr >> b) & set_index_mask;
    unsigned long long int tag = addr >> (s + b);

    set_t set = cache[set_index];

    for (i = 0; i < E; ++i) {
        if (set[i].valid) {
            if (set[i].tag == tag) {
                set[i].timestamp = timestamp++;
                hits++;
                return;
            }
        }
    }

    misses++;

    unsigned long long int eviction_lru = ULONG_MAX;

    for (int i = 0; i < E; ++i) {
        if (eviction_lru > set[i].timestamp) {
            eviction_line = i;
            eviction_lru = set[i].timestamp;
        }
    }

    if (set[eviction_line].valid) {
        evictions++;
    }

    set[eviction_line].valid = 1;
    set[eviction_line].tag = tag;
    set[eviction_line].timestamp = timestamp++;
}

void replayTrace(char* trace_fn) {
    FILE* trace_fp = fopen(trace_fn, "r");
    char trace_cmd;
    unsigned long long int address;
    int size;

    while (fscanf(trace_fp, " %c %llx,%d", &trace_cmd, &address, &size) == 3) {
        switch(trace_cmd) {
            case 'L': accessData(address); break;
            case 'S': accessData(address); break;
            case 'M': accessData(address); accessData(address); break;
            default: break;
        }
    }

    fclose(trace_fp);
}

int main(int argc, char* argv[]) {
    char c;

    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        if (c == 's') {
            s = atoi(optarg);
        }
        else if (c == 'E') {
            E = atoi(optarg);
        }
        else if (c == 'b') {
            b = atoi(optarg);
        }
        else if (c == 't') {
            file = optarg;
        }
        else {
            exit(1);
        }
    }

    /* Compute S, E and B from command line args */
    S = (unsigned int)(1 << s);
    B = (unsigned int)(1 << b);

    initCache();
    replayTrace(file);
    freeCache();
    printSummary(hits, misses, evictions, dirty_evicted, dirty_active, double_accesses);

    return 0;
}