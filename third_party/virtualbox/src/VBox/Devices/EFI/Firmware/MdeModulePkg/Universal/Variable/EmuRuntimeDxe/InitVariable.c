/** @file

  Implment all four UEFI runtime variable services and
  install variable architeture protocol.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

EFI_EVENT   mVirtualAddressChangeEvent = NULL;

#ifdef VBOX
# include <Library/PrintLib.h>
# include <Library/TimerLib.h>
# include "VBoxPkg.h"
# include "DevEFI.h"
# include "iprt/asm.h"


static UINT32 VBoxReadNVRAM(UINT8 *pu8Buffer, UINT32 cbBuffer)
{
    UINT32 idxBuffer = 0;
    for (idxBuffer = 0; idxBuffer < cbBuffer; ++idxBuffer)
        pu8Buffer[idxBuffer] = ASMInU8(EFI_PORT_VARIABLE_OP);
    return idxBuffer;
}

DECLINLINE(void) VBoxWriteNVRAMU32Param(UINT32 u32CodeParam, UINT32 u32Param)
{
    ASMOutU32(EFI_PORT_VARIABLE_OP, u32CodeParam);
    ASMOutU32(EFI_PORT_VARIABLE_PARAM, u32Param);
}

static UINT32 VBoxWriteNVRAMByteArrayParam(const UINT8 *pbParam, UINT32 cbParam)
{
    UINT32 idxParam = 0;
    for (idxParam = 0; idxParam < cbParam; ++idxParam)
        ASMOutU8(EFI_PORT_VARIABLE_PARAM, pbParam[idxParam]);
    return idxParam;
}

static void VBoxWriteNVRAMNameParam(const CHAR16 *pwszName)
{
    UINTN i;
    UINTN cwcName = StrLen(pwszName);

    ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_NAME_UTF16);
    for (i = 0; i <= cwcName; i++)
        ASMOutU16(EFI_PORT_VARIABLE_PARAM, pwszName[i]);
}

DECLINLINE(UINT32) VBoxWriteNVRAMGuidParam(const EFI_GUID *pGuid)
{
    ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_GUID);
    return VBoxWriteNVRAMByteArrayParam((UINT8 *)pGuid, sizeof(EFI_GUID));
}

static UINT32 VBoxWriteNVRAMDoOp(UINT32 u32Operation)
{
    UINT32 u32Rc;
    VBoxLogFlowFuncEnter();
    VBoxLogFlowFuncMarkVar(u32Operation, "%x");
    VBoxWriteNVRAMU32Param(EFI_VM_VARIABLE_OP_START, u32Operation);

    while ((u32Rc = ASMInU32(EFI_PORT_VARIABLE_OP)) == EFI_VARIABLE_OP_STATUS_BSY)
    {
#if 0
        MicroSecondDelay (400);
#endif
        /* @todo: sleep here. bird: won't ever happen, so don't bother. */
    }
    VBoxLogFlowFuncMarkVar(u32Rc, "%x");
    VBoxLogFlowFuncLeave();
    return u32Rc;
}
#endif

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       Data pointer.

  @return EFI_INVALID_PARAMETER     Invalid parameter
  @return EFI_SUCCESS               Find the specified variable
  @return EFI_NOT_FOUND             Not found
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  OUT UINT32       *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT VOID         *Data
  )
{
#ifndef VBOX
  return EmuGetVariable (
          VariableName,
          VendorGuid,
          Attributes OPTIONAL,
          DataSize,
          Data,
          &mVariableModuleGlobal->VariableGlobal[Physical]
          );
#else
    EFI_STATUS rc;
    UINT32 u32Rc;

    VBoxLogFlowFuncEnter();

    /*
     * Tell DevEFI to look for the specified variable.
     */
    VBoxWriteNVRAMGuidParam(VendorGuid);
    VBoxWriteNVRAMNameParam(VariableName);
    u32Rc = VBoxWriteNVRAMDoOp(EFI_VARIABLE_OP_QUERY);
    if (u32Rc == EFI_VARIABLE_OP_STATUS_OK)
    {
        /*
         * Check if we got enought space for the value.
         */
        UINT32 VarLen;
        ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_VALUE_LENGTH);
        VarLen = ASMInU32(EFI_PORT_VARIABLE_OP);
        VBoxLogFlowFuncMarkVar(*DataSize, "%d");
        VBoxLogFlowFuncMarkVar(VarLen, "%d");
        if (   VarLen <= *DataSize
            && Data)
        {
            /*
             * We do, then read it and, if requrest, the attribute.
             */
            *DataSize = VarLen;
            ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_VALUE);
            VBoxReadNVRAM((UINT8 *)Data, VarLen);

            if (Attributes)
            {
                ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_ATTRIBUTE);
                *Attributes = ASMInU32(EFI_PORT_VARIABLE_OP);
                VBoxLogFlowFuncMarkVar(Attributes, "%x");
            }

            rc = EFI_SUCCESS;
        }
        else
        {
            *DataSize = VarLen;
            rc = EFI_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        rc = EFI_NOT_FOUND;
    }

    VBoxLogFlowFuncLeaveRC(rc);
    return rc;
#endif
}

/**

  This code Finds the Next available variable.

  @param VariableNameSize           Size of the variable name
  @param VariableName               Pointer to variable name
  @param VendorGuid                 Variable Vendor Guid

  @return EFI_INVALID_PARAMETER     Invalid parameter
  @return EFI_SUCCESS               Find the specified variable
  @return EFI_NOT_FOUND             Not found
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result

**/
EFI_STATUS
EFIAPI
RuntimeServiceGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
#ifndef VBOX
  return EmuGetNextVariableName (
          VariableNameSize,
          VariableName,
          VendorGuid,
          &mVariableModuleGlobal->VariableGlobal[Physical]
          );
#else
    uint32_t    u32Rc;
    EFI_STATUS  rc;
    VBoxLogFlowFuncEnter();

    /*
     * Validate inputs.
     */
    if (!VariableNameSize || !VariableName || !VendorGuid)
    {
        VBoxLogFlowFuncLeaveRC(EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
    }

    /*
     * Tell DevEFI which the current variable is, then ask for the next one.
     */
    if (!VariableName[0])
        u32Rc = VBoxWriteNVRAMDoOp(EFI_VARIABLE_OP_QUERY_REWIND);
    else
    {
        VBoxWriteNVRAMGuidParam(VendorGuid);
        VBoxWriteNVRAMNameParam(VariableName);
        u32Rc = VBoxWriteNVRAMDoOp(EFI_VARIABLE_OP_QUERY);
    }
    if (u32Rc == EFI_VARIABLE_OP_STATUS_OK)
        u32Rc = VBoxWriteNVRAMDoOp(EFI_VARIABLE_OP_QUERY_NEXT);
    /** @todo We're supposed to skip stuff depending on attributes and
     *        runtime/boottime, at least if EmuGetNextVariableName is something
     *        to go by... */

    if (u32Rc == EFI_VARIABLE_OP_STATUS_OK)
    {
        /*
         * Output buffer check.
         */
        UINT32      cwcName;
        ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_NAME_LENGTH_UTF16);
        cwcName = ASMInU32(EFI_PORT_VARIABLE_OP);
        if ((cwcName + 1) * 2 <= *VariableNameSize) /* ASSUMES byte size is specified */
        {
            UINT32 i;

            /*
             * Read back the result.
             */
            ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_GUID);
            VBoxReadNVRAM((UINT8 *)VendorGuid, sizeof(EFI_GUID));

            ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_NAME_UTF16);
            for (i = 0; i < cwcName; i++)
                VariableName[i] = ASMInU16(EFI_PORT_VARIABLE_OP);
            VariableName[i] = '\0';

            rc = EFI_SUCCESS;
        }
        else
            rc = EFI_BUFFER_TOO_SMALL;
        *VariableNameSize = (cwcName + 1) * 2;
    }
    else
        rc = EFI_NOT_FOUND; /* whatever */

    VBoxLogFlowFuncLeaveRC(rc);
    return rc;
#endif
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName                     Name of Variable to be found
  @param VendorGuid                       Variable vendor GUID
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer

  @return EFI_INVALID_PARAMETER           Invalid parameter
  @return EFI_SUCCESS                     Set successfully
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable
  @return EFI_NOT_FOUND                   Not found
  @return EFI_WRITE_PROTECTED             Variable is read-only

**/
EFI_STATUS
EFIAPI
RuntimeServiceSetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  IN UINT32        Attributes,
  IN UINTN         DataSize,
  IN VOID          *Data
  )
{
#ifndef VBOX
  return EmuSetVariable (
          VariableName,
          VendorGuid,
          Attributes,
          DataSize,
          Data,
          &mVariableModuleGlobal->VariableGlobal[Physical],
          &mVariableModuleGlobal->VolatileLastVariableOffset,
          &mVariableModuleGlobal->NonVolatileLastVariableOffset
          );
#else
    UINT32 u32Rc;
    VBoxLogFlowFuncEnter();
    VBoxLogFlowFuncMarkVar(VendorGuid, "%g");
    VBoxLogFlowFuncMarkVar(VariableName, "%s");
    VBoxLogFlowFuncMarkVar(DataSize, "%d");
    /* set guid */
    VBoxWriteNVRAMGuidParam(VendorGuid);
    /* set name */
    VBoxWriteNVRAMNameParam(VariableName);
    /* set attribute */
    VBoxWriteNVRAMU32Param(EFI_VM_VARIABLE_OP_ATTRIBUTE, Attributes);
    /* set value length */
    VBoxWriteNVRAMU32Param(EFI_VM_VARIABLE_OP_VALUE_LENGTH, (UINT32)DataSize);
    /* fill value bytes */
    ASMOutU32(EFI_PORT_VARIABLE_OP, EFI_VM_VARIABLE_OP_VALUE);
    VBoxWriteNVRAMByteArrayParam(Data, (UINT32)DataSize);
    /* start fetch operation */
    u32Rc = VBoxWriteNVRAMDoOp(EFI_VARIABLE_OP_ADD);
    /* process errors */
    VBoxLogFlowFuncLeave();
    switch (u32Rc)
    {
        case EFI_VARIABLE_OP_STATUS_OK:
            return EFI_SUCCESS;
        case EFI_VARIABLE_OP_STATUS_WP:
        default:
            return EFI_WRITE_PROTECTED;
    }
#endif
}

/**

  This code returns information about the EFI variables.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
RuntimeServiceQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
{
#ifndef VBOX
  return EmuQueryVariableInfo (
          Attributes,
          MaximumVariableStorageSize,
          RemainingVariableStorageSize,
          MaximumVariableSize,
          &mVariableModuleGlobal->VariableGlobal[Physical]
          );
#else
    *MaximumVariableStorageSize = 64 * 1024 * 1024;
    *MaximumVariableSize = 1024;
    *RemainingVariableStorageSize = 32 * 1024 * 1024;
    return EFI_SUCCESS;
#endif
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
#ifndef VBOX
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLangCodes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->LangCodes);
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal->PlatformLang);
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase
    );
  EfiConvertPointer (
    0x0,
    (VOID **) &mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase
    );
  EfiConvertPointer (0x0, (VOID **) &mVariableModuleGlobal);
#endif
}

/**
  EmuVariable Driver main entry point. The Variable driver places the 4 EFI
  runtime services in the EFI System Table and installs arch protocols
  for variable read and write services being available. It also registers
  notification function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_HANDLE  NewHandle;
  EFI_STATUS  Status;

  Status = VariableCommonInitialize (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  SystemTable->RuntimeServices->GetVariable         = RuntimeServiceGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = RuntimeServiceGetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = RuntimeServiceSetVariable;
  SystemTable->RuntimeServices->QueryVariableInfo   = RuntimeServiceQueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural Protocol on a new handle
  //
  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiVariableArchProtocolGuid,
                  NULL,
                  &gEfiVariableWriteArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VariableClassAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

#ifdef VBOX
  /* Self Test */
    {
        EFI_GUID TestUUID = {0xe660597e, 0xb94d, 0x4209, {0x9c, 0x80, 0x18, 0x05, 0xb5, 0xd1, 0x9b, 0x69}};
        const char *pszVariable0 = "This is test!!!";
        const CHAR16 *pszVariable1 = L"This is test!!!";
        char szTestVariable[512];
#if 0
        rc = runtime->SetVariable(&TestUUID,
            NULL ,
            (EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS| EFI_VARIABLE_RUNTIME_ACCESS),
            0,
            NULL );
        ASSERT(rc == EFI_INVALID_PARAMETER);
#endif
        UINTN size = sizeof(szTestVariable),
        rc = RuntimeServiceSetVariable(
            L"Test0" ,
            &TestUUID,
            (EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS| EFI_VARIABLE_RUNTIME_ACCESS),
            AsciiStrSize(pszVariable0),
            (void *)pszVariable0);
        ASSERT_EFI_ERROR(rc);
        SetMem(szTestVariable, 512, 0);
        rc = RuntimeServiceGetVariable(
            L"Test0" ,
            &TestUUID,
            NULL,
            &size,
            (void *)szTestVariable);
        VBoxLogFlowFuncMarkVar(szTestVariable, "%a");

        ASSERT(CompareMem(szTestVariable, pszVariable0, size) == 0);

        rc = RuntimeServiceSetVariable(
            L"Test1" ,
            &TestUUID,
            (EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS| EFI_VARIABLE_RUNTIME_ACCESS),
            StrSize(pszVariable1),
            (void *)pszVariable1);
        ASSERT_EFI_ERROR(rc);
        SetMem(szTestVariable, 512, 0);
        size = StrSize(pszVariable1);
        rc = RuntimeServiceGetVariable(
            L"Test1" ,
            &TestUUID,
            NULL,
            &size,
            (void *)szTestVariable);
        VBoxLogFlowFuncMarkVar((CHAR16 *)szTestVariable, "%s");
        ASSERT(CompareMem(szTestVariable, pszVariable1, size) == 0);
    }
#endif

  return EFI_SUCCESS;
}
