
#include <seom/seom.h>

typedef void (packedFunc) (uint8_t * x_ptr,
								 int x_stride,
								 uint8_t * y_src,
								 uint8_t * v_src,
								 uint8_t * u_src,
								 int y_stride,
								 int uv_stride,
								 int width,
								 int height,
								 int vflip);

packedFunc rgba_to_yv12_c;


void seomConvert(uint8_t *out[3], uint32_t *in, uint32_t w, uint32_t h)
{
	rgba_to_yv12_c(in, w, out[0], out[2], out[1], w, w/2, w, h, 0);
}


/********** generic colorspace macro **********/


#define MAKE_COLORSPACE(NAME,SIZE,PIXELS,VPIXELS,FUNC,C1,C2,C3,C4) \
void	\
NAME(uint8_t * x_ptr, int x_stride,	\
				 uint8_t * y_ptr, uint8_t * u_ptr, uint8_t * v_ptr,	\
				 int y_stride, int uv_stride,	\
				 int width, int height, int vflip)	\
{	\
	int fixed_width = (width + 1) & ~1;				\
	int x_dif = x_stride - (SIZE)*fixed_width;		\
	int y_dif = y_stride - fixed_width;				\
	int uv_dif = uv_stride - (fixed_width / 2);		\
	int x, y;										\
	if (vflip) {								\
		x_ptr += (height - 1) * x_stride;			\
		x_dif = -(SIZE)*fixed_width - x_stride;		\
		x_stride = -x_stride;						\
	}												\
	for (y = 0; y < height; y+=(VPIXELS)) {			\
		FUNC##_ROW(SIZE,C1,C2,C3,C4);				\
		for (x = 0; x < fixed_width; x+=(PIXELS)) {	\
			FUNC(SIZE,C1,C2,C3,C4);				\
			x_ptr += (PIXELS)*(SIZE);				\
			y_ptr += (PIXELS);						\
			u_ptr += (PIXELS)/2;					\
			v_ptr += (PIXELS)/2;					\
		}											\
		x_ptr += x_dif + (VPIXELS-1)*x_stride;		\
		y_ptr += y_dif + (VPIXELS-1)*y_stride;		\
		u_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;	\
		v_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;	\
	}												\
}



/********** colorspace input (xxx_to_yv12) functions **********/

/*	rgb -> yuv def's

	this following constants are "official spec"
	Video Demystified" (ISBN 1-878707-09-4)

	rgb<->yuv _is_ lossy, since most programs do the conversion differently

	SCALEBITS/FIX taken from  ffmpeg
*/

#define Y_R_IN			0.257
#define Y_G_IN			0.504
#define Y_B_IN			0.098
#define Y_ADD_IN		16

#define U_R_IN			0.148
#define U_G_IN			0.291
#define U_B_IN			0.439
#define U_ADD_IN		128

#define V_R_IN			0.439
#define V_G_IN			0.368
#define V_B_IN			0.071
#define V_ADD_IN		128

#define SCALEBITS_IN	8
#define FIX_IN(x)		((uint16_t) ((x) * (1L<<SCALEBITS_IN) + 0.5))


/* rgb/rgbi input */

#define READ_RGB_Y(SIZE, ROW, UVID, C1,C2,C3,C4)	\
	r##UVID += r = x_ptr[(ROW)*x_stride+(C1)];						\
	g##UVID += g = x_ptr[(ROW)*x_stride+(C2)];						\
	b##UVID += b = x_ptr[(ROW)*x_stride+(C3)];						\
	y_ptr[(ROW)*y_stride+0] =									\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +	\
					FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;	\
	r##UVID += r = x_ptr[(ROW)*x_stride+(SIZE)+(C1)];				\
	g##UVID += g = x_ptr[(ROW)*x_stride+(SIZE)+(C2)];				\
	b##UVID += b = x_ptr[(ROW)*x_stride+(SIZE)+(C3)];				\
	y_ptr[(ROW)*y_stride+1] =									\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +	\
					FIX_IN(Y_B_IN) * b) >> SCALEBITS_IN) + Y_ADD_IN;

#define READ_RGB_UV(UV_ROW,UVID)	\
	u_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((-FIX_IN(U_R_IN) * r##UVID - FIX_IN(U_G_IN) * g##UVID +			\
					FIX_IN(U_B_IN) * b##UVID) >> (SCALEBITS_IN + 2)) + U_ADD_IN;	\
	v_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((FIX_IN(V_R_IN) * r##UVID - FIX_IN(V_G_IN) * g##UVID -			\
					FIX_IN(V_B_IN) * b##UVID) >> (SCALEBITS_IN + 2)) + V_ADD_IN;


#define RGB_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGB_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t r, g, b, r0, g0, b0;		\
	r0 = g0 = b0 = 0;					\
	READ_RGB_Y(SIZE, 0, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 1, 0, C1,C2,C3,C4)	\
	READ_RGB_UV(     0, 0)

#define RGBI_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGBI_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t r, g, b, r0, g0, b0, r1, g1, b1;	\
	r0 = g0 = b0 = r1 = g1 = b1 = 0;	\
	READ_RGB_Y(SIZE, 0, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 1, 1, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 2, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 3, 1, C1,C2,C3,C4)	\
	READ_RGB_UV(     0, 0)				\
	READ_RGB_UV(     1, 1)



MAKE_COLORSPACE(rgba_to_yv12_c,    4,2,2, RGB_TO_YV12,    0,1,2, 0)

