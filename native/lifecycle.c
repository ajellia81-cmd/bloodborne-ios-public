#include "lifecycle.h"
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
void (*bb_ios_main_pump)(void);
static atomic_int background;
static _Thread_local unsigned gpu_depth;
static _Thread_local int exclusive;
static unsigned active_gpu;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t foreground = PTHREAD_COND_INITIALIZER;
int bb_ios_is_background(void) { return atomic_load_explicit(&background,memory_order_acquire); }
void bb_ios_set_background(int value) {
    pthread_mutex_lock(&gate);
    atomic_store_explicit(&background,value,memory_order_release);
    pthread_cond_broadcast(&foreground);
    pthread_mutex_unlock(&gate);
}
void bb_ios_wait_foreground(void) {
    if (!bb_ios_is_background() || gpu_depth) return;
    if (bb_ios_main_pump) bb_ios_main_pump();
    pthread_mutex_lock(&gate);
    while (bb_ios_is_background()) pthread_cond_wait(&foreground,&gate);
    pthread_mutex_unlock(&gate);
}
/* Parallel foreground leases: drawable waits must not serialize GPU submissions.
 * Close admission before waiting for all old leases when entering background. */
void bb_ios_gpu_begin(void) {
    if (gpu_depth) { ++gpu_depth; return; }
    for (;;) {
        bb_ios_wait_foreground();
        pthread_mutex_lock(&gate);
        if (!bb_ios_is_background()) break;
        pthread_mutex_unlock(&gate);
    }
    ++active_gpu;gpu_depth=1;
    pthread_mutex_unlock(&gate);
}
void bb_ios_gpu_end(void) {
    assert(gpu_depth);
    if (--gpu_depth) return;
    if (exclusive) { exclusive=0;pthread_mutex_unlock(&gate);return; }
    pthread_mutex_lock(&gate);
    assert(active_gpu);
    if (--active_gpu==0) pthread_cond_broadcast(&foreground);
    pthread_mutex_unlock(&gate);
}
void bb_ios_gpu_quiesce_begin(void) {
    assert(!gpu_depth && bb_ios_is_background());
    pthread_mutex_lock(&gate);
    while (active_gpu) pthread_cond_wait(&foreground,&gate);
    exclusive=1;gpu_depth=1;
}
