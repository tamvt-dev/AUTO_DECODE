#ifndef LRU_CACHE_H
#define LRU_CACHE_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct LRUCache LRUCache;

LRUCache* lru_cache_new(size_t max_size);
void lru_cache_free(LRUCache *cache);
void lru_cache_put(LRUCache *cache, const char *key, const char *value);
char* lru_cache_get(LRUCache *cache, const char *key);
void lru_cache_clear(LRUCache *cache);
size_t lru_cache_size(LRUCache *cache);

#ifdef __cplusplus
}
#endif
#endif /* LRU_CACHE_H */