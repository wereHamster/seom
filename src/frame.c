
#include <seom/seom.h>

void __seomFrameResample(void *buf, unsigned long w, unsigned long h);
void __seomFrameConvert(void *dst[3], void *src, unsigned long w, unsigned long h);

seomFrame *seomFrameCreate(uint8_t type, uint32_t size[2])
{
	seomFrame *frame = NULL;
	
	if (type == 'r') {
		frame = malloc(sizeof(seomFrame) + size[0] * size[1] * 4);
	} else if (type == 'c') {
		frame = malloc(sizeof(seomFrame) + size[0] * size[1] * 3 / 2);
	}
	
	return frame;
}

void seomFrameResample(seomFrame *frame, uint32_t size[2])
{
	__seomFrameResample((void *) &frame->data[0], size[0], size[1]);
}

void seomFrameConvert(seomFrame *dst, seomFrame *src, uint32_t size[2])
{
	void *yuv[3];
	yuv[0] = (void *) &dst->data[0];
	yuv[1] = yuv[0] + size[0] * size[1];
	yuv[2] = yuv[1] + size[0] * size[1] / 4;
	
	dst->pts = src->pts;
	
	__seomFrameConvert(yuv, (void *) &src->data[0], size[0], size[1]);	
}

void seomFrameDestroy(seomFrame *frame)
{
	free(frame);
}
