#include "haptics.h"
#include <assert.h>
#include <stdio.h>
static int calls,allow=1;static uint16_t lows[16],highs[16];static uint32_t durations[16];
SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad*p){return p->id;}
Uint64 SDL_GetTicksNS(void){static Uint64 t;t+=2000000;return t;}
const char* SDL_GetGamepadName(SDL_Gamepad*p){(void)p;return "test pad";}
bool SDL_RumbleGamepad(SDL_Gamepad*p,uint16_t l,uint16_t h,uint32_t d){(void)p;lows[calls]=l;highs[calls]=h;durations[calls++]=d;return allow;}
int main(void){
 SDL_Gamepad p={1},q={2};bb_ios_haptic_warmup(NULL);assert(calls==0);
 bb_ios_haptic_warmup(&p);assert(calls==2&&lows[0]==1&&highs[0]==1&&durations[0]==1&&lows[1]==0&&highs[1]==0&&durations[1]==0);
 bb_ios_haptic_warmup(&p);assert(calls==2);
 bb_ios_haptic_runtime_ready(&p);assert(calls==4);bb_ios_haptic_runtime_ready(&p);assert(calls==4);
 allow=0;bb_ios_haptic_warmup(&q);assert(calls==5);
 allow=1;bb_ios_haptic_warmup(&q);assert(calls==7);
 bb_ios_haptic_warmup(&q);assert(calls==7);
 puts("PASS: tiny nonzero warmup then stop, launcher/runtime phase and per-controller deduplication, unavailable-controller retry");
}
