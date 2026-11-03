#define _POSIX_C_SOURCE 200809L
#include "locale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdatomic.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void bb_ios_locale_prepare(const char *game) {
    char path[4096];snprintf(path,sizeof(path),"%s/dvdroot_ps4/msg/korko",game);
    DIR *dir=opendir(path);
    unsetenv("BB_TEXT_LOCALE");
    setenv("BB_LANGUAGE","9",1);
    if(!dir) { puts("Korean: cannot open expected msg/korko directory; resource availability not established");return; }
    unsigned bundles=0,shown=0;
    for(struct dirent *entry;(entry=readdir(dir));) {
        if(entry->d_name[0]=='.') continue;
        char file[4096];struct stat s;
        if(snprintf(file,sizeof(file),"%s/%s",path,entry->d_name)>=(int)sizeof(file)) continue;
        if(!stat(file,&s) && S_ISREG(s.st_mode) && s.st_size>0 &&
            ((strlen(entry->d_name)>=7 && !strcmp(entry->d_name+strlen(entry->d_name)-7,".msgbnd")) ||
             (strlen(entry->d_name)>=11 && !strcmp(entry->d_name+strlen(entry->d_name)-11,".msgbnd.dcx"))) && !access(file,R_OK)) {
            ++bundles;
            if(shown++<16) printf("Korean message archive found: msg/korko/%s (%lld bytes)\n",entry->d_name,(long long)s.st_size);
        }
    }
    closedir(dir);
    if(bundles) {
        setenv("BB_TEXT_LOCALE","korko",1);
        printf("Korean: %u nonempty readable message archives; korko override enabled; game rendering still needs verification\n",bundles);
    } else puts("Korean: msg/korko exists, but no readable nonempty .msgbnd archives found; no file override enabled");
}

/* Read-only selection: never rename or rewrite the player's files, and never remap saves.
 * Used by the shared guest path resolver, so stat/access/open select the same existing file. */
void bb_ios_locale_remap(const char *guest,char *host,size_t capacity) {
    const char *requested=getenv("BB_TEXT_LOCALE");
    if(!requested || strcmp(requested,"korko") || !guest || !host ||
       (strncmp(guest,"/app0/",6) && strncmp(guest,"/hostapp/",9)) || strstr(guest,"/../")) return;
    const char *prefixes[]={"/dvdroot_ps4/msg/","/dvdroot_ps4/font/"};
    for(unsigned p=0;p<2;++p) {
        if(!strstr(guest,prefixes[p])) continue;
        char *start=strstr(host,prefixes[p]);if(!start) continue;
        char *locale=start+strlen(prefixes[p]);char *slash=strchr(locale,'/');
        if(!slash || slash-locale!=5 || !slash[1] || !strncmp(locale,"korko/",6)) continue;
        int valid=1;for(unsigned i=0;i<5;++i) if(!isalpha((unsigned char)locale[i])) valid=0;
        if(!valid) continue;
        char candidate[4096];size_t before=(size_t)(locale-host);
        int n=snprintf(candidate,sizeof(candidate),"%.*skorko%s",(int)before,host,slash);
        if(n<0 || (size_t)n>=sizeof(candidate) || (size_t)n>=capacity) return;
        struct stat s;
        if(stat(candidate,&s) || !S_ISREG(s.st_mode) || s.st_size<=0 || access(candidate,R_OK)) return;
        memcpy(host,candidate,(size_t)n+1);
        return;
    }
}
void bb_ios_locale_opened(const char *guest,const char *host) {
    struct stat s;if(stat(host,&s) || !S_ISREG(s.st_mode)) return;
    const char *p=strstr(host,"/dvdroot_ps4/msg/korko/");
    if(!p) p=strstr(host,"/dvdroot_ps4/font/korko/");
    if(p) {
        static atomic_uint logged;
        if(atomic_fetch_add_explicit(&logged,1,memory_order_relaxed)<24)
            printf("Korean file opened: %s -> %s\n",guest,p+1);
    }
}
