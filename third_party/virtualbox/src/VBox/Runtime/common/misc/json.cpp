/* $Id: json.cpp $ */
/** @file
 * IPRT JSON parser API (JSON).
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/cdefs.h>
#include <iprt/ctype.h>
#include <iprt/json.h>
#include <iprt/mem.h>
#include <iprt/stream.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * JSON parser position information.
 */
typedef struct RTJSONPOS
{
    /** Line in the source. */
    size_t         iLine;
    /** Current start character .*/
    size_t         iChStart;
    /** Current end character. */
    size_t         iChEnd;
} RTJSONPOS;
/** Pointer to a position. */
typedef RTJSONPOS *PRTJSONPOS;

/**
 * JSON token class.
 */
typedef enum RTJSONTOKENCLASS
{
    /** Invalid. */
    RTJSONTOKENCLASS_INVALID = 0,
    /** Array begin. */
    RTJSONTOKENCLASS_BEGIN_ARRAY,
    /** Object begin. */
    RTJSONTOKENCLASS_BEGIN_OBJECT,
    /** Array end. */
    RTJSONTOKENCLASS_END_ARRAY,
    /** Object end. */
    RTJSONTOKENCLASS_END_OBJECT,
    /** Separator for name/value pairs. */
    RTJSONTOKENCLASS_NAME_SEPARATOR,
    /** Value separator. */
    RTJSONTOKENCLASS_VALUE_SEPARATOR,
    /** String */
    RTJSONTOKENCLASS_STRING,
    /** Number. */
    RTJSONTOKENCLASS_NUMBER,
    /** null keyword. */
    RTJSONTOKENCLASS_NULL,
    /** false keyword. */
    RTJSONTOKENCLASS_FALSE,
    /** true keyword. */
    RTJSONTOKENCLASS_TRUE,
    /** End of stream */
    RTJSONTOKENCLASS_EOS,
    /** 32bit hack. */
    RTJSONTOKENCLASS_32BIT_HACK = 0x7fffffff
} RTJSONTOKENCLASS;
/** Pointer to a token class. */
typedef RTJSONTOKENCLASS *PRTJSONTOKENCLASS;

/**
 * JSON token.
 */
typedef struct RTJSONTOKEN
{
    /** Token class. */
    RTJSONTOKENCLASS        enmClass;
    /** Token position in the source buffer. */
    RTJSONPOS               Pos;
    /** Data based on the token class. */
    union
    {
        /** String. */
        struct
        {
            /** Pointer to the start of the string. */
            char            *pszStr;
        } String;
        /** Number. */
        struct
        {
            int64_t         i64Num;
        } Number;
    } Class;
} RTJSONTOKEN;
/** Pointer to a JSON token. */
typedef RTJSONTOKEN *PRTJSONTOKEN;
/** Pointer to a const script token. */
typedef const RTJSONTOKEN *PCRTJSONTOKEN;

/**
 * Tokenizer read input callback.
 *
 * @returns IPRT status code.
 * @param   pvUser          Opaque user data for the callee.
 * @param   offInput        Start offset from the start of the input stream to read from.
 * @param   pvBuf           Where to store the read data.
 * @param   cbBuf           How much to read.
 * @param   pcbRead         Where to store the amount of data read on succcess.
 */
typedef DECLCALLBACK(int) FNRTJSONTOKENIZERREAD(void *pvUser, size_t offInput, void *pvBuf, size_t cbBuf,
                                                size_t *pcbRead);
/** Pointer to a tokenizer read buffer callback. */
typedef FNRTJSONTOKENIZERREAD *PFNRTJSONTOKENIZERREAD;

/**
 * Tokenizer state.
 */
typedef struct RTJSONTOKENIZER
{
    /** Read callback. */
    PFNRTJSONTOKENIZERREAD  pfnRead;
    /** Opaque user data. */
    void                    *pvUser;
    /** Current offset into the input stream. */
    size_t                  offInput;
    /** Number of valid bytes in the input buffer. */
    size_t                  cbBuf;
    /* Current offset into the input buffer. */
    size_t                  offBuf;
    /** Input cache buffer. */
    char                    achBuf[512];
    /** Current position into the input stream. */
    RTJSONPOS               Pos;
    /** Token 1. */
    RTJSONTOKEN             Token1;
    /** Token 2. */
    RTJSONTOKEN             Token2;
    /** Pointer to the current active token. */
    PRTJSONTOKEN            pTokenCurr;
    /** The next token in the input stream (used for peeking). */
    PRTJSONTOKEN            pTokenNext;
} RTJSONTOKENIZER;
/** Pointer to a JSON tokenizer. */
typedef RTJSONTOKENIZER *PRTJSONTOKENIZER;

/** Pointer to the internal JSON value instance. */
typedef struct RTJSONVALINT *PRTJSONVALINT;

/**
 * A JSON value.
 */
typedef struct RTJSONVALINT
{
    /** Type of the JSON value. */
    RTJSONVALTYPE           enmType;
    /** Reference count for this JSON value. */
    volatile uint32_t       cRefs;
    /** Type dependent data. */
    union
    {
        /** String type*/
        struct
        {
            /** Pointer to the string. */
            char            *pszStr;
        } String;
        /** Number type. */
        struct
        {
            /** Signed 64-bit integer. */
            int64_t         i64Num;
        } Number;
        /** Array type. */
        struct
        {
            /** Number of elements in the array. */
            unsigned        cItems;
            /** Pointer to the array of items. */
            PRTJSONVALINT   *papItems;
        } Array;
        /** Object type. */
        struct
        {
            /** Number of members. */
            unsigned        cMembers;
            /** Pointer to the array holding the member names. */
            char            **papszNames;
            /** Pointer to the array holding the values. */
            PRTJSONVALINT   *papValues;
        } Object;
    } Type;
} RTJSONVALINT;

/**
 * A JSON iterator.
 */
typedef struct RTJSONITINT
{
    /** Referenced JSON value. */
    PRTJSONVALINT           pJsonVal;
    /** Current index. */
    unsigned                idxCur;
} RTJSONITINT;
/** Pointer to the internal JSON iterator instance. */
typedef RTJSONITINT *PRTJSONITINT;

/**
 * Passing arguments for the read callbacks.
 */
typedef struct RTJSONREADERARGS
{
    /** Buffer/File size  */
    size_t                  cbData;
    /** Data specific for one callback. */
    union
    {
        PRTSTREAM           hStream;
        const uint8_t       *pbBuf;
    } u;
} RTJSONREADERARGS;
/** Pointer to a readers argument. */
typedef RTJSONREADERARGS *PRTJSONREADERARGS;


/*********************************************************************************************************************************
*   Global variables                                                                                                             *
*********************************************************************************************************************************/

static int rtJsonParseValue(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken,
                            PRTJSONVALINT *ppJsonVal, PRTERRINFO pErrInfo);

/**
 * Fill the input buffer from the input stream.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer state.
 */
static int rtJsonTokenizerRead(PRTJSONTOKENIZER pTokenizer)
{
    size_t cbRead = 0;
    int rc = pTokenizer->pfnRead(pTokenizer->pvUser, pTokenizer->offInput, &pTokenizer->achBuf[0],
                                 sizeof(pTokenizer->achBuf), &cbRead);
    if (RT_SUCCESS(rc))
    {
        pTokenizer->cbBuf    = cbRead;
        pTokenizer->offInput += cbRead;
        pTokenizer->offBuf   = 0;
        /* Validate UTF-8 encoding. */
        rc = RTStrValidateEncodingEx(&pTokenizer->achBuf[0], cbRead, 0 /* fFlags */);
        /* If we read less than requested we reached the end and fill the remainder with terminators. */
        if (cbRead < sizeof(pTokenizer->achBuf))
            memset(&pTokenizer->achBuf[cbRead], 0, sizeof(pTokenizer->achBuf) - cbRead);
    }

    return rc;
}

/**
 * Skips the given amount of characters in the input stream.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer state.
 * @param   cchSkip         The amount of characters to skip.
 */
static int rtJsonTokenizerSkip(PRTJSONTOKENIZER pTokenizer, size_t cchSkip)
{
    int rc = VINF_SUCCESS;

    /*
     * In case we reached the end of the stream don't even attempt to read new data.
     * Safety precaution for possible bugs in the parser causing out of bounds reads
     */
    if (pTokenizer->achBuf[pTokenizer->offBuf] == '\0')
        return rc;

    while (   cchSkip > 0
           && pTokenizer->offBuf < pTokenizer->cbBuf
           && RT_SUCCESS(rc))
    {
        size_t cchThisSkip = RT_MIN(cchSkip, pTokenizer->cbBuf - pTokenizer->offBuf);

        pTokenizer->offBuf += cchThisSkip;
        /* Read new data if required and we didn't reach the end yet. */
        if (   pTokenizer->offBuf == pTokenizer->cbBuf
            && pTokenizer->cbBuf == sizeof(pTokenizer->achBuf))
            rc = rtJsonTokenizerRead(pTokenizer);

        cchSkip -= cchThisSkip;
    }

    return rc;
}


/**
 * Returns whether the tokenizer reached the end of the stream.
 *
 * @returns true if the tokenizer reached the end of stream marker
 *          false otherwise.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(bool) rtJsonTokenizerIsEos(PRTJSONTOKENIZER pTokenizer)
{
    return pTokenizer->achBuf[pTokenizer->offBuf] == '\0';
}

/**
 * Skip one character in the input stream.
 *
 * @returns nothing.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(void) rtJsonTokenizerSkipCh(PRTJSONTOKENIZER pTokenizer)
{
    rtJsonTokenizerSkip(pTokenizer, 1);
    pTokenizer->Pos.iChStart++;
    pTokenizer->Pos.iChEnd++;
}

/**
 * Returns the next char in the input buffer without advancing it.
 *
 * @returns Next character in the input buffer.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(char) rtJsonTokenizerPeekCh(PRTJSONTOKENIZER pTokenizer)
{
    return   rtJsonTokenizerIsEos(pTokenizer)
           ? '\0'
           : pTokenizer->achBuf[pTokenizer->offBuf + 1]; /** @todo Read out of bounds */
}

/**
 * Returns the next character in the input buffer advancing the internal
 * position.
 *
 * @returns Next character in the stream.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(char) rtJsonTokenizerGetCh(PRTJSONTOKENIZER pTokenizer)
{
    char ch;

    if (rtJsonTokenizerIsEos(pTokenizer))
        ch = '\0';
    else
        ch = pTokenizer->achBuf[pTokenizer->offBuf];

    return ch;
}

/**
 * Sets a new line for the tokenizer.
 *
 * @returns nothing.
 * @param   pTokenizer      The tokenizer state.
 * @param   cSkip           Amount of characters to skip making up the new line.
 */
DECLINLINE(void) rtJsonTokenizerNewLine(PRTJSONTOKENIZER pTokenizer, unsigned cSkip)
{
    rtJsonTokenizerSkip(pTokenizer, cSkip);
    pTokenizer->Pos.iLine++;
    pTokenizer->Pos.iChStart = 1;
    pTokenizer->Pos.iChEnd   = 1;
}

/**
 * Checks whether the current position in the input stream is a new line
 * and skips it.
 *
 * @returns Flag whether there was a new line at the current position
 *          in the input buffer.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(bool) rtJsonTokenizerIsSkipNewLine(PRTJSONTOKENIZER pTokenizer)
{
    bool fNewline = true;

    if (   rtJsonTokenizerGetCh(pTokenizer) == '\r'
        && rtJsonTokenizerPeekCh(pTokenizer) == '\n')
        rtJsonTokenizerNewLine(pTokenizer, 2);
    else if (rtJsonTokenizerGetCh(pTokenizer) == '\n')
        rtJsonTokenizerNewLine(pTokenizer, 1);
    else
        fNewline = false;

    return fNewline;
}

/**
 * Skip all whitespace starting from the current input buffer position.
 * Skips all present comments too.
 *
 * @returns nothing.
 * @param   pTokenizer      The tokenizer state.
 */
DECLINLINE(void) rtJsonTokenizerSkipWhitespace(PRTJSONTOKENIZER pTokenizer)
{
    while (!rtJsonTokenizerIsEos(pTokenizer))
    {
        while (   rtJsonTokenizerGetCh(pTokenizer) == ' '
               || rtJsonTokenizerGetCh(pTokenizer) == '\t')
            rtJsonTokenizerSkipCh(pTokenizer);

        if (   !rtJsonTokenizerIsEos(pTokenizer)
            && !rtJsonTokenizerIsSkipNewLine(pTokenizer))
            break; /* Skipped everything, next is some real content. */
    }
}

/**
 * Get an literal token from the tokenizer.
 *
 * @returns IPRT status code.
 * @param   pTokenizer    The tokenizer state.
 * @param   pToken        The uninitialized token.
 */
static int rtJsonTokenizerGetLiteral(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken)
{
    int rc = VINF_SUCCESS;
    char ch = rtJsonTokenizerGetCh(pTokenizer);
    size_t cchLiteral = 0;
    char szLiteral[6]; /* false + 0 terminator as the lingest possible literal. */
    RT_ZERO(szLiteral);

    pToken->Pos = pTokenizer->Pos;

    Assert(RT_C_IS_ALPHA(ch));

    while (   RT_C_IS_ALPHA(ch)
           && cchLiteral < RT_ELEMENTS(szLiteral) - 1)
    {
        szLiteral[cchLiteral] = ch;
        cchLiteral++;
        rtJsonTokenizerSkipCh(pTokenizer);
        ch = rtJsonTokenizerGetCh(pTokenizer);
    }

    if (!RTStrNCmp(&szLiteral[0], "false", RT_ELEMENTS(szLiteral)))
        pToken->enmClass = RTJSONTOKENCLASS_FALSE;
    else if (!RTStrNCmp(&szLiteral[0], "true", RT_ELEMENTS(szLiteral)))
        pToken->enmClass = RTJSONTOKENCLASS_TRUE;
    else if (!RTStrNCmp(&szLiteral[0], "null", RT_ELEMENTS(szLiteral)))
        pToken->enmClass = RTJSONTOKENCLASS_NULL;
    else
    {
        pToken->enmClass = RTJSONTOKENCLASS_INVALID;
        rc = VERR_JSON_MALFORMED;
    }

    pToken->Pos.iChEnd += cchLiteral;
    return rc;
}

/**
 * Get a numerical constant from the tokenizer.
 *
 * @returns IPRT status code.
 * @param   pTokenizer    The tokenizer state.
 * @param   pToken        The uninitialized token.
 */
static int rtJsonTokenizerGetNumber(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken)
{
    size_t cchNum = 0;
    char   szTmp[128]; /* Everything larger is not possible to display in signed 64bit. */

    pToken->enmClass = RTJSONTOKENCLASS_NUMBER;

    char ch = rtJsonTokenizerGetCh(pTokenizer);
    while (   RT_C_IS_DIGIT(ch)
           && cchNum < sizeof(szTmp) - 1)
    {
        szTmp[cchNum] = ch;
        cchNum++;
        rtJsonTokenizerSkipCh(pTokenizer);
        ch = rtJsonTokenizerGetCh(pTokenizer);
    }

    int rc = VINF_SUCCESS;
    if (RT_C_IS_DIGIT(ch) && cchNum >= sizeof(szTmp) - 1)
        rc = VERR_NUMBER_TOO_BIG;
    else
    {
        szTmp[cchNum] = '\0';
        rc = RTStrToInt64Ex(&szTmp[0], NULL, 0, &pToken->Class.Number.i64Num);
        Assert(RT_SUCCESS(rc) || rc == VWRN_NUMBER_TOO_BIG);
        if (rc == VWRN_NUMBER_TOO_BIG)
            rc = VERR_NUMBER_TOO_BIG;
    }

    return rc;
}

/**
 * Parses a string constant.
 *
 * @returns IPRT status code.
 * @param   pTokenizer    The tokenizer state.
 * @param   pToken        The uninitialized token.
 */
static int rtJsonTokenizerGetString(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken)
{
    int rc = VINF_SUCCESS;
    size_t cchStr = 0;
    char szTmp[_4K];
    RT_ZERO(szTmp);

    Assert(rtJsonTokenizerGetCh(pTokenizer) == '\"');
    rtJsonTokenizerSkipCh(pTokenizer); /* Skip " */

    pToken->enmClass = RTJSONTOKENCLASS_STRING;
    pToken->Pos      = pTokenizer->Pos;

    char ch = rtJsonTokenizerGetCh(pTokenizer);
    while (   ch != '\"'
           && ch != '\0'
           && cchStr < sizeof(szTmp) - 1)
    {
        if (ch == '\\')
        {
            /* Escape sequence, check the next character  */
            rtJsonTokenizerSkipCh(pTokenizer);
            char chNext = rtJsonTokenizerGetCh(pTokenizer);
            switch (chNext)
            {
                case '\"':
                    szTmp[cchStr] = '\"';
                    break;
                case '\\':
                    szTmp[cchStr] = '\\';
                    break;
                case '/':
                    szTmp[cchStr] = '/';
                    break;
                case '\b':
                    szTmp[cchStr] = '\b';
                    break;
                case '\n':
                    szTmp[cchStr] = '\n';
                    break;
                case '\f':
                    szTmp[cchStr] = '\f';
                    break;
                case '\r':
                    szTmp[cchStr] = '\r';
                    break;
                case '\t':
                    szTmp[cchStr] = '\t';
                    break;
                case 'u':
                    rc = VERR_NOT_SUPPORTED;
                    break;
                default:
                    rc = VERR_JSON_MALFORMED;
            }
        }
        else
            szTmp[cchStr] = ch;
        cchStr++;
        rtJsonTokenizerSkipCh(pTokenizer);
        ch = rtJsonTokenizerGetCh(pTokenizer);
    }

    if (rtJsonTokenizerGetCh(pTokenizer) == '\"')
        rtJsonTokenizerSkipCh(pTokenizer); /* Skip closing " */

    pToken->Class.String.pszStr = RTStrDupN(&szTmp[0], cchStr);
    if (pToken->Class.String.pszStr)
        pToken->Pos.iChEnd += cchStr;
    else
        rc = VERR_NO_STR_MEMORY;
    return rc;
}

/**
 * Get the end of stream token.
 *
 * @returns IPRT status code.
 * @param   pTokenizer    The tokenizer state.
 * @param   pToken        The uninitialized token.
 */
static int rtJsonTokenizerGetEos(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken)
{
    Assert(rtJsonTokenizerGetCh(pTokenizer) == '\0');

    pToken->enmClass = RTJSONTOKENCLASS_EOS;
    pToken->Pos      = pTokenizer->Pos;
    return VINF_SUCCESS;
}

/**
 * Read the next token from the tokenizer stream.
 *
 * @returns IPRT status code.
 * @param   pTokenizer    The tokenizer to read from.
 * @param   pToken        Uninitialized token to fill the token data into.
 */
static int rtJsonTokenizerReadNextToken(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken)
{
    int rc = VINF_SUCCESS;

    /* Skip all eventually existing whitespace and newlines first. */
    rtJsonTokenizerSkipWhitespace(pTokenizer);

    char ch = rtJsonTokenizerGetCh(pTokenizer);
    if (RT_C_IS_ALPHA(ch))
        rc = rtJsonTokenizerGetLiteral(pTokenizer, pToken);
    else if (RT_C_IS_DIGIT(ch))
        rc = rtJsonTokenizerGetNumber(pTokenizer, pToken);
    else if (ch == '\"')
        rc = rtJsonTokenizerGetString(pTokenizer, pToken);
    else if (ch == '\0')
        rc = rtJsonTokenizerGetEos(pTokenizer, pToken);
    else if (ch == '{')
    {
        pToken->enmClass = RTJSONTOKENCLASS_BEGIN_OBJECT;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else if (ch == '}')
    {
        pToken->enmClass = RTJSONTOKENCLASS_END_OBJECT;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else if (ch == '[')
    {
        pToken->enmClass = RTJSONTOKENCLASS_BEGIN_ARRAY;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else if (ch == ']')
    {
        pToken->enmClass = RTJSONTOKENCLASS_END_ARRAY;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else if (ch == ':')
    {
        pToken->enmClass = RTJSONTOKENCLASS_NAME_SEPARATOR;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else if (ch == ',')
    {
        pToken->enmClass = RTJSONTOKENCLASS_VALUE_SEPARATOR;
        rtJsonTokenizerSkipCh(pTokenizer);
    }
    else
    {
        pToken->enmClass = RTJSONTOKENCLASS_INVALID;
        rc = VERR_JSON_MALFORMED;
    }

    return rc;
}

/**
 * Create a new tokenizer.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer state to initialize.
 * @param   pfnRead         Read callback for the input stream.
 * @param   pvUser          Opaque user data to pass to the callback.
 */
static int rtJsonTokenizerInit(PRTJSONTOKENIZER pTokenizer, PFNRTJSONTOKENIZERREAD pfnRead, void *pvUser)
{
    pTokenizer->pfnRead      = pfnRead;
    pTokenizer->pvUser       = pvUser;
    pTokenizer->offInput     = 0;
    pTokenizer->cbBuf        = 0;
    pTokenizer->offBuf       = 0;
    pTokenizer->Pos.iLine    = 1;
    pTokenizer->Pos.iChStart = 1;
    pTokenizer->Pos.iChEnd   = 1;
    pTokenizer->pTokenCurr   = &pTokenizer->Token1;
    pTokenizer->pTokenNext   = &pTokenizer->Token2;

    RT_ZERO(pTokenizer->achBuf);

    /* Fill the input buffer. */
    int rc = rtJsonTokenizerRead(pTokenizer);

    /* Fill the tokenizer with two first tokens. */
    if (RT_SUCCESS(rc))
        rc = rtJsonTokenizerReadNextToken(pTokenizer, pTokenizer->pTokenCurr);
    if (RT_SUCCESS(rc))
        rc = rtJsonTokenizerReadNextToken(pTokenizer, pTokenizer->pTokenNext);

    return rc;
}

/**
 * Destroys a given tokenizer state.
 *
 * @returns nothing.
 * @param   pTokenizer      The tokenizer to destroy.
 */
static void rtJsonTokenizerDestroy(PRTJSONTOKENIZER pTokenizer)
{
    RT_NOREF_PV(pTokenizer);
}

/**
 * Get the current token in the input stream.
 *
 * @returns Pointer to the next token in the stream.
 * @param   pTokenizer      The tokenizer state.
 * @param   ppToken         Where to store the pointer to the current token on success.
 */
DECLINLINE(int) rtJsonTokenizerGetToken(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN *ppToken)
{
    *ppToken = pTokenizer->pTokenCurr;
    return VINF_SUCCESS;
}

/**
 * Consume the current token advancing to the next in the stream.
 *
 * @returns nothing.
 * @param   pTokenizer    The tokenizer state.
 */
static void rtJsonTokenizerConsume(PRTJSONTOKENIZER pTokenizer)
{
    PRTJSONTOKEN  pTokenTmp = pTokenizer->pTokenCurr;

    /* Switch next token to current token and read in the next token. */
    pTokenizer->pTokenCurr = pTokenizer->pTokenNext;
    pTokenizer->pTokenNext = pTokenTmp;
    rtJsonTokenizerReadNextToken(pTokenizer, pTokenizer->pTokenNext);
}

/**
 * Consumes the current token if it matches the given class returning an indicator.
 *
 * @returns true if the class matched and the token was consumed.
 * @retval  false otherwise.
 * @param   pTokenizer      The tokenizer state.
 * @param   enmClass        The token class to match against.
 */
static bool rtJsonTokenizerConsumeIfMatched(PRTJSONTOKENIZER pTokenizer, RTJSONTOKENCLASS enmClass)
{
    PRTJSONTOKEN pToken = NULL;
    int rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);
    AssertRC(rc);

    if (pToken->enmClass == enmClass)
    {
        rtJsonTokenizerConsume(pTokenizer);
        return true;
    }

    return false;
}

/**
 * Destroys a given JSON value releasing the reference to all child values.
 *
 * @returns nothing.
 * @param   pThis           The JSON value to destroy.
 */
static void rtJsonValDestroy(PRTJSONVALINT pThis)
{
    switch (pThis->enmType)
    {
        case RTJSONVALTYPE_OBJECT:
            for (unsigned i = 0; i < pThis->Type.Object.cMembers; i++)
            {
                RTStrFree(pThis->Type.Object.papszNames[i]);
                RTJsonValueRelease(pThis->Type.Object.papValues[i]);
            }
            RTMemFree(pThis->Type.Object.papszNames);
            RTMemFree(pThis->Type.Object.papValues);
            break;
        case RTJSONVALTYPE_ARRAY:
            for (unsigned i = 0; i < pThis->Type.Array.cItems; i++)
                RTJsonValueRelease(pThis->Type.Array.papItems[i]);
            RTMemFree(pThis->Type.Array.papItems);
            break;
        case RTJSONVALTYPE_STRING:
            RTStrFree(pThis->Type.String.pszStr);
            break;
        case RTJSONVALTYPE_NUMBER:
        case RTJSONVALTYPE_NULL:
        case RTJSONVALTYPE_TRUE:
        case RTJSONVALTYPE_FALSE:
            /* nothing to do. */
            break;
        default:
            AssertMsgFailed(("Invalid JSON value type: %u\n", pThis->enmType));
    }
    RTMemFree(pThis);
}

/**
 * Creates a new JSON value with the given type.
 *
 * @returns Pointer to JSON value on success, NULL if out of memory.
 * @param   enmType         The JSON value type.
 */
static PRTJSONVALINT rtJsonValueCreate(RTJSONVALTYPE enmType)
{
    PRTJSONVALINT pThis = (PRTJSONVALINT)RTMemAllocZ(sizeof(RTJSONVALINT));
    if (RT_LIKELY(pThis))
    {
        pThis->enmType = enmType;
        pThis->cRefs   = 1;
    }

    return pThis;
}

/**
 * Parses an JSON array.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer to use.
 * @param   pJsonVal        The JSON array value to fill in.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
static int rtJsonParseArray(PRTJSONTOKENIZER pTokenizer, PRTJSONVALINT pJsonVal, PRTERRINFO pErrInfo)
{
    int rc = VINF_SUCCESS;
    PRTJSONTOKEN pToken = NULL;
    uint32_t cItems = 0;
    uint32_t cItemsMax = 0;
    PRTJSONVALINT *papItems = NULL;

    rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);
    while (   RT_SUCCESS(rc)
           && pToken->enmClass != RTJSONTOKENCLASS_END_ARRAY
           && pToken->enmClass != RTJSONTOKENCLASS_EOS)
    {
        PRTJSONVALINT pVal = NULL;
        rc = rtJsonParseValue(pTokenizer, pToken, &pVal, pErrInfo);
        if (RT_SUCCESS(rc))
        {
            if (cItems == cItemsMax)
            {
                cItemsMax += 10;
                PRTJSONVALINT *papItemsNew = (PRTJSONVALINT *)RTMemRealloc(papItems, cItemsMax * sizeof(PRTJSONVALINT));
                if (RT_UNLIKELY(!papItemsNew))
                {
                    rc = VERR_NO_MEMORY;
                    break;
                }
                papItems = papItemsNew;
            }

            Assert(cItems < cItemsMax);
            papItems[cItems] = pVal;
            cItems++;
        }

        /* Skip value separator and continue with next token. */
        bool fSkippedSep = rtJsonTokenizerConsumeIfMatched(pTokenizer, RTJSONTOKENCLASS_VALUE_SEPARATOR);
        rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);

        if (   RT_SUCCESS(rc)
            && !fSkippedSep
            && pToken->enmClass != RTJSONTOKENCLASS_END_ARRAY)
            rc = VERR_JSON_MALFORMED;
    }

    if (RT_SUCCESS(rc))
    {
        if (pToken->enmClass == RTJSONTOKENCLASS_END_ARRAY)
        {
            rtJsonTokenizerConsume(pTokenizer);
            pJsonVal->Type.Array.cItems = cItems;
            pJsonVal->Type.Array.papItems = papItems;
        }
        else
            rc = VERR_JSON_MALFORMED;
    }

    if (RT_FAILURE(rc))
    {
        for (uint32_t i = 0; i < cItems; i++)
            RTJsonValueRelease(papItems[i]);
        RTMemFree(papItems);
    }

    return rc;
}

/**
 * Parses an JSON object.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer to use.
 * @param   pJsonVal        The JSON object value to fill in.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
static int rtJsonParseObject(PRTJSONTOKENIZER pTokenizer, PRTJSONVALINT pJsonVal, PRTERRINFO pErrInfo)
{
    int rc = VINF_SUCCESS;
    PRTJSONTOKEN pToken = NULL;
    uint32_t cMembers = 0;
    uint32_t cMembersMax = 0;
    PRTJSONVALINT *papValues = NULL;
    char **papszNames = NULL;

    rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);
    while (   RT_SUCCESS(rc)
           && pToken->enmClass == RTJSONTOKENCLASS_STRING)
    {
        char *pszName = pToken->Class.String.pszStr;

        rtJsonTokenizerConsume(pTokenizer);
        if (rtJsonTokenizerConsumeIfMatched(pTokenizer, RTJSONTOKENCLASS_NAME_SEPARATOR))
        {
            PRTJSONVALINT pVal = NULL;
            rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);
            if (RT_SUCCESS(rc))
                rc = rtJsonParseValue(pTokenizer, pToken, &pVal, pErrInfo);
            if (RT_SUCCESS(rc))
            {
                if (cMembers == cMembersMax)
                {
                    cMembersMax += 10;
                    PRTJSONVALINT *papValuesNew = (PRTJSONVALINT *)RTMemRealloc(papValues, cMembersMax * sizeof(PRTJSONVALINT));
                    char **papszNamesNew =  (char **)RTMemRealloc(papValues, cMembersMax * sizeof(char *));
                    if (RT_UNLIKELY(!papValuesNew || !papszNamesNew))
                    {
                        if (papValuesNew)
                            RTMemFree(papValuesNew);
                        if (papszNamesNew)
                            RTMemFree(papszNamesNew);
                        rc = VERR_NO_MEMORY;
                        break;
                    }

                    papValues = papValuesNew;
                    papszNames = papszNamesNew;
                }

                Assert(cMembers < cMembersMax);
                papszNames[cMembers] = pszName;
                papValues[cMembers] = pVal;
                cMembers++;

                /* Skip value separator and continue with next token. */
                bool fSkippedSep = rtJsonTokenizerConsumeIfMatched(pTokenizer, RTJSONTOKENCLASS_VALUE_SEPARATOR);
                rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);

                if (   RT_SUCCESS(rc)
                    && !fSkippedSep
                    && pToken->enmClass != RTJSONTOKENCLASS_END_OBJECT)
                    rc = VERR_JSON_MALFORMED;
            }
        }
        else
            rc = VERR_JSON_MALFORMED;
    }

    if (RT_SUCCESS(rc))
    {
        if (pToken->enmClass == RTJSONTOKENCLASS_END_OBJECT)
        {
            rtJsonTokenizerConsume(pTokenizer);
            pJsonVal->Type.Object.cMembers = cMembers;
            pJsonVal->Type.Object.papValues = papValues;
            pJsonVal->Type.Object.papszNames = papszNames;
        }
        else
            rc = VERR_JSON_MALFORMED;
    }

    if (RT_FAILURE(rc))
    {
        for (uint32_t i = 0; i < cMembers; i++)
        {
            RTJsonValueRelease(papValues[i]);
            RTStrFree(papszNames[i]);
        }
        RTMemFree(papValues);
        RTMemFree(papszNames);
    }

    return rc;
}

/**
 * Parses a single JSON value and returns it on success.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer to use.
 * @param   pToken          The token to parse.
 * @param   ppJsonVal       Where to store the pointer to the JSON value on success.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
static int rtJsonParseValue(PRTJSONTOKENIZER pTokenizer, PRTJSONTOKEN pToken,
                            PRTJSONVALINT *ppJsonVal, PRTERRINFO pErrInfo)
{
    int rc = VINF_SUCCESS;
    PRTJSONVALINT pVal = NULL;

    switch (pToken->enmClass)
    {
        case RTJSONTOKENCLASS_BEGIN_ARRAY:
            rtJsonTokenizerConsume(pTokenizer);
            pVal = rtJsonValueCreate(RTJSONVALTYPE_ARRAY);
            if (RT_LIKELY(pVal))
                rc = rtJsonParseArray(pTokenizer, pVal, pErrInfo);
            break;
        case RTJSONTOKENCLASS_BEGIN_OBJECT:
            rtJsonTokenizerConsume(pTokenizer);
            pVal = rtJsonValueCreate(RTJSONVALTYPE_OBJECT);
            if (RT_LIKELY(pVal))
                rc = rtJsonParseObject(pTokenizer, pVal, pErrInfo);
            break;
        case RTJSONTOKENCLASS_STRING:
            pVal = rtJsonValueCreate(RTJSONVALTYPE_STRING);
            if (RT_LIKELY(pVal))
                pVal->Type.String.pszStr = pToken->Class.String.pszStr;
            rtJsonTokenizerConsume(pTokenizer);
            break;
        case RTJSONTOKENCLASS_NUMBER:
            pVal = rtJsonValueCreate(RTJSONVALTYPE_NUMBER);
            if (RT_LIKELY(pVal))
                pVal->Type.Number.i64Num = pToken->Class.Number.i64Num;
            rtJsonTokenizerConsume(pTokenizer);
            break;
        case RTJSONTOKENCLASS_NULL:
            rtJsonTokenizerConsume(pTokenizer);
            pVal = rtJsonValueCreate(RTJSONVALTYPE_NULL);
            break;
        case RTJSONTOKENCLASS_FALSE:
            rtJsonTokenizerConsume(pTokenizer);
            pVal = rtJsonValueCreate(RTJSONVALTYPE_FALSE);
            break;
        case RTJSONTOKENCLASS_TRUE:
            rtJsonTokenizerConsume(pTokenizer);
            pVal = rtJsonValueCreate(RTJSONVALTYPE_TRUE);
            break;
        case RTJSONTOKENCLASS_END_ARRAY:
        case RTJSONTOKENCLASS_END_OBJECT:
        case RTJSONTOKENCLASS_NAME_SEPARATOR:
        case RTJSONTOKENCLASS_VALUE_SEPARATOR:
        case RTJSONTOKENCLASS_EOS:
        default:
            /** @todo Error info */
            rc = VERR_JSON_MALFORMED;
            break;
    }

    if (RT_SUCCESS(rc))
    {
        if (pVal)
            *ppJsonVal = pVal;
        else
            rc = VERR_NO_MEMORY;
    }
    else if (pVal)
        rtJsonValDestroy(pVal);

    return rc;
}

/**
 * Entry point to parse a JSON document.
 *
 * @returns IPRT status code.
 * @param   pTokenizer      The tokenizer state.
 * @param   ppJsonVal       Where to store the root JSON value on success.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
static int rtJsonParse(PRTJSONTOKENIZER pTokenizer, PRTJSONVALINT *ppJsonVal,
                       PRTERRINFO pErrInfo)
{
    PRTJSONTOKEN pToken = NULL;
    int rc = rtJsonTokenizerGetToken(pTokenizer, &pToken);
    if (RT_SUCCESS(rc))
        rc = rtJsonParseValue(pTokenizer, pToken, ppJsonVal, pErrInfo);

    return rc;
}

/**
 * Read callback for RTJsonParseFromBuf().
 */
static DECLCALLBACK(int) rtJsonTokenizerParseFromBuf(void *pvUser, size_t offInput,
                                                     void *pvBuf, size_t cbBuf,
                                                     size_t *pcbRead)
{
    PRTJSONREADERARGS pArgs = (PRTJSONREADERARGS)pvUser;
    size_t cbLeft = offInput < pArgs->cbData ? pArgs->cbData - offInput : 0;

    if (cbLeft)
        memcpy(pvBuf, &pArgs->u.pbBuf[offInput], RT_MIN(cbLeft, cbBuf));

    *pcbRead = RT_MIN(cbLeft, cbBuf);

    return VINF_SUCCESS;
}

/**
 * Read callback for RTJsonParseFromString().
 */
static DECLCALLBACK(int) rtJsonTokenizerParseFromString(void *pvUser, size_t offInput,
                                                        void *pvBuf, size_t cbBuf,
                                                        size_t *pcbRead)
{
    const char *pszStr = (const char *)pvUser;
    size_t cchStr = strlen(pszStr) + 1; /* Include zero terminator. */
    size_t cbLeft = offInput < cchStr ? cchStr - offInput : 0;

    if (cbLeft)
        memcpy(pvBuf, &pszStr[offInput], RT_MIN(cbLeft, cbBuf));

    *pcbRead = RT_MIN(cbLeft, cbBuf);

    return VINF_SUCCESS;
}

/**
 * Read callback for RTJsonParseFromFile().
 */
static DECLCALLBACK(int) rtJsonTokenizerParseFromFile(void *pvUser, size_t offInput,
                                                      void *pvBuf, size_t cbBuf,
                                                      size_t *pcbRead)
{
    PRTJSONREADERARGS pArgs = (PRTJSONREADERARGS)pvUser;

    RT_NOREF_PV(offInput);

    size_t cbRead = 0;
    int rc = RTStrmReadEx(pArgs->u.hStream, pvBuf, cbBuf, &cbRead);
    if (RT_SUCCESS(rc))
        *pcbRead = cbRead;

    return rc;
}

RTDECL(int) RTJsonParseFromBuf(PRTJSONVAL phJsonVal, const uint8_t *pbBuf, size_t cbBuf,
                               PRTERRINFO pErrInfo)
{
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);
    AssertPtrReturn(pbBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf > 0, VERR_INVALID_PARAMETER);

    RTJSONTOKENIZER Tokenizer;
    RTJSONREADERARGS Args;
    Args.cbData  = cbBuf;
    Args.u.pbBuf = pbBuf;

    int rc = rtJsonTokenizerInit(&Tokenizer, rtJsonTokenizerParseFromBuf, &Args);
    if (RT_SUCCESS(rc))
    {
        rc = rtJsonParse(&Tokenizer, phJsonVal, pErrInfo);
        rtJsonTokenizerDestroy(&Tokenizer);
    }

    return rc;
}

RTDECL(int) RTJsonParseFromString(PRTJSONVAL phJsonVal, const char *pszStr, PRTERRINFO pErrInfo)
{
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);
    AssertPtrReturn(pszStr, VERR_INVALID_POINTER);

    RTJSONTOKENIZER Tokenizer;
    int rc = rtJsonTokenizerInit(&Tokenizer, rtJsonTokenizerParseFromString, (void *)pszStr);
    if (RT_SUCCESS(rc))
    {
        rc = rtJsonParse(&Tokenizer, phJsonVal, pErrInfo);
        rtJsonTokenizerDestroy(&Tokenizer);
    }

    return rc;
}

RTDECL(int) RTJsonParseFromFile(PRTJSONVAL phJsonVal, const char *pszFilename, PRTERRINFO pErrInfo)
{
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    RTJSONREADERARGS Args;

    Args.cbData  = 0;
    rc = RTStrmOpen(pszFilename, "r", &Args.u.hStream);
    if (RT_SUCCESS(rc))
    {
        RTJSONTOKENIZER Tokenizer;

        rc = rtJsonTokenizerInit(&Tokenizer, rtJsonTokenizerParseFromFile, &Args);
        if (RT_SUCCESS(rc))
        {
            rc = rtJsonParse(&Tokenizer, phJsonVal, pErrInfo);
            rtJsonTokenizerDestroy(&Tokenizer);
        }
        RTStrmClose(Args.u.hStream);
    }

    return rc;
}

RTDECL(uint32_t) RTJsonValueRetain(RTJSONVAL hJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(pThis, UINT32_MAX);

    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1M, ("%#x %p\n", cRefs, pThis));
    return cRefs;
}

RTDECL(uint32_t) RTJsonValueRelease(RTJSONVAL hJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    if (pThis == NIL_RTJSONVAL)
        return 0;
    AssertPtrReturn(pThis, UINT32_MAX);

    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    AssertMsg(cRefs < _1M, ("%#x %p\n", cRefs, pThis));
    if (cRefs == 0)
        rtJsonValDestroy(pThis);
    return cRefs;
}

RTDECL(RTJSONVALTYPE) RTJsonValueGetType(RTJSONVAL hJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    if (pThis == NIL_RTJSONVAL)
        return RTJSONVALTYPE_INVALID;
    AssertPtrReturn(pThis, RTJSONVALTYPE_INVALID);

    return pThis->enmType;
}

RTDECL(const char *) RTJsonValueGetString(RTJSONVAL hJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertReturn(pThis != NIL_RTJSONVAL, NULL);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_STRING, NULL);

    return pThis->Type.String.pszStr;
}

RTDECL(int) RTJsonValueQueryString(RTJSONVAL hJsonVal, const char **ppszStr)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(ppszStr, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_STRING, VERR_JSON_VALUE_INVALID_TYPE);

    *ppszStr = pThis->Type.String.pszStr;
    return VINF_SUCCESS;
}

RTDECL(int) RTJsonValueQueryInteger(RTJSONVAL hJsonVal, int64_t *pi64Num)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(pi64Num, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_NUMBER, VERR_JSON_VALUE_INVALID_TYPE);

    *pi64Num = pThis->Type.Number.i64Num;
    return VINF_SUCCESS;
}

RTDECL(int) RTJsonValueQueryByName(RTJSONVAL hJsonVal, const char *pszName, PRTJSONVAL phJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_OBJECT, VERR_JSON_VALUE_INVALID_TYPE);

    int rc = VERR_NOT_FOUND;
    for (unsigned i = 0; i < pThis->Type.Object.cMembers; i++)
    {
        if (!RTStrCmp(pThis->Type.Object.papszNames[i], pszName))
        {
            RTJsonValueRetain(pThis->Type.Object.papValues[i]);
            *phJsonVal = pThis->Type.Object.papValues[i];
            rc = VINF_SUCCESS;
            break;
        }
    }

    return rc;
}

RTDECL(int) RTJsonValueQueryIntegerByName(RTJSONVAL hJsonVal, const char *pszName, int64_t *pi64Num)
{
    RTJSONVAL hJsonValNum = NIL_RTJSONVAL;
    int rc = RTJsonValueQueryByName(hJsonVal, pszName, &hJsonValNum);
    if (RT_SUCCESS(rc))
    {
        rc = RTJsonValueQueryInteger(hJsonValNum, pi64Num);
        RTJsonValueRelease(hJsonValNum);
    }

    return rc;
}

RTDECL(int) RTJsonValueQueryStringByName(RTJSONVAL hJsonVal, const char *pszName, char **ppszStr)
{
    RTJSONVAL hJsonValStr = NIL_RTJSONVAL;
    int rc = RTJsonValueQueryByName(hJsonVal, pszName, &hJsonValStr);
    if (RT_SUCCESS(rc))
    {
        const char *pszStr = NULL;
        rc = RTJsonValueQueryString(hJsonValStr, &pszStr);
        if (RT_SUCCESS(rc))
        {
            *ppszStr = RTStrDup(pszStr);
            if (!*ppszStr)
                rc = VERR_NO_STR_MEMORY;
        }
        RTJsonValueRelease(hJsonValStr);
    }

    return rc;
}

RTDECL(int) RTJsonValueQueryBooleanByName(RTJSONVAL hJsonVal, const char *pszName, bool *pfBoolean)
{
    AssertPtrReturn(pfBoolean, VERR_INVALID_POINTER);

    RTJSONVAL hJsonValBool = NIL_RTJSONVAL;
    int rc = RTJsonValueQueryByName(hJsonVal, pszName, &hJsonValBool);
    if (RT_SUCCESS(rc))
    {
        RTJSONVALTYPE enmType = RTJsonValueGetType(hJsonValBool);
        if (enmType == RTJSONVALTYPE_TRUE)
            *pfBoolean = true;
        else if (enmType == RTJSONVALTYPE_FALSE)
            *pfBoolean = false;
        else
            rc = VERR_JSON_VALUE_INVALID_TYPE;
        RTJsonValueRelease(hJsonValBool);
    }

    return rc;
}

RTDECL(unsigned) RTJsonValueGetArraySize(RTJSONVAL hJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertReturn(pThis != NIL_RTJSONVAL, 0);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_ARRAY, 0);

    return pThis->Type.Array.cItems;
}

RTDECL(int) RTJsonValueQueryArraySize(RTJSONVAL hJsonVal, unsigned *pcItems)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(pcItems, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_ARRAY, VERR_JSON_VALUE_INVALID_TYPE);

    *pcItems = pThis->Type.Array.cItems;
    return VINF_SUCCESS;
}

RTDECL(int) RTJsonValueQueryByIndex(RTJSONVAL hJsonVal, unsigned idx, PRTJSONVAL phJsonVal)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_ARRAY, VERR_JSON_VALUE_INVALID_TYPE);
    AssertReturn(idx < pThis->Type.Array.cItems, VERR_OUT_OF_RANGE);

    RTJsonValueRetain(pThis->Type.Array.papItems[idx]);
    *phJsonVal = pThis->Type.Array.papItems[idx];
    return VINF_SUCCESS;
}

RTDECL(int) RTJsonIteratorBegin(RTJSONVAL hJsonVal, PRTJSONIT phJsonIt)
{
    PRTJSONVALINT pThis = hJsonVal;
    AssertPtrReturn(phJsonIt, VERR_INVALID_POINTER);
    AssertReturn(pThis != NIL_RTJSONVAL, VERR_INVALID_HANDLE);
    AssertReturn(pThis->enmType == RTJSONVALTYPE_ARRAY || pThis->enmType == RTJSONVALTYPE_OBJECT,
                 VERR_JSON_VALUE_INVALID_TYPE);

    PRTJSONITINT pIt = (PRTJSONITINT)RTMemTmpAllocZ(sizeof(RTJSONITINT));
    if (RT_UNLIKELY(!pIt))
        return VERR_NO_MEMORY;

    RTJsonValueRetain(hJsonVal);
    pIt->pJsonVal = pThis;
    pIt->idxCur = 0;
    *phJsonIt = pIt;

    return VINF_SUCCESS;
}

RTDECL(int) RTJsonIteratorQueryValue(RTJSONIT hJsonIt, PRTJSONVAL phJsonVal, const char **ppszName)
{
    PRTJSONITINT pIt = hJsonIt;
    AssertReturn(pIt != NIL_RTJSONIT, VERR_INVALID_HANDLE);
    AssertPtrReturn(phJsonVal, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    PRTJSONVALINT pThis = pIt->pJsonVal;
    if (pThis->enmType == RTJSONVALTYPE_ARRAY)
    {
        if (pIt->idxCur < pThis->Type.Array.cItems)
        {
            if (ppszName)
                *ppszName = NULL;

            RTJsonValueRetain(pThis->Type.Array.papItems[pIt->idxCur]);
            *phJsonVal = pThis->Type.Array.papItems[pIt->idxCur];
        }
        else
            rc = VERR_JSON_ITERATOR_END;
    }
    else
    {
        Assert(pThis->enmType == RTJSONVALTYPE_OBJECT);

        if (pIt->idxCur < pThis->Type.Object.cMembers)
        {
            if (ppszName)
                *ppszName = pThis->Type.Object.papszNames[pIt->idxCur];

            RTJsonValueRetain(pThis->Type.Object.papValues[pIt->idxCur]);
            *phJsonVal = pThis->Type.Object.papValues[pIt->idxCur];
        }
        else
            rc = VERR_JSON_ITERATOR_END;
    }

    return rc;
}

RTDECL(int) RTJsonIteratorNext(RTJSONIT hJsonIt)
{
    PRTJSONITINT pIt = hJsonIt;
    AssertReturn(pIt != NIL_RTJSONIT, VERR_INVALID_HANDLE);

    int rc = VINF_SUCCESS;
    PRTJSONVALINT pThis = pIt->pJsonVal;
    if (pThis->enmType == RTJSONVALTYPE_ARRAY)
    {
        if (pIt->idxCur < pThis->Type.Array.cItems)
            pIt->idxCur++;

        if (pIt->idxCur == pThis->Type.Object.cMembers)
            rc = VERR_JSON_ITERATOR_END;
    }
    else
    {
        Assert(pThis->enmType == RTJSONVALTYPE_OBJECT);

        if (pIt->idxCur < pThis->Type.Object.cMembers)
            pIt->idxCur++;

        if (pIt->idxCur == pThis->Type.Object.cMembers)
            rc = VERR_JSON_ITERATOR_END;
    }

    return rc;
}

RTDECL(void) RTJsonIteratorFree(RTJSONIT hJsonIt)
{
    PRTJSONITINT pThis = hJsonIt;
    if (pThis == NIL_RTJSONIT)
        return;
    AssertPtrReturnVoid(pThis);

    RTJsonValueRelease(pThis->pJsonVal);
    RTMemTmpFree(pThis);
}

