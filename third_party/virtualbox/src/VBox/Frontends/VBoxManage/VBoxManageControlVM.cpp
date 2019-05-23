/* $Id: VBoxManageControlVM.cpp $ */
/** @file
 * VBoxManage - Implementation of the controlvm command.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/ctype.h>
#include <VBox/err.h>
#include <iprt/getopt.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/uuid.h>
#include <iprt/file.h>
#include <VBox/log.h>

#include "VBoxManage.h"

#include <list>


/**
 * Parses a number.
 *
 * @returns Valid number on success.
 * @returns 0 if invalid number. All necessary bitching has been done.
 * @param   psz     Pointer to the nic number.
 */
static unsigned parseNum(const char *psz, unsigned cMaxNum, const char *name)
{
    uint32_t u32;
    char *pszNext;
    int rc = RTStrToUInt32Ex(psz, &pszNext, 10, &u32);
    if (    RT_SUCCESS(rc)
        &&  *pszNext == '\0'
        &&  u32 >= 1
        &&  u32 <= cMaxNum)
        return (unsigned)u32;
    errorArgument("Invalid %s number '%s'", name, psz);
    return 0;
}

unsigned int getMaxNics(IVirtualBox* vbox, IMachine* mach)
{
    ComPtr<ISystemProperties> info;
    ChipsetType_T aChipset;
    ULONG NetworkAdapterCount = 0;
    HRESULT rc;

    do {
        CHECK_ERROR_BREAK(vbox, COMGETTER(SystemProperties)(info.asOutParam()));
        CHECK_ERROR_BREAK(mach, COMGETTER(ChipsetType)(&aChipset));
        CHECK_ERROR_BREAK(info, GetMaxNetworkAdapters(aChipset, &NetworkAdapterCount));

        return (unsigned int)NetworkAdapterCount;
    } while (0);

    return 0;
}

#define KBDCHARDEF_MOD_NONE  0x00
#define KBDCHARDEF_MOD_SHIFT 0x01

typedef struct KBDCHARDEF
{
    uint8_t u8Scancode;
    uint8_t u8Modifiers;
} KBDCHARDEF;

static const KBDCHARDEF g_aASCIIChars[0x80] =
{
    /* 0x00 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x01 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x02 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x03 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x04 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x05 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x06 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x07 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x08 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x09 ' ' */ {0x0f, KBDCHARDEF_MOD_NONE},
    /* 0x0A ' ' */ {0x1c, KBDCHARDEF_MOD_NONE},
    /* 0x0B ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x0C ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x0D ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x0E ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x0F ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x10 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x11 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x12 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x13 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x14 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x15 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x16 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x17 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x18 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x19 ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1A ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1B ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1C ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1D ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1E ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x1F ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
    /* 0x20 ' ' */ {0x39, KBDCHARDEF_MOD_NONE},
    /* 0x21 '!' */ {0x02, KBDCHARDEF_MOD_SHIFT},
    /* 0x22 '"' */ {0x28, KBDCHARDEF_MOD_SHIFT},
    /* 0x23 '#' */ {0x04, KBDCHARDEF_MOD_SHIFT},
    /* 0x24 '$' */ {0x05, KBDCHARDEF_MOD_SHIFT},
    /* 0x25 '%' */ {0x06, KBDCHARDEF_MOD_SHIFT},
    /* 0x26 '&' */ {0x08, KBDCHARDEF_MOD_SHIFT},
    /* 0x27 ''' */ {0x28, KBDCHARDEF_MOD_NONE},
    /* 0x28 '(' */ {0x0a, KBDCHARDEF_MOD_SHIFT},
    /* 0x29 ')' */ {0x0b, KBDCHARDEF_MOD_SHIFT},
    /* 0x2A '*' */ {0x09, KBDCHARDEF_MOD_SHIFT},
    /* 0x2B '+' */ {0x0d, KBDCHARDEF_MOD_SHIFT},
    /* 0x2C ',' */ {0x33, KBDCHARDEF_MOD_NONE},
    /* 0x2D '-' */ {0x0c, KBDCHARDEF_MOD_NONE},
    /* 0x2E '.' */ {0x34, KBDCHARDEF_MOD_NONE},
    /* 0x2F '/' */ {0x35, KBDCHARDEF_MOD_NONE},
    /* 0x30 '0' */ {0x0b, KBDCHARDEF_MOD_NONE},
    /* 0x31 '1' */ {0x02, KBDCHARDEF_MOD_NONE},
    /* 0x32 '2' */ {0x03, KBDCHARDEF_MOD_NONE},
    /* 0x33 '3' */ {0x04, KBDCHARDEF_MOD_NONE},
    /* 0x34 '4' */ {0x05, KBDCHARDEF_MOD_NONE},
    /* 0x35 '5' */ {0x06, KBDCHARDEF_MOD_NONE},
    /* 0x36 '6' */ {0x07, KBDCHARDEF_MOD_NONE},
    /* 0x37 '7' */ {0x08, KBDCHARDEF_MOD_NONE},
    /* 0x38 '8' */ {0x09, KBDCHARDEF_MOD_NONE},
    /* 0x39 '9' */ {0x0a, KBDCHARDEF_MOD_NONE},
    /* 0x3A ':' */ {0x27, KBDCHARDEF_MOD_SHIFT},
    /* 0x3B ';' */ {0x27, KBDCHARDEF_MOD_NONE},
    /* 0x3C '<' */ {0x33, KBDCHARDEF_MOD_SHIFT},
    /* 0x3D '=' */ {0x0d, KBDCHARDEF_MOD_NONE},
    /* 0x3E '>' */ {0x34, KBDCHARDEF_MOD_SHIFT},
    /* 0x3F '?' */ {0x35, KBDCHARDEF_MOD_SHIFT},
    /* 0x40 '@' */ {0x03, KBDCHARDEF_MOD_SHIFT},
    /* 0x41 'A' */ {0x1e, KBDCHARDEF_MOD_SHIFT},
    /* 0x42 'B' */ {0x30, KBDCHARDEF_MOD_SHIFT},
    /* 0x43 'C' */ {0x2e, KBDCHARDEF_MOD_SHIFT},
    /* 0x44 'D' */ {0x20, KBDCHARDEF_MOD_SHIFT},
    /* 0x45 'E' */ {0x12, KBDCHARDEF_MOD_SHIFT},
    /* 0x46 'F' */ {0x21, KBDCHARDEF_MOD_SHIFT},
    /* 0x47 'G' */ {0x22, KBDCHARDEF_MOD_SHIFT},
    /* 0x48 'H' */ {0x23, KBDCHARDEF_MOD_SHIFT},
    /* 0x49 'I' */ {0x17, KBDCHARDEF_MOD_SHIFT},
    /* 0x4A 'J' */ {0x24, KBDCHARDEF_MOD_SHIFT},
    /* 0x4B 'K' */ {0x25, KBDCHARDEF_MOD_SHIFT},
    /* 0x4C 'L' */ {0x26, KBDCHARDEF_MOD_SHIFT},
    /* 0x4D 'M' */ {0x32, KBDCHARDEF_MOD_SHIFT},
    /* 0x4E 'N' */ {0x31, KBDCHARDEF_MOD_SHIFT},
    /* 0x4F 'O' */ {0x18, KBDCHARDEF_MOD_SHIFT},
    /* 0x50 'P' */ {0x19, KBDCHARDEF_MOD_SHIFT},
    /* 0x51 'Q' */ {0x10, KBDCHARDEF_MOD_SHIFT},
    /* 0x52 'R' */ {0x13, KBDCHARDEF_MOD_SHIFT},
    /* 0x53 'S' */ {0x1f, KBDCHARDEF_MOD_SHIFT},
    /* 0x54 'T' */ {0x14, KBDCHARDEF_MOD_SHIFT},
    /* 0x55 'U' */ {0x16, KBDCHARDEF_MOD_SHIFT},
    /* 0x56 'V' */ {0x2f, KBDCHARDEF_MOD_SHIFT},
    /* 0x57 'W' */ {0x11, KBDCHARDEF_MOD_SHIFT},
    /* 0x58 'X' */ {0x2d, KBDCHARDEF_MOD_SHIFT},
    /* 0x59 'Y' */ {0x15, KBDCHARDEF_MOD_SHIFT},
    /* 0x5A 'Z' */ {0x2c, KBDCHARDEF_MOD_SHIFT},
    /* 0x5B '[' */ {0x1a, KBDCHARDEF_MOD_NONE},
    /* 0x5C '\' */ {0x2b, KBDCHARDEF_MOD_NONE},
    /* 0x5D ']' */ {0x1b, KBDCHARDEF_MOD_NONE},
    /* 0x5E '^' */ {0x07, KBDCHARDEF_MOD_SHIFT},
    /* 0x5F '_' */ {0x0c, KBDCHARDEF_MOD_SHIFT},
    /* 0x60 '`' */ {0x28, KBDCHARDEF_MOD_NONE},
    /* 0x61 'a' */ {0x1e, KBDCHARDEF_MOD_NONE},
    /* 0x62 'b' */ {0x30, KBDCHARDEF_MOD_NONE},
    /* 0x63 'c' */ {0x2e, KBDCHARDEF_MOD_NONE},
    /* 0x64 'd' */ {0x20, KBDCHARDEF_MOD_NONE},
    /* 0x65 'e' */ {0x12, KBDCHARDEF_MOD_NONE},
    /* 0x66 'f' */ {0x21, KBDCHARDEF_MOD_NONE},
    /* 0x67 'g' */ {0x22, KBDCHARDEF_MOD_NONE},
    /* 0x68 'h' */ {0x23, KBDCHARDEF_MOD_NONE},
    /* 0x69 'i' */ {0x17, KBDCHARDEF_MOD_NONE},
    /* 0x6A 'j' */ {0x24, KBDCHARDEF_MOD_NONE},
    /* 0x6B 'k' */ {0x25, KBDCHARDEF_MOD_NONE},
    /* 0x6C 'l' */ {0x26, KBDCHARDEF_MOD_NONE},
    /* 0x6D 'm' */ {0x32, KBDCHARDEF_MOD_NONE},
    /* 0x6E 'n' */ {0x31, KBDCHARDEF_MOD_NONE},
    /* 0x6F 'o' */ {0x18, KBDCHARDEF_MOD_NONE},
    /* 0x70 'p' */ {0x19, KBDCHARDEF_MOD_NONE},
    /* 0x71 'q' */ {0x10, KBDCHARDEF_MOD_NONE},
    /* 0x72 'r' */ {0x13, KBDCHARDEF_MOD_NONE},
    /* 0x73 's' */ {0x1f, KBDCHARDEF_MOD_NONE},
    /* 0x74 't' */ {0x14, KBDCHARDEF_MOD_NONE},
    /* 0x75 'u' */ {0x16, KBDCHARDEF_MOD_NONE},
    /* 0x76 'v' */ {0x2f, KBDCHARDEF_MOD_NONE},
    /* 0x77 'w' */ {0x11, KBDCHARDEF_MOD_NONE},
    /* 0x78 'x' */ {0x2d, KBDCHARDEF_MOD_NONE},
    /* 0x79 'y' */ {0x15, KBDCHARDEF_MOD_NONE},
    /* 0x7A 'z' */ {0x2c, KBDCHARDEF_MOD_NONE},
    /* 0x7B '{' */ {0x1a, KBDCHARDEF_MOD_SHIFT},
    /* 0x7C '|' */ {0x2b, KBDCHARDEF_MOD_SHIFT},
    /* 0x7D '}' */ {0x1b, KBDCHARDEF_MOD_SHIFT},
    /* 0x7E '~' */ {0x29, KBDCHARDEF_MOD_SHIFT},
    /* 0x7F ' ' */ {0x00, KBDCHARDEF_MOD_NONE},
};

static HRESULT keyboardPutScancodes(IKeyboard *pKeyboard, const std::list<LONG> &llScancodes)
{
    /* Send scancodes to the VM. */
    com::SafeArray<LONG> saScancodes(llScancodes);

#if 1
    HRESULT rc = S_OK;
    size_t i;
    for (i = 0; i < saScancodes.size(); ++i)
    {
        rc = pKeyboard->PutScancode(saScancodes[i]);
        if (FAILED(rc))
        {
            RTMsgError("Failed to send a scancode");
            break;
        }

        RTThreadSleep(10); /* "Typing" too fast causes lost characters. */
    }
#else
    /** @todo PutScancodes does not deliver more than 20 scancodes. */
    ULONG codesStored = 0;
    HRESULT rc = pKeyboard->PutScancodes(ComSafeArrayAsInParam(saScancodes),
                                         &codesStored);
    if (SUCCEEDED(rc) && codesStored < saScancodes.size())
    {
        RTMsgError("Only %d scancodes were stored", codesStored);
        rc = E_FAIL;
    }
#endif

    return rc;
}

static void keyboardCharsToScancodes(const char *pch, size_t cchMax, std::list<LONG> &llScancodes, bool *pfShift)
{
    size_t cchProcessed = 0;
    const char *p = pch;
    while (cchProcessed < cchMax)
    {
        ++cchProcessed;
        const uint8_t c = (uint8_t)*p++;
        if (c < RT_ELEMENTS(g_aASCIIChars))
        {
            const KBDCHARDEF *d = &g_aASCIIChars[c];
            if (d->u8Scancode)
            {
                const bool fNeedShift = RT_BOOL(d->u8Modifiers & KBDCHARDEF_MOD_SHIFT);
                if (*pfShift != fNeedShift)
                {
                    *pfShift = fNeedShift;
                    /* Press or release the SHIFT key. */
                    llScancodes.push_back(0x2a | (fNeedShift? 0x00: 0x80));
                }

                llScancodes.push_back(d->u8Scancode);
                llScancodes.push_back(d->u8Scancode | 0x80);
            }
        }
    }
}

static HRESULT keyboardPutString(IKeyboard *pKeyboard, int argc, char **argv)
{
    std::list<LONG> llScancodes;
    bool fShift = false;

    /* Convert command line string(s) to the en-us keyboard scancodes. */
    int i;
    for (i = 1 + 1; i < argc; ++i)
    {
        if (llScancodes.size() > 0)
        {
            /* Insert a SPACE before the next string. */
            llScancodes.push_back(0x39);
            llScancodes.push_back(0x39 | 0x80);
        }

        keyboardCharsToScancodes(argv[i], strlen(argv[i]), llScancodes, &fShift);
    }

    /* Release SHIFT if pressed. */
    if (fShift)
        llScancodes.push_back(0x2a | 0x80);

    return keyboardPutScancodes(pKeyboard, llScancodes);
}

static HRESULT keyboardPutFile(IKeyboard *pKeyboard, const char *pszFilename)
{
    std::list<LONG> llScancodes;
    bool fShift = false;

    RTFILE File = NIL_RTFILE;
    int vrc = RTFileOpen(&File, pszFilename, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
    if (RT_SUCCESS(vrc))
    {
        uint64_t cbFile = 0;
        vrc = RTFileGetSize(File, &cbFile);
        if (RT_SUCCESS(vrc))
        {
            const uint64_t cbFileMax = _64K;
            if (cbFile <= cbFileMax)
            {
                const size_t cbBuffer = _4K;
                char *pchBuf = (char *)RTMemAlloc(cbBuffer);
                if (pchBuf)
                {
                    size_t cbRemaining = (size_t)cbFile;
                    while (cbRemaining > 0)
                    {
                        const size_t cbToRead = cbRemaining > cbBuffer ? cbBuffer : cbRemaining;

                        size_t cbRead = 0;
                        vrc = RTFileRead(File, pchBuf, cbToRead, &cbRead);
                        if (RT_FAILURE(vrc) || cbRead == 0)
                            break;

                        keyboardCharsToScancodes(pchBuf, cbRead, llScancodes, &fShift);
                        cbRemaining -= cbRead;
                    }

                    RTMemFree(pchBuf);
                }
                else
                    RTMsgError("Out of memory allocating %d bytes", cbBuffer);
            }
            else
                RTMsgError("File size %RI64 is greater than %RI64: '%s'", cbFile, cbFileMax, pszFilename);
        }
        else
            RTMsgError("Cannot get size of file '%s': %Rrc", pszFilename, vrc);

        RTFileClose(File);
    }
    else
        RTMsgError("Cannot open file '%s': %Rrc", pszFilename, vrc);

    /* Release SHIFT if pressed. */
    if (fShift)
        llScancodes.push_back(0x2a | 0x80);

    return keyboardPutScancodes(pKeyboard, llScancodes);
}


RTEXITCODE handleControlVM(HandlerArg *a)
{
    using namespace com;
    bool fNeedsSaving = false;
    HRESULT rc;

    if (a->argc < 2)
        return errorSyntax(USAGE_CONTROLVM, "Not enough parameters");

    /* try to find the given machine */
    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (FAILED(rc))
        return RTEXITCODE_FAILURE;

    /* open a session for the VM */
    CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);

    ComPtr<IConsole> console;
    ComPtr<IMachine> sessionMachine;

    do
    {
        /* get the associated console */
        CHECK_ERROR_BREAK(a->session, COMGETTER(Console)(console.asOutParam()));
        /* ... and session machine */
        CHECK_ERROR_BREAK(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()));

        if (!console)
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Machine '%s' is not currently running", a->argv[0]);

        /* which command? */
        if (!strcmp(a->argv[1], "pause"))
        {
            CHECK_ERROR_BREAK(console, Pause());
        }
        else if (!strcmp(a->argv[1], "resume"))
        {
            CHECK_ERROR_BREAK(console, Resume());
        }
        else if (!strcmp(a->argv[1], "reset"))
        {
            CHECK_ERROR_BREAK(console, Reset());
        }
        else if (!strcmp(a->argv[1], "unplugcpu"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected CPU number.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            unsigned n = parseNum(a->argv[2], 32, "CPU");

            CHECK_ERROR_BREAK(sessionMachine, HotUnplugCPU(n));
        }
        else if (!strcmp(a->argv[1], "plugcpu"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected CPU number.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            unsigned n = parseNum(a->argv[2], 32, "CPU");

            CHECK_ERROR_BREAK(sessionMachine, HotPlugCPU(n));
        }
        else if (!strcmp(a->argv[1], "cpuexecutioncap"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected execution cap number.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            unsigned n = parseNum(a->argv[2], 100, "ExecutionCap");

            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(CPUExecutionCap)(n));
        }
        else if (!strcmp(a->argv[1], "audioin"))
        {
            ComPtr<IAudioAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, COMGETTER(AudioAdapter)(adapter.asOutParam()));
            if (adapter)
            {
                if (!strcmp(a->argv[2], "on"))
                {
                    CHECK_ERROR_RET(adapter, COMSETTER(EnabledIn)(TRUE), RTEXITCODE_FAILURE);
                }
                else if (!strcmp(a->argv[2], "off"))
                {
                    CHECK_ERROR_RET(adapter, COMSETTER(EnabledIn)(FALSE), RTEXITCODE_FAILURE);
                }
                else
                {
                    errorArgument("Invalid value '%s'", Utf8Str(a->argv[2]).c_str());
                    rc = E_FAIL;
                    break;
                }
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
            else
            {
                errorArgument("audio adapter not enabled in VM configuration");
                rc = E_FAIL;
                break;
            }
        }
        else if (!strcmp(a->argv[1], "audioout"))
        {
            ComPtr<IAudioAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, COMGETTER(AudioAdapter)(adapter.asOutParam()));
            if (adapter)
            {
                if (!strcmp(a->argv[2], "on"))
                {
                    CHECK_ERROR_RET(adapter, COMSETTER(EnabledOut)(TRUE), RTEXITCODE_FAILURE);
                }
                else if (!strcmp(a->argv[2], "off"))
                {
                    CHECK_ERROR_RET(adapter, COMSETTER(EnabledOut)(FALSE), RTEXITCODE_FAILURE);
                }
                else
                {
                    errorArgument("Invalid value '%s'", Utf8Str(a->argv[2]).c_str());
                    rc = E_FAIL;
                    break;
                }
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
            else
            {
                errorArgument("audio adapter not enabled in VM configuration");
                rc = E_FAIL;
                break;
            }
        }
        else if (!strcmp(a->argv[1], "clipboard"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected clipboard mode.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            ClipboardMode_T mode = ClipboardMode_Disabled; /* Shut up MSC */
            if (!strcmp(a->argv[2], "disabled"))
                mode = ClipboardMode_Disabled;
            else if (!strcmp(a->argv[2], "hosttoguest"))
                mode = ClipboardMode_HostToGuest;
            else if (!strcmp(a->argv[2], "guesttohost"))
                mode = ClipboardMode_GuestToHost;
            else if (!strcmp(a->argv[2], "bidirectional"))
                mode = ClipboardMode_Bidirectional;
            else
            {
                errorArgument("Invalid '%s' argument '%s'.", a->argv[1], a->argv[2]);
                rc = E_FAIL;
            }
            if (SUCCEEDED(rc))
            {
                CHECK_ERROR_BREAK(sessionMachine, COMSETTER(ClipboardMode)(mode));
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        else if (!strcmp(a->argv[1], "draganddrop"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected drag and drop mode.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            DnDMode_T mode = DnDMode_Disabled; /* Shup up MSC. */
            if (!strcmp(a->argv[2], "disabled"))
                mode = DnDMode_Disabled;
            else if (!strcmp(a->argv[2], "hosttoguest"))
                mode = DnDMode_HostToGuest;
            else if (!strcmp(a->argv[2], "guesttohost"))
                mode = DnDMode_GuestToHost;
            else if (!strcmp(a->argv[2], "bidirectional"))
                mode = DnDMode_Bidirectional;
            else
            {
                errorArgument("Invalid '%s' argument '%s'.", a->argv[1], a->argv[2]);
                rc = E_FAIL;
            }
            if (SUCCEEDED(rc))
            {
                CHECK_ERROR_BREAK(sessionMachine, COMSETTER(DnDMode)(mode));
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        else if (!strcmp(a->argv[1], "poweroff"))
        {
            ComPtr<IProgress> progress;
            CHECK_ERROR_BREAK(console, PowerDown(progress.asOutParam()));

            rc = showProgress(progress);
            CHECK_PROGRESS_ERROR(progress, ("Failed to power off machine"));
        }
        else if (!strcmp(a->argv[1], "savestate"))
        {
            /* first pause so we don't trigger a live save which needs more time/resources */
            bool fPaused = false;
            rc = console->Pause();
            if (FAILED(rc))
            {
                bool fError = true;
                if (rc == VBOX_E_INVALID_VM_STATE)
                {
                    /* check if we are already paused */
                    MachineState_T machineState;
                    CHECK_ERROR_BREAK(console, COMGETTER(State)(&machineState));
                    /* the error code was lost by the previous instruction */
                    rc = VBOX_E_INVALID_VM_STATE;
                    if (machineState != MachineState_Paused)
                    {
                        RTMsgError("Machine in invalid state %d -- %s\n",
                                   machineState, machineStateToName(machineState, false));
                    }
                    else
                    {
                        fError = false;
                        fPaused = true;
                    }
                }
                if (fError)
                    break;
            }

            ComPtr<IProgress> progress;
            CHECK_ERROR(sessionMachine, SaveState(progress.asOutParam()));
            if (FAILED(rc))
            {
                if (!fPaused)
                    console->Resume();
                break;
            }

            rc = showProgress(progress);
            CHECK_PROGRESS_ERROR(progress, ("Failed to save machine state"));
            if (FAILED(rc))
            {
                if (!fPaused)
                    console->Resume();
            }
        }
        else if (!strcmp(a->argv[1], "acpipowerbutton"))
        {
            CHECK_ERROR_BREAK(console, PowerButton());
        }
        else if (!strcmp(a->argv[1], "acpisleepbutton"))
        {
            CHECK_ERROR_BREAK(console, SleepButton());
        }
        else if (!strcmp(a->argv[1], "keyboardputscancode"))
        {
            ComPtr<IKeyboard> pKeyboard;
            CHECK_ERROR_BREAK(console, COMGETTER(Keyboard)(pKeyboard.asOutParam()));
            if (!pKeyboard)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected IBM PC AT set 2 keyboard scancode(s) as hex byte(s).", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            std::list<LONG> llScancodes;

            /* Process the command line. */
            int i;
            for (i = 1 + 1; i < a->argc; i++)
            {
                if (   RT_C_IS_XDIGIT (a->argv[i][0])
                    && RT_C_IS_XDIGIT (a->argv[i][1])
                    && a->argv[i][2] == 0)
                {
                    uint8_t u8Scancode;
                    int irc = RTStrToUInt8Ex(a->argv[i], NULL, 16, &u8Scancode);
                    if (RT_FAILURE (irc))
                    {
                        RTMsgError("Converting '%s' returned %Rrc!", a->argv[i], rc);
                        rc = E_FAIL;
                        break;
                    }

                    llScancodes.push_back(u8Scancode);
                }
                else
                {
                    RTMsgError("Error: '%s' is not a hex byte!", a->argv[i]);
                    rc = E_FAIL;
                    break;
                }
            }

            if (FAILED(rc))
                break;

            rc = keyboardPutScancodes(pKeyboard, llScancodes);
        }
        else if (!strcmp(a->argv[1], "keyboardputstring"))
        {
            ComPtr<IKeyboard> pKeyboard;
            CHECK_ERROR_BREAK(console, COMGETTER(Keyboard)(pKeyboard.asOutParam()));
            if (!pKeyboard)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected ASCII string(s).", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            rc = keyboardPutString(pKeyboard, a->argc, a->argv);
        }
        else if (!strcmp(a->argv[1], "keyboardputfile"))
        {
            ComPtr<IKeyboard> pKeyboard;
            CHECK_ERROR_BREAK(console, COMGETTER(Keyboard)(pKeyboard.asOutParam()));
            if (!pKeyboard)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'. Expected file name.", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            rc = keyboardPutFile(pKeyboard, a->argv[2]);
        }
        else if (!strncmp(a->argv[1], "setlinkstate", 12))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][12], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }
            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                if (!strcmp(a->argv[2], "on"))
                {
                    CHECK_ERROR_BREAK(adapter, COMSETTER(CableConnected)(TRUE));
                }
                else if (!strcmp(a->argv[2], "off"))
                {
                    CHECK_ERROR_BREAK(adapter, COMSETTER(CableConnected)(FALSE));
                }
                else
                {
                    errorArgument("Invalid link state '%s'", Utf8Str(a->argv[2]).c_str());
                    rc = E_FAIL;
                    break;
                }
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        /* here the order in which strncmp is called is important
         * cause nictracefile can be very well compared with
         * nictrace and nic and thus everything will always fail
         * if the order is changed
         */
        else if (!strncmp(a->argv[1], "nictracefile", 12))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][12], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                BOOL fEnabled;
                adapter->COMGETTER(Enabled)(&fEnabled);
                if (fEnabled)
                {
                    if (a->argv[2])
                    {
                        CHECK_ERROR_RET(adapter, COMSETTER(TraceFile)(Bstr(a->argv[2]).raw()), RTEXITCODE_FAILURE);
                    }
                    else
                    {
                        errorArgument("Invalid filename or filename not specified for NIC %lu", n);
                        rc = E_FAIL;
                        break;
                    }
                    if (SUCCEEDED(rc))
                        fNeedsSaving = true;
                }
                else
                    RTMsgError("The NIC %d is currently disabled and thus its tracefile can't be changed", n);
            }
        }
        else if (!strncmp(a->argv[1], "nictrace", 8))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][8], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                BOOL fEnabled;
                adapter->COMGETTER(Enabled)(&fEnabled);
                if (fEnabled)
                {
                    if (!strcmp(a->argv[2], "on"))
                    {
                        CHECK_ERROR_RET(adapter, COMSETTER(TraceEnabled)(TRUE), RTEXITCODE_FAILURE);
                    }
                    else if (!strcmp(a->argv[2], "off"))
                    {
                        CHECK_ERROR_RET(adapter, COMSETTER(TraceEnabled)(FALSE), RTEXITCODE_FAILURE);
                    }
                    else
                    {
                        errorArgument("Invalid nictrace%lu argument '%s'", n, Utf8Str(a->argv[2]).c_str());
                        rc = E_FAIL;
                        break;
                    }
                    if (SUCCEEDED(rc))
                        fNeedsSaving = true;
                }
                else
                    RTMsgError("The NIC %d is currently disabled and thus its trace flag can't be changed", n);
            }
        }
        else if(   a->argc > 2
                && !strncmp(a->argv[1], "natpf", 5))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][5], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (!adapter)
            {
                rc = E_FAIL;
                break;
            }
            ComPtr<INATEngine> engine;
            CHECK_ERROR(adapter, COMGETTER(NATEngine)(engine.asOutParam()));
            if (!engine)
            {
                rc = E_FAIL;
                break;
            }

            if (!strcmp(a->argv[2], "delete"))
            {
                if (a->argc >= 3)
                    CHECK_ERROR(engine, RemoveRedirect(Bstr(a->argv[3]).raw()));
            }
            else
            {
#define ITERATE_TO_NEXT_TERM(ch)                                           \
    do {                                                                   \
        while (*ch != ',')                                                 \
        {                                                                  \
            if (*ch == 0)                                                  \
            {                                                              \
                return errorSyntax(USAGE_CONTROLVM,                        \
                                   "Missing or invalid argument to '%s'",  \
                                    a->argv[1]);                           \
            }                                                              \
            ch++;                                                          \
        }                                                                  \
        *ch = '\0';                                                        \
        ch++;                                                              \
    } while(0)

                char *strName;
                char *strProto;
                char *strHostIp;
                char *strHostPort;
                char *strGuestIp;
                char *strGuestPort;
                char *strRaw = RTStrDup(a->argv[2]);
                char *ch = strRaw;
                strName = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strProto = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strHostIp = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strHostPort = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strGuestIp = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strGuestPort = RTStrStrip(ch);
                NATProtocol_T proto;
                if (RTStrICmp(strProto, "udp") == 0)
                    proto = NATProtocol_UDP;
                else if (RTStrICmp(strProto, "tcp") == 0)
                    proto = NATProtocol_TCP;
                else
                {
                    return errorSyntax(USAGE_CONTROLVM,
                                       "Wrong rule proto '%s' specified -- only 'udp' and 'tcp' are allowed.",
                                       strProto);
                }
                CHECK_ERROR(engine, AddRedirect(Bstr(strName).raw(), proto, Bstr(strHostIp).raw(),
                        RTStrToUInt16(strHostPort), Bstr(strGuestIp).raw(), RTStrToUInt16(strGuestPort)));
#undef ITERATE_TO_NEXT_TERM
            }
            if (SUCCEEDED(rc))
                fNeedsSaving = true;
        }
        else if (!strncmp(a->argv[1], "nicproperty", 11))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][11], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                BOOL fEnabled;
                adapter->COMGETTER(Enabled)(&fEnabled);
                if (fEnabled)
                {
                    /* Parse 'name=value' */
                    char *pszProperty = RTStrDup(a->argv[2]);
                    if (pszProperty)
                    {
                        char *pDelimiter = strchr(pszProperty, '=');
                        if (pDelimiter)
                        {
                            *pDelimiter = '\0';

                            Bstr bstrName = pszProperty;
                            Bstr bstrValue = &pDelimiter[1];
                            CHECK_ERROR(adapter, SetProperty(bstrName.raw(), bstrValue.raw()));
                            if (SUCCEEDED(rc))
                                fNeedsSaving = true;
                        }
                        else
                        {
                            errorArgument("Invalid nicproperty%d argument '%s'", n, a->argv[2]);
                            rc = E_FAIL;
                        }
                        RTStrFree(pszProperty);
                    }
                    else
                    {
                        RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for nicproperty%d '%s'\n", n, a->argv[2]);
                        rc = E_FAIL;
                    }
                    if (FAILED(rc))
                        break;
                }
                else
                    RTMsgError("The NIC %d is currently disabled and thus its properties can't be changed", n);
            }
        }
        else if (!strncmp(a->argv[1], "nicpromisc", 10))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][10], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                BOOL fEnabled;
                adapter->COMGETTER(Enabled)(&fEnabled);
                if (fEnabled)
                {
                    NetworkAdapterPromiscModePolicy_T enmPromiscModePolicy;
                    if (!strcmp(a->argv[2], "deny"))
                        enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_Deny;
                    else if (  !strcmp(a->argv[2], "allow-vms")
                            || !strcmp(a->argv[2], "allow-network"))
                        enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowNetwork;
                    else if (!strcmp(a->argv[2], "allow-all"))
                        enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowAll;
                    else
                    {
                        errorArgument("Unknown promiscuous mode policy '%s'", a->argv[2]);
                        rc = E_INVALIDARG;
                        break;
                    }

                    CHECK_ERROR(adapter, COMSETTER(PromiscModePolicy)(enmPromiscModePolicy));
                    if (SUCCEEDED(rc))
                        fNeedsSaving = true;
                }
                else
                    RTMsgError("The NIC %d is currently disabled and thus its promiscuous mode can't be changed", n);
            }
        }
        else if (!strncmp(a->argv[1], "nic", 3))
        {
            /* Get the number of network adapters */
            ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, sessionMachine);
            unsigned n = parseNum(&a->argv[1][3], NetworkAdapterCount, "NIC");
            if (!n)
            {
                rc = E_FAIL;
                break;
            }
            if (a->argc <= 2)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            /* get the corresponding network adapter */
            ComPtr<INetworkAdapter> adapter;
            CHECK_ERROR_BREAK(sessionMachine, GetNetworkAdapter(n - 1, adapter.asOutParam()));
            if (adapter)
            {
                BOOL fEnabled;
                adapter->COMGETTER(Enabled)(&fEnabled);
                if (fEnabled)
                {
                    if (!strcmp(a->argv[2], "null"))
                    {
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_Null), RTEXITCODE_FAILURE);
                    }
                    else if (!strcmp(a->argv[2], "nat"))
                    {
                        if (a->argc == 4)
                            CHECK_ERROR_RET(adapter, COMSETTER(NATNetwork)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_NAT), RTEXITCODE_FAILURE);
                    }
                    else if (  !strcmp(a->argv[2], "bridged")
                            || !strcmp(a->argv[2], "hostif")) /* backward compatibility */
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(BridgedInterface)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_Bridged), RTEXITCODE_FAILURE);
                    }
                    else if (!strcmp(a->argv[2], "intnet"))
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(InternalNetwork)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_Internal), RTEXITCODE_FAILURE);
                    }
#if defined(VBOX_WITH_NETFLT)
                    else if (!strcmp(a->argv[2], "hostonly"))
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(HostOnlyInterface)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_HostOnly), RTEXITCODE_FAILURE);
                    }
#endif
                    else if (!strcmp(a->argv[2], "generic"))
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(GenericDriver)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_Generic), RTEXITCODE_FAILURE);
                    }
                    else if (!strcmp(a->argv[2], "natnetwork"))
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(NATNetwork)(Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_NATNetwork), RTEXITCODE_FAILURE);
                    }
                    /** @todo obsolete, remove eventually */
                    else if (!strcmp(a->argv[2], "vde"))
                    {
                        if (a->argc <= 3)
                        {
                            errorArgument("Missing argument to '%s'", a->argv[2]);
                            rc = E_FAIL;
                            break;
                        }
                        CHECK_ERROR_RET(adapter, COMSETTER(AttachmentType)(NetworkAttachmentType_Generic), RTEXITCODE_FAILURE);
                        CHECK_ERROR_RET(adapter, SetProperty(Bstr("name").raw(), Bstr(a->argv[3]).raw()), RTEXITCODE_FAILURE);
                    }
                    else
                    {
                        errorArgument("Invalid type '%s' specfied for NIC %lu", Utf8Str(a->argv[2]).c_str(), n);
                        rc = E_FAIL;
                        break;
                    }
                    if (SUCCEEDED(rc))
                        fNeedsSaving = true;
                }
                else
                    RTMsgError("The NIC %d is currently disabled and thus its attachment type can't be changed", n);
            }
        }
        else if (   !strcmp(a->argv[1], "vrde")
                 || !strcmp(a->argv[1], "vrdp"))
        {
            if (!strcmp(a->argv[1], "vrdp"))
                RTStrmPrintf(g_pStdErr, "Warning: 'vrdp' is deprecated. Use 'vrde'.\n");

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }
            ComPtr<IVRDEServer> vrdeServer;
            sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
            ASSERT(vrdeServer);
            if (vrdeServer)
            {
                if (!strcmp(a->argv[2], "on"))
                {
                    CHECK_ERROR_BREAK(vrdeServer, COMSETTER(Enabled)(TRUE));
                }
                else if (!strcmp(a->argv[2], "off"))
                {
                    CHECK_ERROR_BREAK(vrdeServer, COMSETTER(Enabled)(FALSE));
                }
                else
                {
                    errorArgument("Invalid remote desktop server state '%s'", Utf8Str(a->argv[2]).c_str());
                    rc = E_FAIL;
                    break;
                }
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        else if (   !strcmp(a->argv[1], "vrdeport")
                 || !strcmp(a->argv[1], "vrdpport"))
        {
            if (!strcmp(a->argv[1], "vrdpport"))
                RTStrmPrintf(g_pStdErr, "Warning: 'vrdpport' is deprecated. Use 'vrdeport'.\n");

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            ComPtr<IVRDEServer> vrdeServer;
            sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
            ASSERT(vrdeServer);
            if (vrdeServer)
            {
                Bstr ports;

                if (!strcmp(a->argv[2], "default"))
                    ports = "0";
                else
                    ports = a->argv[2];

                CHECK_ERROR_BREAK(vrdeServer, SetVRDEProperty(Bstr("TCP/Ports").raw(), ports.raw()));
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        else if (   !strcmp(a->argv[1], "vrdevideochannelquality")
                 || !strcmp(a->argv[1], "vrdpvideochannelquality"))
        {
            if (!strcmp(a->argv[1], "vrdpvideochannelquality"))
                RTStrmPrintf(g_pStdErr, "Warning: 'vrdpvideochannelquality' is deprecated. Use 'vrdevideochannelquality'.\n");

            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }
            ComPtr<IVRDEServer> vrdeServer;
            sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
            ASSERT(vrdeServer);
            if (vrdeServer)
            {
                Bstr value = a->argv[2];

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("VideoChannel/Quality").raw(), value.raw()));
                if (SUCCEEDED(rc))
                    fNeedsSaving = true;
            }
        }
        else if (!strcmp(a->argv[1], "vrdeproperty"))
        {
            if (a->argc <= 1 + 1)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }
            ComPtr<IVRDEServer> vrdeServer;
            sessionMachine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
            ASSERT(vrdeServer);
            if (vrdeServer)
            {
                /* Parse 'name=value' */
                char *pszProperty = RTStrDup(a->argv[2]);
                if (pszProperty)
                {
                    char *pDelimiter = strchr(pszProperty, '=');
                    if (pDelimiter)
                    {
                        *pDelimiter = '\0';

                        Bstr bstrName = pszProperty;
                        Bstr bstrValue = &pDelimiter[1];
                        CHECK_ERROR(vrdeServer, SetVRDEProperty(bstrName.raw(), bstrValue.raw()));
                        if (SUCCEEDED(rc))
                            fNeedsSaving = true;
                    }
                    else
                    {
                        errorArgument("Invalid vrdeproperty argument '%s'", a->argv[2]);
                        rc = E_FAIL;
                    }
                    RTStrFree(pszProperty);
                }
                else
                {
                    RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for VRDE property '%s'\n", a->argv[2]);
                    rc = E_FAIL;
                }
            }
            if (FAILED(rc))
            {
                break;
            }
        }
        else if (   !strcmp(a->argv[1], "usbattach")
                 || !strcmp(a->argv[1], "usbdetach"))
        {
            if (a->argc < 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Not enough parameters");
                rc = E_FAIL;
                break;
            }
            else if (a->argc == 4 || a->argc > 5)
            {
                errorSyntax(USAGE_CONTROLVM, "Wrong number of arguments");
                rc = E_FAIL;
                break;
            }

            bool attach = !strcmp(a->argv[1], "usbattach");

            Bstr usbId = a->argv[2];
            Bstr captureFilename;

            if (a->argc == 5)
            {
                if (!strcmp(a->argv[3], "--capturefile"))
                    captureFilename = a->argv[4];
                else
                {
                    errorArgument("Invalid parameter '%s'", a->argv[3]);
                    rc = E_FAIL;
                    break;
                }
            }

            Guid guid(usbId);
            if (!guid.isValid())
            {
                // assume address
                if (attach)
                {
                    ComPtr<IHost> host;
                    CHECK_ERROR_BREAK(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                    SafeIfaceArray <IHostUSBDevice> coll;
                    CHECK_ERROR_BREAK(host, COMGETTER(USBDevices)(ComSafeArrayAsOutParam(coll)));
                    ComPtr<IHostUSBDevice> dev;
                    CHECK_ERROR_BREAK(host, FindUSBDeviceByAddress(Bstr(a->argv[2]).raw(),
                                                                   dev.asOutParam()));
                    CHECK_ERROR_BREAK(dev, COMGETTER(Id)(usbId.asOutParam()));
                }
                else
                {
                    SafeIfaceArray <IUSBDevice> coll;
                    CHECK_ERROR_BREAK(console, COMGETTER(USBDevices)(ComSafeArrayAsOutParam(coll)));
                    ComPtr<IUSBDevice> dev;
                    CHECK_ERROR_BREAK(console, FindUSBDeviceByAddress(Bstr(a->argv[2]).raw(),
                                                                      dev.asOutParam()));
                    CHECK_ERROR_BREAK(dev, COMGETTER(Id)(usbId.asOutParam()));
                }
            }
            else if (guid.isZero())
            {
                errorArgument("Zero UUID argument '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }

            if (attach)
                CHECK_ERROR_BREAK(console, AttachUSBDevice(usbId.raw(), captureFilename.raw()));
            else
            {
                ComPtr<IUSBDevice> dev;
                CHECK_ERROR_BREAK(console, DetachUSBDevice(usbId.raw(),
                                                           dev.asOutParam()));
            }
        }
        else if (!strcmp(a->argv[1], "setvideomodehint"))
        {
            if (a->argc != 5 && a->argc != 6 && a->argc != 7 && a->argc != 9)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }
            bool fEnabled = true;
            uint32_t uXRes = RTStrToUInt32(a->argv[2]);
            uint32_t uYRes = RTStrToUInt32(a->argv[3]);
            uint32_t uBpp  = RTStrToUInt32(a->argv[4]);
            uint32_t uDisplayIdx = 0;
            bool fChangeOrigin = false;
            int32_t iOriginX = 0;
            int32_t iOriginY = 0;
            if (a->argc >= 6)
                uDisplayIdx = RTStrToUInt32(a->argv[5]);
            if (a->argc >= 7)
            {
                int vrc = parseBool(a->argv[6], &fEnabled);
                if (RT_FAILURE(vrc))
                {
                    errorSyntax(USAGE_CONTROLVM, "Either \"yes\" or \"no\" is expected");
                    rc = E_FAIL;
                    break;
                }
                fEnabled = !RTStrICmp(a->argv[6], "yes");
            }
            if (a->argc == 9)
            {
                iOriginX = RTStrToInt32(a->argv[7]);
                iOriginY = RTStrToInt32(a->argv[8]);
                fChangeOrigin = true;
            }

            ComPtr<IDisplay> pDisplay;
            CHECK_ERROR_BREAK(console, COMGETTER(Display)(pDisplay.asOutParam()));
            if (!pDisplay)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(pDisplay, SetVideoModeHint(uDisplayIdx, fEnabled,
                                                         fChangeOrigin, iOriginX, iOriginY,
                                                         uXRes, uYRes, uBpp));
        }
        else if (!strcmp(a->argv[1], "setcredentials"))
        {
            bool fAllowLocalLogon = true;
            if (   a->argc == 7
                || (   a->argc == 8
                    && (   !strcmp(a->argv[3], "-p")
                        || !strcmp(a->argv[3], "--passwordfile"))))
            {
                if (   strcmp(a->argv[5 + (a->argc - 7)], "--allowlocallogon")
                    && strcmp(a->argv[5 + (a->argc - 7)], "-allowlocallogon"))
                {
                    errorArgument("Invalid parameter '%s'", a->argv[5]);
                    rc = E_FAIL;
                    break;
                }
                if (!strcmp(a->argv[6 + (a->argc - 7)], "no"))
                    fAllowLocalLogon = false;
            }
            else if (   a->argc != 5
                     && (   a->argc != 6
                         || (   strcmp(a->argv[3], "-p")
                             && strcmp(a->argv[3], "--passwordfile"))))
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }
            Utf8Str passwd, domain;
            if (a->argc == 5 || a->argc == 7)
            {
                passwd = a->argv[3];
                domain = a->argv[4];
            }
            else
            {
                RTEXITCODE rcExit = readPasswordFile(a->argv[4], &passwd);
                if (rcExit != RTEXITCODE_SUCCESS)
                {
                    rc = E_FAIL;
                    break;
                }
                domain = a->argv[5];
            }

            ComPtr<IGuest> pGuest;
            CHECK_ERROR_BREAK(console, COMGETTER(Guest)(pGuest.asOutParam()));
            if (!pGuest)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(pGuest, SetCredentials(Bstr(a->argv[2]).raw(),
                                                     Bstr(passwd).raw(),
                                                     Bstr(domain).raw(),
                                                     fAllowLocalLogon));
        }
#if 0 /** @todo review & remove */
        else if (!strcmp(a->argv[1], "dvdattach"))
        {
            Bstr uuid;
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            ComPtr<IMedium> dvdMedium;

            /* unmount? */
            if (!strcmp(a->argv[2], "none"))
            {
                /* nothing to do, NULL object will cause unmount */
            }
            /* host drive? */
            else if (!strncmp(a->argv[2], "host:", 5))
            {
                ComPtr<IHost> host;
                CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));

                rc = host->FindHostDVDDrive(Bstr(a->argv[2] + 5), dvdMedium.asOutParam());
                if (!dvdMedium)
                {
                    errorArgument("Invalid host DVD drive name \"%s\"",
                                  a->argv[2] + 5);
                    rc = E_FAIL;
                    break;
                }
            }
            else
            {
                /* first assume it's a UUID */
                uuid = a->argv[2];
                rc = a->virtualBox->GetDVDImage(uuid, dvdMedium.asOutParam());
                if (FAILED(rc) || !dvdMedium)
                {
                    /* must be a filename, check if it's in the collection */
                    rc = a->virtualBox->FindDVDImage(Bstr(a->argv[2]), dvdMedium.asOutParam());
                    /* not registered, do that on the fly */
                    if (!dvdMedium)
                    {
                        Bstr emptyUUID;
                        CHECK_ERROR(a->virtualBox, OpenDVDImage(Bstr(a->argv[2]), emptyUUID, dvdMedium.asOutParam()));
                    }
                }
                if (!dvdMedium)
                {
                    rc = E_FAIL;
                    break;
                }
            }

            /** @todo generalize this, allow arbitrary number of DVD drives
             * and as a consequence multiple attachments and different
             * storage controllers. */
            if (dvdMedium)
                dvdMedium->COMGETTER(Id)(uuid.asOutParam());
            else
                uuid = Guid().toString();
            CHECK_ERROR(machine, MountMedium(Bstr("IDE Controller"), 1, 0, uuid, FALSE /* aForce */));
        }
        else if (!strcmp(a->argv[1], "floppyattach"))
        {
            Bstr uuid;
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            ComPtr<IMedium> floppyMedium;

            /* unmount? */
            if (!strcmp(a->argv[2], "none"))
            {
                /* nothing to do, NULL object will cause unmount */
            }
            /* host drive? */
            else if (!strncmp(a->argv[2], "host:", 5))
            {
                ComPtr<IHost> host;
                CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                host->FindHostFloppyDrive(Bstr(a->argv[2] + 5), floppyMedium.asOutParam());
                if (!floppyMedium)
                {
                    errorArgument("Invalid host floppy drive name \"%s\"",
                                  a->argv[2] + 5);
                    rc = E_FAIL;
                    break;
                }
            }
            else
            {
                /* first assume it's a UUID */
                uuid = a->argv[2];
                rc = a->virtualBox->GetFloppyImage(uuid, floppyMedium.asOutParam());
                if (FAILED(rc) || !floppyMedium)
                {
                    /* must be a filename, check if it's in the collection */
                    rc = a->virtualBox->FindFloppyImage(Bstr(a->argv[2]), floppyMedium.asOutParam());
                    /* not registered, do that on the fly */
                    if (!floppyMedium)
                    {
                        Bstr emptyUUID;
                        CHECK_ERROR(a->virtualBox, OpenFloppyImage(Bstr(a->argv[2]), emptyUUID, floppyMedium.asOutParam()));
                    }
                }
                if (!floppyMedium)
                {
                    rc = E_FAIL;
                    break;
                }
            }
            floppyMedium->COMGETTER(Id)(uuid.asOutParam());
            CHECK_ERROR(machine, MountMedium(Bstr("Floppy Controller"), 0, 0, uuid, FALSE /* aForce */));
        }
#endif /* obsolete dvdattach/floppyattach */
        else if (!strcmp(a->argv[1], "guestmemoryballoon"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }
            uint32_t uVal;
            int vrc;
            vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (vrc != VINF_SUCCESS)
            {
                errorArgument("Error parsing guest memory balloon size '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            /* guest is running; update IGuest */
            ComPtr<IGuest> pGuest;
            rc = console->COMGETTER(Guest)(pGuest.asOutParam());
            if (SUCCEEDED(rc))
            {
                if (!pGuest)
                {
                    RTMsgError("Guest not running");
                    rc = E_FAIL;
                    break;
                }
                CHECK_ERROR(pGuest, COMSETTER(MemoryBalloonSize)(uVal));
            }
        }
        else if (!strcmp(a->argv[1], "teleport"))
        {
            Bstr        bstrHostname;
            uint32_t    uMaxDowntime = 250 /*ms*/;
            uint32_t    uPort        = UINT32_MAX;
            uint32_t    cMsTimeout   = 0;
            Utf8Str     strPassword;
            static const RTGETOPTDEF s_aTeleportOptions[] =
            {
                { "--host",              'h', RTGETOPT_REQ_STRING }, /** @todo RTGETOPT_FLAG_MANDATORY */
                { "--hostname",          'h', RTGETOPT_REQ_STRING }, /** @todo remove this */
                { "--maxdowntime",       'd', RTGETOPT_REQ_UINT32 },
                { "--port",              'P', RTGETOPT_REQ_UINT32 }, /** @todo RTGETOPT_FLAG_MANDATORY */
                { "--passwordfile",      'p', RTGETOPT_REQ_STRING },
                { "--password",          'W', RTGETOPT_REQ_STRING },
                { "--timeout",           't', RTGETOPT_REQ_UINT32 },
                { "--detailed-progress", 'D', RTGETOPT_REQ_NOTHING }
            };
            RTGETOPTSTATE GetOptState;
            RTGetOptInit(&GetOptState, a->argc, a->argv, s_aTeleportOptions, RT_ELEMENTS(s_aTeleportOptions), 2, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
            int ch;
            RTGETOPTUNION Value;
            while (   SUCCEEDED(rc)
                   && (ch = RTGetOpt(&GetOptState, &Value)))
            {
                switch (ch)
                {
                    case 'h': bstrHostname  = Value.psz; break;
                    case 'd': uMaxDowntime  = Value.u32; break;
                    case 'D': g_fDetailedProgress = true; break;
                    case 'P': uPort         = Value.u32; break;
                    case 'p':
                    {
                        RTEXITCODE rcExit = readPasswordFile(Value.psz, &strPassword);
                        if (rcExit != RTEXITCODE_SUCCESS)
                            rc = E_FAIL;
                        break;
                    }
                    case 'W': strPassword   = Value.psz; break;
                    case 't': cMsTimeout    = Value.u32; break;
                    default:
                        errorGetOpt(USAGE_CONTROLVM, ch, &Value);
                        rc = E_FAIL;
                        break;
                }
            }
            if (FAILED(rc))
                break;

            ComPtr<IProgress> progress;
            CHECK_ERROR_BREAK(console, Teleport(bstrHostname.raw(), uPort,
                                                Bstr(strPassword).raw(),
                                                uMaxDowntime,
                                                progress.asOutParam()));

            if (cMsTimeout)
            {
                rc = progress->COMSETTER(Timeout)(cMsTimeout);
                if (FAILED(rc) && rc != VBOX_E_INVALID_OBJECT_STATE)
                    CHECK_ERROR_BREAK(progress, COMSETTER(Timeout)(cMsTimeout)); /* lazyness */
            }

            rc = showProgress(progress);
            CHECK_PROGRESS_ERROR(progress, ("Teleportation failed"));
        }
        else if (!strcmp(a->argv[1], "screenshotpng"))
        {
            if (a->argc <= 2 || a->argc > 4)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }
            int vrc;
            uint32_t iScreen = 0;
            if (a->argc == 4)
            {
                vrc = RTStrToUInt32Ex(a->argv[3], NULL, 0, &iScreen);
                if (vrc != VINF_SUCCESS)
                {
                    errorArgument("Error parsing display number '%s'", a->argv[3]);
                    rc = E_FAIL;
                    break;
                }
            }
            ComPtr<IDisplay> pDisplay;
            CHECK_ERROR_BREAK(console, COMGETTER(Display)(pDisplay.asOutParam()));
            if (!pDisplay)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }
            ULONG width, height, bpp;
            LONG xOrigin, yOrigin;
            GuestMonitorStatus_T monitorStatus;
            CHECK_ERROR_BREAK(pDisplay, GetScreenResolution(iScreen, &width, &height, &bpp, &xOrigin, &yOrigin, &monitorStatus));
            com::SafeArray<BYTE> saScreenshot;
            CHECK_ERROR_BREAK(pDisplay, TakeScreenShotToArray(iScreen, width, height, BitmapFormat_PNG, ComSafeArrayAsOutParam(saScreenshot)));
            RTFILE pngFile = NIL_RTFILE;
            vrc = RTFileOpen(&pngFile, a->argv[2], RTFILE_O_OPEN_CREATE | RTFILE_O_WRITE | RTFILE_O_TRUNCATE | RTFILE_O_DENY_ALL);
            if (RT_FAILURE(vrc))
            {
                RTMsgError("Failed to create file '%s' (%Rrc)", a->argv[2], vrc);
                rc = E_FAIL;
                break;
            }
            vrc = RTFileWrite(pngFile, saScreenshot.raw(), saScreenshot.size(), NULL);
            if (RT_FAILURE(vrc))
            {
                RTMsgError("Failed to write screenshot to file '%s' (%Rrc)", a->argv[2], vrc);
                rc = E_FAIL;
            }
            RTFileClose(pngFile);
        }
#ifdef VBOX_WITH_VIDEOREC
        /*
         * Note: Commands starting with "vcp" are the deprecated versions and are
         *       kept to ensure backwards compatibility.
         */
        else if (   !strcmp(a->argv[1], "videocap")
                 || !strcmp(a->argv[1], "vcpenabled"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }
            if (!strcmp(a->argv[2], "on"))
            {
                CHECK_ERROR_RET(sessionMachine, COMSETTER(VideoCaptureEnabled)(TRUE), RTEXITCODE_FAILURE);
            }
            else if (!strcmp(a->argv[2], "off"))
            {
                CHECK_ERROR_RET(sessionMachine, COMSETTER(VideoCaptureEnabled)(FALSE), RTEXITCODE_FAILURE);
            }
            else
            {
                errorArgument("Invalid state '%s'", Utf8Str(a->argv[2]).c_str());
                rc = E_FAIL;
                break;
            }
        }
        else if (   !strcmp(a->argv[1], "videocapscreens")
                 || !strcmp(a->argv[1], "vcpscreens"))
        {
            ULONG cMonitors = 64;
            CHECK_ERROR_BREAK(machine, COMGETTER(MonitorCount)(&cMonitors));
            com::SafeArray<BOOL> saScreens(cMonitors);
            if (   a->argc == 3
                && !strcmp(a->argv[2], "all"))
            {
                /* enable all screens */
                for (unsigned i = 0; i < cMonitors; i++)
                    saScreens[i] = true;
            }
            else if (   a->argc == 3
                     && !strcmp(a->argv[2], "none"))
            {
                /* disable all screens */
                for (unsigned i = 0; i < cMonitors; i++)
                    saScreens[i] = false;

                /** @todo r=andy What if this is specified? */
            }
            else
            {
                /* enable selected screens */
                for (unsigned i = 0; i < cMonitors; i++)
                    saScreens[i] = false;
                for (int i = 2; SUCCEEDED(rc) && i < a->argc; i++)
                {
                    uint32_t iScreen;
                    int vrc = RTStrToUInt32Ex(a->argv[i], NULL, 0, &iScreen);
                    if (vrc != VINF_SUCCESS)
                    {
                        errorArgument("Error parsing display number '%s'", a->argv[i]);
                        rc = E_FAIL;
                        break;
                    }
                    if (iScreen >= cMonitors)
                    {
                        errorArgument("Invalid screen ID specified '%u'", iScreen);
                        rc = E_FAIL;
                        break;
                    }
                    saScreens[iScreen] = true;
                }
            }

            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureScreens)(ComSafeArrayAsInParam(saScreens)));
        }
        else if (   !strcmp(a->argv[1], "videocapfile")
                 || !strcmp(a->argv[1], "vcpfile"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureFile)(Bstr(a->argv[2]).raw()));
        }
        else if (!strcmp(a->argv[1], "videocapres"))
        {
            if (a->argc != 4)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing width '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureWidth)(uVal));

            vrc = RTStrToUInt32Ex(a->argv[3], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing height '%s'", a->argv[3]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureHeight)(uVal));
        }
        else if (!strcmp(a->argv[1], "vcpwidth")) /* Deprecated; keeping for compatibility. */
        {
            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing width '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureWidth)(uVal));
        }
        else if (!strcmp(a->argv[1], "vcpheight")) /* Deprecated; keeping for compatibility. */
        {
            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing height '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureHeight)(uVal));
        }
        else if (   !strcmp(a->argv[1], "videocaprate")
                 || !strcmp(a->argv[1], "vcprate"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing rate '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureRate)(uVal));
        }
        else if (   !strcmp(a->argv[1], "videocapfps")
                 || !strcmp(a->argv[1], "vcpfps"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing FPS '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureFPS)(uVal));
        }
        else if (   !strcmp(a->argv[1], "videocapmaxtime")
                 || !strcmp(a->argv[1], "vcpmaxtime"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing maximum time '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureMaxTime)(uVal));
        }
        else if (   !strcmp(a->argv[1], "videocapmaxsize")
                 || !strcmp(a->argv[1], "vcpmaxsize"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            uint32_t uVal;
            int vrc = RTStrToUInt32Ex(a->argv[2], NULL, 0, &uVal);
            if (RT_FAILURE(vrc))
            {
                errorArgument("Error parsing maximum file size '%s'", a->argv[2]);
                rc = E_FAIL;
                break;
            }
            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureMaxFileSize)(uVal));
        }
        else if (   !strcmp(a->argv[1], "videocapopts")
                 || !strcmp(a->argv[1], "vcpoptions"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                rc = E_FAIL;
                break;
            }

            CHECK_ERROR_BREAK(sessionMachine, COMSETTER(VideoCaptureOptions)(Bstr(a->argv[3]).raw()));
        }
#endif /* VBOX_WITH_VIDEOREC */
        else if (!strcmp(a->argv[1], "webcam"))
        {
            if (a->argc < 3)
            {
                errorArgument("Missing argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }

            ComPtr<IEmulatedUSB> pEmulatedUSB;
            CHECK_ERROR_BREAK(console, COMGETTER(EmulatedUSB)(pEmulatedUSB.asOutParam()));
            if (!pEmulatedUSB)
            {
                RTMsgError("Guest not running");
                rc = E_FAIL;
                break;
            }

            if (!strcmp(a->argv[2], "attach"))
            {
                Bstr path("");
                if (a->argc >= 4)
                    path = a->argv[3];
                Bstr settings("");
                if (a->argc >= 5)
                    settings = a->argv[4];
                CHECK_ERROR_BREAK(pEmulatedUSB, WebcamAttach(path.raw(), settings.raw()));
            }
            else if (!strcmp(a->argv[2], "detach"))
            {
                Bstr path("");
                if (a->argc >= 4)
                    path = a->argv[3];
                CHECK_ERROR_BREAK(pEmulatedUSB, WebcamDetach(path.raw()));
            }
            else if (!strcmp(a->argv[2], "list"))
            {
                com::SafeArray <BSTR> webcams;
                CHECK_ERROR_BREAK(pEmulatedUSB, COMGETTER(Webcams)(ComSafeArrayAsOutParam(webcams)));
                for (size_t i = 0; i < webcams.size(); ++i)
                {
                    RTPrintf("%ls\n", webcams[i][0]? webcams[i]: Bstr("default").raw());
                }
            }
            else
            {
                errorArgument("Invalid argument to '%s'", a->argv[1]);
                rc = E_FAIL;
                break;
            }
        }
        else if (!strcmp(a->argv[1], "addencpassword"))
        {
            if (   a->argc != 4
                && a->argc != 6)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                break;
            }

            BOOL fRemoveOnSuspend = FALSE;
            if (a->argc == 6)
            {
                if (   strcmp(a->argv[4], "--removeonsuspend")
                    || (   strcmp(a->argv[5], "yes")
                        && strcmp(a->argv[5], "no")))
                {
                    errorSyntax(USAGE_CONTROLVM, "Invalid parameters");
                    break;
                }
                if (!strcmp(a->argv[5], "yes"))
                    fRemoveOnSuspend = TRUE;
            }

            Bstr bstrPwId(a->argv[2]);
            Utf8Str strPassword;

            if (!RTStrCmp(a->argv[3], "-"))
            {
                /* Get password from console. */
                RTEXITCODE rcExit = readPasswordFromConsole(&strPassword, "Enter password:");
                if (rcExit == RTEXITCODE_FAILURE)
                    break;
            }
            else
            {
                RTEXITCODE rcExit = readPasswordFile(a->argv[3], &strPassword);
                if (rcExit == RTEXITCODE_FAILURE)
                {
                    RTMsgError("Failed to read new password from file");
                    break;
                }
            }

            CHECK_ERROR_BREAK(console, AddDiskEncryptionPassword(bstrPwId.raw(), Bstr(strPassword).raw(), fRemoveOnSuspend));
        }
        else if (!strcmp(a->argv[1], "removeencpassword"))
        {
            if (a->argc != 3)
            {
                errorSyntax(USAGE_CONTROLVM, "Incorrect number of parameters");
                break;
            }
            Bstr bstrPwId(a->argv[2]);
            CHECK_ERROR_BREAK(console, RemoveDiskEncryptionPassword(bstrPwId.raw()));
        }
        else if (!strcmp(a->argv[1], "removeallencpasswords"))
        {
            CHECK_ERROR_BREAK(console, ClearAllDiskEncryptionPasswords());
        }
        else
        {
            errorSyntax(USAGE_CONTROLVM, "Invalid parameter '%s'", a->argv[1]);
            rc = E_FAIL;
        }
    } while (0);

    /* The client has to trigger saving the state explicitely. */
    if (fNeedsSaving)
        CHECK_ERROR(sessionMachine, SaveSettings());

    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}
