
#ifndef __SEOM_CODEC_H__
#define __SEOM_CODEC_H__

#include <seom/seom.h>

uint8_t *seomCodecEncode(uint8_t *dst, const uint8_t *src, uint32_t size);
uint8_t *seomCodecDecode(uint8_t *dst, const uint8_t *src, uint32_t size);

#endif /* __SEOM_CODEC_H__ */
