#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cachelab.h"

typedef struct {
    uint64_t tag;
    uint64_t last_used;
    int valid;
} cache_line_t;

typedef struct {
    cache_line_t *lines;
    size_t set_count;
    int lines_per_set;
    int set_bits;
    int block_bits;
    uint64_t clock;
    int hits;
    int misses;
    int evictions;
    int verbose;
} cache_t;

static void usage(const char *program)
{
    printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", program);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
}

static int parse_nonnegative_int(const char *text, int *value)
{
    char *end;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || *text == '\0' || *end != '\0' ||
        parsed < 0 || parsed > INT_MAX) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static void access_cache(cache_t *cache, uint64_t address)
{
    uint64_t set_mask;
    uint64_t set_index;
    uint64_t tag;
    cache_line_t *set;
    cache_line_t *empty = NULL;
    cache_line_t *lru = NULL;
    int i;

    set_mask = cache->set_bits == 0 ? 0 : ((UINT64_C(1) << cache->set_bits) - 1);
    set_index = (address >> cache->block_bits) & set_mask;
    tag = address >> (cache->block_bits + cache->set_bits);
    set = cache->lines + (size_t)set_index * (size_t)cache->lines_per_set;
    cache->clock++;

    for (i = 0; i < cache->lines_per_set; i++) {
        if (set[i].valid && set[i].tag == tag) {
            cache->hits++;
            set[i].last_used = cache->clock;
            if (cache->verbose) {
                printf(" hit");
            }
            return;
        }
        if (!set[i].valid && empty == NULL) {
            empty = &set[i];
        }
        if (set[i].valid && (lru == NULL || set[i].last_used < lru->last_used)) {
            lru = &set[i];
        }
    }

    cache->misses++;
    if (cache->verbose) {
        printf(" miss");
    }
    if (empty != NULL) {
        empty->valid = 1;
        empty->tag = tag;
        empty->last_used = cache->clock;
    } else {
        cache->evictions++;
        lru->tag = tag;
        lru->last_used = cache->clock;
        if (cache->verbose) {
            printf(" eviction");
        }
    }
}

int main(int argc, char **argv)
{
    cache_t cache = {0};
    const char *trace_path = NULL;
    FILE *trace;
    char buffer[256];
    char operation;
    unsigned long long address;
    int size;
    int option;
    int have_s = 0;
    int have_e = 0;
    int have_b = 0;
    size_t line_count;

    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'v':
            cache.verbose = 1;
            break;
        case 's':
            have_s = parse_nonnegative_int(optarg, &cache.set_bits);
            break;
        case 'E':
            have_e = parse_nonnegative_int(optarg, &cache.lines_per_set);
            break;
        case 'b':
            have_b = parse_nonnegative_int(optarg, &cache.block_bits);
            break;
        case 't':
            trace_path = optarg;
            break;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (!have_s || !have_e || !have_b || trace_path == NULL ||
        cache.lines_per_set <= 0 || cache.set_bits >= 63 ||
        cache.block_bits >= 64 || cache.set_bits + cache.block_bits >= 64) {
        fprintf(stderr, "%s: Missing or invalid required command line argument\n", argv[0]);
        usage(argv[0]);
        return 1;
    }

    cache.set_count = (size_t)UINT64_C(1) << cache.set_bits;
    if ((size_t)cache.lines_per_set > SIZE_MAX / cache.set_count) {
        fprintf(stderr, "%s: Cache dimensions are too large\n", argv[0]);
        return 1;
    }
    line_count = cache.set_count * (size_t)cache.lines_per_set;
    cache.lines = calloc(line_count, sizeof(*cache.lines));
    if (cache.lines == NULL) {
        fprintf(stderr, "%s: Unable to allocate cache\n", argv[0]);
        return 1;
    }

    trace = fopen(trace_path, "r");
    if (trace == NULL) {
        fprintf(stderr, "%s: %s: %s\n", argv[0], trace_path, strerror(errno));
        free(cache.lines);
        return 1;
    }

    while (fgets(buffer, sizeof(buffer), trace) != NULL) {
        if (sscanf(buffer, " %c %llx,%d", &operation, &address, &size) != 3 ||
            operation == 'I') {
            continue;
        }
        if (operation != 'L' && operation != 'S' && operation != 'M') {
            continue;
        }
        if (cache.verbose) {
            printf("%c %llx,%d", operation, address, size);
        }
        access_cache(&cache, (uint64_t)address);
        if (operation == 'M') {
            access_cache(&cache, (uint64_t)address);
        }
        if (cache.verbose) {
            printf(" \n");
        }
    }

    fclose(trace);
    free(cache.lines);
    printSummary(cache.hits, cache.misses, cache.evictions);
    return 0;
}
