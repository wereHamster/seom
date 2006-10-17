
#ifndef __SEOM_CODEC_H__
#define __SEOM_CODEC_H__

#include <seom/seom.h>

struct seomCodecTable {
	uint32_t codes[256];
	uint8_t *pointers[32];
	uint8_t data[129 * 25];
};

extern struct seomCodecTable seomCodecTable;

uint32_t *seomCodecEncode(uint32_t *dst, uint8_t *src, uint32_t width, uint32_t height);
uint32_t *seomCodecDecode(uint8_t *dst, uint32_t *src, uint32_t width, uint32_t height);

#endif /* __SEOM_CODEC_H__ */
