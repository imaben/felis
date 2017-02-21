#pragma once

#include "dfa/dfa/dfa.h"
#include <pthread.h>

#define MAX_DICT_NAME_LENGTH 64

typedef struct {
    char name[MAX_DICT_NAME_LENGTH];
    dfa_trie_t *trie;
    pthread_rwlock_t rwlock;
} dict_t;

#define felis_dict_rlock(dict) pthread_rwlock_rdlock(&dict->rwlock)
#define felis_dict_wlock(dict) pthread_rwlock_wrlock(&dict->rwlock)
#define felis_dict_unlock(dict) pthread_rwlock_unlock(&dict->rwlock)

dict_t *dict_new(char *name);
void dict_destroy(dict_t *dict);
