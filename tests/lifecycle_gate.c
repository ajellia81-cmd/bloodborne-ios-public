#include "lifecycle.h"
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <time.h>
static atomic_int ran;
static void delay(void) { struct timespec t={0,20000000}; nanosleep(&t,0); }
static void *guest(void *unused) { (void)unused; bb_ios_wait_foreground(); atomic_fetch_add(&ran,1); return 0; }
static void *gpu(void *unused) { (void)unused; bb_ios_gpu_begin(); atomic_fetch_add(&ran,1); bb_ios_gpu_end(); return 0; }
int main(void) {
    pthread_t a,b;
    bb_ios_gpu_begin();bb_ios_gpu_begin();bb_ios_gpu_end();bb_ios_gpu_end();
    for(int i=0;i<10;++i) {
        bb_ios_set_background(1);atomic_store(&ran,0);
        assert(!pthread_create(&a,0,guest,0));assert(!pthread_create(&b,0,gpu,0));
        delay();assert(atomic_load(&ran)==0);
        bb_ios_gpu_quiesce_begin();assert(atomic_load(&ran)==0);bb_ios_gpu_end();
        bb_ios_set_background(0);pthread_join(a,0);pthread_join(b,0);
        assert(atomic_load(&ran)==2);
    }
    return 0;
}
