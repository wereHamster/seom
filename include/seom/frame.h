
#ifndef __SEOM_FRAME_H__
#define __SEOM_FRAME_H__

#include <seom/seom.h>

typedef struct seomFrame {
	uint64_t pts;
	uint8_t data[];
} seomFrame;

seomFrame *seomFrameCreate(uint8_t type, uint32_t size[2]);
void seomFrameResample(seomFrame *frame, uint32_t size[2]);
void seomFrameConvert(seomFrame *dst, seomFrame *src, uint32_t size[2]);
void seomFrameDestroy(seomFrame *frame);

#endif /* __SEOM_FRAME_H__ */
