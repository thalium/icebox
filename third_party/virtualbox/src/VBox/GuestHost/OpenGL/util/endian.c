#include "chromium.h"
#include "cr_endian.h"

char crDetermineEndianness( void )
{
	union {
		struct {
			char c1;
			char c2;
			char c3;
			char c4;
		} c;
		unsigned int i;
	} e_test;

	e_test.c.c1 = 1;
	e_test.c.c2 = 2;
	e_test.c.c3 = 3;
	e_test.c.c4 = 4;

	if (e_test.i == 0x01020304)
	{
		return CR_BIG_ENDIAN;
	}
	return CR_LITTLE_ENDIAN;
}  

double SWAPDOUBLE( double d )
{
	CR64BitType *ptr = (CR64BitType *) (&d);
#ifdef __STDC__
	CR64BitType swapped;
	SWAP64( *ptr );
	swapped = *ptr;
#else
	CR64BitType swapped = SWAP64( *ptr );
#endif
	return *((double *) (&swapped));
}
