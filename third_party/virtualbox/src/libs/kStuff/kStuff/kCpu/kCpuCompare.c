/* $Id: kCpuCompare.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kCpu - kCpuCompare.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kCpu.h>
#include <k/kErrors.h>


/**
 * Compares arch+cpu some code was generated for with a arch+cpu for executing it
 * to see if it'll work out fine or not.
 *
 * @returns 0 if the code is compatible with the cpu.
 * @returns KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE if the arch+cpu isn't compatible with the code.
 *
 * @param   enmCodeArch The architecture the code was generated for.
 * @param   enmCodeCpu  The cpu the code was generated for.
 * @param   enmArch     The architecture to run it on.
 * @param   enmCpu      The cpu to run it on.
 */
KCPU_DECL(int) kCpuCompare(KCPUARCH enmCodeArch, KCPU enmCodeCpu, KCPUARCH enmArch, KCPU enmCpu)
{
    /*
     * Compare arch and cpu.
     */
    if (enmCodeArch != enmArch)
        return KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;

    /* exact match is nice. */
    if (enmCodeCpu == enmCpu)
        return 0;

    switch (enmArch)
    {
        case K_ARCH_X86_16:
            if (enmCpu < KCPU_FIRST_X86_16 || enmCpu > KCPU_LAST_X86_16)
                return KERR_INVALID_PARAMETER;

            /* intel? */
            if (enmCodeCpu <= KCPU_CORE2_16)
            {
                /* also intel? */
                if (enmCpu <= KCPU_CORE2_16)
                    return enmCodeCpu <= enmCpu ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                switch (enmCpu)
                {
                    case KCPU_K6_16:
                        return enmCodeCpu <= KCPU_I586 ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                    case KCPU_K7_16:
                    case KCPU_K8_16:
                    default:
                        return enmCodeCpu <= KCPU_I686 ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                }
            }
            /* amd */
            return enmCpu >= KCPU_K6_16 && enmCpu <= KCPU_K8_16
                    ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;

        case K_ARCH_X86_32:
            if (enmCpu < KCPU_FIRST_X86_32 || enmCpu > KCPU_LAST_X86_32)
                return KERR_INVALID_PARAMETER;

            /* blend? */
            if (enmCodeCpu == KCPU_X86_32_BLEND)
                return 0;

            /* intel? */
            if (enmCodeCpu <= KCPU_CORE2_32)
            {
                /* also intel? */
                if (enmCpu <= KCPU_CORE2_32)
                    return enmCodeCpu <= enmCpu ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                switch (enmCpu)
                {
                    case KCPU_K6:
                        return enmCodeCpu <= KCPU_I586 ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                    case KCPU_K7:
                    case KCPU_K8_32:
                    default:
                        return enmCodeCpu <= KCPU_I686 ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
                }
            }
            /* amd */
            return enmCpu >= KCPU_K6 && enmCpu <= KCPU_K8_32
                    ? 0 : KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;

        case K_ARCH_AMD64:
            if (enmCpu < KCPU_FIRST_AMD64 || enmCpu > KCPU_LAST_AMD64)
                return KERR_INVALID_PARAMETER;

            /* blend? */
            if (enmCodeCpu == KCPU_AMD64_BLEND)
                return 0;
            /* this is simple for now. */
            return KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;

        default:
            break;
    }
    return KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE;
}

