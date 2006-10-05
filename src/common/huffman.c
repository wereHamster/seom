
#include <seom/seom.h>

uint32_t *seomCodecEncode(uint32_t * dst, uint8_t * src, uint8_t * end, struct seomCodecTable *tbl)
{
	int bits = 32;
	int epos = 0;
	uint64_t qword = 0;
	while (src < end) {
		uint8_t sym = *src++;
		uint32_t code = tbl->codes[sym];
		int len = code & 0xff;
		qword &= 0xffffffff00000000;
		qword |= code;
		if (bits - len <= 0) {
			qword <<= bits;
			dst[epos++] = htonl(qword >> 32);
			len -= bits;
			bits = 32;
		}
		qword <<= len;
		bits -= len;
	}

	if (bits > 0) {
		qword <<= bits;
		dst[epos++] = htonl(qword >> 32);
	}
	return dst + epos;
}

uint8_t *seomCodecDecode(uint8_t *dst, uint32_t *src, uint8_t *end, struct seomCodecTable *tbl)
{
	int bpos = 0;
	int opos = 0;
	for (;;) {
		int vpos = bpos % 32;
		uint32_t val = ntohl(src[bpos / 32]);
		uint32_t val2 = ntohl(src[bpos / 32 + 1]);
		if (vpos) {
			val <<= vpos;
			val += val2 >> (32 - vpos);
		}

		int firstbit = 0;
		val |= 1;
		for (int i = 31; i >= 0; --i) {
			if (val & (1 << i)) {
				firstbit = i;
				break;
			}
		}

		val -= (1 << firstbit);

		uint8_t *table = tbl->pointers[firstbit];

		val >>= table[0];

		uint8_t sym = table[1 + val];
		dst[opos++] = sym;
		bpos += tbl->codes[sym] & 0xff;
		if (dst + opos >= end) {
			break;
		}
	}
	
	return end;
}
