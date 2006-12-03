
#include <seom/seom.h>

#define ri 2
#define gi 1
#define bi 0

#define scale 8
#define f(x) ((uint16_t) ((x) * (1L<<scale) + 0.5))

static const uint16_t m[3][3] = {
	{ f(0.098), f(0.504), f(0.257) },
	{ f(0.439), f(0.291), f(0.148) },
	{ f(0.071), f(0.368), f(0.439) }
};

void __seomFrameResample(uint32_t *buf, uint32_t w, uint32_t h)
{
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( buf[(y+yo)*w+(x+xo)]>>((s)*8) )
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
			
			buf[( y / 2 ) * ( w / 2) + ( x / 2 )] = r[0] | r[1] << 8 | r[2] << 16;
		}
	}
}

void __seomFrameConvert(uint8_t *dst[3], uint32_t *src, uint32_t w, uint32_t h)
{
	uint8_t *in8 = (uint8_t *) src;
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( in8[(y+yo)*(w*4)+(x+xo)*4+s] )
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
			
			#define sy(xo,yo) dst[0][(y+yo)*w+(x+xo)] = (uint8_t) ((m[0][ri] * p[xo][yo][ri] + m[0][gi] * p[xo][yo][gi] + m[0][bi] * p[xo][yo][bi]) >> scale) + 16
			sy(0,0);
			sy(1,0);
			sy(0,1);
			sy(1,1);
			#undef sy
			
			dst[1][y/2*w/2+x/2] = (uint8_t) ((-m[1][ri] * r[ri] - m[1][gi] * r[gi] + m[1][bi] * r[bi]) >> (scale + 2)) + 128;
			dst[2][y/2*w/2+x/2] = (uint8_t) ((m[2][ri] * r[ri] - m[2][gi] * r[gi] - m[2][bi] * r[bi]) >> (scale + 2)) + 128;
		}
	}
}
