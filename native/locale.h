#ifndef BB_IOS_LOCALE_H
#define BB_IOS_LOCALE_H
#include <stddef.h>
#include <stdint.h>
int bb_ios_locale_prepare(const char *game);
void bb_ios_locale_opened(const char *guest,const char *host);
int bb_ios_locale_translate(char *host,size_t capacity);
void bb_ios_locale_bind_image(uintptr_t base,size_t size);
void bb_ios_locale_default_query(void);
int bb_ios_locale_copy_map(uint32_t out[21],const uint32_t in[21]);
#endif
