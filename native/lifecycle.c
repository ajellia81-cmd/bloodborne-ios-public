#include "lifecycle.h"
#include <pthread.h>
#include <stdatomic.h>

void (*bb_ios_main_pump)(void);
static atomic_int background;
static _Thread_local unsigned gpu_depth;
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t foreground = PTHREAD_COND_INITIALIZER;

int bb_ios_is_background(void) { return atomic_load_explicit(&background,memory_order_acquire); }
void bb_ios_set_background(int value) {
    atomic_store_explicit(&background,value,memory_order_release);
    if (!value) {
        pthread_mutex_lock(&gate);
        pthread_cond_broadcast(&foreground);
        pthread_mutex_unlock(&gate);
    }
}
void bb_ios_wait_foreground(void) {
    /* Foreground hot path must not resolve thread-local storage on every AOT host call. */
    if (!bb_ios_is_background()) return;
    if (gpu_depth) return;
    if (bb_ios_main_pump) bb_ios_main_pump();
    pthread_mutex_lock(&gate);
    while (bb_ios_is_background()) pthread_cond_wait(&foreground,&gate);
    pthread_mutex_unlock(&gate);
}
/* A GPU lease covers submission, and the full drawable acquisition/present transaction.
 * The UIKit observer closes the gate before draining; it never parks the UI event loop. */
void bb_ios_gpu_begin(void) {
    if (gpu_depth) { ++gpu_depth; return; }
    bb_ios_wait_foreground();
    pthread_mutex_lock(&gate);
    while (bb_ios_is_background()) pthread_cond_wait(&foreground,&gate);
    gpu_depth=1;
}
void bb_ios_gpu_end(void) { if (--gpu_depth==0) pthread_mutex_unlock(&gate); }
void bb_ios_gpu_quiesce_begin(void) {
    if (gpu_depth) { ++gpu_depth; return; }
    pthread_mutex_lock(&gate);gpu_depth=1;
}
