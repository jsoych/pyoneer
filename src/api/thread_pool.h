#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stddef.h>

#include "server.h"

typedef struct ThreadPool ThreadPool;

/*
 * Creates a new thread pool.
 * - nworkers: number of worker threads
 * - qcap: bounded queue capacity for pending jobs
 * - server: passed to client handler
 */
ThreadPool *thread_pool_create(size_t nworkers, size_t qcap, Server *server);

/*
 * Submit a new client connection to the pool.
 * Returns:
 *   0  on success
 *  -1  if shutting down (caller should close fd)
 */
int thread_pool_submit(ThreadPool *pool, int client_fd, int listener_index);

/*
 * Signal the pool to stop accepting new work and exit when queue drains.
 */
void thread_pool_shutdown(ThreadPool *pool);

/*
 * Shutdown (if needed), join all workers, and free resources.
 */
void thread_pool_destroy(ThreadPool *pool);

#endif
