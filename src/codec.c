
#include <seom/codec.h>

/**
 * Based on quicklz (http://www.quicklz.com)
 * 
 * Copyright 2006, Lasse Reinhold (lar@quicklz.com)
 */

#define min(v0,v1) ( (v0) < (v1) ? (v0) : (v1) )
#define u8(ptr) ( *(uint8_t *) (ptr) )
#define u32(ptr) ( *(uint32_t *) (ptr) )

static void __memcpy(void *dst, const void *src, unsigned long len)
{
	if (src + len > dst) {
		const void *end = dst + len;
		while (dst < end)
			*(char *)dst++ = *(char *)src++;
	} else {
		memcpy(dst, src, len);
	}
}

void *seomCodecEncode(void *dst, const void *src, unsigned long size, void *ctx)
{
	const void *end = src + size;
	void **hashtable = ctx;
	void *cptr = dst++;
	uint8_t cbyte = 0;
	unsigned char counter = 8;

	for (int i = 0; i < 4096; ++i)
		hashtable[i] = (void *) src;

	while (src < end - 4) {
		if (u32(src) == u32(src + 1)) { /* RLE sequence */
			uint8_t val = u8(src);
			src += 5;
			const void *start = src;
			const void *last = min(start + 0x0fff, end);
			while (src < last && val == u8(src))
				++src;
			unsigned long len = src - start;
			u8(dst++) = (uint8_t) 0xf0 | (len >> 8);
			u8(dst++) = (uint8_t) len;
			u8(dst++) = (uint8_t) val;
			cbyte = (cbyte << 1) | 1;
		} else {
			/* fetch source data and update hash table */
			uint32_t fetch = ntohl(u32(src));
			unsigned long hash = ((fetch >> 20) ^ (fetch >> 8)) & 0x0fff;
			const void *o = hashtable[hash];
			hashtable[hash] = (void *) src;

			unsigned long offset = src - o;
			if (offset < 131072 && offset > 3 && ((ntohl(u32(o)) ^ ntohl(u32(src))) & 0xffffff00) == 0) {
				if (u8(o + 3) != u8(src + 3)) {
					if (offset < 128) { /* LZ match */
						u8(dst++) = offset;
						cbyte = (cbyte << 1) | 1;
						src += 3;
					} else if (offset < 8192) { /* LZ match */
						u8(dst++) = (uint8_t) 0x80 | offset >> 8;
						u8(dst++) = (uint8_t) offset;
						cbyte = (cbyte << 1) | 1;
						src += 3;
					} else { /* literal */
						u8(dst++) = u8(src++);
						cbyte = (cbyte << 1);
					}
				} else { /* LZ match */
					cbyte = (cbyte << 1) | 1;
					unsigned long len = 0;

					while (u8(o + len + 4) == u8(src + len + 4) && len < (1 << 11) - 1 && src + len + 5 < end)
						++len;
					src += len + 4;

					if (len < 8 && offset < 1024) { /* 10bits offset, 3bits length */
						u8(dst++) = (uint8_t) 0xa0 | (len << 2) | (offset >> 8);
						u8(dst++) = (uint8_t) offset;
					} else if (len < 32 && offset < 65536) { /* 16bits offset, 5bits length */
						u8(dst++) = (uint8_t) 0xc0 | len;
						u8(dst++) = (uint8_t) (offset >> 8);
						u8(dst++) = (uint8_t) offset;
					} else { /* 17bits offset, 11bits length */
						u8(dst++) = (uint8_t) 0xe0 | (len >> 7);
						u8(dst++) = (uint8_t) (len << 1) | (offset >> 16);
						u8(dst++) = (uint8_t) (offset >> 8);
						u8(dst++) = (uint8_t) offset;
					}
				}
			} else { /* literal */
				u8(dst++) = u8(src++);
				cbyte = (cbyte << 1);
			}
		}

		--counter;
		if (counter == 0) { /* store control byte */
			u8(cptr) = cbyte;
			counter = 8;
			cptr = dst++;
		}
	}

	/* save last source bytes as literals */
	while (src < end) {
		u8(dst++) = u8(src++);
		cbyte = (cbyte << 1);
		--counter;
		if (counter == 0) {
			u8(cptr) = cbyte;
			counter = 8;
			cptr = dst++;
		}
	}

	if (counter > 0)
		cbyte = (cbyte << counter) | (1 << (counter - 1));
	u8(cptr) = cbyte;

	return dst;
}

void *seomCodecDecode(void *dst, const void *src, unsigned long size)
{
	const void *end = dst + size;
	unsigned char counter = 8;
	uint8_t cbyte = u8(src++);

	while (dst < end - 4) {
		if (counter == 0) { /* fetch control byte */
			cbyte = u8(src++);
			counter = 8;
		}

		if (cbyte & (1 << 7)) { /* LZ match or RLE sequence */
			cbyte = (cbyte << 1) | 1;
			--counter;
			if ((u8(src) & 0x80) == 0) { /* 7bits offset */
				unsigned long offset = u8(src);
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 1;
			} else if ((u8(src) & 0x60) == 0) { /* 13bits offset */
				unsigned long offset = ((u8(src) & 0x1f) << 8) | u8(src + 1);
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 2;
			} else if ((u8(src) & 0x40) == 0) { /* 10bits offset, 3bits length */
				unsigned long len = ((u8(src) >> 2) & 7) + 4;
				unsigned long offset = ((u8(src) & 0x03) << 8) | u8(src + 1);
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 2;
			} else if ((u8(src) & 0x20) == 0) { /* 16bits offset, 5bits length */
				unsigned long len = (u8(src) & 0x1f) + 4;
				unsigned long offset = (u8(src + 1) << 8) | u8(src + 2);
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 3;
			} else if ((u8(src) & 0x10) == 0) { /* 17bits offset, 11bits length */
				unsigned long len = (((u8(src) & 0x0f) << 7) | (u8(src + 1) >> 1)) + 4;
				unsigned long offset = ((u8(src + 1) & 0x01) << 16) | (u8(src + 2) << 8) | (u8(src + 3));
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 4;
			} else { /* RLE sequence */
				unsigned long len = (((u8(src) & 0x0f) << 8) | u8(src + 1)) + 5;
				memset(dst, u8(src + 2), len);
				dst += len;
				src += 3;
			}
		} else { /* literal */
			static const uint8_t map[8][2] = { { 4, 0x0f }, { 3, 0x07 }, { 2, 0x03 }, { 2, 0x03 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 } };
			unsigned char index = cbyte >> 4;
			memcpy(dst, src, map[index][0]);
			dst += map[index][0];
			src += map[index][0];
			counter -= map[index][0];
			cbyte = (cbyte << map[index][0]) | map[index][1];
		}
	}
	
	while (dst < end) {
		if (counter == 0) {
			counter = 8;
			++src;
		}
		u8(dst++) = u8(src++);
		--counter;
	}
	
	return dst;
}
