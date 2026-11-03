#!/usr/bin/env python3
"""Bound noncritical texture collection; keep critical/OOM collection unchanged.
Report collector and Metal pipeline creation time separately on iOS.
"""
from pathlib import Path
p=Path('upstream-runtime/gpu/shadps4/video_core/texture_cache/texture_cache.cpp');s=p.read_text()
s='#include <cstdio>\n#include <TargetConditionals.h>\n'+s
needle='    static const u64 idle_seconds = [] {'
assert s.count(needle)==1
s=s.replace(needle,'''#if TARGET_OS_IPHONE
    // Collection used to run on every submission, hundreds of times a second. Spread ordinary
    // work across submissions; no cooldown or time limit applies at the critical memory mark.
    const auto ios_gc_start=std::chrono::steady_clock::now();
    static auto ios_last_gc=ios_gc_start-std::chrono::seconds(1);
    if(total_used_memory<critical_gc_memory && ios_gc_start-ios_last_gc<std::chrono::milliseconds(8)) return;
    ios_last_gc=ios_gc_start;
    struct IosGcTiming {
        std::chrono::steady_clock::time_point start;
        ~IosGcTiming() {
            const auto now=std::chrono::steady_clock::now();
            const auto us=static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(now-start).count());
            static auto last=now;
            static unsigned long long calls=0,total=0,maximum=0;
            ++calls;total+=us;maximum=std::max(maximum,us);
            if(now-last>=std::chrono::seconds(5)) {
                std::printf("iOS streaming GC: %llu calls, %.2f ms total, %.2f ms max over last interval\\n",calls,total/1000.0,maximum/1000.0);
                calls=total=maximum=0;last=now;
            }
        }
    } ios_gc_timing{ios_gc_start};
#endif
'''+needle)
needle='''        if (num_deletions == 0 || ++visited > 256) {
            return true;
        }'''
assert s.count(needle)==1
s=s.replace(needle,needle+'''
#if TARGET_OS_IPHONE
        // Check every eight candidates; a single expensive free may exceed the budget. Never
        // interrupt critical collection: its existing three passes still make memory headroom.
        if(total_used_memory<critical_gc_memory && (visited&7)==0 &&
           std::chrono::steady_clock::now()-ios_gc_start>=std::chrono::milliseconds(2)) return true;
#endif''')
p.write_text(s)
p=Path('upstream-runtime/gpu/shadps4/video_core/renderer_metal/mtl_pso_cache.mm');s=p.read_text()
s='#include <TargetConditionals.h>\n'+s
needle='std::string FirstLine(NSError* error) {'
assert s.count(needle)==1
s=s.replace(needle,'''void NoteIosPipelineCompile(u64 us) {
#if TARGET_OS_IPHONE
    static std::mutex mutex;
    std::scoped_lock lock{mutex};
    static auto last=Clock::now();
    static u64 calls=0,total=0,maximum=0,slow=0;
    ++calls;total+=us;maximum=std::max(maximum,us);slow+=us>=8000;
    const auto now=Clock::now();
    if(now-last>=std::chrono::seconds(5)) {
        std::printf("iOS streaming PSO: %llu creations, %.2f ms total, %.2f ms max, %llu over 8 ms in last interval\\n",
                    (unsigned long long)calls,total/1000.0,maximum/1000.0,(unsigned long long)slow);
        calls=total=maximum=slow=0;last=now;
    }
#else
    (void)us;
#endif
}

'''+needle)
needle='            elapsed_us = UsSince(start);'
assert s.count(needle)==4
s=s.replace(needle,needle+'\n            NoteIosPipelineCompile(elapsed_us);')
p.write_text(s)
print('iOS streaming: noncritical GC spread/bounded; critical cleanup unchanged; separate GC/PSO timing')
