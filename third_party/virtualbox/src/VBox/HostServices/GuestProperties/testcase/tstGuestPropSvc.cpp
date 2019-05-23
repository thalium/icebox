/* $Id: tstGuestPropSvc.cpp $ */
/** @file
 *
 * Testcase for the guest property service.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/HostServices/GuestPropertySvc.h>
#include <iprt/test.h>
#include <iprt/time.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static RTTEST g_hTest = NIL_RTTEST;

using namespace guestProp;

extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable);

/** Simple call handle structure for the guest call completion callback */
struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code */
    int32_t rc;
};

/** Call completion callback for guest calls. */
static DECLCALLBACK(void) callComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    callHandle->rc = rc;
}

/**
 * Initialise the HGCM service table as much as we need to start the
 * service
 * @param  pTable the table to initialise
 */
void initTable(VBOXHGCMSVCFNTABLE *pTable, VBOXHGCMSVCHELPERS *pHelpers)
{
    pTable->cbSize              = sizeof (VBOXHGCMSVCFNTABLE);
    pTable->u32Version          = VBOX_HGCM_SVC_VERSION;
    pHelpers->pfnCallComplete   = callComplete;
    pTable->pHelpers            = pHelpers;
}

/**
 * A list of valid flag strings for testConvertFlags.  The flag conversion
 * functions should accept these and convert them from string to a flag type
 * and back without errors.
 */
struct flagStrings
{
    /** Flag string in a format the functions should recognise */
    const char *pcszIn;
    /** How the functions should output the string again */
    const char *pcszOut;
}
g_aValidFlagStrings[] =
{
    /* pcszIn,                                          pcszOut */
    { "  ",                                             "" },
    { "transient, ",                                    "TRANSIENT" },
    { "  rdOnLyHOST, transIENT  ,     READONLY    ",    "TRANSIENT, READONLY" },
    { " rdonlyguest",                                   "RDONLYGUEST" },
    { "rdonlyhost     ",                                "RDONLYHOST" },
    { "transient, transreset, rdonlyhost",              "TRANSIENT, RDONLYHOST, TRANSRESET" },
    { "transient, transreset, rdonlyguest",             "TRANSIENT, RDONLYGUEST, TRANSRESET" },     /* max length */
    { "rdonlyguest, rdonlyhost",                        "READONLY" },
    { "transient,   transreset, ",                      "TRANSIENT, TRANSRESET" }, /* Don't combine them ... */
    { "transreset, ",                                   "TRANSIENT, TRANSRESET" }, /* ... instead expand transreset for old adds. */
};

/**
 * A list of invalid flag strings for testConvertFlags.  The flag conversion
 * functions should reject these.
 */
const char *g_apszInvalidFlagStrings[] =
{
    "RDONLYHOST,,",
    "  TRANSIENT READONLY"
};

/**
 * Test the flag conversion functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testConvertFlags(void)
{
    int rc = VINF_SUCCESS;
    char *pszFlagBuffer = (char *)RTTestGuardedAllocTail(g_hTest, MAX_FLAGS_LEN);

    RTTestISub("Conversion of valid flags strings");
    for (unsigned i = 0; i < RT_ELEMENTS(g_aValidFlagStrings) && RT_SUCCESS(rc); ++i)
    {
        uint32_t fFlags;
        rc = validateFlags(g_aValidFlagStrings[i].pcszIn, &fFlags);
        if (RT_FAILURE(rc))
            RTTestIFailed("Failed to validate flag string '%s'", g_aValidFlagStrings[i].pcszIn);
        if (RT_SUCCESS(rc))
        {
            rc = writeFlags(fFlags, pszFlagBuffer);
            if (RT_FAILURE(rc))
                RTTestIFailed("Failed to convert flag string '%s' back to a string.",
                              g_aValidFlagStrings[i].pcszIn);
        }
        if (RT_SUCCESS(rc) && (strlen(pszFlagBuffer) > MAX_FLAGS_LEN - 1))
        {
            RTTestIFailed("String '%s' converts back to a flag string which is too long.\n",
                          g_aValidFlagStrings[i].pcszIn);
            rc = VERR_TOO_MUCH_DATA;
        }
        if (RT_SUCCESS(rc) && (strcmp(pszFlagBuffer, g_aValidFlagStrings[i].pcszOut) != 0))
        {
            RTTestIFailed("String '%s' converts back to '%s' instead of to '%s'\n",
                          g_aValidFlagStrings[i].pcszIn, pszFlagBuffer,
                          g_aValidFlagStrings[i].pcszOut);
            rc = VERR_PARSE_ERROR;
        }
    }
    if (RT_SUCCESS(rc))
    {
        RTTestISub("Rejection of invalid flags strings");
        for (unsigned i = 0; i < RT_ELEMENTS(g_apszInvalidFlagStrings) && RT_SUCCESS(rc); ++i)
        {
            uint32_t fFlags;
            /* This is required to fail. */
            if (RT_SUCCESS(validateFlags(g_apszInvalidFlagStrings[i], &fFlags)))
            {
                RTTestIFailed("String '%s' was incorrectly accepted as a valid flag string.\n",
                              g_apszInvalidFlagStrings[i]);
                rc = VERR_PARSE_ERROR;
            }
        }
    }
    if (RT_SUCCESS(rc))
    {
        uint32_t u32BadFlags = ALLFLAGS << 1;
        RTTestISub("Rejection of an invalid flags field");
        /* This is required to fail. */
        if (RT_SUCCESS(writeFlags(u32BadFlags, pszFlagBuffer)))
        {
            RTTestIFailed("Flags 0x%x were incorrectly written out as '%.*s'\n",
                          u32BadFlags, MAX_FLAGS_LEN, pszFlagBuffer);
            rc = VERR_PARSE_ERROR;
        }
    }

    RTTestGuardedFree(g_hTest, pszFlagBuffer);
}

/**
 * List of property names for testSetPropsHost.
 */
const char *g_apcszNameBlock[] =
{
    "test/name/",
    "test name",
    "TEST NAME",
    "/test/name",
    NULL
};

/**
 * List of property values for testSetPropsHost.
 */
const char *g_apcszValueBlock[] =
{
    "test/value/",
    "test value",
    "TEST VALUE",
    "/test/value",
    NULL
};

/**
 * List of property timestamps for testSetPropsHost.
 */
uint64_t g_au64TimestampBlock[] =
{
    0, 999, 999999, UINT64_C(999999999999), 0
};

/**
 * List of property flags for testSetPropsHost.
 */
const char *g_apcszFlagsBlock[] =
{
    "",
    "readonly, transient",
    "RDONLYHOST",
    "RdOnlyGuest",
    NULL
};

/**
 * Test the SET_PROPS_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testSetPropsHost(VBOXHGCMSVCFNTABLE *ptable)
{
    RTTestISub("SET_PROPS_HOST");
    RTTESTI_CHECK_RETV(RT_VALID_PTR(ptable->pfnHostCall));

    VBOXHGCMSVCPARM aParms[4];
    aParms[0].setPointer((void *)g_apcszNameBlock, 0);
    aParms[1].setPointer((void *)g_apcszValueBlock, 0);
    aParms[2].setPointer((void *)g_au64TimestampBlock, 0);
    aParms[3].setPointer((void *)g_apcszFlagsBlock, 0);
    RTTESTI_CHECK_RC(ptable->pfnHostCall(ptable->pvService, SET_PROPS_HOST, 4, &aParms[0]), VINF_SUCCESS);
}

/** Result strings for zeroth enumeration test */
static const char *g_apchEnumResult0[] =
{
    "test/name/\0test/value/\0""0\0",
    "test name\0test value\0""999\0TRANSIENT, READONLY",
    "TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST",
    "/test/name\0/test/value\0""999999999999\0RDONLYGUEST",
    NULL
};

/** Result string sizes for zeroth enumeration test */
static const uint32_t g_acbEnumResult0[] =
{
    sizeof("test/name/\0test/value/\0""0\0"),
    sizeof("test name\0test value\0""999\0TRANSIENT, READONLY"),
    sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST"),
    sizeof("/test/name\0/test/value\0""999999999999\0RDONLYGUEST"),
    0
};

/**
 * The size of the buffer returned by the zeroth enumeration test -
 * the - 1 at the end is because of the hidden zero terminator
 */
static const uint32_t g_cbEnumBuffer0 =
    sizeof("test/name/\0test/value/\0""0\0\0"
           "test name\0test value\0""999\0TRANSIENT, READONLY\0"
           "TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST\0"
           "/test/name\0/test/value\0""999999999999\0RDONLYGUEST\0\0\0\0\0") - 1;

/** Result strings for first and second enumeration test */
static const char *g_apchEnumResult1[] =
{
    "TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST",
    "/test/name\0/test/value\0""999999999999\0RDONLYGUEST",
    NULL
};

/** Result string sizes for first and second enumeration test */
static const uint32_t g_acbEnumResult1[] =
{
    sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST"),
    sizeof("/test/name\0/test/value\0""999999999999\0RDONLYGUEST"),
    0
};

/**
 * The size of the buffer returned by the first enumeration test -
 * the - 1 at the end is because of the hidden zero terminator
 */
static const uint32_t g_cbEnumBuffer1 =
    sizeof("TEST NAME\0TEST VALUE\0""999999\0RDONLYHOST\0"
           "/test/name\0/test/value\0""999999999999\0RDONLYGUEST\0\0\0\0\0") - 1;

static const struct enumStringStruct
{
    /** The enumeration pattern to test */
    const char     *pszPatterns;
    /** The size of the pattern string */
    const uint32_t  cchPatterns;
    /** The expected enumeration output strings */
    const char    **papchResult;
    /** The size of the output strings */
    const uint32_t *pacchResult;
    /** The size of the buffer needed for the enumeration */
    const uint32_t  cbBuffer;
}   g_aEnumStrings[] =
{
    {
        "", sizeof(""),
        g_apchEnumResult0,
        g_acbEnumResult0,
        g_cbEnumBuffer0
    },
    {
        "/*\0?E*", sizeof("/*\0?E*"),
        g_apchEnumResult1,
        g_acbEnumResult1,
        g_cbEnumBuffer1
    },
    {
        "/*|?E*", sizeof("/*|?E*"),
        g_apchEnumResult1,
        g_acbEnumResult1,
        g_cbEnumBuffer1
    }
};

/**
 * Test the ENUM_PROPS_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testEnumPropsHost(VBOXHGCMSVCFNTABLE *ptable)
{
    RTTestISub("ENUM_PROPS_HOST");
    RTTESTI_CHECK_RETV(RT_VALID_PTR(ptable->pfnHostCall));

    for (unsigned i = 0; i < RT_ELEMENTS(g_aEnumStrings); ++i)
    {
        VBOXHGCMSVCPARM aParms[3];
        char            abBuffer[2048];
        RTTESTI_CHECK_RETV(g_aEnumStrings[i].cbBuffer < sizeof(abBuffer));

        /* Check that we get buffer overflow with a too small buffer. */
        aParms[0].setPointer((void *)g_aEnumStrings[i].pszPatterns, g_aEnumStrings[i].cchPatterns);
        aParms[1].setPointer((void *)abBuffer, g_aEnumStrings[i].cbBuffer - 1);
        memset(abBuffer, 0x55, sizeof(abBuffer));
        int rc2 = ptable->pfnHostCall(ptable->pvService, ENUM_PROPS_HOST, 3, aParms);
        if (rc2 == VERR_BUFFER_OVERFLOW)
        {
            uint32_t cbNeeded;
            RTTESTI_CHECK_RC(rc2 = aParms[2].getUInt32(&cbNeeded), VINF_SUCCESS);
            if (RT_SUCCESS(rc2))
                RTTESTI_CHECK_MSG(cbNeeded == g_aEnumStrings[i].cbBuffer,
                                  ("expected %u, got %u, pattern %d\n", g_aEnumStrings[i].cbBuffer, cbNeeded, i));
        }
        else
            RTTestIFailed("ENUM_PROPS_HOST returned %Rrc instead of VERR_BUFFER_OVERFLOW on too small buffer, pattern number %d.", rc2, i);

        /* Make a successfull call. */
        aParms[0].setPointer((void *)g_aEnumStrings[i].pszPatterns, g_aEnumStrings[i].cchPatterns);
        aParms[1].setPointer((void *)abBuffer, g_aEnumStrings[i].cbBuffer);
        memset(abBuffer, 0x55, sizeof(abBuffer));
        rc2 = ptable->pfnHostCall(ptable->pvService, ENUM_PROPS_HOST, 3, aParms);
        if (rc2 == VINF_SUCCESS)
        {
            /* Look for each of the result strings in the buffer which was returned */
            for (unsigned j = 0; g_aEnumStrings[i].papchResult[j] != NULL; ++j)
            {
                bool found = false;
                for (unsigned k = 0; !found && k <   g_aEnumStrings[i].cbBuffer
                                                   - g_aEnumStrings[i].pacchResult[j];
                     ++k)
                    if (memcmp(abBuffer + k, g_aEnumStrings[i].papchResult[j],
                        g_aEnumStrings[i].pacchResult[j]) == 0)
                        found = true;
                if (!found)
                    RTTestIFailed("ENUM_PROPS_HOST did not produce the expected output for pattern %d.", i);
            }
        }
        else
            RTTestIFailed("ENUM_PROPS_HOST returned %Rrc instead of VINF_SUCCESS, pattern number %d.", rc2, i);
    }
}

/**
 * Set a property by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable      the service instance handle
 * @param   pcszName    the name of the property to set
 * @param   pcszValue   the value to set the property to
 * @param   pcszFlags   the flag string to set if one of the SET_PROP[_HOST]
 *                      commands is used
 * @param   isHost      whether the SET_PROP[_VALUE]_HOST commands should be
 *                      used, rather than the guest ones
 * @param   useSetProp  whether SET_PROP[_HOST] should be used rather than
 *                      SET_PROP_VALUE[_HOST]
 */
int doSetProperty(VBOXHGCMSVCFNTABLE *pTable, const char *pcszName,
                  const char *pcszValue, const char *pcszFlags, bool isHost,
                  bool useSetProp)
{
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    int command = SET_PROP_VALUE;
    if (isHost)
    {
        if (useSetProp)
            command = SET_PROP_HOST;
        else
            command = SET_PROP_VALUE_HOST;
    }
    else if (useSetProp)
        command = SET_PROP;
    VBOXHGCMSVCPARM aParms[3];
    /* Work around silly constant issues - we ought to allow passing
     * constant strings in the hgcm parameters. */
    char szName[MAX_NAME_LEN];
    char szValue[MAX_VALUE_LEN];
    char szFlags[MAX_FLAGS_LEN];
    RTStrPrintf(szName, sizeof(szName), "%s", pcszName);
    RTStrPrintf(szValue, sizeof(szValue), "%s", pcszValue);
    RTStrPrintf(szFlags, sizeof(szFlags), "%s", pcszFlags);
    aParms[0].setString(szName);
    aParms[1].setString(szValue);
    aParms[2].setString(szFlags);
    if (isHost)
        callHandle.rc = pTable->pfnHostCall(pTable->pvService, command,
                                            useSetProp ? 3 : 2, aParms);
    else
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, command,
                        useSetProp ? 3 : 2, aParms);
    return callHandle.rc;
}

/**
 * Test the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST
 * functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testSetProp(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("SET_PROP, _VALUE, _HOST, _VALUE_HOST");

    /** Array of properties for testing SET_PROP_HOST and _GUEST. */
    static const struct
    {
        /** Property name */
        const char *pcszName;
        /** Property value */
        const char *pcszValue;
        /** Property flags */
        const char *pcszFlags;
        /** Should this be set as the host or the guest? */
        bool isHost;
        /** Should we use SET_PROP or SET_PROP_VALUE? */
        bool useSetProp;
        /** Should this succeed or be rejected with VERR_PERMISSION_DENIED? */
        bool isAllowed;
    }
    s_aSetProperties[] =
    {
        { "Red", "Stop!", "transient", false, true, true },
        { "Amber", "Caution!", "", false, false, true },
        { "Green", "Go!", "readonly", true, true, true },
        { "Blue", "What on earth...?", "", true, false, true },
        { "/test/name", "test", "", false, true, false },
        { "TEST NAME", "test", "", true, true, false },
        { "Green", "gone out...", "", false, false, false },
        { "Green", "gone out...", "", true, false, false },
        { "/VirtualBox/GuestAdd/SharedFolders/MountDir", "test", "", false, true, false },
        { "/VirtualBox/GuestAdd/SomethingElse", "test", "", false, true, true },
        { "/VirtualBox/HostInfo/VRDP/Client/1/Name", "test", "", false, false, false },
        { "/VirtualBox/GuestAdd/SharedFolders/MountDir", "test", "", true, true, true },
        { "/VirtualBox/HostInfo/VRDP/Client/1/Name", "test", "TRANSRESET", true, true, true },
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aSetProperties); ++i)
    {
        int rc = doSetProperty(pTable,
                               s_aSetProperties[i].pcszName,
                               s_aSetProperties[i].pcszValue,
                               s_aSetProperties[i].pcszFlags,
                               s_aSetProperties[i].isHost,
                               s_aSetProperties[i].useSetProp);
        if (s_aSetProperties[i].isAllowed && RT_FAILURE(rc))
            RTTestIFailed("Setting property '%s' failed with rc=%Rrc.",
                          s_aSetProperties[i].pcszName, rc);
        else if (   !s_aSetProperties[i].isAllowed
                 && rc != VERR_PERMISSION_DENIED)
            RTTestIFailed("Setting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.",
                          s_aSetProperties[i].pcszName, rc);
    }
}

/**
 * Delete a property by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable    the service instance handle
 * @param   pcszName  the name of the property to delete
 * @param   isHost    whether the DEL_PROP_HOST command should be used, rather
 *                    than the guest one
 */
static int doDelProp(VBOXHGCMSVCFNTABLE *pTable, const char *pcszName, bool isHost)
{
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle = { VINF_SUCCESS };
    int command = DEL_PROP;
    if (isHost)
        command = DEL_PROP_HOST;
    VBOXHGCMSVCPARM aParms[1];
    aParms[0].setString(pcszName);
    if (isHost)
        callHandle.rc = pTable->pfnHostCall(pTable->pvService, command, 1, aParms);
    else
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, command, 1, aParms);
    return callHandle.rc;
}

/**
 * Test the DEL_PROP, and DEL_PROP_HOST functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testDelProp(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("DEL_PROP, DEL_PROP_HOST");

    /** Array of properties for testing DEL_PROP_HOST and _GUEST. */
    static const struct
    {
        /** Property name */
        const char *pcszName;
        /** Should this be set as the host or the guest? */
        bool isHost;
        /** Should this succeed or be rejected with VERR_PERMISSION_DENIED? */
        bool isAllowed;
    }
    s_aDelProperties[] =
    {
        { "Red", false, true },
        { "Amber", true, true },
        { "Red2", false, true },
        { "Amber2", true, true },
        { "Green", false, false },
        { "Green", true, false },
        { "/test/name", false, false },
        { "TEST NAME", true, false },
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aDelProperties); ++i)
    {
        int rc = doDelProp(pTable, s_aDelProperties[i].pcszName,
                           s_aDelProperties[i].isHost);
        if (s_aDelProperties[i].isAllowed && RT_FAILURE(rc))
            RTTestIFailed("Deleting property '%s' failed with rc=%Rrc.",
                          s_aDelProperties[i].pcszName, rc);
        else if (   !s_aDelProperties[i].isAllowed
                 && rc != VERR_PERMISSION_DENIED )
            RTTestIFailed("Deleting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.",
                          s_aDelProperties[i].pcszName, rc);
    }
}

/**
 * Test the GET_PROP_HOST function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testGetProp(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("GET_PROP_HOST");

    /** Array of properties for testing GET_PROP_HOST. */
    static const struct
    {
        /** Property name */
        const char *pcszName;
        /** What value/flags pattern do we expect back? */
        const char *pchValue;
        /** What size should the value/flags array be? */
        uint32_t cchValue;
        /** Should this property exist? */
        bool exists;
        /** Do we expect a particular timestamp? */
        bool hasTimestamp;
        /** What timestamp if any do ex expect? */
        uint64_t u64Timestamp;
    }
    s_aGetProperties[] =
    {
        { "test/name/", "test/value/\0", sizeof("test/value/\0"), true, true, 0 },
        { "test name", "test value\0TRANSIENT, READONLY",
          sizeof("test value\0TRANSIENT, READONLY"), true, true, 999 },
        { "TEST NAME", "TEST VALUE\0RDONLYHOST", sizeof("TEST VALUE\0RDONLYHOST"),
          true, true, 999999 },
        { "/test/name", "/test/value\0RDONLYGUEST",
          sizeof("/test/value\0RDONLYGUEST"), true, true, UINT64_C(999999999999) },
        { "Green", "Go!\0READONLY", sizeof("Go!\0READONLY"), true, false, 0 },
        { "Blue", "What on earth...?\0", sizeof("What on earth...?\0"), true,
          false, 0 },
        { "Red", "", 0, false, false, 0 },
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aGetProperties); ++i)
    {
        VBOXHGCMSVCPARM aParms[4];
        /* Work around silly constant issues - we ought to allow passing
         * constant strings in the hgcm parameters. */
        char szBuffer[MAX_VALUE_LEN + MAX_FLAGS_LEN];
        RTTESTI_CHECK_RETV(s_aGetProperties[i].cchValue < sizeof(szBuffer));

        aParms[0].setString(s_aGetProperties[i].pcszName);
        memset(szBuffer, 0x55, sizeof(szBuffer));
        aParms[1].setPointer(szBuffer, sizeof(szBuffer));
        int rc2 = pTable->pfnHostCall(pTable->pvService, GET_PROP_HOST, 4, aParms);

        if (s_aGetProperties[i].exists && RT_FAILURE(rc2))
        {
            RTTestIFailed("Getting property '%s' failed with rc=%Rrc.",
                          s_aGetProperties[i].pcszName, rc2);
            continue;
        }

        if (!s_aGetProperties[i].exists && rc2 != VERR_NOT_FOUND)
        {
            RTTestIFailed("Getting property '%s' returned %Rrc instead of VERR_NOT_FOUND.",
                          s_aGetProperties[i].pcszName, rc2);
            continue;
        }

        if (s_aGetProperties[i].exists)
        {
            AssertRC(rc2);

            uint32_t u32ValueLen = UINT32_MAX;
            RTTESTI_CHECK_RC(rc2 = aParms[3].getUInt32(&u32ValueLen), VINF_SUCCESS);
            if (RT_SUCCESS(rc2))
            {
                RTTESTI_CHECK_MSG(u32ValueLen <= sizeof(szBuffer), ("u32ValueLen=%d", u32ValueLen));
                if (memcmp(szBuffer, s_aGetProperties[i].pchValue, s_aGetProperties[i].cchValue) != 0)
                    RTTestIFailed("Unexpected result '%.*s' for property '%s', expected '%.*s'.",
                                  u32ValueLen, szBuffer, s_aGetProperties[i].pcszName,
                                  s_aGetProperties[i].cchValue, s_aGetProperties[i].pchValue);
            }

            if (s_aGetProperties[i].hasTimestamp)
            {
                uint64_t u64Timestamp = UINT64_MAX;
                RTTESTI_CHECK_RC(rc2 = aParms[2].getUInt64(&u64Timestamp), VINF_SUCCESS);
                if (u64Timestamp != s_aGetProperties[i].u64Timestamp)
                    RTTestIFailed("Bad timestamp %llu for property '%s', expected %llu.",
                                  u64Timestamp, s_aGetProperties[i].pcszName,
                                  s_aGetProperties[i].u64Timestamp);
            }
        }
    }
}

/** Array of properties for testing GET_PROP_HOST. */
static const struct
{
    /** Buffer returned */
    const char *pchBuffer;
    /** What size should the buffer be? */
    uint32_t cbBuffer;
}
g_aGetNotifications[] =
{
    { "Red\0Stop!\0TRANSIENT", sizeof("Red\0Stop!\0TRANSIENT") },
    { "Amber\0Caution!\0", sizeof("Amber\0Caution!\0") },
    { "Green\0Go!\0READONLY", sizeof("Green\0Go!\0READONLY") },
    { "Blue\0What on earth...?\0", sizeof("Blue\0What on earth...?\0") },
    { "/VirtualBox/GuestAdd/SomethingElse\0test\0",
      sizeof("/VirtualBox/GuestAdd/SomethingElse\0test\0") },
    { "/VirtualBox/GuestAdd/SharedFolders/MountDir\0test\0RDONLYGUEST",
      sizeof("/VirtualBox/GuestAdd/SharedFolders/MountDir\0test\0RDONLYGUEST") },
    { "/VirtualBox/HostInfo/VRDP/Client/1/Name\0test\0TRANSIENT, RDONLYGUEST, TRANSRESET",
      sizeof("/VirtualBox/HostInfo/VRDP/Client/1/Name\0test\0TRANSIENT, RDONLYGUEST, TRANSRESET") },
    { "Red\0\0", sizeof("Red\0\0") },
    { "Amber\0\0", sizeof("Amber\0\0") },
};

/**
 * Test the GET_NOTIFICATION function.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testGetNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("GET_NOTIFICATION");

    /* Test "buffer too small" */
    static char                 s_szPattern[] = "";
    VBOXHGCMCALLHANDLE_TYPEDEF  callHandle = { VINF_SUCCESS };
    VBOXHGCMSVCPARM             aParms[4];
    uint32_t                    cbRetNeeded;

    for (uint32_t cbBuf = 1;
         cbBuf < g_aGetNotifications[0].cbBuffer - 1;
         cbBuf++)
    {
        void *pvBuf = RTTestGuardedAllocTail(g_hTest, cbBuf);
        RTTESTI_CHECK_BREAK(pvBuf);
        memset(pvBuf, 0x55, cbBuf);

        aParms[0].setPointer((void *)s_szPattern, sizeof(s_szPattern));
        aParms[1].setUInt64(1);
        aParms[2].setPointer(pvBuf, cbBuf);
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, GET_NOTIFICATION, 4, aParms);

        if (   callHandle.rc != VERR_BUFFER_OVERFLOW
            || RT_FAILURE(aParms[3].getUInt32(&cbRetNeeded))
            || cbRetNeeded != g_aGetNotifications[0].cbBuffer
           )
        {
            RTTestIFailed("Getting notification for property '%s' with a too small buffer did not fail correctly: %Rrc",
                          g_aGetNotifications[0].pchBuffer, callHandle.rc);
        }
        RTTestGuardedFree(g_hTest, pvBuf);
    }

    /* Test successful notification queries.  Start with an unknown timestamp
     * to get the oldest available notification. */
    uint64_t u64Timestamp = 1;
    for (unsigned i = 0; i < RT_ELEMENTS(g_aGetNotifications); ++i)
    {
        uint32_t cbBuf = g_aGetNotifications[i].cbBuffer + _1K;
        void *pvBuf = RTTestGuardedAllocTail(g_hTest, cbBuf);
        RTTESTI_CHECK_BREAK(pvBuf);
        memset(pvBuf, 0x55, cbBuf);

        aParms[0].setPointer((void *)s_szPattern, sizeof(s_szPattern));
        aParms[1].setUInt64(u64Timestamp);
        aParms[2].setPointer(pvBuf, cbBuf);
        pTable->pfnCall(pTable->pvService, &callHandle, 0, NULL, GET_NOTIFICATION, 4, aParms);
        if (   RT_FAILURE(callHandle.rc)
            || (i == 0 && callHandle.rc != VWRN_NOT_FOUND)
            || RT_FAILURE(aParms[1].getUInt64(&u64Timestamp))
            || RT_FAILURE(aParms[3].getUInt32(&cbRetNeeded))
            || cbRetNeeded != g_aGetNotifications[i].cbBuffer
            || memcmp(pvBuf, g_aGetNotifications[i].pchBuffer, cbRetNeeded) != 0
           )
        {
            RTTestIFailed("Failed to get notification for property '%s' (rc=%Rrc).",
                          g_aGetNotifications[i].pchBuffer, callHandle.rc);
        }
        RTTestGuardedFree(g_hTest, pvBuf);
    }
}

/** Parameters for the asynchronous guest notification call */
struct asyncNotification_
{
    /** Call parameters */
    VBOXHGCMSVCPARM aParms[4];
    /** Result buffer */
    char abBuffer[MAX_NAME_LEN + MAX_VALUE_LEN + MAX_FLAGS_LEN];
    /** Return value */
    VBOXHGCMCALLHANDLE_TYPEDEF callHandle;
} g_AsyncNotification;

/**
 * Set up the test for the asynchronous GET_NOTIFICATION function.
 */
static void setupAsyncNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("Async GET_NOTIFICATION without notifications");
    static char s_szPattern[] = "";

    g_AsyncNotification.aParms[0].setPointer((void *)s_szPattern, sizeof(s_szPattern));
    g_AsyncNotification.aParms[1].setUInt64(0);
    g_AsyncNotification.aParms[2].setPointer((void *)g_AsyncNotification.abBuffer,
                                             sizeof(g_AsyncNotification.abBuffer));
    g_AsyncNotification.callHandle.rc = VINF_HGCM_ASYNC_EXECUTE;
    pTable->pfnCall(pTable->pvService, &g_AsyncNotification.callHandle, 0, NULL,
                    GET_NOTIFICATION, 4, g_AsyncNotification.aParms);
    if (RT_FAILURE(g_AsyncNotification.callHandle.rc))
        RTTestIFailed("GET_NOTIFICATION call failed, rc=%Rrc.", g_AsyncNotification.callHandle.rc);
    else if (g_AsyncNotification.callHandle.rc != VINF_HGCM_ASYNC_EXECUTE)
        RTTestIFailed("GET_NOTIFICATION call completed when no new notifications should be available.");
}

/**
 * Test the asynchronous GET_NOTIFICATION function.
 */
static void testAsyncNotification(VBOXHGCMSVCFNTABLE *pTable)
{
    RT_NOREF1(pTable);
    uint64_t u64Timestamp;
    uint32_t u32Size;
    if (   g_AsyncNotification.callHandle.rc != VINF_SUCCESS
        || RT_FAILURE(g_AsyncNotification.aParms[1].getUInt64(&u64Timestamp))
        || RT_FAILURE(g_AsyncNotification.aParms[3].getUInt32(&u32Size))
        || u32Size != g_aGetNotifications[0].cbBuffer
        || memcmp(g_AsyncNotification.abBuffer, g_aGetNotifications[0].pchBuffer, u32Size) != 0
       )
    {
        RTTestIFailed("Asynchronous GET_NOTIFICATION call did not complete as expected, rc=%Rrc.",
                      g_AsyncNotification.callHandle.rc);
    }
}


static void test2(void)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    initTable(&svcTable, &svcHelpers);

    /* The function is inside the service, not HGCM. */
    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(&svcTable));

    testSetPropsHost(&svcTable);
    testEnumPropsHost(&svcTable);

    /* Set up the asynchronous notification test */
    setupAsyncNotification(&svcTable);
    testSetProp(&svcTable);
    RTTestISub("Async notification call data");
    testAsyncNotification(&svcTable); /* Our previous notification call should have completed by now. */

    testDelProp(&svcTable);
    testGetProp(&svcTable);
    testGetNotification(&svcTable);

    /* Cleanup */
    RTTESTI_CHECK_RC_OK(svcTable.pfnUnload(svcTable.pvService));
}

/**
 * Set the global flags value by calling the service
 * @returns the status returned by the call to the service
 *
 * @param   pTable  the service instance handle
 * @param   eFlags  the flags to set
 */
static int doSetGlobalFlags(VBOXHGCMSVCFNTABLE *pTable, ePropFlags eFlags)
{
    VBOXHGCMSVCPARM paParm;
    paParm.setUInt32(eFlags);
    int rc = pTable->pfnHostCall(pTable->pvService, SET_GLOBAL_FLAGS_HOST, 1, &paParm);
    if (RT_FAILURE(rc))
    {
        char szFlags[MAX_FLAGS_LEN];
        if (RT_FAILURE(writeFlags(eFlags, szFlags)))
            RTTestIFailed("Failed to set the global flags.");
        else
            RTTestIFailed("Failed to set the global flags \"%s\".", szFlags);
    }
    return rc;
}

/**
 * Test the SET_PROP, SET_PROP_VALUE, SET_PROP_HOST and SET_PROP_VALUE_HOST
 * functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testSetPropROGuest(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("global READONLYGUEST and SET_PROP*");

    /** Array of properties for testing SET_PROP_HOST and _GUEST with the
     * READONLYGUEST global flag set. */
    static const struct
    {
        /** Property name */
        const char *pcszName;
        /** Property value */
        const char *pcszValue;
        /** Property flags */
        const char *pcszFlags;
        /** Should this be set as the host or the guest? */
        bool isHost;
        /** Should we use SET_PROP or SET_PROP_VALUE? */
        bool useSetProp;
        /** Should this succeed or be rejected with VERR_ (NOT VINF_!)
         * PERMISSION_DENIED?  The global check is done after the property one. */
        bool isAllowed;
    }
    s_aSetPropertiesROGuest[] =
    {
        { "Red", "Stop!", "transient", false, true, true },
        { "Amber", "Caution!", "", false, false, true },
        { "Green", "Go!", "readonly", true, true, true },
        { "Blue", "What on earth...?", "", true, false, true },
        { "/test/name", "test", "", false, true, true },
        { "TEST NAME", "test", "", true, true, true },
        { "Green", "gone out...", "", false, false, false },
        { "Green", "gone out....", "", true, false, false },
    };

    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(pTable));
    int rc = doSetGlobalFlags(pTable, RDONLYGUEST);
    if (RT_SUCCESS(rc))
    {
        for (unsigned i = 0; i < RT_ELEMENTS(s_aSetPropertiesROGuest); ++i)
        {
            rc = doSetProperty(pTable, s_aSetPropertiesROGuest[i].pcszName,
                               s_aSetPropertiesROGuest[i].pcszValue,
                               s_aSetPropertiesROGuest[i].pcszFlags,
                               s_aSetPropertiesROGuest[i].isHost,
                               s_aSetPropertiesROGuest[i].useSetProp);
            if (s_aSetPropertiesROGuest[i].isAllowed && RT_FAILURE(rc))
                RTTestIFailed("Setting property '%s' to '%s' failed with rc=%Rrc.",
                              s_aSetPropertiesROGuest[i].pcszName,
                              s_aSetPropertiesROGuest[i].pcszValue, rc);
            else if (   !s_aSetPropertiesROGuest[i].isAllowed
                     && rc != VERR_PERMISSION_DENIED)
                RTTestIFailed("Setting property '%s' to '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.\n",
                              s_aSetPropertiesROGuest[i].pcszName,
                              s_aSetPropertiesROGuest[i].pcszValue, rc);
            else if (   !s_aSetPropertiesROGuest[i].isHost
                     && s_aSetPropertiesROGuest[i].isAllowed
                     && rc != VINF_PERMISSION_DENIED)
                RTTestIFailed("Setting property '%s' to '%s' returned %Rrc instead of VINF_PERMISSION_DENIED.\n",
                              s_aSetPropertiesROGuest[i].pcszName,
                              s_aSetPropertiesROGuest[i].pcszValue, rc);
        }
    }
    RTTESTI_CHECK_RC_OK(pTable->pfnUnload(pTable->pvService));
}

/**
 * Test the DEL_PROP, and DEL_PROP_HOST functions.
 * @returns iprt status value to indicate whether the test went as expected.
 * @note    prints its own diagnostic information to stdout.
 */
static void testDelPropROGuest(VBOXHGCMSVCFNTABLE *pTable)
{
    RTTestISub("global READONLYGUEST and DEL_PROP*");

    /** Array of properties for testing DEL_PROP_HOST and _GUEST with
     * READONLYGUEST set globally. */
    static const struct
    {
        /** Property name */
        const char *pcszName;
        /** Should this be deleted as the host (or the guest)? */
        bool isHost;
        /** Should this property be created first?  (As host, obviously) */
        bool shouldCreate;
        /** And with what flags? */
        const char *pcszFlags;
        /** Should this succeed or be rejected with VERR_ (NOT VINF_!)
         * PERMISSION_DENIED?  The global check is done after the property one. */
        bool isAllowed;
    }
    s_aDelPropertiesROGuest[] =
    {
        { "Red", true, true, "", true },
        { "Amber", false, true, "", true },
        { "Red2", true, false, "", true },
        { "Amber2", false, false, "", true },
        { "Red3", true, true, "READONLY", false },
        { "Amber3", false, true, "READONLY", false },
        { "Red4", true, true, "RDONLYHOST", false },
        { "Amber4", false, true, "RDONLYHOST", true },
    };

    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(pTable));
    int rc = doSetGlobalFlags(pTable, RDONLYGUEST);
    if (RT_SUCCESS(rc))
    {
        for (unsigned i = 0; i < RT_ELEMENTS(s_aDelPropertiesROGuest); ++i)
        {
            if (s_aDelPropertiesROGuest[i].shouldCreate)
                rc = doSetProperty(pTable, s_aDelPropertiesROGuest[i].pcszName,
                                   "none", s_aDelPropertiesROGuest[i].pcszFlags,
                                   true, true);
            rc = doDelProp(pTable, s_aDelPropertiesROGuest[i].pcszName,
                           s_aDelPropertiesROGuest[i].isHost);
            if (s_aDelPropertiesROGuest[i].isAllowed && RT_FAILURE(rc))
                RTTestIFailed("Deleting property '%s' failed with rc=%Rrc.",
                              s_aDelPropertiesROGuest[i].pcszName, rc);
            else if (   !s_aDelPropertiesROGuest[i].isAllowed
                     && rc != VERR_PERMISSION_DENIED)
                RTTestIFailed("Deleting property '%s' returned %Rrc instead of VERR_PERMISSION_DENIED.",
                              s_aDelPropertiesROGuest[i].pcszName, rc);
            else if (   !s_aDelPropertiesROGuest[i].isHost
                     && s_aDelPropertiesROGuest[i].shouldCreate
                     && s_aDelPropertiesROGuest[i].isAllowed
                     && rc != VINF_PERMISSION_DENIED)
                RTTestIFailed("Deleting property '%s' as guest returned %Rrc instead of VINF_PERMISSION_DENIED.",
                              s_aDelPropertiesROGuest[i].pcszName, rc);
        }
    }
    RTTESTI_CHECK_RC_OK(pTable->pfnUnload(pTable->pvService));
}

static void test3(void)
{
    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    initTable(&svcTable, &svcHelpers);
    testSetPropROGuest(&svcTable);
    testDelPropROGuest(&svcTable);
}

static void test4(void)
{
    RTTestISub("GET_PROP_HOST buffer handling");

    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    initTable(&svcTable, &svcHelpers);
    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(&svcTable));

    /* Insert a property that we can mess around with. */
    static char const s_szProp[]  = "/MyProperties/Sub/Sub/Sub/Sub/Sub/Sub/Sub/Property";
    static char const s_szValue[] = "Property Value";
    RTTESTI_CHECK_RC_OK(doSetProperty(&svcTable, s_szProp, s_szValue, "", true, true));


    /* Get the value with buffer sizes up to 1K.  */
    for (unsigned iVariation = 0; iVariation < 2; iVariation++)
    {
        for (uint32_t cbBuf = 0; cbBuf < _1K; cbBuf++)
        {
            void *pvBuf;
            RTTESTI_CHECK_RC_BREAK(RTTestGuardedAlloc(g_hTest, cbBuf, 1, iVariation == 0, &pvBuf), VINF_SUCCESS);

            VBOXHGCMSVCPARM aParms[4];
            aParms[0].setString(s_szProp);
            aParms[1].setPointer(pvBuf, cbBuf);
            svcTable.pfnHostCall(svcTable.pvService, GET_PROP_HOST, RT_ELEMENTS(aParms), aParms);

            RTTestGuardedFree(g_hTest, pvBuf);
        }
    }

    /* Done. */
    RTTESTI_CHECK_RC_OK(svcTable.pfnUnload(svcTable.pvService));
}

static void test5(void)
{
    RTTestISub("ENUM_PROPS_HOST buffer handling");

    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    initTable(&svcTable, &svcHelpers);
    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(&svcTable));

    /* Insert a few property that we can mess around with. */
    RTTESTI_CHECK_RC_OK(doSetProperty(&svcTable, "/MyProperties/Sub/Sub/Sub/Sub/Sub/Sub/Sub/Property", "Property Value", "", true, true));
    RTTESTI_CHECK_RC_OK(doSetProperty(&svcTable, "/MyProperties/12357",  "83848569", "", true, true));
    RTTESTI_CHECK_RC_OK(doSetProperty(&svcTable, "/MyProperties/56678",  "abcdefghijklm", "", true, true));
    RTTESTI_CHECK_RC_OK(doSetProperty(&svcTable, "/MyProperties/932769", "n", "", true, true));

    /* Get the value with buffer sizes up to 1K.  */
    for (unsigned iVariation = 0; iVariation < 2; iVariation++)
    {
        for (uint32_t cbBuf = 0; cbBuf < _1K; cbBuf++)
        {
            void *pvBuf;
            RTTESTI_CHECK_RC_BREAK(RTTestGuardedAlloc(g_hTest, cbBuf, 1, iVariation == 0, &pvBuf), VINF_SUCCESS);

            VBOXHGCMSVCPARM aParms[3];
            aParms[0].setString("*");
            aParms[1].setPointer(pvBuf, cbBuf);
            svcTable.pfnHostCall(svcTable.pvService, ENUM_PROPS_HOST, RT_ELEMENTS(aParms), aParms);

            RTTestGuardedFree(g_hTest, pvBuf);
        }
    }

    /* Done. */
    RTTESTI_CHECK_RC_OK(svcTable.pfnUnload(svcTable.pvService));
}

static void test6(void)
{
    RTTestISub("Max properties");

    VBOXHGCMSVCFNTABLE  svcTable;
    VBOXHGCMSVCHELPERS  svcHelpers;
    initTable(&svcTable, &svcHelpers);
    RTTESTI_CHECK_RC_OK_RETV(VBoxHGCMSvcLoad(&svcTable));

    /* Insert the max number of properties. */
    static char const   s_szPropFmt[] = "/MyProperties/Sub/Sub/Sub/Sub/Sub/Sub/Sub/PropertyNo#%u";
    char                szProp[80];
    unsigned            cProps = 0;
    for (;;)
    {
        RTStrPrintf(szProp, sizeof(szProp), s_szPropFmt, cProps);
        int rc = doSetProperty(&svcTable, szProp, "myvalue", "", true, true);
        if (rc == VERR_TOO_MUCH_DATA)
            break;
        if (RT_FAILURE(rc))
        {
            RTTestIFailed("Unexpected error %Rrc setting property number %u", rc, cProps);
            break;
        }
        cProps++;
    }
    RTTestIValue("Max Properties", cProps, RTTESTUNIT_OCCURRENCES);

    /* Touch them all again. */
    for (unsigned iProp = 0; iProp < cProps; iProp++)
    {
        RTStrPrintf(szProp, sizeof(szProp), s_szPropFmt, iProp);
        int rc;
        RTTESTI_CHECK_MSG((rc = doSetProperty(&svcTable, szProp, "myvalue", "", true, true)) == VINF_SUCCESS,
                          ("%Rrc - #%u\n", rc, iProp));
        RTTESTI_CHECK_MSG((rc = doSetProperty(&svcTable, szProp, "myvalue", "", true, false)) == VINF_SUCCESS,
                          ("%Rrc - #%u\n", rc, iProp));
        RTTESTI_CHECK_MSG((rc = doSetProperty(&svcTable, szProp, "myvalue", "", false, true)) == VINF_SUCCESS,
                          ("%Rrc - #%u\n", rc, iProp));
        RTTESTI_CHECK_MSG((rc = doSetProperty(&svcTable, szProp, "myvalue", "", false, false)) == VINF_SUCCESS,
                          ("%Rrc - #%u\n", rc, iProp));
    }

    /* Benchmark. */
    uint64_t cNsMax = 0;
    uint64_t cNsMin = UINT64_MAX;
    uint64_t cNsAvg = 0;
    for (unsigned iProp = 0; iProp < cProps; iProp++)
    {
        size_t cchProp = RTStrPrintf(szProp, sizeof(szProp), s_szPropFmt, iProp);

        uint64_t cNsElapsed = RTTimeNanoTS();
        unsigned iCall;
        for (iCall = 0; iCall < 1000; iCall++)
        {
            VBOXHGCMSVCPARM aParms[4];
            char            szBuffer[256];
            aParms[0].setPointer(szProp, (uint32_t)cchProp + 1);
            aParms[1].setPointer(szBuffer, sizeof(szBuffer));
            RTTESTI_CHECK_RC_BREAK(svcTable.pfnHostCall(svcTable.pvService, GET_PROP_HOST, 4, aParms), VINF_SUCCESS);
        }
        cNsElapsed = RTTimeNanoTS() - cNsElapsed;
        if (iCall)
        {
            uint64_t cNsPerCall = cNsElapsed / iCall;
            cNsAvg += cNsPerCall;
            if (cNsPerCall < cNsMin)
                cNsMin = cNsPerCall;
            if (cNsPerCall > cNsMax)
                cNsMax = cNsPerCall;
        }
    }
    if (cProps)
        cNsAvg /= cProps;
    RTTestIValue("GET_PROP_HOST Min", cNsMin, RTTESTUNIT_NS_PER_CALL);
    RTTestIValue("GET_PROP_HOST Avg", cNsAvg, RTTESTUNIT_NS_PER_CALL);
    RTTestIValue("GET_PROP_HOST Max", cNsMax, RTTESTUNIT_NS_PER_CALL);

    /* Done. */
    RTTESTI_CHECK_RC_OK(svcTable.pfnUnload(svcTable.pvService));
}




int main()
{
    RTEXITCODE rcExit = RTTestInitAndCreate("tstGuestPropSvc", &g_hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(g_hTest);

    testConvertFlags();
    test2();
    test3();
    test4();
    test5();
    test6();

    return RTTestSummaryAndDestroy(g_hTest);
}
