#!/usr/bin/env python3
from pathlib import Path
p=Path('upstream-runtime/src/runtime_file.c');s=p.read_text()
s='''#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_locale_opened(const char*,const char*);
extern int bb_ios_locale_translate(char*,size_t);
#else
#define bb_ios_locale_opened(guest,host) ((void)0)
#define bb_ios_locale_translate(host,size) 0
#endif
'''.replace('#ifdef __APPLE__','#include <stddef.h>\n#ifdef __APPLE__',1)+s
needle='    if (!fstat(host,&s) && S_ISDIR(s.st_mode)) dir=list_directory(path);'
assert s.count(needle)==1
s=s.replace(needle,'    bb_ios_locale_opened(guest,current);\n'+needle)
needle='    pthread_mutex_unlock(&lock);\n    if (result==ENOENT)'
assert s.count(needle)==1
s=s.replace(needle,'    pthread_mutex_unlock(&lock);\n    if (!result) result=bb_ios_locale_translate(out,size);\n    if (result==ENOENT)')
p.write_text(s)
p=Path('upstream-runtime/src/runtime_services.c');s=p.read_text()
s='''#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_locale_default_query(void);
#else
#define bb_ios_locale_default_query() ((void)0)
#endif
'''+s
needle='case 1: *value=language(); break;'
assert s.count(needle)==1
s=s.replace(needle,'''case 1: {
        bb_ios_locale_default_query();
        *value=language();
        static unsigned logged;
        if(__atomic_fetch_add(&logged,1,__ATOMIC_RELAXED)<8)
            printf("System language queried: SCE %d\\n",*value);
        break;
    }''')
p.write_text(s)
p=Path('upstream-runtime/src/probe.c');s=p.read_text()
s='''#include <stdint.h>
#include <stddef.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_locale_bind_image(uintptr_t,size_t);
#else
#define bb_ios_locale_bind_image(base,size) ((void)0)
#endif
'''+s
needle='    runtime_game_lock_load(bb_image_base);'
assert s.count(needle)==1
s=s.replace(needle,needle+'\n    bb_ios_locale_bind_image((uintptr_t)image,(size_t)size);')
p.write_text(s)
print('Default: guarded Korean region mapping; only Korean path alias; real message opens logged')
