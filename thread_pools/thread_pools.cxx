#include "thread_pools.h"
#include <pthread.h>

static void *worker(void *arg) {
    ThreadPools *tp = (ThreadPools *)arg;
    while (true) {
        pthread_mutex_lock(&tp->mu);
        while (tp->queue.empty()) {
            pthread_cond_wait(&tp->not_empty, &tp->mu);
        }

        Work work = tp->queue.front();
        tp->queue.pop_front();
        pthread_mutex_unlock(&tp->mu);
        
        (work.f)(work.args);
    }
}

void thread_pool_init(ThreadPools *tp, size_t sz) {
    assert (sz > 0);
    int rv = pthread_mutex_init(&tp->mu, NULL);
    assert (rv == 0);

    rv = pthread_cond_init(&tp->not_empty, NULL);
    assert (rv == 0);
    tp->thread_pools.resize(sz);

    for (int i = 0; i < sz; i++) {
        rv = pthread_create(
            &tp->thread_pools[i],
            NULL,
            &worker,
            tp);
    }
}

void thread_pool_queue(ThreadPools *tp,
        void (*f)(void *args), void *args) {
    Work work;
    work.f = f;
    work.args = args;

    pthread_mutex_lock(&tp->mu);
    tp->queue.push_back(work);
    pthread_cond_signal(&tp->not_empty);
    pthread_mutex_unlock(&tp->mu);
}
