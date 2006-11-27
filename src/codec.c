
#include <seom/seom.h>

/**
 * Based on quicklz (http://www.quicklz.com)
 * 
 * Copyright 2006, Lasse Reinhold (lar@quicklz.com)
 */

#define u32(ptr) ( *(uint32_t *) (ptr) )

static void __memcpy(void *dst, const void *src, long len)
{
	if (src + len > dst) {
		void *end = dst + len;
		while (dst < end)
			*(char *)dst++ = *(char *)src++;
	} else {
		memcpy(dst, src, len);
	}
}

uint8_t *seomCodecEncode(uint8_t *dst, const uint8_t *src, uint32_t size)
{
	const uint8_t *end = src + size;
	const uint8_t **hashtable = (const uint8_t **)(dst + size + 36000 - sizeof(uint8_t *) * 4096);
	uint8_t *cword_ptr = dst++;
	uint8_t cword_val = 0;
	uint8_t cword_counter = 8;

	for (int i = 0; i < 4096; ++i)
		hashtable[i] = src;

	while (src < end - sizeof(uint32_t)) {
		if (u32(src) == u32(src + 1)) {	/* RLE sequence */
			uint32_t fetch = u32(src);
			src += sizeof(uint32_t);
			const uint8_t *orig = src;
			while (fetch == u32(src) && src < orig + (0x0fff << 2) - sizeof(uint32_t) && src < end - sizeof(uint32_t))
				src += sizeof(uint32_t);
			uint32_t len = (src - orig) / sizeof(uint32_t);
			*dst++ = (uint8_t) 0xf0 | (len >> 8);
			*dst++ = (uint8_t) len;
			*dst++ = (uint8_t) (fetch & 0xff);
			cword_val = (cword_val << 1) | 1;
		} else {
			/* fetch source data and update hash table */
			uint32_t fetch = ntohl(u32(src));
			uint32_t hash = ((fetch >> 20) ^ (fetch >> 8)) & 0x0fff;
			const uint8_t *o = hashtable[hash];
			hashtable[hash] = src;

			uint32_t offset = (uint32_t) (src - o);
			if (offset <= 131071 && offset > 3 && ((ntohl(u32(o)) ^ ntohl(u32(src))) & 0xffffff00) == 0) {
				if (o[3] != src[3]) {
					if (offset <= 127) {	/* LZ match */
						*dst++ = offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else if (offset <= 8191) {	/* LZ match */
						*dst++ = (uint8_t) 0x80 | offset >> 8;
						*dst++ = (uint8_t) offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else {	/* literal */
						*dst++ = *src++;
						cword_val = (cword_val << 1);
					}
				} else {		/* LZ match */
					cword_val = (cword_val << 1) | 1;
					uint32_t len = 0;

					while (*(o + len + 4) == *(src + len + 4) && len < (1 << 11) - 1 && src + len + 4 < end - sizeof(uint32_t))
						++len;
					src += len + 4;
					if (len <= 7 && offset <= 1023) {	/* 10bits offset, 3bits length */
						*dst++ = (uint8_t) 0xa0 | (len << 2) | (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else if (len <= 31 && offset <= 65535) {	/* 16bits offset, 5bits length */
						*dst++ = (uint8_t) 0xc0 | len;
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else {	/* 17bits offset, 11bits length */
						*dst++ = (uint8_t) 0xe0 | (len >> 7);
						*dst++ = (uint8_t) (len << 1) | (offset >> 16);
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					}
				}
			} else {			/* literal */
				*dst++ = *src++;
				cword_val = (cword_val << 1);
			}
		}

		--cword_counter;
		if (cword_counter == 0) {	/* store control word */
			*cword_ptr = cword_val;
			cword_counter = 8;
			cword_ptr = dst++;
		}
	}

	/* save last source bytes as literals */
	while (src < end) {
		*dst++ = *src++;
		cword_val = (cword_val << 1);
		--cword_counter;
		if (cword_counter == 0) {
			*cword_ptr = cword_val;
			cword_counter = 8;
			cword_ptr = dst++;
		}
	}

	if (cword_counter > 0)
		cword_val = (cword_val << cword_counter) | (1 << (cword_counter - 1));
	*cword_ptr = cword_val;

	return dst;
}

uint8_t *seomCodecDecode(uint8_t *dst, const uint8_t *src, uint32_t size)
{
	const uint8_t *end = dst + size;
	uint8_t cword_val = *src++;
	uint8_t cword_counter = 8;

	while (dst < end - sizeof(uint32_t)) {
		if (cword_counter == 0) { /* fetch control word */
			cword_val = *src++;
			cword_counter = 8;
		}

		if (cword_val & (1 << 7)) { /* LZ match or RLE sequence */
			cword_val = (cword_val << 1) | 1;
			--cword_counter;
			if ((src[0] & 0x80) == 0) { /* 7bits offset */
				uint32_t offset = src[0];
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 1;
			} else if ((src[0] & 0x60) == 0) { /* 13bits offset */
				uint32_t offset = ((src[0] & 0x1f) << 8) | src[1];
				__memcpy(dst, dst - offset, 3);
				dst += 3;
				src += 2;
			} else if ((src[0] & 0x40) == 0) { /* 10bits offset, 3bits length */
				uint32_t len = ((src[0] >> 2) & 7) + 4;
				uint32_t offset = ((src[0] & 0x03) << 8) | src[1];
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 2;
			} else if ((src[0] & 0x20) == 0) { /* 16bits offset, 5bits length */
				uint32_t len = (src[0] & 0x1f) + 4;
				uint32_t offset = (src[1] << 8) | src[2];
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 3;
			} else if ((src[0] & 0x10) == 0) { /* 17bits offset, 11bits length */
				uint32_t len = (((src[0] & 0x0f) << 7) | (src[1] >> 1)) + 4;
				uint32_t offset = ((src[1] & 0x01) << 16) | (src[2] << 8) | (src[3]);
				__memcpy(dst, dst - offset, len);
				dst += len;
				src += 4;
			} else { /* RLE sequence */
				uint32_t len = ((src[0] & 0x0f) << 8) | src[1];
				uint32_t val = src[2] | (src[2] << 8) | (src[2] << 16) | (src[2] << 24);

				u32(dst) = val;
				dst += sizeof(uint32_t);

				const uint8_t *end = dst + len * sizeof(uint32_t);
				while (dst < end) {
					u32(dst) = val;
					dst += sizeof(uint32_t);
				}
				src += 3;
			}
		} else { /* literal */
			uint8_t index  = cword_val >> 4;
			const uint8_t map[8][2] = { { 4, 0x0f }, { 3, 0x07 }, { 2, 0x03 }, { 2, 0x03 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 }, { 1, 0x01 } };
			const uint8_t *end = dst + map[index][0];
			while (dst < end)
				*dst++ = *src++;
			cword_counter -= map[index][0];
			cword_val = (cword_val << map[index][0]) | map[index][1];
		}
	}
	
	while (dst < end) {
		if (cword_counter == 0) {
			cword_counter = 8;
			++src;
		}
		*dst++ = *src++;
		--cword_counter;
	}
	
	return dst;
}
