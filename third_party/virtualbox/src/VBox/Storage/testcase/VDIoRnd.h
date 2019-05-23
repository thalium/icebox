/** @file
 *
 * VBox HDD container test utility - I/O data generator.
 */

/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef _VDIoRnd_h__
#define _VDIoRnd_h__

/** Pointer to the I/O random number generator. */
typedef struct VDIORND *PVDIORND;
/** Pointer to a I/O random number generator pointer. */
typedef PVDIORND *PPVDIORND;

/**
 * Creates a I/O RNG.
 *
 * @returns VBox status code.
 *
 * @param ppIoRnd    Where to store the handle on success.
 * @param cbPattern  Size of the test pattern to create.
 * @param uSeed      Seed for the RNG.
 */
int VDIoRndCreate(PPVDIORND ppIoRnd, size_t cbPattern, uint64_t uSeed);

/**
 * Destroys the I/O RNG.
 *
 * @param pIoRnd     I/O RNG handle.
 */
void VDIoRndDestroy(PVDIORND pIoRnd);

/**
 * Returns a pointer filled with random data of the given size.
 *
 * @returns VBox status code.
 *
 * @param pIoRnd    I/O RNG handle.
 * @param ppv       Where to store the pointer on success.
 * @param cb        Size of the buffer.
 */
int VDIoRndGetBuffer(PVDIORND pIoRnd, void **ppv, size_t cb);

uint32_t VDIoRndGetU32Ex(PVDIORND pIoRnd, uint32_t uMin, uint32_t uMax);
#endif /* _VDIoRnd_h__ */
