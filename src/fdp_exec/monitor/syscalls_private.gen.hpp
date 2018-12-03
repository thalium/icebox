#pragma once

#include "generic_mon.hpp"


bool monitor::GenericMonitor::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAcceptConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtAcceptConnectPort");

    d_->observers_NtAcceptConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAcceptConnectPort()
{
    if(false)
        LOG(INFO, "break on NtAcceptConnectPort");

    const auto PortHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto PortContext       = arg<nt::PVOID>(core_, 1);
    const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto AcceptConnection  = arg<nt::BOOLEAN>(core_, 3);
    const auto ServerView        = arg<nt::PPORT_VIEW>(core_, 4);
    const auto ClientView        = arg<nt::PREMOTE_PORT_VIEW>(core_, 5);

    for(const auto& it : d_->observers_NtAcceptConnectPort)
        it(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
}

bool monitor::GenericMonitor::register_NtAccessCheckAndAuditAlarm(proc_t proc, const on_NtAccessCheckAndAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckAndAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckAndAuditAlarm");

    d_->observers_NtAccessCheckAndAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckAndAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckAndAuditAlarm");

    const auto SubsystemName      = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId           = arg<nt::PVOID>(core_, 1);
    const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(core_, 2);
    const auto ObjectName         = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core_, 4);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 5);
    const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(core_, 6);
    const auto ObjectCreation     = arg<nt::BOOLEAN>(core_, 7);
    const auto GrantedAccess      = arg<nt::PACCESS_MASK>(core_, 8);
    const auto AccessStatus       = arg<nt::PNTSTATUS>(core_, 9);
    const auto GenerateOnClose    = arg<nt::PBOOLEAN>(core_, 10);

    for(const auto& it : d_->observers_NtAccessCheckAndAuditAlarm)
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeAndAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckByTypeAndAuditAlarm");

    d_->observers_NtAccessCheckByTypeAndAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeAndAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckByTypeAndAuditAlarm");

    const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId             = arg<nt::PVOID>(core_, 1);
    const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core_, 2);
    const auto ObjectName           = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core_, 4);
    const auto PrincipalSelfSid     = arg<nt::PSID>(core_, 5);
    const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core_, 6);
    const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core_, 7);
    const auto Flags                = arg<nt::ULONG>(core_, 8);
    const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core_, 9);
    const auto ObjectTypeListLength = arg<nt::ULONG>(core_, 10);
    const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core_, 11);
    const auto ObjectCreation       = arg<nt::BOOLEAN>(core_, 12);
    const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core_, 13);
    const auto AccessStatus         = arg<nt::PNTSTATUS>(core_, 14);
    const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core_, 15);

    for(const auto& it : d_->observers_NtAccessCheckByTypeAndAuditAlarm)
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckByType");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckByType");

    d_->observers_NtAccessCheckByType.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByType()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckByType");

    const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core_, 0);
    const auto PrincipalSelfSid     = arg<nt::PSID>(core_, 1);
    const auto ClientToken          = arg<nt::HANDLE>(core_, 2);
    const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core_, 3);
    const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core_, 4);
    const auto ObjectTypeListLength = arg<nt::ULONG>(core_, 5);
    const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core_, 6);
    const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(core_, 7);
    const auto PrivilegeSetLength   = arg<nt::PULONG>(core_, 8);
    const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core_, 9);
    const auto AccessStatus         = arg<nt::PNTSTATUS>(core_, 10);

    for(const auto& it : d_->observers_NtAccessCheckByType)
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckByTypeResultListAndAuditAlarmByHandle");

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckByTypeResultListAndAuditAlarmByHandle");

    const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId             = arg<nt::PVOID>(core_, 1);
    const auto ClientToken          = arg<nt::HANDLE>(core_, 2);
    const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto ObjectName           = arg<nt::PUNICODE_STRING>(core_, 4);
    const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core_, 5);
    const auto PrincipalSelfSid     = arg<nt::PSID>(core_, 6);
    const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core_, 7);
    const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core_, 8);
    const auto Flags                = arg<nt::ULONG>(core_, 9);
    const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core_, 10);
    const auto ObjectTypeListLength = arg<nt::ULONG>(core_, 11);
    const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core_, 12);
    const auto ObjectCreation       = arg<nt::BOOLEAN>(core_, 13);
    const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core_, 14);
    const auto AccessStatus         = arg<nt::PNTSTATUS>(core_, 15);
    const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core_, 16);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle)
        it(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultListAndAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckByTypeResultListAndAuditAlarm");

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckByTypeResultListAndAuditAlarm");

    const auto SubsystemName        = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId             = arg<nt::PVOID>(core_, 1);
    const auto ObjectTypeName       = arg<nt::PUNICODE_STRING>(core_, 2);
    const auto ObjectName           = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core_, 4);
    const auto PrincipalSelfSid     = arg<nt::PSID>(core_, 5);
    const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core_, 6);
    const auto AuditType            = arg<nt::AUDIT_EVENT_TYPE>(core_, 7);
    const auto Flags                = arg<nt::ULONG>(core_, 8);
    const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core_, 9);
    const auto ObjectTypeListLength = arg<nt::ULONG>(core_, 10);
    const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core_, 11);
    const auto ObjectCreation       = arg<nt::BOOLEAN>(core_, 12);
    const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core_, 13);
    const auto AccessStatus         = arg<nt::PNTSTATUS>(core_, 14);
    const auto GenerateOnClose      = arg<nt::PBOOLEAN>(core_, 15);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm)
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultList");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheckByTypeResultList");

    d_->observers_NtAccessCheckByTypeResultList.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultList()
{
    if(false)
        LOG(INFO, "break on NtAccessCheckByTypeResultList");

    const auto SecurityDescriptor   = arg<nt::PSECURITY_DESCRIPTOR>(core_, 0);
    const auto PrincipalSelfSid     = arg<nt::PSID>(core_, 1);
    const auto ClientToken          = arg<nt::HANDLE>(core_, 2);
    const auto DesiredAccess        = arg<nt::ACCESS_MASK>(core_, 3);
    const auto ObjectTypeList       = arg<nt::POBJECT_TYPE_LIST>(core_, 4);
    const auto ObjectTypeListLength = arg<nt::ULONG>(core_, 5);
    const auto GenericMapping       = arg<nt::PGENERIC_MAPPING>(core_, 6);
    const auto PrivilegeSet         = arg<nt::PPRIVILEGE_SET>(core_, 7);
    const auto PrivilegeSetLength   = arg<nt::PULONG>(core_, 8);
    const auto GrantedAccess        = arg<nt::PACCESS_MASK>(core_, 9);
    const auto AccessStatus         = arg<nt::PNTSTATUS>(core_, 10);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultList)
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAccessCheck");
    if(!ok)
        FAIL(false, "Unable to register NtAccessCheck");

    d_->observers_NtAccessCheck.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheck()
{
    if(false)
        LOG(INFO, "break on NtAccessCheck");

    const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core_, 0);
    const auto ClientToken        = arg<nt::HANDLE>(core_, 1);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 2);
    const auto GenericMapping     = arg<nt::PGENERIC_MAPPING>(core_, 3);
    const auto PrivilegeSet       = arg<nt::PPRIVILEGE_SET>(core_, 4);
    const auto PrivilegeSetLength = arg<nt::PULONG>(core_, 5);
    const auto GrantedAccess      = arg<nt::PACCESS_MASK>(core_, 6);
    const auto AccessStatus       = arg<nt::PNTSTATUS>(core_, 7);

    for(const auto& it : d_->observers_NtAccessCheck)
        it(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAddAtom");
    if(!ok)
        FAIL(false, "Unable to register NtAddAtom");

    d_->observers_NtAddAtom.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAddAtom()
{
    if(false)
        LOG(INFO, "break on NtAddAtom");

    const auto AtomName = arg<nt::PWSTR>(core_, 0);
    const auto Length   = arg<nt::ULONG>(core_, 1);
    const auto Atom     = arg<nt::PRTL_ATOM>(core_, 2);

    for(const auto& it : d_->observers_NtAddAtom)
        it(AtomName, Length, Atom);
}

bool monitor::GenericMonitor::register_NtAddBootEntry(proc_t proc, const on_NtAddBootEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAddBootEntry");
    if(!ok)
        FAIL(false, "Unable to register NtAddBootEntry");

    d_->observers_NtAddBootEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAddBootEntry()
{
    if(false)
        LOG(INFO, "break on NtAddBootEntry");

    const auto BootEntry = arg<nt::PBOOT_ENTRY>(core_, 0);
    const auto Id        = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtAddBootEntry)
        it(BootEntry, Id);
}

bool monitor::GenericMonitor::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAddDriverEntry");
    if(!ok)
        FAIL(false, "Unable to register NtAddDriverEntry");

    d_->observers_NtAddDriverEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAddDriverEntry()
{
    if(false)
        LOG(INFO, "break on NtAddDriverEntry");

    const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(core_, 0);
    const auto Id          = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtAddDriverEntry)
        it(DriverEntry, Id);
}

bool monitor::GenericMonitor::register_NtAdjustGroupsToken(proc_t proc, const on_NtAdjustGroupsToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAdjustGroupsToken");
    if(!ok)
        FAIL(false, "Unable to register NtAdjustGroupsToken");

    d_->observers_NtAdjustGroupsToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAdjustGroupsToken()
{
    if(false)
        LOG(INFO, "break on NtAdjustGroupsToken");

    const auto TokenHandle    = arg<nt::HANDLE>(core_, 0);
    const auto ResetToDefault = arg<nt::BOOLEAN>(core_, 1);
    const auto NewState       = arg<nt::PTOKEN_GROUPS>(core_, 2);
    const auto BufferLength   = arg<nt::ULONG>(core_, 3);
    const auto PreviousState  = arg<nt::PTOKEN_GROUPS>(core_, 4);
    const auto ReturnLength   = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtAdjustGroupsToken)
        it(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAdjustPrivilegesToken(proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAdjustPrivilegesToken");
    if(!ok)
        FAIL(false, "Unable to register NtAdjustPrivilegesToken");

    d_->observers_NtAdjustPrivilegesToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAdjustPrivilegesToken()
{
    if(false)
        LOG(INFO, "break on NtAdjustPrivilegesToken");

    const auto TokenHandle          = arg<nt::HANDLE>(core_, 0);
    const auto DisableAllPrivileges = arg<nt::BOOLEAN>(core_, 1);
    const auto NewState             = arg<nt::PTOKEN_PRIVILEGES>(core_, 2);
    const auto BufferLength         = arg<nt::ULONG>(core_, 3);
    const auto PreviousState        = arg<nt::PTOKEN_PRIVILEGES>(core_, 4);
    const auto ReturnLength         = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtAdjustPrivilegesToken)
        it(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlertResumeThread");
    if(!ok)
        FAIL(false, "Unable to register NtAlertResumeThread");

    d_->observers_NtAlertResumeThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlertResumeThread()
{
    if(false)
        LOG(INFO, "break on NtAlertResumeThread");

    const auto ThreadHandle         = arg<nt::HANDLE>(core_, 0);
    const auto PreviousSuspendCount = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtAlertResumeThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlertThread");
    if(!ok)
        FAIL(false, "Unable to register NtAlertThread");

    d_->observers_NtAlertThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlertThread()
{
    if(false)
        LOG(INFO, "break on NtAlertThread");

    const auto ThreadHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtAlertThread)
        it(ThreadHandle);
}

bool monitor::GenericMonitor::register_NtAllocateLocallyUniqueId(proc_t proc, const on_NtAllocateLocallyUniqueId_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAllocateLocallyUniqueId");
    if(!ok)
        FAIL(false, "Unable to register NtAllocateLocallyUniqueId");

    d_->observers_NtAllocateLocallyUniqueId.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateLocallyUniqueId()
{
    if(false)
        LOG(INFO, "break on NtAllocateLocallyUniqueId");

    const auto Luid = arg<nt::PLUID>(core_, 0);

    for(const auto& it : d_->observers_NtAllocateLocallyUniqueId)
        it(Luid);
}

bool monitor::GenericMonitor::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAllocateReserveObject");
    if(!ok)
        FAIL(false, "Unable to register NtAllocateReserveObject");

    d_->observers_NtAllocateReserveObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateReserveObject()
{
    if(false)
        LOG(INFO, "break on NtAllocateReserveObject");

    const auto MemoryReserveHandle = arg<nt::PHANDLE>(core_, 0);
    const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto Type                = arg<nt::MEMORY_RESERVE_TYPE>(core_, 2);

    for(const auto& it : d_->observers_NtAllocateReserveObject)
        it(MemoryReserveHandle, ObjectAttributes, Type);
}

bool monitor::GenericMonitor::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAllocateUserPhysicalPages");
    if(!ok)
        FAIL(false, "Unable to register NtAllocateUserPhysicalPages");

    d_->observers_NtAllocateUserPhysicalPages.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateUserPhysicalPages()
{
    if(false)
        LOG(INFO, "break on NtAllocateUserPhysicalPages");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto NumberOfPages = arg<nt::PULONG_PTR>(core_, 1);
    const auto UserPfnArra   = arg<nt::PULONG_PTR>(core_, 2);

    for(const auto& it : d_->observers_NtAllocateUserPhysicalPages)
        it(ProcessHandle, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAllocateUuids");
    if(!ok)
        FAIL(false, "Unable to register NtAllocateUuids");

    d_->observers_NtAllocateUuids.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateUuids()
{
    if(false)
        LOG(INFO, "break on NtAllocateUuids");

    const auto Time     = arg<nt::PULARGE_INTEGER>(core_, 0);
    const auto Range    = arg<nt::PULONG>(core_, 1);
    const auto Sequence = arg<nt::PULONG>(core_, 2);
    const auto Seed     = arg<nt::PCHAR>(core_, 3);

    for(const auto& it : d_->observers_NtAllocateUuids)
        it(Time, Range, Sequence, Seed);
}

bool monitor::GenericMonitor::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAllocateVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtAllocateVirtualMemory");

    d_->observers_NtAllocateVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtAllocateVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto ZeroBits        = arg<nt::ULONG_PTR>(core_, 2);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 3);
    const auto AllocationType  = arg<nt::ULONG>(core_, 4);
    const auto Protect         = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtAllocateVirtualMemory)
        it(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

bool monitor::GenericMonitor::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcAcceptConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcAcceptConnectPort");

    d_->observers_NtAlpcAcceptConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcAcceptConnectPort()
{
    if(false)
        LOG(INFO, "break on NtAlpcAcceptConnectPort");

    const auto PortHandle                  = arg<nt::PHANDLE>(core_, 0);
    const auto ConnectionPortHandle        = arg<nt::HANDLE>(core_, 1);
    const auto Flags                       = arg<nt::ULONG>(core_, 2);
    const auto ObjectAttributes            = arg<nt::POBJECT_ATTRIBUTES>(core_, 3);
    const auto PortAttributes              = arg<nt::PALPC_PORT_ATTRIBUTES>(core_, 4);
    const auto PortContext                 = arg<nt::PVOID>(core_, 5);
    const auto ConnectionRequest           = arg<nt::PPORT_MESSAGE>(core_, 6);
    const auto ConnectionMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core_, 7);
    const auto AcceptConnection            = arg<nt::BOOLEAN>(core_, 8);

    for(const auto& it : d_->observers_NtAlpcAcceptConnectPort)
        it(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
}

bool monitor::GenericMonitor::register_NtAlpcCancelMessage(proc_t proc, const on_NtAlpcCancelMessage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCancelMessage");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCancelMessage");

    d_->observers_NtAlpcCancelMessage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCancelMessage()
{
    if(false)
        LOG(INFO, "break on NtAlpcCancelMessage");

    const auto PortHandle     = arg<nt::HANDLE>(core_, 0);
    const auto Flags          = arg<nt::ULONG>(core_, 1);
    const auto MessageContext = arg<nt::PALPC_CONTEXT_ATTR>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcCancelMessage)
        it(PortHandle, Flags, MessageContext);
}

bool monitor::GenericMonitor::register_NtAlpcConnectPort(proc_t proc, const on_NtAlpcConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcConnectPort");

    d_->observers_NtAlpcConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcConnectPort()
{
    if(false)
        LOG(INFO, "break on NtAlpcConnectPort");

    const auto PortHandle           = arg<nt::PHANDLE>(core_, 0);
    const auto PortName             = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto ObjectAttributes     = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto PortAttributes       = arg<nt::PALPC_PORT_ATTRIBUTES>(core_, 3);
    const auto Flags                = arg<nt::ULONG>(core_, 4);
    const auto RequiredServerSid    = arg<nt::PSID>(core_, 5);
    const auto ConnectionMessage    = arg<nt::PPORT_MESSAGE>(core_, 6);
    const auto BufferLength         = arg<nt::PULONG>(core_, 7);
    const auto OutMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core_, 8);
    const auto InMessageAttributes  = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core_, 9);
    const auto Timeout              = arg<nt::PLARGE_INTEGER>(core_, 10);

    for(const auto& it : d_->observers_NtAlpcConnectPort)
        it(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
}

bool monitor::GenericMonitor::register_NtAlpcCreatePort(proc_t proc, const on_NtAlpcCreatePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCreatePort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCreatePort");

    d_->observers_NtAlpcCreatePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreatePort()
{
    if(false)
        LOG(INFO, "break on NtAlpcCreatePort");

    const auto PortHandle       = arg<nt::PHANDLE>(core_, 0);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto PortAttributes   = arg<nt::PALPC_PORT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcCreatePort)
        it(PortHandle, ObjectAttributes, PortAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCreatePortSection");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCreatePortSection");

    d_->observers_NtAlpcCreatePortSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreatePortSection()
{
    if(false)
        LOG(INFO, "break on NtAlpcCreatePortSection");

    const auto PortHandle        = arg<nt::HANDLE>(core_, 0);
    const auto Flags             = arg<nt::ULONG>(core_, 1);
    const auto SectionHandle     = arg<nt::HANDLE>(core_, 2);
    const auto SectionSize       = arg<nt::SIZE_T>(core_, 3);
    const auto AlpcSectionHandle = arg<nt::PALPC_HANDLE>(core_, 4);
    const auto ActualSectionSize = arg<nt::PSIZE_T>(core_, 5);

    for(const auto& it : d_->observers_NtAlpcCreatePortSection)
        it(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
}

bool monitor::GenericMonitor::register_NtAlpcCreateResourceReserve(proc_t proc, const on_NtAlpcCreateResourceReserve_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCreateResourceReserve");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCreateResourceReserve");

    d_->observers_NtAlpcCreateResourceReserve.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateResourceReserve()
{
    if(false)
        LOG(INFO, "break on NtAlpcCreateResourceReserve");

    const auto PortHandle  = arg<nt::HANDLE>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto MessageSize = arg<nt::SIZE_T>(core_, 2);
    const auto ResourceId  = arg<nt::PALPC_HANDLE>(core_, 3);

    for(const auto& it : d_->observers_NtAlpcCreateResourceReserve)
        it(PortHandle, Flags, MessageSize, ResourceId);
}

bool monitor::GenericMonitor::register_NtAlpcCreateSectionView(proc_t proc, const on_NtAlpcCreateSectionView_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCreateSectionView");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCreateSectionView");

    d_->observers_NtAlpcCreateSectionView.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateSectionView()
{
    if(false)
        LOG(INFO, "break on NtAlpcCreateSectionView");

    const auto PortHandle     = arg<nt::HANDLE>(core_, 0);
    const auto Flags          = arg<nt::ULONG>(core_, 1);
    const auto ViewAttributes = arg<nt::PALPC_DATA_VIEW_ATTR>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcCreateSectionView)
        it(PortHandle, Flags, ViewAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcCreateSecurityContext(proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcCreateSecurityContext");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcCreateSecurityContext");

    d_->observers_NtAlpcCreateSecurityContext.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateSecurityContext()
{
    if(false)
        LOG(INFO, "break on NtAlpcCreateSecurityContext");

    const auto PortHandle        = arg<nt::HANDLE>(core_, 0);
    const auto Flags             = arg<nt::ULONG>(core_, 1);
    const auto SecurityAttribute = arg<nt::PALPC_SECURITY_ATTR>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcCreateSecurityContext)
        it(PortHandle, Flags, SecurityAttribute);
}

bool monitor::GenericMonitor::register_NtAlpcDeletePortSection(proc_t proc, const on_NtAlpcDeletePortSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcDeletePortSection");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcDeletePortSection");

    d_->observers_NtAlpcDeletePortSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeletePortSection()
{
    if(false)
        LOG(INFO, "break on NtAlpcDeletePortSection");

    const auto PortHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Flags         = arg<nt::ULONG>(core_, 1);
    const auto SectionHandle = arg<nt::ALPC_HANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcDeletePortSection)
        it(PortHandle, Flags, SectionHandle);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteResourceReserve");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcDeleteResourceReserve");

    d_->observers_NtAlpcDeleteResourceReserve.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteResourceReserve()
{
    if(false)
        LOG(INFO, "break on NtAlpcDeleteResourceReserve");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);
    const auto Flags      = arg<nt::ULONG>(core_, 1);
    const auto ResourceId = arg<nt::ALPC_HANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcDeleteResourceReserve)
        it(PortHandle, Flags, ResourceId);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteSectionView");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcDeleteSectionView");

    d_->observers_NtAlpcDeleteSectionView.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteSectionView()
{
    if(false)
        LOG(INFO, "break on NtAlpcDeleteSectionView");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);
    const auto Flags      = arg<nt::ULONG>(core_, 1);
    const auto ViewBase   = arg<nt::PVOID>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcDeleteSectionView)
        it(PortHandle, Flags, ViewBase);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteSecurityContext");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcDeleteSecurityContext");

    d_->observers_NtAlpcDeleteSecurityContext.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteSecurityContext()
{
    if(false)
        LOG(INFO, "break on NtAlpcDeleteSecurityContext");

    const auto PortHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Flags         = arg<nt::ULONG>(core_, 1);
    const auto ContextHandle = arg<nt::ALPC_HANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcDeleteSecurityContext)
        it(PortHandle, Flags, ContextHandle);
}

bool monitor::GenericMonitor::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcDisconnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcDisconnectPort");

    d_->observers_NtAlpcDisconnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDisconnectPort()
{
    if(false)
        LOG(INFO, "break on NtAlpcDisconnectPort");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);
    const auto Flags      = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtAlpcDisconnectPort)
        it(PortHandle, Flags);
}

bool monitor::GenericMonitor::register_NtAlpcImpersonateClientOfPort(proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcImpersonateClientOfPort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcImpersonateClientOfPort");

    d_->observers_NtAlpcImpersonateClientOfPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcImpersonateClientOfPort()
{
    if(false)
        LOG(INFO, "break on NtAlpcImpersonateClientOfPort");

    const auto PortHandle  = arg<nt::HANDLE>(core_, 0);
    const auto PortMessage = arg<nt::PPORT_MESSAGE>(core_, 1);
    const auto Reserved    = arg<nt::PVOID>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcImpersonateClientOfPort)
        it(PortHandle, PortMessage, Reserved);
}

bool monitor::GenericMonitor::register_NtAlpcOpenSenderProcess(proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcOpenSenderProcess");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcOpenSenderProcess");

    d_->observers_NtAlpcOpenSenderProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderProcess()
{
    if(false)
        LOG(INFO, "break on NtAlpcOpenSenderProcess");

    const auto ProcessHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto PortHandle       = arg<nt::HANDLE>(core_, 1);
    const auto PortMessage      = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto Flags            = arg<nt::ULONG>(core_, 3);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 4);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 5);

    for(const auto& it : d_->observers_NtAlpcOpenSenderProcess)
        it(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcOpenSenderThread(proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcOpenSenderThread");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcOpenSenderThread");

    d_->observers_NtAlpcOpenSenderThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderThread()
{
    if(false)
        LOG(INFO, "break on NtAlpcOpenSenderThread");

    const auto ThreadHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto PortHandle       = arg<nt::HANDLE>(core_, 1);
    const auto PortMessage      = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto Flags            = arg<nt::ULONG>(core_, 3);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 4);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 5);

    for(const auto& it : d_->observers_NtAlpcOpenSenderThread)
        it(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcQueryInformation(proc_t proc, const on_NtAlpcQueryInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcQueryInformation");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcQueryInformation");

    d_->observers_NtAlpcQueryInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcQueryInformation()
{
    if(false)
        LOG(INFO, "break on NtAlpcQueryInformation");

    const auto PortHandle           = arg<nt::HANDLE>(core_, 0);
    const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(core_, 1);
    const auto PortInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length               = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength         = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtAlpcQueryInformation)
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlpcQueryInformationMessage(proc_t proc, const on_NtAlpcQueryInformationMessage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcQueryInformationMessage");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcQueryInformationMessage");

    d_->observers_NtAlpcQueryInformationMessage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcQueryInformationMessage()
{
    if(false)
        LOG(INFO, "break on NtAlpcQueryInformationMessage");

    const auto PortHandle              = arg<nt::HANDLE>(core_, 0);
    const auto PortMessage             = arg<nt::PPORT_MESSAGE>(core_, 1);
    const auto MessageInformationClass = arg<nt::ALPC_MESSAGE_INFORMATION_CLASS>(core_, 2);
    const auto MessageInformation      = arg<nt::PVOID>(core_, 3);
    const auto Length                  = arg<nt::ULONG>(core_, 4);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtAlpcQueryInformationMessage)
        it(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcRevokeSecurityContext");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcRevokeSecurityContext");

    d_->observers_NtAlpcRevokeSecurityContext.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcRevokeSecurityContext()
{
    if(false)
        LOG(INFO, "break on NtAlpcRevokeSecurityContext");

    const auto PortHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Flags         = arg<nt::ULONG>(core_, 1);
    const auto ContextHandle = arg<nt::ALPC_HANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtAlpcRevokeSecurityContext)
        it(PortHandle, Flags, ContextHandle);
}

bool monitor::GenericMonitor::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcSendWaitReceivePort");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcSendWaitReceivePort");

    d_->observers_NtAlpcSendWaitReceivePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcSendWaitReceivePort()
{
    if(false)
        LOG(INFO, "break on NtAlpcSendWaitReceivePort");

    const auto PortHandle               = arg<nt::HANDLE>(core_, 0);
    const auto Flags                    = arg<nt::ULONG>(core_, 1);
    const auto SendMessage              = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto SendMessageAttributes    = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core_, 3);
    const auto ReceiveMessage           = arg<nt::PPORT_MESSAGE>(core_, 4);
    const auto BufferLength             = arg<nt::PULONG>(core_, 5);
    const auto ReceiveMessageAttributes = arg<nt::PALPC_MESSAGE_ATTRIBUTES>(core_, 6);
    const auto Timeout                  = arg<nt::PLARGE_INTEGER>(core_, 7);

    for(const auto& it : d_->observers_NtAlpcSendWaitReceivePort)
        it(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
}

bool monitor::GenericMonitor::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAlpcSetInformation");
    if(!ok)
        FAIL(false, "Unable to register NtAlpcSetInformation");

    d_->observers_NtAlpcSetInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcSetInformation()
{
    if(false)
        LOG(INFO, "break on NtAlpcSetInformation");

    const auto PortHandle           = arg<nt::HANDLE>(core_, 0);
    const auto PortInformationClass = arg<nt::ALPC_PORT_INFORMATION_CLASS>(core_, 1);
    const auto PortInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length               = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtAlpcSetInformation)
        it(PortHandle, PortInformationClass, PortInformation, Length);
}

bool monitor::GenericMonitor::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_func)
{
    const auto ok = setup_func(proc, "NtApphelpCacheControl");
    if(!ok)
        FAIL(false, "Unable to register NtApphelpCacheControl");

    d_->observers_NtApphelpCacheControl.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtApphelpCacheControl()
{
    if(false)
        LOG(INFO, "break on NtApphelpCacheControl");

    const auto type = arg<nt::APPHELPCOMMAND>(core_, 0);
    const auto buf  = arg<nt::PVOID>(core_, 1);

    for(const auto& it : d_->observers_NtApphelpCacheControl)
        it(type, buf);
}

bool monitor::GenericMonitor::register_NtAreMappedFilesTheSame(proc_t proc, const on_NtAreMappedFilesTheSame_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAreMappedFilesTheSame");
    if(!ok)
        FAIL(false, "Unable to register NtAreMappedFilesTheSame");

    d_->observers_NtAreMappedFilesTheSame.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAreMappedFilesTheSame()
{
    if(false)
        LOG(INFO, "break on NtAreMappedFilesTheSame");

    const auto File1MappedAsAnImage = arg<nt::PVOID>(core_, 0);
    const auto File2MappedAsFile    = arg<nt::PVOID>(core_, 1);

    for(const auto& it : d_->observers_NtAreMappedFilesTheSame)
        it(File1MappedAsAnImage, File2MappedAsFile);
}

bool monitor::GenericMonitor::register_NtAssignProcessToJobObject(proc_t proc, const on_NtAssignProcessToJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtAssignProcessToJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtAssignProcessToJobObject");

    d_->observers_NtAssignProcessToJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtAssignProcessToJobObject()
{
    if(false)
        LOG(INFO, "break on NtAssignProcessToJobObject");

    const auto JobHandle     = arg<nt::HANDLE>(core_, 0);
    const auto ProcessHandle = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtAssignProcessToJobObject)
        it(JobHandle, ProcessHandle);
}

bool monitor::GenericMonitor::register_NtCallbackReturn(proc_t proc, const on_NtCallbackReturn_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCallbackReturn");
    if(!ok)
        FAIL(false, "Unable to register NtCallbackReturn");

    d_->observers_NtCallbackReturn.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCallbackReturn()
{
    if(false)
        LOG(INFO, "break on NtCallbackReturn");

    const auto OutputBuffer = arg<nt::PVOID>(core_, 0);
    const auto OutputLength = arg<nt::ULONG>(core_, 1);
    const auto Status       = arg<nt::NTSTATUS>(core_, 2);

    for(const auto& it : d_->observers_NtCallbackReturn)
        it(OutputBuffer, OutputLength, Status);
}

bool monitor::GenericMonitor::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCancelIoFileEx");
    if(!ok)
        FAIL(false, "Unable to register NtCancelIoFileEx");

    d_->observers_NtCancelIoFileEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCancelIoFileEx()
{
    if(false)
        LOG(INFO, "break on NtCancelIoFileEx");

    const auto FileHandle        = arg<nt::HANDLE>(core_, 0);
    const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 2);

    for(const auto& it : d_->observers_NtCancelIoFileEx)
        it(FileHandle, IoRequestToCancel, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelIoFile(proc_t proc, const on_NtCancelIoFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCancelIoFile");
    if(!ok)
        FAIL(false, "Unable to register NtCancelIoFile");

    d_->observers_NtCancelIoFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCancelIoFile()
{
    if(false)
        LOG(INFO, "break on NtCancelIoFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 1);

    for(const auto& it : d_->observers_NtCancelIoFile)
        it(FileHandle, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCancelSynchronousIoFile");
    if(!ok)
        FAIL(false, "Unable to register NtCancelSynchronousIoFile");

    d_->observers_NtCancelSynchronousIoFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCancelSynchronousIoFile()
{
    if(false)
        LOG(INFO, "break on NtCancelSynchronousIoFile");

    const auto ThreadHandle      = arg<nt::HANDLE>(core_, 0);
    const auto IoRequestToCancel = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 2);

    for(const auto& it : d_->observers_NtCancelSynchronousIoFile)
        it(ThreadHandle, IoRequestToCancel, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelTimer(proc_t proc, const on_NtCancelTimer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCancelTimer");
    if(!ok)
        FAIL(false, "Unable to register NtCancelTimer");

    d_->observers_NtCancelTimer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCancelTimer()
{
    if(false)
        LOG(INFO, "break on NtCancelTimer");

    const auto TimerHandle  = arg<nt::HANDLE>(core_, 0);
    const auto CurrentState = arg<nt::PBOOLEAN>(core_, 1);

    for(const auto& it : d_->observers_NtCancelTimer)
        it(TimerHandle, CurrentState);
}

bool monitor::GenericMonitor::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtClearEvent");
    if(!ok)
        FAIL(false, "Unable to register NtClearEvent");

    d_->observers_NtClearEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtClearEvent()
{
    if(false)
        LOG(INFO, "break on NtClearEvent");

    const auto EventHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtClearEvent)
        it(EventHandle);
}

bool monitor::GenericMonitor::register_NtClose(proc_t proc, const on_NtClose_fn& on_func)
{
    const auto ok = setup_func(proc, "NtClose");
    if(!ok)
        FAIL(false, "Unable to register NtClose");

    d_->observers_NtClose.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtClose()
{
    if(false)
        LOG(INFO, "break on NtClose");

    const auto Handle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtClose)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtCloseObjectAuditAlarm(proc_t proc, const on_NtCloseObjectAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCloseObjectAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtCloseObjectAuditAlarm");

    d_->observers_NtCloseObjectAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCloseObjectAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtCloseObjectAuditAlarm");

    const auto SubsystemName   = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId        = arg<nt::PVOID>(core_, 1);
    const auto GenerateOnClose = arg<nt::BOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtCloseObjectAuditAlarm)
        it(SubsystemName, HandleId, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtCommitComplete(proc_t proc, const on_NtCommitComplete_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCommitComplete");
    if(!ok)
        FAIL(false, "Unable to register NtCommitComplete");

    d_->observers_NtCommitComplete.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCommitComplete()
{
    if(false)
        LOG(INFO, "break on NtCommitComplete");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtCommitComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCommitEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtCommitEnlistment");

    d_->observers_NtCommitEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCommitEnlistment()
{
    if(false)
        LOG(INFO, "break on NtCommitEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtCommitEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCommitTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtCommitTransaction");

    d_->observers_NtCommitTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCommitTransaction()
{
    if(false)
        LOG(INFO, "break on NtCommitTransaction");

    const auto TransactionHandle = arg<nt::HANDLE>(core_, 0);
    const auto Wait              = arg<nt::BOOLEAN>(core_, 1);

    for(const auto& it : d_->observers_NtCommitTransaction)
        it(TransactionHandle, Wait);
}

bool monitor::GenericMonitor::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCompactKeys");
    if(!ok)
        FAIL(false, "Unable to register NtCompactKeys");

    d_->observers_NtCompactKeys.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCompactKeys()
{
    if(false)
        LOG(INFO, "break on NtCompactKeys");

    const auto Count    = arg<nt::ULONG>(core_, 0);
    const auto KeyArray = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtCompactKeys)
        it(Count, KeyArray);
}

bool monitor::GenericMonitor::register_NtCompareTokens(proc_t proc, const on_NtCompareTokens_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCompareTokens");
    if(!ok)
        FAIL(false, "Unable to register NtCompareTokens");

    d_->observers_NtCompareTokens.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCompareTokens()
{
    if(false)
        LOG(INFO, "break on NtCompareTokens");

    const auto FirstTokenHandle  = arg<nt::HANDLE>(core_, 0);
    const auto SecondTokenHandle = arg<nt::HANDLE>(core_, 1);
    const auto Equal             = arg<nt::PBOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtCompareTokens)
        it(FirstTokenHandle, SecondTokenHandle, Equal);
}

bool monitor::GenericMonitor::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCompleteConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtCompleteConnectPort");

    d_->observers_NtCompleteConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCompleteConnectPort()
{
    if(false)
        LOG(INFO, "break on NtCompleteConnectPort");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtCompleteConnectPort)
        it(PortHandle);
}

bool monitor::GenericMonitor::register_NtCompressKey(proc_t proc, const on_NtCompressKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCompressKey");
    if(!ok)
        FAIL(false, "Unable to register NtCompressKey");

    d_->observers_NtCompressKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCompressKey()
{
    if(false)
        LOG(INFO, "break on NtCompressKey");

    const auto Key = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtCompressKey)
        it(Key);
}

bool monitor::GenericMonitor::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtConnectPort");

    d_->observers_NtConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtConnectPort()
{
    if(false)
        LOG(INFO, "break on NtConnectPort");

    const auto PortHandle                  = arg<nt::PHANDLE>(core_, 0);
    const auto PortName                    = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core_, 2);
    const auto ClientView                  = arg<nt::PPORT_VIEW>(core_, 3);
    const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(core_, 4);
    const auto MaxMessageLength            = arg<nt::PULONG>(core_, 5);
    const auto ConnectionInformation       = arg<nt::PVOID>(core_, 6);
    const auto ConnectionInformationLength = arg<nt::PULONG>(core_, 7);

    for(const auto& it : d_->observers_NtConnectPort)
        it(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
}

bool monitor::GenericMonitor::register_NtContinue(proc_t proc, const on_NtContinue_fn& on_func)
{
    const auto ok = setup_func(proc, "NtContinue");
    if(!ok)
        FAIL(false, "Unable to register NtContinue");

    d_->observers_NtContinue.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtContinue()
{
    if(false)
        LOG(INFO, "break on NtContinue");

    const auto ContextRecord = arg<nt::PCONTEXT>(core_, 0);
    const auto TestAlert     = arg<nt::BOOLEAN>(core_, 1);

    for(const auto& it : d_->observers_NtContinue)
        it(ContextRecord, TestAlert);
}

bool monitor::GenericMonitor::register_NtCreateDebugObject(proc_t proc, const on_NtCreateDebugObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateDebugObject");
    if(!ok)
        FAIL(false, "Unable to register NtCreateDebugObject");

    d_->observers_NtCreateDebugObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateDebugObject()
{
    if(false)
        LOG(INFO, "break on NtCreateDebugObject");

    const auto DebugObjectHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto Flags             = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtCreateDebugObject)
        it(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
}

bool monitor::GenericMonitor::register_NtCreateDirectoryObject(proc_t proc, const on_NtCreateDirectoryObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateDirectoryObject");
    if(!ok)
        FAIL(false, "Unable to register NtCreateDirectoryObject");

    d_->observers_NtCreateDirectoryObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateDirectoryObject()
{
    if(false)
        LOG(INFO, "break on NtCreateDirectoryObject");

    const auto DirectoryHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtCreateDirectoryObject)
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateEnlistment(proc_t proc, const on_NtCreateEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtCreateEnlistment");

    d_->observers_NtCreateEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEnlistment()
{
    if(false)
        LOG(INFO, "break on NtCreateEnlistment");

    const auto EnlistmentHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ResourceManagerHandle = arg<nt::HANDLE>(core_, 2);
    const auto TransactionHandle     = arg<nt::HANDLE>(core_, 3);
    const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core_, 4);
    const auto CreateOptions         = arg<nt::ULONG>(core_, 5);
    const auto NotificationMask      = arg<nt::NOTIFICATION_MASK>(core_, 6);
    const auto EnlistmentKey         = arg<nt::PVOID>(core_, 7);

    for(const auto& it : d_->observers_NtCreateEnlistment)
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
}

bool monitor::GenericMonitor::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateEvent");
    if(!ok)
        FAIL(false, "Unable to register NtCreateEvent");

    d_->observers_NtCreateEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEvent()
{
    if(false)
        LOG(INFO, "break on NtCreateEvent");

    const auto EventHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto EventType        = arg<nt::EVENT_TYPE>(core_, 3);
    const auto InitialState     = arg<nt::BOOLEAN>(core_, 4);

    for(const auto& it : d_->observers_NtCreateEvent)
        it(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
}

bool monitor::GenericMonitor::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtCreateEventPair");

    d_->observers_NtCreateEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEventPair()
{
    if(false)
        LOG(INFO, "break on NtCreateEventPair");

    const auto EventPairHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtCreateEventPair)
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateFile");
    if(!ok)
        FAIL(false, "Unable to register NtCreateFile");

    d_->observers_NtCreateFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateFile()
{
    if(false)
        LOG(INFO, "break on NtCreateFile");

    const auto FileHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 3);
    const auto AllocationSize    = arg<nt::PLARGE_INTEGER>(core_, 4);
    const auto FileAttributes    = arg<nt::ULONG>(core_, 5);
    const auto ShareAccess       = arg<nt::ULONG>(core_, 6);
    const auto CreateDisposition = arg<nt::ULONG>(core_, 7);
    const auto CreateOptions     = arg<nt::ULONG>(core_, 8);
    const auto EaBuffer          = arg<nt::PVOID>(core_, 9);
    const auto EaLength          = arg<nt::ULONG>(core_, 10);

    for(const auto& it : d_->observers_NtCreateFile)
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

bool monitor::GenericMonitor::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateIoCompletion");
    if(!ok)
        FAIL(false, "Unable to register NtCreateIoCompletion");

    d_->observers_NtCreateIoCompletion.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateIoCompletion()
{
    if(false)
        LOG(INFO, "break on NtCreateIoCompletion");

    const auto IoCompletionHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto Count              = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtCreateIoCompletion)
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
}

bool monitor::GenericMonitor::register_NtCreateJobObject(proc_t proc, const on_NtCreateJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtCreateJobObject");

    d_->observers_NtCreateJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateJobObject()
{
    if(false)
        LOG(INFO, "break on NtCreateJobObject");

    const auto JobHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtCreateJobObject)
        it(JobHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateJobSet");
    if(!ok)
        FAIL(false, "Unable to register NtCreateJobSet");

    d_->observers_NtCreateJobSet.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateJobSet()
{
    if(false)
        LOG(INFO, "break on NtCreateJobSet");

    const auto NumJob     = arg<nt::ULONG>(core_, 0);
    const auto UserJobSet = arg<nt::PJOB_SET_ARRAY>(core_, 1);
    const auto Flags      = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtCreateJobSet)
        it(NumJob, UserJobSet, Flags);
}

bool monitor::GenericMonitor::register_NtCreateKeyedEvent(proc_t proc, const on_NtCreateKeyedEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateKeyedEvent");
    if(!ok)
        FAIL(false, "Unable to register NtCreateKeyedEvent");

    d_->observers_NtCreateKeyedEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKeyedEvent()
{
    if(false)
        LOG(INFO, "break on NtCreateKeyedEvent");

    const auto KeyedEventHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto Flags            = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtCreateKeyedEvent)
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
}

bool monitor::GenericMonitor::register_NtCreateKey(proc_t proc, const on_NtCreateKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateKey");
    if(!ok)
        FAIL(false, "Unable to register NtCreateKey");

    d_->observers_NtCreateKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKey()
{
    if(false)
        LOG(INFO, "break on NtCreateKey");

    const auto KeyHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto TitleIndex       = arg<nt::ULONG>(core_, 3);
    const auto Class            = arg<nt::PUNICODE_STRING>(core_, 4);
    const auto CreateOptions    = arg<nt::ULONG>(core_, 5);
    const auto Disposition      = arg<nt::PULONG>(core_, 6);

    for(const auto& it : d_->observers_NtCreateKey)
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}

bool monitor::GenericMonitor::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateKeyTransacted");
    if(!ok)
        FAIL(false, "Unable to register NtCreateKeyTransacted");

    d_->observers_NtCreateKeyTransacted.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKeyTransacted()
{
    if(false)
        LOG(INFO, "break on NtCreateKeyTransacted");

    const auto KeyHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto TitleIndex        = arg<nt::ULONG>(core_, 3);
    const auto Class             = arg<nt::PUNICODE_STRING>(core_, 4);
    const auto CreateOptions     = arg<nt::ULONG>(core_, 5);
    const auto TransactionHandle = arg<nt::HANDLE>(core_, 6);
    const auto Disposition       = arg<nt::PULONG>(core_, 7);

    for(const auto& it : d_->observers_NtCreateKeyTransacted)
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
}

bool monitor::GenericMonitor::register_NtCreateMailslotFile(proc_t proc, const on_NtCreateMailslotFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateMailslotFile");
    if(!ok)
        FAIL(false, "Unable to register NtCreateMailslotFile");

    d_->observers_NtCreateMailslotFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateMailslotFile()
{
    if(false)
        LOG(INFO, "break on NtCreateMailslotFile");

    const auto FileHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ULONG>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 3);
    const auto CreateOptions      = arg<nt::ULONG>(core_, 4);
    const auto MailslotQuota      = arg<nt::ULONG>(core_, 5);
    const auto MaximumMessageSize = arg<nt::ULONG>(core_, 6);
    const auto ReadTimeout        = arg<nt::PLARGE_INTEGER>(core_, 7);

    for(const auto& it : d_->observers_NtCreateMailslotFile)
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
}

bool monitor::GenericMonitor::register_NtCreateMutant(proc_t proc, const on_NtCreateMutant_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateMutant");
    if(!ok)
        FAIL(false, "Unable to register NtCreateMutant");

    d_->observers_NtCreateMutant.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateMutant()
{
    if(false)
        LOG(INFO, "break on NtCreateMutant");

    const auto MutantHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto InitialOwner     = arg<nt::BOOLEAN>(core_, 3);

    for(const auto& it : d_->observers_NtCreateMutant)
        it(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
}

bool monitor::GenericMonitor::register_NtCreateNamedPipeFile(proc_t proc, const on_NtCreateNamedPipeFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateNamedPipeFile");
    if(!ok)
        FAIL(false, "Unable to register NtCreateNamedPipeFile");

    d_->observers_NtCreateNamedPipeFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateNamedPipeFile()
{
    if(false)
        LOG(INFO, "break on NtCreateNamedPipeFile");

    const auto FileHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ULONG>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 3);
    const auto ShareAccess       = arg<nt::ULONG>(core_, 4);
    const auto CreateDisposition = arg<nt::ULONG>(core_, 5);
    const auto CreateOptions     = arg<nt::ULONG>(core_, 6);
    const auto NamedPipeType     = arg<nt::ULONG>(core_, 7);
    const auto ReadMode          = arg<nt::ULONG>(core_, 8);
    const auto CompletionMode    = arg<nt::ULONG>(core_, 9);
    const auto MaximumInstances  = arg<nt::ULONG>(core_, 10);
    const auto InboundQuota      = arg<nt::ULONG>(core_, 11);
    const auto OutboundQuota     = arg<nt::ULONG>(core_, 12);
    const auto DefaultTimeout    = arg<nt::PLARGE_INTEGER>(core_, 13);

    for(const auto& it : d_->observers_NtCreateNamedPipeFile)
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
}

bool monitor::GenericMonitor::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreatePagingFile");
    if(!ok)
        FAIL(false, "Unable to register NtCreatePagingFile");

    d_->observers_NtCreatePagingFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePagingFile()
{
    if(false)
        LOG(INFO, "break on NtCreatePagingFile");

    const auto PageFileName = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto MinimumSize  = arg<nt::PLARGE_INTEGER>(core_, 1);
    const auto MaximumSize  = arg<nt::PLARGE_INTEGER>(core_, 2);
    const auto Priority     = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtCreatePagingFile)
        it(PageFileName, MinimumSize, MaximumSize, Priority);
}

bool monitor::GenericMonitor::register_NtCreatePort(proc_t proc, const on_NtCreatePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreatePort");
    if(!ok)
        FAIL(false, "Unable to register NtCreatePort");

    d_->observers_NtCreatePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePort()
{
    if(false)
        LOG(INFO, "break on NtCreatePort");

    const auto PortHandle              = arg<nt::PHANDLE>(core_, 0);
    const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto MaxConnectionInfoLength = arg<nt::ULONG>(core_, 2);
    const auto MaxMessageLength        = arg<nt::ULONG>(core_, 3);
    const auto MaxPoolUsage            = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtCreatePort)
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
}

bool monitor::GenericMonitor::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreatePrivateNamespace");
    if(!ok)
        FAIL(false, "Unable to register NtCreatePrivateNamespace");

    d_->observers_NtCreatePrivateNamespace.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePrivateNamespace()
{
    if(false)
        LOG(INFO, "break on NtCreatePrivateNamespace");

    const auto NamespaceHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto BoundaryDescriptor = arg<nt::PVOID>(core_, 3);

    for(const auto& it : d_->observers_NtCreatePrivateNamespace)
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
}

bool monitor::GenericMonitor::register_NtCreateProcessEx(proc_t proc, const on_NtCreateProcessEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateProcessEx");
    if(!ok)
        FAIL(false, "Unable to register NtCreateProcessEx");

    d_->observers_NtCreateProcessEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProcessEx()
{
    if(false)
        LOG(INFO, "break on NtCreateProcessEx");

    const auto ProcessHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ParentProcess    = arg<nt::HANDLE>(core_, 3);
    const auto Flags            = arg<nt::ULONG>(core_, 4);
    const auto SectionHandle    = arg<nt::HANDLE>(core_, 5);
    const auto DebugPort        = arg<nt::HANDLE>(core_, 6);
    const auto ExceptionPort    = arg<nt::HANDLE>(core_, 7);
    const auto JobMemberLevel   = arg<nt::ULONG>(core_, 8);

    for(const auto& it : d_->observers_NtCreateProcessEx)
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
}

bool monitor::GenericMonitor::register_NtCreateProcess(proc_t proc, const on_NtCreateProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateProcess");
    if(!ok)
        FAIL(false, "Unable to register NtCreateProcess");

    d_->observers_NtCreateProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProcess()
{
    if(false)
        LOG(INFO, "break on NtCreateProcess");

    const auto ProcessHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ParentProcess      = arg<nt::HANDLE>(core_, 3);
    const auto InheritObjectTable = arg<nt::BOOLEAN>(core_, 4);
    const auto SectionHandle      = arg<nt::HANDLE>(core_, 5);
    const auto DebugPort          = arg<nt::HANDLE>(core_, 6);
    const auto ExceptionPort      = arg<nt::HANDLE>(core_, 7);

    for(const auto& it : d_->observers_NtCreateProcess)
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
}

bool monitor::GenericMonitor::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateProfileEx");
    if(!ok)
        FAIL(false, "Unable to register NtCreateProfileEx");

    d_->observers_NtCreateProfileEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProfileEx()
{
    if(false)
        LOG(INFO, "break on NtCreateProfileEx");

    const auto ProfileHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto Process            = arg<nt::HANDLE>(core_, 1);
    const auto ProfileBase        = arg<nt::PVOID>(core_, 2);
    const auto ProfileSize        = arg<nt::SIZE_T>(core_, 3);
    const auto BucketSize         = arg<nt::ULONG>(core_, 4);
    const auto Buffer             = arg<nt::PULONG>(core_, 5);
    const auto BufferSize         = arg<nt::ULONG>(core_, 6);
    const auto ProfileSource      = arg<nt::KPROFILE_SOURCE>(core_, 7);
    const auto GroupAffinityCount = arg<nt::ULONG>(core_, 8);
    const auto GroupAffinity      = arg<nt::PGROUP_AFFINITY>(core_, 9);

    for(const auto& it : d_->observers_NtCreateProfileEx)
        it(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
}

bool monitor::GenericMonitor::register_NtCreateProfile(proc_t proc, const on_NtCreateProfile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateProfile");
    if(!ok)
        FAIL(false, "Unable to register NtCreateProfile");

    d_->observers_NtCreateProfile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProfile()
{
    if(false)
        LOG(INFO, "break on NtCreateProfile");

    const auto ProfileHandle = arg<nt::PHANDLE>(core_, 0);
    const auto Process       = arg<nt::HANDLE>(core_, 1);
    const auto RangeBase     = arg<nt::PVOID>(core_, 2);
    const auto RangeSize     = arg<nt::SIZE_T>(core_, 3);
    const auto BucketSize    = arg<nt::ULONG>(core_, 4);
    const auto Buffer        = arg<nt::PULONG>(core_, 5);
    const auto BufferSize    = arg<nt::ULONG>(core_, 6);
    const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(core_, 7);
    const auto Affinity      = arg<nt::KAFFINITY>(core_, 8);

    for(const auto& it : d_->observers_NtCreateProfile)
        it(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
}

bool monitor::GenericMonitor::register_NtCreateResourceManager(proc_t proc, const on_NtCreateResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtCreateResourceManager");

    d_->observers_NtCreateResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateResourceManager()
{
    if(false)
        LOG(INFO, "break on NtCreateResourceManager");

    const auto ResourceManagerHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core_, 1);
    const auto TmHandle              = arg<nt::HANDLE>(core_, 2);
    const auto RmGuid                = arg<nt::LPGUID>(core_, 3);
    const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core_, 4);
    const auto CreateOptions         = arg<nt::ULONG>(core_, 5);
    const auto Description           = arg<nt::PUNICODE_STRING>(core_, 6);

    for(const auto& it : d_->observers_NtCreateResourceManager)
        it(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
}

bool monitor::GenericMonitor::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateSection");
    if(!ok)
        FAIL(false, "Unable to register NtCreateSection");

    d_->observers_NtCreateSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSection()
{
    if(false)
        LOG(INFO, "break on NtCreateSection");

    const auto SectionHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto MaximumSize           = arg<nt::PLARGE_INTEGER>(core_, 3);
    const auto SectionPageProtection = arg<nt::ULONG>(core_, 4);
    const auto AllocationAttributes  = arg<nt::ULONG>(core_, 5);
    const auto FileHandle            = arg<nt::HANDLE>(core_, 6);

    for(const auto& it : d_->observers_NtCreateSection)
        it(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
}

bool monitor::GenericMonitor::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateSemaphore");
    if(!ok)
        FAIL(false, "Unable to register NtCreateSemaphore");

    d_->observers_NtCreateSemaphore.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSemaphore()
{
    if(false)
        LOG(INFO, "break on NtCreateSemaphore");

    const auto SemaphoreHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto InitialCount     = arg<nt::LONG>(core_, 3);
    const auto MaximumCount     = arg<nt::LONG>(core_, 4);

    for(const auto& it : d_->observers_NtCreateSemaphore)
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
}

bool monitor::GenericMonitor::register_NtCreateSymbolicLinkObject(proc_t proc, const on_NtCreateSymbolicLinkObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateSymbolicLinkObject");
    if(!ok)
        FAIL(false, "Unable to register NtCreateSymbolicLinkObject");

    d_->observers_NtCreateSymbolicLinkObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSymbolicLinkObject()
{
    if(false)
        LOG(INFO, "break on NtCreateSymbolicLinkObject");

    const auto LinkHandle       = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto LinkTarget       = arg<nt::PUNICODE_STRING>(core_, 3);

    for(const auto& it : d_->observers_NtCreateSymbolicLinkObject)
        it(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
}

bool monitor::GenericMonitor::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateThreadEx");
    if(!ok)
        FAIL(false, "Unable to register NtCreateThreadEx");

    d_->observers_NtCreateThreadEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateThreadEx()
{
    if(false)
        LOG(INFO, "break on NtCreateThreadEx");

    const auto ThreadHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ProcessHandle    = arg<nt::HANDLE>(core_, 3);
    const auto StartRoutine     = arg<nt::PVOID>(core_, 4);
    const auto Argument         = arg<nt::PVOID>(core_, 5);
    const auto CreateFlags      = arg<nt::ULONG>(core_, 6);
    const auto ZeroBits         = arg<nt::ULONG_PTR>(core_, 7);
    const auto StackSize        = arg<nt::SIZE_T>(core_, 8);
    const auto MaximumStackSize = arg<nt::SIZE_T>(core_, 9);
    const auto AttributeList    = arg<nt::PPS_ATTRIBUTE_LIST>(core_, 10);

    for(const auto& it : d_->observers_NtCreateThreadEx)
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
}

bool monitor::GenericMonitor::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateThread");
    if(!ok)
        FAIL(false, "Unable to register NtCreateThread");

    d_->observers_NtCreateThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateThread()
{
    if(false)
        LOG(INFO, "break on NtCreateThread");

    const auto ThreadHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ProcessHandle    = arg<nt::HANDLE>(core_, 3);
    const auto ClientId         = arg<nt::PCLIENT_ID>(core_, 4);
    const auto ThreadContext    = arg<nt::PCONTEXT>(core_, 5);
    const auto InitialTeb       = arg<nt::PINITIAL_TEB>(core_, 6);
    const auto CreateSuspended  = arg<nt::BOOLEAN>(core_, 7);

    for(const auto& it : d_->observers_NtCreateThread)
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
}

bool monitor::GenericMonitor::register_NtCreateTimer(proc_t proc, const on_NtCreateTimer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateTimer");
    if(!ok)
        FAIL(false, "Unable to register NtCreateTimer");

    d_->observers_NtCreateTimer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTimer()
{
    if(false)
        LOG(INFO, "break on NtCreateTimer");

    const auto TimerHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto TimerType        = arg<nt::TIMER_TYPE>(core_, 3);

    for(const auto& it : d_->observers_NtCreateTimer)
        it(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
}

bool monitor::GenericMonitor::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateToken");
    if(!ok)
        FAIL(false, "Unable to register NtCreateToken");

    d_->observers_NtCreateToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateToken()
{
    if(false)
        LOG(INFO, "break on NtCreateToken");

    const auto TokenHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto TokenType        = arg<nt::TOKEN_TYPE>(core_, 3);
    const auto AuthenticationId = arg<nt::PLUID>(core_, 4);
    const auto ExpirationTime   = arg<nt::PLARGE_INTEGER>(core_, 5);
    const auto User             = arg<nt::PTOKEN_USER>(core_, 6);
    const auto Groups           = arg<nt::PTOKEN_GROUPS>(core_, 7);
    const auto Privileges       = arg<nt::PTOKEN_PRIVILEGES>(core_, 8);
    const auto Owner            = arg<nt::PTOKEN_OWNER>(core_, 9);
    const auto PrimaryGroup     = arg<nt::PTOKEN_PRIMARY_GROUP>(core_, 10);
    const auto DefaultDacl      = arg<nt::PTOKEN_DEFAULT_DACL>(core_, 11);
    const auto TokenSource      = arg<nt::PTOKEN_SOURCE>(core_, 12);

    for(const auto& it : d_->observers_NtCreateToken)
        it(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
}

bool monitor::GenericMonitor::register_NtCreateTransactionManager(proc_t proc, const on_NtCreateTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtCreateTransactionManager");

    d_->observers_NtCreateTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtCreateTransactionManager");

    const auto TmHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto LogFileName      = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto CreateOptions    = arg<nt::ULONG>(core_, 4);
    const auto CommitStrength   = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtCreateTransactionManager)
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
}

bool monitor::GenericMonitor::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtCreateTransaction");

    d_->observers_NtCreateTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTransaction()
{
    if(false)
        LOG(INFO, "break on NtCreateTransaction");

    const auto TransactionHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto Uow               = arg<nt::LPGUID>(core_, 3);
    const auto TmHandle          = arg<nt::HANDLE>(core_, 4);
    const auto CreateOptions     = arg<nt::ULONG>(core_, 5);
    const auto IsolationLevel    = arg<nt::ULONG>(core_, 6);
    const auto IsolationFlags    = arg<nt::ULONG>(core_, 7);
    const auto Timeout           = arg<nt::PLARGE_INTEGER>(core_, 8);
    const auto Description       = arg<nt::PUNICODE_STRING>(core_, 9);

    for(const auto& it : d_->observers_NtCreateTransaction)
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
}

bool monitor::GenericMonitor::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateUserProcess");
    if(!ok)
        FAIL(false, "Unable to register NtCreateUserProcess");

    d_->observers_NtCreateUserProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateUserProcess()
{
    if(false)
        LOG(INFO, "break on NtCreateUserProcess");

    const auto ProcessHandle           = arg<nt::PHANDLE>(core_, 0);
    const auto ThreadHandle            = arg<nt::PHANDLE>(core_, 1);
    const auto ProcessDesiredAccess    = arg<nt::ACCESS_MASK>(core_, 2);
    const auto ThreadDesiredAccess     = arg<nt::ACCESS_MASK>(core_, 3);
    const auto ProcessObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 4);
    const auto ThreadObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 5);
    const auto ProcessFlags            = arg<nt::ULONG>(core_, 6);
    const auto ThreadFlags             = arg<nt::ULONG>(core_, 7);
    const auto ProcessParameters       = arg<nt::PRTL_USER_PROCESS_PARAMETERS>(core_, 8);
    const auto CreateInfo              = arg<nt::PPROCESS_CREATE_INFO>(core_, 9);
    const auto AttributeList           = arg<nt::PPROCESS_ATTRIBUTE_LIST>(core_, 10);

    for(const auto& it : d_->observers_NtCreateUserProcess)
        it(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
}

bool monitor::GenericMonitor::register_NtCreateWaitablePort(proc_t proc, const on_NtCreateWaitablePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateWaitablePort");
    if(!ok)
        FAIL(false, "Unable to register NtCreateWaitablePort");

    d_->observers_NtCreateWaitablePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateWaitablePort()
{
    if(false)
        LOG(INFO, "break on NtCreateWaitablePort");

    const auto PortHandle              = arg<nt::PHANDLE>(core_, 0);
    const auto ObjectAttributes        = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto MaxConnectionInfoLength = arg<nt::ULONG>(core_, 2);
    const auto MaxMessageLength        = arg<nt::ULONG>(core_, 3);
    const auto MaxPoolUsage            = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtCreateWaitablePort)
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
}

bool monitor::GenericMonitor::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtCreateWorkerFactory");
    if(!ok)
        FAIL(false, "Unable to register NtCreateWorkerFactory");

    d_->observers_NtCreateWorkerFactory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtCreateWorkerFactory()
{
    if(false)
        LOG(INFO, "break on NtCreateWorkerFactory");

    const auto WorkerFactoryHandleReturn = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess             = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes          = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto CompletionPortHandle      = arg<nt::HANDLE>(core_, 3);
    const auto WorkerProcessHandle       = arg<nt::HANDLE>(core_, 4);
    const auto StartRoutine              = arg<nt::PVOID>(core_, 5);
    const auto StartParameter            = arg<nt::PVOID>(core_, 6);
    const auto MaxThreadCount            = arg<nt::ULONG>(core_, 7);
    const auto StackReserve              = arg<nt::SIZE_T>(core_, 8);
    const auto StackCommit               = arg<nt::SIZE_T>(core_, 9);

    for(const auto& it : d_->observers_NtCreateWorkerFactory)
        it(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
}

bool monitor::GenericMonitor::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDebugActiveProcess");
    if(!ok)
        FAIL(false, "Unable to register NtDebugActiveProcess");

    d_->observers_NtDebugActiveProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDebugActiveProcess()
{
    if(false)
        LOG(INFO, "break on NtDebugActiveProcess");

    const auto ProcessHandle     = arg<nt::HANDLE>(core_, 0);
    const auto DebugObjectHandle = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtDebugActiveProcess)
        it(ProcessHandle, DebugObjectHandle);
}

bool monitor::GenericMonitor::register_NtDebugContinue(proc_t proc, const on_NtDebugContinue_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDebugContinue");
    if(!ok)
        FAIL(false, "Unable to register NtDebugContinue");

    d_->observers_NtDebugContinue.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDebugContinue()
{
    if(false)
        LOG(INFO, "break on NtDebugContinue");

    const auto DebugObjectHandle = arg<nt::HANDLE>(core_, 0);
    const auto ClientId          = arg<nt::PCLIENT_ID>(core_, 1);
    const auto ContinueStatus    = arg<nt::NTSTATUS>(core_, 2);

    for(const auto& it : d_->observers_NtDebugContinue)
        it(DebugObjectHandle, ClientId, ContinueStatus);
}

bool monitor::GenericMonitor::register_NtDelayExecution(proc_t proc, const on_NtDelayExecution_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDelayExecution");
    if(!ok)
        FAIL(false, "Unable to register NtDelayExecution");

    d_->observers_NtDelayExecution.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDelayExecution()
{
    if(false)
        LOG(INFO, "break on NtDelayExecution");

    const auto Alertable     = arg<nt::BOOLEAN>(core_, 0);
    const auto DelayInterval = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtDelayExecution)
        it(Alertable, DelayInterval);
}

bool monitor::GenericMonitor::register_NtDeleteAtom(proc_t proc, const on_NtDeleteAtom_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteAtom");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteAtom");

    d_->observers_NtDeleteAtom.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteAtom()
{
    if(false)
        LOG(INFO, "break on NtDeleteAtom");

    const auto Atom = arg<nt::RTL_ATOM>(core_, 0);

    for(const auto& it : d_->observers_NtDeleteAtom)
        it(Atom);
}

bool monitor::GenericMonitor::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteBootEntry");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteBootEntry");

    d_->observers_NtDeleteBootEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteBootEntry()
{
    if(false)
        LOG(INFO, "break on NtDeleteBootEntry");

    const auto Id = arg<nt::ULONG>(core_, 0);

    for(const auto& it : d_->observers_NtDeleteBootEntry)
        it(Id);
}

bool monitor::GenericMonitor::register_NtDeleteDriverEntry(proc_t proc, const on_NtDeleteDriverEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteDriverEntry");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteDriverEntry");

    d_->observers_NtDeleteDriverEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteDriverEntry()
{
    if(false)
        LOG(INFO, "break on NtDeleteDriverEntry");

    const auto Id = arg<nt::ULONG>(core_, 0);

    for(const auto& it : d_->observers_NtDeleteDriverEntry)
        it(Id);
}

bool monitor::GenericMonitor::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteFile");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteFile");

    d_->observers_NtDeleteFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteFile()
{
    if(false)
        LOG(INFO, "break on NtDeleteFile");

    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);

    for(const auto& it : d_->observers_NtDeleteFile)
        it(ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtDeleteKey(proc_t proc, const on_NtDeleteKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteKey");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteKey");

    d_->observers_NtDeleteKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteKey()
{
    if(false)
        LOG(INFO, "break on NtDeleteKey");

    const auto KeyHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtDeleteKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteObjectAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteObjectAuditAlarm");

    d_->observers_NtDeleteObjectAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteObjectAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtDeleteObjectAuditAlarm");

    const auto SubsystemName   = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId        = arg<nt::PVOID>(core_, 1);
    const auto GenerateOnClose = arg<nt::BOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtDeleteObjectAuditAlarm)
        it(SubsystemName, HandleId, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeletePrivateNamespace");
    if(!ok)
        FAIL(false, "Unable to register NtDeletePrivateNamespace");

    d_->observers_NtDeletePrivateNamespace.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeletePrivateNamespace()
{
    if(false)
        LOG(INFO, "break on NtDeletePrivateNamespace");

    const auto NamespaceHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtDeletePrivateNamespace)
        it(NamespaceHandle);
}

bool monitor::GenericMonitor::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeleteValueKey");
    if(!ok)
        FAIL(false, "Unable to register NtDeleteValueKey");

    d_->observers_NtDeleteValueKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteValueKey()
{
    if(false)
        LOG(INFO, "break on NtDeleteValueKey");

    const auto KeyHandle = arg<nt::HANDLE>(core_, 0);
    const auto ValueName = arg<nt::PUNICODE_STRING>(core_, 1);

    for(const auto& it : d_->observers_NtDeleteValueKey)
        it(KeyHandle, ValueName);
}

bool monitor::GenericMonitor::register_NtDeviceIoControlFile(proc_t proc, const on_NtDeviceIoControlFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDeviceIoControlFile");
    if(!ok)
        FAIL(false, "Unable to register NtDeviceIoControlFile");

    d_->observers_NtDeviceIoControlFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDeviceIoControlFile()
{
    if(false)
        LOG(INFO, "break on NtDeviceIoControlFile");

    const auto FileHandle         = arg<nt::HANDLE>(core_, 0);
    const auto Event              = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext         = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto IoControlCode      = arg<nt::ULONG>(core_, 5);
    const auto InputBuffer        = arg<nt::PVOID>(core_, 6);
    const auto InputBufferLength  = arg<nt::ULONG>(core_, 7);
    const auto OutputBuffer       = arg<nt::PVOID>(core_, 8);
    const auto OutputBufferLength = arg<nt::ULONG>(core_, 9);

    for(const auto& it : d_->observers_NtDeviceIoControlFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDisplayString");
    if(!ok)
        FAIL(false, "Unable to register NtDisplayString");

    d_->observers_NtDisplayString.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDisplayString()
{
    if(false)
        LOG(INFO, "break on NtDisplayString");

    const auto String = arg<nt::PUNICODE_STRING>(core_, 0);

    for(const auto& it : d_->observers_NtDisplayString)
        it(String);
}

bool monitor::GenericMonitor::register_NtDrawText(proc_t proc, const on_NtDrawText_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDrawText");
    if(!ok)
        FAIL(false, "Unable to register NtDrawText");

    d_->observers_NtDrawText.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDrawText()
{
    if(false)
        LOG(INFO, "break on NtDrawText");

    const auto Text = arg<nt::PUNICODE_STRING>(core_, 0);

    for(const auto& it : d_->observers_NtDrawText)
        it(Text);
}

bool monitor::GenericMonitor::register_NtDuplicateObject(proc_t proc, const on_NtDuplicateObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDuplicateObject");
    if(!ok)
        FAIL(false, "Unable to register NtDuplicateObject");

    d_->observers_NtDuplicateObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDuplicateObject()
{
    if(false)
        LOG(INFO, "break on NtDuplicateObject");

    const auto SourceProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto SourceHandle        = arg<nt::HANDLE>(core_, 1);
    const auto TargetProcessHandle = arg<nt::HANDLE>(core_, 2);
    const auto TargetHandle        = arg<nt::PHANDLE>(core_, 3);
    const auto DesiredAccess       = arg<nt::ACCESS_MASK>(core_, 4);
    const auto HandleAttributes    = arg<nt::ULONG>(core_, 5);
    const auto Options             = arg<nt::ULONG>(core_, 6);

    for(const auto& it : d_->observers_NtDuplicateObject)
        it(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
}

bool monitor::GenericMonitor::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDuplicateToken");
    if(!ok)
        FAIL(false, "Unable to register NtDuplicateToken");

    d_->observers_NtDuplicateToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDuplicateToken()
{
    if(false)
        LOG(INFO, "break on NtDuplicateToken");

    const auto ExistingTokenHandle = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess       = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes    = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto EffectiveOnly       = arg<nt::BOOLEAN>(core_, 3);
    const auto TokenType           = arg<nt::TOKEN_TYPE>(core_, 4);
    const auto NewTokenHandle      = arg<nt::PHANDLE>(core_, 5);

    for(const auto& it : d_->observers_NtDuplicateToken)
        it(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
}

bool monitor::GenericMonitor::register_NtEnumerateBootEntries(proc_t proc, const on_NtEnumerateBootEntries_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateBootEntries");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateBootEntries");

    d_->observers_NtEnumerateBootEntries.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateBootEntries()
{
    if(false)
        LOG(INFO, "break on NtEnumerateBootEntries");

    const auto Buffer       = arg<nt::PVOID>(core_, 0);
    const auto BufferLength = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtEnumerateBootEntries)
        it(Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateDriverEntries");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateDriverEntries");

    d_->observers_NtEnumerateDriverEntries.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateDriverEntries()
{
    if(false)
        LOG(INFO, "break on NtEnumerateDriverEntries");

    const auto Buffer       = arg<nt::PVOID>(core_, 0);
    const auto BufferLength = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtEnumerateDriverEntries)
        it(Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateKey(proc_t proc, const on_NtEnumerateKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateKey");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateKey");

    d_->observers_NtEnumerateKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateKey()
{
    if(false)
        LOG(INFO, "break on NtEnumerateKey");

    const auto KeyHandle           = arg<nt::HANDLE>(core_, 0);
    const auto Index               = arg<nt::ULONG>(core_, 1);
    const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(core_, 2);
    const auto KeyInformation      = arg<nt::PVOID>(core_, 3);
    const auto Length              = arg<nt::ULONG>(core_, 4);
    const auto ResultLength        = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtEnumerateKey)
        it(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateSystemEnvironmentValuesEx");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateSystemEnvironmentValuesEx");

    d_->observers_NtEnumerateSystemEnvironmentValuesEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateSystemEnvironmentValuesEx()
{
    if(false)
        LOG(INFO, "break on NtEnumerateSystemEnvironmentValuesEx");

    const auto InformationClass = arg<nt::ULONG>(core_, 0);
    const auto Buffer           = arg<nt::PVOID>(core_, 1);
    const auto BufferLength     = arg<nt::PULONG>(core_, 2);

    for(const auto& it : d_->observers_NtEnumerateSystemEnvironmentValuesEx)
        it(InformationClass, Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateTransactionObject(proc_t proc, const on_NtEnumerateTransactionObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateTransactionObject");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateTransactionObject");

    d_->observers_NtEnumerateTransactionObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateTransactionObject()
{
    if(false)
        LOG(INFO, "break on NtEnumerateTransactionObject");

    const auto RootObjectHandle   = arg<nt::HANDLE>(core_, 0);
    const auto QueryType          = arg<nt::KTMOBJECT_TYPE>(core_, 1);
    const auto ObjectCursor       = arg<nt::PKTMOBJECT_CURSOR>(core_, 2);
    const auto ObjectCursorLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength       = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtEnumerateTransactionObject)
        it(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnumerateValueKey");
    if(!ok)
        FAIL(false, "Unable to register NtEnumerateValueKey");

    d_->observers_NtEnumerateValueKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateValueKey()
{
    if(false)
        LOG(INFO, "break on NtEnumerateValueKey");

    const auto KeyHandle                = arg<nt::HANDLE>(core_, 0);
    const auto Index                    = arg<nt::ULONG>(core_, 1);
    const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(core_, 2);
    const auto KeyValueInformation      = arg<nt::PVOID>(core_, 3);
    const auto Length                   = arg<nt::ULONG>(core_, 4);
    const auto ResultLength             = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtEnumerateValueKey)
        it(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtExtendSection(proc_t proc, const on_NtExtendSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtExtendSection");
    if(!ok)
        FAIL(false, "Unable to register NtExtendSection");

    d_->observers_NtExtendSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtExtendSection()
{
    if(false)
        LOG(INFO, "break on NtExtendSection");

    const auto SectionHandle  = arg<nt::HANDLE>(core_, 0);
    const auto NewSectionSize = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtExtendSection)
        it(SectionHandle, NewSectionSize);
}

bool monitor::GenericMonitor::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFilterToken");
    if(!ok)
        FAIL(false, "Unable to register NtFilterToken");

    d_->observers_NtFilterToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFilterToken()
{
    if(false)
        LOG(INFO, "break on NtFilterToken");

    const auto ExistingTokenHandle = arg<nt::HANDLE>(core_, 0);
    const auto Flags               = arg<nt::ULONG>(core_, 1);
    const auto SidsToDisable       = arg<nt::PTOKEN_GROUPS>(core_, 2);
    const auto PrivilegesToDelete  = arg<nt::PTOKEN_PRIVILEGES>(core_, 3);
    const auto RestrictedSids      = arg<nt::PTOKEN_GROUPS>(core_, 4);
    const auto NewTokenHandle      = arg<nt::PHANDLE>(core_, 5);

    for(const auto& it : d_->observers_NtFilterToken)
        it(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
}

bool monitor::GenericMonitor::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFindAtom");
    if(!ok)
        FAIL(false, "Unable to register NtFindAtom");

    d_->observers_NtFindAtom.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFindAtom()
{
    if(false)
        LOG(INFO, "break on NtFindAtom");

    const auto AtomName = arg<nt::PWSTR>(core_, 0);
    const auto Length   = arg<nt::ULONG>(core_, 1);
    const auto Atom     = arg<nt::PRTL_ATOM>(core_, 2);

    for(const auto& it : d_->observers_NtFindAtom)
        it(AtomName, Length, Atom);
}

bool monitor::GenericMonitor::register_NtFlushBuffersFile(proc_t proc, const on_NtFlushBuffersFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushBuffersFile");
    if(!ok)
        FAIL(false, "Unable to register NtFlushBuffersFile");

    d_->observers_NtFlushBuffersFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushBuffersFile()
{
    if(false)
        LOG(INFO, "break on NtFlushBuffersFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 1);

    for(const auto& it : d_->observers_NtFlushBuffersFile)
        it(FileHandle, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtFlushInstallUILanguage(proc_t proc, const on_NtFlushInstallUILanguage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushInstallUILanguage");
    if(!ok)
        FAIL(false, "Unable to register NtFlushInstallUILanguage");

    d_->observers_NtFlushInstallUILanguage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushInstallUILanguage()
{
    if(false)
        LOG(INFO, "break on NtFlushInstallUILanguage");

    const auto InstallUILanguage = arg<nt::LANGID>(core_, 0);
    const auto SetComittedFlag   = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtFlushInstallUILanguage)
        it(InstallUILanguage, SetComittedFlag);
}

bool monitor::GenericMonitor::register_NtFlushInstructionCache(proc_t proc, const on_NtFlushInstructionCache_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushInstructionCache");
    if(!ok)
        FAIL(false, "Unable to register NtFlushInstructionCache");

    d_->observers_NtFlushInstructionCache.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushInstructionCache()
{
    if(false)
        LOG(INFO, "break on NtFlushInstructionCache");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress   = arg<nt::PVOID>(core_, 1);
    const auto Length        = arg<nt::SIZE_T>(core_, 2);

    for(const auto& it : d_->observers_NtFlushInstructionCache)
        it(ProcessHandle, BaseAddress, Length);
}

bool monitor::GenericMonitor::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushKey");
    if(!ok)
        FAIL(false, "Unable to register NtFlushKey");

    d_->observers_NtFlushKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushKey()
{
    if(false)
        LOG(INFO, "break on NtFlushKey");

    const auto KeyHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtFlushKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtFlushVirtualMemory(proc_t proc, const on_NtFlushVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtFlushVirtualMemory");

    d_->observers_NtFlushVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtFlushVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 2);
    const auto IoStatus        = arg<nt::PIO_STATUS_BLOCK>(core_, 3);

    for(const auto& it : d_->observers_NtFlushVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
}

bool monitor::GenericMonitor::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFreeUserPhysicalPages");
    if(!ok)
        FAIL(false, "Unable to register NtFreeUserPhysicalPages");

    d_->observers_NtFreeUserPhysicalPages.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFreeUserPhysicalPages()
{
    if(false)
        LOG(INFO, "break on NtFreeUserPhysicalPages");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto NumberOfPages = arg<nt::PULONG_PTR>(core_, 1);
    const auto UserPfnArra   = arg<nt::PULONG_PTR>(core_, 2);

    for(const auto& it : d_->observers_NtFreeUserPhysicalPages)
        it(ProcessHandle, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFreeVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtFreeVirtualMemory");

    d_->observers_NtFreeVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFreeVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtFreeVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 2);
    const auto FreeType        = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtFreeVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
}

bool monitor::GenericMonitor::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFreezeRegistry");
    if(!ok)
        FAIL(false, "Unable to register NtFreezeRegistry");

    d_->observers_NtFreezeRegistry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFreezeRegistry()
{
    if(false)
        LOG(INFO, "break on NtFreezeRegistry");

    const auto TimeOutInSeconds = arg<nt::ULONG>(core_, 0);

    for(const auto& it : d_->observers_NtFreezeRegistry)
        it(TimeOutInSeconds);
}

bool monitor::GenericMonitor::register_NtFreezeTransactions(proc_t proc, const on_NtFreezeTransactions_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFreezeTransactions");
    if(!ok)
        FAIL(false, "Unable to register NtFreezeTransactions");

    d_->observers_NtFreezeTransactions.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFreezeTransactions()
{
    if(false)
        LOG(INFO, "break on NtFreezeTransactions");

    const auto FreezeTimeout = arg<nt::PLARGE_INTEGER>(core_, 0);
    const auto ThawTimeout   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtFreezeTransactions)
        it(FreezeTimeout, ThawTimeout);
}

bool monitor::GenericMonitor::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFsControlFile");
    if(!ok)
        FAIL(false, "Unable to register NtFsControlFile");

    d_->observers_NtFsControlFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFsControlFile()
{
    if(false)
        LOG(INFO, "break on NtFsControlFile");

    const auto FileHandle         = arg<nt::HANDLE>(core_, 0);
    const auto Event              = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine         = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext         = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto IoControlCode      = arg<nt::ULONG>(core_, 5);
    const auto InputBuffer        = arg<nt::PVOID>(core_, 6);
    const auto InputBufferLength  = arg<nt::ULONG>(core_, 7);
    const auto OutputBuffer       = arg<nt::PVOID>(core_, 8);
    const auto OutputBufferLength = arg<nt::ULONG>(core_, 9);

    for(const auto& it : d_->observers_NtFsControlFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetContextThread");
    if(!ok)
        FAIL(false, "Unable to register NtGetContextThread");

    d_->observers_NtGetContextThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetContextThread()
{
    if(false)
        LOG(INFO, "break on NtGetContextThread");

    const auto ThreadHandle  = arg<nt::HANDLE>(core_, 0);
    const auto ThreadContext = arg<nt::PCONTEXT>(core_, 1);

    for(const auto& it : d_->observers_NtGetContextThread)
        it(ThreadHandle, ThreadContext);
}

bool monitor::GenericMonitor::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetDevicePowerState");
    if(!ok)
        FAIL(false, "Unable to register NtGetDevicePowerState");

    d_->observers_NtGetDevicePowerState.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetDevicePowerState()
{
    if(false)
        LOG(INFO, "break on NtGetDevicePowerState");

    const auto Device    = arg<nt::HANDLE>(core_, 0);
    const auto STARState = arg<nt::DEVICE_POWER_STATE>(core_, 1);

    for(const auto& it : d_->observers_NtGetDevicePowerState)
        it(Device, STARState);
}

bool monitor::GenericMonitor::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetMUIRegistryInfo");
    if(!ok)
        FAIL(false, "Unable to register NtGetMUIRegistryInfo");

    d_->observers_NtGetMUIRegistryInfo.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetMUIRegistryInfo()
{
    if(false)
        LOG(INFO, "break on NtGetMUIRegistryInfo");

    const auto Flags    = arg<nt::ULONG>(core_, 0);
    const auto DataSize = arg<nt::PULONG>(core_, 1);
    const auto Data     = arg<nt::PVOID>(core_, 2);

    for(const auto& it : d_->observers_NtGetMUIRegistryInfo)
        it(Flags, DataSize, Data);
}

bool monitor::GenericMonitor::register_NtGetNextProcess(proc_t proc, const on_NtGetNextProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetNextProcess");
    if(!ok)
        FAIL(false, "Unable to register NtGetNextProcess");

    d_->observers_NtGetNextProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetNextProcess()
{
    if(false)
        LOG(INFO, "break on NtGetNextProcess");

    const auto ProcessHandle    = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto HandleAttributes = arg<nt::ULONG>(core_, 2);
    const auto Flags            = arg<nt::ULONG>(core_, 3);
    const auto NewProcessHandle = arg<nt::PHANDLE>(core_, 4);

    for(const auto& it : d_->observers_NtGetNextProcess)
        it(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
}

bool monitor::GenericMonitor::register_NtGetNextThread(proc_t proc, const on_NtGetNextThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetNextThread");
    if(!ok)
        FAIL(false, "Unable to register NtGetNextThread");

    d_->observers_NtGetNextThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetNextThread()
{
    if(false)
        LOG(INFO, "break on NtGetNextThread");

    const auto ProcessHandle    = arg<nt::HANDLE>(core_, 0);
    const auto ThreadHandle     = arg<nt::HANDLE>(core_, 1);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 2);
    const auto HandleAttributes = arg<nt::ULONG>(core_, 3);
    const auto Flags            = arg<nt::ULONG>(core_, 4);
    const auto NewThreadHandle  = arg<nt::PHANDLE>(core_, 5);

    for(const auto& it : d_->observers_NtGetNextThread)
        it(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
}

bool monitor::GenericMonitor::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetNlsSectionPtr");
    if(!ok)
        FAIL(false, "Unable to register NtGetNlsSectionPtr");

    d_->observers_NtGetNlsSectionPtr.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetNlsSectionPtr()
{
    if(false)
        LOG(INFO, "break on NtGetNlsSectionPtr");

    const auto SectionType        = arg<nt::ULONG>(core_, 0);
    const auto SectionData        = arg<nt::ULONG>(core_, 1);
    const auto ContextData        = arg<nt::PVOID>(core_, 2);
    const auto STARSectionPointer = arg<nt::PVOID>(core_, 3);
    const auto SectionSize        = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtGetNlsSectionPtr)
        it(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
}

bool monitor::GenericMonitor::register_NtGetNotificationResourceManager(proc_t proc, const on_NtGetNotificationResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetNotificationResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtGetNotificationResourceManager");

    d_->observers_NtGetNotificationResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetNotificationResourceManager()
{
    if(false)
        LOG(INFO, "break on NtGetNotificationResourceManager");

    const auto ResourceManagerHandle   = arg<nt::HANDLE>(core_, 0);
    const auto TransactionNotification = arg<nt::PTRANSACTION_NOTIFICATION>(core_, 1);
    const auto NotificationLength      = arg<nt::ULONG>(core_, 2);
    const auto Timeout                 = arg<nt::PLARGE_INTEGER>(core_, 3);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 4);
    const auto Asynchronous            = arg<nt::ULONG>(core_, 5);
    const auto AsynchronousContext     = arg<nt::ULONG_PTR>(core_, 6);

    for(const auto& it : d_->observers_NtGetNotificationResourceManager)
        it(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
}

bool monitor::GenericMonitor::register_NtGetPlugPlayEvent(proc_t proc, const on_NtGetPlugPlayEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetPlugPlayEvent");
    if(!ok)
        FAIL(false, "Unable to register NtGetPlugPlayEvent");

    d_->observers_NtGetPlugPlayEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetPlugPlayEvent()
{
    if(false)
        LOG(INFO, "break on NtGetPlugPlayEvent");

    const auto EventHandle     = arg<nt::HANDLE>(core_, 0);
    const auto Context         = arg<nt::PVOID>(core_, 1);
    const auto EventBlock      = arg<nt::PPLUGPLAY_EVENT_BLOCK>(core_, 2);
    const auto EventBufferSize = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtGetPlugPlayEvent)
        it(EventHandle, Context, EventBlock, EventBufferSize);
}

bool monitor::GenericMonitor::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetWriteWatch");
    if(!ok)
        FAIL(false, "Unable to register NtGetWriteWatch");

    d_->observers_NtGetWriteWatch.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetWriteWatch()
{
    if(false)
        LOG(INFO, "break on NtGetWriteWatch");

    const auto ProcessHandle             = arg<nt::HANDLE>(core_, 0);
    const auto Flags                     = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress               = arg<nt::PVOID>(core_, 2);
    const auto RegionSize                = arg<nt::SIZE_T>(core_, 3);
    const auto STARUserAddressArray      = arg<nt::PVOID>(core_, 4);
    const auto EntriesInUserAddressArray = arg<nt::PULONG_PTR>(core_, 5);
    const auto Granularity               = arg<nt::PULONG>(core_, 6);

    for(const auto& it : d_->observers_NtGetWriteWatch)
        it(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
}

bool monitor::GenericMonitor::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtImpersonateAnonymousToken");
    if(!ok)
        FAIL(false, "Unable to register NtImpersonateAnonymousToken");

    d_->observers_NtImpersonateAnonymousToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateAnonymousToken()
{
    if(false)
        LOG(INFO, "break on NtImpersonateAnonymousToken");

    const auto ThreadHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtImpersonateAnonymousToken)
        it(ThreadHandle);
}

bool monitor::GenericMonitor::register_NtImpersonateClientOfPort(proc_t proc, const on_NtImpersonateClientOfPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtImpersonateClientOfPort");
    if(!ok)
        FAIL(false, "Unable to register NtImpersonateClientOfPort");

    d_->observers_NtImpersonateClientOfPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateClientOfPort()
{
    if(false)
        LOG(INFO, "break on NtImpersonateClientOfPort");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);
    const auto Message    = arg<nt::PPORT_MESSAGE>(core_, 1);

    for(const auto& it : d_->observers_NtImpersonateClientOfPort)
        it(PortHandle, Message);
}

bool monitor::GenericMonitor::register_NtImpersonateThread(proc_t proc, const on_NtImpersonateThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtImpersonateThread");
    if(!ok)
        FAIL(false, "Unable to register NtImpersonateThread");

    d_->observers_NtImpersonateThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateThread()
{
    if(false)
        LOG(INFO, "break on NtImpersonateThread");

    const auto ServerThreadHandle = arg<nt::HANDLE>(core_, 0);
    const auto ClientThreadHandle = arg<nt::HANDLE>(core_, 1);
    const auto SecurityQos        = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core_, 2);

    for(const auto& it : d_->observers_NtImpersonateThread)
        it(ServerThreadHandle, ClientThreadHandle, SecurityQos);
}

bool monitor::GenericMonitor::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_func)
{
    const auto ok = setup_func(proc, "NtInitializeNlsFiles");
    if(!ok)
        FAIL(false, "Unable to register NtInitializeNlsFiles");

    d_->observers_NtInitializeNlsFiles.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtInitializeNlsFiles()
{
    if(false)
        LOG(INFO, "break on NtInitializeNlsFiles");

    const auto STARBaseAddress        = arg<nt::PVOID>(core_, 0);
    const auto DefaultLocaleId        = arg<nt::PLCID>(core_, 1);
    const auto DefaultCasingTableSize = arg<nt::PLARGE_INTEGER>(core_, 2);

    for(const auto& it : d_->observers_NtInitializeNlsFiles)
        it(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
}

bool monitor::GenericMonitor::register_NtInitializeRegistry(proc_t proc, const on_NtInitializeRegistry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtInitializeRegistry");
    if(!ok)
        FAIL(false, "Unable to register NtInitializeRegistry");

    d_->observers_NtInitializeRegistry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtInitializeRegistry()
{
    if(false)
        LOG(INFO, "break on NtInitializeRegistry");

    const auto BootCondition = arg<nt::USHORT>(core_, 0);

    for(const auto& it : d_->observers_NtInitializeRegistry)
        it(BootCondition);
}

bool monitor::GenericMonitor::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtInitiatePowerAction");
    if(!ok)
        FAIL(false, "Unable to register NtInitiatePowerAction");

    d_->observers_NtInitiatePowerAction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtInitiatePowerAction()
{
    if(false)
        LOG(INFO, "break on NtInitiatePowerAction");

    const auto SystemAction   = arg<nt::POWER_ACTION>(core_, 0);
    const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(core_, 1);
    const auto Flags          = arg<nt::ULONG>(core_, 2);
    const auto Asynchronous   = arg<nt::BOOLEAN>(core_, 3);

    for(const auto& it : d_->observers_NtInitiatePowerAction)
        it(SystemAction, MinSystemState, Flags, Asynchronous);
}

bool monitor::GenericMonitor::register_NtIsProcessInJob(proc_t proc, const on_NtIsProcessInJob_fn& on_func)
{
    const auto ok = setup_func(proc, "NtIsProcessInJob");
    if(!ok)
        FAIL(false, "Unable to register NtIsProcessInJob");

    d_->observers_NtIsProcessInJob.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtIsProcessInJob()
{
    if(false)
        LOG(INFO, "break on NtIsProcessInJob");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto JobHandle     = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtIsProcessInJob)
        it(ProcessHandle, JobHandle);
}

bool monitor::GenericMonitor::register_NtListenPort(proc_t proc, const on_NtListenPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtListenPort");
    if(!ok)
        FAIL(false, "Unable to register NtListenPort");

    d_->observers_NtListenPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtListenPort()
{
    if(false)
        LOG(INFO, "break on NtListenPort");

    const auto PortHandle        = arg<nt::HANDLE>(core_, 0);
    const auto ConnectionRequest = arg<nt::PPORT_MESSAGE>(core_, 1);

    for(const auto& it : d_->observers_NtListenPort)
        it(PortHandle, ConnectionRequest);
}

bool monitor::GenericMonitor::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLoadDriver");
    if(!ok)
        FAIL(false, "Unable to register NtLoadDriver");

    d_->observers_NtLoadDriver.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLoadDriver()
{
    if(false)
        LOG(INFO, "break on NtLoadDriver");

    const auto DriverServiceName = arg<nt::PUNICODE_STRING>(core_, 0);

    for(const auto& it : d_->observers_NtLoadDriver)
        it(DriverServiceName);
}

bool monitor::GenericMonitor::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLoadKey2");
    if(!ok)
        FAIL(false, "Unable to register NtLoadKey2");

    d_->observers_NtLoadKey2.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKey2()
{
    if(false)
        LOG(INFO, "break on NtLoadKey2");

    const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto Flags      = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtLoadKey2)
        it(TargetKey, SourceFile, Flags);
}

bool monitor::GenericMonitor::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLoadKeyEx");
    if(!ok)
        FAIL(false, "Unable to register NtLoadKeyEx");

    d_->observers_NtLoadKeyEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKeyEx()
{
    if(false)
        LOG(INFO, "break on NtLoadKeyEx");

    const auto TargetKey     = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto SourceFile    = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);
    const auto Flags         = arg<nt::ULONG>(core_, 2);
    const auto TrustClassKey = arg<nt::HANDLE>(core_, 3);

    for(const auto& it : d_->observers_NtLoadKeyEx)
        it(TargetKey, SourceFile, Flags, TrustClassKey);
}

bool monitor::GenericMonitor::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLoadKey");
    if(!ok)
        FAIL(false, "Unable to register NtLoadKey");

    d_->observers_NtLoadKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKey()
{
    if(false)
        LOG(INFO, "break on NtLoadKey");

    const auto TargetKey  = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto SourceFile = arg<nt::POBJECT_ATTRIBUTES>(core_, 1);

    for(const auto& it : d_->observers_NtLoadKey)
        it(TargetKey, SourceFile);
}

bool monitor::GenericMonitor::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLockFile");
    if(!ok)
        FAIL(false, "Unable to register NtLockFile");

    d_->observers_NtLockFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLockFile()
{
    if(false)
        LOG(INFO, "break on NtLockFile");

    const auto FileHandle      = arg<nt::HANDLE>(core_, 0);
    const auto Event           = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine      = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext      = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock   = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto ByteOffset      = arg<nt::PLARGE_INTEGER>(core_, 5);
    const auto Length          = arg<nt::PLARGE_INTEGER>(core_, 6);
    const auto Key             = arg<nt::ULONG>(core_, 7);
    const auto FailImmediately = arg<nt::BOOLEAN>(core_, 8);
    const auto ExclusiveLock   = arg<nt::BOOLEAN>(core_, 9);

    for(const auto& it : d_->observers_NtLockFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
}

bool monitor::GenericMonitor::register_NtLockProductActivationKeys(proc_t proc, const on_NtLockProductActivationKeys_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLockProductActivationKeys");
    if(!ok)
        FAIL(false, "Unable to register NtLockProductActivationKeys");

    d_->observers_NtLockProductActivationKeys.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLockProductActivationKeys()
{
    if(false)
        LOG(INFO, "break on NtLockProductActivationKeys");

    const auto STARpPrivateVer = arg<nt::ULONG>(core_, 0);
    const auto STARpSafeMode   = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtLockProductActivationKeys)
        it(STARpPrivateVer, STARpSafeMode);
}

bool monitor::GenericMonitor::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLockRegistryKey");
    if(!ok)
        FAIL(false, "Unable to register NtLockRegistryKey");

    d_->observers_NtLockRegistryKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLockRegistryKey()
{
    if(false)
        LOG(INFO, "break on NtLockRegistryKey");

    const auto KeyHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtLockRegistryKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtLockVirtualMemory(proc_t proc, const on_NtLockVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtLockVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtLockVirtualMemory");

    d_->observers_NtLockVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtLockVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtLockVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 2);
    const auto MapType         = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtLockVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
}

bool monitor::GenericMonitor::register_NtMakePermanentObject(proc_t proc, const on_NtMakePermanentObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMakePermanentObject");
    if(!ok)
        FAIL(false, "Unable to register NtMakePermanentObject");

    d_->observers_NtMakePermanentObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMakePermanentObject()
{
    if(false)
        LOG(INFO, "break on NtMakePermanentObject");

    const auto Handle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtMakePermanentObject)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMakeTemporaryObject");
    if(!ok)
        FAIL(false, "Unable to register NtMakeTemporaryObject");

    d_->observers_NtMakeTemporaryObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMakeTemporaryObject()
{
    if(false)
        LOG(INFO, "break on NtMakeTemporaryObject");

    const auto Handle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtMakeTemporaryObject)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtMapCMFModule(proc_t proc, const on_NtMapCMFModule_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMapCMFModule");
    if(!ok)
        FAIL(false, "Unable to register NtMapCMFModule");

    d_->observers_NtMapCMFModule.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMapCMFModule()
{
    if(false)
        LOG(INFO, "break on NtMapCMFModule");

    const auto What            = arg<nt::ULONG>(core_, 0);
    const auto Index           = arg<nt::ULONG>(core_, 1);
    const auto CacheIndexOut   = arg<nt::PULONG>(core_, 2);
    const auto CacheFlagsOut   = arg<nt::PULONG>(core_, 3);
    const auto ViewSizeOut     = arg<nt::PULONG>(core_, 4);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 5);

    for(const auto& it : d_->observers_NtMapCMFModule)
        it(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
}

bool monitor::GenericMonitor::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMapUserPhysicalPages");
    if(!ok)
        FAIL(false, "Unable to register NtMapUserPhysicalPages");

    d_->observers_NtMapUserPhysicalPages.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPages()
{
    if(false)
        LOG(INFO, "break on NtMapUserPhysicalPages");

    const auto VirtualAddress = arg<nt::PVOID>(core_, 0);
    const auto NumberOfPages  = arg<nt::ULONG_PTR>(core_, 1);
    const auto UserPfnArra    = arg<nt::PULONG_PTR>(core_, 2);

    for(const auto& it : d_->observers_NtMapUserPhysicalPages)
        it(VirtualAddress, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtMapUserPhysicalPagesScatter(proc_t proc, const on_NtMapUserPhysicalPagesScatter_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMapUserPhysicalPagesScatter");
    if(!ok)
        FAIL(false, "Unable to register NtMapUserPhysicalPagesScatter");

    d_->observers_NtMapUserPhysicalPagesScatter.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPagesScatter()
{
    if(false)
        LOG(INFO, "break on NtMapUserPhysicalPagesScatter");

    const auto STARVirtualAddresses = arg<nt::PVOID>(core_, 0);
    const auto NumberOfPages        = arg<nt::ULONG_PTR>(core_, 1);
    const auto UserPfnArray         = arg<nt::PULONG_PTR>(core_, 2);

    for(const auto& it : d_->observers_NtMapUserPhysicalPagesScatter)
        it(STARVirtualAddresses, NumberOfPages, UserPfnArray);
}

bool monitor::GenericMonitor::register_NtMapViewOfSection(proc_t proc, const on_NtMapViewOfSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtMapViewOfSection");
    if(!ok)
        FAIL(false, "Unable to register NtMapViewOfSection");

    d_->observers_NtMapViewOfSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtMapViewOfSection()
{
    if(false)
        LOG(INFO, "break on NtMapViewOfSection");

    const auto SectionHandle      = arg<nt::HANDLE>(core_, 0);
    const auto ProcessHandle      = arg<nt::HANDLE>(core_, 1);
    const auto STARBaseAddress    = arg<nt::PVOID>(core_, 2);
    const auto ZeroBits           = arg<nt::ULONG_PTR>(core_, 3);
    const auto CommitSize         = arg<nt::SIZE_T>(core_, 4);
    const auto SectionOffset      = arg<nt::PLARGE_INTEGER>(core_, 5);
    const auto ViewSize           = arg<nt::PSIZE_T>(core_, 6);
    const auto InheritDisposition = arg<nt::SECTION_INHERIT>(core_, 7);
    const auto AllocationType     = arg<nt::ULONG>(core_, 8);
    const auto Win32Protect       = arg<nt::WIN32_PROTECTION_MASK>(core_, 9);

    for(const auto& it : d_->observers_NtMapViewOfSection)
        it(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

bool monitor::GenericMonitor::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtModifyBootEntry");
    if(!ok)
        FAIL(false, "Unable to register NtModifyBootEntry");

    d_->observers_NtModifyBootEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtModifyBootEntry()
{
    if(false)
        LOG(INFO, "break on NtModifyBootEntry");

    const auto BootEntry = arg<nt::PBOOT_ENTRY>(core_, 0);

    for(const auto& it : d_->observers_NtModifyBootEntry)
        it(BootEntry);
}

bool monitor::GenericMonitor::register_NtModifyDriverEntry(proc_t proc, const on_NtModifyDriverEntry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtModifyDriverEntry");
    if(!ok)
        FAIL(false, "Unable to register NtModifyDriverEntry");

    d_->observers_NtModifyDriverEntry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtModifyDriverEntry()
{
    if(false)
        LOG(INFO, "break on NtModifyDriverEntry");

    const auto DriverEntry = arg<nt::PEFI_DRIVER_ENTRY>(core_, 0);

    for(const auto& it : d_->observers_NtModifyDriverEntry)
        it(DriverEntry);
}

bool monitor::GenericMonitor::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtNotifyChangeDirectoryFile");
    if(!ok)
        FAIL(false, "Unable to register NtNotifyChangeDirectoryFile");

    d_->observers_NtNotifyChangeDirectoryFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeDirectoryFile()
{
    if(false)
        LOG(INFO, "break on NtNotifyChangeDirectoryFile");

    const auto FileHandle       = arg<nt::HANDLE>(core_, 0);
    const auto Event            = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext       = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto Buffer           = arg<nt::PVOID>(core_, 5);
    const auto Length           = arg<nt::ULONG>(core_, 6);
    const auto CompletionFilter = arg<nt::ULONG>(core_, 7);
    const auto WatchTree        = arg<nt::BOOLEAN>(core_, 8);

    for(const auto& it : d_->observers_NtNotifyChangeDirectoryFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
}

bool monitor::GenericMonitor::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtNotifyChangeKey");
    if(!ok)
        FAIL(false, "Unable to register NtNotifyChangeKey");

    d_->observers_NtNotifyChangeKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeKey()
{
    if(false)
        LOG(INFO, "break on NtNotifyChangeKey");

    const auto KeyHandle        = arg<nt::HANDLE>(core_, 0);
    const auto Event            = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext       = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto CompletionFilter = arg<nt::ULONG>(core_, 5);
    const auto WatchTree        = arg<nt::BOOLEAN>(core_, 6);
    const auto Buffer           = arg<nt::PVOID>(core_, 7);
    const auto BufferSize       = arg<nt::ULONG>(core_, 8);
    const auto Asynchronous     = arg<nt::BOOLEAN>(core_, 9);

    for(const auto& it : d_->observers_NtNotifyChangeKey)
        it(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

bool monitor::GenericMonitor::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_func)
{
    const auto ok = setup_func(proc, "NtNotifyChangeMultipleKeys");
    if(!ok)
        FAIL(false, "Unable to register NtNotifyChangeMultipleKeys");

    d_->observers_NtNotifyChangeMultipleKeys.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeMultipleKeys()
{
    if(false)
        LOG(INFO, "break on NtNotifyChangeMultipleKeys");

    const auto MasterKeyHandle  = arg<nt::HANDLE>(core_, 0);
    const auto Count            = arg<nt::ULONG>(core_, 1);
    const auto SlaveObjects     = arg<nt::OBJECT_ATTRIBUTES>(core_, 2);
    const auto Event            = arg<nt::HANDLE>(core_, 3);
    const auto ApcRoutine       = arg<nt::PIO_APC_ROUTINE>(core_, 4);
    const auto ApcContext       = arg<nt::PVOID>(core_, 5);
    const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core_, 6);
    const auto CompletionFilter = arg<nt::ULONG>(core_, 7);
    const auto WatchTree        = arg<nt::BOOLEAN>(core_, 8);
    const auto Buffer           = arg<nt::PVOID>(core_, 9);
    const auto BufferSize       = arg<nt::ULONG>(core_, 10);
    const auto Asynchronous     = arg<nt::BOOLEAN>(core_, 11);

    for(const auto& it : d_->observers_NtNotifyChangeMultipleKeys)
        it(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

bool monitor::GenericMonitor::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_func)
{
    const auto ok = setup_func(proc, "NtNotifyChangeSession");
    if(!ok)
        FAIL(false, "Unable to register NtNotifyChangeSession");

    d_->observers_NtNotifyChangeSession.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeSession()
{
    if(false)
        LOG(INFO, "break on NtNotifyChangeSession");

    const auto Session         = arg<nt::HANDLE>(core_, 0);
    const auto IoStateSequence = arg<nt::ULONG>(core_, 1);
    const auto Reserved        = arg<nt::PVOID>(core_, 2);
    const auto Action          = arg<nt::ULONG>(core_, 3);
    const auto IoState         = arg<nt::IO_SESSION_STATE>(core_, 4);
    const auto IoState2        = arg<nt::IO_SESSION_STATE>(core_, 5);
    const auto Buffer          = arg<nt::PVOID>(core_, 6);
    const auto BufferSize      = arg<nt::ULONG>(core_, 7);

    for(const auto& it : d_->observers_NtNotifyChangeSession)
        it(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
}

bool monitor::GenericMonitor::register_NtOpenDirectoryObject(proc_t proc, const on_NtOpenDirectoryObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenDirectoryObject");
    if(!ok)
        FAIL(false, "Unable to register NtOpenDirectoryObject");

    d_->observers_NtOpenDirectoryObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenDirectoryObject()
{
    if(false)
        LOG(INFO, "break on NtOpenDirectoryObject");

    const auto DirectoryHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenDirectoryObject)
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEnlistment(proc_t proc, const on_NtOpenEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtOpenEnlistment");

    d_->observers_NtOpenEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEnlistment()
{
    if(false)
        LOG(INFO, "break on NtOpenEnlistment");

    const auto EnlistmentHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ResourceManagerHandle = arg<nt::HANDLE>(core_, 2);
    const auto EnlistmentGuid        = arg<nt::LPGUID>(core_, 3);
    const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core_, 4);

    for(const auto& it : d_->observers_NtOpenEnlistment)
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenEvent");
    if(!ok)
        FAIL(false, "Unable to register NtOpenEvent");

    d_->observers_NtOpenEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEvent()
{
    if(false)
        LOG(INFO, "break on NtOpenEvent");

    const auto EventHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenEvent)
        it(EventHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtOpenEventPair");

    d_->observers_NtOpenEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEventPair()
{
    if(false)
        LOG(INFO, "break on NtOpenEventPair");

    const auto EventPairHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenEventPair)
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenFile");
    if(!ok)
        FAIL(false, "Unable to register NtOpenFile");

    d_->observers_NtOpenFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenFile()
{
    if(false)
        LOG(INFO, "break on NtOpenFile");

    const auto FileHandle       = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto IoStatusBlock    = arg<nt::PIO_STATUS_BLOCK>(core_, 3);
    const auto ShareAccess      = arg<nt::ULONG>(core_, 4);
    const auto OpenOptions      = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtOpenFile)
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenIoCompletion(proc_t proc, const on_NtOpenIoCompletion_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenIoCompletion");
    if(!ok)
        FAIL(false, "Unable to register NtOpenIoCompletion");

    d_->observers_NtOpenIoCompletion.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenIoCompletion()
{
    if(false)
        LOG(INFO, "break on NtOpenIoCompletion");

    const auto IoCompletionHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenIoCompletion)
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenJobObject(proc_t proc, const on_NtOpenJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtOpenJobObject");

    d_->observers_NtOpenJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenJobObject()
{
    if(false)
        LOG(INFO, "break on NtOpenJobObject");

    const auto JobHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenJobObject)
        it(JobHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenKeyedEvent");
    if(!ok)
        FAIL(false, "Unable to register NtOpenKeyedEvent");

    d_->observers_NtOpenKeyedEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyedEvent()
{
    if(false)
        LOG(INFO, "break on NtOpenKeyedEvent");

    const auto KeyedEventHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenKeyedEvent)
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyEx(proc_t proc, const on_NtOpenKeyEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenKeyEx");
    if(!ok)
        FAIL(false, "Unable to register NtOpenKeyEx");

    d_->observers_NtOpenKeyEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyEx()
{
    if(false)
        LOG(INFO, "break on NtOpenKeyEx");

    const auto KeyHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto OpenOptions      = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtOpenKeyEx)
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenKey(proc_t proc, const on_NtOpenKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenKey");
    if(!ok)
        FAIL(false, "Unable to register NtOpenKey");

    d_->observers_NtOpenKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKey()
{
    if(false)
        LOG(INFO, "break on NtOpenKey");

    const auto KeyHandle        = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenKey)
        it(KeyHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenKeyTransactedEx");
    if(!ok)
        FAIL(false, "Unable to register NtOpenKeyTransactedEx");

    d_->observers_NtOpenKeyTransactedEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyTransactedEx()
{
    if(false)
        LOG(INFO, "break on NtOpenKeyTransactedEx");

    const auto KeyHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto OpenOptions       = arg<nt::ULONG>(core_, 3);
    const auto TransactionHandle = arg<nt::HANDLE>(core_, 4);

    for(const auto& it : d_->observers_NtOpenKeyTransactedEx)
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
}

bool monitor::GenericMonitor::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenKeyTransacted");
    if(!ok)
        FAIL(false, "Unable to register NtOpenKeyTransacted");

    d_->observers_NtOpenKeyTransacted.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyTransacted()
{
    if(false)
        LOG(INFO, "break on NtOpenKeyTransacted");

    const auto KeyHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto TransactionHandle = arg<nt::HANDLE>(core_, 3);

    for(const auto& it : d_->observers_NtOpenKeyTransacted)
        it(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
}

bool monitor::GenericMonitor::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenMutant");
    if(!ok)
        FAIL(false, "Unable to register NtOpenMutant");

    d_->observers_NtOpenMutant.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenMutant()
{
    if(false)
        LOG(INFO, "break on NtOpenMutant");

    const auto MutantHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenMutant)
        it(MutantHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenObjectAuditAlarm(proc_t proc, const on_NtOpenObjectAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenObjectAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtOpenObjectAuditAlarm");

    d_->observers_NtOpenObjectAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenObjectAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtOpenObjectAuditAlarm");

    const auto SubsystemName      = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId           = arg<nt::PVOID>(core_, 1);
    const auto ObjectTypeName     = arg<nt::PUNICODE_STRING>(core_, 2);
    const auto ObjectName         = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto SecurityDescriptor = arg<nt::PSECURITY_DESCRIPTOR>(core_, 4);
    const auto ClientToken        = arg<nt::HANDLE>(core_, 5);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 6);
    const auto GrantedAccess      = arg<nt::ACCESS_MASK>(core_, 7);
    const auto Privileges         = arg<nt::PPRIVILEGE_SET>(core_, 8);
    const auto ObjectCreation     = arg<nt::BOOLEAN>(core_, 9);
    const auto AccessGranted      = arg<nt::BOOLEAN>(core_, 10);
    const auto GenerateOnClose    = arg<nt::PBOOLEAN>(core_, 11);

    for(const auto& it : d_->observers_NtOpenObjectAuditAlarm)
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenPrivateNamespace");
    if(!ok)
        FAIL(false, "Unable to register NtOpenPrivateNamespace");

    d_->observers_NtOpenPrivateNamespace.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenPrivateNamespace()
{
    if(false)
        LOG(INFO, "break on NtOpenPrivateNamespace");

    const auto NamespaceHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess      = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes   = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto BoundaryDescriptor = arg<nt::PVOID>(core_, 3);

    for(const auto& it : d_->observers_NtOpenPrivateNamespace)
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
}

bool monitor::GenericMonitor::register_NtOpenProcess(proc_t proc, const on_NtOpenProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenProcess");
    if(!ok)
        FAIL(false, "Unable to register NtOpenProcess");

    d_->observers_NtOpenProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcess()
{
    if(false)
        LOG(INFO, "break on NtOpenProcess");

    const auto ProcessHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ClientId         = arg<nt::PCLIENT_ID>(core_, 3);

    for(const auto& it : d_->observers_NtOpenProcess)
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

bool monitor::GenericMonitor::register_NtOpenProcessTokenEx(proc_t proc, const on_NtOpenProcessTokenEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenProcessTokenEx");
    if(!ok)
        FAIL(false, "Unable to register NtOpenProcessTokenEx");

    d_->observers_NtOpenProcessTokenEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcessTokenEx()
{
    if(false)
        LOG(INFO, "break on NtOpenProcessTokenEx");

    const auto ProcessHandle    = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto HandleAttributes = arg<nt::ULONG>(core_, 2);
    const auto TokenHandle      = arg<nt::PHANDLE>(core_, 3);

    for(const auto& it : d_->observers_NtOpenProcessTokenEx)
        it(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenProcessToken(proc_t proc, const on_NtOpenProcessToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenProcessToken");
    if(!ok)
        FAIL(false, "Unable to register NtOpenProcessToken");

    d_->observers_NtOpenProcessToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcessToken()
{
    if(false)
        LOG(INFO, "break on NtOpenProcessToken");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess = arg<nt::ACCESS_MASK>(core_, 1);
    const auto TokenHandle   = arg<nt::PHANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtOpenProcessToken)
        it(ProcessHandle, DesiredAccess, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenResourceManager(proc_t proc, const on_NtOpenResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtOpenResourceManager");

    d_->observers_NtOpenResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenResourceManager()
{
    if(false)
        LOG(INFO, "break on NtOpenResourceManager");

    const auto ResourceManagerHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess         = arg<nt::ACCESS_MASK>(core_, 1);
    const auto TmHandle              = arg<nt::HANDLE>(core_, 2);
    const auto ResourceManagerGuid   = arg<nt::LPGUID>(core_, 3);
    const auto ObjectAttributes      = arg<nt::POBJECT_ATTRIBUTES>(core_, 4);

    for(const auto& it : d_->observers_NtOpenResourceManager)
        it(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenSection");
    if(!ok)
        FAIL(false, "Unable to register NtOpenSection");

    d_->observers_NtOpenSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSection()
{
    if(false)
        LOG(INFO, "break on NtOpenSection");

    const auto SectionHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenSection)
        it(SectionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenSemaphore");
    if(!ok)
        FAIL(false, "Unable to register NtOpenSemaphore");

    d_->observers_NtOpenSemaphore.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSemaphore()
{
    if(false)
        LOG(INFO, "break on NtOpenSemaphore");

    const auto SemaphoreHandle  = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenSemaphore)
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenSession");
    if(!ok)
        FAIL(false, "Unable to register NtOpenSession");

    d_->observers_NtOpenSession.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSession()
{
    if(false)
        LOG(INFO, "break on NtOpenSession");

    const auto SessionHandle    = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenSession)
        it(SessionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenSymbolicLinkObject");
    if(!ok)
        FAIL(false, "Unable to register NtOpenSymbolicLinkObject");

    d_->observers_NtOpenSymbolicLinkObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSymbolicLinkObject()
{
    if(false)
        LOG(INFO, "break on NtOpenSymbolicLinkObject");

    const auto LinkHandle       = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenSymbolicLinkObject)
        it(LinkHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenThread(proc_t proc, const on_NtOpenThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenThread");
    if(!ok)
        FAIL(false, "Unable to register NtOpenThread");

    d_->observers_NtOpenThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThread()
{
    if(false)
        LOG(INFO, "break on NtOpenThread");

    const auto ThreadHandle     = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto ClientId         = arg<nt::PCLIENT_ID>(core_, 3);

    for(const auto& it : d_->observers_NtOpenThread)
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
}

bool monitor::GenericMonitor::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenThreadTokenEx");
    if(!ok)
        FAIL(false, "Unable to register NtOpenThreadTokenEx");

    d_->observers_NtOpenThreadTokenEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThreadTokenEx()
{
    if(false)
        LOG(INFO, "break on NtOpenThreadTokenEx");

    const auto ThreadHandle     = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto OpenAsSelf       = arg<nt::BOOLEAN>(core_, 2);
    const auto HandleAttributes = arg<nt::ULONG>(core_, 3);
    const auto TokenHandle      = arg<nt::PHANDLE>(core_, 4);

    for(const auto& it : d_->observers_NtOpenThreadTokenEx)
        it(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenThreadToken");
    if(!ok)
        FAIL(false, "Unable to register NtOpenThreadToken");

    d_->observers_NtOpenThreadToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThreadToken()
{
    if(false)
        LOG(INFO, "break on NtOpenThreadToken");

    const auto ThreadHandle  = arg<nt::HANDLE>(core_, 0);
    const auto DesiredAccess = arg<nt::ACCESS_MASK>(core_, 1);
    const auto OpenAsSelf    = arg<nt::BOOLEAN>(core_, 2);
    const auto TokenHandle   = arg<nt::PHANDLE>(core_, 3);

    for(const auto& it : d_->observers_NtOpenThreadToken)
        it(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenTimer(proc_t proc, const on_NtOpenTimer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenTimer");
    if(!ok)
        FAIL(false, "Unable to register NtOpenTimer");

    d_->observers_NtOpenTimer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTimer()
{
    if(false)
        LOG(INFO, "break on NtOpenTimer");

    const auto TimerHandle      = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtOpenTimer)
        it(TimerHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenTransactionManager(proc_t proc, const on_NtOpenTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtOpenTransactionManager");

    d_->observers_NtOpenTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtOpenTransactionManager");

    const auto TmHandle         = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess    = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto LogFileName      = arg<nt::PUNICODE_STRING>(core_, 3);
    const auto TmIdentity       = arg<nt::LPGUID>(core_, 4);
    const auto OpenOptions      = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtOpenTransactionManager)
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenTransaction(proc_t proc, const on_NtOpenTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtOpenTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtOpenTransaction");

    d_->observers_NtOpenTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTransaction()
{
    if(false)
        LOG(INFO, "break on NtOpenTransaction");

    const auto TransactionHandle = arg<nt::PHANDLE>(core_, 0);
    const auto DesiredAccess     = arg<nt::ACCESS_MASK>(core_, 1);
    const auto ObjectAttributes  = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);
    const auto Uow               = arg<nt::LPGUID>(core_, 3);
    const auto TmHandle          = arg<nt::HANDLE>(core_, 4);

    for(const auto& it : d_->observers_NtOpenTransaction)
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
}

bool monitor::GenericMonitor::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPlugPlayControl");
    if(!ok)
        FAIL(false, "Unable to register NtPlugPlayControl");

    d_->observers_NtPlugPlayControl.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPlugPlayControl()
{
    if(false)
        LOG(INFO, "break on NtPlugPlayControl");

    const auto PnPControlClass      = arg<nt::PLUGPLAY_CONTROL_CLASS>(core_, 0);
    const auto PnPControlData       = arg<nt::PVOID>(core_, 1);
    const auto PnPControlDataLength = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtPlugPlayControl)
        it(PnPControlClass, PnPControlData, PnPControlDataLength);
}

bool monitor::GenericMonitor::register_NtPowerInformation(proc_t proc, const on_NtPowerInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPowerInformation");
    if(!ok)
        FAIL(false, "Unable to register NtPowerInformation");

    d_->observers_NtPowerInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPowerInformation()
{
    if(false)
        LOG(INFO, "break on NtPowerInformation");

    const auto InformationLevel   = arg<nt::POWER_INFORMATION_LEVEL>(core_, 0);
    const auto InputBuffer        = arg<nt::PVOID>(core_, 1);
    const auto InputBufferLength  = arg<nt::ULONG>(core_, 2);
    const auto OutputBuffer       = arg<nt::PVOID>(core_, 3);
    const auto OutputBufferLength = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtPowerInformation)
        it(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrepareComplete");
    if(!ok)
        FAIL(false, "Unable to register NtPrepareComplete");

    d_->observers_NtPrepareComplete.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrepareComplete()
{
    if(false)
        LOG(INFO, "break on NtPrepareComplete");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtPrepareComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrepareEnlistment(proc_t proc, const on_NtPrepareEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrepareEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtPrepareEnlistment");

    d_->observers_NtPrepareEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrepareEnlistment()
{
    if(false)
        LOG(INFO, "break on NtPrepareEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtPrepareEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrePrepareComplete(proc_t proc, const on_NtPrePrepareComplete_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrePrepareComplete");
    if(!ok)
        FAIL(false, "Unable to register NtPrePrepareComplete");

    d_->observers_NtPrePrepareComplete.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrePrepareComplete()
{
    if(false)
        LOG(INFO, "break on NtPrePrepareComplete");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtPrePrepareComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrePrepareEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtPrePrepareEnlistment");

    d_->observers_NtPrePrepareEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrePrepareEnlistment()
{
    if(false)
        LOG(INFO, "break on NtPrePrepareEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtPrePrepareEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrivilegeCheck(proc_t proc, const on_NtPrivilegeCheck_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrivilegeCheck");
    if(!ok)
        FAIL(false, "Unable to register NtPrivilegeCheck");

    d_->observers_NtPrivilegeCheck.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegeCheck()
{
    if(false)
        LOG(INFO, "break on NtPrivilegeCheck");

    const auto ClientToken        = arg<nt::HANDLE>(core_, 0);
    const auto RequiredPrivileges = arg<nt::PPRIVILEGE_SET>(core_, 1);
    const auto Result             = arg<nt::PBOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtPrivilegeCheck)
        it(ClientToken, RequiredPrivileges, Result);
}

bool monitor::GenericMonitor::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrivilegedServiceAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtPrivilegedServiceAuditAlarm");

    d_->observers_NtPrivilegedServiceAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegedServiceAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtPrivilegedServiceAuditAlarm");

    const auto SubsystemName = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto ServiceName   = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto ClientToken   = arg<nt::HANDLE>(core_, 2);
    const auto Privileges    = arg<nt::PPRIVILEGE_SET>(core_, 3);
    const auto AccessGranted = arg<nt::BOOLEAN>(core_, 4);

    for(const auto& it : d_->observers_NtPrivilegedServiceAuditAlarm)
        it(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
}

bool monitor::GenericMonitor::register_NtPrivilegeObjectAuditAlarm(proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPrivilegeObjectAuditAlarm");
    if(!ok)
        FAIL(false, "Unable to register NtPrivilegeObjectAuditAlarm");

    d_->observers_NtPrivilegeObjectAuditAlarm.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegeObjectAuditAlarm()
{
    if(false)
        LOG(INFO, "break on NtPrivilegeObjectAuditAlarm");

    const auto SubsystemName = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto HandleId      = arg<nt::PVOID>(core_, 1);
    const auto ClientToken   = arg<nt::HANDLE>(core_, 2);
    const auto DesiredAccess = arg<nt::ACCESS_MASK>(core_, 3);
    const auto Privileges    = arg<nt::PPRIVILEGE_SET>(core_, 4);
    const auto AccessGranted = arg<nt::BOOLEAN>(core_, 5);

    for(const auto& it : d_->observers_NtPrivilegeObjectAuditAlarm)
        it(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
}

bool monitor::GenericMonitor::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPropagationComplete");
    if(!ok)
        FAIL(false, "Unable to register NtPropagationComplete");

    d_->observers_NtPropagationComplete.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPropagationComplete()
{
    if(false)
        LOG(INFO, "break on NtPropagationComplete");

    const auto ResourceManagerHandle = arg<nt::HANDLE>(core_, 0);
    const auto RequestCookie         = arg<nt::ULONG>(core_, 1);
    const auto BufferLength          = arg<nt::ULONG>(core_, 2);
    const auto Buffer                = arg<nt::PVOID>(core_, 3);

    for(const auto& it : d_->observers_NtPropagationComplete)
        it(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
}

bool monitor::GenericMonitor::register_NtPropagationFailed(proc_t proc, const on_NtPropagationFailed_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPropagationFailed");
    if(!ok)
        FAIL(false, "Unable to register NtPropagationFailed");

    d_->observers_NtPropagationFailed.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPropagationFailed()
{
    if(false)
        LOG(INFO, "break on NtPropagationFailed");

    const auto ResourceManagerHandle = arg<nt::HANDLE>(core_, 0);
    const auto RequestCookie         = arg<nt::ULONG>(core_, 1);
    const auto PropStatus            = arg<nt::NTSTATUS>(core_, 2);

    for(const auto& it : d_->observers_NtPropagationFailed)
        it(ResourceManagerHandle, RequestCookie, PropStatus);
}

bool monitor::GenericMonitor::register_NtProtectVirtualMemory(proc_t proc, const on_NtProtectVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtProtectVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtProtectVirtualMemory");

    d_->observers_NtProtectVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtProtectVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtProtectVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 2);
    const auto NewProtectWin32 = arg<nt::WIN32_PROTECTION_MASK>(core_, 3);
    const auto OldProtect      = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtProtectVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
}

bool monitor::GenericMonitor::register_NtPulseEvent(proc_t proc, const on_NtPulseEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtPulseEvent");
    if(!ok)
        FAIL(false, "Unable to register NtPulseEvent");

    d_->observers_NtPulseEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtPulseEvent()
{
    if(false)
        LOG(INFO, "break on NtPulseEvent");

    const auto EventHandle   = arg<nt::HANDLE>(core_, 0);
    const auto PreviousState = arg<nt::PLONG>(core_, 1);

    for(const auto& it : d_->observers_NtPulseEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtQueryAttributesFile(proc_t proc, const on_NtQueryAttributesFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryAttributesFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryAttributesFile");

    d_->observers_NtQueryAttributesFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryAttributesFile()
{
    if(false)
        LOG(INFO, "break on NtQueryAttributesFile");

    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto FileInformation  = arg<nt::PFILE_BASIC_INFORMATION>(core_, 1);

    for(const auto& it : d_->observers_NtQueryAttributesFile)
        it(ObjectAttributes, FileInformation);
}

bool monitor::GenericMonitor::register_NtQueryBootEntryOrder(proc_t proc, const on_NtQueryBootEntryOrder_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryBootEntryOrder");
    if(!ok)
        FAIL(false, "Unable to register NtQueryBootEntryOrder");

    d_->observers_NtQueryBootEntryOrder.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryBootEntryOrder()
{
    if(false)
        LOG(INFO, "break on NtQueryBootEntryOrder");

    const auto Ids   = arg<nt::PULONG>(core_, 0);
    const auto Count = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryBootEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtQueryBootOptions(proc_t proc, const on_NtQueryBootOptions_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryBootOptions");
    if(!ok)
        FAIL(false, "Unable to register NtQueryBootOptions");

    d_->observers_NtQueryBootOptions.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryBootOptions()
{
    if(false)
        LOG(INFO, "break on NtQueryBootOptions");

    const auto BootOptions       = arg<nt::PBOOT_OPTIONS>(core_, 0);
    const auto BootOptionsLength = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryBootOptions)
        it(BootOptions, BootOptionsLength);
}

bool monitor::GenericMonitor::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDebugFilterState");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDebugFilterState");

    d_->observers_NtQueryDebugFilterState.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDebugFilterState()
{
    if(false)
        LOG(INFO, "break on NtQueryDebugFilterState");

    const auto ComponentId = arg<nt::ULONG>(core_, 0);
    const auto Level       = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryDebugFilterState)
        it(ComponentId, Level);
}

bool monitor::GenericMonitor::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDefaultLocale");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDefaultLocale");

    d_->observers_NtQueryDefaultLocale.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDefaultLocale()
{
    if(false)
        LOG(INFO, "break on NtQueryDefaultLocale");

    const auto UserProfile     = arg<nt::BOOLEAN>(core_, 0);
    const auto DefaultLocaleId = arg<nt::PLCID>(core_, 1);

    for(const auto& it : d_->observers_NtQueryDefaultLocale)
        it(UserProfile, DefaultLocaleId);
}

bool monitor::GenericMonitor::register_NtQueryDefaultUILanguage(proc_t proc, const on_NtQueryDefaultUILanguage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDefaultUILanguage");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDefaultUILanguage");

    d_->observers_NtQueryDefaultUILanguage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDefaultUILanguage()
{
    if(false)
        LOG(INFO, "break on NtQueryDefaultUILanguage");

    const auto STARDefaultUILanguageId = arg<nt::LANGID>(core_, 0);

    for(const auto& it : d_->observers_NtQueryDefaultUILanguage)
        it(STARDefaultUILanguageId);
}

bool monitor::GenericMonitor::register_NtQueryDirectoryFile(proc_t proc, const on_NtQueryDirectoryFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDirectoryFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDirectoryFile");

    d_->observers_NtQueryDirectoryFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDirectoryFile()
{
    if(false)
        LOG(INFO, "break on NtQueryDirectoryFile");

    const auto FileHandle           = arg<nt::HANDLE>(core_, 0);
    const auto Event                = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine           = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext           = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto FileInformation      = arg<nt::PVOID>(core_, 5);
    const auto Length               = arg<nt::ULONG>(core_, 6);
    const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core_, 7);
    const auto ReturnSingleEntry    = arg<nt::BOOLEAN>(core_, 8);
    const auto FileName             = arg<nt::PUNICODE_STRING>(core_, 9);
    const auto RestartScan          = arg<nt::BOOLEAN>(core_, 10);

    for(const auto& it : d_->observers_NtQueryDirectoryFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

bool monitor::GenericMonitor::register_NtQueryDirectoryObject(proc_t proc, const on_NtQueryDirectoryObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDirectoryObject");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDirectoryObject");

    d_->observers_NtQueryDirectoryObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDirectoryObject()
{
    if(false)
        LOG(INFO, "break on NtQueryDirectoryObject");

    const auto DirectoryHandle   = arg<nt::HANDLE>(core_, 0);
    const auto Buffer            = arg<nt::PVOID>(core_, 1);
    const auto Length            = arg<nt::ULONG>(core_, 2);
    const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core_, 3);
    const auto RestartScan       = arg<nt::BOOLEAN>(core_, 4);
    const auto Context           = arg<nt::PULONG>(core_, 5);
    const auto ReturnLength      = arg<nt::PULONG>(core_, 6);

    for(const auto& it : d_->observers_NtQueryDirectoryObject)
        it(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryDriverEntryOrder");
    if(!ok)
        FAIL(false, "Unable to register NtQueryDriverEntryOrder");

    d_->observers_NtQueryDriverEntryOrder.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDriverEntryOrder()
{
    if(false)
        LOG(INFO, "break on NtQueryDriverEntryOrder");

    const auto Ids   = arg<nt::PULONG>(core_, 0);
    const auto Count = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryDriverEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtQueryEaFile(proc_t proc, const on_NtQueryEaFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryEaFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryEaFile");

    d_->observers_NtQueryEaFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryEaFile()
{
    if(false)
        LOG(INFO, "break on NtQueryEaFile");

    const auto FileHandle        = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto Buffer            = arg<nt::PVOID>(core_, 2);
    const auto Length            = arg<nt::ULONG>(core_, 3);
    const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core_, 4);
    const auto EaList            = arg<nt::PVOID>(core_, 5);
    const auto EaListLength      = arg<nt::ULONG>(core_, 6);
    const auto EaIndex           = arg<nt::PULONG>(core_, 7);
    const auto RestartScan       = arg<nt::BOOLEAN>(core_, 8);

    for(const auto& it : d_->observers_NtQueryEaFile)
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
}

bool monitor::GenericMonitor::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryEvent");
    if(!ok)
        FAIL(false, "Unable to register NtQueryEvent");

    d_->observers_NtQueryEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryEvent()
{
    if(false)
        LOG(INFO, "break on NtQueryEvent");

    const auto EventHandle            = arg<nt::HANDLE>(core_, 0);
    const auto EventInformationClass  = arg<nt::EVENT_INFORMATION_CLASS>(core_, 1);
    const auto EventInformation       = arg<nt::PVOID>(core_, 2);
    const auto EventInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength           = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryEvent)
        it(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryFullAttributesFile(proc_t proc, const on_NtQueryFullAttributesFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryFullAttributesFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryFullAttributesFile");

    d_->observers_NtQueryFullAttributesFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryFullAttributesFile()
{
    if(false)
        LOG(INFO, "break on NtQueryFullAttributesFile");

    const auto ObjectAttributes = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto FileInformation  = arg<nt::PFILE_NETWORK_OPEN_INFORMATION>(core_, 1);

    for(const auto& it : d_->observers_NtQueryFullAttributesFile)
        it(ObjectAttributes, FileInformation);
}

bool monitor::GenericMonitor::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationAtom");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationAtom");

    d_->observers_NtQueryInformationAtom.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationAtom()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationAtom");

    const auto Atom                  = arg<nt::RTL_ATOM>(core_, 0);
    const auto InformationClass      = arg<nt::ATOM_INFORMATION_CLASS>(core_, 1);
    const auto AtomInformation       = arg<nt::PVOID>(core_, 2);
    const auto AtomInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength          = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationAtom)
        it(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationEnlistment(proc_t proc, const on_NtQueryInformationEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationEnlistment");

    d_->observers_NtQueryInformationEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationEnlistment()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationEnlistment");

    const auto EnlistmentHandle            = arg<nt::HANDLE>(core_, 0);
    const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(core_, 1);
    const auto EnlistmentInformation       = arg<nt::PVOID>(core_, 2);
    const auto EnlistmentInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationEnlistment)
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationFile(proc_t proc, const on_NtQueryInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationFile");

    d_->observers_NtQueryInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationFile()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationFile");

    const auto FileHandle           = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto FileInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length               = arg<nt::ULONG>(core_, 3);
    const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationFile)
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

bool monitor::GenericMonitor::register_NtQueryInformationJobObject(proc_t proc, const on_NtQueryInformationJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationJobObject");

    d_->observers_NtQueryInformationJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationJobObject()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationJobObject");

    const auto JobHandle                  = arg<nt::HANDLE>(core_, 0);
    const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(core_, 1);
    const auto JobObjectInformation       = arg<nt::PVOID>(core_, 2);
    const auto JobObjectInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength               = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationJobObject)
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationPort(proc_t proc, const on_NtQueryInformationPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationPort");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationPort");

    d_->observers_NtQueryInformationPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationPort()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationPort");

    const auto PortHandle           = arg<nt::HANDLE>(core_, 0);
    const auto PortInformationClass = arg<nt::PORT_INFORMATION_CLASS>(core_, 1);
    const auto PortInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length               = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength         = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationPort)
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationProcess(proc_t proc, const on_NtQueryInformationProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationProcess");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationProcess");

    d_->observers_NtQueryInformationProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationProcess()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationProcess");

    const auto ProcessHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(core_, 1);
    const auto ProcessInformation       = arg<nt::PVOID>(core_, 2);
    const auto ProcessInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength             = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationProcess)
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationResourceManager(proc_t proc, const on_NtQueryInformationResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationResourceManager");

    d_->observers_NtQueryInformationResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationResourceManager()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationResourceManager");

    const auto ResourceManagerHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(core_, 1);
    const auto ResourceManagerInformation       = arg<nt::PVOID>(core_, 2);
    const auto ResourceManagerInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                     = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationResourceManager)
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationThread");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationThread");

    d_->observers_NtQueryInformationThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationThread()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationThread");

    const auto ThreadHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(core_, 1);
    const auto ThreadInformation       = arg<nt::PVOID>(core_, 2);
    const auto ThreadInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationThread)
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationToken(proc_t proc, const on_NtQueryInformationToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationToken");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationToken");

    d_->observers_NtQueryInformationToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationToken()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationToken");

    const auto TokenHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(core_, 1);
    const auto TokenInformation       = arg<nt::PVOID>(core_, 2);
    const auto TokenInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength           = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationToken)
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationTransaction(proc_t proc, const on_NtQueryInformationTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationTransaction");

    d_->observers_NtQueryInformationTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationTransaction()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationTransaction");

    const auto TransactionHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(core_, 1);
    const auto TransactionInformation       = arg<nt::PVOID>(core_, 2);
    const auto TransactionInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                 = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationTransaction)
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationTransactionManager");

    d_->observers_NtQueryInformationTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationTransactionManager");

    const auto TransactionManagerHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(core_, 1);
    const auto TransactionManagerInformation       = arg<nt::PVOID>(core_, 2);
    const auto TransactionManagerInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                        = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationTransactionManager)
        it(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationWorkerFactory(proc_t proc, const on_NtQueryInformationWorkerFactory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInformationWorkerFactory");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInformationWorkerFactory");

    d_->observers_NtQueryInformationWorkerFactory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationWorkerFactory()
{
    if(false)
        LOG(INFO, "break on NtQueryInformationWorkerFactory");

    const auto WorkerFactoryHandle            = arg<nt::HANDLE>(core_, 0);
    const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(core_, 1);
    const auto WorkerFactoryInformation       = arg<nt::PVOID>(core_, 2);
    const auto WorkerFactoryInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                   = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryInformationWorkerFactory)
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryInstallUILanguage");
    if(!ok)
        FAIL(false, "Unable to register NtQueryInstallUILanguage");

    d_->observers_NtQueryInstallUILanguage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInstallUILanguage()
{
    if(false)
        LOG(INFO, "break on NtQueryInstallUILanguage");

    const auto STARInstallUILanguageId = arg<nt::LANGID>(core_, 0);

    for(const auto& it : d_->observers_NtQueryInstallUILanguage)
        it(STARInstallUILanguageId);
}

bool monitor::GenericMonitor::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryIntervalProfile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryIntervalProfile");

    d_->observers_NtQueryIntervalProfile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryIntervalProfile()
{
    if(false)
        LOG(INFO, "break on NtQueryIntervalProfile");

    const auto ProfileSource = arg<nt::KPROFILE_SOURCE>(core_, 0);
    const auto Interval      = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryIntervalProfile)
        it(ProfileSource, Interval);
}

bool monitor::GenericMonitor::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryIoCompletion");
    if(!ok)
        FAIL(false, "Unable to register NtQueryIoCompletion");

    d_->observers_NtQueryIoCompletion.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryIoCompletion()
{
    if(false)
        LOG(INFO, "break on NtQueryIoCompletion");

    const auto IoCompletionHandle            = arg<nt::HANDLE>(core_, 0);
    const auto IoCompletionInformationClass  = arg<nt::IO_COMPLETION_INFORMATION_CLASS>(core_, 1);
    const auto IoCompletionInformation       = arg<nt::PVOID>(core_, 2);
    const auto IoCompletionInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                  = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryIoCompletion)
        it(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryKey(proc_t proc, const on_NtQueryKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryKey");
    if(!ok)
        FAIL(false, "Unable to register NtQueryKey");

    d_->observers_NtQueryKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryKey()
{
    if(false)
        LOG(INFO, "break on NtQueryKey");

    const auto KeyHandle           = arg<nt::HANDLE>(core_, 0);
    const auto KeyInformationClass = arg<nt::KEY_INFORMATION_CLASS>(core_, 1);
    const auto KeyInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length              = arg<nt::ULONG>(core_, 3);
    const auto ResultLength        = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryKey)
        it(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryLicenseValue");
    if(!ok)
        FAIL(false, "Unable to register NtQueryLicenseValue");

    d_->observers_NtQueryLicenseValue.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryLicenseValue()
{
    if(false)
        LOG(INFO, "break on NtQueryLicenseValue");

    const auto Name           = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto Type           = arg<nt::PULONG>(core_, 1);
    const auto Buffer         = arg<nt::PVOID>(core_, 2);
    const auto Length         = arg<nt::ULONG>(core_, 3);
    const auto ReturnedLength = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryLicenseValue)
        it(Name, Type, Buffer, Length, ReturnedLength);
}

bool monitor::GenericMonitor::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryMultipleValueKey");
    if(!ok)
        FAIL(false, "Unable to register NtQueryMultipleValueKey");

    d_->observers_NtQueryMultipleValueKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryMultipleValueKey()
{
    if(false)
        LOG(INFO, "break on NtQueryMultipleValueKey");

    const auto KeyHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ValueEntries         = arg<nt::PKEY_VALUE_ENTRY>(core_, 1);
    const auto EntryCount           = arg<nt::ULONG>(core_, 2);
    const auto ValueBuffer          = arg<nt::PVOID>(core_, 3);
    const auto BufferLength         = arg<nt::PULONG>(core_, 4);
    const auto RequiredBufferLength = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtQueryMultipleValueKey)
        it(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
}

bool monitor::GenericMonitor::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryMutant");
    if(!ok)
        FAIL(false, "Unable to register NtQueryMutant");

    d_->observers_NtQueryMutant.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryMutant()
{
    if(false)
        LOG(INFO, "break on NtQueryMutant");

    const auto MutantHandle            = arg<nt::HANDLE>(core_, 0);
    const auto MutantInformationClass  = arg<nt::MUTANT_INFORMATION_CLASS>(core_, 1);
    const auto MutantInformation       = arg<nt::PVOID>(core_, 2);
    const auto MutantInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryMutant)
        it(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryObject");
    if(!ok)
        FAIL(false, "Unable to register NtQueryObject");

    d_->observers_NtQueryObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryObject()
{
    if(false)
        LOG(INFO, "break on NtQueryObject");

    const auto Handle                  = arg<nt::HANDLE>(core_, 0);
    const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(core_, 1);
    const auto ObjectInformation       = arg<nt::PVOID>(core_, 2);
    const auto ObjectInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryObject)
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryOpenSubKeysEx");
    if(!ok)
        FAIL(false, "Unable to register NtQueryOpenSubKeysEx");

    d_->observers_NtQueryOpenSubKeysEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeysEx()
{
    if(false)
        LOG(INFO, "break on NtQueryOpenSubKeysEx");

    const auto TargetKey    = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto BufferLength = arg<nt::ULONG>(core_, 1);
    const auto Buffer       = arg<nt::PVOID>(core_, 2);
    const auto RequiredSize = arg<nt::PULONG>(core_, 3);

    for(const auto& it : d_->observers_NtQueryOpenSubKeysEx)
        it(TargetKey, BufferLength, Buffer, RequiredSize);
}

bool monitor::GenericMonitor::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryOpenSubKeys");
    if(!ok)
        FAIL(false, "Unable to register NtQueryOpenSubKeys");

    d_->observers_NtQueryOpenSubKeys.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeys()
{
    if(false)
        LOG(INFO, "break on NtQueryOpenSubKeys");

    const auto TargetKey   = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto HandleCount = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtQueryOpenSubKeys)
        it(TargetKey, HandleCount);
}

bool monitor::GenericMonitor::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryPerformanceCounter");
    if(!ok)
        FAIL(false, "Unable to register NtQueryPerformanceCounter");

    d_->observers_NtQueryPerformanceCounter.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryPerformanceCounter()
{
    if(false)
        LOG(INFO, "break on NtQueryPerformanceCounter");

    const auto PerformanceCounter   = arg<nt::PLARGE_INTEGER>(core_, 0);
    const auto PerformanceFrequency = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtQueryPerformanceCounter)
        it(PerformanceCounter, PerformanceFrequency);
}

bool monitor::GenericMonitor::register_NtQueryQuotaInformationFile(proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryQuotaInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryQuotaInformationFile");

    d_->observers_NtQueryQuotaInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryQuotaInformationFile()
{
    if(false)
        LOG(INFO, "break on NtQueryQuotaInformationFile");

    const auto FileHandle        = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock     = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto Buffer            = arg<nt::PVOID>(core_, 2);
    const auto Length            = arg<nt::ULONG>(core_, 3);
    const auto ReturnSingleEntry = arg<nt::BOOLEAN>(core_, 4);
    const auto SidList           = arg<nt::PVOID>(core_, 5);
    const auto SidListLength     = arg<nt::ULONG>(core_, 6);
    const auto StartSid          = arg<nt::PULONG>(core_, 7);
    const auto RestartScan       = arg<nt::BOOLEAN>(core_, 8);

    for(const auto& it : d_->observers_NtQueryQuotaInformationFile)
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
}

bool monitor::GenericMonitor::register_NtQuerySection(proc_t proc, const on_NtQuerySection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySection");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySection");

    d_->observers_NtQuerySection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySection()
{
    if(false)
        LOG(INFO, "break on NtQuerySection");

    const auto SectionHandle            = arg<nt::HANDLE>(core_, 0);
    const auto SectionInformationClass  = arg<nt::SECTION_INFORMATION_CLASS>(core_, 1);
    const auto SectionInformation       = arg<nt::PVOID>(core_, 2);
    const auto SectionInformationLength = arg<nt::SIZE_T>(core_, 3);
    const auto ReturnLength             = arg<nt::PSIZE_T>(core_, 4);

    for(const auto& it : d_->observers_NtQuerySection)
        it(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySecurityAttributesToken(proc_t proc, const on_NtQuerySecurityAttributesToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySecurityAttributesToken");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySecurityAttributesToken");

    d_->observers_NtQuerySecurityAttributesToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySecurityAttributesToken()
{
    if(false)
        LOG(INFO, "break on NtQuerySecurityAttributesToken");

    const auto TokenHandle        = arg<nt::HANDLE>(core_, 0);
    const auto Attributes         = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto NumberOfAttributes = arg<nt::ULONG>(core_, 2);
    const auto Buffer             = arg<nt::PVOID>(core_, 3);
    const auto Length             = arg<nt::ULONG>(core_, 4);
    const auto ReturnLength       = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtQuerySecurityAttributesToken)
        it(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySecurityObject");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySecurityObject");

    d_->observers_NtQuerySecurityObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySecurityObject()
{
    if(false)
        LOG(INFO, "break on NtQuerySecurityObject");

    const auto Handle              = arg<nt::HANDLE>(core_, 0);
    const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(core_, 1);
    const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(core_, 2);
    const auto Length              = arg<nt::ULONG>(core_, 3);
    const auto LengthNeeded        = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQuerySecurityObject)
        it(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
}

bool monitor::GenericMonitor::register_NtQuerySemaphore(proc_t proc, const on_NtQuerySemaphore_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySemaphore");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySemaphore");

    d_->observers_NtQuerySemaphore.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySemaphore()
{
    if(false)
        LOG(INFO, "break on NtQuerySemaphore");

    const auto SemaphoreHandle            = arg<nt::HANDLE>(core_, 0);
    const auto SemaphoreInformationClass  = arg<nt::SEMAPHORE_INFORMATION_CLASS>(core_, 1);
    const auto SemaphoreInformation       = arg<nt::PVOID>(core_, 2);
    const auto SemaphoreInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength               = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQuerySemaphore)
        it(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySymbolicLinkObject(proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySymbolicLinkObject");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySymbolicLinkObject");

    d_->observers_NtQuerySymbolicLinkObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySymbolicLinkObject()
{
    if(false)
        LOG(INFO, "break on NtQuerySymbolicLinkObject");

    const auto LinkHandle     = arg<nt::HANDLE>(core_, 0);
    const auto LinkTarget     = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto ReturnedLength = arg<nt::PULONG>(core_, 2);

    for(const auto& it : d_->observers_NtQuerySymbolicLinkObject)
        it(LinkHandle, LinkTarget, ReturnedLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemEnvironmentValueEx(proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySystemEnvironmentValueEx");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySystemEnvironmentValueEx");

    d_->observers_NtQuerySystemEnvironmentValueEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValueEx()
{
    if(false)
        LOG(INFO, "break on NtQuerySystemEnvironmentValueEx");

    const auto VariableName = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto VendorGuid   = arg<nt::LPGUID>(core_, 1);
    const auto Value        = arg<nt::PVOID>(core_, 2);
    const auto ValueLength  = arg<nt::PULONG>(core_, 3);
    const auto Attributes   = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValueEx)
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
}

bool monitor::GenericMonitor::register_NtQuerySystemEnvironmentValue(proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySystemEnvironmentValue");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySystemEnvironmentValue");

    d_->observers_NtQuerySystemEnvironmentValue.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValue()
{
    if(false)
        LOG(INFO, "break on NtQuerySystemEnvironmentValue");

    const auto VariableName  = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto VariableValue = arg<nt::PWSTR>(core_, 1);
    const auto ValueLength   = arg<nt::USHORT>(core_, 2);
    const auto ReturnLength  = arg<nt::PUSHORT>(core_, 3);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValue)
        it(VariableName, VariableValue, ValueLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemInformationEx(proc_t proc, const on_NtQuerySystemInformationEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySystemInformationEx");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySystemInformationEx");

    d_->observers_NtQuerySystemInformationEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemInformationEx()
{
    if(false)
        LOG(INFO, "break on NtQuerySystemInformationEx");

    const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core_, 0);
    const auto QueryInformation        = arg<nt::PVOID>(core_, 1);
    const auto QueryInformationLength  = arg<nt::ULONG>(core_, 2);
    const auto SystemInformation       = arg<nt::PVOID>(core_, 3);
    const auto SystemInformationLength = arg<nt::ULONG>(core_, 4);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtQuerySystemInformationEx)
        it(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySystemInformation");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySystemInformation");

    d_->observers_NtQuerySystemInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemInformation()
{
    if(false)
        LOG(INFO, "break on NtQuerySystemInformation");

    const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core_, 0);
    const auto SystemInformation       = arg<nt::PVOID>(core_, 1);
    const auto SystemInformationLength = arg<nt::ULONG>(core_, 2);
    const auto ReturnLength            = arg<nt::PULONG>(core_, 3);

    for(const auto& it : d_->observers_NtQuerySystemInformation)
        it(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQuerySystemTime");
    if(!ok)
        FAIL(false, "Unable to register NtQuerySystemTime");

    d_->observers_NtQuerySystemTime.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemTime()
{
    if(false)
        LOG(INFO, "break on NtQuerySystemTime");

    const auto SystemTime = arg<nt::PLARGE_INTEGER>(core_, 0);

    for(const auto& it : d_->observers_NtQuerySystemTime)
        it(SystemTime);
}

bool monitor::GenericMonitor::register_NtQueryTimer(proc_t proc, const on_NtQueryTimer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryTimer");
    if(!ok)
        FAIL(false, "Unable to register NtQueryTimer");

    d_->observers_NtQueryTimer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryTimer()
{
    if(false)
        LOG(INFO, "break on NtQueryTimer");

    const auto TimerHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TimerInformationClass  = arg<nt::TIMER_INFORMATION_CLASS>(core_, 1);
    const auto TimerInformation       = arg<nt::PVOID>(core_, 2);
    const auto TimerInformationLength = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength           = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtQueryTimer)
        it(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryTimerResolution");
    if(!ok)
        FAIL(false, "Unable to register NtQueryTimerResolution");

    d_->observers_NtQueryTimerResolution.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryTimerResolution()
{
    if(false)
        LOG(INFO, "break on NtQueryTimerResolution");

    const auto MaximumTime = arg<nt::PULONG>(core_, 0);
    const auto MinimumTime = arg<nt::PULONG>(core_, 1);
    const auto CurrentTime = arg<nt::PULONG>(core_, 2);

    for(const auto& it : d_->observers_NtQueryTimerResolution)
        it(MaximumTime, MinimumTime, CurrentTime);
}

bool monitor::GenericMonitor::register_NtQueryValueKey(proc_t proc, const on_NtQueryValueKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryValueKey");
    if(!ok)
        FAIL(false, "Unable to register NtQueryValueKey");

    d_->observers_NtQueryValueKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryValueKey()
{
    if(false)
        LOG(INFO, "break on NtQueryValueKey");

    const auto KeyHandle                = arg<nt::HANDLE>(core_, 0);
    const auto ValueName                = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto KeyValueInformationClass = arg<nt::KEY_VALUE_INFORMATION_CLASS>(core_, 2);
    const auto KeyValueInformation      = arg<nt::PVOID>(core_, 3);
    const auto Length                   = arg<nt::ULONG>(core_, 4);
    const auto ResultLength             = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtQueryValueKey)
        it(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtQueryVirtualMemory");

    d_->observers_NtQueryVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtQueryVirtualMemory");

    const auto ProcessHandle           = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress             = arg<nt::PVOID>(core_, 1);
    const auto MemoryInformationClass  = arg<nt::MEMORY_INFORMATION_CLASS>(core_, 2);
    const auto MemoryInformation       = arg<nt::PVOID>(core_, 3);
    const auto MemoryInformationLength = arg<nt::SIZE_T>(core_, 4);
    const auto ReturnLength            = arg<nt::PSIZE_T>(core_, 5);

    for(const auto& it : d_->observers_NtQueryVirtualMemory)
        it(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryVolumeInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtQueryVolumeInformationFile");

    d_->observers_NtQueryVolumeInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryVolumeInformationFile()
{
    if(false)
        LOG(INFO, "break on NtQueryVolumeInformationFile");

    const auto FileHandle         = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto FsInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length             = arg<nt::ULONG>(core_, 3);
    const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(core_, 4);

    for(const auto& it : d_->observers_NtQueryVolumeInformationFile)
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

bool monitor::GenericMonitor::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueueApcThreadEx");
    if(!ok)
        FAIL(false, "Unable to register NtQueueApcThreadEx");

    d_->observers_NtQueueApcThreadEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueueApcThreadEx()
{
    if(false)
        LOG(INFO, "break on NtQueueApcThreadEx");

    const auto ThreadHandle         = arg<nt::HANDLE>(core_, 0);
    const auto UserApcReserveHandle = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine           = arg<nt::PPS_APC_ROUTINE>(core_, 2);
    const auto ApcArgument1         = arg<nt::PVOID>(core_, 3);
    const auto ApcArgument2         = arg<nt::PVOID>(core_, 4);
    const auto ApcArgument3         = arg<nt::PVOID>(core_, 5);

    for(const auto& it : d_->observers_NtQueueApcThreadEx)
        it(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

bool monitor::GenericMonitor::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueueApcThread");
    if(!ok)
        FAIL(false, "Unable to register NtQueueApcThread");

    d_->observers_NtQueueApcThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueueApcThread()
{
    if(false)
        LOG(INFO, "break on NtQueueApcThread");

    const auto ThreadHandle = arg<nt::HANDLE>(core_, 0);
    const auto ApcRoutine   = arg<nt::PPS_APC_ROUTINE>(core_, 1);
    const auto ApcArgument1 = arg<nt::PVOID>(core_, 2);
    const auto ApcArgument2 = arg<nt::PVOID>(core_, 3);
    const auto ApcArgument3 = arg<nt::PVOID>(core_, 4);

    for(const auto& it : d_->observers_NtQueueApcThread)
        it(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

bool monitor::GenericMonitor::register_NtRaiseException(proc_t proc, const on_NtRaiseException_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRaiseException");
    if(!ok)
        FAIL(false, "Unable to register NtRaiseException");

    d_->observers_NtRaiseException.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRaiseException()
{
    if(false)
        LOG(INFO, "break on NtRaiseException");

    const auto ExceptionRecord = arg<nt::PEXCEPTION_RECORD>(core_, 0);
    const auto ContextRecord   = arg<nt::PCONTEXT>(core_, 1);
    const auto FirstChance     = arg<nt::BOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtRaiseException)
        it(ExceptionRecord, ContextRecord, FirstChance);
}

bool monitor::GenericMonitor::register_NtRaiseHardError(proc_t proc, const on_NtRaiseHardError_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRaiseHardError");
    if(!ok)
        FAIL(false, "Unable to register NtRaiseHardError");

    d_->observers_NtRaiseHardError.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRaiseHardError()
{
    if(false)
        LOG(INFO, "break on NtRaiseHardError");

    const auto ErrorStatus                = arg<nt::NTSTATUS>(core_, 0);
    const auto NumberOfParameters         = arg<nt::ULONG>(core_, 1);
    const auto UnicodeStringParameterMask = arg<nt::ULONG>(core_, 2);
    const auto Parameters                 = arg<nt::PULONG_PTR>(core_, 3);
    const auto ValidResponseOptions       = arg<nt::ULONG>(core_, 4);
    const auto Response                   = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtRaiseHardError)
        it(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
}

bool monitor::GenericMonitor::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReadFile");
    if(!ok)
        FAIL(false, "Unable to register NtReadFile");

    d_->observers_NtReadFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReadFile()
{
    if(false)
        LOG(INFO, "break on NtReadFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Event         = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext    = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto Buffer        = arg<nt::PVOID>(core_, 5);
    const auto Length        = arg<nt::ULONG>(core_, 6);
    const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core_, 7);
    const auto Key           = arg<nt::PULONG>(core_, 8);

    for(const auto& it : d_->observers_NtReadFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReadFileScatter");
    if(!ok)
        FAIL(false, "Unable to register NtReadFileScatter");

    d_->observers_NtReadFileScatter.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReadFileScatter()
{
    if(false)
        LOG(INFO, "break on NtReadFileScatter");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Event         = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext    = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(core_, 5);
    const auto Length        = arg<nt::ULONG>(core_, 6);
    const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core_, 7);
    const auto Key           = arg<nt::PULONG>(core_, 8);

    for(const auto& it : d_->observers_NtReadFileScatter)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtReadOnlyEnlistment(proc_t proc, const on_NtReadOnlyEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReadOnlyEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtReadOnlyEnlistment");

    d_->observers_NtReadOnlyEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReadOnlyEnlistment()
{
    if(false)
        LOG(INFO, "break on NtReadOnlyEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtReadOnlyEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtReadRequestData(proc_t proc, const on_NtReadRequestData_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReadRequestData");
    if(!ok)
        FAIL(false, "Unable to register NtReadRequestData");

    d_->observers_NtReadRequestData.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReadRequestData()
{
    if(false)
        LOG(INFO, "break on NtReadRequestData");

    const auto PortHandle        = arg<nt::HANDLE>(core_, 0);
    const auto Message           = arg<nt::PPORT_MESSAGE>(core_, 1);
    const auto DataEntryIndex    = arg<nt::ULONG>(core_, 2);
    const auto Buffer            = arg<nt::PVOID>(core_, 3);
    const auto BufferSize        = arg<nt::SIZE_T>(core_, 4);
    const auto NumberOfBytesRead = arg<nt::PSIZE_T>(core_, 5);

    for(const auto& it : d_->observers_NtReadRequestData)
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
}

bool monitor::GenericMonitor::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReadVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtReadVirtualMemory");

    d_->observers_NtReadVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReadVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtReadVirtualMemory");

    const auto ProcessHandle     = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress       = arg<nt::PVOID>(core_, 1);
    const auto Buffer            = arg<nt::PVOID>(core_, 2);
    const auto BufferSize        = arg<nt::SIZE_T>(core_, 3);
    const auto NumberOfBytesRead = arg<nt::PSIZE_T>(core_, 4);

    for(const auto& it : d_->observers_NtReadVirtualMemory)
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
}

bool monitor::GenericMonitor::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRecoverEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtRecoverEnlistment");

    d_->observers_NtRecoverEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverEnlistment()
{
    if(false)
        LOG(INFO, "break on NtRecoverEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto EnlistmentKey    = arg<nt::PVOID>(core_, 1);

    for(const auto& it : d_->observers_NtRecoverEnlistment)
        it(EnlistmentHandle, EnlistmentKey);
}

bool monitor::GenericMonitor::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRecoverResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtRecoverResourceManager");

    d_->observers_NtRecoverResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverResourceManager()
{
    if(false)
        LOG(INFO, "break on NtRecoverResourceManager");

    const auto ResourceManagerHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtRecoverResourceManager)
        it(ResourceManagerHandle);
}

bool monitor::GenericMonitor::register_NtRecoverTransactionManager(proc_t proc, const on_NtRecoverTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRecoverTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtRecoverTransactionManager");

    d_->observers_NtRecoverTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtRecoverTransactionManager");

    const auto TransactionManagerHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtRecoverTransactionManager)
        it(TransactionManagerHandle);
}

bool monitor::GenericMonitor::register_NtRegisterProtocolAddressInformation(proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRegisterProtocolAddressInformation");
    if(!ok)
        FAIL(false, "Unable to register NtRegisterProtocolAddressInformation");

    d_->observers_NtRegisterProtocolAddressInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRegisterProtocolAddressInformation()
{
    if(false)
        LOG(INFO, "break on NtRegisterProtocolAddressInformation");

    const auto ResourceManager         = arg<nt::HANDLE>(core_, 0);
    const auto ProtocolId              = arg<nt::PCRM_PROTOCOL_ID>(core_, 1);
    const auto ProtocolInformationSize = arg<nt::ULONG>(core_, 2);
    const auto ProtocolInformation     = arg<nt::PVOID>(core_, 3);
    const auto CreateOptions           = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtRegisterProtocolAddressInformation)
        it(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
}

bool monitor::GenericMonitor::register_NtRegisterThreadTerminatePort(proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRegisterThreadTerminatePort");
    if(!ok)
        FAIL(false, "Unable to register NtRegisterThreadTerminatePort");

    d_->observers_NtRegisterThreadTerminatePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRegisterThreadTerminatePort()
{
    if(false)
        LOG(INFO, "break on NtRegisterThreadTerminatePort");

    const auto PortHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtRegisterThreadTerminatePort)
        it(PortHandle);
}

bool monitor::GenericMonitor::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReleaseKeyedEvent");
    if(!ok)
        FAIL(false, "Unable to register NtReleaseKeyedEvent");

    d_->observers_NtReleaseKeyedEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseKeyedEvent()
{
    if(false)
        LOG(INFO, "break on NtReleaseKeyedEvent");

    const auto KeyedEventHandle = arg<nt::HANDLE>(core_, 0);
    const auto KeyValue         = arg<nt::PVOID>(core_, 1);
    const auto Alertable        = arg<nt::BOOLEAN>(core_, 2);
    const auto Timeout          = arg<nt::PLARGE_INTEGER>(core_, 3);

    for(const auto& it : d_->observers_NtReleaseKeyedEvent)
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtReleaseMutant(proc_t proc, const on_NtReleaseMutant_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReleaseMutant");
    if(!ok)
        FAIL(false, "Unable to register NtReleaseMutant");

    d_->observers_NtReleaseMutant.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseMutant()
{
    if(false)
        LOG(INFO, "break on NtReleaseMutant");

    const auto MutantHandle  = arg<nt::HANDLE>(core_, 0);
    const auto PreviousCount = arg<nt::PLONG>(core_, 1);

    for(const auto& it : d_->observers_NtReleaseMutant)
        it(MutantHandle, PreviousCount);
}

bool monitor::GenericMonitor::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReleaseSemaphore");
    if(!ok)
        FAIL(false, "Unable to register NtReleaseSemaphore");

    d_->observers_NtReleaseSemaphore.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseSemaphore()
{
    if(false)
        LOG(INFO, "break on NtReleaseSemaphore");

    const auto SemaphoreHandle = arg<nt::HANDLE>(core_, 0);
    const auto ReleaseCount    = arg<nt::LONG>(core_, 1);
    const auto PreviousCount   = arg<nt::PLONG>(core_, 2);

    for(const auto& it : d_->observers_NtReleaseSemaphore)
        it(SemaphoreHandle, ReleaseCount, PreviousCount);
}

bool monitor::GenericMonitor::register_NtReleaseWorkerFactoryWorker(proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReleaseWorkerFactoryWorker");
    if(!ok)
        FAIL(false, "Unable to register NtReleaseWorkerFactoryWorker");

    d_->observers_NtReleaseWorkerFactoryWorker.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseWorkerFactoryWorker()
{
    if(false)
        LOG(INFO, "break on NtReleaseWorkerFactoryWorker");

    const auto WorkerFactoryHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtReleaseWorkerFactoryWorker)
        it(WorkerFactoryHandle);
}

bool monitor::GenericMonitor::register_NtRemoveIoCompletionEx(proc_t proc, const on_NtRemoveIoCompletionEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRemoveIoCompletionEx");
    if(!ok)
        FAIL(false, "Unable to register NtRemoveIoCompletionEx");

    d_->observers_NtRemoveIoCompletionEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveIoCompletionEx()
{
    if(false)
        LOG(INFO, "break on NtRemoveIoCompletionEx");

    const auto IoCompletionHandle      = arg<nt::HANDLE>(core_, 0);
    const auto IoCompletionInformation = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(core_, 1);
    const auto Count                   = arg<nt::ULONG>(core_, 2);
    const auto NumEntriesRemoved       = arg<nt::PULONG>(core_, 3);
    const auto Timeout                 = arg<nt::PLARGE_INTEGER>(core_, 4);
    const auto Alertable               = arg<nt::BOOLEAN>(core_, 5);

    for(const auto& it : d_->observers_NtRemoveIoCompletionEx)
        it(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
}

bool monitor::GenericMonitor::register_NtRemoveIoCompletion(proc_t proc, const on_NtRemoveIoCompletion_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRemoveIoCompletion");
    if(!ok)
        FAIL(false, "Unable to register NtRemoveIoCompletion");

    d_->observers_NtRemoveIoCompletion.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveIoCompletion()
{
    if(false)
        LOG(INFO, "break on NtRemoveIoCompletion");

    const auto IoCompletionHandle = arg<nt::HANDLE>(core_, 0);
    const auto STARKeyContext     = arg<nt::PVOID>(core_, 1);
    const auto STARApcContext     = arg<nt::PVOID>(core_, 2);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 3);
    const auto Timeout            = arg<nt::PLARGE_INTEGER>(core_, 4);

    for(const auto& it : d_->observers_NtRemoveIoCompletion)
        it(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
}

bool monitor::GenericMonitor::register_NtRemoveProcessDebug(proc_t proc, const on_NtRemoveProcessDebug_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRemoveProcessDebug");
    if(!ok)
        FAIL(false, "Unable to register NtRemoveProcessDebug");

    d_->observers_NtRemoveProcessDebug.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveProcessDebug()
{
    if(false)
        LOG(INFO, "break on NtRemoveProcessDebug");

    const auto ProcessHandle     = arg<nt::HANDLE>(core_, 0);
    const auto DebugObjectHandle = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtRemoveProcessDebug)
        it(ProcessHandle, DebugObjectHandle);
}

bool monitor::GenericMonitor::register_NtRenameKey(proc_t proc, const on_NtRenameKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRenameKey");
    if(!ok)
        FAIL(false, "Unable to register NtRenameKey");

    d_->observers_NtRenameKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRenameKey()
{
    if(false)
        LOG(INFO, "break on NtRenameKey");

    const auto KeyHandle = arg<nt::HANDLE>(core_, 0);
    const auto NewName   = arg<nt::PUNICODE_STRING>(core_, 1);

    for(const auto& it : d_->observers_NtRenameKey)
        it(KeyHandle, NewName);
}

bool monitor::GenericMonitor::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRenameTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtRenameTransactionManager");

    d_->observers_NtRenameTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRenameTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtRenameTransactionManager");

    const auto LogFileName                    = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto ExistingTransactionManagerGuid = arg<nt::LPGUID>(core_, 1);

    for(const auto& it : d_->observers_NtRenameTransactionManager)
        it(LogFileName, ExistingTransactionManagerGuid);
}

bool monitor::GenericMonitor::register_NtReplaceKey(proc_t proc, const on_NtReplaceKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplaceKey");
    if(!ok)
        FAIL(false, "Unable to register NtReplaceKey");

    d_->observers_NtReplaceKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplaceKey()
{
    if(false)
        LOG(INFO, "break on NtReplaceKey");

    const auto NewFile      = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto TargetHandle = arg<nt::HANDLE>(core_, 1);
    const auto OldFile      = arg<nt::POBJECT_ATTRIBUTES>(core_, 2);

    for(const auto& it : d_->observers_NtReplaceKey)
        it(NewFile, TargetHandle, OldFile);
}

bool monitor::GenericMonitor::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplacePartitionUnit");
    if(!ok)
        FAIL(false, "Unable to register NtReplacePartitionUnit");

    d_->observers_NtReplacePartitionUnit.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplacePartitionUnit()
{
    if(false)
        LOG(INFO, "break on NtReplacePartitionUnit");

    const auto TargetInstancePath = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto SpareInstancePath  = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto Flags              = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtReplacePartitionUnit)
        it(TargetInstancePath, SpareInstancePath, Flags);
}

bool monitor::GenericMonitor::register_NtReplyPort(proc_t proc, const on_NtReplyPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplyPort");
    if(!ok)
        FAIL(false, "Unable to register NtReplyPort");

    d_->observers_NtReplyPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplyPort()
{
    if(false)
        LOG(INFO, "break on NtReplyPort");

    const auto PortHandle   = arg<nt::HANDLE>(core_, 0);
    const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(core_, 1);

    for(const auto& it : d_->observers_NtReplyPort)
        it(PortHandle, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplyWaitReceivePortEx");
    if(!ok)
        FAIL(false, "Unable to register NtReplyWaitReceivePortEx");

    d_->observers_NtReplyWaitReceivePortEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePortEx()
{
    if(false)
        LOG(INFO, "break on NtReplyWaitReceivePortEx");

    const auto PortHandle      = arg<nt::HANDLE>(core_, 0);
    const auto STARPortContext = arg<nt::PVOID>(core_, 1);
    const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(core_, 3);
    const auto Timeout         = arg<nt::PLARGE_INTEGER>(core_, 4);

    for(const auto& it : d_->observers_NtReplyWaitReceivePortEx)
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
}

bool monitor::GenericMonitor::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplyWaitReceivePort");
    if(!ok)
        FAIL(false, "Unable to register NtReplyWaitReceivePort");

    d_->observers_NtReplyWaitReceivePort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePort()
{
    if(false)
        LOG(INFO, "break on NtReplyWaitReceivePort");

    const auto PortHandle      = arg<nt::HANDLE>(core_, 0);
    const auto STARPortContext = arg<nt::PVOID>(core_, 1);
    const auto ReplyMessage    = arg<nt::PPORT_MESSAGE>(core_, 2);
    const auto ReceiveMessage  = arg<nt::PPORT_MESSAGE>(core_, 3);

    for(const auto& it : d_->observers_NtReplyWaitReceivePort)
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
}

bool monitor::GenericMonitor::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtReplyWaitReplyPort");
    if(!ok)
        FAIL(false, "Unable to register NtReplyWaitReplyPort");

    d_->observers_NtReplyWaitReplyPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReplyPort()
{
    if(false)
        LOG(INFO, "break on NtReplyWaitReplyPort");

    const auto PortHandle   = arg<nt::HANDLE>(core_, 0);
    const auto ReplyMessage = arg<nt::PPORT_MESSAGE>(core_, 1);

    for(const auto& it : d_->observers_NtReplyWaitReplyPort)
        it(PortHandle, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtRequestPort(proc_t proc, const on_NtRequestPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRequestPort");
    if(!ok)
        FAIL(false, "Unable to register NtRequestPort");

    d_->observers_NtRequestPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRequestPort()
{
    if(false)
        LOG(INFO, "break on NtRequestPort");

    const auto PortHandle     = arg<nt::HANDLE>(core_, 0);
    const auto RequestMessage = arg<nt::PPORT_MESSAGE>(core_, 1);

    for(const auto& it : d_->observers_NtRequestPort)
        it(PortHandle, RequestMessage);
}

bool monitor::GenericMonitor::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRequestWaitReplyPort");
    if(!ok)
        FAIL(false, "Unable to register NtRequestWaitReplyPort");

    d_->observers_NtRequestWaitReplyPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRequestWaitReplyPort()
{
    if(false)
        LOG(INFO, "break on NtRequestWaitReplyPort");

    const auto PortHandle     = arg<nt::HANDLE>(core_, 0);
    const auto RequestMessage = arg<nt::PPORT_MESSAGE>(core_, 1);
    const auto ReplyMessage   = arg<nt::PPORT_MESSAGE>(core_, 2);

    for(const auto& it : d_->observers_NtRequestWaitReplyPort)
        it(PortHandle, RequestMessage, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtResetEvent");
    if(!ok)
        FAIL(false, "Unable to register NtResetEvent");

    d_->observers_NtResetEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtResetEvent()
{
    if(false)
        LOG(INFO, "break on NtResetEvent");

    const auto EventHandle   = arg<nt::HANDLE>(core_, 0);
    const auto PreviousState = arg<nt::PLONG>(core_, 1);

    for(const auto& it : d_->observers_NtResetEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtResetWriteWatch(proc_t proc, const on_NtResetWriteWatch_fn& on_func)
{
    const auto ok = setup_func(proc, "NtResetWriteWatch");
    if(!ok)
        FAIL(false, "Unable to register NtResetWriteWatch");

    d_->observers_NtResetWriteWatch.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtResetWriteWatch()
{
    if(false)
        LOG(INFO, "break on NtResetWriteWatch");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress   = arg<nt::PVOID>(core_, 1);
    const auto RegionSize    = arg<nt::SIZE_T>(core_, 2);

    for(const auto& it : d_->observers_NtResetWriteWatch)
        it(ProcessHandle, BaseAddress, RegionSize);
}

bool monitor::GenericMonitor::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRestoreKey");
    if(!ok)
        FAIL(false, "Unable to register NtRestoreKey");

    d_->observers_NtRestoreKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRestoreKey()
{
    if(false)
        LOG(INFO, "break on NtRestoreKey");

    const auto KeyHandle  = arg<nt::HANDLE>(core_, 0);
    const auto FileHandle = arg<nt::HANDLE>(core_, 1);
    const auto Flags      = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtRestoreKey)
        it(KeyHandle, FileHandle, Flags);
}

bool monitor::GenericMonitor::register_NtResumeProcess(proc_t proc, const on_NtResumeProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtResumeProcess");
    if(!ok)
        FAIL(false, "Unable to register NtResumeProcess");

    d_->observers_NtResumeProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtResumeProcess()
{
    if(false)
        LOG(INFO, "break on NtResumeProcess");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtResumeProcess)
        it(ProcessHandle);
}

bool monitor::GenericMonitor::register_NtResumeThread(proc_t proc, const on_NtResumeThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtResumeThread");
    if(!ok)
        FAIL(false, "Unable to register NtResumeThread");

    d_->observers_NtResumeThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtResumeThread()
{
    if(false)
        LOG(INFO, "break on NtResumeThread");

    const auto ThreadHandle         = arg<nt::HANDLE>(core_, 0);
    const auto PreviousSuspendCount = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtResumeThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtRollbackComplete(proc_t proc, const on_NtRollbackComplete_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRollbackComplete");
    if(!ok)
        FAIL(false, "Unable to register NtRollbackComplete");

    d_->observers_NtRollbackComplete.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackComplete()
{
    if(false)
        LOG(INFO, "break on NtRollbackComplete");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtRollbackComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRollbackEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtRollbackEnlistment");

    d_->observers_NtRollbackEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackEnlistment()
{
    if(false)
        LOG(INFO, "break on NtRollbackEnlistment");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtRollbackEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRollbackTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtRollbackTransaction");

    d_->observers_NtRollbackTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackTransaction()
{
    if(false)
        LOG(INFO, "break on NtRollbackTransaction");

    const auto TransactionHandle = arg<nt::HANDLE>(core_, 0);
    const auto Wait              = arg<nt::BOOLEAN>(core_, 1);

    for(const auto& it : d_->observers_NtRollbackTransaction)
        it(TransactionHandle, Wait);
}

bool monitor::GenericMonitor::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtRollforwardTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtRollforwardTransactionManager");

    d_->observers_NtRollforwardTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtRollforwardTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtRollforwardTransactionManager");

    const auto TransactionManagerHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock           = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtRollforwardTransactionManager)
        it(TransactionManagerHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSaveKeyEx");
    if(!ok)
        FAIL(false, "Unable to register NtSaveKeyEx");

    d_->observers_NtSaveKeyEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSaveKeyEx()
{
    if(false)
        LOG(INFO, "break on NtSaveKeyEx");

    const auto KeyHandle  = arg<nt::HANDLE>(core_, 0);
    const auto FileHandle = arg<nt::HANDLE>(core_, 1);
    const auto Format     = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtSaveKeyEx)
        it(KeyHandle, FileHandle, Format);
}

bool monitor::GenericMonitor::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSaveKey");
    if(!ok)
        FAIL(false, "Unable to register NtSaveKey");

    d_->observers_NtSaveKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSaveKey()
{
    if(false)
        LOG(INFO, "break on NtSaveKey");

    const auto KeyHandle  = arg<nt::HANDLE>(core_, 0);
    const auto FileHandle = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtSaveKey)
        it(KeyHandle, FileHandle);
}

bool monitor::GenericMonitor::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSaveMergedKeys");
    if(!ok)
        FAIL(false, "Unable to register NtSaveMergedKeys");

    d_->observers_NtSaveMergedKeys.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSaveMergedKeys()
{
    if(false)
        LOG(INFO, "break on NtSaveMergedKeys");

    const auto HighPrecedenceKeyHandle = arg<nt::HANDLE>(core_, 0);
    const auto LowPrecedenceKeyHandle  = arg<nt::HANDLE>(core_, 1);
    const auto FileHandle              = arg<nt::HANDLE>(core_, 2);

    for(const auto& it : d_->observers_NtSaveMergedKeys)
        it(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
}

bool monitor::GenericMonitor::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSecureConnectPort");
    if(!ok)
        FAIL(false, "Unable to register NtSecureConnectPort");

    d_->observers_NtSecureConnectPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSecureConnectPort()
{
    if(false)
        LOG(INFO, "break on NtSecureConnectPort");

    const auto PortHandle                  = arg<nt::PHANDLE>(core_, 0);
    const auto PortName                    = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto SecurityQos                 = arg<nt::PSECURITY_QUALITY_OF_SERVICE>(core_, 2);
    const auto ClientView                  = arg<nt::PPORT_VIEW>(core_, 3);
    const auto RequiredServerSid           = arg<nt::PSID>(core_, 4);
    const auto ServerView                  = arg<nt::PREMOTE_PORT_VIEW>(core_, 5);
    const auto MaxMessageLength            = arg<nt::PULONG>(core_, 6);
    const auto ConnectionInformation       = arg<nt::PVOID>(core_, 7);
    const auto ConnectionInformationLength = arg<nt::PULONG>(core_, 8);

    for(const auto& it : d_->observers_NtSecureConnectPort)
        it(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
}

bool monitor::GenericMonitor::register_NtSetBootEntryOrder(proc_t proc, const on_NtSetBootEntryOrder_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetBootEntryOrder");
    if(!ok)
        FAIL(false, "Unable to register NtSetBootEntryOrder");

    d_->observers_NtSetBootEntryOrder.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetBootEntryOrder()
{
    if(false)
        LOG(INFO, "break on NtSetBootEntryOrder");

    const auto Ids   = arg<nt::PULONG>(core_, 0);
    const auto Count = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtSetBootEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtSetBootOptions(proc_t proc, const on_NtSetBootOptions_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetBootOptions");
    if(!ok)
        FAIL(false, "Unable to register NtSetBootOptions");

    d_->observers_NtSetBootOptions.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetBootOptions()
{
    if(false)
        LOG(INFO, "break on NtSetBootOptions");

    const auto BootOptions    = arg<nt::PBOOT_OPTIONS>(core_, 0);
    const auto FieldsToChange = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtSetBootOptions)
        it(BootOptions, FieldsToChange);
}

bool monitor::GenericMonitor::register_NtSetContextThread(proc_t proc, const on_NtSetContextThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetContextThread");
    if(!ok)
        FAIL(false, "Unable to register NtSetContextThread");

    d_->observers_NtSetContextThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetContextThread()
{
    if(false)
        LOG(INFO, "break on NtSetContextThread");

    const auto ThreadHandle  = arg<nt::HANDLE>(core_, 0);
    const auto ThreadContext = arg<nt::PCONTEXT>(core_, 1);

    for(const auto& it : d_->observers_NtSetContextThread)
        it(ThreadHandle, ThreadContext);
}

bool monitor::GenericMonitor::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetDebugFilterState");
    if(!ok)
        FAIL(false, "Unable to register NtSetDebugFilterState");

    d_->observers_NtSetDebugFilterState.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetDebugFilterState()
{
    if(false)
        LOG(INFO, "break on NtSetDebugFilterState");

    const auto ComponentId = arg<nt::ULONG>(core_, 0);
    const auto Level       = arg<nt::ULONG>(core_, 1);
    const auto State       = arg<nt::BOOLEAN>(core_, 2);

    for(const auto& it : d_->observers_NtSetDebugFilterState)
        it(ComponentId, Level, State);
}

bool monitor::GenericMonitor::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetDefaultHardErrorPort");
    if(!ok)
        FAIL(false, "Unable to register NtSetDefaultHardErrorPort");

    d_->observers_NtSetDefaultHardErrorPort.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultHardErrorPort()
{
    if(false)
        LOG(INFO, "break on NtSetDefaultHardErrorPort");

    const auto DefaultHardErrorPort = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetDefaultHardErrorPort)
        it(DefaultHardErrorPort);
}

bool monitor::GenericMonitor::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetDefaultLocale");
    if(!ok)
        FAIL(false, "Unable to register NtSetDefaultLocale");

    d_->observers_NtSetDefaultLocale.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultLocale()
{
    if(false)
        LOG(INFO, "break on NtSetDefaultLocale");

    const auto UserProfile     = arg<nt::BOOLEAN>(core_, 0);
    const auto DefaultLocaleId = arg<nt::LCID>(core_, 1);

    for(const auto& it : d_->observers_NtSetDefaultLocale)
        it(UserProfile, DefaultLocaleId);
}

bool monitor::GenericMonitor::register_NtSetDefaultUILanguage(proc_t proc, const on_NtSetDefaultUILanguage_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetDefaultUILanguage");
    if(!ok)
        FAIL(false, "Unable to register NtSetDefaultUILanguage");

    d_->observers_NtSetDefaultUILanguage.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultUILanguage()
{
    if(false)
        LOG(INFO, "break on NtSetDefaultUILanguage");

    const auto DefaultUILanguageId = arg<nt::LANGID>(core_, 0);

    for(const auto& it : d_->observers_NtSetDefaultUILanguage)
        it(DefaultUILanguageId);
}

bool monitor::GenericMonitor::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetDriverEntryOrder");
    if(!ok)
        FAIL(false, "Unable to register NtSetDriverEntryOrder");

    d_->observers_NtSetDriverEntryOrder.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetDriverEntryOrder()
{
    if(false)
        LOG(INFO, "break on NtSetDriverEntryOrder");

    const auto Ids   = arg<nt::PULONG>(core_, 0);
    const auto Count = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtSetDriverEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtSetEaFile(proc_t proc, const on_NtSetEaFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetEaFile");
    if(!ok)
        FAIL(false, "Unable to register NtSetEaFile");

    d_->observers_NtSetEaFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetEaFile()
{
    if(false)
        LOG(INFO, "break on NtSetEaFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto Buffer        = arg<nt::PVOID>(core_, 2);
    const auto Length        = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetEaFile)
        it(FileHandle, IoStatusBlock, Buffer, Length);
}

bool monitor::GenericMonitor::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetEventBoostPriority");
    if(!ok)
        FAIL(false, "Unable to register NtSetEventBoostPriority");

    d_->observers_NtSetEventBoostPriority.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetEventBoostPriority()
{
    if(false)
        LOG(INFO, "break on NtSetEventBoostPriority");

    const auto EventHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetEventBoostPriority)
        it(EventHandle);
}

bool monitor::GenericMonitor::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetEvent");
    if(!ok)
        FAIL(false, "Unable to register NtSetEvent");

    d_->observers_NtSetEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetEvent()
{
    if(false)
        LOG(INFO, "break on NtSetEvent");

    const auto EventHandle   = arg<nt::HANDLE>(core_, 0);
    const auto PreviousState = arg<nt::PLONG>(core_, 1);

    for(const auto& it : d_->observers_NtSetEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetHighEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtSetHighEventPair");

    d_->observers_NtSetHighEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetHighEventPair()
{
    if(false)
        LOG(INFO, "break on NtSetHighEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetHighWaitLowEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtSetHighWaitLowEventPair");

    d_->observers_NtSetHighWaitLowEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetHighWaitLowEventPair()
{
    if(false)
        LOG(INFO, "break on NtSetHighWaitLowEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetHighWaitLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetInformationDebugObject(proc_t proc, const on_NtSetInformationDebugObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationDebugObject");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationDebugObject");

    d_->observers_NtSetInformationDebugObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationDebugObject()
{
    if(false)
        LOG(INFO, "break on NtSetInformationDebugObject");

    const auto DebugObjectHandle           = arg<nt::HANDLE>(core_, 0);
    const auto DebugObjectInformationClass = arg<nt::DEBUGOBJECTINFOCLASS>(core_, 1);
    const auto DebugInformation            = arg<nt::PVOID>(core_, 2);
    const auto DebugInformationLength      = arg<nt::ULONG>(core_, 3);
    const auto ReturnLength                = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_NtSetInformationDebugObject)
        it(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationEnlistment");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationEnlistment");

    d_->observers_NtSetInformationEnlistment.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationEnlistment()
{
    if(false)
        LOG(INFO, "break on NtSetInformationEnlistment");

    const auto EnlistmentHandle            = arg<nt::HANDLE>(core_, 0);
    const auto EnlistmentInformationClass  = arg<nt::ENLISTMENT_INFORMATION_CLASS>(core_, 1);
    const auto EnlistmentInformation       = arg<nt::PVOID>(core_, 2);
    const auto EnlistmentInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationEnlistment)
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationFile(proc_t proc, const on_NtSetInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationFile");

    d_->observers_NtSetInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationFile()
{
    if(false)
        LOG(INFO, "break on NtSetInformationFile");

    const auto FileHandle           = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock        = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto FileInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length               = arg<nt::ULONG>(core_, 3);
    const auto FileInformationClass = arg<nt::FILE_INFORMATION_CLASS>(core_, 4);

    for(const auto& it : d_->observers_NtSetInformationFile)
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

bool monitor::GenericMonitor::register_NtSetInformationJobObject(proc_t proc, const on_NtSetInformationJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationJobObject");

    d_->observers_NtSetInformationJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationJobObject()
{
    if(false)
        LOG(INFO, "break on NtSetInformationJobObject");

    const auto JobHandle                  = arg<nt::HANDLE>(core_, 0);
    const auto JobObjectInformationClass  = arg<nt::JOBOBJECTINFOCLASS>(core_, 1);
    const auto JobObjectInformation       = arg<nt::PVOID>(core_, 2);
    const auto JobObjectInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationJobObject)
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationKey(proc_t proc, const on_NtSetInformationKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationKey");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationKey");

    d_->observers_NtSetInformationKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationKey()
{
    if(false)
        LOG(INFO, "break on NtSetInformationKey");

    const auto KeyHandle               = arg<nt::HANDLE>(core_, 0);
    const auto KeySetInformationClass  = arg<nt::KEY_SET_INFORMATION_CLASS>(core_, 1);
    const auto KeySetInformation       = arg<nt::PVOID>(core_, 2);
    const auto KeySetInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationKey)
        it(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationObject(proc_t proc, const on_NtSetInformationObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationObject");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationObject");

    d_->observers_NtSetInformationObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationObject()
{
    if(false)
        LOG(INFO, "break on NtSetInformationObject");

    const auto Handle                  = arg<nt::HANDLE>(core_, 0);
    const auto ObjectInformationClass  = arg<nt::OBJECT_INFORMATION_CLASS>(core_, 1);
    const auto ObjectInformation       = arg<nt::PVOID>(core_, 2);
    const auto ObjectInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationObject)
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationProcess(proc_t proc, const on_NtSetInformationProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationProcess");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationProcess");

    d_->observers_NtSetInformationProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationProcess()
{
    if(false)
        LOG(INFO, "break on NtSetInformationProcess");

    const auto ProcessHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ProcessInformationClass  = arg<nt::PROCESSINFOCLASS>(core_, 1);
    const auto ProcessInformation       = arg<nt::PVOID>(core_, 2);
    const auto ProcessInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationProcess)
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationResourceManager(proc_t proc, const on_NtSetInformationResourceManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationResourceManager");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationResourceManager");

    d_->observers_NtSetInformationResourceManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationResourceManager()
{
    if(false)
        LOG(INFO, "break on NtSetInformationResourceManager");

    const auto ResourceManagerHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ResourceManagerInformationClass  = arg<nt::RESOURCEMANAGER_INFORMATION_CLASS>(core_, 1);
    const auto ResourceManagerInformation       = arg<nt::PVOID>(core_, 2);
    const auto ResourceManagerInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationResourceManager)
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationThread(proc_t proc, const on_NtSetInformationThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationThread");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationThread");

    d_->observers_NtSetInformationThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationThread()
{
    if(false)
        LOG(INFO, "break on NtSetInformationThread");

    const auto ThreadHandle            = arg<nt::HANDLE>(core_, 0);
    const auto ThreadInformationClass  = arg<nt::THREADINFOCLASS>(core_, 1);
    const auto ThreadInformation       = arg<nt::PVOID>(core_, 2);
    const auto ThreadInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationThread)
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationToken(proc_t proc, const on_NtSetInformationToken_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationToken");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationToken");

    d_->observers_NtSetInformationToken.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationToken()
{
    if(false)
        LOG(INFO, "break on NtSetInformationToken");

    const auto TokenHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TokenInformationClass  = arg<nt::TOKEN_INFORMATION_CLASS>(core_, 1);
    const auto TokenInformation       = arg<nt::PVOID>(core_, 2);
    const auto TokenInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationToken)
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationTransaction(proc_t proc, const on_NtSetInformationTransaction_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationTransaction");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationTransaction");

    d_->observers_NtSetInformationTransaction.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationTransaction()
{
    if(false)
        LOG(INFO, "break on NtSetInformationTransaction");

    const auto TransactionHandle            = arg<nt::HANDLE>(core_, 0);
    const auto TransactionInformationClass  = arg<nt::TRANSACTION_INFORMATION_CLASS>(core_, 1);
    const auto TransactionInformation       = arg<nt::PVOID>(core_, 2);
    const auto TransactionInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationTransaction)
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationTransactionManager(proc_t proc, const on_NtSetInformationTransactionManager_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationTransactionManager");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationTransactionManager");

    d_->observers_NtSetInformationTransactionManager.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationTransactionManager()
{
    if(false)
        LOG(INFO, "break on NtSetInformationTransactionManager");

    const auto TmHandle                            = arg<nt::HANDLE>(core_, 0);
    const auto TransactionManagerInformationClass  = arg<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(core_, 1);
    const auto TransactionManagerInformation       = arg<nt::PVOID>(core_, 2);
    const auto TransactionManagerInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationTransactionManager)
        it(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationWorkerFactory(proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetInformationWorkerFactory");
    if(!ok)
        FAIL(false, "Unable to register NtSetInformationWorkerFactory");

    d_->observers_NtSetInformationWorkerFactory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationWorkerFactory()
{
    if(false)
        LOG(INFO, "break on NtSetInformationWorkerFactory");

    const auto WorkerFactoryHandle            = arg<nt::HANDLE>(core_, 0);
    const auto WorkerFactoryInformationClass  = arg<nt::WORKERFACTORYINFOCLASS>(core_, 1);
    const auto WorkerFactoryInformation       = arg<nt::PVOID>(core_, 2);
    const auto WorkerFactoryInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetInformationWorkerFactory)
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
}

bool monitor::GenericMonitor::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetIntervalProfile");
    if(!ok)
        FAIL(false, "Unable to register NtSetIntervalProfile");

    d_->observers_NtSetIntervalProfile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetIntervalProfile()
{
    if(false)
        LOG(INFO, "break on NtSetIntervalProfile");

    const auto Interval = arg<nt::ULONG>(core_, 0);
    const auto Source   = arg<nt::KPROFILE_SOURCE>(core_, 1);

    for(const auto& it : d_->observers_NtSetIntervalProfile)
        it(Interval, Source);
}

bool monitor::GenericMonitor::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetIoCompletionEx");
    if(!ok)
        FAIL(false, "Unable to register NtSetIoCompletionEx");

    d_->observers_NtSetIoCompletionEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetIoCompletionEx()
{
    if(false)
        LOG(INFO, "break on NtSetIoCompletionEx");

    const auto IoCompletionHandle        = arg<nt::HANDLE>(core_, 0);
    const auto IoCompletionReserveHandle = arg<nt::HANDLE>(core_, 1);
    const auto KeyContext                = arg<nt::PVOID>(core_, 2);
    const auto ApcContext                = arg<nt::PVOID>(core_, 3);
    const auto IoStatus                  = arg<nt::NTSTATUS>(core_, 4);
    const auto IoStatusInformation       = arg<nt::ULONG_PTR>(core_, 5);

    for(const auto& it : d_->observers_NtSetIoCompletionEx)
        it(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
}

bool monitor::GenericMonitor::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetIoCompletion");
    if(!ok)
        FAIL(false, "Unable to register NtSetIoCompletion");

    d_->observers_NtSetIoCompletion.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetIoCompletion()
{
    if(false)
        LOG(INFO, "break on NtSetIoCompletion");

    const auto IoCompletionHandle  = arg<nt::HANDLE>(core_, 0);
    const auto KeyContext          = arg<nt::PVOID>(core_, 1);
    const auto ApcContext          = arg<nt::PVOID>(core_, 2);
    const auto IoStatus            = arg<nt::NTSTATUS>(core_, 3);
    const auto IoStatusInformation = arg<nt::ULONG_PTR>(core_, 4);

    for(const auto& it : d_->observers_NtSetIoCompletion)
        it(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
}

bool monitor::GenericMonitor::register_NtSetLdtEntries(proc_t proc, const on_NtSetLdtEntries_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetLdtEntries");
    if(!ok)
        FAIL(false, "Unable to register NtSetLdtEntries");

    d_->observers_NtSetLdtEntries.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetLdtEntries()
{
    if(false)
        LOG(INFO, "break on NtSetLdtEntries");

    const auto Selector0 = arg<nt::ULONG>(core_, 0);
    const auto Entry0Low = arg<nt::ULONG>(core_, 1);
    const auto Entry0Hi  = arg<nt::ULONG>(core_, 2);
    const auto Selector1 = arg<nt::ULONG>(core_, 3);
    const auto Entry1Low = arg<nt::ULONG>(core_, 4);
    const auto Entry1Hi  = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtSetLdtEntries)
        it(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
}

bool monitor::GenericMonitor::register_NtSetLowEventPair(proc_t proc, const on_NtSetLowEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetLowEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtSetLowEventPair");

    d_->observers_NtSetLowEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetLowEventPair()
{
    if(false)
        LOG(INFO, "break on NtSetLowEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetLowWaitHighEventPair(proc_t proc, const on_NtSetLowWaitHighEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetLowWaitHighEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtSetLowWaitHighEventPair");

    d_->observers_NtSetLowWaitHighEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetLowWaitHighEventPair()
{
    if(false)
        LOG(INFO, "break on NtSetLowWaitHighEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSetLowWaitHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetQuotaInformationFile(proc_t proc, const on_NtSetQuotaInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetQuotaInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtSetQuotaInformationFile");

    d_->observers_NtSetQuotaInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetQuotaInformationFile()
{
    if(false)
        LOG(INFO, "break on NtSetQuotaInformationFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto Buffer        = arg<nt::PVOID>(core_, 2);
    const auto Length        = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetQuotaInformationFile)
        it(FileHandle, IoStatusBlock, Buffer, Length);
}

bool monitor::GenericMonitor::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSecurityObject");
    if(!ok)
        FAIL(false, "Unable to register NtSetSecurityObject");

    d_->observers_NtSetSecurityObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSecurityObject()
{
    if(false)
        LOG(INFO, "break on NtSetSecurityObject");

    const auto Handle              = arg<nt::HANDLE>(core_, 0);
    const auto SecurityInformation = arg<nt::SECURITY_INFORMATION>(core_, 1);
    const auto SecurityDescriptor  = arg<nt::PSECURITY_DESCRIPTOR>(core_, 2);

    for(const auto& it : d_->observers_NtSetSecurityObject)
        it(Handle, SecurityInformation, SecurityDescriptor);
}

bool monitor::GenericMonitor::register_NtSetSystemEnvironmentValueEx(proc_t proc, const on_NtSetSystemEnvironmentValueEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSystemEnvironmentValueEx");
    if(!ok)
        FAIL(false, "Unable to register NtSetSystemEnvironmentValueEx");

    d_->observers_NtSetSystemEnvironmentValueEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValueEx()
{
    if(false)
        LOG(INFO, "break on NtSetSystemEnvironmentValueEx");

    const auto VariableName = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto VendorGuid   = arg<nt::LPGUID>(core_, 1);
    const auto Value        = arg<nt::PVOID>(core_, 2);
    const auto ValueLength  = arg<nt::ULONG>(core_, 3);
    const auto Attributes   = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValueEx)
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
}

bool monitor::GenericMonitor::register_NtSetSystemEnvironmentValue(proc_t proc, const on_NtSetSystemEnvironmentValue_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSystemEnvironmentValue");
    if(!ok)
        FAIL(false, "Unable to register NtSetSystemEnvironmentValue");

    d_->observers_NtSetSystemEnvironmentValue.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValue()
{
    if(false)
        LOG(INFO, "break on NtSetSystemEnvironmentValue");

    const auto VariableName  = arg<nt::PUNICODE_STRING>(core_, 0);
    const auto VariableValue = arg<nt::PUNICODE_STRING>(core_, 1);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValue)
        it(VariableName, VariableValue);
}

bool monitor::GenericMonitor::register_NtSetSystemInformation(proc_t proc, const on_NtSetSystemInformation_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSystemInformation");
    if(!ok)
        FAIL(false, "Unable to register NtSetSystemInformation");

    d_->observers_NtSetSystemInformation.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemInformation()
{
    if(false)
        LOG(INFO, "break on NtSetSystemInformation");

    const auto SystemInformationClass  = arg<nt::SYSTEM_INFORMATION_CLASS>(core_, 0);
    const auto SystemInformation       = arg<nt::PVOID>(core_, 1);
    const auto SystemInformationLength = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtSetSystemInformation)
        it(SystemInformationClass, SystemInformation, SystemInformationLength);
}

bool monitor::GenericMonitor::register_NtSetSystemPowerState(proc_t proc, const on_NtSetSystemPowerState_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSystemPowerState");
    if(!ok)
        FAIL(false, "Unable to register NtSetSystemPowerState");

    d_->observers_NtSetSystemPowerState.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemPowerState()
{
    if(false)
        LOG(INFO, "break on NtSetSystemPowerState");

    const auto SystemAction   = arg<nt::POWER_ACTION>(core_, 0);
    const auto MinSystemState = arg<nt::SYSTEM_POWER_STATE>(core_, 1);
    const auto Flags          = arg<nt::ULONG>(core_, 2);

    for(const auto& it : d_->observers_NtSetSystemPowerState)
        it(SystemAction, MinSystemState, Flags);
}

bool monitor::GenericMonitor::register_NtSetSystemTime(proc_t proc, const on_NtSetSystemTime_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetSystemTime");
    if(!ok)
        FAIL(false, "Unable to register NtSetSystemTime");

    d_->observers_NtSetSystemTime.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemTime()
{
    if(false)
        LOG(INFO, "break on NtSetSystemTime");

    const auto SystemTime   = arg<nt::PLARGE_INTEGER>(core_, 0);
    const auto PreviousTime = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtSetSystemTime)
        it(SystemTime, PreviousTime);
}

bool monitor::GenericMonitor::register_NtSetThreadExecutionState(proc_t proc, const on_NtSetThreadExecutionState_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetThreadExecutionState");
    if(!ok)
        FAIL(false, "Unable to register NtSetThreadExecutionState");

    d_->observers_NtSetThreadExecutionState.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetThreadExecutionState()
{
    if(false)
        LOG(INFO, "break on NtSetThreadExecutionState");

    const auto esFlags           = arg<nt::EXECUTION_STATE>(core_, 0);
    const auto STARPreviousFlags = arg<nt::EXECUTION_STATE>(core_, 1);

    for(const auto& it : d_->observers_NtSetThreadExecutionState)
        it(esFlags, STARPreviousFlags);
}

bool monitor::GenericMonitor::register_NtSetTimerEx(proc_t proc, const on_NtSetTimerEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetTimerEx");
    if(!ok)
        FAIL(false, "Unable to register NtSetTimerEx");

    d_->observers_NtSetTimerEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimerEx()
{
    if(false)
        LOG(INFO, "break on NtSetTimerEx");

    const auto TimerHandle               = arg<nt::HANDLE>(core_, 0);
    const auto TimerSetInformationClass  = arg<nt::TIMER_SET_INFORMATION_CLASS>(core_, 1);
    const auto TimerSetInformation       = arg<nt::PVOID>(core_, 2);
    const auto TimerSetInformationLength = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtSetTimerEx)
        it(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
}

bool monitor::GenericMonitor::register_NtSetTimer(proc_t proc, const on_NtSetTimer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetTimer");
    if(!ok)
        FAIL(false, "Unable to register NtSetTimer");

    d_->observers_NtSetTimer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimer()
{
    if(false)
        LOG(INFO, "break on NtSetTimer");

    const auto TimerHandle     = arg<nt::HANDLE>(core_, 0);
    const auto DueTime         = arg<nt::PLARGE_INTEGER>(core_, 1);
    const auto TimerApcRoutine = arg<nt::PTIMER_APC_ROUTINE>(core_, 2);
    const auto TimerContext    = arg<nt::PVOID>(core_, 3);
    const auto WakeTimer       = arg<nt::BOOLEAN>(core_, 4);
    const auto Period          = arg<nt::LONG>(core_, 5);
    const auto PreviousState   = arg<nt::PBOOLEAN>(core_, 6);

    for(const auto& it : d_->observers_NtSetTimer)
        it(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
}

bool monitor::GenericMonitor::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetTimerResolution");
    if(!ok)
        FAIL(false, "Unable to register NtSetTimerResolution");

    d_->observers_NtSetTimerResolution.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimerResolution()
{
    if(false)
        LOG(INFO, "break on NtSetTimerResolution");

    const auto DesiredTime   = arg<nt::ULONG>(core_, 0);
    const auto SetResolution = arg<nt::BOOLEAN>(core_, 1);
    const auto ActualTime    = arg<nt::PULONG>(core_, 2);

    for(const auto& it : d_->observers_NtSetTimerResolution)
        it(DesiredTime, SetResolution, ActualTime);
}

bool monitor::GenericMonitor::register_NtSetUuidSeed(proc_t proc, const on_NtSetUuidSeed_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetUuidSeed");
    if(!ok)
        FAIL(false, "Unable to register NtSetUuidSeed");

    d_->observers_NtSetUuidSeed.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetUuidSeed()
{
    if(false)
        LOG(INFO, "break on NtSetUuidSeed");

    const auto Seed = arg<nt::PCHAR>(core_, 0);

    for(const auto& it : d_->observers_NtSetUuidSeed)
        it(Seed);
}

bool monitor::GenericMonitor::register_NtSetValueKey(proc_t proc, const on_NtSetValueKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetValueKey");
    if(!ok)
        FAIL(false, "Unable to register NtSetValueKey");

    d_->observers_NtSetValueKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetValueKey()
{
    if(false)
        LOG(INFO, "break on NtSetValueKey");

    const auto KeyHandle  = arg<nt::HANDLE>(core_, 0);
    const auto ValueName  = arg<nt::PUNICODE_STRING>(core_, 1);
    const auto TitleIndex = arg<nt::ULONG>(core_, 2);
    const auto Type       = arg<nt::ULONG>(core_, 3);
    const auto Data       = arg<nt::PVOID>(core_, 4);
    const auto DataSize   = arg<nt::ULONG>(core_, 5);

    for(const auto& it : d_->observers_NtSetValueKey)
        it(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
}

bool monitor::GenericMonitor::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSetVolumeInformationFile");
    if(!ok)
        FAIL(false, "Unable to register NtSetVolumeInformationFile");

    d_->observers_NtSetVolumeInformationFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSetVolumeInformationFile()
{
    if(false)
        LOG(INFO, "break on NtSetVolumeInformationFile");

    const auto FileHandle         = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock      = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto FsInformation      = arg<nt::PVOID>(core_, 2);
    const auto Length             = arg<nt::ULONG>(core_, 3);
    const auto FsInformationClass = arg<nt::FS_INFORMATION_CLASS>(core_, 4);

    for(const auto& it : d_->observers_NtSetVolumeInformationFile)
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

bool monitor::GenericMonitor::register_NtShutdownSystem(proc_t proc, const on_NtShutdownSystem_fn& on_func)
{
    const auto ok = setup_func(proc, "NtShutdownSystem");
    if(!ok)
        FAIL(false, "Unable to register NtShutdownSystem");

    d_->observers_NtShutdownSystem.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtShutdownSystem()
{
    if(false)
        LOG(INFO, "break on NtShutdownSystem");

    const auto Action = arg<nt::SHUTDOWN_ACTION>(core_, 0);

    for(const auto& it : d_->observers_NtShutdownSystem)
        it(Action);
}

bool monitor::GenericMonitor::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtShutdownWorkerFactory");
    if(!ok)
        FAIL(false, "Unable to register NtShutdownWorkerFactory");

    d_->observers_NtShutdownWorkerFactory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtShutdownWorkerFactory()
{
    if(false)
        LOG(INFO, "break on NtShutdownWorkerFactory");

    const auto WorkerFactoryHandle    = arg<nt::HANDLE>(core_, 0);
    const auto STARPendingWorkerCount = arg<nt::LONG>(core_, 1);

    for(const auto& it : d_->observers_NtShutdownWorkerFactory)
        it(WorkerFactoryHandle, STARPendingWorkerCount);
}

bool monitor::GenericMonitor::register_NtSignalAndWaitForSingleObject(proc_t proc, const on_NtSignalAndWaitForSingleObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSignalAndWaitForSingleObject");
    if(!ok)
        FAIL(false, "Unable to register NtSignalAndWaitForSingleObject");

    d_->observers_NtSignalAndWaitForSingleObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSignalAndWaitForSingleObject()
{
    if(false)
        LOG(INFO, "break on NtSignalAndWaitForSingleObject");

    const auto SignalHandle = arg<nt::HANDLE>(core_, 0);
    const auto WaitHandle   = arg<nt::HANDLE>(core_, 1);
    const auto Alertable    = arg<nt::BOOLEAN>(core_, 2);
    const auto Timeout      = arg<nt::PLARGE_INTEGER>(core_, 3);

    for(const auto& it : d_->observers_NtSignalAndWaitForSingleObject)
        it(SignalHandle, WaitHandle, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtSinglePhaseReject(proc_t proc, const on_NtSinglePhaseReject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSinglePhaseReject");
    if(!ok)
        FAIL(false, "Unable to register NtSinglePhaseReject");

    d_->observers_NtSinglePhaseReject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSinglePhaseReject()
{
    if(false)
        LOG(INFO, "break on NtSinglePhaseReject");

    const auto EnlistmentHandle = arg<nt::HANDLE>(core_, 0);
    const auto TmVirtualClock   = arg<nt::PLARGE_INTEGER>(core_, 1);

    for(const auto& it : d_->observers_NtSinglePhaseReject)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtStartProfile");
    if(!ok)
        FAIL(false, "Unable to register NtStartProfile");

    d_->observers_NtStartProfile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtStartProfile()
{
    if(false)
        LOG(INFO, "break on NtStartProfile");

    const auto ProfileHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtStartProfile)
        it(ProfileHandle);
}

bool monitor::GenericMonitor::register_NtStopProfile(proc_t proc, const on_NtStopProfile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtStopProfile");
    if(!ok)
        FAIL(false, "Unable to register NtStopProfile");

    d_->observers_NtStopProfile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtStopProfile()
{
    if(false)
        LOG(INFO, "break on NtStopProfile");

    const auto ProfileHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtStopProfile)
        it(ProfileHandle);
}

bool monitor::GenericMonitor::register_NtSuspendProcess(proc_t proc, const on_NtSuspendProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSuspendProcess");
    if(!ok)
        FAIL(false, "Unable to register NtSuspendProcess");

    d_->observers_NtSuspendProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSuspendProcess()
{
    if(false)
        LOG(INFO, "break on NtSuspendProcess");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtSuspendProcess)
        it(ProcessHandle);
}

bool monitor::GenericMonitor::register_NtSuspendThread(proc_t proc, const on_NtSuspendThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSuspendThread");
    if(!ok)
        FAIL(false, "Unable to register NtSuspendThread");

    d_->observers_NtSuspendThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSuspendThread()
{
    if(false)
        LOG(INFO, "break on NtSuspendThread");

    const auto ThreadHandle         = arg<nt::HANDLE>(core_, 0);
    const auto PreviousSuspendCount = arg<nt::PULONG>(core_, 1);

    for(const auto& it : d_->observers_NtSuspendThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSystemDebugControl");
    if(!ok)
        FAIL(false, "Unable to register NtSystemDebugControl");

    d_->observers_NtSystemDebugControl.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSystemDebugControl()
{
    if(false)
        LOG(INFO, "break on NtSystemDebugControl");

    const auto Command            = arg<nt::SYSDBG_COMMAND>(core_, 0);
    const auto InputBuffer        = arg<nt::PVOID>(core_, 1);
    const auto InputBufferLength  = arg<nt::ULONG>(core_, 2);
    const auto OutputBuffer       = arg<nt::PVOID>(core_, 3);
    const auto OutputBufferLength = arg<nt::ULONG>(core_, 4);
    const auto ReturnLength       = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtSystemDebugControl)
        it(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtTerminateJobObject(proc_t proc, const on_NtTerminateJobObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTerminateJobObject");
    if(!ok)
        FAIL(false, "Unable to register NtTerminateJobObject");

    d_->observers_NtTerminateJobObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateJobObject()
{
    if(false)
        LOG(INFO, "break on NtTerminateJobObject");

    const auto JobHandle  = arg<nt::HANDLE>(core_, 0);
    const auto ExitStatus = arg<nt::NTSTATUS>(core_, 1);

    for(const auto& it : d_->observers_NtTerminateJobObject)
        it(JobHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTerminateProcess(proc_t proc, const on_NtTerminateProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTerminateProcess");
    if(!ok)
        FAIL(false, "Unable to register NtTerminateProcess");

    d_->observers_NtTerminateProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateProcess()
{
    if(false)
        LOG(INFO, "break on NtTerminateProcess");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto ExitStatus    = arg<nt::NTSTATUS>(core_, 1);

    for(const auto& it : d_->observers_NtTerminateProcess)
        it(ProcessHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTerminateThread(proc_t proc, const on_NtTerminateThread_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTerminateThread");
    if(!ok)
        FAIL(false, "Unable to register NtTerminateThread");

    d_->observers_NtTerminateThread.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateThread()
{
    if(false)
        LOG(INFO, "break on NtTerminateThread");

    const auto ThreadHandle = arg<nt::HANDLE>(core_, 0);
    const auto ExitStatus   = arg<nt::NTSTATUS>(core_, 1);

    for(const auto& it : d_->observers_NtTerminateThread)
        it(ThreadHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTraceControl(proc_t proc, const on_NtTraceControl_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTraceControl");
    if(!ok)
        FAIL(false, "Unable to register NtTraceControl");

    d_->observers_NtTraceControl.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTraceControl()
{
    if(false)
        LOG(INFO, "break on NtTraceControl");

    const auto FunctionCode = arg<nt::ULONG>(core_, 0);
    const auto InBuffer     = arg<nt::PVOID>(core_, 1);
    const auto InBufferLen  = arg<nt::ULONG>(core_, 2);
    const auto OutBuffer    = arg<nt::PVOID>(core_, 3);
    const auto OutBufferLen = arg<nt::ULONG>(core_, 4);
    const auto ReturnLength = arg<nt::PULONG>(core_, 5);

    for(const auto& it : d_->observers_NtTraceControl)
        it(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
}

bool monitor::GenericMonitor::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTraceEvent");
    if(!ok)
        FAIL(false, "Unable to register NtTraceEvent");

    d_->observers_NtTraceEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTraceEvent()
{
    if(false)
        LOG(INFO, "break on NtTraceEvent");

    const auto TraceHandle = arg<nt::HANDLE>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto FieldSize   = arg<nt::ULONG>(core_, 2);
    const auto Fields      = arg<nt::PVOID>(core_, 3);

    for(const auto& it : d_->observers_NtTraceEvent)
        it(TraceHandle, Flags, FieldSize, Fields);
}

bool monitor::GenericMonitor::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTranslateFilePath");
    if(!ok)
        FAIL(false, "Unable to register NtTranslateFilePath");

    d_->observers_NtTranslateFilePath.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTranslateFilePath()
{
    if(false)
        LOG(INFO, "break on NtTranslateFilePath");

    const auto InputFilePath        = arg<nt::PFILE_PATH>(core_, 0);
    const auto OutputType           = arg<nt::ULONG>(core_, 1);
    const auto OutputFilePath       = arg<nt::PFILE_PATH>(core_, 2);
    const auto OutputFilePathLength = arg<nt::PULONG>(core_, 3);

    for(const auto& it : d_->observers_NtTranslateFilePath)
        it(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
}

bool monitor::GenericMonitor::register_NtUnloadDriver(proc_t proc, const on_NtUnloadDriver_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnloadDriver");
    if(!ok)
        FAIL(false, "Unable to register NtUnloadDriver");

    d_->observers_NtUnloadDriver.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadDriver()
{
    if(false)
        LOG(INFO, "break on NtUnloadDriver");

    const auto DriverServiceName = arg<nt::PUNICODE_STRING>(core_, 0);

    for(const auto& it : d_->observers_NtUnloadDriver)
        it(DriverServiceName);
}

bool monitor::GenericMonitor::register_NtUnloadKey2(proc_t proc, const on_NtUnloadKey2_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnloadKey2");
    if(!ok)
        FAIL(false, "Unable to register NtUnloadKey2");

    d_->observers_NtUnloadKey2.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKey2()
{
    if(false)
        LOG(INFO, "break on NtUnloadKey2");

    const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto Flags     = arg<nt::ULONG>(core_, 1);

    for(const auto& it : d_->observers_NtUnloadKey2)
        it(TargetKey, Flags);
}

bool monitor::GenericMonitor::register_NtUnloadKeyEx(proc_t proc, const on_NtUnloadKeyEx_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnloadKeyEx");
    if(!ok)
        FAIL(false, "Unable to register NtUnloadKeyEx");

    d_->observers_NtUnloadKeyEx.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKeyEx()
{
    if(false)
        LOG(INFO, "break on NtUnloadKeyEx");

    const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);
    const auto Event     = arg<nt::HANDLE>(core_, 1);

    for(const auto& it : d_->observers_NtUnloadKeyEx)
        it(TargetKey, Event);
}

bool monitor::GenericMonitor::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnloadKey");
    if(!ok)
        FAIL(false, "Unable to register NtUnloadKey");

    d_->observers_NtUnloadKey.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKey()
{
    if(false)
        LOG(INFO, "break on NtUnloadKey");

    const auto TargetKey = arg<nt::POBJECT_ATTRIBUTES>(core_, 0);

    for(const auto& it : d_->observers_NtUnloadKey)
        it(TargetKey);
}

bool monitor::GenericMonitor::register_NtUnlockFile(proc_t proc, const on_NtUnlockFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnlockFile");
    if(!ok)
        FAIL(false, "Unable to register NtUnlockFile");

    d_->observers_NtUnlockFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnlockFile()
{
    if(false)
        LOG(INFO, "break on NtUnlockFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 1);
    const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core_, 2);
    const auto Length        = arg<nt::PLARGE_INTEGER>(core_, 3);
    const auto Key           = arg<nt::ULONG>(core_, 4);

    for(const auto& it : d_->observers_NtUnlockFile)
        it(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
}

bool monitor::GenericMonitor::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnlockVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtUnlockVirtualMemory");

    d_->observers_NtUnlockVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnlockVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtUnlockVirtualMemory");

    const auto ProcessHandle   = arg<nt::HANDLE>(core_, 0);
    const auto STARBaseAddress = arg<nt::PVOID>(core_, 1);
    const auto RegionSize      = arg<nt::PSIZE_T>(core_, 2);
    const auto MapType         = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_NtUnlockVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
}

bool monitor::GenericMonitor::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUnmapViewOfSection");
    if(!ok)
        FAIL(false, "Unable to register NtUnmapViewOfSection");

    d_->observers_NtUnmapViewOfSection.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUnmapViewOfSection()
{
    if(false)
        LOG(INFO, "break on NtUnmapViewOfSection");

    const auto ProcessHandle = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress   = arg<nt::PVOID>(core_, 1);

    for(const auto& it : d_->observers_NtUnmapViewOfSection)
        it(ProcessHandle, BaseAddress);
}

bool monitor::GenericMonitor::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_func)
{
    const auto ok = setup_func(proc, "NtVdmControl");
    if(!ok)
        FAIL(false, "Unable to register NtVdmControl");

    d_->observers_NtVdmControl.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtVdmControl()
{
    if(false)
        LOG(INFO, "break on NtVdmControl");

    const auto Service     = arg<nt::VDMSERVICECLASS>(core_, 0);
    const auto ServiceData = arg<nt::PVOID>(core_, 1);

    for(const auto& it : d_->observers_NtVdmControl)
        it(Service, ServiceData);
}

bool monitor::GenericMonitor::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForDebugEvent");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForDebugEvent");

    d_->observers_NtWaitForDebugEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForDebugEvent()
{
    if(false)
        LOG(INFO, "break on NtWaitForDebugEvent");

    const auto DebugObjectHandle = arg<nt::HANDLE>(core_, 0);
    const auto Alertable         = arg<nt::BOOLEAN>(core_, 1);
    const auto Timeout           = arg<nt::PLARGE_INTEGER>(core_, 2);
    const auto WaitStateChange   = arg<nt::PDBGUI_WAIT_STATE_CHANGE>(core_, 3);

    for(const auto& it : d_->observers_NtWaitForDebugEvent)
        it(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
}

bool monitor::GenericMonitor::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForKeyedEvent");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForKeyedEvent");

    d_->observers_NtWaitForKeyedEvent.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForKeyedEvent()
{
    if(false)
        LOG(INFO, "break on NtWaitForKeyedEvent");

    const auto KeyedEventHandle = arg<nt::HANDLE>(core_, 0);
    const auto KeyValue         = arg<nt::PVOID>(core_, 1);
    const auto Alertable        = arg<nt::BOOLEAN>(core_, 2);
    const auto Timeout          = arg<nt::PLARGE_INTEGER>(core_, 3);

    for(const auto& it : d_->observers_NtWaitForKeyedEvent)
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForMultipleObjects32");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForMultipleObjects32");

    d_->observers_NtWaitForMultipleObjects32.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects32()
{
    if(false)
        LOG(INFO, "break on NtWaitForMultipleObjects32");

    const auto Count     = arg<nt::ULONG>(core_, 0);
    const auto Handles   = arg<nt::LONG>(core_, 1);
    const auto WaitType  = arg<nt::WAIT_TYPE>(core_, 2);
    const auto Alertable = arg<nt::BOOLEAN>(core_, 3);
    const auto Timeout   = arg<nt::PLARGE_INTEGER>(core_, 4);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects32)
        it(Count, Handles, WaitType, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForMultipleObjects");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForMultipleObjects");

    d_->observers_NtWaitForMultipleObjects.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects()
{
    if(false)
        LOG(INFO, "break on NtWaitForMultipleObjects");

    const auto Count     = arg<nt::ULONG>(core_, 0);
    const auto Handles   = arg<nt::HANDLE>(core_, 1);
    const auto WaitType  = arg<nt::WAIT_TYPE>(core_, 2);
    const auto Alertable = arg<nt::BOOLEAN>(core_, 3);
    const auto Timeout   = arg<nt::PLARGE_INTEGER>(core_, 4);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects)
        it(Count, Handles, WaitType, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForSingleObject(proc_t proc, const on_NtWaitForSingleObject_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForSingleObject");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForSingleObject");

    d_->observers_NtWaitForSingleObject.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForSingleObject()
{
    if(false)
        LOG(INFO, "break on NtWaitForSingleObject");

    const auto Handle    = arg<nt::HANDLE>(core_, 0);
    const auto Alertable = arg<nt::BOOLEAN>(core_, 1);
    const auto Timeout   = arg<nt::PLARGE_INTEGER>(core_, 2);

    for(const auto& it : d_->observers_NtWaitForSingleObject)
        it(Handle, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitForWorkViaWorkerFactory");
    if(!ok)
        FAIL(false, "Unable to register NtWaitForWorkViaWorkerFactory");

    d_->observers_NtWaitForWorkViaWorkerFactory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForWorkViaWorkerFactory()
{
    if(false)
        LOG(INFO, "break on NtWaitForWorkViaWorkerFactory");

    const auto WorkerFactoryHandle = arg<nt::HANDLE>(core_, 0);
    const auto MiniPacket          = arg<nt::PFILE_IO_COMPLETION_INFORMATION>(core_, 1);

    for(const auto& it : d_->observers_NtWaitForWorkViaWorkerFactory)
        it(WorkerFactoryHandle, MiniPacket);
}

bool monitor::GenericMonitor::register_NtWaitHighEventPair(proc_t proc, const on_NtWaitHighEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitHighEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtWaitHighEventPair");

    d_->observers_NtWaitHighEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitHighEventPair()
{
    if(false)
        LOG(INFO, "break on NtWaitHighEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtWaitHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWaitLowEventPair");
    if(!ok)
        FAIL(false, "Unable to register NtWaitLowEventPair");

    d_->observers_NtWaitLowEventPair.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWaitLowEventPair()
{
    if(false)
        LOG(INFO, "break on NtWaitLowEventPair");

    const auto EventPairHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtWaitLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWorkerFactoryWorkerReady");
    if(!ok)
        FAIL(false, "Unable to register NtWorkerFactoryWorkerReady");

    d_->observers_NtWorkerFactoryWorkerReady.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWorkerFactoryWorkerReady()
{
    if(false)
        LOG(INFO, "break on NtWorkerFactoryWorkerReady");

    const auto WorkerFactoryHandle = arg<nt::HANDLE>(core_, 0);

    for(const auto& it : d_->observers_NtWorkerFactoryWorkerReady)
        it(WorkerFactoryHandle);
}

bool monitor::GenericMonitor::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWriteFileGather");
    if(!ok)
        FAIL(false, "Unable to register NtWriteFileGather");

    d_->observers_NtWriteFileGather.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWriteFileGather()
{
    if(false)
        LOG(INFO, "break on NtWriteFileGather");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Event         = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext    = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto SegmentArray  = arg<nt::PFILE_SEGMENT_ELEMENT>(core_, 5);
    const auto Length        = arg<nt::ULONG>(core_, 6);
    const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core_, 7);
    const auto Key           = arg<nt::PULONG>(core_, 8);

    for(const auto& it : d_->observers_NtWriteFileGather)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWriteFile");
    if(!ok)
        FAIL(false, "Unable to register NtWriteFile");

    d_->observers_NtWriteFile.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWriteFile()
{
    if(false)
        LOG(INFO, "break on NtWriteFile");

    const auto FileHandle    = arg<nt::HANDLE>(core_, 0);
    const auto Event         = arg<nt::HANDLE>(core_, 1);
    const auto ApcRoutine    = arg<nt::PIO_APC_ROUTINE>(core_, 2);
    const auto ApcContext    = arg<nt::PVOID>(core_, 3);
    const auto IoStatusBlock = arg<nt::PIO_STATUS_BLOCK>(core_, 4);
    const auto Buffer        = arg<nt::PVOID>(core_, 5);
    const auto Length        = arg<nt::ULONG>(core_, 6);
    const auto ByteOffset    = arg<nt::PLARGE_INTEGER>(core_, 7);
    const auto Key           = arg<nt::PULONG>(core_, 8);

    for(const auto& it : d_->observers_NtWriteFile)
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWriteRequestData");
    if(!ok)
        FAIL(false, "Unable to register NtWriteRequestData");

    d_->observers_NtWriteRequestData.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWriteRequestData()
{
    if(false)
        LOG(INFO, "break on NtWriteRequestData");

    const auto PortHandle           = arg<nt::HANDLE>(core_, 0);
    const auto Message              = arg<nt::PPORT_MESSAGE>(core_, 1);
    const auto DataEntryIndex       = arg<nt::ULONG>(core_, 2);
    const auto Buffer               = arg<nt::PVOID>(core_, 3);
    const auto BufferSize           = arg<nt::SIZE_T>(core_, 4);
    const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(core_, 5);

    for(const auto& it : d_->observers_NtWriteRequestData)
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
}

bool monitor::GenericMonitor::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_func)
{
    const auto ok = setup_func(proc, "NtWriteVirtualMemory");
    if(!ok)
        FAIL(false, "Unable to register NtWriteVirtualMemory");

    d_->observers_NtWriteVirtualMemory.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtWriteVirtualMemory()
{
    if(false)
        LOG(INFO, "break on NtWriteVirtualMemory");

    const auto ProcessHandle        = arg<nt::HANDLE>(core_, 0);
    const auto BaseAddress          = arg<nt::PVOID>(core_, 1);
    const auto Buffer               = arg<nt::PVOID>(core_, 2);
    const auto BufferSize           = arg<nt::SIZE_T>(core_, 3);
    const auto NumberOfBytesWritten = arg<nt::PSIZE_T>(core_, 4);

    for(const auto& it : d_->observers_NtWriteVirtualMemory)
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
}

bool monitor::GenericMonitor::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_func)
{
    const auto ok = setup_func(proc, "NtDisableLastKnownGood");
    if(!ok)
        FAIL(false, "Unable to register NtDisableLastKnownGood");

    d_->observers_NtDisableLastKnownGood.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtDisableLastKnownGood()
{
    if(false)
        LOG(INFO, "break on NtDisableLastKnownGood");

    

    for(const auto& it : d_->observers_NtDisableLastKnownGood)
        it();
}

bool monitor::GenericMonitor::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_func)
{
    const auto ok = setup_func(proc, "NtEnableLastKnownGood");
    if(!ok)
        FAIL(false, "Unable to register NtEnableLastKnownGood");

    d_->observers_NtEnableLastKnownGood.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtEnableLastKnownGood()
{
    if(false)
        LOG(INFO, "break on NtEnableLastKnownGood");

    

    for(const auto& it : d_->observers_NtEnableLastKnownGood)
        it();
}

bool monitor::GenericMonitor::register_NtFlushProcessWriteBuffers(proc_t proc, const on_NtFlushProcessWriteBuffers_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushProcessWriteBuffers");
    if(!ok)
        FAIL(false, "Unable to register NtFlushProcessWriteBuffers");

    d_->observers_NtFlushProcessWriteBuffers.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushProcessWriteBuffers()
{
    if(false)
        LOG(INFO, "break on NtFlushProcessWriteBuffers");

    

    for(const auto& it : d_->observers_NtFlushProcessWriteBuffers)
        it();
}

bool monitor::GenericMonitor::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_func)
{
    const auto ok = setup_func(proc, "NtFlushWriteBuffer");
    if(!ok)
        FAIL(false, "Unable to register NtFlushWriteBuffer");

    d_->observers_NtFlushWriteBuffer.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtFlushWriteBuffer()
{
    if(false)
        LOG(INFO, "break on NtFlushWriteBuffer");

    

    for(const auto& it : d_->observers_NtFlushWriteBuffer)
        it();
}

bool monitor::GenericMonitor::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_func)
{
    const auto ok = setup_func(proc, "NtGetCurrentProcessorNumber");
    if(!ok)
        FAIL(false, "Unable to register NtGetCurrentProcessorNumber");

    d_->observers_NtGetCurrentProcessorNumber.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtGetCurrentProcessorNumber()
{
    if(false)
        LOG(INFO, "break on NtGetCurrentProcessorNumber");

    

    for(const auto& it : d_->observers_NtGetCurrentProcessorNumber)
        it();
}

bool monitor::GenericMonitor::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_func)
{
    const auto ok = setup_func(proc, "NtIsSystemResumeAutomatic");
    if(!ok)
        FAIL(false, "Unable to register NtIsSystemResumeAutomatic");

    d_->observers_NtIsSystemResumeAutomatic.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtIsSystemResumeAutomatic()
{
    if(false)
        LOG(INFO, "break on NtIsSystemResumeAutomatic");

    

    for(const auto& it : d_->observers_NtIsSystemResumeAutomatic)
        it();
}

bool monitor::GenericMonitor::register_NtIsUILanguageComitted(proc_t proc, const on_NtIsUILanguageComitted_fn& on_func)
{
    const auto ok = setup_func(proc, "NtIsUILanguageComitted");
    if(!ok)
        FAIL(false, "Unable to register NtIsUILanguageComitted");

    d_->observers_NtIsUILanguageComitted.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtIsUILanguageComitted()
{
    if(false)
        LOG(INFO, "break on NtIsUILanguageComitted");

    

    for(const auto& it : d_->observers_NtIsUILanguageComitted)
        it();
}

bool monitor::GenericMonitor::register_NtQueryPortInformationProcess(proc_t proc, const on_NtQueryPortInformationProcess_fn& on_func)
{
    const auto ok = setup_func(proc, "NtQueryPortInformationProcess");
    if(!ok)
        FAIL(false, "Unable to register NtQueryPortInformationProcess");

    d_->observers_NtQueryPortInformationProcess.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtQueryPortInformationProcess()
{
    if(false)
        LOG(INFO, "break on NtQueryPortInformationProcess");

    

    for(const auto& it : d_->observers_NtQueryPortInformationProcess)
        it();
}

bool monitor::GenericMonitor::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_func)
{
    const auto ok = setup_func(proc, "NtSerializeBoot");
    if(!ok)
        FAIL(false, "Unable to register NtSerializeBoot");

    d_->observers_NtSerializeBoot.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtSerializeBoot()
{
    if(false)
        LOG(INFO, "break on NtSerializeBoot");

    

    for(const auto& it : d_->observers_NtSerializeBoot)
        it();
}

bool monitor::GenericMonitor::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_func)
{
    const auto ok = setup_func(proc, "NtTestAlert");
    if(!ok)
        FAIL(false, "Unable to register NtTestAlert");

    d_->observers_NtTestAlert.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtTestAlert()
{
    if(false)
        LOG(INFO, "break on NtTestAlert");

    

    for(const auto& it : d_->observers_NtTestAlert)
        it();
}

bool monitor::GenericMonitor::register_NtThawRegistry(proc_t proc, const on_NtThawRegistry_fn& on_func)
{
    const auto ok = setup_func(proc, "NtThawRegistry");
    if(!ok)
        FAIL(false, "Unable to register NtThawRegistry");

    d_->observers_NtThawRegistry.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtThawRegistry()
{
    if(false)
        LOG(INFO, "break on NtThawRegistry");

    

    for(const auto& it : d_->observers_NtThawRegistry)
        it();
}

bool monitor::GenericMonitor::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_func)
{
    const auto ok = setup_func(proc, "NtThawTransactions");
    if(!ok)
        FAIL(false, "Unable to register NtThawTransactions");

    d_->observers_NtThawTransactions.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtThawTransactions()
{
    if(false)
        LOG(INFO, "break on NtThawTransactions");

    

    for(const auto& it : d_->observers_NtThawTransactions)
        it();
}

bool monitor::GenericMonitor::register_NtUmsThreadYield(proc_t proc, const on_NtUmsThreadYield_fn& on_func)
{
    const auto ok = setup_func(proc, "NtUmsThreadYield");
    if(!ok)
        FAIL(false, "Unable to register NtUmsThreadYield");

    d_->observers_NtUmsThreadYield.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtUmsThreadYield()
{
    if(false)
        LOG(INFO, "break on NtUmsThreadYield");

    

    for(const auto& it : d_->observers_NtUmsThreadYield)
        it();
}

bool monitor::GenericMonitor::register_NtYieldExecution(proc_t proc, const on_NtYieldExecution_fn& on_func)
{
    const auto ok = setup_func(proc, "NtYieldExecution");
    if(!ok)
        FAIL(false, "Unable to register NtYieldExecution");

    d_->observers_NtYieldExecution.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_NtYieldExecution()
{
    if(false)
        LOG(INFO, "break on NtYieldExecution");

    

    for(const auto& it : d_->observers_NtYieldExecution)
        it();
}

bool monitor::GenericMonitor::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlpAllocateHeapInternal");
    if(!ok)
        FAIL(false, "Unable to register RtlpAllocateHeapInternal");

    d_->observers_RtlpAllocateHeapInternal.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlpAllocateHeapInternal()
{
    if(false)
        LOG(INFO, "break on RtlpAllocateHeapInternal");

    const auto HeapHandle = arg<nt::PVOID>(core_, 0);
    const auto Size       = arg<nt::SIZE_T>(core_, 1);

    for(const auto& it : d_->observers_RtlpAllocateHeapInternal)
        it(HeapHandle, Size);
}

bool monitor::GenericMonitor::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlFreeHeap");
    if(!ok)
        FAIL(false, "Unable to register RtlFreeHeap");

    d_->observers_RtlFreeHeap.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlFreeHeap()
{
    if(false)
        LOG(INFO, "break on RtlFreeHeap");

    const auto HeapHandle  = arg<nt::PVOID>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress = arg<nt::PVOID>(core_, 2);

    for(const auto& it : d_->observers_RtlFreeHeap)
        it(HeapHandle, Flags, BaseAddress);
}

bool monitor::GenericMonitor::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlpReAllocateHeapInternal");
    if(!ok)
        FAIL(false, "Unable to register RtlpReAllocateHeapInternal");

    d_->observers_RtlpReAllocateHeapInternal.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlpReAllocateHeapInternal()
{
    if(false)
        LOG(INFO, "break on RtlpReAllocateHeapInternal");

    const auto HeapHandle  = arg<nt::PVOID>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress = arg<nt::PVOID>(core_, 2);
    const auto Size        = arg<nt::ULONG>(core_, 3);

    for(const auto& it : d_->observers_RtlpReAllocateHeapInternal)
        it(HeapHandle, Flags, BaseAddress, Size);
}

bool monitor::GenericMonitor::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlSizeHeap");
    if(!ok)
        FAIL(false, "Unable to register RtlSizeHeap");

    d_->observers_RtlSizeHeap.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlSizeHeap()
{
    if(false)
        LOG(INFO, "break on RtlSizeHeap");

    const auto HeapHandle  = arg<nt::PVOID>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress = arg<nt::PVOID>(core_, 2);

    for(const auto& it : d_->observers_RtlSizeHeap)
        it(HeapHandle, Flags, BaseAddress);
}

bool monitor::GenericMonitor::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlSetUserValueHeap");
    if(!ok)
        FAIL(false, "Unable to register RtlSetUserValueHeap");

    d_->observers_RtlSetUserValueHeap.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlSetUserValueHeap()
{
    if(false)
        LOG(INFO, "break on RtlSetUserValueHeap");

    const auto HeapHandle  = arg<nt::PVOID>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress = arg<nt::PVOID>(core_, 2);
    const auto UserValue   = arg<nt::PVOID>(core_, 3);

    for(const auto& it : d_->observers_RtlSetUserValueHeap)
        it(HeapHandle, Flags, BaseAddress, UserValue);
}

bool monitor::GenericMonitor::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_func)
{
    const auto ok = setup_func(proc, "RtlGetUserInfoHeap");
    if(!ok)
        FAIL(false, "Unable to register RtlGetUserInfoHeap");

    d_->observers_RtlGetUserInfoHeap.push_back(on_func);
    return true;
}

void monitor::GenericMonitor::on_RtlGetUserInfoHeap()
{
    if(false)
        LOG(INFO, "break on RtlGetUserInfoHeap");

    const auto HeapHandle  = arg<nt::PVOID>(core_, 0);
    const auto Flags       = arg<nt::ULONG>(core_, 1);
    const auto BaseAddress = arg<nt::PVOID>(core_, 2);
    const auto UserValue   = arg<nt::PVOID>(core_, 3);
    const auto UserFlags   = arg<nt::PULONG>(core_, 4);

    for(const auto& it : d_->observers_RtlGetUserInfoHeap)
        it(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
}
