
#include <seom/seom.h>

#define u8 (ptr) ( *(uint8_t  *) (ptr) )
#define u16(ptr) ( *(uint16_t *) (ptr) )
#define u32(ptr) ( *(uint32_t *) (ptr) )

uint8_t *qlz_compress(const uint8_t *source, uint8_t *destination, uint32_t size)
{
	const uint8_t *last_byte = source + size - 1;
	const uint8_t *src = source;
	const uint8_t **hashtable = (const uint8_t **)(destination + size + 36000 - sizeof(uint8_t *) * 4096);
	uint8_t *cword_ptr = destination;
	uint8_t *dst = destination + sizeof(uint32_t);
	uint32_t cword_val = 0;
	uint32_t cword_counter = 31;

	/* save first byte uncompressed */
	*dst++ = *src++;
	
	for (int i = 0; i < 4096; ++i)
		hashtable[i] = src;

	while (src < last_byte - sizeof(uint32_t)) {
		if (--cword_counter == 0) { /* store control word */
			u32(cword_ptr) = htonl((cword_val << 1) | 1);
			cword_counter = 31;
			cword_ptr = dst;
			dst += sizeof(uint32_t);
		}
		
		if (u32(src) == u32(src + 1)) { /* RLE sequence */
			uint32_t fetch = u32(src);
			src += sizeof(uint32_t);
			const uint8_t *orig = src;
			while (fetch == u32(src) && src < orig + (0x0fff << 2) - sizeof(uint32_t) && src < last_byte - sizeof(uint32_t))
				src += sizeof(uint32_t);
			uint32_t len = (src - orig) / sizeof(uint32_t);
			*dst++ = (uint8_t) 0xf0 | (len >> 8);
			*dst++ = (uint8_t) len;
			*dst++ = (uint8_t) (fetch & 0xff);
			cword_val = (cword_val << 1) | 1;
		} else {
			const uint8_t *o;

			/* fetch source data and update hash table */
			uint32_t fetch = ntohl(u32(src));
			uint32_t hash = ((fetch >> 20) ^ (fetch >> 8)) & 0x0fff;
			o = hashtable[hash];
			hashtable[hash] = src;

			uint32_t offset = (uint32_t) (src - o);
			if (offset <= 131071 && offset > 3 && ((ntohl(u32(o)) ^ ntohl(u32(src))) & 0xffffff00) == 0) {
				if ((u32(o)) != (u32(src))) {
					if (offset <= 127) { /* LZ match */
						*dst++ = offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else if (offset <= 8191) { /* LZ match */
						*dst++ = (uint8_t) 0x80 | offset >> 8;
						*dst++ = (uint8_t) offset;
						cword_val = (cword_val << 1) | 1;
						src += 3;
					} else { /* literal */
						*dst++ = *src++;
						cword_val = (cword_val << 1);
					}
				} else { /* LZ match */
					cword_val = (cword_val << 1) | 1;
					uint32_t len = 0;

					while (*(o + len + 4) == *(src + len + 4) && len < (1 << 11) - 1 && o + len + 4 < src)
						++len;
					src += len + 4;
					if (len <= 7 && offset <= 1023) { /* 10bits offset, 3bits length */
						*dst++ = (uint8_t) 0xa0 | (len << 2)| (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else if (len <= 31 && offset <= 65535) { /* 16bits offset, 5bits length */
						*dst++ = (uint8_t) 0xc0 | len;
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					} else { /* 17bits offset, 11bits length */
						*dst++ = (uint8_t) 0xe0 | (len >> 7);
						*dst++ = (uint8_t) (len << 1) | (offset >> 16);
						*dst++ = (uint8_t) (offset >> 8);
						*dst++ = (uint8_t) offset;
					}
				}
			} else { /* literal */
				*dst++ = *src++;
				cword_val = (cword_val << 1);
			}
		}
	}

	/* save last source bytes as literals */
	while (src <= last_byte) {
		if (--cword_counter == 0) {
			u32(cword_ptr) = htonl((cword_val << 1) | 1);
			cword_counter = 31;
			cword_ptr = dst;
			dst += sizeof(uint32_t);
		}
		*dst++ = *src++;
		cword_val = (cword_val << 1);
	}

	u32(cword_ptr) = htonl((cword_val << cword_counter) | 1);
	return (uint8_t *) dst;
}

uint8_t *qlz_decompress(const uint8_t *source, uint8_t *destination, uint32_t size)
{
	const uint8_t *src = source;
	uint8_t *dst = destination;
	const uint8_t *last_byte = destination + size;
	uint32_t cword_val = ntohl(u32(src));
	uint32_t cword_counter = 0;
	const uint8_t *guaranteed_uncompressed = last_byte - sizeof(uint32_t);

	/* prevent spurious memory read on a source with size < 4 */
	if (dst >= guaranteed_uncompressed) {
		src = src + sizeof(uint32_t);
		while (dst < last_byte) {
			*dst++ = *src++;
		}
		return (uint8_t *) dst;
	}


	for (;;) {
		if (cword_counter == 0) { /* fetch control word */
			cword_val = ntohl(u32(src));
			src += sizeof(uint32_t);
			cword_counter = 31;
		}

		if (cword_val & 1 << 31) { /* LZ match or RLE sequence */
			cword_val = cword_val << 1;
			--cword_counter;
			if ((src[0] & 0x80) == 0) { /* 7bits offset */
				uint32_t offset = src[0];
				u32(dst) = u32(dst - offset);
				dst += 3;
				src += 1;
			} else if ((src[0] & 0x60) == 0) { /* 13bits offset */
				uint32_t offset = ((src[0] & 0x1f) << 8) | src[1];
				u32(dst) = u32(dst - offset);
				dst += 3;
				src += 2;
			} else if ((src[0] & 0x40) == 0) { /* 10bits offset, 3bits length */
				uint32_t len = ((src[0] >> 2) & 7) + 4; 
				uint32_t offset = ((src[0] & 0x03) << 8) | src[1];
				memcpy(dst, dst - offset, len);
				dst += len;
				src += 2;
			} else if ((src[0] & 0x20) == 0) { /* 16bits offset, 5bits length */
				uint32_t len = (src[0] & 0x1f) + 4;
				uint32_t offset = (src[1] << 8) | src[2];
				memcpy(dst, dst - offset, len);
				dst += len;
				src += 3;
			} else if ((src[0] & 0x10) == 0) { /* 17bits offset, 11bits length */
				uint32_t len = (((src[0] & 0x0f) << 7) | (src[1] >> 1)) + 4;
				uint32_t offset = ((src[1] & 0x01) << 16) | (src[2] << 8) | (src[3]);
				memcpy(dst, dst - offset, len);
				dst += len;
				src += 4;
			} else { /* RLE sequence */
				uint32_t len = ((src[0] & 0x0f) << 8) | src[1];
				uint32_t val = src[2] | (src[2] << 8) | (src[2] << 16) | (src[2] << 24);
				
				u32(dst) = val;
				dst += sizeof(uint32_t);
				
				uint8_t *end = dst + len * sizeof(uint32_t);
				while (dst < end) {
					u32(dst) = val;
					dst += sizeof(uint32_t);
				}
				src += 3;
			}
		} else { /* literal */
			const uint32_t map[16] = { 4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
			u32(dst) = u32(src);
			dst += map[cword_val >> 28];
			src += map[cword_val >> 28];
			cword_counter -= map[cword_val >> 28];
			cword_val = cword_val << (map[cword_val >> 28]);

			if (dst >= guaranteed_uncompressed) {
				/* decode last literals and exit */
				while (dst < last_byte) {
					if (cword_counter == 0) {
						src += sizeof(uint32_t);
						cword_counter = 31;
					}
					*dst++ = *src++;
					--cword_counter;
				}
				return (uint8_t *) dst;
			}
		}
	}
}
