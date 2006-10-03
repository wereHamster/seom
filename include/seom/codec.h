
#ifndef __SEOM_CODEC_H__
#define __SEOM_CODEC_H__

#include <seom/seom.h>

struct seomCodecTable {
	uint32_t codes[256];
	uint8_t *pointers[32];
	uint8_t data[129 * 25];
};

extern struct seomCodecTable seomCodecTable;

extern uint32_t *seomCodecEncode(uint32_t *dst, uint8_t *src, uint8_t *end, struct seomCodecTable *tbl);
extern uint32_t *seomCodecEncodeReference(uint32_t *dst, uint8_t *src, uint8_t *end, struct seomCodecTable *tbl);

extern uint8_t *seomCodecDecode(uint8_t *dst, uint32_t *src, uint8_t *end, struct seomCodecTable *tbl);
extern uint8_t *seomCodecDecodeReference(uint8_t *dst, uint32_t *src, uint8_t *end, struct seomCodecTable *tbl);

#endif /* __SEOM_CODEC_H__ */
