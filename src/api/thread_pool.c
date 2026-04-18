#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "thread_pool.h"
#include "client.h"

typedef struct
{
    int fd;
    int listener_index;
} job;

struct ThreadPool
{
    pthread_t *threads;
    size_t nthreads;

    job *queue;
    size_t qcap;
    size_t qhead;
    size_t qtail;
    size_t qlen;

    int shutting_down;

    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

    Server *server; // context passed to client handler
};

static int queue_push(ThreadPool *pool, job in)
{
    pool->queue[pool->qtail] = in;
    pool->qtail = (pool->qtail + 1) % pool->qcap;
    pool->qlen++;
    return 0;
}

static int queue_pop(ThreadPool *pool, job *out)
{
    *out = pool->queue[pool->qhead];
    pool->qhead = (pool->qhead + 1) % pool->qcap;
    pool->qlen--;
    return 0;
}

static void *worker_thread(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;

    for (;;)
    {
        job j;

        pthread_mutex_lock(&pool->lock);

        // Wait for work while not shutting down
        while (pool->qlen == 0 && !pool->shutting_down)
        {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        // If shutting down and no work remains, exit
        if (pool->qlen == 0 && pool->shutting_down)
        {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        // Pop a job
        queue_pop(pool, &j);

        // Wake any submitter waiting on "not_full"
        pthread_cond_signal(&pool->not_full);

        pthread_mutex_unlock(&pool->lock);

        // Handle the connection (worker owns fd until handler closes it)
        client_handle_fd(pool->server, j.fd, j.listener_index);
    }

    return NULL;
}

ThreadPool *thread_pool_create(size_t nworkers, size_t qcap, Server *server)
{
    if (nworkers == 0 || qcap == 0 || !server)
    {
        errno = EINVAL;
        return NULL;
    }

    ThreadPool *pool = calloc(1, sizeof(ThreadPool));
    if (!pool)
        return NULL;

    pool->threads = calloc(nworkers, sizeof(pthread_t));
    pool->queue = calloc(qcap, sizeof(job));
    if (!pool->threads || !pool->queue)
    {
        free(pool->threads);
        free(pool->queue);
        free(pool);
        return NULL;
    }

    pool->nthreads = nworkers;
    pool->qcap = qcap;
    pool->qhead = 0;
    pool->qtail = 0;
    pool->qlen = 0;
    pool->shutting_down = 0;
    pool->server = server;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);

    // Start workers
    for (size_t i = 0; i < nworkers; i++)
    {
        int err = pthread_create(&pool->threads[i], NULL, worker_thread, pool);
        if (err != 0)
        {
            // If we fail to create a worker, shutdown and join what we created
            pool->shutting_down = 1;
            pthread_cond_broadcast(&pool->not_empty);
            pthread_cond_broadcast(&pool->not_full);

            // Join already created threads
            for (size_t j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }

            pthread_mutex_destroy(&pool->lock);
            pthread_cond_destroy(&pool->not_empty);
            pthread_cond_destroy(&pool->not_full);

            free(pool->threads);
            free(pool->queue);
            free(pool);
            errno = err;
            return NULL;
        }
    }

    return pool;
}

int thread_pool_submit(ThreadPool *pool, int client_fd, int listener_index)
{
    pthread_mutex_lock(&pool->lock);

    // If shutting down, reject immediately
    if (pool->shutting_down)
    {
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    // Wait until there's space or shutdown begins
    while (pool->qlen == pool->qcap && !pool->shutting_down)
    {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    if (pool->shutting_down)
    {
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }

    job in = {.fd = client_fd, .listener_index = listener_index};
    queue_push(pool, in);

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
    return 0;
}

void thread_pool_shutdown(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->lock);
    pool->shutting_down = 1;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_cond_broadcast(&pool->not_full);
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_destroy(ThreadPool *pool)
{
    if (!pool)
        return;

    thread_pool_shutdown(pool);

    // Join all workers
    for (size_t i = 0; i < pool->nthreads; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);

    free(pool->threads);
    free(pool->queue);
    free(pool);
}
