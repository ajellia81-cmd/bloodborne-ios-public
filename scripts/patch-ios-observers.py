#!/usr/bin/env python3
from pathlib import Path
p=Path('upstream-runtime/src/runtime_pad.c');s=p.read_text()
needle='#include <SDL3/SDL.h>'
assert s.count(needle)==1
s=s.replace(needle,needle+"\n#ifdef __APPLE__\n#include <TargetConditionals.h>\n#endif\n#if defined(__APPLE__) && TARGET_OS_IPHONE\nextern void bb_ios_haptic_runtime_ready(SDL_Gamepad*);\n#else\n#define bb_ios_haptic_runtime_ready(pad) ((void)0)\n#endif")
needle='    puts("Runtime: pad opened for user 1 (SDL gamepad or keyboard)");'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n    pthread_mutex_lock(&lock);\n    bb_ios_haptic_runtime_ready(current_gamepad());\n    pthread_mutex_unlock(&lock);')

old='''    if (g) SDL_RumbleGamepad(g,(uint16_t)(param[0]*257),(uint16_t)(param[1]*257),1000);'''
assert s.count(old)==1
s=s.replace(old,'''    static unsigned vibration_logged;
    const int observe=g&&(param[0]||param[1])&&vibration_logged<8;
    const uint64_t started=observe?now_us():0;
    const int ok=g?SDL_RumbleGamepad(g,(uint16_t)(param[0]*257),(uint16_t)(param[1]*257),1000):0;
    if(observe){
        ++vibration_logged;
        printf("iOS haptics game request: low=%u high=%u ok=%d time=%.2f ms\\n",param[0],param[1],ok,(now_us()-started)/1000.0);
    }''');p.write_text(s)
p=Path('upstream-runtime/gpu/shadps4/video_core/renderer_vulkan/vk_presenter.cpp');s=p.read_text()
s='''#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern "C" void bb_ios_note_frame(void);
#else
#define bb_ios_note_frame() ((void)0)
#endif
'''+s
needle='    BbStats::frame_number.fetch_add(1, std::memory_order_relaxed); // bbport: game frames shown'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n    bb_ios_note_frame();');p.write_text(s)
print('iOS observers: first nonzero rumble timings; lightweight game-frame interval observer')
