#ifndef CR_ENDIAN_H
#define CR_ENDIAN_H

#define CR_LITTLE_ENDIAN 0
#define CR_BIG_ENDIAN 1

#include <iprt/cdefs.h>

#ifdef __cplusplus 
extern "C" {
#endif

extern DECLEXPORT(char) crDetermineEndianness( void );

#ifdef WINDOWS
typedef __int64 CR64BitType;
#else
#ifndef __STDC__
typedef long long CR64BitType;
#else
typedef struct  _CR64BitType
{
    unsigned int  lo;
    unsigned int  hi;

} CR64BitType;
#endif
#endif

#define SWAP8(x) x
#define SWAP16(x) (short) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#define SWAP32(x) ((((x) & 0x000000FF) << 24) | \
		   (((x) & 0x0000FF00) << 8)  | \
		   (((x) & 0x00FF0000) >> 8)  | \
		   (((x) & 0xFF000000) >> 24))
#ifdef WINDOWS
#define SWAP64(x) \
	 ((((x) & 0xFF00000000000000L) >> 56) | \
		(((x) & 0x00FF000000000000L) >> 40) | \
		(((x) & 0x0000FF0000000000L) >> 24) | \
		(((x) & 0x000000FF00000000L) >> 8)  | \
		(((x) & 0x00000000FF000000L) << 8)  | \
		(((x) & 0x0000000000FF0000L) << 24) | \
		(((x) & 0x000000000000FF00L) << 40) | \
		(((x) & 0x00000000000000FFL) << 56))
#else
#ifndef __STDC__
#define SWAP64(x) \
	  x = ((((x) & 0xFF00000000000000LL) >> 56) | \
		(((x) & 0x00FF000000000000LL) >> 40) | \
		(((x) & 0x0000FF0000000000LL) >> 24) | \
		(((x) & 0x000000FF00000000LL) >> 8)  | \
		(((x) & 0x00000000FF000000LL) << 8)  | \
		(((x) & 0x0000000000FF0000LL) << 24) | \
		(((x) & 0x000000000000FF00LL) << 40) | \
		(((x) & 0x00000000000000FFLL) << 56))
#else
#define SWAP64(x) \
   {                                            \
      GLubyte *bytes = (GLubyte *) &(x);    \
      GLubyte tmp = bytes[0];                   \
      bytes[0] = bytes[7];                      \
      bytes[7] = tmp;                           \
      tmp = bytes[1];                           \
      bytes[1] = bytes[6];                      \
      bytes[6] = tmp;                           \
      tmp = bytes[2];                           \
      bytes[2] = bytes[5];                      \
      bytes[5] = tmp;                           \
      tmp = bytes[4];                           \
      bytes[4] = bytes[3];                      \
      bytes[3] = tmp;                           \
   }
#endif
#endif

DECLEXPORT(double) SWAPDOUBLE( double d );

#define SWAPFLOAT(x) ( ((*((int *) &(x)) & 0x000000FF) << 24) | \
                       ((*((int *) &(x)) & 0x0000FF00) << 8) | \
                       ((*((int *) &(x)) & 0x00FF0000) >> 8) | \
                       ((*((int *) &(x)) & 0xFF000000) >> 24))


#ifdef __cplusplus
} /* extern "C" */
#endif
 

#endif /* CR_ENDIAN_H */
