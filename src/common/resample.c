
#include <seom/seom.h>

void seomResample(uint32_t *buf, uint32_t w, uint32_t h)
{
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			#define c(xo,yo,s) ( buf[(y+yo)*w+(x+xo)]>>((s)*8) )
			uint8_t p[2][2][4] = {
				{ { c(0,0,0), c(0,0,1), c(0,0,2), c(0,0,3) }, { c(1,0,0), c(1,0,1), c(1,0,2), c(1,0,3) } },
				{ { c(0,1,0), c(0,1,1), c(0,1,2), c(0,0,3) }, { c(1,1,0), c(1,1,1), c(1,1,2), c(1,1,3) } },
			};
			#undef c
			
			uint8_t r[4] = {
				( p[0][0][0] + p[1][0][0] + p[0][1][0] + p[1][1][0] ) / 4,
				( p[0][0][1] + p[1][0][1] + p[0][1][1] + p[1][1][1] ) / 4,
				( p[0][0][2] + p[1][0][2] + p[0][1][2] + p[1][1][2] ) / 4,
				( p[0][0][3] + p[1][0][3] + p[0][1][3] + p[1][1][3] ) / 4,
			};
			
			buf[( y / 2 ) * w + ( x / 2 )] = r[0] | r[1] << 8 | r[2] << 16 | r[3] << 24;
		}
	}
}
