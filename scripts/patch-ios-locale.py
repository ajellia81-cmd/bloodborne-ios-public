#!/usr/bin/env python3
from pathlib import Path
p=Path('upstream-runtime/src/runtime_file.c');s=p.read_text()
s='''#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern void bb_ios_locale_remap(const char*,char*,unsigned long);
extern void bb_ios_locale_opened(const char*,const char*);
#else
#define bb_ios_locale_remap(guest,host,size) ((void)0)
#define bb_ios_locale_opened(guest,host) ((void)0)
#endif
'''+s
needle='    if (result==ENOENT) fprintf(stderr,"Runtime: no mount for guest path %s\\n",guest);'
assert s.count(needle)==1
s=s.replace(needle,'    if (!result) bb_ios_locale_remap(buffer,out,size);\n'+needle)
needle='    if (!fstat(host,&s) && S_ISDIR(s.st_mode)) dir=list_directory(path);'
assert s.count(needle)==1
s=s.replace(needle,'    bb_ios_locale_opened(guest,current);\n'+needle)
p.write_text(s)
print('Korean: shared stat/access/open path selection; successful opens logged')
