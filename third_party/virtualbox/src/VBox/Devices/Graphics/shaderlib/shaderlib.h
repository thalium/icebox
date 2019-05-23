/* $Id: shaderlib.h $ */
/** @file
 * shaderlib -- interface to WINE's Direct3D shader functions
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#ifndef ___shaderlib_h___
#define ___shaderlib_h___

#include <VBox/cdefs.h>

RT_C_DECLS_BEGIN

#ifdef IN_SHADERLIB_STATIC
# define SHADERDECL(type)           DECLHIDDEN(type) RTCALL
#else
# define SHADERDECL(type)           DECLEXPORT(type) RTCALL
#endif

/** Pointer to shaderlib callback interface. */
typedef struct VBOXVMSVGASHADERIF *PVBOXVMSVGASHADERIF;
/**
 * Interface the shader lib can use to talk back to the VBox VMSVGA OGL 3D code.
 */
typedef struct VBOXVMSVGASHADERIF
{
    /**
     * Switches the initialization profile in builds where we have to juggle two
     * OpenGL profiles to gather all the data (i.e. mac os x).
     *
     * @param   pThis           Pointer to this structure.
     * @param   fOtherProfile   If set, switch to the non-default profile.  If
     *                          clear, switch back to the default profile.
     */
    DECLCALLBACKMEMBER(void, pfnSwitchInitProfile)(PVBOXVMSVGASHADERIF pThis, bool fOtherProfile);

    /**
     * Extension enumeration function.
     *
     * @param   pThis           Pointer to this structure.
     * @param   ppvEnumCtx      Pointer to a void point that's initialized to NULL
     *                          before the first call.
     * @param   pszBuf          Where to store the extension name. Garbled on
     *                          overflow (we assume no overflow).
     * @param   cbBuf           The size of the buffer @a pszBuf points to.
     * @param   fOtherProfile   Indicates which profile to get extensions from,
     *                          we'll use the default profile if CLEAR and the
     *                          non-default if SET.
     */
    DECLCALLBACKMEMBER(bool, pfnGetNextExtension)(PVBOXVMSVGASHADERIF pThis, void **ppvEnumCtx, char *pszBuf, size_t cbBuf,
                                                  bool fOtherProfile);
} VBOXVMSVGASHADERIF;

SHADERDECL(int) ShaderInitLib(PVBOXVMSVGASHADERIF pVBoxShaderIf);
SHADERDECL(int) ShaderDestroyLib(void);

SHADERDECL(int) ShaderContextCreate(void **ppShaderContext);
SHADERDECL(int) ShaderContextDestroy(void *pShaderContext);

SHADERDECL(int) ShaderCreateVertexShader(void *pShaderContext, const uint32_t *pShaderData, void **pShaderObj);
SHADERDECL(int) ShaderCreatePixelShader(void *pShaderContext, const uint32_t *pShaderData, void **pShaderObj);

SHADERDECL(int) ShaderDestroyVertexShader(void *pShaderContext, void *pShaderObj);
SHADERDECL(int) ShaderDestroyPixelShader(void *pShaderContext, void *pShaderObj);

SHADERDECL(int) ShaderSetVertexShader(void *pShaderContext, void *pShaderObj);
SHADERDECL(int) ShaderSetPixelShader(void *pShaderContext, void *pShaderObj);

SHADERDECL(int) ShaderSetVertexShaderConstantB(void *pShaderContext, uint32_t reg, const uint8_t *pValues, uint32_t cRegisters);
SHADERDECL(int) ShaderSetVertexShaderConstantI(void *pShaderContext, uint32_t reg, const int32_t *pValues, uint32_t cRegisters);
SHADERDECL(int) ShaderSetVertexShaderConstantF(void *pShaderContext, uint32_t reg, const float *pValues, uint32_t cRegisters);

SHADERDECL(int) ShaderSetPixelShaderConstantB(void *pShaderContext, uint32_t reg, const uint8_t *pValues, uint32_t cRegisters);
SHADERDECL(int) ShaderSetPixelShaderConstantI(void *pShaderContext, uint32_t reg, const int32_t *pValues, uint32_t cRegisters);
SHADERDECL(int) ShaderSetPixelShaderConstantF(void *pShaderContext, uint32_t reg, const float *pValues, uint32_t cRegisters);

SHADERDECL(int) ShaderSetPositionTransformed(void *pShaderContext, unsigned cxViewPort, unsigned cyViewPort, bool fPreTransformed);

SHADERDECL(int) ShaderUpdateState(void *pShaderContext, uint32_t rtHeight);

SHADERDECL(int) ShaderTransformProjection(unsigned cxViewPort, unsigned cyViewPort, float matrix[16], bool fPretransformed);

RT_C_DECLS_END

#endif /* !___shaderlib_h___ */

