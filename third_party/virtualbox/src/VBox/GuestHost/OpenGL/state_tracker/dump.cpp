/* $Id: dump.cpp $ */

/** @file
 * Blitter API implementation
 */
/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#include "cr_blitter.h"
#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_net.h"
#include "cr_rand.h"
#include "cr_mem.h"
#include "cr_string.h"
#include <cr_dump.h>
#include "cr_pixeldata.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/mem.h>

#include <stdio.h>

#ifdef VBOX_WITH_CRDUMPER

static uint32_t g_CrDbgDumpRecTexInfo = 1;
static uint32_t g_CrDbgDumpAlphaData = 1;

/* dump stuff */
#pragma pack(1)
typedef struct VBOX_BITMAPFILEHEADER {
        uint16_t    bfType;
        uint32_t   bfSize;
        uint16_t    bfReserved1;
        uint16_t    bfReserved2;
        uint32_t   bfOffBits;
} VBOX_BITMAPFILEHEADER;

typedef struct VBOX_BITMAPINFOHEADER {
        uint32_t      biSize;
        int32_t       biWidth;
        int32_t       biHeight;
        uint16_t       biPlanes;
        uint16_t       biBitCount;
        uint32_t      biCompression;
        uint32_t      biSizeImage;
        int32_t       biXPelsPerMeter;
        int32_t       biYPelsPerMeter;
        uint32_t      biClrUsed;
        uint32_t      biClrImportant;
} VBOX_BITMAPINFOHEADER;
#pragma pack()

void crDmpImgBmp(CR_BLITTER_IMG *pImg, const char *pszFilename)
{
    static int sIdx = 0;

    if (   pImg->bpp != 16
        && pImg->bpp != 24
        && pImg->bpp != 32)
    {
        crWarning("not supported bpp %d", pImg->bpp);
        return;
    }

    FILE *f = fopen (pszFilename, "wb");
    if (!f)
    {
        crWarning("fopen failed");
        return;
    }

    VBOX_BITMAPFILEHEADER bf;

    bf.bfType = 'MB';
    bf.bfSize = sizeof (VBOX_BITMAPFILEHEADER) + sizeof (VBOX_BITMAPINFOHEADER) + pImg->cbData;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    bf.bfOffBits = sizeof (VBOX_BITMAPFILEHEADER) + sizeof (VBOX_BITMAPINFOHEADER);

    VBOX_BITMAPINFOHEADER bi;

    bi.biSize = sizeof (bi);
    bi.biWidth = pImg->width;
    bi.biHeight = pImg->height;
    bi.biPlanes = 1;
    bi.biBitCount = pImg->bpp;
    bi.biCompression = 0;
    bi.biSizeImage = pImg->cbData;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    fwrite (&bf, 1, sizeof (bf), f);
    fwrite (&bi, 1, sizeof (bi), f);
    fwrite (pImg->pvData, 1, pImg->cbData, f);

    fclose (f);
}

typedef struct CRDUMPGETHWID_DATA
{
    GLuint hwid;
    PFNCRDUMPGETHWID pfnGetHwid;
    unsigned long Key;
    void* pvObj;
} CRDUMPGETHWID_DATA;

static void crDmpHashtableSearchByHwidCB(unsigned long key, void *pData1, void *pData2)
{
    CRDUMPGETHWID_DATA *pData = (CRDUMPGETHWID_DATA*)pData2;
    if (pData->pvObj)
        return;

    if (pData->hwid == pData->pfnGetHwid(pData1))
    {
        pData->Key = key;
        pData->pvObj = pData1;
    }
}

void* crDmpHashtableSearchByHwid(CRHashTable *pHash, GLuint hwid, PFNCRDUMPGETHWID pfnGetHwid, unsigned long *pKey)
{
    CRDUMPGETHWID_DATA Data = {0};
    Data.hwid = hwid;
    Data.pfnGetHwid = pfnGetHwid;
    crHashtableWalk(pHash, crDmpHashtableSearchByHwidCB, &Data);

    Assert(Data.pvObj);

    if (pKey)
        *pKey = Data.Key;
    return Data.pvObj;
}

#if 0
typedef struct CR_SERVER_DUMP_FIND_TEX
{
    GLint hwid;
    CRTextureObj *pTobj
} CR_SERVER_DUMP_FIND_TEX;

void crServerDumpFindTexCb(unsigned long key, void *pData1, void *pData2)
{
    CR_SERVER_DUMP_FIND_TEX *pTex = (CR_SERVER_DUMP_FIND_TEX*)pData2;
    CRTextureObj *pTobj = (CRTextureObj *)pData1;
    if (pTobj->hwid == pTex->hwid)
        pTex->pTobj = pTobj;
}
#endif

#define CR_DUMP_MAKE_CASE(_val) case _val: return #_val
#define CR_DUMP_MAKE_CASE_UNKNOWN(_val, _str, _pDumper) default: { \
    crWarning("%s %d", (_str), _val); \
    crDmpStrF((_pDumper), "WARNING: %s %d", (_str), _val); \
    return (_str); \
}

DECLINLINE(size_t) crDmpFormatVal(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbVal, const void *pvVal)
{
    if (pszElFormat[0] != '%' || pszElFormat[1] == '\0')
    {
        crWarning("invalid format %s", pszElFormat);
        return 0;
    }
    switch (cbVal)
    {
        case 8:
            return sprintf_s(pString, cbString, pszElFormat, *((double*)pvVal));
        case 4:
        {
            /* we do not care only about type specifiers, all the rest is not accepted */
            switch (pszElFormat[1])
            {
                case 'f':
                    /* float would be promoted to double */
                    return sprintf_s(pString, cbString, pszElFormat, *((float*)pvVal));
                default:
                    return sprintf_s(pString, cbString, pszElFormat, *((uint32_t*)pvVal));
            }
        }
        case 2:
            return sprintf_s(pString, cbString, pszElFormat, *((uint16_t*)pvVal));
        case 1:
            return sprintf_s(pString, cbString, pszElFormat, *((uint8_t*)pvVal));
        default:
            crWarning("unsupported size %d", cbVal);
            return 0;
    }
}

VBOXDUMPDECL(size_t) crDmpFormatRawArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cVal)
{
    if (cbString < 2)
    {
        crWarning("too few buffer size");
        return 0;
    }

    const size_t cbInitString = cbString;
    *pString++ = '{';
    --cbString;
    size_t cbWritten;
    const uint8_t *pu8Val = (const uint8_t *)pvVal;
    for (uint32_t i = 0; i < cVal; ++i)
    {
        cbWritten = crDmpFormatVal(pString, cbString, pszElFormat, cbEl, (const void *)pu8Val);
        pu8Val += cbEl;
        Assert(cbString >= cbWritten);
        pString += cbWritten;
        cbString -= cbWritten;
        if (i != cVal - 1)
        {
            cbWritten = sprintf_s(pString, cbString, ", ");
            Assert(cbString >= cbWritten);
            pString += cbWritten;
            cbString -= cbWritten;
        }
    }

    if (!cbString)
    {
        crWarning("too few buffer size");
        return 0;
    }
    *pString++ = '}';
    --cbString;

    if (!cbString)
    {
        crWarning("too few buffer size");
        return 0;
    }
    *pString++ = '\0';

    return cbInitString - cbString;
}

VBOXDUMPDECL(size_t) crDmpFormatMatrixArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cX, uint32_t cY)
{
    if (cbString < 2)
    {
        crWarning("too few buffer size");
        return 0;
    }

    const size_t cbInitString = cbString;
    *pString++ = '{';
    --cbString;
    size_t cbWritten;
    const uint8_t *pu8Val = (const uint8_t *)pvVal;
    for (uint32_t i = 0; i < cY; ++i)
    {
        cbWritten = crDmpFormatRawArray(pString, cbString, pszElFormat, cbEl, (const void *)pu8Val, cX);
        pu8Val += (cbEl * cX);
        Assert(cbString >= cbWritten);
        pString += cbWritten;
        cbString -= cbWritten;
        if (i != cY - 1)
        {
            if (cbString < 3)
            {
                crWarning("too few buffer size");
                return 0;
            }
            *pString++ = ',';
            --cbString;
            *pString++ = '\n';
            --cbString;
        }
    }
    if (!cbString)
    {
        crWarning("too few buffer size");
        return 0;
    }
    *pString++ = '}';
    --cbString;

    if (!cbString)
    {
        crWarning("too few buffer size");
        return 0;
    }
    *pString++ = '\0';

    return cbInitString - cbString;
}

VBOXDUMPDECL(size_t) crDmpFormatArray(char *pString, size_t cbString, const char *pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cVal)
{
    switch(cVal)
    {
        case 1:
            return crDmpFormatVal(pString, cbString, pszElFormat, cbEl, pvVal);
        case 16:
            return crDmpFormatMatrixArray(pString, cbString, pszElFormat, cbEl, pvVal, 4, 4);
        case 9:
            return crDmpFormatMatrixArray(pString, cbString, pszElFormat, cbEl, pvVal, 3, 3);
        case 0:
            crWarning("value array is empty");
            return 0;
        default:
            return crDmpFormatRawArray(pString, cbString, pszElFormat, cbEl, pvVal, cVal);
    }
}

VBOXDUMPDECL(void) crRecDumpVertAttrv(CR_RECORDER *pRec, CRContext *ctx, GLuint idx, const char*pszElFormat, uint32_t cbEl, const void *pvVal, uint32_t cVal)
{
    char aBuf[1024];
    crDmpFormatRawArray(aBuf, sizeof (aBuf), pszElFormat, cbEl, pvVal, cVal);
    crDmpStrF(pRec->pDumper, "(%u, %s)", idx, aBuf);
}

VBOXDUMPDECL(void) crRecDumpVertAttrV(CR_RECORDER *pRec, CRContext *ctx, const char*pszFormat, va_list pArgList)
{
    crDmpStrV(pRec->pDumper, pszFormat, pArgList);
}

VBOXDUMPDECL(void) crRecDumpVertAttrF(CR_RECORDER *pRec, CRContext *ctx, const char*pszFormat, ...)
{
    va_list pArgList;
    va_start(pArgList, pszFormat);
    crRecDumpVertAttrV(pRec, ctx, pszFormat, pArgList);
    va_end(pArgList);
}

void crRecDumpBuffer(CR_RECORDER *pRec, CRContext *ctx, GLint idRedirFBO, VBOXVR_TEXTURE *pRedirTex)
{
    GLenum texTarget = 0;
    GLint hwBuf = 0, hwDrawBuf = 0;
    GLint hwTex = 0, hwObjType = 0, hwTexLevel = 0, hwCubeFace = 0;
    GLint width = 0, height = 0, depth = 0;
    GLint id = 0;
    CR_BLITTER_IMG Img = {0};
    VBOXVR_TEXTURE Tex;
    int rc;

    pRec->pDispatch->GetIntegerv(GL_DRAW_BUFFER, &hwDrawBuf);
    pRec->pDispatch->GetIntegerv(GL_FRAMEBUFFER_BINDING, &hwBuf);
    if (hwBuf)
    {
        pRec->pDispatch->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, hwDrawBuf, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &hwTex);
        pRec->pDispatch->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, hwDrawBuf, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &hwObjType);
        if (hwObjType == GL_TEXTURE)
        {
            pRec->pDispatch->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, hwDrawBuf, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &hwTexLevel);
            pRec->pDispatch->GetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, hwDrawBuf, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &hwCubeFace);
            if (hwCubeFace)
            {
                crWarning("cube face: unsupported");
                return;
            }

            if (hwTexLevel)
            {
                crWarning("non-zero tex level attached, unsupported");
                return;
            }
        }
        else
        {
            crWarning("unsupported");
            return;
        }
    }
    else
    {
        crWarning("no buffer attached: unsupported");
        return;
    }

    if (ctx->framebufferobject.drawFB)
    {
        GLuint iColor = (hwDrawBuf - GL_COLOR_ATTACHMENT0_EXT);
        CRTextureObj *pTobj = (CRTextureObj *)crHashtableSearch(ctx->shared->textureTable, ctx->framebufferobject.drawFB->color[iColor].name);
        CRTextureLevel *pTl = NULL;

        id = pTobj->id;

        Assert(iColor < RT_ELEMENTS(ctx->framebufferobject.drawFB->color));

        if (!pTobj)
        {
            crWarning("no tobj");
            return;
        }
        Assert(pTobj->hwid == hwTex);
        Assert(pTobj);
        Assert(ctx->framebufferobject.drawFB->hwid);
        Assert(ctx->framebufferobject.drawFB->hwid == hwBuf);
        Assert(ctx->framebufferobject.drawFB->drawbuffer[0] == hwDrawBuf);

        Assert(ctx->framebufferobject.drawFB->color[iColor].level == hwTexLevel);
        Assert(ctx->framebufferobject.drawFB->color[iColor].type == hwObjType);

        texTarget = pTobj->target;

        Assert(texTarget == GL_TEXTURE_2D);

        pTl = &pTobj->level[0][hwTexLevel];

        rc = CrBltEnter(pRec->pBlitter);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrBltEnter failed, rc %d", rc);
            return;
        }

        pRec->pDispatch->BindTexture(texTarget, hwTex);

        pRec->pDispatch->GetTexLevelParameteriv(texTarget, hwTexLevel, GL_TEXTURE_WIDTH, &width);
        pRec->pDispatch->GetTexLevelParameteriv(texTarget, hwTexLevel, GL_TEXTURE_HEIGHT, &height);
        pRec->pDispatch->GetTexLevelParameteriv(texTarget, hwTexLevel, GL_TEXTURE_DEPTH, &depth);

        Assert(width == pTl->width);
        Assert(height == pTl->height);
        Assert(depth == pTl->depth);

        pRec->pDispatch->BindTexture(texTarget, 0);
    }
    else
    {
        Assert(hwBuf == idRedirFBO);
        if (!pRedirTex)
        {
            crWarning("pRedirTex is expected for non-FBO state!");
            return;
        }

        Assert(hwTex == pRedirTex->hwid);

        texTarget = pRedirTex->target;

        width = pRedirTex->width;
        height = pRedirTex->height;

        rc = CrBltEnter(pRec->pBlitter);
        if (!RT_SUCCESS(rc))
        {
            crWarning("CrBltEnter failed, rc %d", rc);
            return;
        }
    }

    Tex.width = width;
    Tex.height = height;
    Tex.target = texTarget;
    Tex.hwid = hwTex;

    rc = CrBltImgGetTex(pRec->pBlitter, &Tex, GL_BGRA, &Img);
    if (RT_SUCCESS(rc))
    {
        crDmpImgF(pRec->pDumper, &Img, "ctx(%d), BUFFER: id(%d) hwid(%d), width(%d), height(%d)", ctx, id, Tex.hwid, width, height);

        if (g_CrDbgDumpAlphaData)
        {
            CR_BLITTER_IMG AlphaImg = {0};
            rc = crRecAlphaImgCreate(&Img, &AlphaImg);
            if (RT_SUCCESS(rc))
            {
                crDmpImgF(pRec->pDumper, &AlphaImg, "Buffer ALPHA Data");
                crRecAlphaImgDestroy(&AlphaImg);
            }
            else
            {
                crWarning("crRecAlphaImgCreate failed rc %d", rc);
            }
        }

        CrBltImgFree(pRec->pBlitter, &Img);
    }
    else
    {
        crWarning("CrBltImgGetTex failed, rc %d", rc);
    }

    CrBltLeave(pRec->pBlitter);
}

static const char *crRecDumpShaderTypeString(GLenum enmType, CR_DUMPER *pDumper)
{
    switch (enmType)
    {
        CR_DUMP_MAKE_CASE(GL_VERTEX_SHADER_ARB);
        CR_DUMP_MAKE_CASE(GL_FRAGMENT_SHADER_ARB);
        CR_DUMP_MAKE_CASE(GL_GEOMETRY_SHADER_ARB);
        CR_DUMP_MAKE_CASE_UNKNOWN(enmType, "Unknown Shader Type", pDumper);
    }
}

static const char *crRecDumpVarTypeString(GLenum enmType, CR_DUMPER *pDumper)
{
    switch (enmType)
    {
        CR_DUMP_MAKE_CASE(GL_BYTE);
        CR_DUMP_MAKE_CASE(GL_UNSIGNED_BYTE);
        CR_DUMP_MAKE_CASE(GL_SHORT);
        CR_DUMP_MAKE_CASE(GL_UNSIGNED_SHORT);
        CR_DUMP_MAKE_CASE(GL_FLOAT);
        CR_DUMP_MAKE_CASE(GL_DOUBLE);
        CR_DUMP_MAKE_CASE(GL_FLOAT_VEC2);
        CR_DUMP_MAKE_CASE(GL_FLOAT_VEC3);
        CR_DUMP_MAKE_CASE(GL_FLOAT_VEC4);
        CR_DUMP_MAKE_CASE(GL_INT);
        CR_DUMP_MAKE_CASE(GL_UNSIGNED_INT);
        CR_DUMP_MAKE_CASE(GL_INT_VEC2);
        CR_DUMP_MAKE_CASE(GL_INT_VEC3);
        CR_DUMP_MAKE_CASE(GL_INT_VEC4);
        CR_DUMP_MAKE_CASE(GL_BOOL);
        CR_DUMP_MAKE_CASE(GL_BOOL_VEC2);
        CR_DUMP_MAKE_CASE(GL_BOOL_VEC3);
        CR_DUMP_MAKE_CASE(GL_BOOL_VEC4);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT2);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT3);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT4);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_1D);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_2D);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_3D);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_CUBE);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_1D_SHADOW);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_2D_SHADOW);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_2D_RECT_ARB);
        CR_DUMP_MAKE_CASE(GL_SAMPLER_2D_RECT_SHADOW_ARB);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT2x3);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT2x4);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT3x2);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT3x4);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT4x2);
        CR_DUMP_MAKE_CASE(GL_FLOAT_MAT4x3);
        CR_DUMP_MAKE_CASE_UNKNOWN(enmType, "Unknown Variable Type", pDumper);
    }
}

static char *crRecDumpGetLine(char **ppszStr, uint32_t *pcbStr)
{
    char *pszStr, *pNewLine;
    const uint32_t cbStr = *pcbStr;

    if (!cbStr)
    {
        /* zero-length string */
        return NULL;
    }

    if ((*ppszStr)[cbStr-1] != '\0')
    {
        crWarning("string should be null-rerminated, forcing it!");
        (*ppszStr)[cbStr-1] = '\0';
    }
    pszStr = *ppszStr;
    if (!*pszStr)
    {
        *pcbStr = 0;
        return NULL;
    }

    if (!(pNewLine = strstr(pszStr, "\n")))
    {
        /* the string contains a single line! */
        *ppszStr += strlen(pszStr);
        *pcbStr = 0;
        return pszStr;
    }

    *pNewLine = '\0';
    *pcbStr = cbStr - (((uintptr_t)pNewLine) - ((uintptr_t)pszStr)) - 1;
    Assert((*pcbStr) < UINT32_MAX/2);
    Assert((*pcbStr) < cbStr);
    *ppszStr = pNewLine + 1;

    return pszStr;
}

static void crRecDumpStrByLine(CR_DUMPER *pDumper, char *pszStr, uint32_t cbStr)
{
    char *pszCurLine;
    while ((pszCurLine = crRecDumpGetLine(&pszStr, &cbStr)) != NULL)
    {
        crDmpStrF(pDumper, "%s", pszCurLine);
    }
}

static DECLCALLBACK(GLuint) crDmpGetHwidShaderCB(void *pvObj)
{
    return ((CRGLSLShader*)pvObj)->hwid;
}

static DECLCALLBACK(GLuint) crDmpGetHwidProgramCB(void *pvObj)
{
    return ((CRGLSLProgram*)pvObj)->hwid;
}

/* Context activation is done by the caller. */
void crRecDumpLog(CR_RECORDER *pRec, GLint hwid)
{
    GLint cbLog = 0;
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_INFO_LOG_LENGTH_ARB, &cbLog);

    crDmpStrF(pRec->pDumper, "Log===%d===", hwid);

    if (cbLog > 1)
    {
        GLchar *pszLog = (GLchar *) crAlloc(cbLog*sizeof (GLchar));

        pRec->pDispatch->GetInfoLogARB(hwid, cbLog, NULL, pszLog);

        crRecDumpStrByLine(pRec->pDumper, pszLog, cbLog);

        crFree(pszLog);
    }
    crDmpStrF(pRec->pDumper, "End Log======");
}

void crRecDumpShader(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    GLint length = 0;
    GLint type = 0;
    GLint compileStatus = 0;

#ifndef IN_GUEST
    CRGLSLShader *pShad;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pShad = (CRGLSLShader *)crDmpHashtableSearchByHwid(ctx->glsl.shaders, hwid, crDmpGetHwidShaderCB, &tstKey);
        Assert(pShad);
        if (!pShad)
            return;
        id = pShad->id;
        Assert(tstKey == id);
    }
    else
    {
        pShad = (CRGLSLShader *)crHashtableSearch(ctx->glsl.shaders, id);
        Assert(pShad);
        if (!pShad)
            return;
    }

    if (!hwid)
        hwid = pShad->hwid;

    Assert(pShad->hwid == hwid);
    Assert(pShad->id == id);
#else
    if (!id)
        id = hwid;
    else if (!hwid)
        hwid = id;

    Assert(id);
    Assert(hwid);
    Assert(hwid == id);
#endif

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_SUBTYPE_ARB, &type);
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_COMPILE_STATUS_ARB, &compileStatus);
    crDmpStrF(pRec->pDumper, "SHADER ctx(%d) id(%d) hwid(%d) type(%s) status(%d):", ctx->id, id, hwid, crRecDumpShaderTypeString(type, pRec->pDumper), compileStatus);

    crRecDumpLog(pRec, hwid);

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &length);

    char *pszSource = (char*)crCalloc(length + 1);
    if (!pszSource)
    {
        crWarning("crCalloc failed");
        crDmpStrF(pRec->pDumper, "WARNING: crCalloc failed");
        return;
    }

    pRec->pDispatch->GetShaderSource(hwid, length, NULL, pszSource);
    crRecDumpStrByLine(pRec->pDumper, pszSource, length);

    crFree(pszSource);

    crDmpStr(pRec->pDumper, "===END SHADER====");
}

void crRecDumpProgram(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    GLint cShaders = 0, linkStatus = 0;
    char *source = NULL;
    CRGLSLProgram *pProg;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pProg = (CRGLSLProgram*)crDmpHashtableSearchByHwid(ctx->glsl.programs, hwid, crDmpGetHwidProgramCB, &tstKey);
        Assert(pProg);
        if (!pProg)
            return;
        id = pProg->id;
        Assert(tstKey == id);
    }
    else
    {
        pProg = (CRGLSLProgram *) crHashtableSearch(ctx->glsl.programs, id);
        Assert(pProg);
        if (!pProg)
            return;
    }

    if (!hwid)
        hwid = pProg->hwid;

    Assert(pProg->hwid == hwid);
    Assert(pProg->id == id);

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_ATTACHED_OBJECTS_ARB, &cShaders);
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);

    crDmpStrF(pRec->pDumper, "PROGRAM ctx(%d) id(%d) hwid(%d) status(%d) shaders(%d):", ctx->id, id, hwid, linkStatus, cShaders);

    crRecDumpLog(pRec, hwid);

    VBoxGLhandleARB *pShaders = (VBoxGLhandleARB*)crCalloc(cShaders * sizeof (*pShaders));
    if (!pShaders)
    {
        crWarning("crCalloc failed");
        crDmpStrF(pRec->pDumper, "WARNING: crCalloc failed");
        return;
    }

    pRec->pDispatch->GetAttachedObjectsARB(hwid, cShaders, NULL, pShaders);
    for (GLint i = 0; i < cShaders; ++i)
    {
        if (pShaders[i])
            crRecDumpShader(pRec, ctx, 0, pShaders[i]);
        else
            crDmpStrF(pRec->pDumper, "WARNING: Shader[%d] is null", i);
    }

    crFree(pShaders);

    GLsizei cbLog = 0;

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_INFO_LOG_LENGTH_ARB, &cbLog);
    if (cbLog)
    {
        char *pszLog = (char *)crCalloc(cbLog+1);
        pRec->pDispatch->GetInfoLogARB(hwid, cbLog, NULL, pszLog);
        crDmpStrF(pRec->pDumper, "==LOG==");
        crRecDumpStrByLine(pRec->pDumper, pszLog, cbLog);
        crDmpStrF(pRec->pDumper, "==Done LOG==");
        crFree(pszLog);
    }
    else
    {
        crDmpStrF(pRec->pDumper, "==No LOG==");
    }

    crDmpStr(pRec->pDumper, "===END PROGRAM====");
}

void crRecRecompileShader(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    GLint length = 0;
    GLint type = 0;
    GLint compileStatus = 0;
    CRGLSLShader *pShad;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pShad = (CRGLSLShader *)crDmpHashtableSearchByHwid(ctx->glsl.shaders, hwid, crDmpGetHwidShaderCB, &tstKey);
        Assert(pShad);
        if (!pShad)
            return;
        id = pShad->id;
        Assert(tstKey == id);
    }
    else
    {
        pShad = (CRGLSLShader *)crHashtableSearch(ctx->glsl.shaders, id);
        Assert(pShad);
        if (!pShad)
            return;
    }

    if (!hwid)
        hwid = pShad->hwid;

    Assert(pShad->hwid == hwid);
    Assert(pShad->id == id);

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_SUBTYPE_ARB, &type);
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_COMPILE_STATUS_ARB, &compileStatus);
    crDmpStrF(pRec->pDumper, "==RECOMPILE SHADER ctx(%d) id(%d) hwid(%d) type(%s) status(%d)==", ctx->id, id, hwid, crRecDumpShaderTypeString(type, pRec->pDumper), compileStatus);

    compileStatus = 0;
    GLenum status;
    while ((status = pRec->pDispatch->GetError()) != GL_NO_ERROR) {/*Assert(0);*/}
    pRec->pDispatch->CompileShader(hwid);
    while ((status = pRec->pDispatch->GetError()) != GL_NO_ERROR) {Assert(0);}
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_COMPILE_STATUS_ARB, &compileStatus);

    crDmpStrF(pRec->pDumper, "==Done RECOMPILE SHADER, status(%d)==", compileStatus);
}

void crRecRecompileProgram(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    GLint cShaders = 0, linkStatus = 0;
    char *source = NULL;
    CRGLSLProgram *pProg;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pProg = (CRGLSLProgram*)crDmpHashtableSearchByHwid(ctx->glsl.programs, hwid, crDmpGetHwidProgramCB, &tstKey);
        Assert(pProg);
        if (!pProg)
            return;
        id = pProg->id;
        Assert(tstKey == id);
    }
    else
    {
        pProg = (CRGLSLProgram *) crHashtableSearch(ctx->glsl.programs, id);
        Assert(pProg);
        if (!pProg)
            return;
    }

    if (!hwid)
        hwid = pProg->hwid;

    Assert(pProg->hwid == hwid);
    Assert(pProg->id == id);

    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_ATTACHED_OBJECTS_ARB, &cShaders);
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);

    crDmpStrF(pRec->pDumper, "==RECOMPILE PROGRAM ctx(%d) id(%d) hwid(%d) status(%d) shaders(%d)==", ctx->id, id, hwid, linkStatus, cShaders);

    VBoxGLhandleARB *pShaders = (VBoxGLhandleARB*)crCalloc(cShaders * sizeof (*pShaders));
    if (!pShaders)
    {
        crWarning("crCalloc failed");
        crDmpStrF(pRec->pDumper, "WARNING: crCalloc failed");
        return;
    }

    pRec->pDispatch->GetAttachedObjectsARB(hwid, cShaders, NULL, pShaders);
    for (GLint i = 0; i < cShaders; ++i)
    {
        crRecRecompileShader(pRec, ctx, 0, pShaders[i]);
    }

    crFree(pShaders);

    linkStatus = 0;
    GLenum status;
    while ((status = pRec->pDispatch->GetError()) != GL_NO_ERROR) {/*Assert(0);*/}
    pRec->pDispatch->LinkProgram(hwid);
    while ((status = pRec->pDispatch->GetError()) != GL_NO_ERROR) {Assert(0);}
    pRec->pDispatch->GetObjectParameterivARB(hwid, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);

    crDmpStrF(pRec->pDumper, "==Done RECOMPILE PROGRAM, status(%d)==", linkStatus);
}

VBOXDUMPDECL(void) crRecDumpCurrentProgram(CR_RECORDER *pRec, CRContext *ctx)
{
    GLint curProgram = 0;
    pRec->pDispatch->GetIntegerv(GL_CURRENT_PROGRAM, &curProgram);
    if (curProgram)
    {
        Assert(ctx->glsl.activeProgram);
        if (!ctx->glsl.activeProgram)
            crWarning("no active program state with active hw program");
        else
            Assert(ctx->glsl.activeProgram->hwid == curProgram);
        crRecDumpProgram(pRec, ctx, 0, curProgram);
    }
    else
    {
        Assert(!ctx->glsl.activeProgram);
        crDmpStrF(pRec->pDumper, "--no active program");
    }
}

void crRecDumpProgramUniforms(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    CRGLSLProgram *pProg;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pProg = (CRGLSLProgram*)crDmpHashtableSearchByHwid(ctx->glsl.programs, hwid, crDmpGetHwidProgramCB, &tstKey);
        Assert(pProg);
        if (!pProg)
            return;
        id = pProg->id;
        Assert(tstKey == id);
    }
    else
    {
        pProg = (CRGLSLProgram *) crHashtableSearch(ctx->glsl.programs, id);
        Assert(pProg);
        if (!pProg)
            return;
    }

    if (!hwid)
        hwid = pProg->hwid;

    Assert(pProg->hwid == hwid);
    Assert(pProg->id == id);

    GLint maxUniformLen = 0, activeUniforms = 0, i, j, uniformsCount = 0;
    GLenum type;
    GLint size, location;
    GLchar *pszName = NULL;
    pRec->pDispatch->GetProgramiv(hwid, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);
    pRec->pDispatch->GetProgramiv(hwid, GL_ACTIVE_UNIFORMS, &activeUniforms);

    if (!maxUniformLen)
    {
        if (activeUniforms)
        {
            crWarning("activeUniforms (%d), while maxUniformLen is zero", activeUniforms);
            activeUniforms = 0;
        }
    }

    if (activeUniforms>0)
    {
        pszName = (GLchar *) crAlloc((maxUniformLen+8)*sizeof(GLchar));

        if (!pszName)
        {
            crWarning("crRecDumpProgramUniforms: out of memory");
            return;
        }
    }

    for (i=0; i<activeUniforms; ++i)
    {
        pRec->pDispatch->GetActiveUniform(hwid, i, maxUniformLen, NULL, &size, &type, pszName);
        uniformsCount += size;
    }
    Assert(uniformsCount>=activeUniforms);

    if (activeUniforms>0)
    {
        GLfloat fdata[16];
        GLint idata[16];
        char *pIndexStr=NULL;

        for (i=0; i<activeUniforms; ++i)
        {
            bool fPrintBraketsWithName = false;
            pRec->pDispatch->GetActiveUniform(hwid, i, maxUniformLen, NULL, &size, &type, pszName);

            if (size>1)
            {
                pIndexStr = crStrchr(pszName, '[');
                if (!pIndexStr)
                {
                    pIndexStr = pszName+crStrlen(pszName);
                    fPrintBraketsWithName = true;
                }
            }

            if (fPrintBraketsWithName)
            {
                crDmpStrF(pRec->pDumper, "%s %s[%d];", crRecDumpVarTypeString(type, pRec->pDumper), pszName, size);
                Assert(size > 1);
            }
            else
                crDmpStrF(pRec->pDumper, "%s %s;", crRecDumpVarTypeString(type, pRec->pDumper), pszName);

            GLint uniformTypeSize = crStateGetUniformSize(type);
            Assert(uniformTypeSize >= 1);

            for (j=0; j<size; ++j)
            {
                if (size>1)
                {
                    sprintf(pIndexStr, "[%i]", j);
                }
                location = pRec->pDispatch->GetUniformLocation(hwid, pszName);

                if (crStateIsIntUniform(type))
                {
                    pRec->pDispatch->GetUniformiv(hwid, location, &idata[0]);
                    switch (uniformTypeSize)
                    {
                        case 1:
                            crDmpStrF(pRec->pDumper, "%s = %d; //location %d", pszName, idata[0], location);
                            break;
                        case 2:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d}; //location %d", pszName, idata[0], idata[1], location);
                            break;
                        case 3:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d, %d}; //location %d", pszName, idata[0], idata[1], idata[2], location);
                            break;
                        case 4:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d, %d, %d}; //location %d", pszName, idata[0], idata[1], idata[2], idata[3], location);
                            break;
                        default:
                            for (GLint k = 0; k < uniformTypeSize; ++k)
                            {
                                crDmpStrF(pRec->pDumper, "%s[%d] = %d; //location %d", pszName, k, idata[k], location);
                            }
                            break;
                    }
                }
                else
                {
                    pRec->pDispatch->GetUniformfv(hwid, location, &fdata[0]);
                    switch (uniformTypeSize)
                    {
                        case 1:
                            crDmpStrF(pRec->pDumper, "%s = %f; //location %d", pszName, fdata[0], location);
                            break;
                        case 2:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f}; //location %d", pszName, fdata[0], fdata[1], location);
                            break;
                        case 3:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f, %f}; //location %d", pszName, fdata[0], fdata[1], fdata[2], location);
                            break;
                        case 4:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f, %f, %f}; //location %d", pszName, fdata[0], fdata[1], fdata[2], fdata[3], location);
                            break;
                        default:
                            for (GLint k = 0; k < uniformTypeSize; ++k)
                            {
                                crDmpStrF(pRec->pDumper, "%s[%d] = %f; //location %d", pszName, k, fdata[k], location);
                            }
                            break;
                    }
                }
            }
        }

        crFree(pszName);
    }
}

void crRecDumpProgramAttribs(CR_RECORDER *pRec, CRContext *ctx, GLint id, GLint hwid)
{
    CRGLSLProgram *pProg;

    if (!id)
    {
        unsigned long tstKey = 0;
        Assert(hwid);
        pProg = (CRGLSLProgram*)crDmpHashtableSearchByHwid(ctx->glsl.programs, hwid, crDmpGetHwidProgramCB, &tstKey);
        Assert(pProg);
        if (!pProg)
            return;
        id = pProg->id;
        Assert(tstKey == id);
    }
    else
    {
        pProg = (CRGLSLProgram *) crHashtableSearch(ctx->glsl.programs, id);
        Assert(pProg);
        if (!pProg)
            return;
    }

    if (!hwid)
        hwid = pProg->hwid;

    Assert(pProg->hwid == hwid);
    Assert(pProg->id == id);

    GLint maxAttribLen = 0, activeAttrib = 0, i, j, attribCount = 0;
    GLenum type;
    GLint size, location;
    GLchar *pszName = NULL;
    pRec->pDispatch->GetProgramiv(hwid, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttribLen);
    pRec->pDispatch->GetProgramiv(hwid, GL_ACTIVE_ATTRIBUTES, &activeAttrib);

    if (!maxAttribLen)
    {
        if (activeAttrib)
        {
            crWarning("activeAttrib (%d), while maxAttribLen is zero", activeAttrib);
            activeAttrib = 0;
        }
    }

    if (activeAttrib>0)
    {
        pszName = (GLchar *) crAlloc((maxAttribLen+8)*sizeof(GLchar));

        if (!pszName)
        {
            crWarning("crRecDumpProgramAttrib: out of memory");
            return;
        }
    }

    for (i=0; i<activeAttrib; ++i)
    {
        pRec->pDispatch->GetActiveAttrib(hwid, i, maxAttribLen, NULL, &size, &type, pszName);
        attribCount += size;
    }
    Assert(attribCount>=activeAttrib);

    if (activeAttrib>0)
    {
        GLfloat fdata[16];
        GLint idata[16];
        char *pIndexStr=NULL;

        for (i=0; i<activeAttrib; ++i)
        {
            bool fPrintBraketsWithName = false;
            pRec->pDispatch->GetActiveAttrib(hwid, i, maxAttribLen, NULL, &size, &type, pszName);
            GLint arrayBufferBind = 0, arrayEnabled = 0, arraySize = 0, arrayStride = 0, arrayType = 0, arrayNormalized = 0, arrayInteger = 0/*, arrayDivisor = 0*/;

            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &arrayBufferBind);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &arrayEnabled);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &arraySize);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &arrayStride);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &arrayType);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &arrayNormalized);
            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &arrayInteger);
//            pRec->pDispatch->GetVertexAttribivARB(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &arrayDivisor);

            if (size>1)
            {
                pIndexStr = crStrchr(pszName, '[');
                if (!pIndexStr)
                {
                    pIndexStr = pszName+crStrlen(pszName);
                    fPrintBraketsWithName = true;
                }
            }

            if (fPrintBraketsWithName)
            {
                crDmpStrF(pRec->pDumper, "%s %s[%d];", crRecDumpVarTypeString(type, pRec->pDumper), pszName, size);
                Assert(size > 1);
            }
            else
                crDmpStrF(pRec->pDumper, "%s %s;", crRecDumpVarTypeString(type, pRec->pDumper), pszName);

            crDmpStrF(pRec->pDumper, "Array buff(%d), enabled(%d) size(%d), stride(%d), type(%s), normalized(%d), integer(%d)", arrayBufferBind, arrayEnabled, arraySize, arrayStride, crRecDumpVarTypeString(arrayType, pRec->pDumper), arrayNormalized, arrayInteger);

            GLint attribTypeSize = crStateGetUniformSize(type);
            Assert(attribTypeSize >= 1);

            for (j=0; j<size; ++j)
            {
                if (size>1)
                {
                    sprintf(pIndexStr, "[%i]", j);
                }
                location = pRec->pDispatch->GetAttribLocation(hwid, pszName);

                if (crStateIsIntUniform(type))
                {
                    pRec->pDispatch->GetVertexAttribivARB(location, GL_CURRENT_VERTEX_ATTRIB, &idata[0]);
                    switch (attribTypeSize)
                    {
                        case 1:
                            crDmpStrF(pRec->pDumper, "%s = %d; //location %d", pszName, idata[0], location);
                            break;
                        case 2:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d}; //location %d", pszName, idata[0], idata[1], location);
                            break;
                        case 3:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d, %d}; //location %d", pszName, idata[0], idata[1], idata[2], location);
                            break;
                        case 4:
                            crDmpStrF(pRec->pDumper, "%s = {%d, %d, %d, %d}; //location %d", pszName, idata[0], idata[1], idata[2], idata[3], location);
                            break;
                        default:
                            for (GLint k = 0; k < attribTypeSize; ++k)
                            {
                                crDmpStrF(pRec->pDumper, "%s[%d] = %d; //location %d", pszName, k, idata[k], location);
                            }
                            break;
                    }
                }
                else
                {
                    pRec->pDispatch->GetVertexAttribfvARB(location, GL_CURRENT_VERTEX_ATTRIB, &fdata[0]);
                    switch (attribTypeSize)
                    {
                        case 1:
                            crDmpStrF(pRec->pDumper, "%s = %f; //location %d", pszName, fdata[0], location);
                            break;
                        case 2:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f}; //location %d", pszName, fdata[0], fdata[1], location);
                            break;
                        case 3:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f, %f}; //location %d", pszName, fdata[0], fdata[1], fdata[2], location);
                            break;
                        case 4:
                            crDmpStrF(pRec->pDumper, "%s = {%f, %f, %f, %f}; //location %d", pszName, fdata[0], fdata[1], fdata[2], fdata[3], location);
                            break;
                        default:
                            for (GLint k = 0; k < attribTypeSize; ++k)
                            {
                                crDmpStrF(pRec->pDumper, "%s[%d] = %f; //location %d", pszName, k, fdata[k], location);
                            }
                            break;
                    }
                }
            }
        }

        crFree(pszName);
    }
}

VBOXDUMPDECL(void) crRecDumpCurrentProgramUniforms(CR_RECORDER *pRec, CRContext *ctx)
{
    GLint curProgram = 0;
    pRec->pDispatch->GetIntegerv(GL_CURRENT_PROGRAM, &curProgram);
    if (curProgram)
    {
        Assert(ctx->glsl.activeProgram);
        if (!ctx->glsl.activeProgram)
            crWarning("no active program state with active hw program");
        else
            Assert(ctx->glsl.activeProgram->hwid == curProgram);
        crRecDumpProgramUniforms(pRec, ctx, 0, curProgram);
    }
    else
    {
        Assert(!ctx->glsl.activeProgram);
        crDmpStrF(pRec->pDumper, "--no active program");
    }
}

VBOXDUMPDECL(void) crRecDumpCurrentProgramAttribs(CR_RECORDER *pRec, CRContext *ctx)
{
    GLint curProgram = 0;
    pRec->pDispatch->GetIntegerv(GL_CURRENT_PROGRAM, &curProgram);
    if (curProgram)
    {
        Assert(ctx->glsl.activeProgram);
        if (!ctx->glsl.activeProgram)
            crWarning("no active program state with active hw program");
        else
            Assert(ctx->glsl.activeProgram->hwid == curProgram);
        crRecDumpProgramAttribs(pRec, ctx, 0, curProgram);
    }
    else
    {
        Assert(!ctx->glsl.activeProgram);
        crDmpStrF(pRec->pDumper, "--no active program");
    }
}

VBOXDUMPDECL(void) crRecRecompileCurrentProgram(CR_RECORDER *pRec, CRContext *ctx)
{
    GLint curProgram = 0;
    pRec->pDispatch->GetIntegerv(GL_CURRENT_PROGRAM, &curProgram);
    if (curProgram)
    {
        Assert(ctx->glsl.activeProgram);
        if (!ctx->glsl.activeProgram)
            crWarning("no active program state with active hw program");
        else
            Assert(ctx->glsl.activeProgram->hwid == curProgram);
        crRecRecompileProgram(pRec, ctx, 0, curProgram);
    }
    else
    {
        Assert(!ctx->glsl.activeProgram);
        crDmpStrF(pRec->pDumper, "--no active program");
    }
}

int crRecAlphaImgCreate(const CR_BLITTER_IMG *pImg, CR_BLITTER_IMG *pAlphaImg)
{
    if (pImg->enmFormat != GL_RGBA
            && pImg->enmFormat != GL_BGRA)
    {
        crWarning("unsupported format 0x%x", pImg->enmFormat);
        return VERR_NOT_IMPLEMENTED;
    }

    pAlphaImg->bpp = 32;
    pAlphaImg->pitch = pImg->width * 4;
    pAlphaImg->cbData = pAlphaImg->pitch * pImg->height;
    pAlphaImg->enmFormat = GL_BGRA;
    pAlphaImg->width = pImg->width;
    pAlphaImg->height = pImg->height;

    pAlphaImg->pvData = RTMemAlloc(pAlphaImg->cbData);
    if (!pAlphaImg->pvData)
    {
        crWarning("RTMemAlloc failed");
        return VERR_NO_MEMORY;
    }

    uint8_t *pu8SrcBuf = (uint8_t*)pImg->pvData;
    uint8_t *pu8DstBuf = (uint8_t*)pAlphaImg->pvData;
    for (uint32_t ih = 0; ih < pAlphaImg->height; ++ih)
    {
        uint32_t *pu32SrcBuf = (uint32_t*)pu8SrcBuf;
        uint32_t *pu32DstBuf = (uint32_t*)pu8DstBuf;
        for (uint32_t iw = 0; iw < pAlphaImg->width; ++iw)
        {
            uint8_t alpha = (((*pu32SrcBuf) >> 24) & 0xff);
            *pu32DstBuf = (0xff << 24) || (alpha << 16) || (alpha << 8) || alpha;
            ++pu32SrcBuf;
            ++pu32DstBuf;
        }
        pu8SrcBuf += pImg->pitch;
        pu8DstBuf += pAlphaImg->pitch;
    }

    return VINF_SUCCESS;
}

void crRecAlphaImgDestroy(CR_BLITTER_IMG *pImg)
{
    RTMemFree(pImg->pvData);
    pImg->pvData = NULL;
}

void crRecDumpTextureV(CR_RECORDER *pRec, const VBOXVR_TEXTURE *pTex, const char *pszStr, va_list pArgList)
{
    CR_BLITTER_IMG Img = {0};
    int rc = CrBltEnter(pRec->pBlitter);
    if (RT_SUCCESS(rc))
    {
        rc = CrBltImgGetTex(pRec->pBlitter, pTex, GL_BGRA, &Img);
        if (RT_SUCCESS(rc))
        {
            crDmpImgV(pRec->pDumper, &Img, pszStr, pArgList);
            if (g_CrDbgDumpAlphaData)
            {
                CR_BLITTER_IMG AlphaImg = {0};
                rc = crRecAlphaImgCreate(&Img, &AlphaImg);
                if (RT_SUCCESS(rc))
                {
                    crDmpImgF(pRec->pDumper, &AlphaImg, "Texture ALPHA Data");
                    crRecAlphaImgDestroy(&AlphaImg);
                }
                else
                {
                    crWarning("crRecAlphaImgCreate failed rc %d", rc);
                }
            }
            CrBltImgFree(pRec->pBlitter, &Img);
        }
        else
        {
            crWarning("CrBltImgGetTex failed, rc %d", rc);
        }
        CrBltLeave(pRec->pBlitter);
    }
    else
    {
        crWarning("CrBltEnter failed, rc %d", rc);
    }
}

void crRecDumpTextureF(CR_RECORDER *pRec, const VBOXVR_TEXTURE *pTex, const char *pszStr, ...)
{
    va_list pArgList;
    va_start(pArgList, pszStr);
    crRecDumpTextureV(pRec, pTex, pszStr, pArgList);
    va_end(pArgList);
}

void crRecDumpTextureByIdV(CR_RECORDER *pRec, CRContext *ctx, GLint id, const char *pszStr, va_list pArgList)
{
    CRTextureObj *pTobj = (CRTextureObj *)crHashtableSearch(ctx->shared->textureTable, id);
    if (!pTobj)
    {
        crWarning("no texture of id %d", id);
        return;
    }

    CRTextureLevel *pTl = &pTobj->level[0][0 /* level */];
    VBOXVR_TEXTURE Tex;
    Tex.width = pTl->width;
    Tex.height = pTl->height;
    Tex.target = pTobj->target;
    Assert(Tex.target == GL_TEXTURE_2D);
    Tex.hwid = pTobj->hwid;
    if (!Tex.hwid)
    {
        crWarning("no texture hwid of id %d", id);
        return;
    }

    crRecDumpTextureV(pRec, &Tex, pszStr, pArgList);
}

void crRecDumpTextureByIdF(CR_RECORDER *pRec, CRContext *ctx, GLint id, const char *pszStr, ...)
{
    va_list pArgList;
    va_start(pArgList, pszStr);
    crRecDumpTextureByIdV(pRec, ctx, id, pszStr, pArgList);
    va_end(pArgList);
}

void crRecDumpTextures(CR_RECORDER *pRec, CRContext *ctx)
{
    GLint maxUnits = 0;
    GLint curTexUnit = 0;
    GLint restoreTexUnit = 0;
    GLint curProgram = 0;
    int i;

    pRec->pDispatch->GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
    maxUnits = RT_MIN(CR_MAX_TEXTURE_UNITS, maxUnits);

    pRec->pDispatch->GetIntegerv(GL_CURRENT_PROGRAM, &curProgram);
    Assert(curProgram);
    Assert(ctx->glsl.activeProgram && ctx->glsl.activeProgram->hwid == curProgram);

    Assert(maxUnits);
    pRec->pDispatch->GetIntegerv(GL_ACTIVE_TEXTURE, &curTexUnit);
    restoreTexUnit = curTexUnit;
    Assert(curTexUnit >= GL_TEXTURE0);
    Assert(curTexUnit < GL_TEXTURE0 + maxUnits);

    Assert(ctx->texture.curTextureUnit == restoreTexUnit - GL_TEXTURE0);

    for (i = 0; i < maxUnits; ++i)
    {
        GLboolean enabled1D;
        GLboolean enabled2D;
        GLboolean enabled3D;
        GLboolean enabledCubeMap;
        GLboolean enabledRect;
        CRTextureUnit *tu = &ctx->texture.unit[i];

        if (i > 1)
            break;

        if (curTexUnit != i + GL_TEXTURE0)
        {
            pRec->pDispatch->ActiveTextureARB(i + GL_TEXTURE0);
            curTexUnit = i + GL_TEXTURE0;
        }

        enabled1D = pRec->pDispatch->IsEnabled(GL_TEXTURE_1D);
        enabled2D = pRec->pDispatch->IsEnabled(GL_TEXTURE_2D);
        enabled3D = pRec->pDispatch->IsEnabled(GL_TEXTURE_3D);
        enabledCubeMap = pRec->pDispatch->IsEnabled(GL_TEXTURE_CUBE_MAP_ARB);
        enabledRect = pRec->pDispatch->IsEnabled(GL_TEXTURE_RECTANGLE_NV);

        Assert(enabled1D == tu->enabled1D);
        Assert(enabled2D == tu->enabled2D);
        Assert(enabled3D == tu->enabled3D);
        Assert(enabledCubeMap == tu->enabledCubeMap);
        Assert(enabledRect == tu->enabledRect);

        if (enabled1D)
        {
            crWarning("GL_TEXTURE_1D: unsupported");
        }

//        if (enabled2D)
        {
            GLint hwTex = 0;
            VBOXVR_TEXTURE Tex;

            GLint width = 0, height = 0, depth = 0;
            CRTextureObj *pTobj = tu->currentTexture2D;

            pRec->pDispatch->GetIntegerv(GL_TEXTURE_BINDING_2D, &hwTex);
            if (hwTex)
            {
                CRTextureLevel *pTl = &pTobj->level[0][0 /* level */];
                Assert(pTobj
                        && pTobj->hwid == hwTex);

                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH, &depth);

                Assert(width == pTl->width);
                Assert(height == pTl->height);
                Assert(depth == pTl->depth);

                Tex.width = width;
                Tex.height = height;
                Tex.target = GL_TEXTURE_2D;
                Tex.hwid = hwTex;

                if (g_CrDbgDumpRecTexInfo)
                {
                    crRecDumpTexParam(pRec, ctx, GL_TEXTURE_2D);
                    crRecDumpTexEnv(pRec, ctx);
                    crRecDumpTexGen(pRec, ctx);
                }

                crRecDumpTextureF(pRec, &Tex, "ctx(%d), Unit %d: TEXTURE_2D id(%d) hwid(%d), width(%d), height(%d)", ctx, i, pTobj->id, pTobj->hwid, width, height);
            }
//            else
//            {
//                Assert(!pTobj || pTobj->hwid == 0);
//                crWarning("no TEXTURE_2D bound!");
//            }
        }
#if 0
        if (enabled3D)
        {
            crWarning("GL_TEXTURE_3D: unsupported");
        }

        if (enabledCubeMap)
        {
            crWarning("GL_TEXTURE_CUBE_MAP_ARB: unsupported");
        }

//        if (enabledRect)
        {
            GLint hwTex = 0;
            CR_BLITTER_IMG Img = {0};
            VBOXVR_TEXTURE Tex;

            GLint width = 0, height = 0, depth = 0;
            CRTextureObj *pTobj = tu->currentTextureRect;

            pRec->pDispatch->GetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_NV, &hwTex);
            if (hwTex)
            {
                CRTextureLevel *pTl = &pTobj->level[0][0 /* level */];
                Assert(pTobj
                        && pTobj->hwid == hwTex);

                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_RECTANGLE_NV, 0, GL_TEXTURE_WIDTH, &width);
                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_RECTANGLE_NV, 0, GL_TEXTURE_HEIGHT, &height);
                pRec->pDispatch->GetTexLevelParameteriv(GL_TEXTURE_RECTANGLE_NV, 0, GL_TEXTURE_DEPTH, &depth);

                Assert(width == pTl->width);
                Assert(height == pTl->height);
                Assert(depth == pTl->depth);

                Tex.width = width;
                Tex.height = height;
                Tex.target = GL_TEXTURE_RECTANGLE_NV;
                Tex.hwid = hwTex;

                rc = CrBltEnter(pRec->pBlitter);
                if (RT_SUCCESS(rc))
                {
                    rc = CrBltImgGetTex(pRec->pBlitter, &Tex, GL_BGRA, &Img);
                    if (RT_SUCCESS(rc))
                    {
                        crDmpImgF(pRec->pDumper, &Img, "Unit %d: TEXTURE_RECTANGLE data", i);
                        CrBltImgFree(pRec->pBlitter, &Img);
                    }
                    else
                    {
                        crWarning("CrBltImgGetTex failed, rc %d", rc);
                    }
                    CrBltLeave(pRec->pBlitter);
                }
                else
                {
                    crWarning("CrBltEnter failed, rc %d", rc);
                }
            }
//            else
//            {
//                Assert(!pTobj || pTobj->hwid == 0);
//                crWarning("no TEXTURE_RECTANGLE bound!");
//            }
        }
#endif
    }

    if (curTexUnit != restoreTexUnit)
    {
        pRec->pDispatch->ActiveTextureARB(restoreTexUnit);
        curTexUnit = restoreTexUnit;
    }
}

#ifdef RT_OS_WINDOWS
static void crDmpPrint(const char* szString, ...)
{
    char szBuffer[4096] = {0};
    va_list pArgList;
    va_start(pArgList, szString);
    RTStrPrintfV(szBuffer, sizeof (szBuffer), szString, pArgList);
    va_end(pArgList);

    OutputDebugStringA(szBuffer);
}

static void crDmpPrintDmlCmd(const char* pszDesc, const char* pszCmd)
{
    crDmpPrint("<?dml?><exec cmd=\"%s\">%s</exec>, ( %s )\n", pszCmd, pszDesc, pszCmd);
}

void crDmpPrintDumpDmlCmd(const char* pszDesc, const void *pvData, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch)
{
    char Cmd[1024];
    sprintf(Cmd, "!vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d", pvData, width, height, bpp, pitch);
    crDmpPrintDmlCmd(pszDesc, Cmd);
}

DECLCALLBACK(void) crDmpDumpImgDmlBreak(struct CR_DUMPER * pDumper, CR_BLITTER_IMG *pImg, const char*pszEntryDesc)
{
    crDmpPrintDumpDmlCmd(pszEntryDesc, pImg->pvData, pImg->width, pImg->height, pImg->bpp, pImg->pitch);
    RT_BREAKPOINT();
}

DECLCALLBACK(void) crDmpDumpStrDbgPrint(struct CR_DUMPER * pDumper, const char*pszStr)
{
    crDmpPrint("%s\n", pszStr);
}
#endif

static void crDmpHtmlDumpStrExact(struct CR_HTML_DUMPER * pDumper, const char *pszStr)
{
    fprintf(pDumper->pFile, "%s", pszStr);
    fflush(pDumper->pFile);
}

static DECLCALLBACK(void) crDmpHtmlDumpStr(struct CR_DUMPER * pDumper, const char*pszStr)
{
    CR_HTML_DUMPER * pHtmlDumper = (CR_HTML_DUMPER*)pDumper;
    fprintf(pHtmlDumper->pFile, "<pre>%s</pre>\n", pszStr);
    fflush(pHtmlDumper->pFile);
}

static DECLCALLBACK(void) crDmpHtmlDumpImg(struct CR_DUMPER * pDumper, CR_BLITTER_IMG *pImg, const char*pszEntryDesc)
{
    CR_HTML_DUMPER * pHtmlDumper = (CR_HTML_DUMPER*)pDumper;
    char szBuffer[4096] = {0};
    size_t cbWritten = RTStrPrintf(szBuffer, sizeof(szBuffer), "%s/", pHtmlDumper->pszDir);
    char *pszFileName = szBuffer + cbWritten;
    RTStrPrintf(pszFileName, sizeof(szBuffer) - cbWritten, "img%d.bmp", ++pHtmlDumper->cImg);
    crDmpImgBmp(pImg, szBuffer);
    fprintf(pHtmlDumper->pFile, "<a href=\"%s\"><pre>%s</pre><img src=\"%s\" alt=\"%s\" width=\"150\" height=\"100\" /></a><br>\n",
            pszFileName, pszEntryDesc, pszFileName, pszEntryDesc);
    fflush(pHtmlDumper->pFile);
}

static void crDmpHtmlPrintHeader(struct CR_HTML_DUMPER * pDumper)
{
    fprintf(pDumper->pFile, "<html><body>\n");
    fflush(pDumper->pFile);
}

static void crDmpHtmlPrintFooter(struct CR_HTML_DUMPER * pDumper)
{
    fprintf(pDumper->pFile, "</body></html>\n");
    fflush(pDumper->pFile);
}

DECLEXPORT(bool) crDmpHtmlIsInited(struct CR_HTML_DUMPER * pDumper)
{
    return !!pDumper->pFile;
}

DECLEXPORT(void) crDmpHtmlTerm(struct CR_HTML_DUMPER * pDumper)
{
    crDmpHtmlPrintFooter(pDumper);
    fclose (pDumper->pFile);
    pDumper->pFile = NULL;
}

DECLEXPORT(int) crDmpHtmlInit(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile)
{
    int rc = VERR_NO_MEMORY;
    pDumper->Base.pfnDumpImg = crDmpHtmlDumpImg;
    pDumper->Base.pfnDumpStr = crDmpHtmlDumpStr;
    pDumper->cImg = 0;
    pDumper->pszDir = crStrdup(pszDir);
    if (pDumper->pszDir)
    {
        pDumper->pszFile = crStrdup(pszFile);
        if (pDumper->pszFile)
        {
            char szBuffer[4096] = {0};
            RTStrPrintf(szBuffer, sizeof(szBuffer), "%s/%s", pszDir, pszFile);

            pDumper->pszFile = crStrdup(pszFile);
            pDumper->pFile = fopen(szBuffer, "w");
            if (pDumper->pFile)
            {
                crDmpHtmlPrintHeader(pDumper);
                return VINF_SUCCESS;
            }
            else
            {
                crWarning("open failed");
                rc = VERR_OPEN_FAILED;
            }
            crFree((void*)pDumper->pszFile);
        }
        else
        {
            crWarning("open failed");
        }
        crFree((void*)pDumper->pszDir);
    }
    else
    {
        crWarning("open failed");
    }
    return rc;
}

DECLEXPORT(int) crDmpHtmlInitV(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile, va_list pArgList)
{
    char szBuffer[4096] = {0};
    vsprintf_s(szBuffer, sizeof (szBuffer), pszFile, pArgList);
    return crDmpHtmlInit(pDumper, pszDir, szBuffer);
}

DECLEXPORT(int) crDmpHtmlInitF(struct CR_HTML_DUMPER * pDumper, const char *pszDir, const char *pszFile, ...)
{
    int rc;
    va_list pArgList;
    va_start(pArgList, pszFile);
    rc = crDmpHtmlInitV(pDumper, pszDir, pszFile, pArgList);
    va_end(pArgList);
    return rc;
}

#endif
