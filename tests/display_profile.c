#include "../native/display.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct { int width,height; } Display;
bool SDLCALL SDL_GetWindowSizeInPixels(SDL_Window *window,int *w,int *h) {
    const Display *d=(const Display *)window;*w=d->width;*h=d->height;return d->width>0 && d->height>0;
}
int main(void) {
    char dir[]="/tmp/bb-ios-display-XXXXXX";assert(mkdtemp(dir));
    char path[512];snprintf(path,sizeof(path),"%s/bbport.ini",dir);
    FILE *f=fopen(path,"w");assert(f);fputs("brightness=0.8\noutput_res=960x540\naspect_ratio=16:9\nlive_resolution=1\n",f);assert(!fclose(f));
    Display portrait={1320,2868},landscape={2868,1320},invalid={0,0};
    assert(!bb_ios_screen_profile((SDL_Window *)&portrait,path));
    assert(!strcmp(getenv("BB_RENDER_RES"),"1434x660"));
    assert(!strcmp(getenv("BB_OUTPUT_RES"),"2868x1320"));
    assert(!bb_ios_screen_profile((SDL_Window *)&landscape,path));
    char text[2048]={0};f=fopen(path,"r");assert(f);assert(fread(text,1,sizeof(text)-1,f)>0);fclose(f);
    assert(strstr(text,"brightness=0.8"));
    char *output=strstr(text,"output_res=2868x1320");assert(output && !strstr(output+1,"output_res="));
    assert(strstr(text,"aspect_ratio=auto") && !strstr(text,"16:9") && !strstr(text,"960x540"));
    assert(strstr(text,"live_resolution=0"));
    assert(strstr(text,"upscaler=metalfx") && strstr(text,"preset=3"));
    assert(!strcmp(getenv("BB_UPSCALER"),"metalfx"));
    assert(strstr(text,"effect_ssao=1")&&strstr(text,"effect_ssr=0")&&strstr(text,"model_lod=0"));
    f=fopen(path,"a");assert(f);fputs("preset=4\neffect_ssao=0\n",f);assert(!fclose(f));
    assert(!bb_ios_screen_profile((SDL_Window *)&landscape,path));
    assert(!strcmp(getenv("BB_RENDER_RES"),"956x440"));
    f=fopen(path,"r");assert(f);memset(text,0,sizeof(text));assert(fread(text,1,sizeof(text)-1,f)>0);fclose(f);
    assert(strstr(text,"effect_ssao=0")&&strstr(text,"preset=4"));
    Display small={960,540};assert(!bb_ios_screen_profile((SDL_Window *)&small,path));
    assert(!strcmp(getenv("BB_RENDER_RES"),"960x540"));
    assert(!strcmp(getenv("BB_UPSCALER"),"off"));
    assert(bb_ios_screen_profile((SDL_Window *)&invalid,path)<0);
    assert(bb_ios_screen_profile((SDL_Window *)&landscape,"/no-such-bb-directory/settings.ini")<0);
    char marker[600];snprintf(marker,sizeof(marker),"%s.ps4-defaults-v1",path);unlink(marker);unlink(path);rmdir(dir);
    puts("PASS: native output, half-size scene, MetalFX profile, small-display fallback, aspect and settings preservation");
    return 0;
}


