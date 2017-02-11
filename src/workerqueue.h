#pragma once

#include <pthread.h>

struct workqueue;

typedef struct worker {
    pthread_t thread;
    int terminate;
    struct workqueue *workqueue;
    struct worker *prev;
    struct worker *next;
} worker_t;

typedef struct job {
    void (*job_cb)(struct job *job);
    void *user_data;
    struct job *prev;
    struct job *next;
} job_t;

typedef struct workqueue {
    struct worker *workers;
    struct job *waiting_jobs;
    pthread_mutex_t jobs_mutex;
    pthread_cond_t jobs_cond;
} workqueue_t;

int workqueue_init(workqueue_t *queue, int workers);
void workqueue_shutdown(workqueue_t *queue);
void workqueue_append(workqueue_t *queue, job_t *job);

