//
// Created by mete on 14.04.2026.
//

#include "ThreadPool.h"
#include <stdlib.h>
#include <stdio.h>


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


ThreadPool thread_pool_init(int num_threads)
{
    ThreadPool pool;

    pool.thread_count = num_threads;
    pool.queue_size = 0;
    pool.queue_front = 0;
    pool.queue_rear = 0;
    pool.stop = 0;

    pool.threads = malloc(sizeof(pthread_t) * num_threads);

    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&pool.threads[i], NULL, worker, &pool);
    }

    return pool;
}


void thread_pool_add_job(ThreadPool *pool, ThreadJob job)
{
    pthread_mutex_lock(&pool->lock);

    if (pool->queue_size == MAX_QUEUE)
    {
        printf("Queue full!\n");
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    pool->queue[pool->queue_rear] = job;

    pool->queue_rear = (pool->queue_rear + 1) % MAX_QUEUE;
    pool->queue_size++;

    pthread_cond_signal(&pool->notify);
    pthread_mutex_unlock(&pool->lock);
}



ThreadJob thread_job_create(void (*function)(void *), void *arg)
{
    ThreadJob job;
    job.function = function;
    job.arg = arg;
    return job;
}



void thread_pool_destroy(ThreadPool *pool)
{
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
}
