#ifndef CR_TIMER_H
#define CR_TIMER_H

#ifndef WINDOWS
#include <sys/time.h>

#if defined (IRIX) || defined( IRIX64 )
typedef unsigned long long iotimer64_t;
typedef unsigned int iotimer32_t;
#endif
#else
# ifdef VBOX
#  include <iprt/win/windows.h>
# else
#include <windows.h>
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Timer 
{
	double time0, elapsed;
	char running;

	int fd;
#if defined (IRIX) || defined( IRIX64 )
	unsigned long long counter64;
	unsigned int counter32;
	unsigned int cycleval;

	volatile iotimer64_t *iotimer_addr64;
	volatile iotimer32_t *iotimer_addr32;

	void *unmapLocation;
	int unmapSize;
#elif defined(WINDOWS)
	LARGE_INTEGER performance_counter, performance_frequency;
	double one_over_frequency;
#elif defined( Linux ) || defined( FreeBSD ) || defined(DARWIN) || defined(AIX) || defined (SunOS) || defined(OSF1)
	struct timeval timeofday;
#endif
} CRTimer;

CRTimer *crTimerNewTimer( void );
void crDestroyTimer( CRTimer *t );
void crStartTimer( CRTimer *t );
void crStopTimer( CRTimer *t );
void crResetTimer( CRTimer *t );
double crTimerTime( CRTimer *t );

#ifdef __cplusplus
}
#endif

#endif /* CR_TIMER_H */
