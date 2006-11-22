
#include <seom/seom.h>

uint8_t *qlz_compress(const char *source, char *destination, uint32_t size);
uint8_t *qlz_decompress(const char *source, char *destination, uint32_t size);

uint32_t *seomCodecEncode(uint32_t *dst, uint8_t *src, uint32_t width, uint32_t height)
{
	uint8_t *end = qlz_compress(src, dst, width * height * 3 / 2);
	return (uint32_t *) end;
}

uint32_t *seomCodecDecode(uint8_t *dst, uint32_t *src, uint32_t width, uint32_t height)
{
	uint8_t *end =  qlz_decompress(src, dst, width * height * 3 / 2);
	return (uint32_t *) end;
}


