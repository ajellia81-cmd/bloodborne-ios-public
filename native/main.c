/* SPDX-License-Identifier: GPL-2.0-or-later
 * iOS shell for Bloodborne-native runtime (built on bbport and shadPS4).
 * No game files, generated game code or system modules are included.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "display.h"
#include "lifecycle.h"
#include "locale.h"
#include "haptics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
void bb_diagnostic_mark(const char *stage);
int bb_probe_main(int argc,char **argv);
static int exists(const char *p) { return access(p,R_OK)==0; }

int main(int argc,char **argv) {
    (void)argc; (void)argv;
    bb_diagnostic_mark("SDL main entered: static initialization completed");
    char root[4096],game[4096],boot[4096],user[4096],xml[4096],content[4096],config[4096],log[4096];
    snprintf(root,sizeof(root),"%s/Documents/Bloodborne",getenv("HOME") ? getenv("HOME") : ".");
    mkdir(root,0700);
    snprintf(game,sizeof(game),"%s/game",root);
    snprintf(boot,sizeof(boot),"%s/boot-linked.bin",root);
    snprintf(user,sizeof(user),"%s/user",root);
    snprintf(content,sizeof(content),"%s/content.bin",root);
    snprintf(config,sizeof(config),"%s/bbport.ini",root);
    snprintf(log,sizeof(log),"%s/runtime.log",root);
    mkdir(user,0700);
    if (freopen(log,"a",stdout)) {
        setvbuf(stdout,NULL,_IONBF,0);
        dup2(fileno(stdout),STDERR_FILENO);
    }
    puts("Bloodborne iOS build 21; parallel foreground GPU leases, retained resources, read-only PSO cache.");
    setenv("BB_CONFIG",config,1);
    setenv("BB_GPU_USER_DIR",user,1);
    setenv("BB_USER_DIR",user,1);
    setenv("BB_JIT","0",1);
    setenv("BB_IOS_PROGRESS","0",1);
    setenv("BB_METAL_FRAME","1",1);
    setenv("BB_METAL_ARCHIVE","0",1);
    setenv("BB_IOS_PSO_SHARDS","1",1);
    setenv("BB_IOS_PSO_WRITE","0",1);
    setenv("BB_AUDIO_STATS","1",1);
    setenv("BB_VK_RECORD_THREADS","2",1);
    setenv("BB_FRAMES_AHEAD","2",1);
    puts("iOS memory: monolithic archive off; bounded per-pipeline disk cache enabled");
    setenv("BB_FULLSCREEN","1",1);
    setenv("BB_RETINA","1",1);
    setenv("BB_METAL_NATIVE_BOOT","1",1);
    setenv("BB_GFX_BACKEND","metal",1);
    setenv("BB_POOL_SPLIT","0",1);
    setenv("BB_DMEM_MB","4608",1);
    setenv("BB_AUTO_GRAPHICS_HEAP","0",1);
    puts("iOS memory budget: retail graphics heaps; native output with memory-safe internal scene");
    bb_ios_locale_prepare(game);
    setenv("BB_UI_LANGUAGE","ko",1);
    setenv("BB_UI_SCALE","1.75",1);
    setenv("BB_ONLINE","0",1);
    setenv("BB_FPS","30",1);
    setenv("BB_FPS_LIMIT","30",1);
    setenv("BB_UPSCALER","metalfx",1);
    setenv("BB_UPSCALE_PRESET","3",1);
    SDL_SetHint(SDL_HINT_ORIENTATIONS,"LandscapeLeft LandscapeRight");
    bb_diagnostic_mark("entering SDL_Init");
    puts("startup: entering SDL_Init");
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {
        fprintf(stderr,"SDL_Init failed: %s\n",SDL_GetError()); return 1;
    }
    bb_diagnostic_mark("SDL_Init succeeded");
    puts("startup: SDL initialized");
    bb_ios_lifecycle_install();
    int gamepad_count=0;
    SDL_JoystickID *gamepad_ids=SDL_GetGamepads(&gamepad_count);
    for(int i=0;gamepad_ids && i<gamepad_count;i++) {
        SDL_Gamepad *pad=SDL_OpenGamepad(gamepad_ids[i]);
        bb_ios_haptic_warmup(pad);
        fprintf(stdout,"Launcher gamepad %u: %s\n",(unsigned)gamepad_ids[i],pad ? SDL_GetGamepadName(pad) : SDL_GetError());
    }
    SDL_free(gamepad_ids);
    const char *base=SDL_GetBasePath();
    if(!base) { fprintf(stderr,"SDL_GetBasePath failed: %s\n",SDL_GetError()); return 1; }
    snprintf(xml,sizeof(xml),"%spatches/Bloodborne.xml",base);
    char cjk_font[4096];
    snprintf(cjk_font,sizeof(cjk_font),"%sFonts/NotoSansKR.ttf",base);
    setenv("BB_CJK_FONT",cjk_font,1);
    char recomp_library[4096];
    snprintf(recomp_library,sizeof(recomp_library),"%sFrameworks/librecomp.dylib",base);
    const int aot_available=exists(recomp_library);
    if(aot_available) {
        setenv("BB_RECOMP_LIB",recomp_library,1);
        printf("Personal AOT library found: %s\n",recomp_library);
    } else {
        puts("No personal AOT library: interpreter fallback");
    }
    setenv("BB_PATCHES_XML",xml,1);
    setenv("BB_DATA_DIR",root,1);
    SDL_Window *window=NULL; SDL_Renderer *renderer=NULL;
    if (!SDL_CreateWindowAndRenderer("Bloodborne iOS - Experimental",960,540,SDL_WINDOW_FULLSCREEN|SDL_WINDOW_HIGH_PIXEL_DENSITY,&window,&renderer)) {
        fprintf(stderr,"SDL window/renderer failed: %s\n",SDL_GetError()); return 1;
    }
    bb_diagnostic_mark("launcher renderer created");
    puts("startup: launcher renderer created");
    SDL_SetRenderLogicalPresentation(renderer,0,0,SDL_LOGICAL_PRESENTATION_DISABLED);
    int start=0;
    for (;;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_GAMEPAD_ADDED) {
                SDL_Gamepad *pad=SDL_OpenGamepad(event.gdevice.which);
                bb_ios_haptic_warmup(pad);
                fprintf(stdout,"Launcher gamepad added: %s\n",pad ? SDL_GetGamepadName(pad) : SDL_GetError());
            }
            if(event.type==SDL_EVENT_GAMEPAD_REMOVED) {
                SDL_Gamepad *pad=SDL_GetGamepadFromID(event.gdevice.which);
                if(pad) SDL_CloseGamepad(pad);
            }
            if (event.type==SDL_EVENT_QUIT) return 0;
            if (event.type==SDL_EVENT_FINGER_UP || event.type==SDL_EVENT_MOUSE_BUTTON_UP ||
                (event.type==SDL_EVENT_GAMEPAD_BUTTON_UP && event.gbutton.button==SDL_GAMEPAD_BUTTON_SOUTH)) start=1;
        }
        char eboot[4096]; snprintf(eboot,sizeof(eboot),"%s/eboot.bin",game);
        int ready=exists(boot)&&exists(eboot)&&exists(content)&&exists(xml);
        int pixel_width=0,pixel_height=0;
        SDL_GetWindowSizeInPixels(window,&pixel_width,&pixel_height);
        float ui_scale=pixel_height>0 ? pixel_height/540.0f : 1.0f;
        SDL_SetRenderDrawColor(renderer,12,14,20,255); SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,225,225,230,255); SDL_SetRenderScale(renderer,2*ui_scale,2*ui_scale);
        SDL_RenderDebugText(renderer,20,20,"Bloodborne iOS - Experimental");
        SDL_SetRenderScale(renderer,ui_scale,ui_scale);
        SDL_RenderDebugText(renderer,40,110,"Native Metal runtime. No game files are included.");
        SDL_RenderDebugText(renderer,40,140,"Files app > Bloodborne iOS > Bloodborne:");
        SDL_RenderDebugText(renderer,40,170,"  game/            Your decrypted game folder, update 1.09 merged");
        SDL_RenderDebugText(renderer,40,195,"  boot-linked.bin  Prepared from your game and system modules");
        SDL_RenderDebugText(renderer,40,220,"  Resolution patches are built on-device for the native display.");
        SDL_RenderDebugText(renderer,40,240,"  content.bin      AppContent profile from your game");
        SDL_RenderDebugText(renderer,40,260,"Korean requested (SCE 9); available text depends on your regional game data.");
        SDL_RenderDebugText(renderer,40,290,"This build has not been tested on a physical iPhone.");
        SDL_RenderDebugText(renderer,40,330,ready?"Files found. Tap screen or press controller A to try booting.":"Required files missing. Copy files into the app, then tap to recheck.");
        SDL_RenderDebugText(renderer,40,370,aot_available ? "Personal AOT library present; runtime validates functions at boot." : "CPU interpreter only: game performance is expected to be limited.");
        SDL_RenderDebugText(renderer,40,400,"Logs: Bloodborne/runtime.log");
        char display_text[128];
        snprintf(display_text,sizeof(display_text),"Native display: %d x %d pixels, aspect %.5f",pixel_width,pixel_height,pixel_height ? (double)pixel_width/pixel_height : 0.0);
        SDL_RenderDebugText(renderer,40,440,display_text);
        SDL_RenderPresent(renderer);
        if (start&&ready && !bb_ios_screen_profile(window,config)) break;
        start=0; SDL_Delay(33);
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    // A downloaded Documents dylib is not loaded: game AOT code must be embedded and re-signed
    // as part of a separate, personal build. Never publish generated game code.
    char *arguments[]={"BloodborneIOS",boot,"--app0",game,"--user",user,"--patches","-","--content-profile",content,"--timeout","0",NULL};
    bb_diagnostic_mark("entering game runtime");
    int result=bb_probe_main(12,arguments);
    bb_diagnostic_mark("game runtime returned");
    return result;
}






