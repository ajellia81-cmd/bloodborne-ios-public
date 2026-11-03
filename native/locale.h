#pragma once
#include <stddef.h>
void bb_ios_locale_prepare(const char *game);
void bb_ios_locale_remap(const char *guest,char *host,size_t capacity);
void bb_ios_locale_opened(const char *guest,const char *host);
