
#include <seom/seom.h>

void __seomFrameResample(uint32_t *buf, uint32_t w, uint32_t h);
void __seomFrameConvert(uint8_t *dst[3], uint32_t *src, uint32_t w, uint32_t h);

seomFrame *seomFrameCreate(uint8_t type, uint32_t width, uint32_t height)
{
	seomFrame *frame;
	
	if (type == 'r') {
		frame = malloc(sizeof(seomFrame) + width * height * 4);
	} else if (type == 'c') {
		frame = malloc(sizeof(seomFrame) + width * height * 3 / 2);
	} else {
		fprintf(stderr, "Unknown type: %c\n", type);
		return NULL;
	}
	
	if (__builtin_expect(frame == NULL, 0))
		return NULL;
	
	frame->type = type;
	
	return frame;
}

void seomFrameResample(seomFrame *frame, uint32_t width, uint32_t height)
{
	__seomFrameResample((uint32_t *) &frame->data[0], width, height);
}

void seomFrameConvert(seomFrame *dst, seomFrame *src, uint32_t width, uint32_t height)
{
	uint8_t *yuv[3];
	yuv[0] = (uint8_t *) &dst->data[0];
	yuv[1] = yuv[0] + width * height;
	yuv[2] = yuv[1] + width * height / 4;
	
	dst->pts = src->pts;
	
	__seomFrameConvert(yuv, (uint32_t *) &src->data[0], width, height);
}

void seomFrameDestroy(seomFrame *frame)
{
	free(frame);
}
