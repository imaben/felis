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
    dict->count = 0;
    dict->trie = dfa_trie_create();
    dict->next = NULL;
    return dict;
}

dict_t *dict_find(dict_t *head, char *name)
{
    if (!head) {
        return NULL;
    }
    do {
        if (strcasecmp(head->name, name) == 0) {
            return head;
        }
    } while (NULL != (head = head->next));

    return NULL;
}

int dict_word_add(dict_t *dict, char *word, char *ext)
{
    if (!dict || !word) {
        return -1;
    }

    dict_wlock(dict);

    int r = dfa_trie_add(dict->trie, word, ext);
    if (r == -1) {
        dict_unlock(dict);
        return -1;
    }
    dict->count++;
    dict_unlock(dict);

    return 0;
}

void dict_destroy(dict_t *dict)
{
    pthread_rwlock_destroy(&dict->rwlock);
    dfa_trie_release(dict->trie);
    free(dict);
}
