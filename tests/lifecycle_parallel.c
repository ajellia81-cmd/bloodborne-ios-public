#define _POSIX_C_SOURCE 200809L
#include "lifecycle.h"
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
static pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
static int entered,release_readers,closing,drained,resumed;
static void wait_for(int* p,int value){
 struct timespec until;clock_gettime(CLOCK_REALTIME,&until);until.tv_sec+=3;
 while(*p<value)assert(pthread_cond_timedwait(&cv,&m,&until)==0);
}
static void* reader(void* p){
 (void)p;bb_ios_gpu_begin();bb_ios_gpu_begin();
 pthread_mutex_lock(&m);++entered;pthread_cond_broadcast(&cv);
 wait_for(&release_readers,1);pthread_mutex_unlock(&m);
 bb_ios_gpu_end();bb_ios_gpu_end();return NULL;
}
static void* pause_thread(void* p){
 (void)p;bb_ios_set_background(1);
 pthread_mutex_lock(&m);closing=1;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&m);
 bb_ios_gpu_quiesce_begin();
 pthread_mutex_lock(&m);drained=1;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&m);
 bb_ios_gpu_end();return NULL;
}
static void* late_reader(void* p){
 (void)p;bb_ios_gpu_begin();pthread_mutex_lock(&m);resumed=1;pthread_cond_broadcast(&cv);pthread_mutex_unlock(&m);bb_ios_gpu_end();return NULL;
}
int main(void){
 pthread_t a,b,p,c;assert(!pthread_create(&a,NULL,reader,NULL));assert(!pthread_create(&b,NULL,reader,NULL));
 pthread_mutex_lock(&m);wait_for(&entered,2);pthread_mutex_unlock(&m);
 assert(!pthread_create(&p,NULL,pause_thread,NULL));
 pthread_mutex_lock(&m);wait_for(&closing,1);assert(!drained);pthread_mutex_unlock(&m);
 assert(!pthread_create(&c,NULL,late_reader,NULL));
 pthread_mutex_lock(&m);assert(!resumed);release_readers=1;pthread_cond_broadcast(&cv);wait_for(&drained,1);assert(!resumed);pthread_mutex_unlock(&m);
 pthread_join(a,NULL);pthread_join(b,NULL);pthread_join(p,NULL);
 bb_ios_set_background(0);pthread_mutex_lock(&m);wait_for(&resumed,1);pthread_mutex_unlock(&m);pthread_join(c,NULL);
 puts("PASS: parallel and nested leases; drain waits for readers; background rejects new admission; resume releases waiting reader");
}
