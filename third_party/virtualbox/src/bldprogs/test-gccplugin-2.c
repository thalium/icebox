/* Only valid stuff in this one. */
extern void MyIprtPrintf(const char *pszFormat, ...) __attribute__((__iprt_format__(1,2)));
extern void foo(void);

extern unsigned long long g_ull;

typedef unsigned long long RTGCPHYS;
extern RTGCPHYS g_GCPhys;

typedef RTGCPHYS *PRTGCPHYS;
extern PRTGCPHYS  g_pGCPhys;

#include <stdarg.h>

void foo(const char *pszFormat, ...)
{
    MyIprtPrintf("%s %RX64 %RGp %p %RGp", "foobar", g_ull, g_GCPhys, g_pGCPhys, *g_pGCPhys);
    MyIprtPrintf("%RX32 %d %s\n", 10, 42, "string");
    MyIprtPrintf("%u %.8Rhxs %d\n", 10, &g_ull, 42);
    MyIprtPrintf("%u %.8RhxD %d\n", 10, &g_ull, 42);
    {
        va_list va;
        int iValue;
        va_start(va, pszFormat);
        iValue = va_arg(va, int);
        va_end(va);
        MyIprtPrintf("%u\n", iValue);
    }
}

