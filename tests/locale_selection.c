#define _DARWIN_C_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#include "locale.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void file(const char *root,const char *name) {
    char p[1024];snprintf(p,sizeof(p),"%s/%s",root,name);FILE *f=fopen(p,"w");assert(f);fputs("fixture",f);fclose(f);
}
int main(void) {
    char root[]="/tmp/bb-locale-XXXXXX";assert(mkdtemp(root));char p[1024],expected[1024];
    const char *dirs[]={"dvdroot_ps4","dvdroot_ps4/msg","dvdroot_ps4/msg/enggb","dvdroot_ps4/msg/korko"};
    for(unsigned i=0;i<4;++i){snprintf(p,sizeof(p),"%s/%s",root,dirs[i]);assert(!mkdir(p,0700));}
    assert(bb_ios_locale_prepare(root)==0);assert(!getenv("BB_TEXT_LOCALE"));
    file(root,"dvdroot_ps4/msg/korko/menu.msgbnd.dcx");file(root,"dvdroot_ps4/msg/enggb/menu.msgbnd.dcx");
    assert(bb_ios_locale_prepare(root)==1);assert(!getenv("BB_TEXT_LOCALE"));
    assert(!strcmp(getenv("BB_LANGUAGE"),"9"));
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/korkr/menu.msgbnd.dcx",root);
    snprintf(expected,sizeof(expected),"%s/dvdroot_ps4/msg/korko/menu.msgbnd.dcx",root);
    assert(!bb_ios_locale_translate(p,sizeof(p)));assert(!strcmp(p,expected));
    strcpy(p,"/app0/dvdroot_ps4/msg/enggb/menu.msgbnd.dcx");strcpy(expected,p);
    assert(!bb_ios_locale_translate(p,sizeof(p)));assert(!strcmp(p,expected));
    strcpy(p,"/app0/dvdroot_ps4/msg/frafr/menu.msgbnd.dcx");strcpy(expected,p);
    assert(!bb_ios_locale_translate(p,sizeof(p)));assert(!strcmp(p,expected));
    uint32_t original[21],mapped[21];for(unsigned i=0;i<21;++i) original[i]=i;
    original[19]=1;assert(bb_ios_locale_copy_map(mapped,original));
    for(unsigned i=0;i<21;++i) assert(mapped[i]==(i==19?19:original[i]));
    original[4]=99;assert(!bb_ios_locale_copy_map(mapped,original));
    bb_ios_locale_bind_image(0,0);bb_ios_locale_default_query();
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/korko/menu.msgbnd.dcx",root);unlink(p);
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/korko",root);
    snprintf(expected,sizeof(expected),"%s/dvdroot_ps4/msg/KorKR",root);assert(!rename(p,expected));
    file(root,"dvdroot_ps4/msg/KorKR/menu.msgbnd.dcx");assert(bb_ios_locale_prepare(root)==1);
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/korkr/menu.msgbnd.dcx",root);
    snprintf(expected,sizeof(expected),"%s/dvdroot_ps4/msg/KorKR/menu.msgbnd.dcx",root);
    assert(!bb_ios_locale_translate(p,sizeof(p)));assert(!strcmp(p,expected));unlink(expected);
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/KorKR",root);rmdir(p);
    snprintf(p,sizeof(p),"%s/dvdroot_ps4/msg/enggb/menu.msgbnd.dcx",root);unlink(p);
    for(int i=2;i>=0;--i){snprintf(p,sizeof(p),"%s/%s",root,dirs[i]);rmdir(p);}rmdir(root);
    puts("PASS: Korean resource case/alias, empty detection, all 20 other mappings/selected paths preserved, invalid-layout safe fallback");return 0;
}
