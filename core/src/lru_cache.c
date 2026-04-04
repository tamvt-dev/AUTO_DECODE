#include "../include/lru_cache.h"
#include "../include/logging.h"
#include <string.h>

typedef struct CacheEntry {
    char *key;
    char *value;
} CacheEntry;

struct LRUCache {
    GHashTable *hash;
    GQueue *order;
    GMutex mutex;
    size_t max_size;
    size_t current_size;
};

LRUCache* lru_cache_new(size_t max_size) {
    LRUCache *cache = g_new0(LRUCache, 1);
    cache->hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    cache->order = g_queue_new();
    cache->max_size = max_size;
    g_mutex_init(&cache->mutex);
    return cache;
}

void lru_cache_free(LRUCache *cache) {
    if (cache) {
        g_mutex_lock(&cache->mutex);
        if (cache->hash) {
            g_hash_table_destroy(cache->hash);
        }
        if (cache->order) {
            while (!g_queue_is_empty(cache->order)) {
                CacheEntry *entry = g_queue_pop_head(cache->order);
                g_free(entry);
            }
            g_queue_free(cache->order);
        }
        g_mutex_unlock(&cache->mutex);
        g_mutex_clear(&cache->mutex);
        g_free(cache);
    }
}

void lru_cache_put(LRUCache *cache, const char *key, const char *value) {
    if (!cache || !key || !value) return;
    
    g_mutex_lock(&cache->mutex);
    
    CacheEntry *existing = g_hash_table_lookup(cache->hash, key);
    if (existing) {
        g_free(existing->value);
        existing->value = g_strdup(value);
        g_queue_remove(cache->order, existing);
        g_queue_push_head(cache->order, existing);
        g_mutex_unlock(&cache->mutex);
        return;
    }
    
    if (cache->current_size >= cache->max_size) {
        CacheEntry *oldest = g_queue_pop_tail(cache->order);
        if (oldest) {
            g_hash_table_remove(cache->hash, oldest->key);
            g_free(oldest->key);
            g_free(oldest->value);
            g_free(oldest);
            cache->current_size--;
        }
    }
    
    CacheEntry *entry = g_new0(CacheEntry, 1);
    entry->key = g_strdup(key);
    entry->value = g_strdup(value);
    
    g_hash_table_insert(cache->hash, entry->key, entry);
    g_queue_push_head(cache->order, entry);
    cache->current_size++;
    
    g_mutex_unlock(&cache->mutex);
}

char* lru_cache_get(LRUCache *cache, const char *key) {
    if (!cache || !key) return NULL;
    
    g_mutex_lock(&cache->mutex);
    
    CacheEntry *entry = g_hash_table_lookup(cache->hash, key);
    if (!entry) {
        g_mutex_unlock(&cache->mutex);
        return NULL;
    }
    
    g_queue_remove(cache->order, entry);
    g_queue_push_head(cache->order, entry);
    
    char *result = g_strdup(entry->value);
    g_mutex_unlock(&cache->mutex);
    
    return result;
}

void lru_cache_clear(LRUCache *cache) {
    if (!cache) return;
    
    g_mutex_lock(&cache->mutex);
    g_hash_table_remove_all(cache->hash);
    
    while (!g_queue_is_empty(cache->order)) {
        CacheEntry *entry = g_queue_pop_head(cache->order);
        g_free(entry);
    }
    
    cache->current_size = 0;
    g_mutex_unlock(&cache->mutex);
}

size_t lru_cache_size(LRUCache *cache) {
    if (!cache) return 0;
    
    g_mutex_lock(&cache->mutex);
    size_t size = cache->current_size;
    g_mutex_unlock(&cache->mutex);
    
    return size;
}