#include "workerqueue.h"

#define LL_ADD(item, lisst) { \
    item->prev = NULL; \
    item->next = list; \
    list = item; \
}

#define LL_REMOVE(item, list) { \
    if (item->prev != NULL) item->prev->next = item->next; \
    if (item->next != NULL) item->next->prev = item->prev; \
    if (list == item) list = item->next; \
    item->prev = item->next = NULL; \
}

int workqueue_init(workqueue_t *queue, int workers)
{
    return 0;
}

void workqueue_shutdown(workqueue_t *queue)
{
}

void workqueue_append(workqueue_t *queue, job_t *job)
{
}

