#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"

typedef struct entry {
    char *key;
    unsigned char *data;
    size_t size;
    struct entry *prev, *next;
} entry_t;

static entry_t *head, *tail;
static size_t used;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void unlink_entry(entry_t *e)
{
    if (e->prev) e->prev->next = e->next;
    else head = e->next;
    if (e->next) e->next->prev = e->prev;
    else tail = e->prev;
}

static void prepend(entry_t *e)
{
    e->prev = NULL;
    e->next = head;
    if (head) head->prev = e;
    else tail = e;
    head = e;
}

int cache_get(const char *key, unsigned char *data, size_t *size)
{
    entry_t *e;
    pthread_mutex_lock(&lock);
    for (e = head; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            memcpy(data, e->data, e->size);
            *size = e->size;
            unlink_entry(e);
            prepend(e);
            pthread_mutex_unlock(&lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

void cache_put(const char *key, const unsigned char *data, size_t size)
{
    entry_t *e, *fresh;
    if (!size || size > MAX_OBJECT_SIZE) return;
    pthread_mutex_lock(&lock);
    for (e = head; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    while (used + size > MAX_CACHE_SIZE) {
        e = tail;
        unlink_entry(e);
        used -= e->size;
        free(e->key);
        free(e->data);
        free(e);
    }
    fresh = calloc(1, sizeof(*fresh));
    if (fresh) {
        fresh->key = strdup(key);
        fresh->data = malloc(size);
        if (fresh->key && fresh->data) {
            memcpy(fresh->data, data, size);
            fresh->size = size;
            prepend(fresh);
            used += size;
        } else {
            free(fresh->key);
            free(fresh->data);
            free(fresh);
        }
    }
    pthread_mutex_unlock(&lock);
}
