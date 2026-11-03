#define _POSIX_C_SOURCE 200809L
#include "locale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

static char korean_dir[256];
static uintptr_t image_base;
static size_t image_size;
static pthread_mutex_t map_lock=PTHREAD_MUTEX_INITIALIZER;
static uint32_t maps[6][21];
static int installed[6];

static int archives(const char *path,const char *name) {
    DIR *d=opendir(path);if(!d) return -1;
    unsigned count=0;
    for(struct dirent *e;(e=readdir(d));) {
        size_t n=strlen(e->d_name);
        if(!((n>=7&&!strcasecmp(e->d_name+n-7,".msgbnd")) ||
             (n>=11&&!strcasecmp(e->d_name+n-11,".msgbnd.dcx")))) continue;
        char f[4096];struct stat s;
        if(snprintf(f,sizeof(f),"%s/%s",path,e->d_name)>=(int)sizeof(f)) continue;
        if(!stat(f,&s)&&S_ISREG(s.st_mode)&&s.st_size>0&&!access(f,R_OK)) {
            if(count++<16) printf("Korean message archive found: msg/%s/%s (%lld bytes)\n",name,e->d_name,(long long)s.st_size);
        }
    }
    closedir(d);return (int)count;
}

int bb_ios_locale_prepare(const char *game) {
    korean_dir[0]=0;
    unsetenv("BB_TEXT_LOCALE");setenv("BB_LANGUAGE","9",1);
    char root[4096];
    if(snprintf(root,sizeof(root),"%s/dvdroot_ps4/msg",game)>=(int)sizeof(root)) return -1;
    DIR *d=opendir(root);
    if(!d) { printf("Korean: cannot inspect %s: errno=%d (%s)\n",root,errno,strerror(errno));return -1; }
    int best=-1,rank=0;
    for(struct dirent *e;(e=readdir(d));) {
        if(strncasecmp(e->d_name,"kor",3)) continue;
        char path[4096];
        if(snprintf(path,sizeof(path),"%s/%s",root,e->d_name)>=(int)sizeof(path)) continue;
        int n=archives(path,e->d_name);
        printf("Korean directory probe: %s; readable nonempty archives=%d\n",path,n);
        int r=!strcasecmp(e->d_name,"korkr")?3:!strcasecmp(e->d_name,"korko")?2:1;
        if(strlen(e->d_name)<sizeof(korean_dir) && (n>0?(best<=0||r>rank):(best<0&&n==0))) {
            best=n;rank=r;snprintf(korean_dir,sizeof(korean_dir),"%s",e->d_name);
        }
    }
    closedir(d);
    if(best<=0) korean_dir[0]=0;
    printf("Korean: selected message directory=%s, archives=%d; only system Default maps to Korean\n",korean_dir[0]?korean_dir:"unresolved",best);
    return best;
}

/* The executable's locale is korKR. Alias only that locale to the user's actual directory;
 * English and every other explicitly selected language keep their original paths. */
int bb_ios_locale_translate(char *host,size_t capacity) {
    const char *tag="/dvdroot_ps4/msg/";
    char *at=strstr(host,tag);if(!at||!korean_dir[0]) return 0;
    at+=strlen(tag);
    if(strncasecmp(at,"korkr",5)||(at[5]!='/'&&at[5]!=0)) return 0;
    char tail[4096];
    if(strlen(at+5)>=sizeof(tail)) return ENAMETOOLONG;
    strcpy(tail,at+5);
    size_t prefix=(size_t)(at-host),needed=prefix+strlen(korean_dir)+strlen(tail)+1;
    if(needed>capacity) return ENAMETOOLONG;
    strcpy(at,korean_dir);strcat(at,tail);return 0;
}

int bb_ios_locale_copy_map(uint32_t out[21],const uint32_t in[21]) {
    for(unsigned i=0;i<21;++i) if(in[i]>20) return 0;
    memcpy(out,in,21*sizeof(*out));out[19]=19;return 1;
}

/* CUSA03173 1.09 only. Guard the complete resolver and SCE mapping before using offsets.
 * Data-only replacement also covers its inlined copies; AOT code remains unchanged. */
void bb_ios_locale_bind_image(uintptr_t base,size_t size) {
    image_base=0;image_size=0;memset(installed,0,sizeof(installed));
    if(!base||size<0x55995a0) return;
    uint64_t h=UINT64_C(14695981039346656037);
    const unsigned char *code=(const unsigned char*)(base+0x1fb7130);
    for(size_t i=0;i<0xeb;++i) h=(h^code[i])*UINT64_C(1099511628211);
    uint32_t sce9;memcpy(&sce9,(void*)(base+0x4736630+9*4),4);
    if(h!=UINT64_C(0x8387d5f48524a90c)||sce9!=19) {
        puts("Korean Default: executable layout guard failed; no mapping changed");return;
    }
    image_base=base;image_size=size;
    puts("Korean Default: executable resolver layout verified (SCE 9 -> game locale 19)");
}

void bb_ios_locale_default_query(void) {
    if(!image_base||!korean_dir[0]) return;
    pthread_mutex_lock(&map_lock);
    uint32_t region;memcpy(&region,(void*)(image_base+0x5128948),4);
    if(region<6&&!installed[region]) {
        uintptr_t slot=image_base+0x55994e0+region*32+16,table;
        memcpy(&table,(void*)slot,sizeof(table));
        if(table>=image_base&&table-image_base<=image_size-21*4 &&
           bb_ios_locale_copy_map(maps[region],(const uint32_t*)table)) {
            uint32_t before=((const uint32_t*)table)[19];
            uintptr_t replacement=(uintptr_t)maps[region];
            memcpy((void*)slot,&replacement,sizeof(replacement));
            installed[region]=1;
            printf("Korean Default: region %u, Korean mapping %u -> 19; other 20 language mappings preserved; msg/%s\n",region,before,korean_dir);
        } else printf("Korean Default: region %u table is not ready/valid; mapping unchanged\n",region);
    }
    pthread_mutex_unlock(&map_lock);
}

void bb_ios_locale_opened(const char *guest,const char *host) {
    const char *p=strstr(host,"/dvdroot_ps4/msg/");if(!p) return;
    struct stat s;if(stat(host,&s)||!S_ISREG(s.st_mode)) return;
    static atomic_uint korean_logged,other_logged;
    int korean=!strncasecmp(p+strlen("/dvdroot_ps4/msg/"),"kor",3);
    atomic_uint *counter=korean?&korean_logged:&other_logged;
    if(atomic_fetch_add_explicit(counter,1,memory_order_relaxed)<(korean?24u:8u))
        printf("%s: %s -> %s\n",korean?"Korean file opened":"Selected-language file opened",guest,p+1);
}
