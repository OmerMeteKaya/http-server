//
// Created by mete on 14.04.2026.
//

#include "ThreadPool.h"
#include "Logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void *worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->lock);


        while (pool->queue_size == 0 && !pool->stop)
        {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->stop)
        {
            pthread_mutex_unlock(&pool->lock);
            break;
        }


        ThreadJob job = pool->queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % MAX_QUEUE;
        pool->queue_size--;

        pthread_mutex_unlock(&pool->lock);


        job.function(job.arg);
    }

    return NULL;
}


ThreadPool* thread_pool_init(int num_threads)
{
    if (num_threads <= 0) {
        LOG_ERROR("thread_pool_init: invalid thread count %d", num_threads);
        return NULL;
    }

    ThreadPool* pool = (ThreadPool*)calloc(1, sizeof(ThreadPool));
    if (!pool) {
        LOG_ERROR("thread_pool_init: malloc failed for ThreadPool");
        return NULL;
    }

    pool->thread_count = num_threads;
    pool->queue_size = 0;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->stop = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)num_threads);
    if (!pool->threads) {
        LOG_ERROR("thread_pool_init: malloc failed for threads array");
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        LOG_ERROR("thread_pool_init: mutex init failed");
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->notify, NULL) != 0) {
        LOG_ERROR("thread_pool_init: cond init failed");
        pthread_mutex_destroy(&pool->lock);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < num_threads; i++)
    {
        int rc = pthread_create(&pool->threads[i], NULL, worker, pool);
        if (rc != 0) {
            LOG_ERROR("thread_pool_init: pthread_create failed for thread %d: %s",
                      i, strerror(rc));
            // Clean up already-created threads
            pool->stop = 1;
            pthread_cond_broadcast(&pool->notify);
            for (int j = 0; j < i; j++) {
                pthread_join(pool->threads[j], NULL);
            }
            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->notify);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }

    LOG_INFO("Thread pool initialized with %d threads", num_threads);
    return pool;
}


void thread_pool_add_job(ThreadPool *pool, ThreadJob job)
{
    if (!pool) {
        LOG_ERROR("thread_pool_add_job: NULL pool");
        return;
    }

    pthread_mutex_lock(&pool->lock);

    if (pool->queue_size == MAX_QUEUE)
    {
        LOG_WARN("ThreadPool queue full (%d jobs) — dropping job", MAX_QUEUE);
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    pool->queue[pool->queue_rear] = job;

    pool->queue_rear = (pool->queue_rear + 1) % MAX_QUEUE;
    pool->queue_size++;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
}



ThreadJob thread_job_create(void* (*function)(void *), void *arg)
{
    ThreadJob job;
    job.function = function;
    job.arg = arg;
    return job;
}



void thread_pool_destroy(ThreadPool *pool)
{
    if (!pool) {
        LOG_ERROR("thread_pool_destroy: NULL pool");
        return;
    }

    LOG_DEBUG("Destroying thread pool (%d threads)...", pool->thread_count);

    pthread_mutex_lock(&pool->lock);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->notify);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->thread_count; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);

    free(pool);

    LOG_DEBUG("Thread pool destroyed");
}
