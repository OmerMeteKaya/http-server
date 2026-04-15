//
// Created by mete on 14.04.2026.
//

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

#define MAX_QUEUE 1024


typedef struct ThreadJob {
    void (*function)(void *);
    void *arg;
} ThreadJob;


typedef struct ThreadPool {
    pthread_t *threads;
    int thread_count;

    ThreadJob queue[MAX_QUEUE];
    int queue_size;
    int queue_front;
    int queue_rear;

    pthread_mutex_t lock;
    pthread_cond_t notify;

    int stop;
} ThreadPool;


ThreadPool thread_pool_init(int num_threads);


void thread_pool_add(ThreadPool *pool, void (*function)(void *), void *arg);
void thread_pool_add_job(ThreadPool *pool, ThreadJob job);

ThreadJob thread_job_create(void (*function)(void *), void *arg);
void thread_pool_destroy(ThreadPool *pool);

#endif