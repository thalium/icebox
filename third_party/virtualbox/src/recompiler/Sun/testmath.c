/* $Id: testmath.c $ */
/** @file
 * Testcase for the no-crt math stuff.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifndef MATHTEST_STANDALONE
# include <iprt/assert.h>
# include <math.h>
# undef printf
# define printf RTAssertMsg2Weak
#else
# include <stdio.h>
# include <math.h>
#endif

/* gcc starting with version 4.3 uses the MPFR library which results in more accurate results. gcc-4.3.1 seems to emit the less accurate result. So just allow both results. */
#define SIN180a -0.8011526357338304777463731115L
#define SIN180b -0.801152635733830477871L

static void bitch(const char *pszWhat, const long double *plrdResult, const long double *plrdExpected)
{
    const unsigned char *pach1 = (const unsigned char *)plrdResult;
    const unsigned char *pach2 = (const unsigned char *)plrdExpected;
#ifndef MATHTEST_STANDALONE
    printf("error: %s - %d instead of %d\n", pszWhat, (int)(*plrdResult * 100000), (int)(*plrdExpected * 100000));
#else
    printf("error: %s - %.25f instead of %.25f\n", pszWhat, (double)*plrdResult, (double)*plrdExpected);
#endif
    printf("   %02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x\n", pach1[0], pach1[1], pach1[2], pach1[3], pach1[4], pach1[5], pach1[6], pach1[7], pach1[8], pach1[9]);
    printf("   %02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x\n", pach2[0], pach2[1], pach2[2], pach2[3], pach2[4], pach2[5], pach2[6], pach2[7], pach2[8], pach2[9]);
}

static void bitchll(const char *pszWhat, long long llResult, long long llExpected)
{
#if defined(__MINGW32__) && !defined(Assert)
    printf("error: %s - %I64d instead of %I64d\n", pszWhat, llResult, llExpected);
#else
    printf("error: %s - %lld instead of %lld\n", pszWhat, llResult, llExpected);
#endif
}

static void bitchl(const char *pszWhat, long lResult, long lExpected)
{
    printf("error: %s - %ld instead of %ld\n", pszWhat, lResult, lExpected);
}

extern int testsin(void)
{
    return sinl(180.0L) == SIN180a || sinl(180.0L) == SIN180b;
}

extern int testremainder(void)
{
    static double s_rd1 = 2.5;
    static double s_rd2 = 2.0;
    static double s_rd3 = 0.5;
    return remainder(s_rd1, s_rd2) == s_rd3;
}

static __inline__ void set_cw(unsigned cw)
{
    __asm __volatile("fldcw %0" : : "m" (cw));
}

static __inline__ unsigned get_cw(void)
{
    unsigned cw;
    __asm __volatile("fstcw %0" : "=m" (cw));
    return cw & 0xffff;
}

static long double check_lrd(const long double lrd, const unsigned long long ull, const unsigned short us)
{
    static volatile long double lrd2;
    lrd2 = lrd;
    if (    *(unsigned long long *)&lrd2 != ull
        ||  ((unsigned short *)&lrd2)[4] != us)
    {
#if defined(__MINGW32__) && !defined(Assert)
        printf("%I64x:%04x instead of %I64x:%04x\n", *(unsigned long long *)&lrd2, ((unsigned short *)&lrd2)[4], ull, us);
#else
        printf("%llx:%04x instead of %llx:%04x\n", *(unsigned long long *)&lrd2, ((unsigned short *)&lrd2)[4], ull, us);
#endif
        __asm__("int $3\n");
    }
    return lrd;
}


static long double make_lrd(const unsigned long long ull, const unsigned short us)
{
    union
    {
        long double lrd;
        struct
        {
            unsigned long long ull;
            unsigned short us;
        } i;
    } u;

    u.i.ull = ull;
    u.i.us = us;
    return u.lrd;
}

static long double check_lrd_cw(const long double lrd, const unsigned long long ull, const unsigned short us, const unsigned cw)
{
    set_cw(cw);
    if (cw != get_cw())
    {
        printf("get_cw() -> %#x expected %#x\n", get_cw(), cw);
        __asm__("int $3\n");
    }
    return check_lrd(lrd, ull, us);
}

static long double make_lrd_cw(unsigned long long ull, unsigned short us, unsigned cw)
{
    set_cw(cw);
    return check_lrd_cw(make_lrd(ull, us), ull, us, cw);
}

extern int testmath(void)
{
    unsigned cErrors = 0;
    long double lrdResult;
    long double lrdExpect;
    long double lrd;
#define CHECK(operation, expect) \
    do { \
        lrdExpect = expect; \
        lrdResult = operation; \
        if (lrdResult != lrdExpect) \
        {  \
            bitch(#operation,  &lrdResult, &lrdExpect); \
            cErrors++; \
        } \
    } while (0)

    long long llResult;
    long long llExpect;
#define CHECKLL(operation, expect) \
    do { \
        llExpect = expect; \
        llResult = operation; \
        if (llResult != llExpect) \
        {  \
            bitchll(#operation,  llResult, llExpect); \
            cErrors++; \
        } \
    } while (0)

    long lResult;
    long lExpect;
#define CHECKL(operation, expect) \
    do { \
        lExpect = expect; \
        lResult = operation; \
        if (lResult != lExpect) \
        {  \
            bitchl(#operation,  lResult, lExpect); \
            cErrors++; \
        } \
    } while (0)

    CHECK(atan2l(1.0L, 1.0L), 0.785398163397448309603L);
    CHECK(atan2l(2.3L, 3.3L), 0.608689307327411694890L);

    CHECK(ceill(1.9L), 2.0L);
    CHECK(ceill(4.5L), 5.0L);
    CHECK(ceill(3.3L), 4.0L);
    CHECK(ceill(6.1L), 7.0L);

    CHECK(floorl(1.9L), 1.0L);
    CHECK(floorl(4.5L), 4.0L);
    CHECK(floorl(7.3L), 7.0L);
    CHECK(floorl(1234.1L), 1234.0L);
    CHECK(floor(1233.1), 1233.0);
    CHECK(floor(1239.98989898), 1239.0);
    CHECK(floorf(9999.999), 9999.0);

    CHECK(ldexpl(1.0L, 1), 2.0L);
    CHECK(ldexpl(1.0L, 10), 1024.0L);
    CHECK(ldexpl(2.25L, 10), 2304.0L);

    CHECKLL(llrintl(1.0L), 1);
    CHECKLL(llrintl(1.3L), 1);
    CHECKLL(llrintl(1.5L), 2);
    CHECKLL(llrintl(1.9L), 2);
    CHECKLL(llrintf(123.34), 123);
    CHECKLL(llrintf(-123.50), -124);
    CHECKLL(llrint(42.42), 42);
    CHECKLL(llrint(-2147483648.12343), -2147483648LL);
#if !defined(RT_ARCH_AMD64)
    CHECKLL(lrint(-21474836499.12343), -2147483648LL);
    CHECKLL(lrint(-2147483649932412.12343), -2147483648LL);
#else
    CHECKLL(lrint(-21474836499.12343), -21474836499L);
    CHECKLL(lrint(-2147483649932412.12343), -2147483649932412L);
#endif

//    __asm__("int $3");
    CHECKL(lrintl(make_lrd_cw(000000000000000000ULL,000000,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x067f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x067f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x0a7f)), 1L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x0a7f)), 1L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x3ffe,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x067f)), -1L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x067f)), -1L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x0a7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x0a7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0xbffe,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x067f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x067f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x0a7f)), 1L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x0a7f)), 1L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x9249249249249000ULL,0x3ffc,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x027f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x067f)), -1L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x067f)), -1L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x0a7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x0a7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0xe38e38e38e38e000ULL,0xbffb,0x0e7f)), 0L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x027f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x027f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x067f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x067f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x0a7f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x0a7f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x0e7f)), 32768L);
    CHECKL(lrintl(make_lrd_cw(0x8000000000000000ULL,0x400e,0x0e7f)), 32768L);
#if !defined(RT_ARCH_AMD64)
    /* c90 says that the constant is 2147483648 (which is not representable as a signed 32-bit
     * value).  To that constant you've then applied the negation operation. c90 doesn't have
     * negative constants, only positive ones that have been negated.  */
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x027f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x027f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x067f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x067f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x0a7f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x0a7f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x0e7f)), (long)(-2147483647L - 1));
    CHECKL(lrintl(make_lrd_cw(0xad78ebc5ac620000ULL,0xc041,0x0e7f)), (long)(-2147483647L - 1));
#endif
    set_cw(0x27f);

    CHECK(logl(2.7182818284590452353602874713526625L), 1.0);

    CHECK(remainderl(1.0L, 1.0L), 0.0);
    CHECK(remainderl(1.0L, 1.5L), -0.5);
    CHECK(remainderl(42.0L, 34.25L), 7.75);
    CHECK(remainderf(43.0, 34.25), 8.75);
    CHECK(remainder(44.25, 34.25), 10.00);
    double rd1 = 44.25;
    double rd2 = 34.25;
    CHECK(remainder(rd1, rd2), 10.00);
    CHECK(remainder(2.5, 2.0), 0.5);
    CHECK(remainder(2.5, 2.0), 0.5);
    CHECK(remainder(2.5, 2.0), 0.5);
    CHECKLL(testremainder(), 1);


    /* Only works in extended precision, while double precision is default on BSD (including Darwin) */
    set_cw(0x37f);
    CHECK(rintl(1.0L), 1.0);
    CHECK(rintl(1.4L), 1.0);
    CHECK(rintl(1.3L), 1.0);
    CHECK(rintl(0.9L), 1.0);
    CHECK(rintl(3123.1232L), 3123.0);
    CHECK(rint(3985.13454), 3985.0);
    CHECK(rintf(9999.999), 10000.0);
    set_cw(0x27f);

    CHECK(sinl(1.0L),  0.84147098480789650664L);
#if 0
    lrd = 180.0L;
    CHECK(sinl(lrd), -0.801152635733830477871L);
#else
    lrd = 180.0L;
    lrdExpect = SIN180a;
    lrdResult = sinl(lrd);
    if (lrdResult != lrdExpect)
    {
        lrdExpect = SIN180b;
        if (lrdResult != lrdExpect)
        {
            bitch("sinl(lrd)",  &lrdResult, &lrdExpect);
            cErrors++;
        }
    }
#endif
#if 0
    CHECK(sinl(180.0L), SIN180);
#else
    lrdExpect = SIN180a;
    lrdResult = sinl(180.0L);
    if (lrdResult != lrdExpect)
    {
        lrdExpect = SIN180b;
        if (lrdResult != lrdExpect)
        {
            bitch("sinl(180.0L)",  &lrdResult, &lrdExpect);
            cErrors++;
        }
    }
#endif
    CHECKLL(testsin(), 1);

    CHECK(sqrtl(1.0L), 1.0);
    CHECK(sqrtl(4.0L), 2.0);
    CHECK(sqrtl(1525225.0L), 1235.0);

    CHECK(tanl(0.0L), 0.0);
    CHECK(tanl(0.7853981633974483096156608458198757L), 1.0);

    CHECK(powl(0.0, 0.0), 1.0);
    CHECK(powl(2.0, 2.0), 4.0);
    CHECK(powl(3.0, 3.0), 27.0);

    return cErrors;
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#if 0

#define floatx_to_int32 floatx80_to_int32
#define floatx_to_int64 floatx80_to_int64
#define floatx_to_int32_round_to_zero floatx80_to_int32_round_to_zero
#define floatx_to_int64_round_to_zero floatx80_to_int64_round_to_zero
#define floatx_abs floatx80_abs
#define floatx_chs floatx80_chs
#define floatx_round_to_int(foo, bar) floatx80_round_to_int(foo, NULL)
#define floatx_compare floatx80_compare
#define floatx_compare_quiet floatx80_compare_quiet
#undef sin
#undef cos
#undef sqrt
#undef pow
#undef log
#undef tan
#undef atan2
#undef floor
#undef ceil
#undef ldexp
#define sin sinl
#define cos cosl
#define sqrt sqrtl
#define pow powl
#define log logl
#define tan tanl
#define atan2 atan2l
#define floor floorl
#define ceil ceill
#define ldexp ldexpl


typedef long double CPU86_LDouble;

typedef union {
    long double d;
    struct {
        unsigned long long lower;
        unsigned short upper;
    } l;
} CPU86_LDoubleU;

/* the following deal with x86 long double-precision numbers */
#define MAXEXPD 0x7fff
#define EXPBIAS 16383
#define EXPD(fp)	(fp.l.upper & 0x7fff)
#define SIGND(fp)	((fp.l.upper) & 0x8000)
#define MANTD(fp)       (fp.l.lower)
#define BIASEXPONENT(fp) fp.l.upper = (fp.l.upper & ~(0x7fff)) | EXPBIAS

typedef long double floatx80;
#define STATUS_PARAM , void *pv

static floatx80 floatx80_round_to_int( floatx80 a STATUS_PARAM)
{
    return rintl(a);
}



struct myenv
{
    unsigned int fpstt; /* top of stack index */
    unsigned int fpus;
    unsigned int fpuc;
    unsigned char fptags[8];   /* 0 = valid, 1 = empty */
    union {
#ifdef USE_X86LDOUBLE
        CPU86_LDouble d __attribute__((aligned(16)));
#else
        CPU86_LDouble d;
#endif
    } fpregs[8];

} my_env, env_org, env_res, *env = &my_env;


#define ST0    (env->fpregs[env->fpstt].d)
#define ST(n)  (env->fpregs[(env->fpstt + (n)) & 7].d)
#define ST1    ST(1)
#define MAXTAN 9223372036854775808.0


static inline void fpush(void)
{
    env->fpstt = (env->fpstt - 1) & 7;
    env->fptags[env->fpstt] = 0; /* validate stack entry */
}

static inline void fpop(void)
{
    env->fptags[env->fpstt] = 1; /* invalidate stack entry */
    env->fpstt = (env->fpstt + 1) & 7;
}

static void helper_f2xm1(void)
{
    ST0 = pow(2.0,ST0) - 1.0;
}

static void helper_fyl2x(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if (fptemp>0.0){
        fptemp = log(fptemp)/log(2.0);	 /* log2(ST) */
        ST1 *= fptemp;
        fpop();
    } else {
        env->fpus &= (~0x4700);
        env->fpus |= 0x400;
    }
}

static void helper_fptan(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if((fptemp > MAXTAN)||(fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = tan(fptemp);
        fpush();
        ST0 = 1.0;
        env->fpus &= (~0x400);  /* C2 <-- 0 */
        /* the above code is for  |arg| < 2**52 only */
    }
}

static void helper_fpatan(void)
{
    CPU86_LDouble fptemp, fpsrcop;

    fpsrcop = ST1;
    fptemp = ST0;
    ST1 = atan2(fpsrcop,fptemp);
    fpop();
}

static void helper_fxtract(void)
{
    CPU86_LDoubleU temp;
    unsigned int expdif;

    temp.d = ST0;
    expdif = EXPD(temp) - EXPBIAS;
    /*DP exponent bias*/
    ST0 = expdif;
    fpush();
    BIASEXPONENT(temp);
    ST0 = temp.d;
}

static void helper_fprem1(void)
{
    CPU86_LDouble dblq, fpsrcop, fptemp;
    CPU86_LDoubleU fpsrcop1, fptemp1;
    int expdif;
    int q;

    fpsrcop = ST0;
    fptemp = ST1;
    fpsrcop1.d = fpsrcop;
    fptemp1.d = fptemp;
    expdif = EXPD(fpsrcop1) - EXPD(fptemp1);
    if (expdif < 53) {
        dblq = fpsrcop / fptemp;
        dblq = (dblq < 0.0)? ceil(dblq): floor(dblq);
        ST0 = fpsrcop - fptemp*dblq;
        q = (int)dblq; /* cutting off top bits is assumed here */
        env->fpus &= (~0x4700); /* (C3,C2,C1,C0) <-- 0000 */
				/* (C0,C1,C3) <-- (q2,q1,q0) */
        env->fpus |= (q&0x4) << 6; /* (C0) <-- q2 */
        env->fpus |= (q&0x2) << 8; /* (C1) <-- q1 */
        env->fpus |= (q&0x1) << 14; /* (C3) <-- q0 */
    } else {
        env->fpus |= 0x400;  /* C2 <-- 1 */
        fptemp = pow(2.0, expdif-50);
        fpsrcop = (ST0 / ST1) / fptemp;
        /* fpsrcop = integer obtained by rounding to the nearest */
        fpsrcop = (fpsrcop-floor(fpsrcop) < ceil(fpsrcop)-fpsrcop)?
            floor(fpsrcop): ceil(fpsrcop);
        ST0 -= (ST1 * fpsrcop * fptemp);
    }
}

static void helper_fprem(void)
{
#if 0
LogFlow(("helper_fprem: ST0=%.*Rhxs ST1=%.*Rhxs fpus=%#x\n", sizeof(ST0), &ST0, sizeof(ST1), &ST1, env->fpus));

    __asm__ __volatile__("fldt (%2)\n"
                         "fldt (%1)\n"
                         "fprem \n"
                         "fnstsw (%0)\n"
                         "fstpt (%1)\n"
                         "fstpt (%2)\n"
                         : : "r" (&env->fpus), "r" (&ST0), "r" (&ST1) : "memory");

LogFlow(("helper_fprem: -> ST0=%.*Rhxs fpus=%#x c\n", sizeof(ST0), &ST0, env->fpus));
#else
    CPU86_LDouble dblq, fpsrcop, fptemp;
    CPU86_LDoubleU fpsrcop1, fptemp1;
    int expdif;
    int q;

    fpsrcop = ST0;
    fptemp = ST1;
    fpsrcop1.d = fpsrcop;
    fptemp1.d = fptemp;

    expdif = EXPD(fpsrcop1) - EXPD(fptemp1);
    if ( expdif < 53 ) {
        dblq = fpsrcop / fptemp;
        dblq = (dblq < 0.0)? ceil(dblq): floor(dblq);
        ST0 = fpsrcop - fptemp*dblq;
        q = (int)dblq; /* cutting off top bits is assumed here */
        env->fpus &= (~0x4700); /* (C3,C2,C1,C0) <-- 0000 */
				/* (C0,C1,C3) <-- (q2,q1,q0) */
        env->fpus |= (q&0x4) << 6; /* (C0) <-- q2 */
        env->fpus |= (q&0x2) << 8; /* (C1) <-- q1 */
        env->fpus |= (q&0x1) << 14; /* (C3) <-- q0 */
    } else {
        env->fpus |= 0x400;  /* C2 <-- 1 */
        fptemp = pow(2.0, expdif-50);
        fpsrcop = (ST0 / ST1) / fptemp;
        /* fpsrcop = integer obtained by chopping */
        fpsrcop = (fpsrcop < 0.0)?
            -(floor(fabs(fpsrcop))): floor(fpsrcop);
        ST0 -= (ST1 * fpsrcop * fptemp);
    }
#endif
}

static void helper_fyl2xp1(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if ((fptemp+1.0)>0.0) {
        fptemp = log(fptemp+1.0) / log(2.0); /* log2(ST+1.0) */
        ST1 *= fptemp;
        fpop();
    } else {
        env->fpus &= (~0x4700);
        env->fpus |= 0x400;
    }
}

static void helper_fsqrt(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if (fptemp<0.0) {
        env->fpus &= (~0x4700);  /* (C3,C2,C1,C0) <-- 0000 */
        env->fpus |= 0x400;
    }
    ST0 = sqrt(fptemp);
}

static void helper_fsincos(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if ((fptemp > MAXTAN)||(fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = sin(fptemp);
        fpush();
        ST0 = cos(fptemp);
        env->fpus &= (~0x400);  /* C2 <-- 0 */
        /* the above code is for  |arg| < 2**63 only */
    }
}

static void helper_frndint(void)
{
    ST0 = floatx_round_to_int(ST0, &env->fp_status);
}

static void helper_fscale(void)
{
    ST0 = ldexp (ST0, (int)(ST1));
}

static void helper_fsin(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if ((fptemp > MAXTAN)||(fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = sin(fptemp);
        env->fpus &= (~0x400);  /* C2 <-- 0 */
        /* the above code is for  |arg| < 2**53 only */
    }
}

static void helper_fcos(void)
{
    CPU86_LDouble fptemp;

    fptemp = ST0;
    if((fptemp > MAXTAN)||(fptemp < -MAXTAN)) {
        env->fpus |= 0x400;
    } else {
        ST0 = cos(fptemp);
        env->fpus &= (~0x400);  /* C2 <-- 0 */
        /* the above code is for  |arg5 < 2**63 only */
    }
}

static void helper_fxam_ST0(void)
{
    CPU86_LDoubleU temp;
    int expdif;

    temp.d = ST0;

    env->fpus &= (~0x4700);  /* (C3,C2,C1,C0) <-- 0000 */
    if (SIGND(temp))
        env->fpus |= 0x200; /* C1 <-- 1 */

    /* XXX: test fptags too */
    expdif = EXPD(temp);
    if (expdif == MAXEXPD) {
#ifdef USE_X86LDOUBLE
        if (MANTD(temp) == 0x8000000000000000ULL)
#else
        if (MANTD(temp) == 0)
#endif
            env->fpus |=  0x500 /*Infinity*/;
        else
            env->fpus |=  0x100 /*NaN*/;
    } else if (expdif == 0) {
        if (MANTD(temp) == 0)
            env->fpus |=  0x4000 /*Zero*/;
        else
            env->fpus |= 0x4400 /*Denormal*/;
    } else {
        env->fpus |= 0x400;
    }
}


void check_env(void)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        CPU86_LDoubleU my, res;
        my.d = env->fpregs[i].d;
        res.d = env_res.fpregs[i].d;

        if (    my.l.lower != res.l.lower
            ||  my.l.upper != res.l.upper)
            printf("register %i: %#018llx:%#06x\n"
                   "    expected %#018llx:%#06x\n",
                   i,
                   my.l.lower, my.l.upper,
                   res.l.lower, res.l.upper);
    }
    for (i = 0; i < 8; i++)
        if (env->fptags[i] != env_res.fptags[i])
            printf("tag %i: %d != %d\n", i, env->fptags[i], env_res.fptags[i]);
    if (env->fpstt != env_res.fpstt)
        printf("fpstt: %#06x != %#06x\n", env->fpstt, env_res.fpstt);
    if (env->fpuc != env_res.fpuc)
        printf("fpuc:  %#06x != %#06x\n", env->fpuc, env_res.fpuc);
    if (env->fpus != env_res.fpus)
        printf("fpus:  %#06x != %#06x\n", env->fpus, env_res.fpus);
}
#endif /* not used. */

#if 0 /* insert this into helper.c */
/* FPU helpers */
CPU86_LDoubleU  my_st[8];
unsigned int    my_fpstt;
unsigned int    my_fpus;
unsigned int    my_fpuc;
unsigned char my_fptags[8];

void hlp_fpu_enter(void)
{
    int i;
    for (i = 0; i < 8; i++)
        my_st[i].d = env->fpregs[i].d;
    my_fpstt = env->fpstt;
    my_fpus = env->fpus;
    my_fpuc = env->fpuc;
    memcpy(&my_fptags, &env->fptags, sizeof(my_fptags));
}

void hlp_fpu_leave(const char *psz)
{
    int i;
    Log(("/*code*/ \n"));
    for (i = 0; i < 8; i++)
        Log(("/*code*/ *(unsigned long long *)&env_org.fpregs[%d] = %#018llxULL; ((unsigned short *)&env_org.fpregs[%d])[4] = %#06x; env_org.fptags[%d]=%d;\n",
             i, my_st[i].l.lower, i, my_st[i].l.upper, i, my_fptags[i]));
    Log(("/*code*/ env_org.fpstt=%#x;\n", my_fpstt));
    Log(("/*code*/ env_org.fpus=%#x;\n", my_fpus));
    Log(("/*code*/ env_org.fpuc=%#x;\n", my_fpuc));
    for (i = 0; i < 8; i++)
    {
        CPU86_LDoubleU u;
        u.d = env->fpregs[i].d;
        Log(("/*code*/ *(unsigned long long *)&env_res.fpregs[%d] = %#018llxULL; ((unsigned short *)&env_res.fpregs[%d])[4] = %#06x; env_res.fptags[%d]=%d;\n",
             i, u.l.lower, i, u.l.upper, i, env->fptags[i]));
    }
    Log(("/*code*/ env_res.fpstt=%#x;\n", env->fpstt));
    Log(("/*code*/ env_res.fpus=%#x;\n", env->fpus));
    Log(("/*code*/ env_res.fpuc=%#x;\n", env->fpuc));

    Log(("/*code*/ my_env = env_org;\n"));
    Log(("/*code*/ %s();\n", psz));
    Log(("/*code*/ check_env();\n"));
}
#endif /* helper.c */

extern void testmath2(void )
{
#if 0
#include "/tmp/code.h"
#endif
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

#ifdef MATHTEST_STANDALONE

void test_fops(double a, double b)
{
    printf("a=%f b=%f a+b=%f\n", a, b, a + b);
    printf("a=%f b=%f a-b=%f\n", a, b, a - b);
    printf("a=%f b=%f a*b=%f\n", a, b, a * b);
    printf("a=%f b=%f a/b=%f\n", a, b, a / b);
    printf("a=%f b=%f fmod(a, b)=%f\n", a, b, (double)fmod(a, b));
    printf("a=%f sqrt(a)=%f\n", a, (double)sqrtl(a));
    printf("a=%f sin(a)=%f\n", a, (double)sinl(a));
    printf("a=%f cos(a)=%f\n", a, (double)cos(a));
    printf("a=%f tan(a)=%f\n", a, (double)tanl(a));
    printf("a=%f log(a)=%f\n", a, (double)log(a));
    printf("a=%f exp(a)=%f\n", a, (double)exp(a));
    printf("a=%f b=%f atan2(a, b)=%f\n", a, b, atan2(a, b));
    /* just to test some op combining */
    printf("a=%f asin(sinl(a))=%f\n", a, (double)asin(sinl(a)));
    printf("a=%f acos(cos(a))=%f\n", a, (double)acos(cos(a)));
    printf("a=%f atan(tanl(a))=%f\n", a, (double)atan(tanl(a)));
}

int main()
{
    unsigned cErrors = testmath();

    testmath2();
    test_fops(2, 3);
    test_fops(1.4, -5);

    printf("cErrors=%u\n", cErrors);
    return cErrors;
}
#endif

