
#include <seom/seom.h>

static uint8_t median(uint8_t v1, uint8_t v2, uint8_t v3)
{
	return (v1 + v2 + v3) / 3;
}

void __seomCodecPredict(uint8_t *plane, uint32_t width, uint32_t height)
{
#define pixel(x,y) ( plane[(y)*width+(x)] )	
	for (uint32_t y = height - 1; y > 0; --y) {
		for (uint32_t x = width - 1; x > 0; --x) {
			plane[y * width + x] -= median(pixel(x-1,y), pixel(x-1,y-1), pixel(x,y-1));
		}
		plane[y * width] -= median(0, 0, pixel(0,y-1));
	}
	
	for (uint32_t x = width - 1; x > 0; --x) {
		plane[x] -= median(pixel(x-1,0), 0, 0);
	}
#undef pixel
}

uint32_t *__seomCodecEncrypt(uint32_t *dst, uint8_t *src, uint32_t length)
{
	uint8_t bits = 0;
	uint8_t *end = src + length;
	uint32_t word = 0;
	
	while (src < end) {
		uint8_t sym = *src++;
		uint32_t code = seomCodecTable.codes[sym];
		uint8_t len = code & 0xff;
		if (bits + len >= 32) {
			uint8_t rem = 32 - bits;
			word = word << rem | code >> (32 - rem);
			*dst++ = htonl(word);
			bits = len - rem;
			word = code >> (32 - len);
		} else {
			word = word << len | code >> (32 - len);
			bits += len;
		}
	}

	if (bits > 0) {
		word = word << (32 - bits);
		*dst++ = htonl(word);
	}
	
	return dst;
}

uint32_t *__seomCodecDecrypt(uint8_t *dst, uint32_t *src, uint32_t length)
{
	uint8_t bit = 0;
	uint8_t *end = dst + length;
	
	for (;;) {
		uint32_t val = bit == 0 ? ntohl(src[0]) : ntohl(src[0]) << bit | ntohl(src[1]) >> (32 - bit);

		int firstbit = 0;
		val |= 1;
		for (int i = 31; i >= 0; --i) {
			if (val & (1 << i)) {
				firstbit = i;
				break;
			}
		}

		val -= (1 << firstbit);

		uint8_t *table = seomCodecTable.pointers[firstbit];

		val >>= table[0];

		uint8_t sym = table[1 + val];
		
		*dst++ = sym;
		bit += seomCodecTable.codes[sym] & 0xff;
		if (bit >= 32) {
			bit -= 32;
			++src;
		}
		
		if (dst == end) {
			break;
		}
	}
	
	if (bit) {
		++src;
	}
	
	return src;
}

void __seomCodecRestore(uint8_t *plane, uint32_t width, uint32_t height)
{
#define pixel(x,y) ( plane[(y)*width+(x)] )
	for (uint32_t x = 1; x < width; ++x) {
		plane[x] += median(pixel(x-1,0), 0, 0);
	}

	for (uint32_t y = 1; y < height; ++y) {
		plane[y * width] += median(0, 0, pixel(0,y-1));
		for (uint32_t x = 1; x < width; ++x) {
			plane[y * width + x] += median(pixel(x-1,y), pixel(x-1,y-1), pixel(x,y-1));
		}
	}
#undef src
}
