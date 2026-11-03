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

int bb_ios_locale_prepare(const char *game) {
    char path[4096];snprintf(path,sizeof(path),"%s/dvdroot_ps4/msg/korko",game);
    DIR *dir=opendir(path);
    unsetenv("BB_TEXT_LOCALE");
    setenv("BB_LANGUAGE","9",1);
    if(!dir) { puts("Korean: cannot open expected msg/korko directory; resource availability not established");return -1; }
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
    printf("Korean: %u nonempty readable archives; Default uses SCE language 9; explicit language choices are not overridden\n",bundles);
    return (int)bundles;
}

void bb_ios_locale_opened(const char *guest,const char *host) {
    const char *p=strstr(host,"/dvdroot_ps4/msg/");
    if(!p) return;
    struct stat s;if(stat(host,&s) || !S_ISREG(s.st_mode)) return;
    static atomic_uint korean_logged,other_logged;
    int korean=!strncmp(p+strlen("/dvdroot_ps4/msg/"),"korko/",6);
    atomic_uint *counter=korean ? &korean_logged : &other_logged;
    if(atomic_fetch_add_explicit(counter,1,memory_order_relaxed)<(korean ? 24u : 8u))
        printf("%s: %s -> %s\n",korean ? "Korean file opened" : "Selected-language file opened",guest,p+1);
}
