#include <deque>
#include <stdlib.h>
#include <sys/_pthread/_pthread_cond_t.h>
#include <sys/_pthread/_pthread_t.h>
#include <vector>

typedef struct Work {
    void (*f)(void *args) = NULL;
    void *args = NULL;
} Work;

typedef struct ThreadPools {
    std::vector<pthread_t> thread_pools;
    std::deque<Work> queue;
    pthread_mutex_t mu;
    pthread_cond_t not_empty;
} ThreadPools;


void thread_pool_init(ThreadPools *tp, size_t sz);
void thread_pool_queue(ThreadPools *tp,
        void (*f)(void *args), void *args);
