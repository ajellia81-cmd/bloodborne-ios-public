#!/usr/bin/env python3
from pathlib import Path
p=Path('upstream-runtime/src/runtime_file.c');s=p.read_text()
s='''#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_locale_opened(const char*,const char*);
#else
#define bb_ios_locale_opened(guest,host) ((void)0)
#endif
'''+s
needle='    if (!fstat(host,&s) && S_ISDIR(s.st_mode)) dir=list_directory(path);'
assert s.count(needle)==1
s=s.replace(needle,'    bb_ios_locale_opened(guest,current);\n'+needle)
p.write_text(s)
p=Path('upstream-runtime/src/runtime_services.c');s=p.read_text()
needle='case 1: *value=language(); break;'
assert s.count(needle)==1
s=s.replace(needle,'''case 1: {
        *value=language();
        static unsigned logged;
        if(__atomic_fetch_add(&logged,1,__ATOMIC_RELAXED)<8)
            printf("System language queried (game Default): SCE %d\\n",*value);
        break;
    }''')
p.write_text(s)
print('Default: system language 9; explicit selections untouched; real message opens logged')
