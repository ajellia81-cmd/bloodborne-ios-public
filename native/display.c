#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* Keep all other options, but make output and projection follow the phone exactly. */
static int display_profile(const char *path,int width,int height,int scaled,int *preset,char upscaler[32]) {
    char marker[4096];snprintf(marker,sizeof(marker),"%s.ps4-defaults-v1",path);
    int reset=access(marker,F_OK)!=0;
    const char *keys[]={"effect_chromatic_aberration","effect_dof","effect_motion_blur","effect_ssao","effect_game_aa","effect_dynamic_shadows","effect_ssr","effect_bloom","model_lod"};
    const int values[]={1,1,1,1,1,1,0,1,0};
    char temp[4096]; snprintf(temp,sizeof(temp),"%s.tmp",path);
    FILE *out=fopen(temp,"w"); if (!out) return -1;
    FILE *in=fopen(path,"r"); char line[1024];
    while (in && fgets(line,sizeof(line),in)) {
        char key[128]={0};
        if (sscanf(line," %127[^=] =",key)==1) {
            size_t n=strlen(key); while(n && (key[n-1]==' '||key[n-1]=='\t')) key[--n]=0;
            if(!strcmp(key,"preset")) { int v; if(sscanf(line,"%*[^=]=%d",&v)==1 && v>=0&&v<=4)*preset=v; }
            if(!strcmp(key,"upscaler")) { char v[32];if(sscanf(line,"%*[^=]=%31s",v)==1 && (!strcmp(v,"off")||!strcmp(v,"metalfx")||!strcmp(v,"taa")))snprintf(upscaler,32,"%s",v); }
            int original_effect=0;for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i)if(!strcmp(key,keys[i]))original_effect=1;
            if(reset&&original_effect)continue;
            if (!strcmp(key,"output_res")||!strcmp(key,"aspect_ratio")||!strcmp(key,"live_resolution")||!strcmp(key,"upscaler")||!strcmp(key,"preset")) continue;
        }
        fputs(line,out);
    }
    if(in) fclose(in);
    fprintf(out,"\noutput_res=%dx%d\naspect_ratio=auto\nlive_resolution=0\n",width,height);
    if(!scaled){*preset=0;snprintf(upscaler,32,"off");}
    fprintf(out,"upscaler=%s\npreset=%d\n",upscaler,*preset);
    if(reset)for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i)fprintf(out,"%s=%d\n",keys[i],values[i]);
    if(fclose(out)) { unlink(temp); return -1; }
    if(rename(temp,path))return -1;
    if(reset){FILE *m=fopen(marker,"w");if(!m)return -1;fputs("PS4 effect defaults applied once; user changes preserved\n",m);if(fclose(m))return -1;}
    return 0;
}
/* Match upstream even_size(v,2): ties-to-even, without a math-library dependency. */
static int half_even(int value) {
    int q=value/4,r=value%4;
    if(r>2 || (r==2 && (q&1))) ++q;
    return q*2;
}
int bb_ios_screen_profile(SDL_Window *window,const char *config) {
    int width=0,height=0;
    if (!SDL_GetWindowSizeInPixels(window,&width,&height)||width<=0||height<=0) return -1;
    if(height>width) { int t=width;width=height;height=t; }
    /* Guest half-resolution surfaces require even dimensions; current iPhone panels are even. */
    width &= ~1; height &= ~1;
    if(width<640||height<360) return -1;
    char resolution[64]; snprintf(resolution,sizeof(resolution),"%dx%d",width,height);
    int scaled=width>=1280 && height>=720;
    int render_width=scaled ? half_even(width) : width;
    int render_height=scaled ? half_even(height) : height;
    int preset=scaled?3:0;char upscaler[32];snprintf(upscaler,sizeof(upscaler),"%s",scaled?"metalfx":"off");
    if(display_profile(config,width,height,scaled,&preset,upscaler)) return -1;
    if(!strcmp(upscaler,"off")){render_width=width;render_height=height;}
    else if(preset!=3){
        static const double scales[]={1.0,1.5,1.7,2.0,3.0};
        render_width=(int)(width/scales[preset]/2.0+0.5)*2;
        render_height=(int)(height/scales[preset]/2.0+0.5)*2;
    }
    setenv("BB_OUTPUT_RES",resolution,1);
    char render[64];snprintf(render,sizeof(render),"%dx%d",render_width,render_height);
    setenv("BB_RENDER_RES",render,1);
    setenv("BB_UPSCALER",upscaler,1);
    char preset_text[8];snprintf(preset_text,sizeof(preset_text),"%d",preset);setenv("BB_UPSCALE_PRESET",preset_text,1);
    printf("iOS scene profile: %s -> native %s; upscaler=%s preset=%d; PS4 effect defaults seeded once\n",render,resolution,upscaler,preset);
    printf("iOS native display: %s, aspect %.9f, fullscreen, automatic projection\n",resolution,(double)width/height);
    return 0;
}



