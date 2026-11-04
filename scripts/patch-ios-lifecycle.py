#!/usr/bin/env python3
"""iOS-only cooperative guest pause and serialized/drained Metal submissions."""
from pathlib import Path
root=Path('upstream-runtime')
def change(path, old, new):
    p=root/path;s=p.read_text();assert s.count(old)==1,(path,old)
    p.write_text(s.replace(old,new))
cpu_header='''
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_wait_foreground(void);
#else
static inline void bb_ios_wait_foreground(void) {}
#endif
'''
for path, functions in {
    'recomp/src/cpu/recomp.c':[
        'static uint64_t rc_step(BbCpu *cpu) {',
        'static void rc_call(BbCpu *cpu, uint64_t target, uint64_t next) {',
        'static void rc_tail(BbCpu *cpu) {',
        'static void rc_bail(BbCpu *cpu, uint64_t entry_rsp) {'],
    'recomp/src/cpu/interp.c':['    while (cpu->rip != stop) {'],
}.items():
    p=root/path;s=p.read_text();s=cpu_header+s
    for fn in functions:
        assert s.count(fn)==1,(path,fn)
        s=s.replace(fn,fn+'\n        bb_ios_wait_foreground();')
    p.write_text(s)

bridge='''
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
extern "C" void bb_ios_metal_commit(void*);
extern "C" void bb_ios_metal_commit_locked(void*);
extern "C" void bb_ios_gpu_begin(void);
extern "C" void bb_ios_gpu_end(void);
#define BB_IOS_COMMIT(command) bb_ios_metal_commit((__bridge void*)(command))
#define BB_IOS_NEW_COMMAND(q) [(q) commandBuffer]
#else
#define BB_IOS_COMMIT(command) [(command) commit]
#define BB_IOS_NEW_COMMAND(q) [(q) commandBufferWithUnretainedReferences]
#endif
'''
metal=root/'gpu/shadps4/video_core/renderer_metal'
for name in ['mtl_vk_layer.mm','mtl_frame.mm','mtl_present.mm','mtl_device.mm']:
    p=metal/name;s=p.read_text();s=bridge+s
    if name=='mtl_present.mm':
        needle='bool Present(void* layer_ptr, PresentInfo info) {'
        assert s.count(needle)==1
        s=s.replace(needle,needle+'''
#if TARGET_OS_IPHONE
    // Keep drawable acquisition, encoding and submission in one foreground lease.
    struct ForegroundLease {
        ForegroundLease() { bb_ios_gpu_begin(); }
        ~ForegroundLease() { bb_ios_gpu_end(); }
    } lease;
#endif
''')
        assert s.count('[cmd commit];')==1
        s=s.replace('[cmd commit];','''#if TARGET_OS_IPHONE
        bb_ios_metal_commit_locked((__bridge void*)cmd);
#else
        [cmd commit];
#endif''')
    else:
        s=s.replace('[(__bridge id<MTLCommandBuffer>)command_buffer commit];','BB_IOS_COMMIT((__bridge id<MTLCommandBuffer>)command_buffer);')
        for command in ['cmd','o->cmd','o.cmd','prime']:
            s=s.replace(f'[{command} commit];',f'BB_IOS_COMMIT({command});')
    # iOS capture/memory-pressure must not retire bound resources before GPU completion.
    # Preserve the upstream macOS ownership policy.
    s=s.replace('[queue commandBufferWithUnretainedReferences]', 'BB_IOS_NEW_COMMAND(queue)')
    s=s.replace('[o->queue commandBufferWithUnretainedReferences]', 'BB_IOS_NEW_COMMAND(o->queue)')
    p.write_text(s)

# Audio stream lifetime is protected by the existing port table and per-port locks.
p=root/'src/runtime_audio.c';s=p.read_text()
needle='static int initialized, sdl_ready=-1;'
assert s.count(needle)==1
s=s.replace(needle,needle+'\nstatic int lifecycle_paused;')
s=s.replace('if (p->stream) SDL_ResumeAudioStreamDevice(p->stream);',
            'if (p->stream) { if (!lifecycle_paused) SDL_ResumeAudioStreamDevice(p->stream); }')
s+='''
void runtime_audio_set_paused(int paused) {
    pthread_mutex_lock(&table_lock);
    lifecycle_paused=paused;
    for (int i=0;i<PORTS;++i) {
        Port *p=&ports[i];
        if (!p->used) continue;
        pthread_mutex_lock(&p->lock);
        if (p->stream) {
            if (paused) SDL_PauseAudioStreamDevice(p->stream);
            SDL_ClearAudioStream(p->stream);
            if (!paused) SDL_ResumeAudioStreamDevice(p->stream);
        }
        p->next_deadline_ns=0;
        p->adjust_ns=0;p->window_min=0;p->window_count=0;
        pthread_mutex_unlock(&p->lock);
    }
    pthread_mutex_unlock(&table_lock);
}
'''
p.write_text(s)

# iOS touch/controller input does not support desktop relative mouse capture.
change('gpu/shim/window.cpp','void WindowSDL::CaptureMouse(bool capture) {',
'''void WindowSDL::CaptureMouse(bool capture) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    (void)capture;
    mouse_captured.store(false, std::memory_order_relaxed);
    return;
#endif''')
print('iOS lifecycle: guest checkpoints, foreground Metal leases, queue drain bridge, audio pause/reset')


# Preserve batching: make local-to-hull scratch dependency explicit on iOS.
p=metal/'mtl_tessellation.mm';s=p.read_text()
s='#include <TargetConditionals.h>\n'+s
needle='                       scratch_offset + plan.vertex_offset, bind_vertex);'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n#if TARGET_OS_IPHONE\n    id<MTLResource> dependency = scratch_buffer;\n    [enc memoryBarrierWithResources:&dependency count:1];\n#endif\n')
p.write_text(s)
print('iOS Metal: retained command-buffer resources; local/hull scratch dependency barrier, batching preserved')
