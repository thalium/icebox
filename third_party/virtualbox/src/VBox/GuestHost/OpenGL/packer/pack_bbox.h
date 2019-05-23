/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_PACK_BBOX_H
#define CR_PACK_BBOX_H

#include "cr_pack.h"

#define UPDATE_1D_BBOX() \
if (pc->bounds_min.x > fx)    pc->bounds_min.x = fx;\
if (pc->bounds_min.y > 0.0f)  pc->bounds_min.y = 0.0f;\
if (pc->bounds_min.z > 0.0f)  pc->bounds_min.z = 0.0f;\
if (pc->bounds_max.x < fx)    pc->bounds_max.x = fx;\
if (pc->bounds_max.y < 0.0f)  pc->bounds_max.y = 0.0f;\
if (pc->bounds_max.z < 0.0f)  pc->bounds_max.z = 0.0f

#define UPDATE_2D_BBOX() \
if (pc->bounds_min.x > fx)    pc->bounds_min.x = fx;\
if (pc->bounds_min.y > fy)    pc->bounds_min.y = fy;\
if (pc->bounds_min.z > 0.0f)  pc->bounds_min.z = 0.0f;\
if (pc->bounds_max.x < fx)    pc->bounds_max.x = fx;\
if (pc->bounds_max.y < fy)    pc->bounds_max.y = fy;\
if (pc->bounds_max.z < 0.0f)  pc->bounds_max.z = 0.0f

#define UPDATE_3D_BBOX() \
if (pc->bounds_min.x > fx)    pc->bounds_min.x = fx; \
if (pc->bounds_min.y > fy)    pc->bounds_min.y = fy; \
if (pc->bounds_min.z > fz)    pc->bounds_min.z = fz; \
if (pc->bounds_max.x < fx)    pc->bounds_max.x = fx; \
if (pc->bounds_max.y < fy)    pc->bounds_max.y = fy; \
if (pc->bounds_max.z < fz)    pc->bounds_max.z = fz

#ifdef SIMD
#define UPDATE_3D_BBOX_SIMD() \
__asm {\
	__asm movups xmm0, fx\
	__asm movaps xmm1, pc->bounds_min.x\
	__asm movaps xmm2, pc->bounds_max.x\
	__asm minps xmm1, xmm0\
	__asm maxps xmm2, xmm0\
	__asm movaps pc->bounds_min.x, xmm1\
	__asm movaps pc->bounds_max.x, xmm2\
}
#define UPDATE_3D_BBOX_SIMD_PACK() \
__asm {\
	__asm mov ecx, [data_ptr]\
	__asm movups xmm0, fx\
	__asm movaps xmm1, pc->bounds_min.x\
	__asm movaps xmm2, pc->bounds_max.x\
	__asm minps xmm1, xmm0\
	__asm maxps xmm2, xmm0\
	__asm movaps pc->bounds_min.x, xmm1\
	__asm movaps pc->bounds_max.x, xmm2\
	__asm movups [ecx], xmm0\
}
#define UPDATE_3DV_BBOX_SIMD() \
__asm {\
	__asm mov eax, [v]\
	__asm mov ecx, [data_ptr]\
	__asm movups xmm0, [eax]\
	__asm movaps xmm1, pc->bounds_min.x\
	__asm movaps xmm2, pc->bounds_max.x\
	__asm minps xmm1, xmm0\
	__asm maxps xmm2, xmm0\
	__asm movaps pc->bounds_min.x, xmm1\
	__asm movaps pc->bounds_max.x, xmm2\
	__asm movups [ecx], xmm0\
}
#else
#define UPDATE_3DV_BBOX_SIMD() { CREATE_3D_VFLOATS(); UPDATE_3D_BBOX();}
#define UPDATE_3D_BBOX_SIMD()  UPDATE_3D_BBOX()
#endif

#define CREATE_1D_FLOATS() \
	GLfloat fx = (GLfloat) x;

#define CREATE_2D_FLOATS() \
	GLfloat fx = (GLfloat) x; \
	GLfloat fy = (GLfloat) y

#define CREATE_3D_FLOATS() \
	GLfloat fx = (GLfloat) x; \
	GLfloat fy = (GLfloat) y; \
	GLfloat fz = (GLfloat) z

#define CREATE_4D_FLOATS() \
	GLfloat fx = (GLfloat) x; \
	GLfloat fy = (GLfloat) y; \
	GLfloat fz = (GLfloat) z; \
	GLfloat fw = (GLfloat) w; \
	fx /= fw; fy /= fw; fz/= fw

/* For glVertexAttrib4N*ARB */
#define CREATE_3D_FLOATS_B_NORMALIZED() \
	GLfloat fx = (GLfloat) x * (1.0 / 128.0); \
	GLfloat fy = (GLfloat) y * (1.0 / 128.0); \
	GLfloat fz = (GLfloat) z * (1.0 / 128.0);

#define CREATE_3D_FLOATS_UB_NORMALIZED() \
	GLfloat fx = (GLfloat) x * (1.0 / 255.0); \
	GLfloat fy = (GLfloat) y * (1.0 / 255.0); \
	GLfloat fz = (GLfloat) z * (1.0 / 255.0);

#define CREATE_3D_FLOATS_US_NORMALIZED() \
	GLfloat fx = (GLfloat) x * (1.0 / 65535.0); \
	GLfloat fy = (GLfloat) y * (1.0 / 65535.0); \
	GLfloat fz = (GLfloat) z * (1.0 / 65535.0);


#define CREATE_1D_VFLOATS() \
	GLfloat fx = (GLfloat) v[0];

#define CREATE_2D_VFLOATS() \
	GLfloat fx = (GLfloat) v[0]; \
	GLfloat fy = (GLfloat) v[1]

#define CREATE_3D_VFLOATS() \
	GLfloat fx = (GLfloat) v[0]; \
	GLfloat fy = (GLfloat) v[1]; \
	GLfloat fz = (GLfloat) v[2]

#define CREATE_4D_VFLOATS() \
	GLfloat fx = (GLfloat) v[0]; \
	GLfloat fy = (GLfloat) v[1]; \
	GLfloat fz = (GLfloat) v[2]; \
	GLfloat fw = (GLfloat) v[3]; \
	fx /= fw; fy /= fw; fz/= fw

/* For glVertexAttrib4N*ARB */
#define CREATE_4D_FLOATS_UB_NORMALIZED() \
	GLfloat fx = (GLfloat) (x * (1.0 / 255.0)); \
	GLfloat fy = (GLfloat) (y * (1.0 / 255.0)); \
	GLfloat fz = (GLfloat) (z * (1.0 / 255.0)); \
	GLfloat fw = (GLfloat) (w * (1.0 / 255.0)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_B_NORMALIZED() \
	GLfloat fx = (GLfloat) (v[0] * (1.0 / 128.0)); \
	GLfloat fy = (GLfloat) (v[1] * (1.0 / 128.0)); \
	GLfloat fz = (GLfloat) (v[2] * (1.0 / 128.0)); \
	GLfloat fw = (GLfloat) (v[3] * (1.0 / 128.0)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_S_NORMALIZED() \
	GLfloat fx = (GLfloat) (2.0 * v[0] + 1.0) / ((GLfloat) (0xffff)); \
	GLfloat fy = (GLfloat) (2.0 * v[1] + 1.0) / ((GLfloat) (0xffff)); \
	GLfloat fz = (GLfloat) (2.0 * v[2] + 1.0) / ((GLfloat) (0xffff)); \
	GLfloat fw = (GLfloat) (2.0 * v[3] + 1.0) / ((GLfloat) (0xffff)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_I_NORMALIZED() \
	GLfloat fx = (GLfloat) (2.0 * v[0] + 1.0) / ((GLfloat) (0xffffffff)); \
	GLfloat fy = (GLfloat) (2.0 * v[1] + 1.0) / ((GLfloat) (0xffffffff)); \
	GLfloat fz = (GLfloat) (2.0 * v[2] + 1.0) / ((GLfloat) (0xffffffff)); \
	GLfloat fw = (GLfloat) (2.0 * v[3] + 1.0) / ((GLfloat) (0xffffffff)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_UB_NORMALIZED() \
	GLfloat fx = (GLfloat) (v[0] * (1.0 / 255.0)); \
	GLfloat fy = (GLfloat) (v[1] * (1.0 / 255.0)); \
	GLfloat fz = (GLfloat) (v[2] * (1.0 / 255.0)); \
	GLfloat fw = (GLfloat) (v[3] * (1.0 / 255.0)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_US_NORMALIZED() \
	GLfloat fx = (GLfloat) (v[0] * (1.0 / 65535.0)); \
	GLfloat fy = (GLfloat) (v[1] * (1.0 / 65535.0)); \
	GLfloat fz = (GLfloat) (v[2] * (1.0 / 65535.0)); \
	GLfloat fw = (GLfloat) (v[3] * (1.0 / 65535.0)); \
	fx /= fw; fy /= fw; fz/= fw

#define CREATE_4D_VFLOATS_UI_NORMALIZED() \
	GLfloat fx = v[0] / ((GLfloat) (0xffffffff)); \
	GLfloat fy = v[1] / ((GLfloat) (0xffffffff)); \
	GLfloat fz = v[2] / ((GLfloat) (0xffffffff)); \
	GLfloat fw = v[3] / ((GLfloat) (0xffffffff)); \
	fx /= fw; fy /= fw; fz/= fw

#endif /* CR_PACK_BBOX_H */

