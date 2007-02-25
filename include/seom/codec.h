
#ifndef __SEOM_CODEC_H__
#define __SEOM_CODEC_H__

#include <seom/seom.h>

void *seomCodecEncode(void *dst, const void *src, unsigned long size);
void *seomCodecDecode(void *dst, const void *src, unsigned long size);

#endif /* __SEOM_CODEC_H__ */
