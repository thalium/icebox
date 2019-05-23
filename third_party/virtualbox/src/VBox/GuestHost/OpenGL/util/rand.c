/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_rand.h"

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
# ifdef VBOX
#  include <iprt/win/windows.h>
# else
#include <windows.h>
# endif
#else
#include <sys/time.h>
#endif


/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

void crRandSeed(unsigned long seed)
{
	/* setting initial seeds to mt[N] using the generator Line 25 of Table 1
	   in [KNUTH 1981, The Art of Computer Programming Vol. 2 (2nd Ed.),
	   pp102
	*/
	if (seed == 0)
		seed = 4357;   /* pick default seed if seed=0 (Guy Zadickario) */
	mt[0]= seed & 0xffffffff;
	for (mti=1; mti<N; mti++)
		mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}


/*
 * Seed the generator based on time of day or a system counter.
 */
void crRandAutoSeed(void)
{
#if defined(WINDOWS)
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );
	crRandSeed((unsigned long) counter.QuadPart);
#else
	struct timeval timeofday;
	gettimeofday( &timeofday, 0 );	
	crRandSeed((unsigned long) timeofday.tv_usec);
#endif
}


static double genrand( void ) 
{
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;
		if (mti == N+1)   /* if sgenrand() has not been called, */
			crRandSeed(4357); /* a default initial seed is used   */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
		mti = 0;
	}

	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return ( (double)y / (unsigned long)0xffffffff ); /* reals */
	/* return y; */ /* for integer generation */
}

float crRandFloat(float min, float max) 
{
	double t = genrand();
	return (float) (t*min + (1-t)*max);
}

/* return a random integer in [min, max] (inclusive). */
int crRandInt(int min, int max)
{
	double t = genrand();
	return min + (int) (t * (max - min + 1));
}
