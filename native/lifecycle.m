#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#include "lifecycle.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#import <os/proc.h>
#import <mach/mach.h>

extern void runtime_audio_set_paused(int paused);
static NSMutableSet<id<MTLCommandQueue>> *queues;
static int inactive;
static pthread_mutex_t queue_registry_lock=PTHREAD_MUTEX_INITIALIZER;
static void memory_report(const char *stage) {
    task_vm_info_data_t info={0};mach_msg_type_number_t count=TASK_VM_INFO_COUNT;
    if(task_info(mach_task_self(),TASK_VM_INFO,(task_info_t)&info,&count)==KERN_SUCCESS)
        printf("iOS memory (%s): footprint=%llu MiB, remaining allocation allowance=%llu MiB\n",stage,
            (unsigned long long)(info.phys_footprint>>20),
            (unsigned long long)(os_proc_available_memory()>>20));
}

/* Called with the GPU lease held. All runtime Metal command buffers use this bridge. */
void bb_ios_metal_commit_locked(void *buffer) {
    id<MTLCommandBuffer> command = (__bridge id<MTLCommandBuffer>)buffer;
    if (!command) { fprintf(stderr,"iOS Metal: nil command buffer (allocation failed)\n"); return; }
    pthread_mutex_lock(&queue_registry_lock);
    if (!queues) queues = [NSMutableSet new];
    [queues addObject:command.commandQueue];
    pthread_mutex_unlock(&queue_registry_lock);
    [command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
        if (completed.status == MTLCommandBufferStatusError) {
            fprintf(stderr,"iOS Metal command failed: label=%s gpu=%.2f ms; %s\n",completed.label.UTF8String,1000.0*(completed.GPUEndTime-completed.GPUStartTime),completed.error.description.UTF8String);
        }
    }];
    [command commit];
}
void bb_ios_metal_commit(void *buffer) {
    bb_ios_gpu_begin();
    bb_ios_metal_commit_locked(buffer);
    bb_ios_gpu_end();
}
static pthread_mutex_t frame_stats_lock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t last_frame_ns,window_ns,frame_max_ns,frame_intervals;
static void reset_frame_stats(void){
    pthread_mutex_lock(&frame_stats_lock);
    last_frame_ns=window_ns=frame_max_ns=frame_intervals=0;
    pthread_mutex_unlock(&frame_stats_lock);
}
void bb_ios_note_frame(void){
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    const uint64_t now=(uint64_t)ts.tv_sec*1000000000ull+(uint64_t)ts.tv_nsec;
    pthread_mutex_lock(&frame_stats_lock);
    if(!last_frame_ns){last_frame_ns=window_ns=now;pthread_mutex_unlock(&frame_stats_lock);return;}
    const uint64_t interval=now-last_frame_ns;last_frame_ns=now;++frame_intervals;
    if(interval>frame_max_ns)frame_max_ns=interval;
    const uint64_t elapsed=now-window_ns;
    if(elapsed>=5000000000ull){
        static const char* names[]={"nominal","fair","serious","critical"};
        const NSInteger thermal=NSProcessInfo.processInfo.thermalState;
        printf("iOS game frames: %.2f FPS, average %.2f ms, maximum %.2f ms; thermal=%s lowPower=%d\n",
            frame_intervals*1000000000.0/elapsed,elapsed/(frame_intervals*1000000.0),frame_max_ns/1000000.0,
            thermal>=0&&thermal<4?names[thermal]:"unknown",(int)NSProcessInfo.processInfo.lowPowerModeEnabled);
        memory_report("frame interval");window_ns=now;frame_max_ns=frame_intervals=0;
    }
    pthread_mutex_unlock(&frame_stats_lock);
}

static void resume_game(void);
static void pause_game(void) {
    if (inactive) return;
    // UIKit can notify reentrantly while this thread owns a drawable transaction.
    // Close admission now and drain after that transaction unwinds.
    if (bb_ios_gpu_in_lease()) {
        static int deferred;
        bb_ios_set_background(1);
        if (!deferred) {
            deferred=1;
            dispatch_async(dispatch_get_main_queue(),^{
                deferred=0;
                if (UIApplication.sharedApplication.applicationState==UIApplicationStateActive) {
                    resume_game();
                    if (!inactive) bb_ios_set_background(0);
                } else pause_game();
            });
        }
        return;
    }
    inactive=1;
    reset_frame_stats();
    memory_report("inactive");
    bb_ios_set_background(1);
    runtime_audio_set_paused(1);
    bb_ios_gpu_quiesce_begin();
    /* Bound the UIKit callback even if an earlier GPU fault has already stalled a queue. */
    for (id<MTLCommandQueue> queue in queues) {
        dispatch_semaphore_t done=dispatch_semaphore_create(0);
        id<MTLCommandBuffer> fence=[queue commandBuffer];
        fence.label=@"Bloodborne background drain";
        [fence addCompletedHandler:^(id<MTLCommandBuffer> completed) { dispatch_semaphore_signal(done); }];
        [fence commit];
        if (dispatch_semaphore_wait(done,dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC)))
            fprintf(stderr,"iOS lifecycle: GPU drain exceeded 500 ms; earlier GPU work still pending\n");
    }
    bb_ios_gpu_end();
    puts("iOS lifecycle: inactive; guest execution, audio and Metal submissions paused");
}
static void resume_game(void) {
    if (!inactive) return;
    runtime_audio_set_paused(0);
    inactive=0;
    reset_frame_stats();
    memory_report("active");
    bb_ios_set_background(0);
    puts("iOS lifecycle: active; audio reset and guest/Metal gates resumed");
}
static void pump_foreground(void) {
    if (![NSThread isMainThread]) return;
    while (bb_ios_is_background()) {
        @autoreleasepool {
            [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.02]];
        }
    }
}
void bb_ios_lifecycle_install(void) {
    static dispatch_once_t once;
    dispatch_once(&once,^{
        bb_ios_main_pump=pump_foreground;
        memory_report("startup");
        NSNotificationCenter *center=[NSNotificationCenter defaultCenter];
        [center addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:nil
                       usingBlock:^(NSNotification *note) { pause_game(); }];
        [center addObserverForName:UIApplicationDidEnterBackgroundNotification object:nil queue:nil
                       usingBlock:^(NSNotification *note) { pause_game(); }];
        [center addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:nil
                       usingBlock:^(NSNotification *note) { resume_game(); }];
        [center addObserverForName:UIApplicationDidReceiveMemoryWarningNotification object:nil queue:nil
                       usingBlock:^(NSNotification *note) { memory_report("UIKit memory warning"); }];
        if (UIApplication.sharedApplication.applicationState != UIApplicationStateActive) pause_game();
    });
}


