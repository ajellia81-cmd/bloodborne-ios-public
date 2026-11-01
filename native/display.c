#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Keep all other options, but make output and projection follow the phone exactly. */
static int display_profile(const char *path,int width,int height) {
    char temp[4096]; snprintf(temp,sizeof(temp),"%s.tmp",path);
    FILE *out=fopen(temp,"w"); if (!out) return -1;
    FILE *in=fopen(path,"r"); char line[1024];
    while (in && fgets(line,sizeof(line),in)) {
        char key[128]={0};
        if (sscanf(line," %127[^=] =",key)==1) {
            size_t n=strlen(key); while(n && (key[n-1]==' '||key[n-1]=='\t')) key[--n]=0;
            if (!strcmp(key,"output_res")||!strcmp(key,"aspect_ratio")||!strcmp(key,"live_resolution")) continue;
        }
        fputs(line,out);
    }
    if(in) fclose(in);
    fprintf(out,"\noutput_res=%dx%d\naspect_ratio=auto\nlive_resolution=0\n",width,height);
    if(fclose(out)) { unlink(temp); return -1; }
    return rename(temp,path);
}
int bb_ios_screen_profile(SDL_Window *window,const char *config) {
    int width=0,height=0;
    if (!SDL_GetWindowSizeInPixels(window,&width,&height)||width<=0||height<=0) return -1;
    if(height>width) { int t=width;width=height;height=t; }
    /* Guest half-resolution surfaces require even dimensions; current iPhone panels are even. */
    width &= ~1; height &= ~1;
    if(width<640||height<360) return -1;
    char resolution[64]; snprintf(resolution,sizeof(resolution),"%dx%d",width,height);
    if(display_profile(config,width,height)) return -1;
    setenv("BB_OUTPUT_RES",resolution,1); setenv("BB_RENDER_RES",resolution,1);
    printf("iOS native display: %s, aspect %.9f, fullscreen, automatic projection\n",resolution,(double)width/height);
    return 0;
}

