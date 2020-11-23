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
int B = 0;
int S = 0;

unsigned long long timestamp = 0;

typedef struct line {
    char valid;
    char dirty;
    int size;
    unsigned long long tag;
    unsigned long timestamp;
} line_t;

typedef line_t * set_t;
typedef set_t * cache_t;

unsigned long long mask;

cache_t cache;

void missed(set_t set, unsigned long long tag) {
    unsigned long long smallest_timestamp = ULONG_MAX;
    unsigned int line = 0;

    misses++;

    for (int i = 0; i < E; ++i) {
        if (smallest_timestamp > set[i].timestamp) {
            line = i;
            smallest_timestamp = set[i].timestamp;
        }
    }

    if (set[line].valid) {
        evictions++;
    }

    if (set[line].dirty) {
        dirty_evicted += set[line].size;
        dirty_active -= set[line].size;
    }

    set[line].valid = 1;
    set[line].tag = tag;
    set[line].timestamp = timestamp;
    timestamp++;
}

void store(unsigned long long addr, int size) {
    unsigned long long set_index = (addr >> b) & mask;
    unsigned long long tag = addr >> (s + b);

    set_t set = cache[set_index];
    int found = 0;

    for (int line = 0; line < E; ++line) {
        if (set[line].valid && set[line].tag == tag) {
            set[line].timestamp = timestamp;
            set[line].dirty = 1;
            set[line].size = size;
            dirty_active += size;
            hits++;
            timestamp++;
            found = 1;
            break;
        }
    }
    if (found == 0) {
        missed(set, tag);
    }
}

void load(unsigned long long addr, int size) {
    unsigned long long set_index = (addr >> b) & mask;
    unsigned long long tag = addr >> (s + b);

    set_t set = cache[set_index];
    int found = 0;

    for (int line = 0; line < E; ++line) {
        if (set[line].valid == 1 && set[line].tag == tag) {
            set[line].timestamp = timestamp;
            hits++;
            timestamp++;
            found = 1;
            break;
        }
    };
    if (found == 0) {
        missed(set, tag);
    }
}

void run(char* fileName) {
    FILE* opened_file = fopen(fileName, "r");
    char operation;
    int size;
    unsigned long long address;


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

int main(int argc, char * argv[]) {
    char arg;

    while ((arg = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        if (arg == 's') {
            s = atoi(optarg);
        }
        else if (arg == 'E') {
            E = atoi(optarg);
        }
        else if (arg == 'b') {
            b = atoi(optarg);
        }
        else if (arg == 't') {
            file = optarg;
        }
        else {
            exit(1);
        }
    }

    mask = (1 << s) - 1;

    S = 1 << s;
    B = 1 << b;

    cache = (set_t *) malloc(sizeof(set_t) * S);

    for (int set = 0; set < S; set++) {
        cache[set] = (line_t *) malloc(sizeof(line_t) * E);
        for (int line = 0; line < E; line++) {
            cache[set][line].valid = 0;
            cache[set][line].dirty = 0;
            cache[set][line].size = 0;
            cache[set][line].tag = 0;
            cache[set][line].timestamp = 0;
        }
    }

    run(file);

    for (int set = 0; set < S; set++) {
        free(cache[set]);
    }
    free(cache);

    printSummary(hits, misses, evictions, dirty_evicted, dirty_active, double_accesses);

    return 0;
}