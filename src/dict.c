#include "dict.h"
#include <stdlib.h>
#include <string.h>

dict_t *dict_new(char *name)
{
    dict_t *dict = (dict_t *)malloc(sizeof(dict_t));
    memset(dict, 0, sizeof(*dict));

    if (strlen(name) > (MAX_DICT_NAME_LENGTH - 1)) {
        strncpy(dict->name, name, MAX_DICT_NAME_LENGTH - 1);
    } else {
        strcpy(dict->name, name);
    }
    pthread_rwlock_init(&dict->rwlock, NULL);
    dict->trie = dfa_trie_create();
    return dict;
}

void dict_destroy(dict_t *dict)
{
    pthread_rwlock_destroy(&dict->rwlock);
    dfa_trie_release(dict->trie);
    free(dict);
}
