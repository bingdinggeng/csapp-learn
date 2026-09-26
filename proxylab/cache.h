#ifndef PROXY_CACHE_H
#define PROXY_CACHE_H
#include <stddef.h>
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
/* data 由调用方提供，至少 MAX_OBJECT_SIZE 字节。 */
int cache_get(const char *key, unsigned char *data, size_t *size);
void cache_put(const char *key, const unsigned char *data, size_t size);
#endif
