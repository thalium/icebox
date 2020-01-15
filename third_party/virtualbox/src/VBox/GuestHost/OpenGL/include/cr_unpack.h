/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_UNPACK_H
#define CR_UNPACK_H

#include "cr_compiler.h"
#include "cr_spu.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_opcodes.h"
#include "cr_glstate.h"

#include <iprt/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unpacker state.
 */
typedef struct CrUnpackerState
{
    /** Opcodes which are going to be unpacked. */
    const uint8_t               *pbOpcodes;
    /** NUmber of Opcodes to unpack. */
    size_t                      cOpcodes;
    /** Start of the data buffer to unpack. */
    const uint8_t               *pbUnpackData;
    /** Number of bytes remaining the inpack buffer. */
    size_t                      cbUnpackDataLeft;
    /** Return pointer. */
    CRNetworkPointer            *pReturnPtr;
    /** Writeback pointer. */
    CRNetworkPointer            *pWritebackPtr;
    /** Pointer to the dispatch table to use. */
    SPUDispatchTable            *pDispatchTbl;
    /** Status code from the unpacker (mostly returns VERR_BUFFER_OVERFLOW
     * on error if one unpacker detected out of bounds buffer access). */
    int                         rcUnpack;
    /** Attached state tracker. */
    PCRStateTracker             pStateTracker;
} CrUnpackerState;
/** Pointer to an unpacker state. */
typedef CrUnpackerState *PCrUnpackerState;
/** Pointer to a const unpacker state. */
typedef const CrUnpackerState *PCCrUnpackerState;

DECLEXPORT(void) crUnpack(PCrUnpackerState pState);

typedef enum
{
    CR_UNPACK_BUFFER_TYPE_GENERIC = 0,
    CR_UNPACK_BUFFER_TYPE_CMDBLOCK_BEGIN,
    CR_UNPACK_BUFFER_TYPE_CMDBLOCK_FLUSH,
    CR_UNPACK_BUFFER_TYPE_CMDBLOCK_END
} CR_UNPACK_BUFFER_TYPE;

DECLEXPORT(CR_UNPACK_BUFFER_TYPE) crUnpackGetBufferType(const void *opcodes, unsigned int num_opcodes);

#if defined(LINUX) || defined(WINDOWS)
#define CR_UNALIGNED_ACCESS_OKAY
#else
#undef CR_UNALIGNED_ACCESS_OKAY
#endif
DECLEXPORT(double) crReadUnalignedDouble( const void *buffer );

/**
 * Paranoid helper for debug builds to make sure the buffer size is validated correctly.
 */
DECLINLINE(const void *) crUnpackAccessChk(PCCrUnpackerState pState, size_t cbAccessVerified,
                                     size_t offAccess, size_t cbType)
{
    AssertMsg(offAccess + cbType <= cbAccessVerified,
             ("CHECK_BUFFER_SIZE_STATIC() has a wrong size given (verified %zu bytes vs tried access %zu)!\n",
             cbAccessVerified, offAccess + cbType));
    RT_NOREF(cbAccessVerified, cbType);
    return pState->pbUnpackData + offAccess;
}

/**
 * Does a one time check whether the buffer has sufficient data to access at least a_cbAccess
 * bytes. Sets an error for the state and returns.
 *
 * @note Should only be used in the prologue of a method.
 * @remark The introduction of int_cbAccessVerified makes sure CHECK_BUFFER_SIZE_STATIC() is there in each method for at least
  *        some rudimentary sanity check (fails to compile otherwise).
 */
#define CHECK_BUFFER_SIZE_STATIC(a_pState, a_cbAccess) \
    if (RT_UNLIKELY((a_pState)->cbUnpackDataLeft < (a_cbAccess))) \
    { \
        (a_pState)->rcUnpack = VERR_BUFFER_OVERFLOW; \
        AssertFailed(); \
        return; \
    } \
    size_t int_cbAccessVerified = (a_cbAccess); RT_NOREF(int_cbAccessVerified)

#define CHECK_BUFFER_SIZE_STATIC_LAST(a_pState, a_offAccessLast, a_Type) CHECK_BUFFER_SIZE_STATIC(a_pState, (a_offAccessLast) + sizeof( a_Type ))

#define CHECK_BUFFER_SIZE_STATIC_UPDATE(a_pState, a_cbAccess) \
    do \
    { \
        if (RT_UNLIKELY((a_pState)->cbUnpackDataLeft < (size_t)(a_cbAccess))) \
        { \
            (a_pState)->rcUnpack = VERR_BUFFER_OVERFLOW; \
            AssertFailed(); \
            return; \
        } \
        int_cbAccessVerified = (a_cbAccess); \
    } \
    while (0)

#define CHECK_ARRAY_SIZE_STATIC_UPDATE(a_pState, a_offAccessLast, a_cElements, a_cbType) \
    do \
    { \
        AssertReturnVoidStmt((size_t)(a_cElements) < (size_t)(UINT32_MAX / 4 / (a_cbType)), \
                             (a_pState)->rcUnpack = VERR_OUT_OF_RANGE); \
        CHECK_BUFFER_SIZE_STATIC_UPDATE(a_pState, a_offAccessLast + (a_cElements) * (a_cbType)); \
    } while (0)

#define CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(a_pState, a_offAccessLast, a_Type) CHECK_BUFFER_SIZE_STATIC_UPDATE(a_pState, (a_offAccessLast) + sizeof( a_Type ))

#define CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(a_pState, a_pArrayStart, a_cElements, a_Type) \
    CHECK_ARRAY_SIZE_STATIC_UPDATE(a_pState, ((const uint8_t *)a_pArrayStart - (a_pState)->pbUnpackData), a_cElements, sizeof(a_Type))

#define CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_SZ_LAST(a_pState, a_pArrayStart, a_cElements, a_cbType) \
    CHECK_ARRAY_SIZE_STATIC_UPDATE(a_pState, ((const uint8_t *)a_pArrayStart - (a_pState)->pbUnpackData), a_cElements, a_cbType)


DECLINLINE(size_t) crUnpackAcccessChkStrUpdate(PCrUnpackerState pState, const char *psz, size_t *pcbVerified)
{
    size_t cchStr = 0;
    size_t cbChkMax = pState->cbUnpackDataLeft - ((const uint8_t *)psz - pState->pbUnpackData);
    void *pv = memchr((void *)psz, '\0', cbChkMax);
    if (!pv)
    {
        pState->rcUnpack = VERR_BUFFER_OVERFLOW;
        AssertFailed();
        return ~(size_t)0;
    }

    cchStr = (uint8_t *)pv - pState->pbUnpackData + 1;
    *pcbVerified = cchStr;

    return cchStr;
}

#define CHECK_STRING_FROM_PTR_UPDATE_NO_RETURN(a_pState, a_pszStr) crUnpackAcccessChkStrUpdate((a_pState), (a_pszStr), &int_cbAccessVerified);

#define CHECK_STRING_FROM_PTR_UPDATE_NO_SZ(a_pState, a_pszStr) \
    do \
    { \
        size_t cchStr = crUnpackAcccessChkStrUpdate((a_pState), (a_pszStr), &int_cbAccessVerified); \
        if (RT_UNLIKELY(cchStr == ~(size_t)0)) \
            return; \
    } \
    while (0)

/**
 * Reads data at the given offset of the given type from the supplied data buffer.
 */
#define READ_DATA(a_pState, offset, type ) *(const type *)crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), sizeof( type ))

#ifdef CR_UNALIGNED_ACCESS_OKAY
#define READ_DOUBLE(a_pState, offset ) READ_DATA(a_pState, offset, GLdouble )
#else
#define READ_DOUBLE(a_pState, offset ) crReadUnalignedDouble(crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), sizeof( GLdouble ) ))
#endif

#define READ_NETWORK_POINTER(a_pState, offset ) (uint8_t *)crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), 0)
#define DATA_POINTER(a_pState, offset, type )   ((type *) (crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), 0)) )

#define DATA_POINTER_CHECK_SIZE(a_pState, a_cbAccess )   ((size_t)(a_cbAccess) <= (a_pState)->cbUnpackDataLeft)

#define INCR_DATA_PTR(a_pState, delta ) \
    do \
    { \
        size_t a_cbAdv = (delta); \
        if (RT_UNLIKELY((a_pState)->cbUnpackDataLeft < a_cbAdv)) \
        { \
          (a_pState)->rcUnpack = VERR_BUFFER_OVERFLOW; \
          AssertFailed(); \
          return; \
        } \
        (a_pState)->pbUnpackData     += a_cbAdv; \
        (a_pState)->cbUnpackDataLeft -= a_cbAdv; \
    } while(0)

#define INCR_DATA_PTR_NO_ARGS(a_pState) INCR_DATA_PTR(a_pState, 4 )

#define INCR_VAR_PTR(a_pState) INCR_DATA_PTR(a_pState, *((int *)(a_pState)->pbUnpackData) )

#define SET_RETURN_PTR(a_pState, offset ) \
    do \
    { \
        CRDBGPTR_CHECKZ((a_pState)->pReturnPtr); \
        if (offset + sizeof(*(a_pState)->pReturnPtr) > (a_pState)->cbUnpackDataLeft) \
        { \
             crError("%s: SET_RETURN_PTR(%u) offset out of bounds\n", __FUNCTION__, offset); \
             return; \
        } \
        crMemcpy((a_pState)->pReturnPtr, crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), sizeof( *(a_pState)->pReturnPtr )), sizeof( *(a_pState)->pReturnPtr ) ); \
    } while (0)

#define SET_WRITEBACK_PTR(a_pState, offset ) \
    do \
    { \
        CRDBGPTR_CHECKZ((a_pState)->pWritebackPtr); \
        if (offset + sizeof(*(a_pState)->pWritebackPtr) > (a_pState)->cbUnpackDataLeft) \
        { \
             crError("%s: SET_WRITEBACK_PTR(%u) offset out of bounds\n", __FUNCTION__, offset); \
             return; \
        } \
        crMemcpy((a_pState)->pWritebackPtr, crUnpackAccessChk((a_pState), int_cbAccessVerified, (offset), sizeof( *(a_pState)->pWritebackPtr )), sizeof( *(a_pState)->pWritebackPtr) ); \
    } while (0);

#ifdef __cplusplus
}
#endif

#endif /* CR_UNPACK_H */
