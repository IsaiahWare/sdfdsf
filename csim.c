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
    char dirty;
    int size;
    unsigned long long int tag;
    unsigned long timestamp;
} line_t;

typedef line_t * set_t;
typedef set_t * cache_t;

unsigned long long int mask;

cache_t cache;

void missed(set_t set, unsigned long long int tag) {
    unsigned long long int eviction_lru = ULONG_MAX;
    unsigned int eviction_line = 0;

    misses++;

    for (int i = 0; i < E; ++i) {
        if (eviction_lru > set[i].timestamp) {
            eviction_line = i;
            eviction_lru = set[i].timestamp;
        }
    }

    if (set[eviction_line].valid) {
        evictions++;
    }

    if (set[eviction_line].dirty) {
        dirty_evicted += set[eviction_line].size;
        dirty_active -= set[eviction_line].size;
    }

    set[eviction_line].valid = 1;
    set[eviction_line].tag = tag;
    set[eviction_line].timestamp = timestamp++;
}

void store(unsigned long long int addr, int size) {
    unsigned long long int set_index = (addr >> b) & mask;
    unsigned long long int tag = addr >> (s + b);

    set_t set = cache[set_index];

    for (int line = 0; line < E; ++line) {
        if (set[line].valid) {
            if (set[line].tag == tag) {
                set[line].timestamp = timestamp++;
                set[line].dirty = 1;
                set[line].size = size;
                dirty_active += size;
                hits++;
                return;
            }
        }
    }

    missed(set, tag);
}

void load(unsigned long long int addr, int size) {
    unsigned long long int set_index = (addr >> b) & mask;
    unsigned long long int tag = addr >> (s + b);

    set_t set = cache[set_index];

    for (int line = 0; line < E; ++line) {
        if (set[line].valid == 1 && set[line].tag == tag) {
            set[line].timestamp = timestamp++;
            hits++;
            return;
        }
    }

    missed(set, tag);
}

void run(char* fileName) {
    FILE* opened_file = fopen(fileName, "r");
    char operation;
    int size;
    unsigned long long int address;


    while (fscanf(opened_file, " %c %llx,%d", &operation, &address, &size) == 3) {
        if (operation == 'L') {
            load(address, size);
        }
        else if (operation == 'S') {
            store(address, size);
        }
        else if (operation == 'M') {
            load(address, size);
            store(address, size);
        }
    }

    fclose(opened_file);
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

    mask = (unsigned long long int )((1 << s)- 1);

    S = (unsigned int)(1 << s);
    B = (unsigned int)(1 << b);

    cache = (set_t*)malloc(sizeof(set_t) * S);
    for (int i = 0; i < S; i++) {
        cache[i] = (line_t*)malloc(sizeof(line_t) * E);
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].dirty = 0;
            cache[i][j].size = 0;
            cache[i][j].tag = 0;
            cache[i][j].timestamp = 0;
        }
    }

    run(file);

    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);

    printSummary(hits, misses, evictions, dirty_evicted, dirty_active, double_accesses);

    return 0;
}