/* $Id: kDbgHlp.h 78 2016-07-13 15:52:04Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Internal Header.
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

#ifndef ___kDbgHlp_h___
#define ___kDbgHlp_h___

#include <k/kDbgBase.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup grp_kDbgHlpHeap   kDbg Internal Heap APIs.
 * @internal
 * @{
 */

/**
 * Allocates memory.
 *
 * @returns Pointer to the allocated memory.
 *          NULL on failure.
 * @param   cb      The number of bytes to allocate.
 */
void *kDbgHlpAlloc(size_t cb);

/**
 * Allocates memory like kDbgHlpAlloc, except that it's zeroed.
 *
 * @returns Pointer to the allocated memory.
 *          NULL on failure.
 * @param   cb      The number of bytes to allocate.
 */
void *kDbgHlpAllocZ(size_t cb);

/**
 * Combination of kDbgHlpAlloc and memcpy.
 *
 * @returns Pointer to the duplicate.
 *          NULL on failure.
 *
 * @param   pv      The memory to be duplicate.
 * @param   cb      The size of the block.
 */
void *kDbgHlpAllocDup(const void *pv, size_t cb);

/**
 * Reallocates a memory block returned by kDbgHlpAlloc, kDbgHlpAllocZ
 * kDbgHlpAllocDup or this function.
 *
 * The content of new memory added to the memory block is undefined.
 *
 * @returns Pointer to the allocated memory.
 *          NULL on failure, the old block remains intact.
 * @param   pv      The memory block to reallocate.
 *                  If NULL this function will work like kDbgHlpAlloc.
 * @param   cb      The number of bytes to allocate.
 *                  If 0 this function will work like kDbgHlpFree.
 */
void *kDbgHlpRealloc(void *pv, size_t cb);

/**
 * Frees memory allocated by kDbgHlpAlloc, kDbgHlpAllocZ
 * kDbgHlpAllocDup, or kDbgHlpRealloc.
 *
 * @param pv
 */
void kDbgHlpFree(void *pv);

/** @} */


/** @defgroup grp_kDbgHlpFile   kDbg Internal File Access APIs.
 * @internal
 * @{
 */
/**
 * Opens the specified file as read-only, buffered if possible.
 *
 * @returns 0 on success, or the appropriate KDBG_ERR_* on failure.
 *
 * @param   pszFilename     The file to open.
 * @param   ppFile          Where to store the handle to the open file.
 */
int kDbgHlpOpenRO(const char *pszFilename, PKDBGHLPFILE *ppFile);


/**
 * Closes a file opened by kDbgHlpOpenRO.
 *
 * @param   pFile           The file handle.
 */
void kDbgHlpClose(PKDBGHLPFILE pFile);

/**
 * Gets the native file handle.
 *
 * @return  The native file handle.
 *          -1 on failure.
 * @param   pFile           The file handle.
 */
uintptr_t kDbgHlpNativeFileHandle(PKDBGHLPFILE pFile);

/**
 * Gets the size of an open file.
 *
 * @returns The file size in bytes on success.
 *          On failure -1 is returned.
 * @param   pFile           The file handle.
 */
int64_t kDbgHlpFileSize(PKDBGHLPFILE pFile);

/**
 * Reads a number of bytes at a specified file location.
 *
 * This will change the current file position to off + cb on success,
 * while on failure the position will be undefined.
 *
 * @returns The file size in bytes on success.
 *          On failure -1 is returned.
 * @param   pFile           The file handle.
 * @param   off             Where to read.
 * @param   pv              Where to store the data.
 * @param   cb              How much to read.
 */
int kDbgHlpReadAt(PKDBGHLPFILE pFile, int64_t off, void *pv, size_t cb);

/**
 * Reads a number of bytes at the current file position.
 *
 * This will advance the current file position by cb bytes on success
 * while on failure the position will be undefined.
 *
 * @returns The file size in bytes on success.
 *          On failure -1 is returned.
 * @param   pFile           The file handle.
 * @param   pv              Where to store the data.
 * @param   cb              How much to read.
 * @param   off             Where to read.
 */
int kDbgHlpRead(PKDBGHLPFILE pFile, void *pv, size_t cb);

/**
 * Sets the current file position.
 *
 * @returns 0 on success, and KDBG_ERR_* on failure.
 * @param   pFile           The file handle.
 * @param   off             The desired file position.
 */
int kDbgHlpSeek(PKDBGHLPFILE pFile, int64_t off);

/**
 * Move the file position relative to the current one.
 *
 * @returns 0 on success, and KDBG_ERR_* on failure.
 * @param   pFile           The file handle.
 * @param   off             How much to move the file position by.
 */
int kDbgHlpSeekByCur(PKDBGHLPFILE pFile, int64_t off);

/**
 * Move the file position relative to the end of the file.
 *
 * @returns 0 on success, and KDBG_ERR_* on failure.
 * @param   pFile           The file handle.
 * @param   off             The offset relative to the end, positive number.
 */
int kDbgHlpSeekByEnd(PKDBGHLPFILE pFile, int64_t off);

/**
 * Gets the current file position.
 *
 * @returns The current file position on success.
 *          -1 on failure.
 * @param   pFile           The file handle.
 */
int64_t kDbgHlpTell(PKDBGHLPFILE pFile);

/** @} */

/** @defgroup grp_kDbgHlpAssert     kDbg Internal Assertion Macros.
 * @internal
 * @{
 */

#ifdef _MSC_VER
# define kDbgAssertBreakpoint() do { __debugbreak(); } while (0)
#else
# define kDbgAssertBreakpoint() do { __asm__ __volatile__ ("int3"); } while (0)
#endif

/**
 * Helper function that displays the first part of the assertion message.
 *
 * @param   pszExpr         The expression.
 * @param   pszFile         The file name.
 * @param   iLine           The line number is the file.
 * @param   pszFunction     The function name.
 */
void kDbgAssertMsg1(const char *pszExpr, const char *pszFile, unsigned iLine, const char *pszFunction);

/**
 * Helper function that displays custom assert message.
 *
 * @param   pszFormat       Format string that get passed to vprintf.
 * @param   ...             Format arguments.
 */
void kDbgAssertMsg2(const char *pszFormat, ...);


#ifdef KDBG_STRICT

# define kDbgAssert(expr) \
    do { \
        if (!(expr)) \
        { \
            kDbgAssertMsg1(#expr, __FILE__, __LINE__, K_FUNCTION); \
            kDbgAssertBreakpoint(); \
        } \
    } while (0)

# define kDbgAssertReturn(expr, rcRet) \
    do { \
        if (!(expr)) \
        { \
            kDbgAssertMsg1(#expr, __FILE__, __LINE__, K_FUNCTION); \
            kDbgAssertBreakpoint(); \
            return (rcRet); \
        } \
    } while (0)

# define kDbgAssertMsg(expr, msg) \
    do { \
        if (!(expr)) \
        { \
            kDbgAssertMsg1(#expr, __FILE__, __LINE__, K_FUNCTION); \
            kDbgAssertMsg2 msg; \
            kDbgAssertBreakpoint(); \
        } \
    } while (0)

# define kDbgAssertMsgReturn(expr, msg, rcRet) \
    do { \
        if (!(expr)) \
        { \
            kDbgAssertMsg1(#expr, __FILE__, __LINE__, K_FUNCTION); \
            kDbgAssertMsg2 msg; \
            kDbgAssertBreakpoint(); \
            return (rcRet); \
        } \
    } while (0)

#else   /* !KDBG_STRICT */
# define kDbgAssert(expr)                       do { } while (0)
# define kDbgAssertReturn(expr, rcRet)          do { if (!(expr)) return (rcRet); } while (0)
# define kDbgAssertMsg(expr, msg)               do { } while (0)
# define kDbgAssertMsgReturn(expr, msg, rcRet)  do { if (!(expr)) return (rcRet); } while (0)
#endif  /* !KDBG_STRICT */

#define kDbgAssertPtr(ptr)                      kDbgAssertMsg(KDBG_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kDbgAssertPtrReturn(ptr, rcRet)         kDbgAssertMsgReturn(KDBG_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kDbgAssertPtrNull(ptr)                  kDbgAssertMsg(!(ptr) || KDBG_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kDbgAssertPtrNullReturn(ptr, rcRet)     kDbgAssertMsgReturn(!(ptr) || KDBG_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kDbgAssertRC(rc)                        kDbgAssertMsg((rc) == 0, ("%s = %d\n", #rc, (rc)))
#define kDbgAssertRCReturn(rc, rcRet)           kDbgAssertMsgReturn((rc) == 0, ("%s = %d -> %d\n", #rc, (rc), (rcRet)), (rcRet))
#define kDbgAssertFailed()                      kDbgAssert(0)
#define kDbgAssertFailedReturn(rcRet)           kDbgAssertReturn(0, (rcRet))
#define kDbgAssertMsgFailed(msg)                kDbgAssertMsg(0, msg)
#define kDbgAssertMsgFailedReturn(msg, rcRet)   kDbgAssertMsgReturn(0, msg, (rcRet))

/** @} */

#ifdef __cplusplus
}
#endif

#endif

