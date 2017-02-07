#ifndef HASHTABLE
#define HASHTABLE

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#define EVER ; ; 

//This hashmap is for hashing integers which have extra data with them

struct hashqueue {
    void *item;
    struct hashqueue *next;
};
typedef struct hashqueue hashQueue;

typedef struct hashmap {
    hashQueue **table;
    size_t totalItems;
    int (*comparator) (const void *, const void *);
    long (*get_key) (const void *);
    size_t alpha;
    size_t beta;
    size_t prime;
    size_t meta;
    bool threadOn;
    pthread_t threadId;
} hashMap;

//return null if insert succeeded, return an item if an identical one is found
void *insert_item_into_map(hashMap *map, void *toAdd);

//makes the hashmap, but sets the dynamic array as double totalItems from the start
hashMap *hash_map_construction(void **items, size_t totalItems,
        int (*comparator) (const void *, const void *),
        long (*get_key) (const void *));

hashMap *new_hash_map(int (*new_comparator) (const void *, const void *),
        long (*new_get_key) (const void *));

//returns the pointers to original not copied values
void **turn_map_into_array(hashMap *map);

//frees everything except void *in hashQueue
void free_map(hashMap *map);

#endif
