#define _POSIX_C_SOURCE 200809L
#include "locale.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void file(const char *root,const char *name) {
    char p[1024];snprintf(p,sizeof(p),"%s/%s",root,name);FILE *f=fopen(p,"w");assert(f);fputs("fixture",f);fclose(f);
}
int main(void) {
    char root[]="/tmp/bb-locale-XXXXXX";assert(mkdtemp(root));char p[1024],original[1024],expected[1024];
    const char *dirs[]={"dvdroot_ps4","dvdroot_ps4/msg","dvdroot_ps4/msg/engus","dvdroot_ps4/msg/korko"};
    for(unsigned i=0;i<4;++i){snprintf(p,sizeof(p),"%s/%s",root,dirs[i]);assert(!mkdir(p,0700));}
    bb_ios_locale_prepare(root);assert(!getenv("BB_TEXT_LOCALE"));
    file(root,"dvdroot_ps4/msg/korko/menu.msgbnd.dcx");file(root,"dvdroot_ps4/msg/engus/menu.msgbnd.dcx");
    bb_ios_locale_prepare(root);assert(!strcmp(getenv("BB_TEXT_LOCALE"),"korko"));assert(!strcmp(getenv("BB_LANGUAGE"),"9"));
    snprintf(original,sizeof(original),"%s/dvdroot_ps4/msg/engus/menu.msgbnd.dcx",root);
    snprintf(expected,sizeof(expected),"%s/dvdroot_ps4/msg/korko/menu.msgbnd.dcx",root);
    strcpy(p,original);bb_ios_locale_remap("/app0/dvdroot_ps4/msg/engus/menu.msgbnd.dcx",p,sizeof(p));assert(!strcmp(p,expected));
    strcpy(p,original);bb_ios_locale_remap("/savedata0/dvdroot_ps4/msg/engus/menu.msgbnd.dcx",p,sizeof(p));assert(!strcmp(p,original));
    strcpy(p,original);bb_ios_locale_remap("/app0/dvdroot_ps4/msg/engus/menu.msgbnd.dcx",p,8);assert(!strcmp(p,original));
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/engus/missing.msgbnd.dcx",root);strcpy(original,p);
    bb_ios_locale_remap("/app0/dvdroot_ps4/msg/engus/missing.msgbnd.dcx",p,sizeof(p));assert(!strcmp(p,original));
    puts("PASS: actual archive detection, Korean override, missing-file fallback, save isolation, bounds");
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/engus/menu.msgbnd.dcx",root);unlink(p);unlink(expected);
    for(int i=3;i>=0;--i){snprintf(p,sizeof(p),"%s/%s",root,dirs[i]);rmdir(p);}rmdir(root);
    return 0;
}
