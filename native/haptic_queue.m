#import <Foundation/Foundation.h>
#include <SDL3/SDL.h>
#include <stdatomic.h>
#include <stdio.h>
#include "haptics.h"

static atomic_uint generation;
static atomic_bool suspended;
static dispatch_queue_t motor_queue(void) {
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ queue = dispatch_queue_create("bloodborne.controller.haptics", DISPATCH_QUEUE_SERIAL); });
    return queue;
}

static bool current(unsigned ticket) {
    return !atomic_load(&suspended) && ticket == atomic_load(&generation);
}

void bb_ios_motor_suspend(void) {
    if (atomic_exchange(&suspended, true)) return;
    atomic_fetch_add(&generation, 1);
}

void bb_ios_motor_resume(void) {
    if (!atomic_exchange(&suspended, false)) return;
    const unsigned ticket = atomic_load(&generation);
    dispatch_async(motor_queue(), ^{
        if (!current(ticket)) return;
        int count = 0;
        SDL_JoystickID *ids = SDL_GetGamepads(&count);
        for (int i = 0; ids && i < count && current(ticket); ++i) {
            SDL_Gamepad *pad = SDL_OpenGamepad(ids[i]);
            if (!pad) continue;
            const Uint64 start = SDL_GetTicksNS();
            bool ready = SDL_RumbleGamepad(pad, 1, 1, 1);
            if (ready) SDL_RumbleGamepad(pad, 0, 0, 0);
            printf("iOS foreground haptics: controller %u ready=%d %.2f ms\n",
                   ids[i], ready, (SDL_GetTicksNS()-start)/1e6);
            SDL_CloseGamepad(pad);
        }
        SDL_free(ids);
    });
}

void bb_ios_motor_prepare(SDL_Gamepad *pad) {
    if (!pad || atomic_load(&suspended)) return;
    const SDL_JoystickID id = SDL_GetGamepadID(pad);
    if (!id) return;
    const unsigned ticket = atomic_load(&generation);
    dispatch_async(motor_queue(), ^{
        if (!current(ticket)) return;
        SDL_Gamepad *active = SDL_OpenGamepad(id);
        if (!active) return;
        bb_ios_haptic_runtime_ready(active);
        SDL_CloseGamepad(active);
    });
}

void bb_ios_motor_request(SDL_Gamepad *pad, Uint16 low, Uint16 high) {
    if (!pad || atomic_load(&suspended)) return;
    const SDL_JoystickID id = SDL_GetGamepadID(pad);
    if (!id) return;
    const unsigned ticket = atomic_load(&generation);
    const Uint64 requested = SDL_GetTicksNS();
    dispatch_async(motor_queue(), ^{
        if (!current(ticket)) return;
        const Uint64 age = (SDL_GetTicksNS()-requested)/1000000;
        if ((low || high) && age >= 1000) return;
        SDL_Gamepad *active = SDL_OpenGamepad(id);
        if (!active) return;
        const Uint64 start = SDL_GetTicksNS();
        const bool ok = SDL_RumbleGamepad(active, low, high,
                                      (low || high) ? (Uint32)(1000-age) : 0);
        const double ms = (SDL_GetTicksNS()-start)/1e6;
        static unsigned logged;
        if ((low || high) && (logged < 8 || ms > 20)) {
            if (logged < 8) ++logged;
            printf("iOS haptic worker: low=%u high=%u ok=%d queue=%llu ms motor=%.2f ms\n",
                   low, high, ok, (unsigned long long)age, ms);
        }
        SDL_CloseGamepad(active);
    });
}
