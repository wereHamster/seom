
#include <seom/seom.h>

#define ri 0
#define gi 1
#define bi 2

#define scale 8
#define f(x) ((uint16_t) ((x) * (1L<<scale) + 0.5))

static const uint16_t m[3][4] = {
	{ f(0.257), f(0.504), f(0.098),  16 },
	{ f(0.148), f(0.291), f(0.439), 128 },
	{ f(0.439), f(0.368), f(0.071), 128 }
};

void seomConvert(uint8_t *out[3], uint32_t *in, uint32_t w, uint32_t h)
{
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( in[(y+yo)*w+(x+xo)]>>((s)*8) )
			uint8_t p[2][2][4] = {
				{ { c(0,0,0), c(0,0,1), c(0,0,2), c(0,0,3) }, { c(1,0,0), c(1,0,1), c(1,0,2), c(1,0,3) } },
				{ { c(0,1,0), c(0,1,1), c(0,1,2), c(0,0,3) }, { c(1,1,0), c(1,1,1), c(1,1,2), c(1,1,3) } },
			};
			#undef c
			
			uint16_t r[4] = {
				p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0],
				p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1],
				p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2],
				p[0][0][3] + p[1][0][3] + p[0][1][3] + p[1][1][3],
			};
			
			#define sy(xo,yo) out[0][(y+xo)*w+(x+xo)] = (uint8_t) ((m[0][ri] * p[xo][yo][ri] + m[0][gi] * p[xo][yo][gi] + m[0][bi] * p[xo][yo][bi]) >> scale) + m[0][4]
			sy(0, 0);
			sy(1, 0);
			sy(0, 1);
			sy(1, 1);
			#undef sy
			
			out[1][y/2*w/2+x/2] = (uint8_t) ((m[1][ri] * r[ri] + m[1][gi] * r[gi] + m[1][bi] * r[bi]) >> scale) + m[1][4];
			out[2][y/2*w/2+x/2] = (uint8_t) ((m[2][ri] * r[ri] + m[2][gi] * r[gi] + m[2][bi] * r[bi]) >> scale) + m[2][4];
		}
	}
}
