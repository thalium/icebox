/* $Id: kCpus.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kCpus - CPU Identifiers.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___k_kCpus_h___
#define ___k_kCpus_h___

/** @defgroup grp_kCpus     kCpus - CPU Identifiers
 * @see the kCpu API for functions operating on the CPU type.
 * @{
 */

/**
 * CPU Architectures.
 *
 * The constants used by this enum has the same values as
 * the K_ARCH_* #defines defined by k/kDefs.h.
 */
typedef enum KCPUARCH
{
    /** @copydoc K_ARCH_UNKNOWN */
    KCPUARCH_UNKNOWN = K_ARCH_UNKNOWN,
    /** @copydoc K_ARCH_X86_16 */
    KCPUARCH_X86_16 = K_ARCH_X86_16,
    /** @copydoc K_ARCH_X86_32 */
    KCPUARCH_X86_32 = K_ARCH_X86_32,
    /** @copydoc K_ARCH_AMD64 */
    KCPUARCH_AMD64 = K_ARCH_AMD64,
    /** @copydoc K_ARCH_IA64 */
    KCPUARCH_IA64 = K_ARCH_IA64,
    /** @copydoc K_ARCH_ALPHA */
    KCPUARCH_ALPHA = K_ARCH_ALPHA,
    /** @copydoc K_ARCH_ALPHA_32 */
    KCPUARCH_ALPHA_32 = K_ARCH_ALPHA_32,
    /** @copydoc K_ARCH_ARM_32 */
    KCPUARCH_ARM_32 = K_ARCH_ARM_32,
    /** @copydoc K_ARCH_ARM_64 */
    KCPUARCH_ARM_64 = K_ARCH_ARM_64,
    /** @copydoc K_ARCH_MIPS_32 */
    KCPUARCH_MIPS_32 = K_ARCH_MIPS_32,
    /** @copydoc K_ARCH_MIPS_64 */
    KCPUARCH_MIPS_64 = K_ARCH_MIPS_64,
    /** @copydoc K_ARCH_POWERPC_32 */
    KCPUARCH_POWERPC_32 = K_ARCH_POWERPC_32,
    /** @copydoc K_ARCH_POWERPC_64 */
    KCPUARCH_POWERPC_64 = K_ARCH_POWERPC_64,
    /** @copydoc K_ARCH_SPARC_32 */
    KCPUARCH_SPARC_32 = K_ARCH_SPARC_32,
    /** @copydoc K_ARCH_SPARC_64 */
    KCPUARCH_SPARC_64 = K_ARCH_SPARC_64,

    /** Hack to blow the type up to 32-bit. */
    KCPUARCH_32BIT_HACK = 0x7fffffff
} KCPUARCH;

/** Pointer to a CPU architecture type. */
typedef KCPUARCH *PKCPUARCH;
/** Pointer to a const CPU architecture type. */
typedef const KCPUARCH *PCKCPUARCH;


/**
 * CPU models.
 */
typedef enum KCPU
{
    /** The usual invalid cpu. */
    KCPU_INVALID = 0,

    /** @name K_ARCH_X86_16
     * @{ */
    KCPU_I8086,
    KCPU_I8088,
    KCPU_I80186,
    KCPU_I80286,
    KCPU_I386_16,
    KCPU_I486_16,
    KCPU_I486SX_16,
    KCPU_I586_16,
    KCPU_I686_16,
    KCPU_P4_16,
    KCPU_CORE2_16,
    KCPU_K6_16,
    KCPU_K7_16,
    KCPU_K8_16,
    KCPU_FIRST_X86_16 = KCPU_I8086,
    KCPU_LAST_X86_16 = KCPU_K8_16,
    /** @} */

    /** @name K_ARCH_X86_32
     * @{ */
    KCPU_X86_32_BLEND,
    KCPU_I386,
    KCPU_I486,
    KCPU_I486SX,
    KCPU_I586,
    KCPU_I686,
    KCPU_P4,
    KCPU_CORE2_32,
    KCPU_K6,
    KCPU_K7,
    KCPU_K8_32,
    KCPU_FIRST_X86_32 = KCPU_I386,
    KCPU_LAST_X86_32 = KCPU_K8_32,
    /** @} */

    /** @name K_ARCH_AMD64
     * @{ */
    KCPU_AMD64_BLEND,
    KCPU_K8,
    KCPU_P4_64,
    KCPU_CORE2,
    KCPU_FIRST_AMD64 = KCPU_K8,
    KCPU_LAST_AMD64 = KCPU_CORE2,
    /** @} */

    /** The end of the valid cpu values (exclusive). */
    KCPU_END,
    /** Hack to blow the type up to 32-bit. */
    KCPU_32BIT_HACK = 0x7fffffff
} KCPU;

/** Pointer to a CPU type. */
typedef KCPU *PKCPU;
/** Pointer to a const CPU type. */
typedef const KCPU *PCKCPU;

/** @} */

#endif

