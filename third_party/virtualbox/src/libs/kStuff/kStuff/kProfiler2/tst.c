#include <stdio.h>

#ifdef _MSC_VER
void __cdecl _penter(void);
void __cdecl _pexit(void);
__declspec(naked) int  naked(void)
{
    __asm
    {
        call    _penter
        call    _pexit
        xor     eax, eax
        ret
    }
}

#endif

int bar(void)
{
    unsigned i;
    for (i = 0; i < 1000; i += 7)
        i += i & 1;
    return i;
}

int foo(void)
{
    unsigned i, rc = 0;
    for (i = 0; i < 1000; i++)
        rc += bar();
#ifdef _MSC_VER
    for (; i < 2000; i++)
        rc += naked();
#endif
    return i;
}

int main()
{
    int rc;
    printf("hello");
    fflush(stdout);
    rc = foo();
    printf("world\n");
    return rc;
}

