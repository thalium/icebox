
#ifndef WINDOWS
#include <stddef.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#if defined( IRIX ) || defined( IRIX64 )
#include <sys/syssgi.h>
#endif
#include <sys/errno.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
# ifdef VBOX
#  include <iprt/win/windows.h>
# else
#include <windows.h>
# endif
#endif

#include "cr_timer.h"
#include "cr_mem.h"
#include "cr_error.h"

static double crTimerGetTime( CRTimer *t )
{
#if defined( IRIX ) || defined( IRIX64 )
	if (t->iotimer_addr64) {
		volatile iotimer64_t counter_value;
		counter_value = *(t->iotimer_addr64);
		return ((double) counter_value * .000000000001) * (double) t->cycleval;
	}
	else {
		volatile iotimer32_t counter_value;
		counter_value = *(t->iotimer_addr32);
		return ((double) counter_value * .000000000001) * (double) t->cycleval;
	}
#elif defined( WINDOWS )
	QueryPerformanceCounter( &t->performance_counter );
	return ((double) t->performance_counter.QuadPart)*t->one_over_frequency;
#elif defined( Linux ) || defined( FreeBSD ) || defined(DARWIN) || defined(AIX) || defined(SunOS) || defined(OSF1)
	gettimeofday( &t->timeofday, NULL );	
	return t->timeofday.tv_sec + t->timeofday.tv_usec / 1000000.0;
#else
#error TIMERS
#endif
}

CRTimer *crTimerNewTimer( void )
{
	CRTimer *t = (CRTimer *) crAlloc( sizeof(*t) );
#if defined(IRIX) || defined( IRIX64 )
	__psunsigned_t phys_addr, raddr;
	int poffmask = getpagesize() - 1;
	int counterSize = syssgi(SGI_CYCLECNTR_SIZE);

	phys_addr = syssgi(SGI_QUERY_CYCLECNTR, &(t->cycleval));
	if (phys_addr == ENODEV) {
		crError( "Sorry, this SGI doesn't support timers." );
	}

	raddr = phys_addr & ~poffmask;
	t->fd = open("/dev/mmem", O_RDONLY);

	if (counterSize == 64) {
		t->iotimer_addr64 =
			(volatile iotimer64_t *)mmap(0, poffmask, PROT_READ,
																	 MAP_PRIVATE, t->fd, (off_t)raddr);
		t->unmapLocation = (void *)t->iotimer_addr64;
		t->unmapSize = poffmask;
		t->iotimer_addr64 = (iotimer64_t *)((__psunsigned_t)t->iotimer_addr64 +
				(phys_addr & poffmask));
	}
	else if (counterSize == 32) {
		t->iotimer_addr32 = (volatile iotimer32_t *)mmap(0, poffmask, PROT_READ,
				MAP_PRIVATE, t->fd,
				(off_t)raddr);
		t->unmapLocation = (void *)t->iotimer_addr32;
		t->unmapSize = poffmask;
		t->iotimer_addr32 = (iotimer32_t *)((__psunsigned_t)t->iotimer_addr32 +
				(phys_addr & poffmask));
	}
	else {
		crError( "Fatal timer init error" );
	}
#elif defined( WINDOWS )
	QueryPerformanceFrequency( &t->performance_frequency );
	t->one_over_frequency = 1.0/((double)t->performance_frequency.QuadPart);
#endif
	t->time0 = t->elapsed = 0;
	t->running = 0;
	return t;
}

void crDestroyTimer( CRTimer *t )
{
#if defined( IRIX ) || defined( IRIX64 )
	close( t->fd );
#endif
	crFree( t );
}

void crStartTimer( CRTimer *t )
{
    t->running = 1;
    t->time0 = crTimerGetTime( t );
}

void crStopTimer( CRTimer *t )
{
    t->running = 0;
    t->elapsed += crTimerGetTime( t ) - t->time0;
}

void crResetTimer( CRTimer *t )
{
    t->running = 0;
    t->elapsed = 0;
}

double crTimerTime( CRTimer *t )
{
	if (t->running) {
		crStopTimer( t );
		crStartTimer( t );
	}
	return t->elapsed;
}
