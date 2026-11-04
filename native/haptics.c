#include "haptics.h"
#include <stdio.h>
/* SDL 3.4.18 MFI creates engines and pattern players on the first nonzero request.
 * Retain the launcher's open pad through game startup and pay that cost before gameplay. */
static void warm(SDL_Gamepad *pad,unsigned phase){
    static struct {SDL_JoystickID id;unsigned phases;} warmed[32];static unsigned next;
    if(!pad)return;
    SDL_JoystickID id=SDL_GetGamepadID(pad);if(!id)return;
    unsigned slot=32;
    for(unsigned i=0;i<32;++i)if(warmed[i].id==id){if(warmed[i].phases&phase)return;slot=i;break;}
    Uint64 start=SDL_GetTicksNS();
    bool ok=SDL_RumbleGamepad(pad,1,1,1);
    if(ok){SDL_RumbleGamepad(pad,0,0,0);if(slot==32){slot=next++%32;warmed[slot].id=id;warmed[slot].phases=0;}warmed[slot].phases|=phase;}
    printf("iOS haptics warmup: %s, phase=%u %s, %.2f ms\n",SDL_GetGamepadName(pad),phase,ok?"ready":"unavailable",(SDL_GetTicksNS()-start)/1000000.0);
}

void bb_ios_haptic_warmup(SDL_Gamepad *pad){warm(pad,1);}
void bb_ios_haptic_runtime_ready(SDL_Gamepad *pad){warm(pad,2);}
