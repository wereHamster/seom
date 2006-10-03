
#include <seom/seom.h>

struct seomCodecTable seomCodecTable;

static const uint8_t lengths[] = {
	34, 35, 36, 37, 38, 103, 168, 9, 8, 10, 11, 11, 10, 12, 8, 237, 14, 9, 15, 11,
	16, 8, 241, 242, 19, 8, 212, 85, 54, 23, 10, 54, 119, 54, 55, 54, 119, 182, 55, 54,
	53, 54, 117, 244, 51, 52, 147, 178, 177, 16, 8, 15, 9, 14, 10, 13, 9, 12, 9, 11,
	9, 10, 11, 9, 8, 168, 103, 38, 37, 36, 34,
};

#define max(a,b) ( a > b ? a : b);

static uint8_t msb(uint32_t val)
{
	for (int i = 31; i >= 0; i--) {
		if (val & (1 << i)) {
			return i;
		}
	}
	
	return 0xff;
}

static void decompress(uint8_t *dst)
{
	const uint8_t *hufftable = lengths;
	int i = 0;
	do {
		int val = *hufftable & 31;
		int repeat = *hufftable++ >> 5;
		if (!repeat)
			repeat = *hufftable++;

		while (repeat--) {
			dst[i++] = val;
		}
	} while (i < 256);
}


__attribute__((constructor))
static void seomCodecInit()
{
	uint8_t shift[256];
	uint32_t add[256];
	
	decompress(shift);
	int min_already_processed = 32;
	uint32_t bits = 0;
	do {
		int max_not_processed = 0;
		for (int i = 0; i < 256; ++i) {
			if (shift[i] < min_already_processed && shift[i] > max_not_processed)
				max_not_processed = shift[i];
		}
		int bit = 1 << (32 - max_not_processed);
		assert(!(bits & (bit - 1)));
		for (int j = 0; j < 256; ++j) {
			if (shift[j] == max_not_processed) {
				add[j] = bits;
				bits += bit;
			}
		}
		min_already_processed = max_not_processed;
	} while (bits & 0xFFFFFFFF);

	char code_lengths[256];
	char code_firstbits[256];
	char table_lengths[32];
	memset(table_lengths, -1, 32);
	int all_zero_code = -1;
	
	for (int i = 0; i < 256; ++i) {
		seomCodecTable.codes[i] = add[i] + (shift[i] & 0xff);
		
		if (add[i]) {
			int fb = msb(add[i]);
			code_firstbits[i] = fb;
			int length = shift[i] - (32 - fb);
			code_lengths[i] = length;
			table_lengths[fb] = max(table_lengths[fb], length);
		} else {
			all_zero_code = i;
		}
	}
	
	uint8_t *p = seomCodecTable.data;
	*p++ = 31;
	*p++ = all_zero_code;
	for (int j = 0; j < 32; ++j) {
		if (table_lengths[j] == -1) {
			seomCodecTable.pointers[j] = seomCodecTable.data;
		} else {
			seomCodecTable.pointers[j] = p;
			*p++ = j - table_lengths[j];
			p += 1 << table_lengths[j];
		}
	}
	for (int k = 0; k < 256; ++k) {
		if (add[k]) {
			int firstbit = code_firstbits[k];
			int val = add[k] - (1 << firstbit);
			uint8_t *table = seomCodecTable.pointers[firstbit];
			memset(&table[1 + (val >> table[0])], k, 1 << (table_lengths[firstbit] - code_lengths[k]));
		}
	}
}

uint32_t *seomCodecEncodeReference(uint32_t * dst, uint8_t * src, uint8_t * end, struct seomCodecTable *tbl)
{
	int bits = 32;
	int epos = 0;
	uint64_t qword = 0;
	while (src < end) {
		uint8_t sym = *src++;
		uint32_t code = seomCodecTable.codes[sym];
		int len = code & 0xff;
		qword &= 0xffffffff00000000;
		qword |= code;
		if (bits - len <= 0) {
			qword <<= bits;
			dst[epos++] = htonl(qword >> 32);
			len -= bits;
			bits = 32;
		}
		//printf("%d/%d\n", bits, len);
		qword <<= len;
		bits -= len;
		//_p32(byte);
	}

	if (bits > 0) {
		qword <<= bits;
		dst[epos++] = htonl(qword >> 32);
	}
	//printf("%012d - %012d\n", epos*4, size);
	return dst + epos;
}

uint8_t *seomCodecDecodeReference(uint8_t *dst, uint32_t *src, uint8_t *end, struct seomCodecTable *tbl)
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
		//printf("msb: %d\n", firstbit);

		val -= (1 << firstbit);

		uint8_t *table = seomCodecTable.pointers[firstbit];
		//printf("length: %d\n", 32-table[0]);

		val >>= table[0];
		//_p32(val);

		uint8_t sym = table[1 + val];
		//printf("symbol: 0x%02x\n", sym);
		dst[opos++] = sym;
		bpos += seomCodecTable.codes[sym] & 0xff;
		if (dst + opos >= end) {
			break;
		}
	}
}
