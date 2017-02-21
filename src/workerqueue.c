#include "workerqueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

#define LL_ADD(item, list) { \
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

static void *worker_function(void *ptr)
{
    worker_t *worker = (worker_t *)ptr;
    job_t *job;

    for (;;) {
        pthread_mutex_lock(&worker->workqueue->jobs_mutex);
        while (worker->workqueue->waiting_jobs == NULL) {
            if (worker->terminate)
                break;
            pthread_cond_wait(&worker->workqueue->jobs_cond,
                    &worker->workqueue->jobs_mutex);
        }

        if (worker->terminate)
            break;

        job = worker->workqueue->waiting_jobs;
        if (NULL != job) {
            LL_REMOVE(job, worker->workqueue->waiting_jobs);
        }
        pthread_mutex_unlock(&worker->workqueue->jobs_mutex);

        job->job_cb(job);
    }
    free(worker);
    pthread_exit(NULL);
}

int workqueue_init(workqueue_t *queue, int workers)
{
    int i;
    worker_t *worker;
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

    if (workers < 1)
        workers = 1;
    memset(queue, 0, sizeof(*queue));
    memcpy(&queue->jobs_cond, &blank_cond, sizeof(blank_cond));
    memcpy(&queue->jobs_mutex, &blank_mutex, sizeof(blank_mutex));

    for (i = 0; i < workers; i++) {
        worker = (worker_t *)malloc(sizeof(*worker));
        if (NULL == worker) {
            log_error("Failed to allocate all workers");
            return -1;
        }
        memset(worker, 0, sizeof(*worker));
        worker->workqueue = queue;
        if (pthread_create(&worker->thread, NULL,
                    worker_function, (void *)worker_function)) {
            log_error("Failed to start all worker threads");
            return -1;
        }
        LL_ADD(worker, worker->workqueue->workers);
    }
    return 0;
}

void workqueue_shutdown(workqueue_t *queue)
{
    worker_t *worker = NULL;

    for (worker = queue->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }

    pthread_mutex_lock(&queue->jobs_mutex);
    queue->workers = NULL;
    queue->waiting_jobs = NULL;
    pthread_cond_broadcast(&queue->jobs_cond);
    pthread_mutex_unlock(&queue->jobs_mutex);
}

void workqueue_append(workqueue_t *queue, job_t *job)
{
    pthread_mutex_lock(&queue->jobs_mutex);
    LL_ADD(job, queue->waiting_jobs);
    pthread_cond_signal(&queue->jobs_cond);
    pthread_mutex_unlock(&queue->jobs_mutex);
}

