#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char * file = NULL:

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
    unsigned long long int address;
    unsigned long timestamp;
} line_t;

typedef line_t * set_t;
typedef set_t * cache_t;

unsigned long long int set_index_mask;

void initCache() {
    int i, j;
    cache = (set_t*)malloc(sizeof(set_t) * S);
    for (i = 0; i < S; i++) {
        cache[i] = (line_t*)malloc(sizeof(line_t) * E);
        for (j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
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

void accessData(mem_addr_t addr) {
    int i;
    unsigned long long int eviction_lru = ULONG_MAX;
    unsigned int eviction_line = 0;
    mem_addr_t set_index = (addr >> b) & set_index_mask;
    mem_addr_t tag = addr >> (s + b);

    cache_set_t cache_set = cache[set_index];

    for (i = 0; i < E; ++i) {
        if (cache_set[i].valid) {
            if (cache_set[i].tag == tag) {
                cache_set[i].timestamp = timestamp++;
                hits++;
                return;
            }
        }
    }

    miss_count++;

    for (int i = 0; i < E; ++i) {
        if (eviction_lru > cache_set[i].timestamp) {
            eviction_line = i;
            eviction_lru = cache_set[i].timestamp;
        }
    }

    if (cache_set[eviction_line].valid) {
        evictions++;
    }

    cache_set[eviction_line].valid = 1;
    cache_set[eviction_line].tag = tag;
    cache_set[eviction_line].lru = lru_counter++;
}

void replayTrace(char* trace_fn) {
    FILE* trace_fp = fopen(trace_fn, "r");
    char trace_cmd;
    mem_addr_t address;
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
        switch (c) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                file = optarg;
                break;
            case 'v':
                break;
            // case 'h':
            //     printUsage(argv);
            //     exit(0);
            default:
                printUsage(argv);
                exit(1);
        }
    }

    // /* Make sure that all required command line args were specified */
    // if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
    //     printf("%s: Missing required command line argument\n", argv[0]);
    //     printUsage(argv);
    //     exit(1);
    // }

    /* Compute S, E and B from command line args */
    S = (unsigned int)(1 << s);
    B = (unsigned int)(1 << b);

    /* Initialize cache */
    initCache();

    replayTrace(file);

    /* Free allocated memory */
    freeCache();

    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count,dirty_active,dirty_active,double_accesses);
    return 0;
}