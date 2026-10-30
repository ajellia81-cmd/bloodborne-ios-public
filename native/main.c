/* SPDX-License-Identifier: GPL-2.0-or-later
 * iOS shell for Bloodborne-native runtime (built on bbport and shadPS4).
 * No game files, generated game code or system modules are included.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int bb_probe_main(int argc,char **argv);
static int exists(const char *p) { return access(p,R_OK)==0; }
int main(int argc,char **argv) {
    (void)argc; (void)argv;
    char root[4096],game[4096],boot[4096],user[4096],patches[4096],content[4096],config[4096],log[4096];
    snprintf(root,sizeof(root),"%s/Documents/Bloodborne",getenv("HOME"));
    mkdir(root,0700);
    snprintf(game,sizeof(game),"%s/game",root);
    snprintf(boot,sizeof(boot),"%s/boot-linked.bin",root);
    snprintf(user,sizeof(user),"%s/user",root);
    snprintf(patches,sizeof(patches),"%s/patches.bin",root);
    snprintf(content,sizeof(content),"%s/content.bin",root);
    snprintf(config,sizeof(config),"%s/bbport.ini",root);
    snprintf(log,sizeof(log),"%s/runtime.log",root);
    mkdir(user,0700);
    freopen(log,"a",stdout); setvbuf(stdout,NULL,_IONBF,0);
    dup2(fileno(stdout),STDERR_FILENO);
    puts("Bloodborne iOS experimental runtime; game execution unverified.");
    setenv("BB_CONFIG",config,1);
    setenv("BB_GPU_USER_DIR",user,1);
    setenv("BB_USER_DIR",user,1);
    setenv("BB_JIT","0",1);
    setenv("BB_METAL_FRAME","1",1);
    setenv("BB_METAL_NATIVE_BOOT","1",1);
    setenv("BB_GFX_BACKEND","metal",1);
    setenv("BB_POOL_SPLIT","0",1);
    setenv("BB_DMEM_MB","4608",1);
    setenv("BB_LANGUAGE","9",1);
    setenv("BB_UI_LANGUAGE","ko",1);
    setenv("BB_ONLINE","0",1);
    setenv("BB_OUTPUT_RES","960x540",1);
    setenv("BB_RENDER_RES","960x540",1);
    setenv("BB_FPS","30",1);
    setenv("BB_FPS_LIMIT","30",1);
    setenv("BB_UPSCALER","off",1);
    if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) return 1;
    SDL_Window *window=NULL; SDL_Renderer *renderer=NULL;
    if (!SDL_CreateWindowAndRenderer("Bloodborne iOS - Experimental",960,540,SDL_WINDOW_FULLSCREEN,&window,&renderer)) return 1;
    SDL_SetRenderLogicalPresentation(renderer,960,540,SDL_LOGICAL_PRESENTATION_LETTERBOX);
    int start=0;
    for (;;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT) return 0;
            if (event.type==SDL_EVENT_FINGER_UP || event.type==SDL_EVENT_MOUSE_BUTTON_UP ||
                (event.type==SDL_EVENT_GAMEPAD_BUTTON_UP && event.gbutton.button==SDL_GAMEPAD_BUTTON_SOUTH)) start=1;
        }
        char eboot[4096]; snprintf(eboot,sizeof(eboot),"%s/eboot.bin",game);
        int ready=exists(boot)&&exists(eboot)&&exists(patches)&&exists(content);
        SDL_SetRenderDrawColor(renderer,12,14,20,255); SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,225,225,230,255); SDL_SetRenderScale(renderer,2,2);
        SDL_RenderDebugText(renderer,20,20,"Bloodborne iOS - Experimental");
        SDL_SetRenderScale(renderer,1,1);
        SDL_RenderDebugText(renderer,40,110,"Native Metal runtime. No game files are included.");
        SDL_RenderDebugText(renderer,40,140,"Files app > Bloodborne iOS > Bloodborne:");
        SDL_RenderDebugText(renderer,40,170,"  game/            Your decrypted game folder, update 1.09 merged");
        SDL_RenderDebugText(renderer,40,195,"  boot-linked.bin  Prepared from your game and system modules");
        SDL_RenderDebugText(renderer,40,220,"  patches.bin      Prepared for 960x540");
        SDL_RenderDebugText(renderer,40,240,"  content.bin      AppContent profile from your game");
        SDL_RenderDebugText(renderer,40,260,"Korean game language requested. Region/DLC compatibility unverified.");
        SDL_RenderDebugText(renderer,40,290,"This build has not been tested on a physical iPhone.");
        SDL_RenderDebugText(renderer,40,330,ready?"Files found. Tap screen or press controller A to try booting.":"Required files missing. Copy files into the app, then tap to recheck.");
        SDL_RenderDebugText(renderer,40,370,"CPU interpreter only: game performance is expected to be limited.");
        SDL_RenderDebugText(renderer,40,400,"Logs: Bloodborne/runtime.log");
        SDL_RenderPresent(renderer);
        if (start&&ready) break;
        start=0; SDL_Delay(33);
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    // A downloaded Documents dylib is not loaded: game AOT code must be embedded and re-signed
    // as part of a separate, personal build. Never publish generated game code.
    char *arguments[]={"BloodborneIOS",boot,"--app0",game,"--user",user,"--patches",patches,"--content-profile",content,"--timeout","0",NULL};
    return bb_probe_main(12,arguments);
}
