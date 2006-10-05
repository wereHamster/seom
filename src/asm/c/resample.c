
#include <seom/seom.h>

void seomResample(uint32_t *buf, uint32_t w, uint32_t h)
{
	for (uint32_t y = 0; y < h; y += 2) {
		for (uint32_t x = 0; x < w; x += 2) {
			#define c(x, y, s) ( buf[(y) * w + (x)] >> ((s) * 8 ) )
			uint8_t p[4][4] = {
				{ c(x  , y  , 0), c(x  , y  , 1), c(x  , y  , 2), c(x  , y  , 3) },
				{ c(x+1, y  , 0), c(x+1, y  , 1), c(x+1, y  , 2), c(x+1, y  , 3) },
				{ c(x  , y+1, 0), c(x  , y+1, 1), c(x  , y+1, 2), c(x  , y+1, 3) },
				{ c(x+1, y+1, 0), c(x+1, y+1, 1), c(x+1, y+1, 2), c(x+1, y+1, 3) }
			};
			#undef c
			
			uint8_t r[4] = {
				( p[0][0] + p[1][0] + p[2][0] + p[3][0] ) / 4,
				( p[0][1] + p[1][1] + p[2][1] + p[3][1] ) / 4,
				( p[0][2] + p[1][2] + p[2][2] + p[3][2] ) / 4,
				( p[0][3] + p[1][3] + p[2][3] + p[3][3] ) / 4,
			};
			
			buf[( y / 2 ) * w + ( x / 2 )] = r[0] | r[1] << 8 | r[2] << 16 | r[3] << 24;
		}
	}
}
