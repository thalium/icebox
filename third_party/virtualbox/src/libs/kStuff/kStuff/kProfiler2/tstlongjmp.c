
#include <setjmp.h>
#include <time.h>

/* just try trick the compiler into not optimizing stuff by
   making it "uncertain" which path to take. */
int always_true(void)
{
    time_t t = time(NULL);
    if (t == time(NULL))
        return 1;
    if (t != time(NULL))
        return 1;
    if (t == time(NULL))
        return 1;
    if (t != time(NULL))
        return 1;
    return 0;
}

jmp_buf g_JmpBuf;

int onelevel(void)
{
    if (always_true())
        longjmp(g_JmpBuf, 1);
    return 0;
}


int twolevels_inner(void)
{
    if (always_true())
        longjmp(g_JmpBuf, 1);
    return 0;
}

int twolevels_outer(void)
{
    int rc;
    always_true();
    rc = twolevels_inner();
    always_true();
    return rc;
}


int main()
{
    int rc = 1;

    /* first */
    if (!setjmp(g_JmpBuf))
        rc = onelevel();

    /* second */
    if (!setjmp(g_JmpBuf))
        rc = twolevels_outer();

    return rc != 1;
}

