
#include <seom/seom.h>

/*
 * Y  = 16  +  46.5594 * R + 156.6288 * G +  15.1881 * B
 * Cb = 128 -  25.6642 * R -  86.3358 * G + 112.0000 * B
 * Cr = 128 + 112.0000 * R - 101.7303 * G -  10.2697 * B
 */

#define ri 2
#define gi 1
#define bi 0

#define scale 8
#define f(x) ((uint16_t) ((x) * (1L<<scale) + 0.5))

#define byte(ptr) ( *(uint8_t *) (ptr) )

static const uint16_t m[3][3] = {
	{ f(0.0596), f(0.6142), f(0.1826) },
	{ f(0.4392), f(0.3386), f(0.1006) },
	{ f(0.0403), f(0.3989), f(0.4392) }
};

void __seomFrameResample(void *buf, unsigned long w, unsigned long h)
{
	for (unsigned long y = 0; y < h; y += 2) {
		for (unsigned long x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( byte(buf + (y+yo) * (w*4) + (x+xo) * 4 + s) )
			uint8_t p[2][2][3] = {
				{ { c(0,0,0), c(0,0,1), c(0,0,2) }, { c(1,0,0), c(1,0,1), c(1,0,2) } },
				{ { c(0,1,0), c(0,1,1), c(0,1,2) }, { c(1,1,0), c(1,1,1), c(1,1,2) } },
			};
			#undef c
			
			uint8_t r[3] = {
				( p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0] ) / 4,
				( p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1] ) / 4,
				( p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2] ) / 4,
			};
			
			void *tmp = buf + ( y / 2 ) * ( w / 2) * 4 + ( x / 2 ) * 4;
			byte(tmp + 0) = r[0];
			byte(tmp + 1) = r[1];
			byte(tmp + 2) = r[2];
		}
	}
}

void __seomFrameConvert(void *dst[3], void *src, unsigned long w, unsigned long h)
{
	for (unsigned long y = 0; y < h; y += 2) {
		for (unsigned long x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( byte(src + (y+yo) * (w*4) + (x+xo) * 4 + s) )
			uint8_t p[2][2][3] = {
				{ { c(0,0,0), c(0,0,1), c(0,0,2) }, { c(0,1,0), c(0,1,1), c(0,1,2) } },
				{ { c(1,0,0), c(1,0,1), c(1,0,2) }, { c(1,1,0), c(1,1,1), c(1,1,2) } },
			};
			#undef c
			
			uint16_t r[3] = {
				p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0],
				p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1],
				p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2],
			};
			
			#define sy(xo,yo) byte(dst[0]+(y+yo)*w+(x+xo)) = (uint8_t) ((m[0][ri] * p[xo][yo][ri] + m[0][gi] * p[xo][yo][gi] + m[0][bi] * p[xo][yo][bi]) >> scale) + 16
			sy(0,0);
			sy(1,0);
			sy(0,1);
			sy(1,1);
			#undef sy
			
			byte(dst[1]+y/2*w/2+x/2) = (uint8_t) ((-m[1][ri] * r[ri] - m[1][gi] * r[gi] + m[1][bi] * r[bi]) >> (scale + 2)) + 128;
			byte(dst[2]+y/2*w/2+x/2) = (uint8_t) ((m[2][ri] * r[ri] - m[2][gi] * r[gi] - m[2][bi] * r[bi]) >> (scale + 2)) + 128;
		}
	}
}
