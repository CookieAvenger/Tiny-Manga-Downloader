#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "hashMap.h"

bool seedDone = false;

//range being [min, max)
size_t random_range(size_t min, size_t max) {
    if (!seedDone) {
        srand48(time(NULL));
        seedDone = true;
    }
    double middle = drand48() * (max - min); 
    return (size_t) (middle + min);          
}                                            

hashMap *new_hash_map(int (*new_comparator) (const void *, const void *),
        long (*new_get_key) (const void *)) {
    hashMap *map = (hashMap *) malloc(sizeof(hashMap));
    if (map == NULL) {
        exit(21);
    }
    map->table = NULL;
    map->totalItems = 0;
    map->comparator = new_comparator;
    map->get_key = new_get_key;
    //can do an if here to see if 32 bit or 64 needed 32 bit is minimum
    if (2147483659U > LONG_MAX) {
        map->prime = 2147483659U; //first prime bigger than signed 32 bit
    } else if (9223372036854775837U > LONG_MAX) {
        map->prime = 9223372036854775837U; //first prime bigger than signed 64 bit
    }
    //otherwise will automatically work it out. may take some time =|
    //trade off here - normally this program only uses 5 mb, so if I set meta
    //at 25000, probs never have to reshuffle, but 25000 will probs use 5mb..
    //so really, dunno if I should save cpu time or ram, both kinda small in
    //the big picture
    map->meta = 4;
    map->threadOn = false;
    return map;
}

void set_to_null(hashQueue **newTable, size_t tableSize) {
    for(size_t i = 0; i < tableSize; i++) {
        newTable[i] = NULL;
    }
}

void *add_to_queue_number(hashMap *map, hashQueue **table, 
        void *toAdd, size_t hashedKey) {
    if (table[hashedKey] == NULL) {
        hashQueue *firstEntry = (hashQueue *) malloc(sizeof(hashQueue));
        if (firstEntry == NULL) {
            exit(21);
        }
        firstEntry->item = toAdd;
        firstEntry->next = NULL;
        table[hashedKey] = firstEntry;
        map->totalItems++;
        return NULL;
    } else {
        hashQueue *last = table[hashedKey];
        for(EVER) {
            if (map->comparator(last->item, toAdd) == 0) {
                return last->item;
            }
            if (last->next == NULL) {
                hashQueue *nextEntry = (hashQueue *) malloc(sizeof(hashQueue));
                if (nextEntry == NULL) {
                    exit(21);
                }
                nextEntry->item = toAdd;
                nextEntry->next = NULL;
                last->next = nextEntry;
                map->totalItems++;
                return NULL; 
            } else {
                last = last->next;
            }
        }
    }
}

void remake_hash_function(hashMap *map) {
    size_t incrementing;
    if (LONG_MAX > map->meta) {
        if (LONG_MAX % 2 == 0) {
            incrementing = LONG_MAX - 1;
        } else {
            incrementing = LONG_MAX;
        }
    } else if (map->meta % 2 == 0) {
        incrementing = map->meta - 1;
    } else {
        incrementing = map->meta;
    }
    bool flag = true;
    //dumb and simple prime calculation, should never come to this though
    while (map->prime < LONG_MAX || map->prime < map->meta) {
        incrementing += 2;
        for (int i = 3; i < incrementing/2; i += 2) {
            if (incrementing % i == 0) {
                flag = false;
                break;
            }
        }
        if (flag) {
            map->prime = incrementing;
        } else {
            flag = true;
        }
    }
    map->alpha = random_range(1, map->prime);
    map->beta = random_range(0, map->prime);
}

//This is the original Universal Hash function proposed by Carter and Wegman
//Made for hashing integers not strings - I should really allow any hash function
//and allow a void that holds a struct with all relevent hash data
//and a remake hash function works like that too
//but... meh.
//BTW the sha hashed are numbers represented by a hexadecimal string, and I'm
//just using the first 64 bits as the number, so I am hashing a number
//not a string in this particular scenerio
size_t get_hash(hashMap *map, void *toAdd) {
    return (size_t) (((map->alpha * map->get_key(toAdd) + map->beta) 
            % map->prime) % map->meta);
}

void *resize_hash_map(void *voidMap) {
    hashMap *map = (hashMap *) voidMap;
    size_t oldSize = map->meta;
    map->meta *= 2;
    remake_hash_function(map);
    hashQueue **newTable = (hashQueue **) malloc(sizeof(hashQueue *) * map->meta);
    if (newTable == NULL) {
        exit(21);
    }
    set_to_null(newTable, map->meta);
    map->totalItems = 0;
    //go through every element and add to new while freeing from old
    for (int i = 0; i < oldSize; i++) {
        if (map->table[i] == NULL) {
            continue;
        }
        hashQueue *current = map->table[i];
        for (EVER) {
            add_to_queue_number(map, newTable, 
                    current->item, get_hash(map, current->item));
            if (current->next == NULL) {
                free(current);
                break;
            } else {
                hashQueue *temp = current->next;
                free(current);
                current = temp;
            }
        }
    }
    free(map->table);
    map->table = newTable;
    return NULL;
}

void *insert_item_into_map(hashMap *map, void *toAdd) {
    //join thread here
    if (map->threadOn) {
        pthread_join(map->threadId, NULL);
        map->threadOn = false;
    }
    //insert sequence here
    if (map->table == NULL) {
        map->table = (hashQueue **) malloc(sizeof(hashQueue *) * map->meta);
        if (map->table == NULL) {
            exit(21);
        }
        set_to_null(map->table, map->meta);
        remake_hash_function(map);
    }
    size_t hashedKey = get_hash(map, toAdd);
    void *toReturn = add_to_queue_number(map, map->table, toAdd, hashedKey);
    //reorganise if needed in a thread here
    if (map->totalItems >= map->meta) {
        pthread_create(&(map->threadId), NULL, resize_hash_map, map);
        map->threadOn = true;
    }
    return toReturn;
}

hashMap *hash_map_construction(void **items, size_t totalItems,
        int (*comparator) (const void *, const void *),
        long (*get_key) (const void *)) {
    hashMap *map = new_hash_map(comparator, get_key);
    if (totalItems * 2 > map->meta) {
        map->meta = totalItems * 2;
    }
    map->table = (hashQueue **) malloc(sizeof(hashQueue *) * map->meta);
    if (map->table == NULL) {
        exit(21);
    }
    remake_hash_function(map);
    int i = 0;
    void *current;
    while(items[i] != NULL) {
        current = items[i++];
        add_to_queue_number(map, map->table, current, get_hash(map, current));
    }
    return map;
}

void **turn_map_into_array(hashMap *map) {
    if (map->threadOn) {
        pthread_join(map->threadId, NULL);
        map->threadOn = false;
    }
    if (map->table == NULL || map->totalItems == 0) {
        return NULL;
    }
    size_t count = 0;
    void **toReturn = (void **) malloc(sizeof(void *) * (map->totalItems + 1));
    if (toReturn == NULL) {
        exit(21);
    }
    for (size_t i = 0; i < map->meta; i++) {
        if (map->table[i] == NULL) {
            continue;
        }
        hashQueue *current = map->table[i];
        for (EVER) {
            //add here
            toReturn[count++] = current->item; 
            if (current->next == NULL) {
                break;
            } else {
                current = current->next;
            }
        }
    }
    toReturn[count] = NULL;
    return toReturn;
}

void free_map(hashMap *map) {
    if (map->threadOn) {
        pthread_join(map->threadId, NULL);
        map->threadOn = false;
    }
    if (map->table == NULL || map->totalItems == 0) {
        return;
    }
    for (size_t i = 0; i < map->meta; i++) {
        if (map->table[i] == NULL) {
            continue;
        }
        hashQueue *current = map->table[i];
        for (EVER) {
            if (current->next == NULL) {
                free(current);
                break;
            } else {
                hashQueue *temp = current->next;
                free(current);
                current = temp;
            }
        }
    }
    free(map->table);
    free(map);
}
