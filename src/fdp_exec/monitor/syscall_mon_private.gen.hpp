#pragma once

#include "generic_mon.hpp"


void monitor::GenericMonitor::register_NtAcceptConnectPort(const on_NtAcceptConnectPort_fn& on_ntacceptconnectport)
{
    d_->observers_NtAcceptConnectPort.push_back(on_ntacceptconnectport);
}

void monitor::GenericMonitor::on_NtAcceptConnectPort()
{
    LOG(INFO, "Break on NtAcceptConnectPort");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortContext     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ConnectionRequest= nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto AcceptConnection= nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto ServerView      = nt::cast_to<nt::PPORT_VIEW>         (args[4]);
    const auto ClientView      = nt::cast_to<nt::PREMOTE_PORT_VIEW>  (args[5]);

    for(const auto& it : d_->observers_NtAcceptConnectPort)
    {
        it(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckAndAuditAlarm(const on_NtAccessCheckAndAuditAlarm_fn& on_ntaccesscheckandauditalarm)
{
    d_->observers_NtAccessCheckAndAuditAlarm.push_back(on_ntaccesscheckandauditalarm);
}

void monitor::GenericMonitor::on_NtAccessCheckAndAuditAlarm()
{
    LOG(INFO, "Break on NtAccessCheckAndAuditAlarm");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ObjectTypeName  = nt::cast_to<nt::PUNICODE_STRING>    (args[2]);
    const auto ObjectName      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[4]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[5]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[6]);
    const auto ObjectCreation  = nt::cast_to<nt::BOOLEAN>            (args[7]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[8]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[9]);
    const auto GenerateOnClose = nt::cast_to<nt::PBOOLEAN>           (args[10]);

    for(const auto& it : d_->observers_NtAccessCheckAndAuditAlarm)
    {
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckByTypeAndAuditAlarm(const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_ntaccesscheckbytypeandauditalarm)
{
    d_->observers_NtAccessCheckByTypeAndAuditAlarm.push_back(on_ntaccesscheckbytypeandauditalarm);
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeAndAuditAlarm()
{
    LOG(INFO, "Break on NtAccessCheckByTypeAndAuditAlarm");
    const auto nargs = 16;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ObjectTypeName  = nt::cast_to<nt::PUNICODE_STRING>    (args[2]);
    const auto ObjectName      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[4]);
    const auto PrincipalSelfSid= nt::cast_to<nt::PSID>               (args[5]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[6]);
    const auto AuditType       = nt::cast_to<nt::AUDIT_EVENT_TYPE>   (args[7]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[8]);
    const auto ObjectTypeList  = nt::cast_to<nt::POBJECT_TYPE_LIST>  (args[9]);
    const auto ObjectTypeListLength= nt::cast_to<nt::ULONG>              (args[10]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[11]);
    const auto ObjectCreation  = nt::cast_to<nt::BOOLEAN>            (args[12]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[13]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[14]);
    const auto GenerateOnClose = nt::cast_to<nt::PBOOLEAN>           (args[15]);

    for(const auto& it : d_->observers_NtAccessCheckByTypeAndAuditAlarm)
    {
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckByType(const on_NtAccessCheckByType_fn& on_ntaccesscheckbytype)
{
    d_->observers_NtAccessCheckByType.push_back(on_ntaccesscheckbytype);
}

void monitor::GenericMonitor::on_NtAccessCheckByType()
{
    LOG(INFO, "Break on NtAccessCheckByType");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[0]);
    const auto PrincipalSelfSid= nt::cast_to<nt::PSID>               (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[3]);
    const auto ObjectTypeList  = nt::cast_to<nt::POBJECT_TYPE_LIST>  (args[4]);
    const auto ObjectTypeListLength= nt::cast_to<nt::ULONG>              (args[5]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[6]);
    const auto PrivilegeSet    = nt::cast_to<nt::PPRIVILEGE_SET>     (args[7]);
    const auto PrivilegeSetLength= nt::cast_to<nt::PULONG>             (args[8]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[9]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[10]);

    for(const auto& it : d_->observers_NtAccessCheckByType)
    {
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_ntaccesscheckbytyperesultlistandauditalarmbyhandle)
{
    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle.push_back(on_ntaccesscheckbytyperesultlistandauditalarmbyhandle);
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle()
{
    LOG(INFO, "Break on NtAccessCheckByTypeResultListAndAuditAlarmByHandle");
    const auto nargs = 17;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto ObjectTypeName  = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto ObjectName      = nt::cast_to<nt::PUNICODE_STRING>    (args[4]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[5]);
    const auto PrincipalSelfSid= nt::cast_to<nt::PSID>               (args[6]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[7]);
    const auto AuditType       = nt::cast_to<nt::AUDIT_EVENT_TYPE>   (args[8]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[9]);
    const auto ObjectTypeList  = nt::cast_to<nt::POBJECT_TYPE_LIST>  (args[10]);
    const auto ObjectTypeListLength= nt::cast_to<nt::ULONG>              (args[11]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[12]);
    const auto ObjectCreation  = nt::cast_to<nt::BOOLEAN>            (args[13]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[14]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[15]);
    const auto GenerateOnClose = nt::cast_to<nt::PBOOLEAN>           (args[16]);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle)
    {
        it(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarm(const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_ntaccesscheckbytyperesultlistandauditalarm)
{
    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.push_back(on_ntaccesscheckbytyperesultlistandauditalarm);
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarm()
{
    LOG(INFO, "Break on NtAccessCheckByTypeResultListAndAuditAlarm");
    const auto nargs = 16;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ObjectTypeName  = nt::cast_to<nt::PUNICODE_STRING>    (args[2]);
    const auto ObjectName      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[4]);
    const auto PrincipalSelfSid= nt::cast_to<nt::PSID>               (args[5]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[6]);
    const auto AuditType       = nt::cast_to<nt::AUDIT_EVENT_TYPE>   (args[7]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[8]);
    const auto ObjectTypeList  = nt::cast_to<nt::POBJECT_TYPE_LIST>  (args[9]);
    const auto ObjectTypeListLength= nt::cast_to<nt::ULONG>              (args[10]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[11]);
    const auto ObjectCreation  = nt::cast_to<nt::BOOLEAN>            (args[12]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[13]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[14]);
    const auto GenerateOnClose = nt::cast_to<nt::PBOOLEAN>           (args[15]);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm)
    {
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtAccessCheckByTypeResultList(const on_NtAccessCheckByTypeResultList_fn& on_ntaccesscheckbytyperesultlist)
{
    d_->observers_NtAccessCheckByTypeResultList.push_back(on_ntaccesscheckbytyperesultlist);
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultList()
{
    LOG(INFO, "Break on NtAccessCheckByTypeResultList");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[0]);
    const auto PrincipalSelfSid= nt::cast_to<nt::PSID>               (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[3]);
    const auto ObjectTypeList  = nt::cast_to<nt::POBJECT_TYPE_LIST>  (args[4]);
    const auto ObjectTypeListLength= nt::cast_to<nt::ULONG>              (args[5]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[6]);
    const auto PrivilegeSet    = nt::cast_to<nt::PPRIVILEGE_SET>     (args[7]);
    const auto PrivilegeSetLength= nt::cast_to<nt::PULONG>             (args[8]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[9]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[10]);

    for(const auto& it : d_->observers_NtAccessCheckByTypeResultList)
    {
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }
}

void monitor::GenericMonitor::register_NtAccessCheck(const on_NtAccessCheck_fn& on_ntaccesscheck)
{
    d_->observers_NtAccessCheck.push_back(on_ntaccesscheck);
}

void monitor::GenericMonitor::on_NtAccessCheck()
{
    LOG(INFO, "Break on NtAccessCheck");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[0]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[2]);
    const auto GenericMapping  = nt::cast_to<nt::PGENERIC_MAPPING>   (args[3]);
    const auto PrivilegeSet    = nt::cast_to<nt::PPRIVILEGE_SET>     (args[4]);
    const auto PrivilegeSetLength= nt::cast_to<nt::PULONG>             (args[5]);
    const auto GrantedAccess   = nt::cast_to<nt::PACCESS_MASK>       (args[6]);
    const auto AccessStatus    = nt::cast_to<nt::PNTSTATUS>          (args[7]);

    for(const auto& it : d_->observers_NtAccessCheck)
    {
        it(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
    }
}

void monitor::GenericMonitor::register_NtAddAtom(const on_NtAddAtom_fn& on_ntaddatom)
{
    d_->observers_NtAddAtom.push_back(on_ntaddatom);
}

void monitor::GenericMonitor::on_NtAddAtom()
{
    LOG(INFO, "Break on NtAddAtom");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto AtomName        = nt::cast_to<nt::PWSTR>              (args[0]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Atom            = nt::cast_to<nt::PRTL_ATOM>          (args[2]);

    for(const auto& it : d_->observers_NtAddAtom)
    {
        it(AtomName, Length, Atom);
    }
}

void monitor::GenericMonitor::register_NtAddBootEntry(const on_NtAddBootEntry_fn& on_ntaddbootentry)
{
    d_->observers_NtAddBootEntry.push_back(on_ntaddbootentry);
}

void monitor::GenericMonitor::on_NtAddBootEntry()
{
    LOG(INFO, "Break on NtAddBootEntry");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootEntry       = nt::cast_to<nt::PBOOT_ENTRY>        (args[0]);
    const auto Id              = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAddBootEntry)
    {
        it(BootEntry, Id);
    }
}

void monitor::GenericMonitor::register_NtAddDriverEntry(const on_NtAddDriverEntry_fn& on_ntadddriverentry)
{
    d_->observers_NtAddDriverEntry.push_back(on_ntadddriverentry);
}

void monitor::GenericMonitor::on_NtAddDriverEntry()
{
    LOG(INFO, "Break on NtAddDriverEntry");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverEntry     = nt::cast_to<nt::PEFI_DRIVER_ENTRY>  (args[0]);
    const auto Id              = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAddDriverEntry)
    {
        it(DriverEntry, Id);
    }
}

void monitor::GenericMonitor::register_NtAdjustGroupsToken(const on_NtAdjustGroupsToken_fn& on_ntadjustgroupstoken)
{
    d_->observers_NtAdjustGroupsToken.push_back(on_ntadjustgroupstoken);
}

void monitor::GenericMonitor::on_NtAdjustGroupsToken()
{
    LOG(INFO, "Break on NtAdjustGroupsToken");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResetToDefault  = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto NewState        = nt::cast_to<nt::PTOKEN_GROUPS>      (args[2]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[3]);
    const auto PreviousState   = nt::cast_to<nt::PTOKEN_GROUPS>      (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAdjustGroupsToken)
    {
        it(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtAdjustPrivilegesToken(const on_NtAdjustPrivilegesToken_fn& on_ntadjustprivilegestoken)
{
    d_->observers_NtAdjustPrivilegesToken.push_back(on_ntadjustprivilegestoken);
}

void monitor::GenericMonitor::on_NtAdjustPrivilegesToken()
{
    LOG(INFO, "Break on NtAdjustPrivilegesToken");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DisableAllPrivileges= nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto NewState        = nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[2]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[3]);
    const auto PreviousState   = nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAdjustPrivilegesToken)
    {
        it(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtAlertResumeThread(const on_NtAlertResumeThread_fn& on_ntalertresumethread)
{
    d_->observers_NtAlertResumeThread.push_back(on_ntalertresumethread);
}

void monitor::GenericMonitor::on_NtAlertResumeThread()
{
    LOG(INFO, "Break on NtAlertResumeThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAlertResumeThread)
    {
        it(ThreadHandle, PreviousSuspendCount);
    }
}

void monitor::GenericMonitor::register_NtAlertThread(const on_NtAlertThread_fn& on_ntalertthread)
{
    d_->observers_NtAlertThread.push_back(on_ntalertthread);
}

void monitor::GenericMonitor::on_NtAlertThread()
{
    LOG(INFO, "Break on NtAlertThread");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtAlertThread)
    {
        it(ThreadHandle);
    }
}

void monitor::GenericMonitor::register_NtAllocateLocallyUniqueId(const on_NtAllocateLocallyUniqueId_fn& on_ntallocatelocallyuniqueid)
{
    d_->observers_NtAllocateLocallyUniqueId.push_back(on_ntallocatelocallyuniqueid);
}

void monitor::GenericMonitor::on_NtAllocateLocallyUniqueId()
{
    LOG(INFO, "Break on NtAllocateLocallyUniqueId");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Luid            = nt::cast_to<nt::PLUID>              (args[0]);

    for(const auto& it : d_->observers_NtAllocateLocallyUniqueId)
    {
        it(Luid);
    }
}

void monitor::GenericMonitor::register_NtAllocateReserveObject(const on_NtAllocateReserveObject_fn& on_ntallocatereserveobject)
{
    d_->observers_NtAllocateReserveObject.push_back(on_ntallocatereserveobject);
}

void monitor::GenericMonitor::on_NtAllocateReserveObject()
{
    LOG(INFO, "Break on NtAllocateReserveObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MemoryReserveHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Type            = nt::cast_to<nt::MEMORY_RESERVE_TYPE>(args[2]);

    for(const auto& it : d_->observers_NtAllocateReserveObject)
    {
        it(MemoryReserveHandle, ObjectAttributes, Type);
    }
}

void monitor::GenericMonitor::register_NtAllocateUserPhysicalPages(const on_NtAllocateUserPhysicalPages_fn& on_ntallocateuserphysicalpages)
{
    d_->observers_NtAllocateUserPhysicalPages.push_back(on_ntallocateuserphysicalpages);
}

void monitor::GenericMonitor::on_NtAllocateUserPhysicalPages()
{
    LOG(INFO, "Break on NtAllocateUserPhysicalPages");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::PULONG_PTR>         (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtAllocateUserPhysicalPages)
    {
        it(ProcessHandle, NumberOfPages, UserPfnArra);
    }
}

void monitor::GenericMonitor::register_NtAllocateUuids(const on_NtAllocateUuids_fn& on_ntallocateuuids)
{
    d_->observers_NtAllocateUuids.push_back(on_ntallocateuuids);
}

void monitor::GenericMonitor::on_NtAllocateUuids()
{
    LOG(INFO, "Break on NtAllocateUuids");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Time            = nt::cast_to<nt::PULARGE_INTEGER>    (args[0]);
    const auto Range           = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Sequence        = nt::cast_to<nt::PULONG>             (args[2]);
    const auto Seed            = nt::cast_to<nt::PCHAR>              (args[3]);

    for(const auto& it : d_->observers_NtAllocateUuids)
    {
        it(Time, Range, Sequence, Seed);
    }
}

void monitor::GenericMonitor::register_NtAllocateVirtualMemory(const on_NtAllocateVirtualMemory_fn& on_ntallocatevirtualmemory)
{
    d_->observers_NtAllocateVirtualMemory.push_back(on_ntallocatevirtualmemory);
}

void monitor::GenericMonitor::on_NtAllocateVirtualMemory()
{
    LOG(INFO, "Break on NtAllocateVirtualMemory");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ZeroBits        = nt::cast_to<nt::ULONG_PTR>          (args[2]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[3]);
    const auto AllocationType  = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Protect         = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtAllocateVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
    }
}

void monitor::GenericMonitor::register_NtAlpcAcceptConnectPort(const on_NtAlpcAcceptConnectPort_fn& on_ntalpcacceptconnectport)
{
    d_->observers_NtAlpcAcceptConnectPort.push_back(on_ntalpcacceptconnectport);
}

void monitor::GenericMonitor::on_NtAlpcAcceptConnectPort()
{
    LOG(INFO, "Break on NtAlpcAcceptConnectPort");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ConnectionPortHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[3]);
    const auto PortAttributes  = nt::cast_to<nt::PALPC_PORT_ATTRIBUTES>(args[4]);
    const auto PortContext     = nt::cast_to<nt::PVOID>              (args[5]);
    const auto ConnectionRequest= nt::cast_to<nt::PPORT_MESSAGE>      (args[6]);
    const auto ConnectionMessageAttributes= nt::cast_to<nt::PALPC_MESSAGE_ATTRIBUTES>(args[7]);
    const auto AcceptConnection= nt::cast_to<nt::BOOLEAN>            (args[8]);

    for(const auto& it : d_->observers_NtAlpcAcceptConnectPort)
    {
        it(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
    }
}

void monitor::GenericMonitor::register_NtAlpcCancelMessage(const on_NtAlpcCancelMessage_fn& on_ntalpccancelmessage)
{
    d_->observers_NtAlpcCancelMessage.push_back(on_ntalpccancelmessage);
}

void monitor::GenericMonitor::on_NtAlpcCancelMessage()
{
    LOG(INFO, "Break on NtAlpcCancelMessage");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto MessageContext  = nt::cast_to<nt::PALPC_CONTEXT_ATTR> (args[2]);

    for(const auto& it : d_->observers_NtAlpcCancelMessage)
    {
        it(PortHandle, Flags, MessageContext);
    }
}

void monitor::GenericMonitor::register_NtAlpcConnectPort(const on_NtAlpcConnectPort_fn& on_ntalpcconnectport)
{
    d_->observers_NtAlpcConnectPort.push_back(on_ntalpcconnectport);
}

void monitor::GenericMonitor::on_NtAlpcConnectPort()
{
    LOG(INFO, "Break on NtAlpcConnectPort");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortName        = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto PortAttributes  = nt::cast_to<nt::PALPC_PORT_ATTRIBUTES>(args[3]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[4]);
    const auto RequiredServerSid= nt::cast_to<nt::PSID>               (args[5]);
    const auto ConnectionMessage= nt::cast_to<nt::PPORT_MESSAGE>      (args[6]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[7]);
    const auto OutMessageAttributes= nt::cast_to<nt::PALPC_MESSAGE_ATTRIBUTES>(args[8]);
    const auto InMessageAttributes= nt::cast_to<nt::PALPC_MESSAGE_ATTRIBUTES>(args[9]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[10]);

    for(const auto& it : d_->observers_NtAlpcConnectPort)
    {
        it(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
    }
}

void monitor::GenericMonitor::register_NtAlpcCreatePort(const on_NtAlpcCreatePort_fn& on_ntalpccreateport)
{
    d_->observers_NtAlpcCreatePort.push_back(on_ntalpccreateport);
}

void monitor::GenericMonitor::on_NtAlpcCreatePort()
{
    LOG(INFO, "Break on NtAlpcCreatePort");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto PortAttributes  = nt::cast_to<nt::PALPC_PORT_ATTRIBUTES>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreatePort)
    {
        it(PortHandle, ObjectAttributes, PortAttributes);
    }
}

void monitor::GenericMonitor::register_NtAlpcCreatePortSection(const on_NtAlpcCreatePortSection_fn& on_ntalpccreateportsection)
{
    d_->observers_NtAlpcCreatePortSection.push_back(on_ntalpccreateportsection);
}

void monitor::GenericMonitor::on_NtAlpcCreatePortSection()
{
    LOG(INFO, "Break on NtAlpcCreatePortSection");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto SectionSize     = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto AlpcSectionHandle= nt::cast_to<nt::PALPC_HANDLE>       (args[4]);
    const auto ActualSectionSize= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtAlpcCreatePortSection)
    {
        it(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
    }
}

void monitor::GenericMonitor::register_NtAlpcCreateResourceReserve(const on_NtAlpcCreateResourceReserve_fn& on_ntalpccreateresourcereserve)
{
    d_->observers_NtAlpcCreateResourceReserve.push_back(on_ntalpccreateresourcereserve);
}

void monitor::GenericMonitor::on_NtAlpcCreateResourceReserve()
{
    LOG(INFO, "Break on NtAlpcCreateResourceReserve");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto MessageSize     = nt::cast_to<nt::SIZE_T>             (args[2]);
    const auto ResourceId      = nt::cast_to<nt::PALPC_HANDLE>       (args[3]);

    for(const auto& it : d_->observers_NtAlpcCreateResourceReserve)
    {
        it(PortHandle, Flags, MessageSize, ResourceId);
    }
}

void monitor::GenericMonitor::register_NtAlpcCreateSectionView(const on_NtAlpcCreateSectionView_fn& on_ntalpccreatesectionview)
{
    d_->observers_NtAlpcCreateSectionView.push_back(on_ntalpccreatesectionview);
}

void monitor::GenericMonitor::on_NtAlpcCreateSectionView()
{
    LOG(INFO, "Break on NtAlpcCreateSectionView");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ViewAttributes  = nt::cast_to<nt::PALPC_DATA_VIEW_ATTR>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreateSectionView)
    {
        it(PortHandle, Flags, ViewAttributes);
    }
}

void monitor::GenericMonitor::register_NtAlpcCreateSecurityContext(const on_NtAlpcCreateSecurityContext_fn& on_ntalpccreatesecuritycontext)
{
    d_->observers_NtAlpcCreateSecurityContext.push_back(on_ntalpccreatesecuritycontext);
}

void monitor::GenericMonitor::on_NtAlpcCreateSecurityContext()
{
    LOG(INFO, "Break on NtAlpcCreateSecurityContext");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SecurityAttribute= nt::cast_to<nt::PALPC_SECURITY_ATTR>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreateSecurityContext)
    {
        it(PortHandle, Flags, SecurityAttribute);
    }
}

void monitor::GenericMonitor::register_NtAlpcDeletePortSection(const on_NtAlpcDeletePortSection_fn& on_ntalpcdeleteportsection)
{
    d_->observers_NtAlpcDeletePortSection.push_back(on_ntalpcdeleteportsection);
}

void monitor::GenericMonitor::on_NtAlpcDeletePortSection()
{
    LOG(INFO, "Break on NtAlpcDeletePortSection");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SectionHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeletePortSection)
    {
        it(PortHandle, Flags, SectionHandle);
    }
}

void monitor::GenericMonitor::register_NtAlpcDeleteResourceReserve(const on_NtAlpcDeleteResourceReserve_fn& on_ntalpcdeleteresourcereserve)
{
    d_->observers_NtAlpcDeleteResourceReserve.push_back(on_ntalpcdeleteresourcereserve);
}

void monitor::GenericMonitor::on_NtAlpcDeleteResourceReserve()
{
    LOG(INFO, "Break on NtAlpcDeleteResourceReserve");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ResourceId      = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteResourceReserve)
    {
        it(PortHandle, Flags, ResourceId);
    }
}

void monitor::GenericMonitor::register_NtAlpcDeleteSectionView(const on_NtAlpcDeleteSectionView_fn& on_ntalpcdeletesectionview)
{
    d_->observers_NtAlpcDeleteSectionView.push_back(on_ntalpcdeletesectionview);
}

void monitor::GenericMonitor::on_NtAlpcDeleteSectionView()
{
    LOG(INFO, "Break on NtAlpcDeleteSectionView");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ViewBase        = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteSectionView)
    {
        it(PortHandle, Flags, ViewBase);
    }
}

void monitor::GenericMonitor::register_NtAlpcDeleteSecurityContext(const on_NtAlpcDeleteSecurityContext_fn& on_ntalpcdeletesecuritycontext)
{
    d_->observers_NtAlpcDeleteSecurityContext.push_back(on_ntalpcdeletesecuritycontext);
}

void monitor::GenericMonitor::on_NtAlpcDeleteSecurityContext()
{
    LOG(INFO, "Break on NtAlpcDeleteSecurityContext");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteSecurityContext)
    {
        it(PortHandle, Flags, ContextHandle);
    }
}

void monitor::GenericMonitor::register_NtAlpcDisconnectPort(const on_NtAlpcDisconnectPort_fn& on_ntalpcdisconnectport)
{
    d_->observers_NtAlpcDisconnectPort.push_back(on_ntalpcdisconnectport);
}

void monitor::GenericMonitor::on_NtAlpcDisconnectPort()
{
    LOG(INFO, "Break on NtAlpcDisconnectPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtAlpcDisconnectPort)
    {
        it(PortHandle, Flags);
    }
}

void monitor::GenericMonitor::register_NtAlpcImpersonateClientOfPort(const on_NtAlpcImpersonateClientOfPort_fn& on_ntalpcimpersonateclientofport)
{
    d_->observers_NtAlpcImpersonateClientOfPort.push_back(on_ntalpcimpersonateclientofport);
}

void monitor::GenericMonitor::on_NtAlpcImpersonateClientOfPort()
{
    LOG(INFO, "Break on NtAlpcImpersonateClientOfPort");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto Reserved        = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtAlpcImpersonateClientOfPort)
    {
        it(PortHandle, PortMessage, Reserved);
    }
}

void monitor::GenericMonitor::register_NtAlpcOpenSenderProcess(const on_NtAlpcOpenSenderProcess_fn& on_ntalpcopensenderprocess)
{
    d_->observers_NtAlpcOpenSenderProcess.push_back(on_ntalpcopensenderprocess);
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderProcess()
{
    LOG(INFO, "Break on NtAlpcOpenSenderProcess");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[5]);

    for(const auto& it : d_->observers_NtAlpcOpenSenderProcess)
    {
        it(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtAlpcOpenSenderThread(const on_NtAlpcOpenSenderThread_fn& on_ntalpcopensenderthread)
{
    d_->observers_NtAlpcOpenSenderThread.push_back(on_ntalpcopensenderthread);
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderThread()
{
    LOG(INFO, "Break on NtAlpcOpenSenderThread");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[5]);

    for(const auto& it : d_->observers_NtAlpcOpenSenderThread)
    {
        it(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtAlpcQueryInformation(const on_NtAlpcQueryInformation_fn& on_ntalpcqueryinformation)
{
    d_->observers_NtAlpcQueryInformation.push_back(on_ntalpcqueryinformation);
}

void monitor::GenericMonitor::on_NtAlpcQueryInformation()
{
    LOG(INFO, "Break on NtAlpcQueryInformation");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::ALPC_PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtAlpcQueryInformation)
    {
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtAlpcQueryInformationMessage(const on_NtAlpcQueryInformationMessage_fn& on_ntalpcqueryinformationmessage)
{
    d_->observers_NtAlpcQueryInformationMessage.push_back(on_ntalpcqueryinformationmessage);
}

void monitor::GenericMonitor::on_NtAlpcQueryInformationMessage()
{
    LOG(INFO, "Break on NtAlpcQueryInformationMessage");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto MessageInformationClass= nt::cast_to<nt::ALPC_MESSAGE_INFORMATION_CLASS>(args[2]);
    const auto MessageInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAlpcQueryInformationMessage)
    {
        it(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtAlpcRevokeSecurityContext(const on_NtAlpcRevokeSecurityContext_fn& on_ntalpcrevokesecuritycontext)
{
    d_->observers_NtAlpcRevokeSecurityContext.push_back(on_ntalpcrevokesecuritycontext);
}

void monitor::GenericMonitor::on_NtAlpcRevokeSecurityContext()
{
    LOG(INFO, "Break on NtAlpcRevokeSecurityContext");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcRevokeSecurityContext)
    {
        it(PortHandle, Flags, ContextHandle);
    }
}

void monitor::GenericMonitor::register_NtAlpcSendWaitReceivePort(const on_NtAlpcSendWaitReceivePort_fn& on_ntalpcsendwaitreceiveport)
{
    d_->observers_NtAlpcSendWaitReceivePort.push_back(on_ntalpcsendwaitreceiveport);
}

void monitor::GenericMonitor::on_NtAlpcSendWaitReceivePort()
{
    LOG(INFO, "Break on NtAlpcSendWaitReceivePort");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SendMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto SendMessageAttributes= nt::cast_to<nt::PALPC_MESSAGE_ATTRIBUTES>(args[3]);
    const auto ReceiveMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[4]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[5]);
    const auto ReceiveMessageAttributes= nt::cast_to<nt::PALPC_MESSAGE_ATTRIBUTES>(args[6]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);

    for(const auto& it : d_->observers_NtAlpcSendWaitReceivePort)
    {
        it(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
    }
}

void monitor::GenericMonitor::register_NtAlpcSetInformation(const on_NtAlpcSetInformation_fn& on_ntalpcsetinformation)
{
    d_->observers_NtAlpcSetInformation.push_back(on_ntalpcsetinformation);
}

void monitor::GenericMonitor::on_NtAlpcSetInformation()
{
    LOG(INFO, "Break on NtAlpcSetInformation");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::ALPC_PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtAlpcSetInformation)
    {
        it(PortHandle, PortInformationClass, PortInformation, Length);
    }
}

void monitor::GenericMonitor::register_NtApphelpCacheControl(const on_NtApphelpCacheControl_fn& on_ntapphelpcachecontrol)
{
    d_->observers_NtApphelpCacheControl.push_back(on_ntapphelpcachecontrol);
}

void monitor::GenericMonitor::on_NtApphelpCacheControl()
{
    LOG(INFO, "Break on NtApphelpCacheControl");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto type            = nt::cast_to<nt::APPHELPCOMMAND>     (args[0]);
    const auto buf             = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtApphelpCacheControl)
    {
        it(type, buf);
    }
}

void monitor::GenericMonitor::register_NtAreMappedFilesTheSame(const on_NtAreMappedFilesTheSame_fn& on_ntaremappedfilesthesame)
{
    d_->observers_NtAreMappedFilesTheSame.push_back(on_ntaremappedfilesthesame);
}

void monitor::GenericMonitor::on_NtAreMappedFilesTheSame()
{
    LOG(INFO, "Break on NtAreMappedFilesTheSame");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto File1MappedAsAnImage= nt::cast_to<nt::PVOID>              (args[0]);
    const auto File2MappedAsFile= nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtAreMappedFilesTheSame)
    {
        it(File1MappedAsAnImage, File2MappedAsFile);
    }
}

void monitor::GenericMonitor::register_NtAssignProcessToJobObject(const on_NtAssignProcessToJobObject_fn& on_ntassignprocesstojobobject)
{
    d_->observers_NtAssignProcessToJobObject.push_back(on_ntassignprocesstojobobject);
}

void monitor::GenericMonitor::on_NtAssignProcessToJobObject()
{
    LOG(INFO, "Break on NtAssignProcessToJobObject");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtAssignProcessToJobObject)
    {
        it(JobHandle, ProcessHandle);
    }
}

void monitor::GenericMonitor::register_NtCallbackReturn(const on_NtCallbackReturn_fn& on_ntcallbackreturn)
{
    d_->observers_NtCallbackReturn.push_back(on_ntcallbackreturn);
}

void monitor::GenericMonitor::on_NtCallbackReturn()
{
    LOG(INFO, "Break on NtCallbackReturn");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[0]);
    const auto OutputLength    = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Status          = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtCallbackReturn)
    {
        it(OutputBuffer, OutputLength, Status);
    }
}

void monitor::GenericMonitor::register_NtCancelIoFileEx(const on_NtCancelIoFileEx_fn& on_ntcanceliofileex)
{
    d_->observers_NtCancelIoFileEx.push_back(on_ntcanceliofileex);
}

void monitor::GenericMonitor::on_NtCancelIoFileEx()
{
    LOG(INFO, "Break on NtCancelIoFileEx");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoRequestToCancel= nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[2]);

    for(const auto& it : d_->observers_NtCancelIoFileEx)
    {
        it(FileHandle, IoRequestToCancel, IoStatusBlock);
    }
}

void monitor::GenericMonitor::register_NtCancelIoFile(const on_NtCancelIoFile_fn& on_ntcanceliofile)
{
    d_->observers_NtCancelIoFile.push_back(on_ntcanceliofile);
}

void monitor::GenericMonitor::on_NtCancelIoFile()
{
    LOG(INFO, "Break on NtCancelIoFile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);

    for(const auto& it : d_->observers_NtCancelIoFile)
    {
        it(FileHandle, IoStatusBlock);
    }
}

void monitor::GenericMonitor::register_NtCancelSynchronousIoFile(const on_NtCancelSynchronousIoFile_fn& on_ntcancelsynchronousiofile)
{
    d_->observers_NtCancelSynchronousIoFile.push_back(on_ntcancelsynchronousiofile);
}

void monitor::GenericMonitor::on_NtCancelSynchronousIoFile()
{
    LOG(INFO, "Break on NtCancelSynchronousIoFile");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoRequestToCancel= nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[2]);

    for(const auto& it : d_->observers_NtCancelSynchronousIoFile)
    {
        it(ThreadHandle, IoRequestToCancel, IoStatusBlock);
    }
}

void monitor::GenericMonitor::register_NtCancelTimer(const on_NtCancelTimer_fn& on_ntcanceltimer)
{
    d_->observers_NtCancelTimer.push_back(on_ntcanceltimer);
}

void monitor::GenericMonitor::on_NtCancelTimer()
{
    LOG(INFO, "Break on NtCancelTimer");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto CurrentState    = nt::cast_to<nt::PBOOLEAN>           (args[1]);

    for(const auto& it : d_->observers_NtCancelTimer)
    {
        it(TimerHandle, CurrentState);
    }
}

void monitor::GenericMonitor::register_NtClearEvent(const on_NtClearEvent_fn& on_ntclearevent)
{
    d_->observers_NtClearEvent.push_back(on_ntclearevent);
}

void monitor::GenericMonitor::on_NtClearEvent()
{
    LOG(INFO, "Break on NtClearEvent");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtClearEvent)
    {
        it(EventHandle);
    }
}

void monitor::GenericMonitor::register_NtClose(const on_NtClose_fn& on_ntclose)
{
    d_->observers_NtClose.push_back(on_ntclose);
}

void monitor::GenericMonitor::on_NtClose()
{
    LOG(INFO, "Break on NtClose");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtClose)
    {
        it(Handle);
    }
}

void monitor::GenericMonitor::register_NtCloseObjectAuditAlarm(const on_NtCloseObjectAuditAlarm_fn& on_ntcloseobjectauditalarm)
{
    d_->observers_NtCloseObjectAuditAlarm.push_back(on_ntcloseobjectauditalarm);
}

void monitor::GenericMonitor::on_NtCloseObjectAuditAlarm()
{
    LOG(INFO, "Break on NtCloseObjectAuditAlarm");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto GenerateOnClose = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtCloseObjectAuditAlarm)
    {
        it(SubsystemName, HandleId, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtCommitComplete(const on_NtCommitComplete_fn& on_ntcommitcomplete)
{
    d_->observers_NtCommitComplete.push_back(on_ntcommitcomplete);
}

void monitor::GenericMonitor::on_NtCommitComplete()
{
    LOG(INFO, "Break on NtCommitComplete");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtCommitComplete)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtCommitEnlistment(const on_NtCommitEnlistment_fn& on_ntcommitenlistment)
{
    d_->observers_NtCommitEnlistment.push_back(on_ntcommitenlistment);
}

void monitor::GenericMonitor::on_NtCommitEnlistment()
{
    LOG(INFO, "Break on NtCommitEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtCommitEnlistment)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtCommitTransaction(const on_NtCommitTransaction_fn& on_ntcommittransaction)
{
    d_->observers_NtCommitTransaction.push_back(on_ntcommittransaction);
}

void monitor::GenericMonitor::on_NtCommitTransaction()
{
    LOG(INFO, "Break on NtCommitTransaction");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Wait            = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtCommitTransaction)
    {
        it(TransactionHandle, Wait);
    }
}

void monitor::GenericMonitor::register_NtCompactKeys(const on_NtCompactKeys_fn& on_ntcompactkeys)
{
    d_->observers_NtCompactKeys.push_back(on_ntcompactkeys);
}

void monitor::GenericMonitor::on_NtCompactKeys()
{
    LOG(INFO, "Break on NtCompactKeys");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto KeyArray        = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtCompactKeys)
    {
        it(Count, KeyArray);
    }
}

void monitor::GenericMonitor::register_NtCompareTokens(const on_NtCompareTokens_fn& on_ntcomparetokens)
{
    d_->observers_NtCompareTokens.push_back(on_ntcomparetokens);
}

void monitor::GenericMonitor::on_NtCompareTokens()
{
    LOG(INFO, "Break on NtCompareTokens");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FirstTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecondTokenHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Equal           = nt::cast_to<nt::PBOOLEAN>           (args[2]);

    for(const auto& it : d_->observers_NtCompareTokens)
    {
        it(FirstTokenHandle, SecondTokenHandle, Equal);
    }
}

void monitor::GenericMonitor::register_NtCompleteConnectPort(const on_NtCompleteConnectPort_fn& on_ntcompleteconnectport)
{
    d_->observers_NtCompleteConnectPort.push_back(on_ntcompleteconnectport);
}

void monitor::GenericMonitor::on_NtCompleteConnectPort()
{
    LOG(INFO, "Break on NtCompleteConnectPort");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtCompleteConnectPort)
    {
        it(PortHandle);
    }
}

void monitor::GenericMonitor::register_NtCompressKey(const on_NtCompressKey_fn& on_ntcompresskey)
{
    d_->observers_NtCompressKey.push_back(on_ntcompresskey);
}

void monitor::GenericMonitor::on_NtCompressKey()
{
    LOG(INFO, "Break on NtCompressKey");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Key             = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtCompressKey)
    {
        it(Key);
    }
}

void monitor::GenericMonitor::register_NtConnectPort(const on_NtConnectPort_fn& on_ntconnectport)
{
    d_->observers_NtConnectPort.push_back(on_ntconnectport);
}

void monitor::GenericMonitor::on_NtConnectPort()
{
    LOG(INFO, "Break on NtConnectPort");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortName        = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto SecurityQos     = nt::cast_to<nt::PSECURITY_QUALITY_OF_SERVICE>(args[2]);
    const auto ClientView      = nt::cast_to<nt::PPORT_VIEW>         (args[3]);
    const auto ServerView      = nt::cast_to<nt::PREMOTE_PORT_VIEW>  (args[4]);
    const auto MaxMessageLength= nt::cast_to<nt::PULONG>             (args[5]);
    const auto ConnectionInformation= nt::cast_to<nt::PVOID>              (args[6]);
    const auto ConnectionInformationLength= nt::cast_to<nt::PULONG>             (args[7]);

    for(const auto& it : d_->observers_NtConnectPort)
    {
        it(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }
}

void monitor::GenericMonitor::register_NtContinue(const on_NtContinue_fn& on_ntcontinue)
{
    d_->observers_NtContinue.push_back(on_ntcontinue);
}

void monitor::GenericMonitor::on_NtContinue()
{
    LOG(INFO, "Break on NtContinue");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ContextRecord   = nt::cast_to<nt::PCONTEXT>           (args[0]);
    const auto TestAlert       = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtContinue)
    {
        it(ContextRecord, TestAlert);
    }
}

void monitor::GenericMonitor::register_NtCreateDebugObject(const on_NtCreateDebugObject_fn& on_ntcreatedebugobject)
{
    d_->observers_NtCreateDebugObject.push_back(on_ntcreatedebugobject);
}

void monitor::GenericMonitor::on_NtCreateDebugObject()
{
    LOG(INFO, "Break on NtCreateDebugObject");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateDebugObject)
    {
        it(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
    }
}

void monitor::GenericMonitor::register_NtCreateDirectoryObject(const on_NtCreateDirectoryObject_fn& on_ntcreatedirectoryobject)
{
    d_->observers_NtCreateDirectoryObject.push_back(on_ntcreatedirectoryobject);
}

void monitor::GenericMonitor::on_NtCreateDirectoryObject()
{
    LOG(INFO, "Break on NtCreateDirectoryObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateDirectoryObject)
    {
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtCreateEnlistment(const on_NtCreateEnlistment_fn& on_ntcreateenlistment)
{
    d_->observers_NtCreateEnlistment.push_back(on_ntcreateenlistment);
}

void monitor::GenericMonitor::on_NtCreateEnlistment()
{
    LOG(INFO, "Break on NtCreateEnlistment");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[2]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto NotificationMask= nt::cast_to<nt::NOTIFICATION_MASK>  (args[6]);
    const auto EnlistmentKey   = nt::cast_to<nt::PVOID>              (args[7]);

    for(const auto& it : d_->observers_NtCreateEnlistment)
    {
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
    }
}

void monitor::GenericMonitor::register_NtCreateEvent(const on_NtCreateEvent_fn& on_ntcreateevent)
{
    d_->observers_NtCreateEvent.push_back(on_ntcreateevent);
}

void monitor::GenericMonitor::on_NtCreateEvent()
{
    LOG(INFO, "Break on NtCreateEvent");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto EventType       = nt::cast_to<nt::EVENT_TYPE>         (args[3]);
    const auto InitialState    = nt::cast_to<nt::BOOLEAN>            (args[4]);

    for(const auto& it : d_->observers_NtCreateEvent)
    {
        it(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
    }
}

void monitor::GenericMonitor::register_NtCreateEventPair(const on_NtCreateEventPair_fn& on_ntcreateeventpair)
{
    d_->observers_NtCreateEventPair.push_back(on_ntcreateeventpair);
}

void monitor::GenericMonitor::on_NtCreateEventPair()
{
    LOG(INFO, "Break on NtCreateEventPair");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateEventPair)
    {
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtCreateFile(const on_NtCreateFile_fn& on_ntcreatefile)
{
    d_->observers_NtCreateFile.push_back(on_ntcreatefile);
}

void monitor::GenericMonitor::on_NtCreateFile()
{
    LOG(INFO, "Break on NtCreateFile");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto AllocationSize  = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);
    const auto FileAttributes  = nt::cast_to<nt::ULONG>              (args[5]);
    const auto ShareAccess     = nt::cast_to<nt::ULONG>              (args[6]);
    const auto CreateDisposition= nt::cast_to<nt::ULONG>              (args[7]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[8]);
    const auto EaBuffer        = nt::cast_to<nt::PVOID>              (args[9]);
    const auto EaLength        = nt::cast_to<nt::ULONG>              (args[10]);

    for(const auto& it : d_->observers_NtCreateFile)
    {
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
    }
}

void monitor::GenericMonitor::register_NtCreateIoCompletion(const on_NtCreateIoCompletion_fn& on_ntcreateiocompletion)
{
    d_->observers_NtCreateIoCompletion.push_back(on_ntcreateiocompletion);
}

void monitor::GenericMonitor::on_NtCreateIoCompletion()
{
    LOG(INFO, "Break on NtCreateIoCompletion");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateIoCompletion)
    {
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
    }
}

void monitor::GenericMonitor::register_NtCreateJobObject(const on_NtCreateJobObject_fn& on_ntcreatejobobject)
{
    d_->observers_NtCreateJobObject.push_back(on_ntcreatejobobject);
}

void monitor::GenericMonitor::on_NtCreateJobObject()
{
    LOG(INFO, "Break on NtCreateJobObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateJobObject)
    {
        it(JobHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtCreateJobSet(const on_NtCreateJobSet_fn& on_ntcreatejobset)
{
    d_->observers_NtCreateJobSet.push_back(on_ntcreatejobset);
}

void monitor::GenericMonitor::on_NtCreateJobSet()
{
    LOG(INFO, "Break on NtCreateJobSet");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NumJob          = nt::cast_to<nt::ULONG>              (args[0]);
    const auto UserJobSet      = nt::cast_to<nt::PJOB_SET_ARRAY>     (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtCreateJobSet)
    {
        it(NumJob, UserJobSet, Flags);
    }
}

void monitor::GenericMonitor::register_NtCreateKeyedEvent(const on_NtCreateKeyedEvent_fn& on_ntcreatekeyedevent)
{
    d_->observers_NtCreateKeyedEvent.push_back(on_ntcreatekeyedevent);
}

void monitor::GenericMonitor::on_NtCreateKeyedEvent()
{
    LOG(INFO, "Break on NtCreateKeyedEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateKeyedEvent)
    {
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
    }
}

void monitor::GenericMonitor::register_NtCreateKey(const on_NtCreateKey_fn& on_ntcreatekey)
{
    d_->observers_NtCreateKey.push_back(on_ntcreatekey);
}

void monitor::GenericMonitor::on_NtCreateKey()
{
    LOG(INFO, "Break on NtCreateKey");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TitleIndex      = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Class           = nt::cast_to<nt::PUNICODE_STRING>    (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto Disposition     = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtCreateKey)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
    }
}

void monitor::GenericMonitor::register_NtCreateKeyTransacted(const on_NtCreateKeyTransacted_fn& on_ntcreatekeytransacted)
{
    d_->observers_NtCreateKeyTransacted.push_back(on_ntcreatekeytransacted);
}

void monitor::GenericMonitor::on_NtCreateKeyTransacted()
{
    LOG(INFO, "Break on NtCreateKeyTransacted");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TitleIndex      = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Class           = nt::cast_to<nt::PUNICODE_STRING>    (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[6]);
    const auto Disposition     = nt::cast_to<nt::PULONG>             (args[7]);

    for(const auto& it : d_->observers_NtCreateKeyTransacted)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
    }
}

void monitor::GenericMonitor::register_NtCreateMailslotFile(const on_NtCreateMailslotFile_fn& on_ntcreatemailslotfile)
{
    d_->observers_NtCreateMailslotFile.push_back(on_ntcreatemailslotfile);
}

void monitor::GenericMonitor::on_NtCreateMailslotFile()
{
    LOG(INFO, "Break on NtCreateMailslotFile");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[4]);
    const auto MailslotQuota   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto MaximumMessageSize= nt::cast_to<nt::ULONG>              (args[6]);
    const auto ReadTimeout     = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);

    for(const auto& it : d_->observers_NtCreateMailslotFile)
    {
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
    }
}

void monitor::GenericMonitor::register_NtCreateMutant(const on_NtCreateMutant_fn& on_ntcreatemutant)
{
    d_->observers_NtCreateMutant.push_back(on_ntcreatemutant);
}

void monitor::GenericMonitor::on_NtCreateMutant()
{
    LOG(INFO, "Break on NtCreateMutant");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto InitialOwner    = nt::cast_to<nt::BOOLEAN>            (args[3]);

    for(const auto& it : d_->observers_NtCreateMutant)
    {
        it(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
    }
}

void monitor::GenericMonitor::register_NtCreateNamedPipeFile(const on_NtCreateNamedPipeFile_fn& on_ntcreatenamedpipefile)
{
    d_->observers_NtCreateNamedPipeFile.push_back(on_ntcreatenamedpipefile);
}

void monitor::GenericMonitor::on_NtCreateNamedPipeFile()
{
    LOG(INFO, "Break on NtCreateNamedPipeFile");
    const auto nargs = 14;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto ShareAccess     = nt::cast_to<nt::ULONG>              (args[4]);
    const auto CreateDisposition= nt::cast_to<nt::ULONG>              (args[5]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[6]);
    const auto NamedPipeType   = nt::cast_to<nt::ULONG>              (args[7]);
    const auto ReadMode        = nt::cast_to<nt::ULONG>              (args[8]);
    const auto CompletionMode  = nt::cast_to<nt::ULONG>              (args[9]);
    const auto MaximumInstances= nt::cast_to<nt::ULONG>              (args[10]);
    const auto InboundQuota    = nt::cast_to<nt::ULONG>              (args[11]);
    const auto OutboundQuota   = nt::cast_to<nt::ULONG>              (args[12]);
    const auto DefaultTimeout  = nt::cast_to<nt::PLARGE_INTEGER>     (args[13]);

    for(const auto& it : d_->observers_NtCreateNamedPipeFile)
    {
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
    }
}

void monitor::GenericMonitor::register_NtCreatePagingFile(const on_NtCreatePagingFile_fn& on_ntcreatepagingfile)
{
    d_->observers_NtCreatePagingFile.push_back(on_ntcreatepagingfile);
}

void monitor::GenericMonitor::on_NtCreatePagingFile()
{
    LOG(INFO, "Break on NtCreatePagingFile");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PageFileName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto MinimumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);
    const auto MaximumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto Priority        = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreatePagingFile)
    {
        it(PageFileName, MinimumSize, MaximumSize, Priority);
    }
}

void monitor::GenericMonitor::register_NtCreatePort(const on_NtCreatePort_fn& on_ntcreateport)
{
    d_->observers_NtCreatePort.push_back(on_ntcreateport);
}

void monitor::GenericMonitor::on_NtCreatePort()
{
    LOG(INFO, "Break on NtCreatePort");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto MaxConnectionInfoLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto MaxMessageLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto MaxPoolUsage    = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtCreatePort)
    {
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }
}

void monitor::GenericMonitor::register_NtCreatePrivateNamespace(const on_NtCreatePrivateNamespace_fn& on_ntcreateprivatenamespace)
{
    d_->observers_NtCreatePrivateNamespace.push_back(on_ntcreateprivatenamespace);
}

void monitor::GenericMonitor::on_NtCreatePrivateNamespace()
{
    LOG(INFO, "Break on NtCreatePrivateNamespace");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto BoundaryDescriptor= nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtCreatePrivateNamespace)
    {
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }
}

void monitor::GenericMonitor::register_NtCreateProcessEx(const on_NtCreateProcessEx_fn& on_ntcreateprocessex)
{
    d_->observers_NtCreateProcessEx.push_back(on_ntcreateprocessex);
}

void monitor::GenericMonitor::on_NtCreateProcessEx()
{
    LOG(INFO, "Break on NtCreateProcessEx");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ParentProcess   = nt::cast_to<nt::HANDLE>             (args[3]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[4]);
    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[5]);
    const auto DebugPort       = nt::cast_to<nt::HANDLE>             (args[6]);
    const auto ExceptionPort   = nt::cast_to<nt::HANDLE>             (args[7]);
    const auto JobMemberLevel  = nt::cast_to<nt::ULONG>              (args[8]);

    for(const auto& it : d_->observers_NtCreateProcessEx)
    {
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
    }
}

void monitor::GenericMonitor::register_NtCreateProcess(const on_NtCreateProcess_fn& on_ntcreateprocess)
{
    d_->observers_NtCreateProcess.push_back(on_ntcreateprocess);
}

void monitor::GenericMonitor::on_NtCreateProcess()
{
    LOG(INFO, "Break on NtCreateProcess");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ParentProcess   = nt::cast_to<nt::HANDLE>             (args[3]);
    const auto InheritObjectTable= nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[5]);
    const auto DebugPort       = nt::cast_to<nt::HANDLE>             (args[6]);
    const auto ExceptionPort   = nt::cast_to<nt::HANDLE>             (args[7]);

    for(const auto& it : d_->observers_NtCreateProcess)
    {
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
    }
}

void monitor::GenericMonitor::register_NtCreateProfileEx(const on_NtCreateProfileEx_fn& on_ntcreateprofileex)
{
    d_->observers_NtCreateProfileEx.push_back(on_ntcreateprofileex);
}

void monitor::GenericMonitor::on_NtCreateProfileEx()
{
    LOG(INFO, "Break on NtCreateProfileEx");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto Process         = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ProfileBase     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ProfileSize     = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto BucketSize      = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Buffer          = nt::cast_to<nt::PULONG>             (args[5]);
    const auto BufferSize      = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ProfileSource   = nt::cast_to<nt::KPROFILE_SOURCE>    (args[7]);
    const auto GroupAffinityCount= nt::cast_to<nt::ULONG>              (args[8]);
    const auto GroupAffinity   = nt::cast_to<nt::PGROUP_AFFINITY>    (args[9]);

    for(const auto& it : d_->observers_NtCreateProfileEx)
    {
        it(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
    }
}

void monitor::GenericMonitor::register_NtCreateProfile(const on_NtCreateProfile_fn& on_ntcreateprofile)
{
    d_->observers_NtCreateProfile.push_back(on_ntcreateprofile);
}

void monitor::GenericMonitor::on_NtCreateProfile()
{
    LOG(INFO, "Break on NtCreateProfile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto Process         = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto RangeBase       = nt::cast_to<nt::PVOID>              (args[2]);
    const auto RangeSize       = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto BucketSize      = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Buffer          = nt::cast_to<nt::PULONG>             (args[5]);
    const auto BufferSize      = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ProfileSource   = nt::cast_to<nt::KPROFILE_SOURCE>    (args[7]);
    const auto Affinity        = nt::cast_to<nt::KAFFINITY>          (args[8]);

    for(const auto& it : d_->observers_NtCreateProfile)
    {
        it(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
    }
}

void monitor::GenericMonitor::register_NtCreateResourceManager(const on_NtCreateResourceManager_fn& on_ntcreateresourcemanager)
{
    d_->observers_NtCreateResourceManager.push_back(on_ntcreateresourcemanager);
}

void monitor::GenericMonitor::on_NtCreateResourceManager()
{
    LOG(INFO, "Break on NtCreateResourceManager");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto RmGuid          = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto Description     = nt::cast_to<nt::PUNICODE_STRING>    (args[6]);

    for(const auto& it : d_->observers_NtCreateResourceManager)
    {
        it(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
    }
}

void monitor::GenericMonitor::register_NtCreateSection(const on_NtCreateSection_fn& on_ntcreatesection)
{
    d_->observers_NtCreateSection.push_back(on_ntcreatesection);
}

void monitor::GenericMonitor::on_NtCreateSection()
{
    LOG(INFO, "Break on NtCreateSection");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto MaximumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto SectionPageProtection= nt::cast_to<nt::ULONG>              (args[4]);
    const auto AllocationAttributes= nt::cast_to<nt::ULONG>              (args[5]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[6]);

    for(const auto& it : d_->observers_NtCreateSection)
    {
        it(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
    }
}

void monitor::GenericMonitor::register_NtCreateSemaphore(const on_NtCreateSemaphore_fn& on_ntcreatesemaphore)
{
    d_->observers_NtCreateSemaphore.push_back(on_ntcreatesemaphore);
}

void monitor::GenericMonitor::on_NtCreateSemaphore()
{
    LOG(INFO, "Break on NtCreateSemaphore");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto InitialCount    = nt::cast_to<nt::LONG>               (args[3]);
    const auto MaximumCount    = nt::cast_to<nt::LONG>               (args[4]);

    for(const auto& it : d_->observers_NtCreateSemaphore)
    {
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
    }
}

void monitor::GenericMonitor::register_NtCreateSymbolicLinkObject(const on_NtCreateSymbolicLinkObject_fn& on_ntcreatesymboliclinkobject)
{
    d_->observers_NtCreateSymbolicLinkObject.push_back(on_ntcreatesymboliclinkobject);
}

void monitor::GenericMonitor::on_NtCreateSymbolicLinkObject()
{
    LOG(INFO, "Break on NtCreateSymbolicLinkObject");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LinkTarget      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);

    for(const auto& it : d_->observers_NtCreateSymbolicLinkObject)
    {
        it(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
    }
}

void monitor::GenericMonitor::register_NtCreateThreadEx(const on_NtCreateThreadEx_fn& on_ntcreatethreadex)
{
    d_->observers_NtCreateThreadEx.push_back(on_ntcreatethreadex);
}

void monitor::GenericMonitor::on_NtCreateThreadEx()
{
    LOG(INFO, "Break on NtCreateThreadEx");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[3]);
    const auto StartRoutine    = nt::cast_to<nt::PVOID>              (args[4]);
    const auto Argument        = nt::cast_to<nt::PVOID>              (args[5]);
    const auto CreateFlags     = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ZeroBits        = nt::cast_to<nt::ULONG_PTR>          (args[7]);
    const auto StackSize       = nt::cast_to<nt::SIZE_T>             (args[8]);
    const auto MaximumStackSize= nt::cast_to<nt::SIZE_T>             (args[9]);
    const auto AttributeList   = nt::cast_to<nt::PPS_ATTRIBUTE_LIST> (args[10]);

    for(const auto& it : d_->observers_NtCreateThreadEx)
    {
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
    }
}

void monitor::GenericMonitor::register_NtCreateThread(const on_NtCreateThread_fn& on_ntcreatethread)
{
    d_->observers_NtCreateThread.push_back(on_ntcreatethread);
}

void monitor::GenericMonitor::on_NtCreateThread()
{
    LOG(INFO, "Break on NtCreateThread");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[3]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[4]);
    const auto ThreadContext   = nt::cast_to<nt::PCONTEXT>           (args[5]);
    const auto InitialTeb      = nt::cast_to<nt::PINITIAL_TEB>       (args[6]);
    const auto CreateSuspended = nt::cast_to<nt::BOOLEAN>            (args[7]);

    for(const auto& it : d_->observers_NtCreateThread)
    {
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    }
}

void monitor::GenericMonitor::register_NtCreateTimer(const on_NtCreateTimer_fn& on_ntcreatetimer)
{
    d_->observers_NtCreateTimer.push_back(on_ntcreatetimer);
}

void monitor::GenericMonitor::on_NtCreateTimer()
{
    LOG(INFO, "Break on NtCreateTimer");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TimerType       = nt::cast_to<nt::TIMER_TYPE>         (args[3]);

    for(const auto& it : d_->observers_NtCreateTimer)
    {
        it(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
    }
}

void monitor::GenericMonitor::register_NtCreateToken(const on_NtCreateToken_fn& on_ntcreatetoken)
{
    d_->observers_NtCreateToken.push_back(on_ntcreatetoken);
}

void monitor::GenericMonitor::on_NtCreateToken()
{
    LOG(INFO, "Break on NtCreateToken");
    const auto nargs = 13;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TokenType       = nt::cast_to<nt::TOKEN_TYPE>         (args[3]);
    const auto AuthenticationId= nt::cast_to<nt::PLUID>              (args[4]);
    const auto ExpirationTime  = nt::cast_to<nt::PLARGE_INTEGER>     (args[5]);
    const auto User            = nt::cast_to<nt::PTOKEN_USER>        (args[6]);
    const auto Groups          = nt::cast_to<nt::PTOKEN_GROUPS>      (args[7]);
    const auto Privileges      = nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[8]);
    const auto Owner           = nt::cast_to<nt::PTOKEN_OWNER>       (args[9]);
    const auto PrimaryGroup    = nt::cast_to<nt::PTOKEN_PRIMARY_GROUP>(args[10]);
    const auto DefaultDacl     = nt::cast_to<nt::PTOKEN_DEFAULT_DACL>(args[11]);
    const auto TokenSource     = nt::cast_to<nt::PTOKEN_SOURCE>      (args[12]);

    for(const auto& it : d_->observers_NtCreateToken)
    {
        it(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
    }
}

void monitor::GenericMonitor::register_NtCreateTransactionManager(const on_NtCreateTransactionManager_fn& on_ntcreatetransactionmanager)
{
    d_->observers_NtCreateTransactionManager.push_back(on_ntcreatetransactionmanager);
}

void monitor::GenericMonitor::on_NtCreateTransactionManager()
{
    LOG(INFO, "Break on NtCreateTransactionManager");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[4]);
    const auto CommitStrength  = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtCreateTransactionManager)
    {
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
    }
}

void monitor::GenericMonitor::register_NtCreateTransaction(const on_NtCreateTransaction_fn& on_ntcreatetransaction)
{
    d_->observers_NtCreateTransaction.push_back(on_ntcreatetransaction);
}

void monitor::GenericMonitor::on_NtCreateTransaction()
{
    LOG(INFO, "Break on NtCreateTransaction");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Uow             = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto IsolationLevel  = nt::cast_to<nt::ULONG>              (args[6]);
    const auto IsolationFlags  = nt::cast_to<nt::ULONG>              (args[7]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[8]);
    const auto Description     = nt::cast_to<nt::PUNICODE_STRING>    (args[9]);

    for(const auto& it : d_->observers_NtCreateTransaction)
    {
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
    }
}

void monitor::GenericMonitor::register_NtCreateUserProcess(const on_NtCreateUserProcess_fn& on_ntcreateuserprocess)
{
    d_->observers_NtCreateUserProcess.push_back(on_ntcreateuserprocess);
}

void monitor::GenericMonitor::on_NtCreateUserProcess()
{
    LOG(INFO, "Break on NtCreateUserProcess");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[1]);
    const auto ProcessDesiredAccess= nt::cast_to<nt::ACCESS_MASK>        (args[2]);
    const auto ThreadDesiredAccess= nt::cast_to<nt::ACCESS_MASK>        (args[3]);
    const auto ProcessObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);
    const auto ThreadObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[5]);
    const auto ProcessFlags    = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ThreadFlags     = nt::cast_to<nt::ULONG>              (args[7]);
    const auto ProcessParameters= nt::cast_to<nt::PRTL_USER_PROCESS_PARAMETERS>(args[8]);
    const auto CreateInfo      = nt::cast_to<nt::PPROCESS_CREATE_INFO>(args[9]);
    const auto AttributeList   = nt::cast_to<nt::PPROCESS_ATTRIBUTE_LIST>(args[10]);

    for(const auto& it : d_->observers_NtCreateUserProcess)
    {
        it(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    }
}

void monitor::GenericMonitor::register_NtCreateWaitablePort(const on_NtCreateWaitablePort_fn& on_ntcreatewaitableport)
{
    d_->observers_NtCreateWaitablePort.push_back(on_ntcreatewaitableport);
}

void monitor::GenericMonitor::on_NtCreateWaitablePort()
{
    LOG(INFO, "Break on NtCreateWaitablePort");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto MaxConnectionInfoLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto MaxMessageLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto MaxPoolUsage    = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtCreateWaitablePort)
    {
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    }
}

void monitor::GenericMonitor::register_NtCreateWorkerFactory(const on_NtCreateWorkerFactory_fn& on_ntcreateworkerfactory)
{
    d_->observers_NtCreateWorkerFactory.push_back(on_ntcreateworkerfactory);
}

void monitor::GenericMonitor::on_NtCreateWorkerFactory()
{
    LOG(INFO, "Break on NtCreateWorkerFactory");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandleReturn= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto CompletionPortHandle= nt::cast_to<nt::HANDLE>             (args[3]);
    const auto WorkerProcessHandle= nt::cast_to<nt::HANDLE>             (args[4]);
    const auto StartRoutine    = nt::cast_to<nt::PVOID>              (args[5]);
    const auto StartParameter  = nt::cast_to<nt::PVOID>              (args[6]);
    const auto MaxThreadCount  = nt::cast_to<nt::ULONG>              (args[7]);
    const auto StackReserve    = nt::cast_to<nt::SIZE_T>             (args[8]);
    const auto StackCommit     = nt::cast_to<nt::SIZE_T>             (args[9]);

    for(const auto& it : d_->observers_NtCreateWorkerFactory)
    {
        it(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
    }
}

void monitor::GenericMonitor::register_NtDebugActiveProcess(const on_NtDebugActiveProcess_fn& on_ntdebugactiveprocess)
{
    d_->observers_NtDebugActiveProcess.push_back(on_ntdebugactiveprocess);
}

void monitor::GenericMonitor::on_NtDebugActiveProcess()
{
    LOG(INFO, "Break on NtDebugActiveProcess");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtDebugActiveProcess)
    {
        it(ProcessHandle, DebugObjectHandle);
    }
}

void monitor::GenericMonitor::register_NtDebugContinue(const on_NtDebugContinue_fn& on_ntdebugcontinue)
{
    d_->observers_NtDebugContinue.push_back(on_ntdebugcontinue);
}

void monitor::GenericMonitor::on_NtDebugContinue()
{
    LOG(INFO, "Break on NtDebugContinue");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[1]);
    const auto ContinueStatus  = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtDebugContinue)
    {
        it(DebugObjectHandle, ClientId, ContinueStatus);
    }
}

void monitor::GenericMonitor::register_NtDelayExecution(const on_NtDelayExecution_fn& on_ntdelayexecution)
{
    d_->observers_NtDelayExecution.push_back(on_ntdelayexecution);
}

void monitor::GenericMonitor::on_NtDelayExecution()
{
    LOG(INFO, "Break on NtDelayExecution");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DelayInterval   = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtDelayExecution)
    {
        it(Alertable, DelayInterval);
    }
}

void monitor::GenericMonitor::register_NtDeleteAtom(const on_NtDeleteAtom_fn& on_ntdeleteatom)
{
    d_->observers_NtDeleteAtom.push_back(on_ntdeleteatom);
}

void monitor::GenericMonitor::on_NtDeleteAtom()
{
    LOG(INFO, "Break on NtDeleteAtom");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Atom            = nt::cast_to<nt::RTL_ATOM>           (args[0]);

    for(const auto& it : d_->observers_NtDeleteAtom)
    {
        it(Atom);
    }
}

void monitor::GenericMonitor::register_NtDeleteBootEntry(const on_NtDeleteBootEntry_fn& on_ntdeletebootentry)
{
    d_->observers_NtDeleteBootEntry.push_back(on_ntdeletebootentry);
}

void monitor::GenericMonitor::on_NtDeleteBootEntry()
{
    LOG(INFO, "Break on NtDeleteBootEntry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Id              = nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtDeleteBootEntry)
    {
        it(Id);
    }
}

void monitor::GenericMonitor::register_NtDeleteDriverEntry(const on_NtDeleteDriverEntry_fn& on_ntdeletedriverentry)
{
    d_->observers_NtDeleteDriverEntry.push_back(on_ntdeletedriverentry);
}

void monitor::GenericMonitor::on_NtDeleteDriverEntry()
{
    LOG(INFO, "Break on NtDeleteDriverEntry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Id              = nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtDeleteDriverEntry)
    {
        it(Id);
    }
}

void monitor::GenericMonitor::register_NtDeleteFile(const on_NtDeleteFile_fn& on_ntdeletefile)
{
    d_->observers_NtDeleteFile.push_back(on_ntdeletefile);
}

void monitor::GenericMonitor::on_NtDeleteFile()
{
    LOG(INFO, "Break on NtDeleteFile");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);

    for(const auto& it : d_->observers_NtDeleteFile)
    {
        it(ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtDeleteKey(const on_NtDeleteKey_fn& on_ntdeletekey)
{
    d_->observers_NtDeleteKey.push_back(on_ntdeletekey);
}

void monitor::GenericMonitor::on_NtDeleteKey()
{
    LOG(INFO, "Break on NtDeleteKey");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtDeleteKey)
    {
        it(KeyHandle);
    }
}

void monitor::GenericMonitor::register_NtDeleteObjectAuditAlarm(const on_NtDeleteObjectAuditAlarm_fn& on_ntdeleteobjectauditalarm)
{
    d_->observers_NtDeleteObjectAuditAlarm.push_back(on_ntdeleteobjectauditalarm);
}

void monitor::GenericMonitor::on_NtDeleteObjectAuditAlarm()
{
    LOG(INFO, "Break on NtDeleteObjectAuditAlarm");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto GenerateOnClose = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtDeleteObjectAuditAlarm)
    {
        it(SubsystemName, HandleId, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtDeletePrivateNamespace(const on_NtDeletePrivateNamespace_fn& on_ntdeleteprivatenamespace)
{
    d_->observers_NtDeletePrivateNamespace.push_back(on_ntdeleteprivatenamespace);
}

void monitor::GenericMonitor::on_NtDeletePrivateNamespace()
{
    LOG(INFO, "Break on NtDeletePrivateNamespace");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtDeletePrivateNamespace)
    {
        it(NamespaceHandle);
    }
}

void monitor::GenericMonitor::register_NtDeleteValueKey(const on_NtDeleteValueKey_fn& on_ntdeletevaluekey)
{
    d_->observers_NtDeleteValueKey.push_back(on_ntdeletevaluekey);
}

void monitor::GenericMonitor::on_NtDeleteValueKey()
{
    LOG(INFO, "Break on NtDeleteValueKey");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtDeleteValueKey)
    {
        it(KeyHandle, ValueName);
    }
}

void monitor::GenericMonitor::register_NtDeviceIoControlFile(const on_NtDeviceIoControlFile_fn& on_ntdeviceiocontrolfile)
{
    d_->observers_NtDeviceIoControlFile.push_back(on_ntdeviceiocontrolfile);
}

void monitor::GenericMonitor::on_NtDeviceIoControlFile()
{
    LOG(INFO, "Break on NtDeviceIoControlFile");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto IoControlCode   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[6]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[7]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[8]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[9]);

    for(const auto& it : d_->observers_NtDeviceIoControlFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }
}

void monitor::GenericMonitor::register_NtDisplayString(const on_NtDisplayString_fn& on_ntdisplaystring)
{
    d_->observers_NtDisplayString.push_back(on_ntdisplaystring);
}

void monitor::GenericMonitor::on_NtDisplayString()
{
    LOG(INFO, "Break on NtDisplayString");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto String          = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtDisplayString)
    {
        it(String);
    }
}

void monitor::GenericMonitor::register_NtDrawText(const on_NtDrawText_fn& on_ntdrawtext)
{
    d_->observers_NtDrawText.push_back(on_ntdrawtext);
}

void monitor::GenericMonitor::on_NtDrawText()
{
    LOG(INFO, "Break on NtDrawText");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Text            = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtDrawText)
    {
        it(Text);
    }
}

void monitor::GenericMonitor::register_NtDuplicateObject(const on_NtDuplicateObject_fn& on_ntduplicateobject)
{
    d_->observers_NtDuplicateObject.push_back(on_ntduplicateobject);
}

void monitor::GenericMonitor::on_NtDuplicateObject()
{
    LOG(INFO, "Break on NtDuplicateObject");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SourceProcessHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SourceHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto TargetProcessHandle= nt::cast_to<nt::HANDLE>             (args[2]);
    const auto TargetHandle    = nt::cast_to<nt::PHANDLE>            (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[5]);
    const auto Options         = nt::cast_to<nt::ULONG>              (args[6]);

    for(const auto& it : d_->observers_NtDuplicateObject)
    {
        it(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
    }
}

void monitor::GenericMonitor::register_NtDuplicateToken(const on_NtDuplicateToken_fn& on_ntduplicatetoken)
{
    d_->observers_NtDuplicateToken.push_back(on_ntduplicatetoken);
}

void monitor::GenericMonitor::on_NtDuplicateToken()
{
    LOG(INFO, "Break on NtDuplicateToken");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExistingTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto EffectiveOnly   = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto TokenType       = nt::cast_to<nt::TOKEN_TYPE>         (args[4]);
    const auto NewTokenHandle  = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtDuplicateToken)
    {
        it(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
    }
}

void monitor::GenericMonitor::register_NtEnumerateBootEntries(const on_NtEnumerateBootEntries_fn& on_ntenumeratebootentries)
{
    d_->observers_NtEnumerateBootEntries.push_back(on_ntenumeratebootentries);
}

void monitor::GenericMonitor::on_NtEnumerateBootEntries()
{
    LOG(INFO, "Break on NtEnumerateBootEntries");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[0]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtEnumerateBootEntries)
    {
        it(Buffer, BufferLength);
    }
}

void monitor::GenericMonitor::register_NtEnumerateDriverEntries(const on_NtEnumerateDriverEntries_fn& on_ntenumeratedriverentries)
{
    d_->observers_NtEnumerateDriverEntries.push_back(on_ntenumeratedriverentries);
}

void monitor::GenericMonitor::on_NtEnumerateDriverEntries()
{
    LOG(INFO, "Break on NtEnumerateDriverEntries");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[0]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtEnumerateDriverEntries)
    {
        it(Buffer, BufferLength);
    }
}

void monitor::GenericMonitor::register_NtEnumerateKey(const on_NtEnumerateKey_fn& on_ntenumeratekey)
{
    d_->observers_NtEnumerateKey.push_back(on_ntenumeratekey);
}

void monitor::GenericMonitor::on_NtEnumerateKey()
{
    LOG(INFO, "Break on NtEnumerateKey");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto KeyInformationClass= nt::cast_to<nt::KEY_INFORMATION_CLASS>(args[2]);
    const auto KeyInformation  = nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtEnumerateKey)
    {
        it(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
    }
}

void monitor::GenericMonitor::register_NtEnumerateSystemEnvironmentValuesEx(const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_ntenumeratesystemenvironmentvaluesex)
{
    d_->observers_NtEnumerateSystemEnvironmentValuesEx.push_back(on_ntenumeratesystemenvironmentvaluesex);
}

void monitor::GenericMonitor::on_NtEnumerateSystemEnvironmentValuesEx()
{
    LOG(INFO, "Break on NtEnumerateSystemEnvironmentValuesEx");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InformationClass= nt::cast_to<nt::ULONG>              (args[0]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[1]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtEnumerateSystemEnvironmentValuesEx)
    {
        it(InformationClass, Buffer, BufferLength);
    }
}

void monitor::GenericMonitor::register_NtEnumerateTransactionObject(const on_NtEnumerateTransactionObject_fn& on_ntenumeratetransactionobject)
{
    d_->observers_NtEnumerateTransactionObject.push_back(on_ntenumeratetransactionobject);
}

void monitor::GenericMonitor::on_NtEnumerateTransactionObject()
{
    LOG(INFO, "Break on NtEnumerateTransactionObject");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto RootObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto QueryType       = nt::cast_to<nt::KTMOBJECT_TYPE>     (args[1]);
    const auto ObjectCursor    = nt::cast_to<nt::PKTMOBJECT_CURSOR>  (args[2]);
    const auto ObjectCursorLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtEnumerateTransactionObject)
    {
        it(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtEnumerateValueKey(const on_NtEnumerateValueKey_fn& on_ntenumeratevaluekey)
{
    d_->observers_NtEnumerateValueKey.push_back(on_ntenumeratevaluekey);
}

void monitor::GenericMonitor::on_NtEnumerateValueKey()
{
    LOG(INFO, "Break on NtEnumerateValueKey");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto KeyValueInformationClass= nt::cast_to<nt::KEY_VALUE_INFORMATION_CLASS>(args[2]);
    const auto KeyValueInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtEnumerateValueKey)
    {
        it(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }
}

void monitor::GenericMonitor::register_NtExtendSection(const on_NtExtendSection_fn& on_ntextendsection)
{
    d_->observers_NtExtendSection.push_back(on_ntextendsection);
}

void monitor::GenericMonitor::on_NtExtendSection()
{
    LOG(INFO, "Break on NtExtendSection");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NewSectionSize  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtExtendSection)
    {
        it(SectionHandle, NewSectionSize);
    }
}

void monitor::GenericMonitor::register_NtFilterToken(const on_NtFilterToken_fn& on_ntfiltertoken)
{
    d_->observers_NtFilterToken.push_back(on_ntfiltertoken);
}

void monitor::GenericMonitor::on_NtFilterToken()
{
    LOG(INFO, "Break on NtFilterToken");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExistingTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SidsToDisable   = nt::cast_to<nt::PTOKEN_GROUPS>      (args[2]);
    const auto PrivilegesToDelete= nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[3]);
    const auto RestrictedSids  = nt::cast_to<nt::PTOKEN_GROUPS>      (args[4]);
    const auto NewTokenHandle  = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtFilterToken)
    {
        it(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
    }
}

void monitor::GenericMonitor::register_NtFindAtom(const on_NtFindAtom_fn& on_ntfindatom)
{
    d_->observers_NtFindAtom.push_back(on_ntfindatom);
}

void monitor::GenericMonitor::on_NtFindAtom()
{
    LOG(INFO, "Break on NtFindAtom");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto AtomName        = nt::cast_to<nt::PWSTR>              (args[0]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Atom            = nt::cast_to<nt::PRTL_ATOM>          (args[2]);

    for(const auto& it : d_->observers_NtFindAtom)
    {
        it(AtomName, Length, Atom);
    }
}

void monitor::GenericMonitor::register_NtFlushBuffersFile(const on_NtFlushBuffersFile_fn& on_ntflushbuffersfile)
{
    d_->observers_NtFlushBuffersFile.push_back(on_ntflushbuffersfile);
}

void monitor::GenericMonitor::on_NtFlushBuffersFile()
{
    LOG(INFO, "Break on NtFlushBuffersFile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);

    for(const auto& it : d_->observers_NtFlushBuffersFile)
    {
        it(FileHandle, IoStatusBlock);
    }
}

void monitor::GenericMonitor::register_NtFlushInstallUILanguage(const on_NtFlushInstallUILanguage_fn& on_ntflushinstalluilanguage)
{
    d_->observers_NtFlushInstallUILanguage.push_back(on_ntflushinstalluilanguage);
}

void monitor::GenericMonitor::on_NtFlushInstallUILanguage()
{
    LOG(INFO, "Break on NtFlushInstallUILanguage");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InstallUILanguage= nt::cast_to<nt::LANGID>             (args[0]);
    const auto SetComittedFlag = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtFlushInstallUILanguage)
    {
        it(InstallUILanguage, SetComittedFlag);
    }
}

void monitor::GenericMonitor::register_NtFlushInstructionCache(const on_NtFlushInstructionCache_fn& on_ntflushinstructioncache)
{
    d_->observers_NtFlushInstructionCache.push_back(on_ntflushinstructioncache);
}

void monitor::GenericMonitor::on_NtFlushInstructionCache()
{
    LOG(INFO, "Break on NtFlushInstructionCache");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Length          = nt::cast_to<nt::SIZE_T>             (args[2]);

    for(const auto& it : d_->observers_NtFlushInstructionCache)
    {
        it(ProcessHandle, BaseAddress, Length);
    }
}

void monitor::GenericMonitor::register_NtFlushKey(const on_NtFlushKey_fn& on_ntflushkey)
{
    d_->observers_NtFlushKey.push_back(on_ntflushkey);
}

void monitor::GenericMonitor::on_NtFlushKey()
{
    LOG(INFO, "Break on NtFlushKey");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtFlushKey)
    {
        it(KeyHandle);
    }
}

void monitor::GenericMonitor::register_NtFlushVirtualMemory(const on_NtFlushVirtualMemory_fn& on_ntflushvirtualmemory)
{
    d_->observers_NtFlushVirtualMemory.push_back(on_ntflushvirtualmemory);
}

void monitor::GenericMonitor::on_NtFlushVirtualMemory()
{
    LOG(INFO, "Break on NtFlushVirtualMemory");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto IoStatus        = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);

    for(const auto& it : d_->observers_NtFlushVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
    }
}

void monitor::GenericMonitor::register_NtFreeUserPhysicalPages(const on_NtFreeUserPhysicalPages_fn& on_ntfreeuserphysicalpages)
{
    d_->observers_NtFreeUserPhysicalPages.push_back(on_ntfreeuserphysicalpages);
}

void monitor::GenericMonitor::on_NtFreeUserPhysicalPages()
{
    LOG(INFO, "Break on NtFreeUserPhysicalPages");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::PULONG_PTR>         (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtFreeUserPhysicalPages)
    {
        it(ProcessHandle, NumberOfPages, UserPfnArra);
    }
}

void monitor::GenericMonitor::register_NtFreeVirtualMemory(const on_NtFreeVirtualMemory_fn& on_ntfreevirtualmemory)
{
    d_->observers_NtFreeVirtualMemory.push_back(on_ntfreevirtualmemory);
}

void monitor::GenericMonitor::on_NtFreeVirtualMemory()
{
    LOG(INFO, "Break on NtFreeVirtualMemory");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto FreeType        = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtFreeVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
    }
}

void monitor::GenericMonitor::register_NtFreezeRegistry(const on_NtFreezeRegistry_fn& on_ntfreezeregistry)
{
    d_->observers_NtFreezeRegistry.push_back(on_ntfreezeregistry);
}

void monitor::GenericMonitor::on_NtFreezeRegistry()
{
    LOG(INFO, "Break on NtFreezeRegistry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimeOutInSeconds= nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtFreezeRegistry)
    {
        it(TimeOutInSeconds);
    }
}

void monitor::GenericMonitor::register_NtFreezeTransactions(const on_NtFreezeTransactions_fn& on_ntfreezetransactions)
{
    d_->observers_NtFreezeTransactions.push_back(on_ntfreezetransactions);
}

void monitor::GenericMonitor::on_NtFreezeTransactions()
{
    LOG(INFO, "Break on NtFreezeTransactions");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FreezeTimeout   = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto ThawTimeout     = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtFreezeTransactions)
    {
        it(FreezeTimeout, ThawTimeout);
    }
}

void monitor::GenericMonitor::register_NtFsControlFile(const on_NtFsControlFile_fn& on_ntfscontrolfile)
{
    d_->observers_NtFsControlFile.push_back(on_ntfscontrolfile);
}

void monitor::GenericMonitor::on_NtFsControlFile()
{
    LOG(INFO, "Break on NtFsControlFile");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto IoControlCode   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[6]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[7]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[8]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[9]);

    for(const auto& it : d_->observers_NtFsControlFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }
}

void monitor::GenericMonitor::register_NtGetContextThread(const on_NtGetContextThread_fn& on_ntgetcontextthread)
{
    d_->observers_NtGetContextThread.push_back(on_ntgetcontextthread);
}

void monitor::GenericMonitor::on_NtGetContextThread()
{
    LOG(INFO, "Break on NtGetContextThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadContext   = nt::cast_to<nt::PCONTEXT>           (args[1]);

    for(const auto& it : d_->observers_NtGetContextThread)
    {
        it(ThreadHandle, ThreadContext);
    }
}

void monitor::GenericMonitor::register_NtGetDevicePowerState(const on_NtGetDevicePowerState_fn& on_ntgetdevicepowerstate)
{
    d_->observers_NtGetDevicePowerState.push_back(on_ntgetdevicepowerstate);
}

void monitor::GenericMonitor::on_NtGetDevicePowerState()
{
    LOG(INFO, "Break on NtGetDevicePowerState");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Device          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARState       = nt::cast_to<nt::DEVICE_POWER_STATE> (args[1]);

    for(const auto& it : d_->observers_NtGetDevicePowerState)
    {
        it(Device, STARState);
    }
}

void monitor::GenericMonitor::register_NtGetMUIRegistryInfo(const on_NtGetMUIRegistryInfo_fn& on_ntgetmuiregistryinfo)
{
    d_->observers_NtGetMUIRegistryInfo.push_back(on_ntgetmuiregistryinfo);
}

void monitor::GenericMonitor::on_NtGetMUIRegistryInfo()
{
    LOG(INFO, "Break on NtGetMUIRegistryInfo");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Flags           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto DataSize        = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Data            = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtGetMUIRegistryInfo)
    {
        it(Flags, DataSize, Data);
    }
}

void monitor::GenericMonitor::register_NtGetNextProcess(const on_NtGetNextProcess_fn& on_ntgetnextprocess)
{
    d_->observers_NtGetNextProcess.push_back(on_ntgetnextprocess);
}

void monitor::GenericMonitor::on_NtGetNextProcess()
{
    LOG(INFO, "Break on NtGetNextProcess");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto NewProcessHandle= nt::cast_to<nt::PHANDLE>            (args[4]);

    for(const auto& it : d_->observers_NtGetNextProcess)
    {
        it(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
    }
}

void monitor::GenericMonitor::register_NtGetNextThread(const on_NtGetNextThread_fn& on_ntgetnextthread)
{
    d_->observers_NtGetNextThread.push_back(on_ntgetnextthread);
}

void monitor::GenericMonitor::on_NtGetNextThread()
{
    LOG(INFO, "Break on NtGetNextThread");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[2]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[3]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[4]);
    const auto NewThreadHandle = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtGetNextThread)
    {
        it(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
    }
}

void monitor::GenericMonitor::register_NtGetNlsSectionPtr(const on_NtGetNlsSectionPtr_fn& on_ntgetnlssectionptr)
{
    d_->observers_NtGetNlsSectionPtr.push_back(on_ntgetnlssectionptr);
}

void monitor::GenericMonitor::on_NtGetNlsSectionPtr()
{
    LOG(INFO, "Break on NtGetNlsSectionPtr");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionType     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto SectionData     = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextData     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto STARSectionPointer= nt::cast_to<nt::PVOID>              (args[3]);
    const auto SectionSize     = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtGetNlsSectionPtr)
    {
        it(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
    }
}

void monitor::GenericMonitor::register_NtGetNotificationResourceManager(const on_NtGetNotificationResourceManager_fn& on_ntgetnotificationresourcemanager)
{
    d_->observers_NtGetNotificationResourceManager.push_back(on_ntgetnotificationresourcemanager);
}

void monitor::GenericMonitor::on_NtGetNotificationResourceManager()
{
    LOG(INFO, "Break on NtGetNotificationResourceManager");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionNotification= nt::cast_to<nt::PTRANSACTION_NOTIFICATION>(args[1]);
    const auto NotificationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);
    const auto Asynchronous    = nt::cast_to<nt::ULONG>              (args[5]);
    const auto AsynchronousContext= nt::cast_to<nt::ULONG_PTR>          (args[6]);

    for(const auto& it : d_->observers_NtGetNotificationResourceManager)
    {
        it(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
    }
}

void monitor::GenericMonitor::register_NtGetPlugPlayEvent(const on_NtGetPlugPlayEvent_fn& on_ntgetplugplayevent)
{
    d_->observers_NtGetPlugPlayEvent.push_back(on_ntgetplugplayevent);
}

void monitor::GenericMonitor::on_NtGetPlugPlayEvent()
{
    LOG(INFO, "Break on NtGetPlugPlayEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Context         = nt::cast_to<nt::PVOID>              (args[1]);
    const auto EventBlock      = nt::cast_to<nt::PPLUGPLAY_EVENT_BLOCK>(args[2]);
    const auto EventBufferSize = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtGetPlugPlayEvent)
    {
        it(EventHandle, Context, EventBlock, EventBufferSize);
    }
}

void monitor::GenericMonitor::register_NtGetWriteWatch(const on_NtGetWriteWatch_fn& on_ntgetwritewatch)
{
    d_->observers_NtGetWriteWatch.push_back(on_ntgetwritewatch);
}

void monitor::GenericMonitor::on_NtGetWriteWatch()
{
    LOG(INFO, "Break on NtGetWriteWatch");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto RegionSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto STARUserAddressArray= nt::cast_to<nt::PVOID>              (args[4]);
    const auto EntriesInUserAddressArray= nt::cast_to<nt::PULONG_PTR>         (args[5]);
    const auto Granularity     = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtGetWriteWatch)
    {
        it(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
    }
}

void monitor::GenericMonitor::register_NtImpersonateAnonymousToken(const on_NtImpersonateAnonymousToken_fn& on_ntimpersonateanonymoustoken)
{
    d_->observers_NtImpersonateAnonymousToken.push_back(on_ntimpersonateanonymoustoken);
}

void monitor::GenericMonitor::on_NtImpersonateAnonymousToken()
{
    LOG(INFO, "Break on NtImpersonateAnonymousToken");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtImpersonateAnonymousToken)
    {
        it(ThreadHandle);
    }
}

void monitor::GenericMonitor::register_NtImpersonateClientOfPort(const on_NtImpersonateClientOfPort_fn& on_ntimpersonateclientofport)
{
    d_->observers_NtImpersonateClientOfPort.push_back(on_ntimpersonateclientofport);
}

void monitor::GenericMonitor::on_NtImpersonateClientOfPort()
{
    LOG(INFO, "Break on NtImpersonateClientOfPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtImpersonateClientOfPort)
    {
        it(PortHandle, Message);
    }
}

void monitor::GenericMonitor::register_NtImpersonateThread(const on_NtImpersonateThread_fn& on_ntimpersonatethread)
{
    d_->observers_NtImpersonateThread.push_back(on_ntimpersonatethread);
}

void monitor::GenericMonitor::on_NtImpersonateThread()
{
    LOG(INFO, "Break on NtImpersonateThread");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ServerThreadHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ClientThreadHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto SecurityQos     = nt::cast_to<nt::PSECURITY_QUALITY_OF_SERVICE>(args[2]);

    for(const auto& it : d_->observers_NtImpersonateThread)
    {
        it(ServerThreadHandle, ClientThreadHandle, SecurityQos);
    }
}

void monitor::GenericMonitor::register_NtInitializeNlsFiles(const on_NtInitializeNlsFiles_fn& on_ntinitializenlsfiles)
{
    d_->observers_NtInitializeNlsFiles.push_back(on_ntinitializenlsfiles);
}

void monitor::GenericMonitor::on_NtInitializeNlsFiles()
{
    LOG(INFO, "Break on NtInitializeNlsFiles");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::PLCID>              (args[1]);
    const auto DefaultCasingTableSize= nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);

    for(const auto& it : d_->observers_NtInitializeNlsFiles)
    {
        it(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
    }
}

void monitor::GenericMonitor::register_NtInitializeRegistry(const on_NtInitializeRegistry_fn& on_ntinitializeregistry)
{
    d_->observers_NtInitializeRegistry.push_back(on_ntinitializeregistry);
}

void monitor::GenericMonitor::on_NtInitializeRegistry()
{
    LOG(INFO, "Break on NtInitializeRegistry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootCondition   = nt::cast_to<nt::USHORT>             (args[0]);

    for(const auto& it : d_->observers_NtInitializeRegistry)
    {
        it(BootCondition);
    }
}

void monitor::GenericMonitor::register_NtInitiatePowerAction(const on_NtInitiatePowerAction_fn& on_ntinitiatepoweraction)
{
    d_->observers_NtInitiatePowerAction.push_back(on_ntinitiatepoweraction);
}

void monitor::GenericMonitor::on_NtInitiatePowerAction()
{
    LOG(INFO, "Break on NtInitiatePowerAction");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemAction    = nt::cast_to<nt::POWER_ACTION>       (args[0]);
    const auto MinSystemState  = nt::cast_to<nt::SYSTEM_POWER_STATE> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Asynchronous    = nt::cast_to<nt::BOOLEAN>            (args[3]);

    for(const auto& it : d_->observers_NtInitiatePowerAction)
    {
        it(SystemAction, MinSystemState, Flags, Asynchronous);
    }
}

void monitor::GenericMonitor::register_NtIsProcessInJob(const on_NtIsProcessInJob_fn& on_ntisprocessinjob)
{
    d_->observers_NtIsProcessInJob.push_back(on_ntisprocessinjob);
}

void monitor::GenericMonitor::on_NtIsProcessInJob()
{
    LOG(INFO, "Break on NtIsProcessInJob");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtIsProcessInJob)
    {
        it(ProcessHandle, JobHandle);
    }
}

void monitor::GenericMonitor::register_NtListenPort(const on_NtListenPort_fn& on_ntlistenport)
{
    d_->observers_NtListenPort.push_back(on_ntlistenport);
}

void monitor::GenericMonitor::on_NtListenPort()
{
    LOG(INFO, "Break on NtListenPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ConnectionRequest= nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtListenPort)
    {
        it(PortHandle, ConnectionRequest);
    }
}

void monitor::GenericMonitor::register_NtLoadDriver(const on_NtLoadDriver_fn& on_ntloaddriver)
{
    d_->observers_NtLoadDriver.push_back(on_ntloaddriver);
}

void monitor::GenericMonitor::on_NtLoadDriver()
{
    LOG(INFO, "Break on NtLoadDriver");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverServiceName= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtLoadDriver)
    {
        it(DriverServiceName);
    }
}

void monitor::GenericMonitor::register_NtLoadKey2(const on_NtLoadKey2_fn& on_ntloadkey2)
{
    d_->observers_NtLoadKey2.push_back(on_ntloadkey2);
}

void monitor::GenericMonitor::on_NtLoadKey2()
{
    LOG(INFO, "Break on NtLoadKey2");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtLoadKey2)
    {
        it(TargetKey, SourceFile, Flags);
    }
}

void monitor::GenericMonitor::register_NtLoadKeyEx(const on_NtLoadKeyEx_fn& on_ntloadkeyex)
{
    d_->observers_NtLoadKeyEx.push_back(on_ntloadkeyex);
}

void monitor::GenericMonitor::on_NtLoadKeyEx()
{
    LOG(INFO, "Break on NtLoadKeyEx");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto TrustClassKey   = nt::cast_to<nt::HANDLE>             (args[3]);

    for(const auto& it : d_->observers_NtLoadKeyEx)
    {
        it(TargetKey, SourceFile, Flags, TrustClassKey);
    }
}

void monitor::GenericMonitor::register_NtLoadKey(const on_NtLoadKey_fn& on_ntloadkey)
{
    d_->observers_NtLoadKey.push_back(on_ntloadkey);
}

void monitor::GenericMonitor::on_NtLoadKey()
{
    LOG(INFO, "Break on NtLoadKey");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);

    for(const auto& it : d_->observers_NtLoadKey)
    {
        it(TargetKey, SourceFile);
    }
}

void monitor::GenericMonitor::register_NtLockFile(const on_NtLockFile_fn& on_ntlockfile)
{
    d_->observers_NtLockFile.push_back(on_ntlockfile);
}

void monitor::GenericMonitor::on_NtLockFile()
{
    LOG(INFO, "Break on NtLockFile");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[5]);
    const auto Length          = nt::cast_to<nt::PLARGE_INTEGER>     (args[6]);
    const auto Key             = nt::cast_to<nt::ULONG>              (args[7]);
    const auto FailImmediately = nt::cast_to<nt::BOOLEAN>            (args[8]);
    const auto ExclusiveLock   = nt::cast_to<nt::BOOLEAN>            (args[9]);

    for(const auto& it : d_->observers_NtLockFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
    }
}

void monitor::GenericMonitor::register_NtLockProductActivationKeys(const on_NtLockProductActivationKeys_fn& on_ntlockproductactivationkeys)
{
    d_->observers_NtLockProductActivationKeys.push_back(on_ntlockproductactivationkeys);
}

void monitor::GenericMonitor::on_NtLockProductActivationKeys()
{
    LOG(INFO, "Break on NtLockProductActivationKeys");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARpPrivateVer = nt::cast_to<nt::ULONG>              (args[0]);
    const auto STARpSafeMode   = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtLockProductActivationKeys)
    {
        it(STARpPrivateVer, STARpSafeMode);
    }
}

void monitor::GenericMonitor::register_NtLockRegistryKey(const on_NtLockRegistryKey_fn& on_ntlockregistrykey)
{
    d_->observers_NtLockRegistryKey.push_back(on_ntlockregistrykey);
}

void monitor::GenericMonitor::on_NtLockRegistryKey()
{
    LOG(INFO, "Break on NtLockRegistryKey");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtLockRegistryKey)
    {
        it(KeyHandle);
    }
}

void monitor::GenericMonitor::register_NtLockVirtualMemory(const on_NtLockVirtualMemory_fn& on_ntlockvirtualmemory)
{
    d_->observers_NtLockVirtualMemory.push_back(on_ntlockvirtualmemory);
}

void monitor::GenericMonitor::on_NtLockVirtualMemory()
{
    LOG(INFO, "Break on NtLockVirtualMemory");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto MapType         = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtLockVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }
}

void monitor::GenericMonitor::register_NtMakePermanentObject(const on_NtMakePermanentObject_fn& on_ntmakepermanentobject)
{
    d_->observers_NtMakePermanentObject.push_back(on_ntmakepermanentobject);
}

void monitor::GenericMonitor::on_NtMakePermanentObject()
{
    LOG(INFO, "Break on NtMakePermanentObject");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtMakePermanentObject)
    {
        it(Handle);
    }
}

void monitor::GenericMonitor::register_NtMakeTemporaryObject(const on_NtMakeTemporaryObject_fn& on_ntmaketemporaryobject)
{
    d_->observers_NtMakeTemporaryObject.push_back(on_ntmaketemporaryobject);
}

void monitor::GenericMonitor::on_NtMakeTemporaryObject()
{
    LOG(INFO, "Break on NtMakeTemporaryObject");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtMakeTemporaryObject)
    {
        it(Handle);
    }
}

void monitor::GenericMonitor::register_NtMapCMFModule(const on_NtMapCMFModule_fn& on_ntmapcmfmodule)
{
    d_->observers_NtMapCMFModule.push_back(on_ntmapcmfmodule);
}

void monitor::GenericMonitor::on_NtMapCMFModule()
{
    LOG(INFO, "Break on NtMapCMFModule");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto What            = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto CacheIndexOut   = nt::cast_to<nt::PULONG>             (args[2]);
    const auto CacheFlagsOut   = nt::cast_to<nt::PULONG>             (args[3]);
    const auto ViewSizeOut     = nt::cast_to<nt::PULONG>             (args[4]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[5]);

    for(const auto& it : d_->observers_NtMapCMFModule)
    {
        it(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
    }
}

void monitor::GenericMonitor::register_NtMapUserPhysicalPages(const on_NtMapUserPhysicalPages_fn& on_ntmapuserphysicalpages)
{
    d_->observers_NtMapUserPhysicalPages.push_back(on_ntmapuserphysicalpages);
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPages()
{
    LOG(INFO, "Break on NtMapUserPhysicalPages");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VirtualAddress  = nt::cast_to<nt::PVOID>              (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::ULONG_PTR>          (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtMapUserPhysicalPages)
    {
        it(VirtualAddress, NumberOfPages, UserPfnArra);
    }
}

void monitor::GenericMonitor::register_NtMapUserPhysicalPagesScatter(const on_NtMapUserPhysicalPagesScatter_fn& on_ntmapuserphysicalpagesscatter)
{
    d_->observers_NtMapUserPhysicalPagesScatter.push_back(on_ntmapuserphysicalpagesscatter);
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPagesScatter()
{
    LOG(INFO, "Break on NtMapUserPhysicalPagesScatter");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARVirtualAddresses= nt::cast_to<nt::PVOID>              (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::ULONG_PTR>          (args[1]);
    const auto UserPfnArray    = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtMapUserPhysicalPagesScatter)
    {
        it(STARVirtualAddresses, NumberOfPages, UserPfnArray);
    }
}

void monitor::GenericMonitor::register_NtMapViewOfSection(const on_NtMapViewOfSection_fn& on_ntmapviewofsection)
{
    d_->observers_NtMapViewOfSection.push_back(on_ntmapviewofsection);
}

void monitor::GenericMonitor::on_NtMapViewOfSection()
{
    LOG(INFO, "Break on NtMapViewOfSection");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ZeroBits        = nt::cast_to<nt::ULONG_PTR>          (args[3]);
    const auto CommitSize      = nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto SectionOffset   = nt::cast_to<nt::PLARGE_INTEGER>     (args[5]);
    const auto ViewSize        = nt::cast_to<nt::PSIZE_T>            (args[6]);
    const auto InheritDisposition= nt::cast_to<nt::SECTION_INHERIT>    (args[7]);
    const auto AllocationType  = nt::cast_to<nt::ULONG>              (args[8]);
    const auto Win32Protect    = nt::cast_to<nt::WIN32_PROTECTION_MASK>(args[9]);

    for(const auto& it : d_->observers_NtMapViewOfSection)
    {
        it(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    }
}

void monitor::GenericMonitor::register_NtModifyBootEntry(const on_NtModifyBootEntry_fn& on_ntmodifybootentry)
{
    d_->observers_NtModifyBootEntry.push_back(on_ntmodifybootentry);
}

void monitor::GenericMonitor::on_NtModifyBootEntry()
{
    LOG(INFO, "Break on NtModifyBootEntry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootEntry       = nt::cast_to<nt::PBOOT_ENTRY>        (args[0]);

    for(const auto& it : d_->observers_NtModifyBootEntry)
    {
        it(BootEntry);
    }
}

void monitor::GenericMonitor::register_NtModifyDriverEntry(const on_NtModifyDriverEntry_fn& on_ntmodifydriverentry)
{
    d_->observers_NtModifyDriverEntry.push_back(on_ntmodifydriverentry);
}

void monitor::GenericMonitor::on_NtModifyDriverEntry()
{
    LOG(INFO, "Break on NtModifyDriverEntry");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverEntry     = nt::cast_to<nt::PEFI_DRIVER_ENTRY>  (args[0]);

    for(const auto& it : d_->observers_NtModifyDriverEntry)
    {
        it(DriverEntry);
    }
}

void monitor::GenericMonitor::register_NtNotifyChangeDirectoryFile(const on_NtNotifyChangeDirectoryFile_fn& on_ntnotifychangedirectoryfile)
{
    d_->observers_NtNotifyChangeDirectoryFile.push_back(on_ntnotifychangedirectoryfile);
}

void monitor::GenericMonitor::on_NtNotifyChangeDirectoryFile()
{
    LOG(INFO, "Break on NtNotifyChangeDirectoryFile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto CompletionFilter= nt::cast_to<nt::ULONG>              (args[7]);
    const auto WatchTree       = nt::cast_to<nt::BOOLEAN>            (args[8]);

    for(const auto& it : d_->observers_NtNotifyChangeDirectoryFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
    }
}

void monitor::GenericMonitor::register_NtNotifyChangeKey(const on_NtNotifyChangeKey_fn& on_ntnotifychangekey)
{
    d_->observers_NtNotifyChangeKey.push_back(on_ntnotifychangekey);
}

void monitor::GenericMonitor::on_NtNotifyChangeKey()
{
    LOG(INFO, "Break on NtNotifyChangeKey");
    const auto nargs = 10;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto CompletionFilter= nt::cast_to<nt::ULONG>              (args[5]);
    const auto WatchTree       = nt::cast_to<nt::BOOLEAN>            (args[6]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[7]);
    const auto BufferSize      = nt::cast_to<nt::ULONG>              (args[8]);
    const auto Asynchronous    = nt::cast_to<nt::BOOLEAN>            (args[9]);

    for(const auto& it : d_->observers_NtNotifyChangeKey)
    {
        it(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }
}

void monitor::GenericMonitor::register_NtNotifyChangeMultipleKeys(const on_NtNotifyChangeMultipleKeys_fn& on_ntnotifychangemultiplekeys)
{
    d_->observers_NtNotifyChangeMultipleKeys.push_back(on_ntnotifychangemultiplekeys);
}

void monitor::GenericMonitor::on_NtNotifyChangeMultipleKeys()
{
    LOG(INFO, "Break on NtNotifyChangeMultipleKeys");
    const auto nargs = 12;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MasterKeyHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SlaveObjects    = nt::cast_to<nt::OBJECT_ATTRIBUTES>  (args[2]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[3]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[4]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[5]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[6]);
    const auto CompletionFilter= nt::cast_to<nt::ULONG>              (args[7]);
    const auto WatchTree       = nt::cast_to<nt::BOOLEAN>            (args[8]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[9]);
    const auto BufferSize      = nt::cast_to<nt::ULONG>              (args[10]);
    const auto Asynchronous    = nt::cast_to<nt::BOOLEAN>            (args[11]);

    for(const auto& it : d_->observers_NtNotifyChangeMultipleKeys)
    {
        it(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
    }
}

void monitor::GenericMonitor::register_NtNotifyChangeSession(const on_NtNotifyChangeSession_fn& on_ntnotifychangesession)
{
    d_->observers_NtNotifyChangeSession.push_back(on_ntnotifychangesession);
}

void monitor::GenericMonitor::on_NtNotifyChangeSession()
{
    LOG(INFO, "Break on NtNotifyChangeSession");
    const auto nargs = 8;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Session         = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStateSequence = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Reserved        = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Action          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto IoState         = nt::cast_to<nt::IO_SESSION_STATE>   (args[4]);
    const auto IoState2        = nt::cast_to<nt::IO_SESSION_STATE>   (args[5]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[6]);
    const auto BufferSize      = nt::cast_to<nt::ULONG>              (args[7]);

    for(const auto& it : d_->observers_NtNotifyChangeSession)
    {
        it(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
    }
}

void monitor::GenericMonitor::register_NtOpenDirectoryObject(const on_NtOpenDirectoryObject_fn& on_ntopendirectoryobject)
{
    d_->observers_NtOpenDirectoryObject.push_back(on_ntopendirectoryobject);
}

void monitor::GenericMonitor::on_NtOpenDirectoryObject()
{
    LOG(INFO, "Break on NtOpenDirectoryObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenDirectoryObject)
    {
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenEnlistment(const on_NtOpenEnlistment_fn& on_ntopenenlistment)
{
    d_->observers_NtOpenEnlistment.push_back(on_ntopenenlistment);
}

void monitor::GenericMonitor::on_NtOpenEnlistment()
{
    LOG(INFO, "Break on NtOpenEnlistment");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[2]);
    const auto EnlistmentGuid  = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);

    for(const auto& it : d_->observers_NtOpenEnlistment)
    {
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenEvent(const on_NtOpenEvent_fn& on_ntopenevent)
{
    d_->observers_NtOpenEvent.push_back(on_ntopenevent);
}

void monitor::GenericMonitor::on_NtOpenEvent()
{
    LOG(INFO, "Break on NtOpenEvent");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenEvent)
    {
        it(EventHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenEventPair(const on_NtOpenEventPair_fn& on_ntopeneventpair)
{
    d_->observers_NtOpenEventPair.push_back(on_ntopeneventpair);
}

void monitor::GenericMonitor::on_NtOpenEventPair()
{
    LOG(INFO, "Break on NtOpenEventPair");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenEventPair)
    {
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenFile(const on_NtOpenFile_fn& on_ntopenfile)
{
    d_->observers_NtOpenFile.push_back(on_ntopenfile);
}

void monitor::GenericMonitor::on_NtOpenFile()
{
    LOG(INFO, "Break on NtOpenFile");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto ShareAccess     = nt::cast_to<nt::ULONG>              (args[4]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtOpenFile)
    {
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
    }
}

void monitor::GenericMonitor::register_NtOpenIoCompletion(const on_NtOpenIoCompletion_fn& on_ntopeniocompletion)
{
    d_->observers_NtOpenIoCompletion.push_back(on_ntopeniocompletion);
}

void monitor::GenericMonitor::on_NtOpenIoCompletion()
{
    LOG(INFO, "Break on NtOpenIoCompletion");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenIoCompletion)
    {
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenJobObject(const on_NtOpenJobObject_fn& on_ntopenjobobject)
{
    d_->observers_NtOpenJobObject.push_back(on_ntopenjobobject);
}

void monitor::GenericMonitor::on_NtOpenJobObject()
{
    LOG(INFO, "Break on NtOpenJobObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenJobObject)
    {
        it(JobHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenKeyedEvent(const on_NtOpenKeyedEvent_fn& on_ntopenkeyedevent)
{
    d_->observers_NtOpenKeyedEvent.push_back(on_ntopenkeyedevent);
}

void monitor::GenericMonitor::on_NtOpenKeyedEvent()
{
    LOG(INFO, "Break on NtOpenKeyedEvent");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenKeyedEvent)
    {
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenKeyEx(const on_NtOpenKeyEx_fn& on_ntopenkeyex)
{
    d_->observers_NtOpenKeyEx.push_back(on_ntopenkeyex);
}

void monitor::GenericMonitor::on_NtOpenKeyEx()
{
    LOG(INFO, "Break on NtOpenKeyEx");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtOpenKeyEx)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
    }
}

void monitor::GenericMonitor::register_NtOpenKey(const on_NtOpenKey_fn& on_ntopenkey)
{
    d_->observers_NtOpenKey.push_back(on_ntopenkey);
}

void monitor::GenericMonitor::on_NtOpenKey()
{
    LOG(INFO, "Break on NtOpenKey");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenKey)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenKeyTransactedEx(const on_NtOpenKeyTransactedEx_fn& on_ntopenkeytransactedex)
{
    d_->observers_NtOpenKeyTransactedEx.push_back(on_ntopenkeytransactedex);
}

void monitor::GenericMonitor::on_NtOpenKeyTransactedEx()
{
    LOG(INFO, "Break on NtOpenKeyTransactedEx");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[3]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[4]);

    for(const auto& it : d_->observers_NtOpenKeyTransactedEx)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenKeyTransacted(const on_NtOpenKeyTransacted_fn& on_ntopenkeytransacted)
{
    d_->observers_NtOpenKeyTransacted.push_back(on_ntopenkeytransacted);
}

void monitor::GenericMonitor::on_NtOpenKeyTransacted()
{
    LOG(INFO, "Break on NtOpenKeyTransacted");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[3]);

    for(const auto& it : d_->observers_NtOpenKeyTransacted)
    {
        it(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenMutant(const on_NtOpenMutant_fn& on_ntopenmutant)
{
    d_->observers_NtOpenMutant.push_back(on_ntopenmutant);
}

void monitor::GenericMonitor::on_NtOpenMutant()
{
    LOG(INFO, "Break on NtOpenMutant");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenMutant)
    {
        it(MutantHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenObjectAuditAlarm(const on_NtOpenObjectAuditAlarm_fn& on_ntopenobjectauditalarm)
{
    d_->observers_NtOpenObjectAuditAlarm.push_back(on_ntopenobjectauditalarm);
}

void monitor::GenericMonitor::on_NtOpenObjectAuditAlarm()
{
    LOG(INFO, "Break on NtOpenObjectAuditAlarm");
    const auto nargs = 12;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ObjectTypeName  = nt::cast_to<nt::PUNICODE_STRING>    (args[2]);
    const auto ObjectName      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[4]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[5]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[6]);
    const auto GrantedAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[7]);
    const auto Privileges      = nt::cast_to<nt::PPRIVILEGE_SET>     (args[8]);
    const auto ObjectCreation  = nt::cast_to<nt::BOOLEAN>            (args[9]);
    const auto AccessGranted   = nt::cast_to<nt::BOOLEAN>            (args[10]);
    const auto GenerateOnClose = nt::cast_to<nt::PBOOLEAN>           (args[11]);

    for(const auto& it : d_->observers_NtOpenObjectAuditAlarm)
    {
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
    }
}

void monitor::GenericMonitor::register_NtOpenPrivateNamespace(const on_NtOpenPrivateNamespace_fn& on_ntopenprivatenamespace)
{
    d_->observers_NtOpenPrivateNamespace.push_back(on_ntopenprivatenamespace);
}

void monitor::GenericMonitor::on_NtOpenPrivateNamespace()
{
    LOG(INFO, "Break on NtOpenPrivateNamespace");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto BoundaryDescriptor= nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtOpenPrivateNamespace)
    {
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
    }
}

void monitor::GenericMonitor::register_NtOpenProcess(const on_NtOpenProcess_fn& on_ntopenprocess)
{
    d_->observers_NtOpenProcess.push_back(on_ntopenprocess);
}

void monitor::GenericMonitor::on_NtOpenProcess()
{
    LOG(INFO, "Break on NtOpenProcess");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[3]);

    for(const auto& it : d_->observers_NtOpenProcess)
    {
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
    }
}

void monitor::GenericMonitor::register_NtOpenProcessTokenEx(const on_NtOpenProcessTokenEx_fn& on_ntopenprocesstokenex)
{
    d_->observers_NtOpenProcessTokenEx.push_back(on_ntopenprocesstokenex);
}

void monitor::GenericMonitor::on_NtOpenProcessTokenEx()
{
    LOG(INFO, "Break on NtOpenProcessTokenEx");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[3]);

    for(const auto& it : d_->observers_NtOpenProcessTokenEx)
    {
        it(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenProcessToken(const on_NtOpenProcessToken_fn& on_ntopenprocesstoken)
{
    d_->observers_NtOpenProcessToken.push_back(on_ntopenprocesstoken);
}

void monitor::GenericMonitor::on_NtOpenProcessToken()
{
    LOG(INFO, "Break on NtOpenProcessToken");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[2]);

    for(const auto& it : d_->observers_NtOpenProcessToken)
    {
        it(ProcessHandle, DesiredAccess, TokenHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenResourceManager(const on_NtOpenResourceManager_fn& on_ntopenresourcemanager)
{
    d_->observers_NtOpenResourceManager.push_back(on_ntopenresourcemanager);
}

void monitor::GenericMonitor::on_NtOpenResourceManager()
{
    LOG(INFO, "Break on NtOpenResourceManager");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto ResourceManagerGuid= nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);

    for(const auto& it : d_->observers_NtOpenResourceManager)
    {
        it(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenSection(const on_NtOpenSection_fn& on_ntopensection)
{
    d_->observers_NtOpenSection.push_back(on_ntopensection);
}

void monitor::GenericMonitor::on_NtOpenSection()
{
    LOG(INFO, "Break on NtOpenSection");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSection)
    {
        it(SectionHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenSemaphore(const on_NtOpenSemaphore_fn& on_ntopensemaphore)
{
    d_->observers_NtOpenSemaphore.push_back(on_ntopensemaphore);
}

void monitor::GenericMonitor::on_NtOpenSemaphore()
{
    LOG(INFO, "Break on NtOpenSemaphore");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSemaphore)
    {
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenSession(const on_NtOpenSession_fn& on_ntopensession)
{
    d_->observers_NtOpenSession.push_back(on_ntopensession);
}

void monitor::GenericMonitor::on_NtOpenSession()
{
    LOG(INFO, "Break on NtOpenSession");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SessionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSession)
    {
        it(SessionHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenSymbolicLinkObject(const on_NtOpenSymbolicLinkObject_fn& on_ntopensymboliclinkobject)
{
    d_->observers_NtOpenSymbolicLinkObject.push_back(on_ntopensymboliclinkobject);
}

void monitor::GenericMonitor::on_NtOpenSymbolicLinkObject()
{
    LOG(INFO, "Break on NtOpenSymbolicLinkObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSymbolicLinkObject)
    {
        it(LinkHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenThread(const on_NtOpenThread_fn& on_ntopenthread)
{
    d_->observers_NtOpenThread.push_back(on_ntopenthread);
}

void monitor::GenericMonitor::on_NtOpenThread()
{
    LOG(INFO, "Break on NtOpenThread");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[3]);

    for(const auto& it : d_->observers_NtOpenThread)
    {
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
    }
}

void monitor::GenericMonitor::register_NtOpenThreadTokenEx(const on_NtOpenThreadTokenEx_fn& on_ntopenthreadtokenex)
{
    d_->observers_NtOpenThreadTokenEx.push_back(on_ntopenthreadtokenex);
}

void monitor::GenericMonitor::on_NtOpenThreadTokenEx()
{
    LOG(INFO, "Break on NtOpenThreadTokenEx");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto OpenAsSelf      = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[3]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[4]);

    for(const auto& it : d_->observers_NtOpenThreadTokenEx)
    {
        it(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenThreadToken(const on_NtOpenThreadToken_fn& on_ntopenthreadtoken)
{
    d_->observers_NtOpenThreadToken.push_back(on_ntopenthreadtoken);
}

void monitor::GenericMonitor::on_NtOpenThreadToken()
{
    LOG(INFO, "Break on NtOpenThreadToken");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto OpenAsSelf      = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[3]);

    for(const auto& it : d_->observers_NtOpenThreadToken)
    {
        it(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
    }
}

void monitor::GenericMonitor::register_NtOpenTimer(const on_NtOpenTimer_fn& on_ntopentimer)
{
    d_->observers_NtOpenTimer.push_back(on_ntopentimer);
}

void monitor::GenericMonitor::on_NtOpenTimer()
{
    LOG(INFO, "Break on NtOpenTimer");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenTimer)
    {
        it(TimerHandle, DesiredAccess, ObjectAttributes);
    }
}

void monitor::GenericMonitor::register_NtOpenTransactionManager(const on_NtOpenTransactionManager_fn& on_ntopentransactionmanager)
{
    d_->observers_NtOpenTransactionManager.push_back(on_ntopentransactionmanager);
}

void monitor::GenericMonitor::on_NtOpenTransactionManager()
{
    LOG(INFO, "Break on NtOpenTransactionManager");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto TmIdentity      = nt::cast_to<nt::LPGUID>             (args[4]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtOpenTransactionManager)
    {
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
    }
}

void monitor::GenericMonitor::register_NtOpenTransaction(const on_NtOpenTransaction_fn& on_ntopentransaction)
{
    d_->observers_NtOpenTransaction.push_back(on_ntopentransaction);
}

void monitor::GenericMonitor::on_NtOpenTransaction()
{
    LOG(INFO, "Break on NtOpenTransaction");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Uow             = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[4]);

    for(const auto& it : d_->observers_NtOpenTransaction)
    {
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
    }
}

void monitor::GenericMonitor::register_NtPlugPlayControl(const on_NtPlugPlayControl_fn& on_ntplugplaycontrol)
{
    d_->observers_NtPlugPlayControl.push_back(on_ntplugplaycontrol);
}

void monitor::GenericMonitor::on_NtPlugPlayControl()
{
    LOG(INFO, "Break on NtPlugPlayControl");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PnPControlClass = nt::cast_to<nt::PLUGPLAY_CONTROL_CLASS>(args[0]);
    const auto PnPControlData  = nt::cast_to<nt::PVOID>              (args[1]);
    const auto PnPControlDataLength= nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtPlugPlayControl)
    {
        it(PnPControlClass, PnPControlData, PnPControlDataLength);
    }
}

void monitor::GenericMonitor::register_NtPowerInformation(const on_NtPowerInformation_fn& on_ntpowerinformation)
{
    d_->observers_NtPowerInformation.push_back(on_ntpowerinformation);
}

void monitor::GenericMonitor::on_NtPowerInformation()
{
    LOG(INFO, "Break on NtPowerInformation");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InformationLevel= nt::cast_to<nt::POWER_INFORMATION_LEVEL>(args[0]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtPowerInformation)
    {
        it(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
    }
}

void monitor::GenericMonitor::register_NtPrepareComplete(const on_NtPrepareComplete_fn& on_ntpreparecomplete)
{
    d_->observers_NtPrepareComplete.push_back(on_ntpreparecomplete);
}

void monitor::GenericMonitor::on_NtPrepareComplete()
{
    LOG(INFO, "Break on NtPrepareComplete");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrepareComplete)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtPrepareEnlistment(const on_NtPrepareEnlistment_fn& on_ntprepareenlistment)
{
    d_->observers_NtPrepareEnlistment.push_back(on_ntprepareenlistment);
}

void monitor::GenericMonitor::on_NtPrepareEnlistment()
{
    LOG(INFO, "Break on NtPrepareEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrepareEnlistment)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtPrePrepareComplete(const on_NtPrePrepareComplete_fn& on_ntprepreparecomplete)
{
    d_->observers_NtPrePrepareComplete.push_back(on_ntprepreparecomplete);
}

void monitor::GenericMonitor::on_NtPrePrepareComplete()
{
    LOG(INFO, "Break on NtPrePrepareComplete");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrePrepareComplete)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtPrePrepareEnlistment(const on_NtPrePrepareEnlistment_fn& on_ntpreprepareenlistment)
{
    d_->observers_NtPrePrepareEnlistment.push_back(on_ntpreprepareenlistment);
}

void monitor::GenericMonitor::on_NtPrePrepareEnlistment()
{
    LOG(INFO, "Break on NtPrePrepareEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrePrepareEnlistment)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtPrivilegeCheck(const on_NtPrivilegeCheck_fn& on_ntprivilegecheck)
{
    d_->observers_NtPrivilegeCheck.push_back(on_ntprivilegecheck);
}

void monitor::GenericMonitor::on_NtPrivilegeCheck()
{
    LOG(INFO, "Break on NtPrivilegeCheck");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequiredPrivileges= nt::cast_to<nt::PPRIVILEGE_SET>     (args[1]);
    const auto Result          = nt::cast_to<nt::PBOOLEAN>           (args[2]);

    for(const auto& it : d_->observers_NtPrivilegeCheck)
    {
        it(ClientToken, RequiredPrivileges, Result);
    }
}

void monitor::GenericMonitor::register_NtPrivilegedServiceAuditAlarm(const on_NtPrivilegedServiceAuditAlarm_fn& on_ntprivilegedserviceauditalarm)
{
    d_->observers_NtPrivilegedServiceAuditAlarm.push_back(on_ntprivilegedserviceauditalarm);
}

void monitor::GenericMonitor::on_NtPrivilegedServiceAuditAlarm()
{
    LOG(INFO, "Break on NtPrivilegedServiceAuditAlarm");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto ServiceName     = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto Privileges      = nt::cast_to<nt::PPRIVILEGE_SET>     (args[3]);
    const auto AccessGranted   = nt::cast_to<nt::BOOLEAN>            (args[4]);

    for(const auto& it : d_->observers_NtPrivilegedServiceAuditAlarm)
    {
        it(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
    }
}

void monitor::GenericMonitor::register_NtPrivilegeObjectAuditAlarm(const on_NtPrivilegeObjectAuditAlarm_fn& on_ntprivilegeobjectauditalarm)
{
    d_->observers_NtPrivilegeObjectAuditAlarm.push_back(on_ntprivilegeobjectauditalarm);
}

void monitor::GenericMonitor::on_NtPrivilegeObjectAuditAlarm()
{
    LOG(INFO, "Break on NtPrivilegeObjectAuditAlarm");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[3]);
    const auto Privileges      = nt::cast_to<nt::PPRIVILEGE_SET>     (args[4]);
    const auto AccessGranted   = nt::cast_to<nt::BOOLEAN>            (args[5]);

    for(const auto& it : d_->observers_NtPrivilegeObjectAuditAlarm)
    {
        it(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
    }
}

void monitor::GenericMonitor::register_NtPropagationComplete(const on_NtPropagationComplete_fn& on_ntpropagationcomplete)
{
    d_->observers_NtPropagationComplete.push_back(on_ntpropagationcomplete);
}

void monitor::GenericMonitor::on_NtPropagationComplete()
{
    LOG(INFO, "Break on NtPropagationComplete");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestCookie   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtPropagationComplete)
    {
        it(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
    }
}

void monitor::GenericMonitor::register_NtPropagationFailed(const on_NtPropagationFailed_fn& on_ntpropagationfailed)
{
    d_->observers_NtPropagationFailed.push_back(on_ntpropagationfailed);
}

void monitor::GenericMonitor::on_NtPropagationFailed()
{
    LOG(INFO, "Break on NtPropagationFailed");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestCookie   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto PropStatus      = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtPropagationFailed)
    {
        it(ResourceManagerHandle, RequestCookie, PropStatus);
    }
}

void monitor::GenericMonitor::register_NtProtectVirtualMemory(const on_NtProtectVirtualMemory_fn& on_ntprotectvirtualmemory)
{
    d_->observers_NtProtectVirtualMemory.push_back(on_ntprotectvirtualmemory);
}

void monitor::GenericMonitor::on_NtProtectVirtualMemory()
{
    LOG(INFO, "Break on NtProtectVirtualMemory");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto NewProtectWin32 = nt::cast_to<nt::WIN32_PROTECTION_MASK>(args[3]);
    const auto OldProtect      = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtProtectVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
    }
}

void monitor::GenericMonitor::register_NtPulseEvent(const on_NtPulseEvent_fn& on_ntpulseevent)
{
    d_->observers_NtPulseEvent.push_back(on_ntpulseevent);
}

void monitor::GenericMonitor::on_NtPulseEvent()
{
    LOG(INFO, "Break on NtPulseEvent");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtPulseEvent)
    {
        it(EventHandle, PreviousState);
    }
}

void monitor::GenericMonitor::register_NtQueryAttributesFile(const on_NtQueryAttributesFile_fn& on_ntqueryattributesfile)
{
    d_->observers_NtQueryAttributesFile.push_back(on_ntqueryattributesfile);
}

void monitor::GenericMonitor::on_NtQueryAttributesFile()
{
    LOG(INFO, "Break on NtQueryAttributesFile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto FileInformation = nt::cast_to<nt::PFILE_BASIC_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtQueryAttributesFile)
    {
        it(ObjectAttributes, FileInformation);
    }
}

void monitor::GenericMonitor::register_NtQueryBootEntryOrder(const on_NtQueryBootEntryOrder_fn& on_ntquerybootentryorder)
{
    d_->observers_NtQueryBootEntryOrder.push_back(on_ntquerybootentryorder);
}

void monitor::GenericMonitor::on_NtQueryBootEntryOrder()
{
    LOG(INFO, "Break on NtQueryBootEntryOrder");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryBootEntryOrder)
    {
        it(Ids, Count);
    }
}

void monitor::GenericMonitor::register_NtQueryBootOptions(const on_NtQueryBootOptions_fn& on_ntquerybootoptions)
{
    d_->observers_NtQueryBootOptions.push_back(on_ntquerybootoptions);
}

void monitor::GenericMonitor::on_NtQueryBootOptions()
{
    LOG(INFO, "Break on NtQueryBootOptions");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootOptions     = nt::cast_to<nt::PBOOT_OPTIONS>      (args[0]);
    const auto BootOptionsLength= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryBootOptions)
    {
        it(BootOptions, BootOptionsLength);
    }
}

void monitor::GenericMonitor::register_NtQueryDebugFilterState(const on_NtQueryDebugFilterState_fn& on_ntquerydebugfilterstate)
{
    d_->observers_NtQueryDebugFilterState.push_back(on_ntquerydebugfilterstate);
}

void monitor::GenericMonitor::on_NtQueryDebugFilterState()
{
    LOG(INFO, "Break on NtQueryDebugFilterState");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ComponentId     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Level           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtQueryDebugFilterState)
    {
        it(ComponentId, Level);
    }
}

void monitor::GenericMonitor::register_NtQueryDefaultLocale(const on_NtQueryDefaultLocale_fn& on_ntquerydefaultlocale)
{
    d_->observers_NtQueryDefaultLocale.push_back(on_ntquerydefaultlocale);
}

void monitor::GenericMonitor::on_NtQueryDefaultLocale()
{
    LOG(INFO, "Break on NtQueryDefaultLocale");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto UserProfile     = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::PLCID>              (args[1]);

    for(const auto& it : d_->observers_NtQueryDefaultLocale)
    {
        it(UserProfile, DefaultLocaleId);
    }
}

void monitor::GenericMonitor::register_NtQueryDefaultUILanguage(const on_NtQueryDefaultUILanguage_fn& on_ntquerydefaultuilanguage)
{
    d_->observers_NtQueryDefaultUILanguage.push_back(on_ntquerydefaultuilanguage);
}

void monitor::GenericMonitor::on_NtQueryDefaultUILanguage()
{
    LOG(INFO, "Break on NtQueryDefaultUILanguage");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARDefaultUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtQueryDefaultUILanguage)
    {
        it(STARDefaultUILanguageId);
    }
}

void monitor::GenericMonitor::register_NtQueryDirectoryFile(const on_NtQueryDirectoryFile_fn& on_ntquerydirectoryfile)
{
    d_->observers_NtQueryDirectoryFile.push_back(on_ntquerydirectoryfile);
}

void monitor::GenericMonitor::on_NtQueryDirectoryFile()
{
    LOG(INFO, "Break on NtQueryDirectoryFile");
    const auto nargs = 11;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto FileInformation = nt::cast_to<nt::PVOID>              (args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto FileInformationClass= nt::cast_to<nt::FILE_INFORMATION_CLASS>(args[7]);
    const auto ReturnSingleEntry= nt::cast_to<nt::BOOLEAN>            (args[8]);
    const auto FileName        = nt::cast_to<nt::PUNICODE_STRING>    (args[9]);
    const auto RestartScan     = nt::cast_to<nt::BOOLEAN>            (args[10]);

    for(const auto& it : d_->observers_NtQueryDirectoryFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
    }
}

void monitor::GenericMonitor::register_NtQueryDirectoryObject(const on_NtQueryDirectoryObject_fn& on_ntquerydirectoryobject)
{
    d_->observers_NtQueryDirectoryObject.push_back(on_ntquerydirectoryobject);
}

void monitor::GenericMonitor::on_NtQueryDirectoryObject()
{
    LOG(INFO, "Break on NtQueryDirectoryObject");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[2]);
    const auto ReturnSingleEntry= nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto RestartScan     = nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto Context         = nt::cast_to<nt::PULONG>             (args[5]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtQueryDirectoryObject)
    {
        it(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryDriverEntryOrder(const on_NtQueryDriverEntryOrder_fn& on_ntquerydriverentryorder)
{
    d_->observers_NtQueryDriverEntryOrder.push_back(on_ntquerydriverentryorder);
}

void monitor::GenericMonitor::on_NtQueryDriverEntryOrder()
{
    LOG(INFO, "Break on NtQueryDriverEntryOrder");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryDriverEntryOrder)
    {
        it(Ids, Count);
    }
}

void monitor::GenericMonitor::register_NtQueryEaFile(const on_NtQueryEaFile_fn& on_ntqueryeafile)
{
    d_->observers_NtQueryEaFile.push_back(on_ntqueryeafile);
}

void monitor::GenericMonitor::on_NtQueryEaFile()
{
    LOG(INFO, "Break on NtQueryEaFile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnSingleEntry= nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto EaList          = nt::cast_to<nt::PVOID>              (args[5]);
    const auto EaListLength    = nt::cast_to<nt::ULONG>              (args[6]);
    const auto EaIndex         = nt::cast_to<nt::PULONG>             (args[7]);
    const auto RestartScan     = nt::cast_to<nt::BOOLEAN>            (args[8]);

    for(const auto& it : d_->observers_NtQueryEaFile)
    {
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
    }
}

void monitor::GenericMonitor::register_NtQueryEvent(const on_NtQueryEvent_fn& on_ntqueryevent)
{
    d_->observers_NtQueryEvent.push_back(on_ntqueryevent);
}

void monitor::GenericMonitor::on_NtQueryEvent()
{
    LOG(INFO, "Break on NtQueryEvent");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EventInformationClass= nt::cast_to<nt::EVENT_INFORMATION_CLASS>(args[1]);
    const auto EventInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EventInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryEvent)
    {
        it(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryFullAttributesFile(const on_NtQueryFullAttributesFile_fn& on_ntqueryfullattributesfile)
{
    d_->observers_NtQueryFullAttributesFile.push_back(on_ntqueryfullattributesfile);
}

void monitor::GenericMonitor::on_NtQueryFullAttributesFile()
{
    LOG(INFO, "Break on NtQueryFullAttributesFile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto FileInformation = nt::cast_to<nt::PFILE_NETWORK_OPEN_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtQueryFullAttributesFile)
    {
        it(ObjectAttributes, FileInformation);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationAtom(const on_NtQueryInformationAtom_fn& on_ntqueryinformationatom)
{
    d_->observers_NtQueryInformationAtom.push_back(on_ntqueryinformationatom);
}

void monitor::GenericMonitor::on_NtQueryInformationAtom()
{
    LOG(INFO, "Break on NtQueryInformationAtom");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Atom            = nt::cast_to<nt::RTL_ATOM>           (args[0]);
    const auto InformationClass= nt::cast_to<nt::ATOM_INFORMATION_CLASS>(args[1]);
    const auto AtomInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto AtomInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationAtom)
    {
        it(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationEnlistment(const on_NtQueryInformationEnlistment_fn& on_ntqueryinformationenlistment)
{
    d_->observers_NtQueryInformationEnlistment.push_back(on_ntqueryinformationenlistment);
}

void monitor::GenericMonitor::on_NtQueryInformationEnlistment()
{
    LOG(INFO, "Break on NtQueryInformationEnlistment");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentInformationClass= nt::cast_to<nt::ENLISTMENT_INFORMATION_CLASS>(args[1]);
    const auto EnlistmentInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EnlistmentInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationEnlistment)
    {
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationFile(const on_NtQueryInformationFile_fn& on_ntqueryinformationfile)
{
    d_->observers_NtQueryInformationFile.push_back(on_ntqueryinformationfile);
}

void monitor::GenericMonitor::on_NtQueryInformationFile()
{
    LOG(INFO, "Break on NtQueryInformationFile");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FileInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FileInformationClass= nt::cast_to<nt::FILE_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtQueryInformationFile)
    {
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationJobObject(const on_NtQueryInformationJobObject_fn& on_ntqueryinformationjobobject)
{
    d_->observers_NtQueryInformationJobObject.push_back(on_ntqueryinformationjobobject);
}

void monitor::GenericMonitor::on_NtQueryInformationJobObject()
{
    LOG(INFO, "Break on NtQueryInformationJobObject");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobObjectInformationClass= nt::cast_to<nt::JOBOBJECTINFOCLASS> (args[1]);
    const auto JobObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto JobObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationJobObject)
    {
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationPort(const on_NtQueryInformationPort_fn& on_ntqueryinformationport)
{
    d_->observers_NtQueryInformationPort.push_back(on_ntqueryinformationport);
}

void monitor::GenericMonitor::on_NtQueryInformationPort()
{
    LOG(INFO, "Break on NtQueryInformationPort");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationPort)
    {
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationProcess(const on_NtQueryInformationProcess_fn& on_ntqueryinformationprocess)
{
    d_->observers_NtQueryInformationProcess.push_back(on_ntqueryinformationprocess);
}

void monitor::GenericMonitor::on_NtQueryInformationProcess()
{
    LOG(INFO, "Break on NtQueryInformationProcess");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessInformationClass= nt::cast_to<nt::PROCESSINFOCLASS>   (args[1]);
    const auto ProcessInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ProcessInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationProcess)
    {
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationResourceManager(const on_NtQueryInformationResourceManager_fn& on_ntqueryinformationresourcemanager)
{
    d_->observers_NtQueryInformationResourceManager.push_back(on_ntqueryinformationresourcemanager);
}

void monitor::GenericMonitor::on_NtQueryInformationResourceManager()
{
    LOG(INFO, "Break on NtQueryInformationResourceManager");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResourceManagerInformationClass= nt::cast_to<nt::RESOURCEMANAGER_INFORMATION_CLASS>(args[1]);
    const auto ResourceManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ResourceManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationResourceManager)
    {
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationThread(const on_NtQueryInformationThread_fn& on_ntqueryinformationthread)
{
    d_->observers_NtQueryInformationThread.push_back(on_ntqueryinformationthread);
}

void monitor::GenericMonitor::on_NtQueryInformationThread()
{
    LOG(INFO, "Break on NtQueryInformationThread");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadInformationClass= nt::cast_to<nt::THREADINFOCLASS>    (args[1]);
    const auto ThreadInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ThreadInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationThread)
    {
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationToken(const on_NtQueryInformationToken_fn& on_ntqueryinformationtoken)
{
    d_->observers_NtQueryInformationToken.push_back(on_ntqueryinformationtoken);
}

void monitor::GenericMonitor::on_NtQueryInformationToken()
{
    LOG(INFO, "Break on NtQueryInformationToken");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TokenInformationClass= nt::cast_to<nt::TOKEN_INFORMATION_CLASS>(args[1]);
    const auto TokenInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TokenInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationToken)
    {
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationTransaction(const on_NtQueryInformationTransaction_fn& on_ntqueryinformationtransaction)
{
    d_->observers_NtQueryInformationTransaction.push_back(on_ntqueryinformationtransaction);
}

void monitor::GenericMonitor::on_NtQueryInformationTransaction()
{
    LOG(INFO, "Break on NtQueryInformationTransaction");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionInformationClass= nt::cast_to<nt::TRANSACTION_INFORMATION_CLASS>(args[1]);
    const auto TransactionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationTransaction)
    {
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationTransactionManager(const on_NtQueryInformationTransactionManager_fn& on_ntqueryinformationtransactionmanager)
{
    d_->observers_NtQueryInformationTransactionManager.push_back(on_ntqueryinformationtransactionmanager);
}

void monitor::GenericMonitor::on_NtQueryInformationTransactionManager()
{
    LOG(INFO, "Break on NtQueryInformationTransactionManager");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionManagerInformationClass= nt::cast_to<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(args[1]);
    const auto TransactionManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationTransactionManager)
    {
        it(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInformationWorkerFactory(const on_NtQueryInformationWorkerFactory_fn& on_ntqueryinformationworkerfactory)
{
    d_->observers_NtQueryInformationWorkerFactory.push_back(on_ntqueryinformationworkerfactory);
}

void monitor::GenericMonitor::on_NtQueryInformationWorkerFactory()
{
    LOG(INFO, "Break on NtQueryInformationWorkerFactory");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WorkerFactoryInformationClass= nt::cast_to<nt::WORKERFACTORYINFOCLASS>(args[1]);
    const auto WorkerFactoryInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto WorkerFactoryInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationWorkerFactory)
    {
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryInstallUILanguage(const on_NtQueryInstallUILanguage_fn& on_ntqueryinstalluilanguage)
{
    d_->observers_NtQueryInstallUILanguage.push_back(on_ntqueryinstalluilanguage);
}

void monitor::GenericMonitor::on_NtQueryInstallUILanguage()
{
    LOG(INFO, "Break on NtQueryInstallUILanguage");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARInstallUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtQueryInstallUILanguage)
    {
        it(STARInstallUILanguageId);
    }
}

void monitor::GenericMonitor::register_NtQueryIntervalProfile(const on_NtQueryIntervalProfile_fn& on_ntqueryintervalprofile)
{
    d_->observers_NtQueryIntervalProfile.push_back(on_ntqueryintervalprofile);
}

void monitor::GenericMonitor::on_NtQueryIntervalProfile()
{
    LOG(INFO, "Break on NtQueryIntervalProfile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileSource   = nt::cast_to<nt::KPROFILE_SOURCE>    (args[0]);
    const auto Interval        = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryIntervalProfile)
    {
        it(ProfileSource, Interval);
    }
}

void monitor::GenericMonitor::register_NtQueryIoCompletion(const on_NtQueryIoCompletion_fn& on_ntqueryiocompletion)
{
    d_->observers_NtQueryIoCompletion.push_back(on_ntqueryiocompletion);
}

void monitor::GenericMonitor::on_NtQueryIoCompletion()
{
    LOG(INFO, "Break on NtQueryIoCompletion");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionInformationClass= nt::cast_to<nt::IO_COMPLETION_INFORMATION_CLASS>(args[1]);
    const auto IoCompletionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoCompletionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryIoCompletion)
    {
        it(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryKey(const on_NtQueryKey_fn& on_ntquerykey)
{
    d_->observers_NtQueryKey.push_back(on_ntquerykey);
}

void monitor::GenericMonitor::on_NtQueryKey()
{
    LOG(INFO, "Break on NtQueryKey");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyInformationClass= nt::cast_to<nt::KEY_INFORMATION_CLASS>(args[1]);
    const auto KeyInformation  = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryKey)
    {
        it(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
    }
}

void monitor::GenericMonitor::register_NtQueryLicenseValue(const on_NtQueryLicenseValue_fn& on_ntquerylicensevalue)
{
    d_->observers_NtQueryLicenseValue.push_back(on_ntquerylicensevalue);
}

void monitor::GenericMonitor::on_NtQueryLicenseValue()
{
    LOG(INFO, "Break on NtQueryLicenseValue");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Name            = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto Type            = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnedLength  = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryLicenseValue)
    {
        it(Name, Type, Buffer, Length, ReturnedLength);
    }
}

void monitor::GenericMonitor::register_NtQueryMultipleValueKey(const on_NtQueryMultipleValueKey_fn& on_ntquerymultiplevaluekey)
{
    d_->observers_NtQueryMultipleValueKey.push_back(on_ntquerymultiplevaluekey);
}

void monitor::GenericMonitor::on_NtQueryMultipleValueKey()
{
    LOG(INFO, "Break on NtQueryMultipleValueKey");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueEntries    = nt::cast_to<nt::PKEY_VALUE_ENTRY>   (args[1]);
    const auto EntryCount      = nt::cast_to<nt::ULONG>              (args[2]);
    const auto ValueBuffer     = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[4]);
    const auto RequiredBufferLength= nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQueryMultipleValueKey)
    {
        it(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
    }
}

void monitor::GenericMonitor::register_NtQueryMutant(const on_NtQueryMutant_fn& on_ntquerymutant)
{
    d_->observers_NtQueryMutant.push_back(on_ntquerymutant);
}

void monitor::GenericMonitor::on_NtQueryMutant()
{
    LOG(INFO, "Break on NtQueryMutant");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto MutantInformationClass= nt::cast_to<nt::MUTANT_INFORMATION_CLASS>(args[1]);
    const auto MutantInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto MutantInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryMutant)
    {
        it(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryObject(const on_NtQueryObject_fn& on_ntqueryobject)
{
    d_->observers_NtQueryObject.push_back(on_ntqueryobject);
}

void monitor::GenericMonitor::on_NtQueryObject()
{
    LOG(INFO, "Break on NtQueryObject");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ObjectInformationClass= nt::cast_to<nt::OBJECT_INFORMATION_CLASS>(args[1]);
    const auto ObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryObject)
    {
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryOpenSubKeysEx(const on_NtQueryOpenSubKeysEx_fn& on_ntqueryopensubkeysex)
{
    d_->observers_NtQueryOpenSubKeysEx.push_back(on_ntqueryopensubkeysex);
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeysEx()
{
    LOG(INFO, "Break on NtQueryOpenSubKeysEx");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto RequiredSize    = nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtQueryOpenSubKeysEx)
    {
        it(TargetKey, BufferLength, Buffer, RequiredSize);
    }
}

void monitor::GenericMonitor::register_NtQueryOpenSubKeys(const on_NtQueryOpenSubKeys_fn& on_ntqueryopensubkeys)
{
    d_->observers_NtQueryOpenSubKeys.push_back(on_ntqueryopensubkeys);
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeys()
{
    LOG(INFO, "Break on NtQueryOpenSubKeys");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto HandleCount     = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryOpenSubKeys)
    {
        it(TargetKey, HandleCount);
    }
}

void monitor::GenericMonitor::register_NtQueryPerformanceCounter(const on_NtQueryPerformanceCounter_fn& on_ntqueryperformancecounter)
{
    d_->observers_NtQueryPerformanceCounter.push_back(on_ntqueryperformancecounter);
}

void monitor::GenericMonitor::on_NtQueryPerformanceCounter()
{
    LOG(INFO, "Break on NtQueryPerformanceCounter");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PerformanceCounter= nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto PerformanceFrequency= nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtQueryPerformanceCounter)
    {
        it(PerformanceCounter, PerformanceFrequency);
    }
}

void monitor::GenericMonitor::register_NtQueryQuotaInformationFile(const on_NtQueryQuotaInformationFile_fn& on_ntqueryquotainformationfile)
{
    d_->observers_NtQueryQuotaInformationFile.push_back(on_ntqueryquotainformationfile);
}

void monitor::GenericMonitor::on_NtQueryQuotaInformationFile()
{
    LOG(INFO, "Break on NtQueryQuotaInformationFile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnSingleEntry= nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto SidList         = nt::cast_to<nt::PVOID>              (args[5]);
    const auto SidListLength   = nt::cast_to<nt::ULONG>              (args[6]);
    const auto StartSid        = nt::cast_to<nt::PULONG>             (args[7]);
    const auto RestartScan     = nt::cast_to<nt::BOOLEAN>            (args[8]);

    for(const auto& it : d_->observers_NtQueryQuotaInformationFile)
    {
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
    }
}

void monitor::GenericMonitor::register_NtQuerySection(const on_NtQuerySection_fn& on_ntquerysection)
{
    d_->observers_NtQuerySection.push_back(on_ntquerysection);
}

void monitor::GenericMonitor::on_NtQuerySection()
{
    LOG(INFO, "Break on NtQuerySection");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SectionInformationClass= nt::cast_to<nt::SECTION_INFORMATION_CLASS>(args[1]);
    const auto SectionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto SectionInformationLength= nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtQuerySection)
    {
        it(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySecurityAttributesToken(const on_NtQuerySecurityAttributesToken_fn& on_ntquerysecurityattributestoken)
{
    d_->observers_NtQuerySecurityAttributesToken.push_back(on_ntquerysecurityattributestoken);
}

void monitor::GenericMonitor::on_NtQuerySecurityAttributesToken()
{
    LOG(INFO, "Break on NtQuerySecurityAttributesToken");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Attributes      = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto NumberOfAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQuerySecurityAttributesToken)
    {
        it(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySecurityObject(const on_NtQuerySecurityObject_fn& on_ntquerysecurityobject)
{
    d_->observers_NtQuerySecurityObject.push_back(on_ntquerysecurityobject);
}

void monitor::GenericMonitor::on_NtQuerySecurityObject()
{
    LOG(INFO, "Break on NtQuerySecurityObject");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecurityInformation= nt::cast_to<nt::SECURITY_INFORMATION>(args[1]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto LengthNeeded    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySecurityObject)
    {
        it(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
    }
}

void monitor::GenericMonitor::register_NtQuerySemaphore(const on_NtQuerySemaphore_fn& on_ntquerysemaphore)
{
    d_->observers_NtQuerySemaphore.push_back(on_ntquerysemaphore);
}

void monitor::GenericMonitor::on_NtQuerySemaphore()
{
    LOG(INFO, "Break on NtQuerySemaphore");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SemaphoreInformationClass= nt::cast_to<nt::SEMAPHORE_INFORMATION_CLASS>(args[1]);
    const auto SemaphoreInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto SemaphoreInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySemaphore)
    {
        it(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySymbolicLinkObject(const on_NtQuerySymbolicLinkObject_fn& on_ntquerysymboliclinkobject)
{
    d_->observers_NtQuerySymbolicLinkObject.push_back(on_ntquerysymboliclinkobject);
}

void monitor::GenericMonitor::on_NtQuerySymbolicLinkObject()
{
    LOG(INFO, "Break on NtQuerySymbolicLinkObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto LinkTarget      = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto ReturnedLength  = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtQuerySymbolicLinkObject)
    {
        it(LinkHandle, LinkTarget, ReturnedLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySystemEnvironmentValueEx(const on_NtQuerySystemEnvironmentValueEx_fn& on_ntquerysystemenvironmentvalueex)
{
    d_->observers_NtQuerySystemEnvironmentValueEx.push_back(on_ntquerysystemenvironmentvalueex);
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValueEx()
{
    LOG(INFO, "Break on NtQuerySystemEnvironmentValueEx");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VendorGuid      = nt::cast_to<nt::LPGUID>             (args[1]);
    const auto Value           = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ValueLength     = nt::cast_to<nt::PULONG>             (args[3]);
    const auto Attributes      = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValueEx)
    {
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }
}

void monitor::GenericMonitor::register_NtQuerySystemEnvironmentValue(const on_NtQuerySystemEnvironmentValue_fn& on_ntquerysystemenvironmentvalue)
{
    d_->observers_NtQuerySystemEnvironmentValue.push_back(on_ntquerysystemenvironmentvalue);
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValue()
{
    LOG(INFO, "Break on NtQuerySystemEnvironmentValue");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VariableValue   = nt::cast_to<nt::PWSTR>              (args[1]);
    const auto ValueLength     = nt::cast_to<nt::USHORT>             (args[2]);
    const auto ReturnLength    = nt::cast_to<nt::PUSHORT>            (args[3]);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValue)
    {
        it(VariableName, VariableValue, ValueLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySystemInformationEx(const on_NtQuerySystemInformationEx_fn& on_ntquerysysteminformationex)
{
    d_->observers_NtQuerySystemInformationEx.push_back(on_ntquerysysteminformationex);
}

void monitor::GenericMonitor::on_NtQuerySystemInformationEx()
{
    LOG(INFO, "Break on NtQuerySystemInformationEx");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto QueryInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto QueryInformationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQuerySystemInformationEx)
    {
        it(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySystemInformation(const on_NtQuerySystemInformation_fn& on_ntquerysysteminformation)
{
    d_->observers_NtQuerySystemInformation.push_back(on_ntquerysysteminformation);
}

void monitor::GenericMonitor::on_NtQuerySystemInformation()
{
    LOG(INFO, "Break on NtQuerySystemInformation");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtQuerySystemInformation)
    {
        it(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQuerySystemTime(const on_NtQuerySystemTime_fn& on_ntquerysystemtime)
{
    d_->observers_NtQuerySystemTime.push_back(on_ntquerysystemtime);
}

void monitor::GenericMonitor::on_NtQuerySystemTime()
{
    LOG(INFO, "Break on NtQuerySystemTime");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemTime      = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);

    for(const auto& it : d_->observers_NtQuerySystemTime)
    {
        it(SystemTime);
    }
}

void monitor::GenericMonitor::register_NtQueryTimer(const on_NtQueryTimer_fn& on_ntquerytimer)
{
    d_->observers_NtQueryTimer.push_back(on_ntquerytimer);
}

void monitor::GenericMonitor::on_NtQueryTimer()
{
    LOG(INFO, "Break on NtQueryTimer");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TimerInformationClass= nt::cast_to<nt::TIMER_INFORMATION_CLASS>(args[1]);
    const auto TimerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TimerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryTimer)
    {
        it(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryTimerResolution(const on_NtQueryTimerResolution_fn& on_ntquerytimerresolution)
{
    d_->observers_NtQueryTimerResolution.push_back(on_ntquerytimerresolution);
}

void monitor::GenericMonitor::on_NtQueryTimerResolution()
{
    LOG(INFO, "Break on NtQueryTimerResolution");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MaximumTime     = nt::cast_to<nt::PULONG>             (args[0]);
    const auto MinimumTime     = nt::cast_to<nt::PULONG>             (args[1]);
    const auto CurrentTime     = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtQueryTimerResolution)
    {
        it(MaximumTime, MinimumTime, CurrentTime);
    }
}

void monitor::GenericMonitor::register_NtQueryValueKey(const on_NtQueryValueKey_fn& on_ntqueryvaluekey)
{
    d_->observers_NtQueryValueKey.push_back(on_ntqueryvaluekey);
}

void monitor::GenericMonitor::on_NtQueryValueKey()
{
    LOG(INFO, "Break on NtQueryValueKey");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto KeyValueInformationClass= nt::cast_to<nt::KEY_VALUE_INFORMATION_CLASS>(args[2]);
    const auto KeyValueInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQueryValueKey)
    {
        it(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    }
}

void monitor::GenericMonitor::register_NtQueryVirtualMemory(const on_NtQueryVirtualMemory_fn& on_ntqueryvirtualmemory)
{
    d_->observers_NtQueryVirtualMemory.push_back(on_ntqueryvirtualmemory);
}

void monitor::GenericMonitor::on_NtQueryVirtualMemory()
{
    LOG(INFO, "Break on NtQueryVirtualMemory");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto MemoryInformationClass= nt::cast_to<nt::MEMORY_INFORMATION_CLASS>(args[2]);
    const auto MemoryInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto MemoryInformationLength= nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtQueryVirtualMemory)
    {
        it(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtQueryVolumeInformationFile(const on_NtQueryVolumeInformationFile_fn& on_ntqueryvolumeinformationfile)
{
    d_->observers_NtQueryVolumeInformationFile.push_back(on_ntqueryvolumeinformationfile);
}

void monitor::GenericMonitor::on_NtQueryVolumeInformationFile()
{
    LOG(INFO, "Break on NtQueryVolumeInformationFile");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FsInformation   = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FsInformationClass= nt::cast_to<nt::FS_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtQueryVolumeInformationFile)
    {
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }
}

void monitor::GenericMonitor::register_NtQueueApcThreadEx(const on_NtQueueApcThreadEx_fn& on_ntqueueapcthreadex)
{
    d_->observers_NtQueueApcThreadEx.push_back(on_ntqueueapcthreadex);
}

void monitor::GenericMonitor::on_NtQueueApcThreadEx()
{
    LOG(INFO, "Break on NtQueueApcThreadEx");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto UserApcReserveHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PPS_APC_ROUTINE>    (args[2]);
    const auto ApcArgument1    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto ApcArgument2    = nt::cast_to<nt::PVOID>              (args[4]);
    const auto ApcArgument3    = nt::cast_to<nt::PVOID>              (args[5]);

    for(const auto& it : d_->observers_NtQueueApcThreadEx)
    {
        it(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }
}

void monitor::GenericMonitor::register_NtQueueApcThread(const on_NtQueueApcThread_fn& on_ntqueueapcthread)
{
    d_->observers_NtQueueApcThread.push_back(on_ntqueueapcthread);
}

void monitor::GenericMonitor::on_NtQueueApcThread()
{
    LOG(INFO, "Break on NtQueueApcThread");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ApcRoutine      = nt::cast_to<nt::PPS_APC_ROUTINE>    (args[1]);
    const auto ApcArgument1    = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ApcArgument2    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto ApcArgument3    = nt::cast_to<nt::PVOID>              (args[4]);

    for(const auto& it : d_->observers_NtQueueApcThread)
    {
        it(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
    }
}

void monitor::GenericMonitor::register_NtRaiseException(const on_NtRaiseException_fn& on_ntraiseexception)
{
    d_->observers_NtRaiseException.push_back(on_ntraiseexception);
}

void monitor::GenericMonitor::on_NtRaiseException()
{
    LOG(INFO, "Break on NtRaiseException");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExceptionRecord = nt::cast_to<nt::PEXCEPTION_RECORD>  (args[0]);
    const auto ContextRecord   = nt::cast_to<nt::PCONTEXT>           (args[1]);
    const auto FirstChance     = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtRaiseException)
    {
        it(ExceptionRecord, ContextRecord, FirstChance);
    }
}

void monitor::GenericMonitor::register_NtRaiseHardError(const on_NtRaiseHardError_fn& on_ntraiseharderror)
{
    d_->observers_NtRaiseHardError.push_back(on_ntraiseharderror);
}

void monitor::GenericMonitor::on_NtRaiseHardError()
{
    LOG(INFO, "Break on NtRaiseHardError");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ErrorStatus     = nt::cast_to<nt::NTSTATUS>           (args[0]);
    const auto NumberOfParameters= nt::cast_to<nt::ULONG>              (args[1]);
    const auto UnicodeStringParameterMask= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Parameters      = nt::cast_to<nt::PULONG_PTR>         (args[3]);
    const auto ValidResponseOptions= nt::cast_to<nt::ULONG>              (args[4]);
    const auto Response        = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtRaiseHardError)
    {
        it(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
    }
}

void monitor::GenericMonitor::register_NtReadFile(const on_NtReadFile_fn& on_ntreadfile)
{
    d_->observers_NtReadFile.push_back(on_ntreadfile);
}

void monitor::GenericMonitor::on_NtReadFile()
{
    LOG(INFO, "Break on NtReadFile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);
    const auto Key             = nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto& it : d_->observers_NtReadFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }
}

void monitor::GenericMonitor::register_NtReadFileScatter(const on_NtReadFileScatter_fn& on_ntreadfilescatter)
{
    d_->observers_NtReadFileScatter.push_back(on_ntreadfilescatter);
}

void monitor::GenericMonitor::on_NtReadFileScatter()
{
    LOG(INFO, "Break on NtReadFileScatter");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto SegmentArray    = nt::cast_to<nt::PFILE_SEGMENT_ELEMENT>(args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);
    const auto Key             = nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto& it : d_->observers_NtReadFileScatter)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }
}

void monitor::GenericMonitor::register_NtReadOnlyEnlistment(const on_NtReadOnlyEnlistment_fn& on_ntreadonlyenlistment)
{
    d_->observers_NtReadOnlyEnlistment.push_back(on_ntreadonlyenlistment);
}

void monitor::GenericMonitor::on_NtReadOnlyEnlistment()
{
    LOG(INFO, "Break on NtReadOnlyEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtReadOnlyEnlistment)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtReadRequestData(const on_NtReadRequestData_fn& on_ntreadrequestdata)
{
    d_->observers_NtReadRequestData.push_back(on_ntreadrequestdata);
}

void monitor::GenericMonitor::on_NtReadRequestData()
{
    LOG(INFO, "Break on NtReadRequestData");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto DataEntryIndex  = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto NumberOfBytesRead= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtReadRequestData)
    {
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
    }
}

void monitor::GenericMonitor::register_NtReadVirtualMemory(const on_NtReadVirtualMemory_fn& on_ntreadvirtualmemory)
{
    d_->observers_NtReadVirtualMemory.push_back(on_ntreadvirtualmemory);
}

void monitor::GenericMonitor::on_NtReadVirtualMemory()
{
    LOG(INFO, "Break on NtReadVirtualMemory");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto NumberOfBytesRead= nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtReadVirtualMemory)
    {
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
    }
}

void monitor::GenericMonitor::register_NtRecoverEnlistment(const on_NtRecoverEnlistment_fn& on_ntrecoverenlistment)
{
    d_->observers_NtRecoverEnlistment.push_back(on_ntrecoverenlistment);
}

void monitor::GenericMonitor::on_NtRecoverEnlistment()
{
    LOG(INFO, "Break on NtRecoverEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentKey   = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtRecoverEnlistment)
    {
        it(EnlistmentHandle, EnlistmentKey);
    }
}

void monitor::GenericMonitor::register_NtRecoverResourceManager(const on_NtRecoverResourceManager_fn& on_ntrecoverresourcemanager)
{
    d_->observers_NtRecoverResourceManager.push_back(on_ntrecoverresourcemanager);
}

void monitor::GenericMonitor::on_NtRecoverResourceManager()
{
    LOG(INFO, "Break on NtRecoverResourceManager");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRecoverResourceManager)
    {
        it(ResourceManagerHandle);
    }
}

void monitor::GenericMonitor::register_NtRecoverTransactionManager(const on_NtRecoverTransactionManager_fn& on_ntrecovertransactionmanager)
{
    d_->observers_NtRecoverTransactionManager.push_back(on_ntrecovertransactionmanager);
}

void monitor::GenericMonitor::on_NtRecoverTransactionManager()
{
    LOG(INFO, "Break on NtRecoverTransactionManager");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRecoverTransactionManager)
    {
        it(TransactionManagerHandle);
    }
}

void monitor::GenericMonitor::register_NtRegisterProtocolAddressInformation(const on_NtRegisterProtocolAddressInformation_fn& on_ntregisterprotocoladdressinformation)
{
    d_->observers_NtRegisterProtocolAddressInformation.push_back(on_ntregisterprotocoladdressinformation);
}

void monitor::GenericMonitor::on_NtRegisterProtocolAddressInformation()
{
    LOG(INFO, "Break on NtRegisterProtocolAddressInformation");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManager = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProtocolId      = nt::cast_to<nt::PCRM_PROTOCOL_ID>   (args[1]);
    const auto ProtocolInformationSize= nt::cast_to<nt::ULONG>              (args[2]);
    const auto ProtocolInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtRegisterProtocolAddressInformation)
    {
        it(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
    }
}

void monitor::GenericMonitor::register_NtRegisterThreadTerminatePort(const on_NtRegisterThreadTerminatePort_fn& on_ntregisterthreadterminateport)
{
    d_->observers_NtRegisterThreadTerminatePort.push_back(on_ntregisterthreadterminateport);
}

void monitor::GenericMonitor::on_NtRegisterThreadTerminatePort()
{
    LOG(INFO, "Break on NtRegisterThreadTerminatePort");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRegisterThreadTerminatePort)
    {
        it(PortHandle);
    }
}

void monitor::GenericMonitor::register_NtReleaseKeyedEvent(const on_NtReleaseKeyedEvent_fn& on_ntreleasekeyedevent)
{
    d_->observers_NtReleaseKeyedEvent.push_back(on_ntreleasekeyedevent);
}

void monitor::GenericMonitor::on_NtReleaseKeyedEvent()
{
    LOG(INFO, "Break on NtReleaseKeyedEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyValue        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtReleaseKeyedEvent)
    {
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtReleaseMutant(const on_NtReleaseMutant_fn& on_ntreleasemutant)
{
    d_->observers_NtReleaseMutant.push_back(on_ntreleasemutant);
}

void monitor::GenericMonitor::on_NtReleaseMutant()
{
    LOG(INFO, "Break on NtReleaseMutant");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousCount   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtReleaseMutant)
    {
        it(MutantHandle, PreviousCount);
    }
}

void monitor::GenericMonitor::register_NtReleaseSemaphore(const on_NtReleaseSemaphore_fn& on_ntreleasesemaphore)
{
    d_->observers_NtReleaseSemaphore.push_back(on_ntreleasesemaphore);
}

void monitor::GenericMonitor::on_NtReleaseSemaphore()
{
    LOG(INFO, "Break on NtReleaseSemaphore");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReleaseCount    = nt::cast_to<nt::LONG>               (args[1]);
    const auto PreviousCount   = nt::cast_to<nt::PLONG>              (args[2]);

    for(const auto& it : d_->observers_NtReleaseSemaphore)
    {
        it(SemaphoreHandle, ReleaseCount, PreviousCount);
    }
}

void monitor::GenericMonitor::register_NtReleaseWorkerFactoryWorker(const on_NtReleaseWorkerFactoryWorker_fn& on_ntreleaseworkerfactoryworker)
{
    d_->observers_NtReleaseWorkerFactoryWorker.push_back(on_ntreleaseworkerfactoryworker);
}

void monitor::GenericMonitor::on_NtReleaseWorkerFactoryWorker()
{
    LOG(INFO, "Break on NtReleaseWorkerFactoryWorker");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtReleaseWorkerFactoryWorker)
    {
        it(WorkerFactoryHandle);
    }
}

void monitor::GenericMonitor::register_NtRemoveIoCompletionEx(const on_NtRemoveIoCompletionEx_fn& on_ntremoveiocompletionex)
{
    d_->observers_NtRemoveIoCompletionEx.push_back(on_ntremoveiocompletionex);
}

void monitor::GenericMonitor::on_NtRemoveIoCompletionEx()
{
    LOG(INFO, "Break on NtRemoveIoCompletionEx");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionInformation= nt::cast_to<nt::PFILE_IO_COMPLETION_INFORMATION>(args[1]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto NumEntriesRemoved= nt::cast_to<nt::PULONG>             (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[5]);

    for(const auto& it : d_->observers_NtRemoveIoCompletionEx)
    {
        it(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
    }
}

void monitor::GenericMonitor::register_NtRemoveIoCompletion(const on_NtRemoveIoCompletion_fn& on_ntremoveiocompletion)
{
    d_->observers_NtRemoveIoCompletion.push_back(on_ntremoveiocompletion);
}

void monitor::GenericMonitor::on_NtRemoveIoCompletion()
{
    LOG(INFO, "Break on NtRemoveIoCompletion");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARKeyContext  = nt::cast_to<nt::PVOID>              (args[1]);
    const auto STARApcContext  = nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtRemoveIoCompletion)
    {
        it(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
    }
}

void monitor::GenericMonitor::register_NtRemoveProcessDebug(const on_NtRemoveProcessDebug_fn& on_ntremoveprocessdebug)
{
    d_->observers_NtRemoveProcessDebug.push_back(on_ntremoveprocessdebug);
}

void monitor::GenericMonitor::on_NtRemoveProcessDebug()
{
    LOG(INFO, "Break on NtRemoveProcessDebug");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtRemoveProcessDebug)
    {
        it(ProcessHandle, DebugObjectHandle);
    }
}

void monitor::GenericMonitor::register_NtRenameKey(const on_NtRenameKey_fn& on_ntrenamekey)
{
    d_->observers_NtRenameKey.push_back(on_ntrenamekey);
}

void monitor::GenericMonitor::on_NtRenameKey()
{
    LOG(INFO, "Break on NtRenameKey");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NewName         = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtRenameKey)
    {
        it(KeyHandle, NewName);
    }
}

void monitor::GenericMonitor::register_NtRenameTransactionManager(const on_NtRenameTransactionManager_fn& on_ntrenametransactionmanager)
{
    d_->observers_NtRenameTransactionManager.push_back(on_ntrenametransactionmanager);
}

void monitor::GenericMonitor::on_NtRenameTransactionManager()
{
    LOG(INFO, "Break on NtRenameTransactionManager");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto ExistingTransactionManagerGuid= nt::cast_to<nt::LPGUID>             (args[1]);

    for(const auto& it : d_->observers_NtRenameTransactionManager)
    {
        it(LogFileName, ExistingTransactionManagerGuid);
    }
}

void monitor::GenericMonitor::register_NtReplaceKey(const on_NtReplaceKey_fn& on_ntreplacekey)
{
    d_->observers_NtReplaceKey.push_back(on_ntreplacekey);
}

void monitor::GenericMonitor::on_NtReplaceKey()
{
    LOG(INFO, "Break on NtReplaceKey");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NewFile         = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto TargetHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto OldFile         = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtReplaceKey)
    {
        it(NewFile, TargetHandle, OldFile);
    }
}

void monitor::GenericMonitor::register_NtReplacePartitionUnit(const on_NtReplacePartitionUnit_fn& on_ntreplacepartitionunit)
{
    d_->observers_NtReplacePartitionUnit.push_back(on_ntreplacepartitionunit);
}

void monitor::GenericMonitor::on_NtReplacePartitionUnit()
{
    LOG(INFO, "Break on NtReplacePartitionUnit");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetInstancePath= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto SpareInstancePath= nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtReplacePartitionUnit)
    {
        it(TargetInstancePath, SpareInstancePath, Flags);
    }
}

void monitor::GenericMonitor::register_NtReplyPort(const on_NtReplyPort_fn& on_ntreplyport)
{
    d_->observers_NtReplyPort.push_back(on_ntreplyport);
}

void monitor::GenericMonitor::on_NtReplyPort()
{
    LOG(INFO, "Break on NtReplyPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtReplyPort)
    {
        it(PortHandle, ReplyMessage);
    }
}

void monitor::GenericMonitor::register_NtReplyWaitReceivePortEx(const on_NtReplyWaitReceivePortEx_fn& on_ntreplywaitreceiveportex)
{
    d_->observers_NtReplyWaitReceivePortEx.push_back(on_ntreplywaitreceiveportex);
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePortEx()
{
    LOG(INFO, "Break on NtReplyWaitReceivePortEx");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPortContext = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto ReceiveMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtReplyWaitReceivePortEx)
    {
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
    }
}

void monitor::GenericMonitor::register_NtReplyWaitReceivePort(const on_NtReplyWaitReceivePort_fn& on_ntreplywaitreceiveport)
{
    d_->observers_NtReplyWaitReceivePort.push_back(on_ntreplywaitreceiveport);
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePort()
{
    LOG(INFO, "Break on NtReplyWaitReceivePort");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPortContext = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto ReceiveMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[3]);

    for(const auto& it : d_->observers_NtReplyWaitReceivePort)
    {
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
    }
}

void monitor::GenericMonitor::register_NtReplyWaitReplyPort(const on_NtReplyWaitReplyPort_fn& on_ntreplywaitreplyport)
{
    d_->observers_NtReplyWaitReplyPort.push_back(on_ntreplywaitreplyport);
}

void monitor::GenericMonitor::on_NtReplyWaitReplyPort()
{
    LOG(INFO, "Break on NtReplyWaitReplyPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtReplyWaitReplyPort)
    {
        it(PortHandle, ReplyMessage);
    }
}

void monitor::GenericMonitor::register_NtRequestPort(const on_NtRequestPort_fn& on_ntrequestport)
{
    d_->observers_NtRequestPort.push_back(on_ntrequestport);
}

void monitor::GenericMonitor::on_NtRequestPort()
{
    LOG(INFO, "Break on NtRequestPort");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtRequestPort)
    {
        it(PortHandle, RequestMessage);
    }
}

void monitor::GenericMonitor::register_NtRequestWaitReplyPort(const on_NtRequestWaitReplyPort_fn& on_ntrequestwaitreplyport)
{
    d_->observers_NtRequestWaitReplyPort.push_back(on_ntrequestwaitreplyport);
}

void monitor::GenericMonitor::on_NtRequestWaitReplyPort()
{
    LOG(INFO, "Break on NtRequestWaitReplyPort");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);

    for(const auto& it : d_->observers_NtRequestWaitReplyPort)
    {
        it(PortHandle, RequestMessage, ReplyMessage);
    }
}

void monitor::GenericMonitor::register_NtResetEvent(const on_NtResetEvent_fn& on_ntresetevent)
{
    d_->observers_NtResetEvent.push_back(on_ntresetevent);
}

void monitor::GenericMonitor::on_NtResetEvent()
{
    LOG(INFO, "Break on NtResetEvent");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtResetEvent)
    {
        it(EventHandle, PreviousState);
    }
}

void monitor::GenericMonitor::register_NtResetWriteWatch(const on_NtResetWriteWatch_fn& on_ntresetwritewatch)
{
    d_->observers_NtResetWriteWatch.push_back(on_ntresetwritewatch);
}

void monitor::GenericMonitor::on_NtResetWriteWatch()
{
    LOG(INFO, "Break on NtResetWriteWatch");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::SIZE_T>             (args[2]);

    for(const auto& it : d_->observers_NtResetWriteWatch)
    {
        it(ProcessHandle, BaseAddress, RegionSize);
    }
}

void monitor::GenericMonitor::register_NtRestoreKey(const on_NtRestoreKey_fn& on_ntrestorekey)
{
    d_->observers_NtRestoreKey.push_back(on_ntrestorekey);
}

void monitor::GenericMonitor::on_NtRestoreKey()
{
    LOG(INFO, "Break on NtRestoreKey");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtRestoreKey)
    {
        it(KeyHandle, FileHandle, Flags);
    }
}

void monitor::GenericMonitor::register_NtResumeProcess(const on_NtResumeProcess_fn& on_ntresumeprocess)
{
    d_->observers_NtResumeProcess.push_back(on_ntresumeprocess);
}

void monitor::GenericMonitor::on_NtResumeProcess()
{
    LOG(INFO, "Break on NtResumeProcess");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtResumeProcess)
    {
        it(ProcessHandle);
    }
}

void monitor::GenericMonitor::register_NtResumeThread(const on_NtResumeThread_fn& on_ntresumethread)
{
    d_->observers_NtResumeThread.push_back(on_ntresumethread);
}

void monitor::GenericMonitor::on_NtResumeThread()
{
    LOG(INFO, "Break on NtResumeThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtResumeThread)
    {
        it(ThreadHandle, PreviousSuspendCount);
    }
}

void monitor::GenericMonitor::register_NtRollbackComplete(const on_NtRollbackComplete_fn& on_ntrollbackcomplete)
{
    d_->observers_NtRollbackComplete.push_back(on_ntrollbackcomplete);
}

void monitor::GenericMonitor::on_NtRollbackComplete()
{
    LOG(INFO, "Break on NtRollbackComplete");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollbackComplete)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtRollbackEnlistment(const on_NtRollbackEnlistment_fn& on_ntrollbackenlistment)
{
    d_->observers_NtRollbackEnlistment.push_back(on_ntrollbackenlistment);
}

void monitor::GenericMonitor::on_NtRollbackEnlistment()
{
    LOG(INFO, "Break on NtRollbackEnlistment");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollbackEnlistment)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtRollbackTransaction(const on_NtRollbackTransaction_fn& on_ntrollbacktransaction)
{
    d_->observers_NtRollbackTransaction.push_back(on_ntrollbacktransaction);
}

void monitor::GenericMonitor::on_NtRollbackTransaction()
{
    LOG(INFO, "Break on NtRollbackTransaction");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Wait            = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtRollbackTransaction)
    {
        it(TransactionHandle, Wait);
    }
}

void monitor::GenericMonitor::register_NtRollforwardTransactionManager(const on_NtRollforwardTransactionManager_fn& on_ntrollforwardtransactionmanager)
{
    d_->observers_NtRollforwardTransactionManager.push_back(on_ntrollforwardtransactionmanager);
}

void monitor::GenericMonitor::on_NtRollforwardTransactionManager()
{
    LOG(INFO, "Break on NtRollforwardTransactionManager");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollforwardTransactionManager)
    {
        it(TransactionManagerHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtSaveKeyEx(const on_NtSaveKeyEx_fn& on_ntsavekeyex)
{
    d_->observers_NtSaveKeyEx.push_back(on_ntsavekeyex);
}

void monitor::GenericMonitor::on_NtSaveKeyEx()
{
    LOG(INFO, "Break on NtSaveKeyEx");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Format          = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSaveKeyEx)
    {
        it(KeyHandle, FileHandle, Format);
    }
}

void monitor::GenericMonitor::register_NtSaveKey(const on_NtSaveKey_fn& on_ntsavekey)
{
    d_->observers_NtSaveKey.push_back(on_ntsavekey);
}

void monitor::GenericMonitor::on_NtSaveKey()
{
    LOG(INFO, "Break on NtSaveKey");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtSaveKey)
    {
        it(KeyHandle, FileHandle);
    }
}

void monitor::GenericMonitor::register_NtSaveMergedKeys(const on_NtSaveMergedKeys_fn& on_ntsavemergedkeys)
{
    d_->observers_NtSaveMergedKeys.push_back(on_ntsavemergedkeys);
}

void monitor::GenericMonitor::on_NtSaveMergedKeys()
{
    LOG(INFO, "Break on NtSaveMergedKeys");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HighPrecedenceKeyHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto LowPrecedenceKeyHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[2]);

    for(const auto& it : d_->observers_NtSaveMergedKeys)
    {
        it(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
    }
}

void monitor::GenericMonitor::register_NtSecureConnectPort(const on_NtSecureConnectPort_fn& on_ntsecureconnectport)
{
    d_->observers_NtSecureConnectPort.push_back(on_ntsecureconnectport);
}

void monitor::GenericMonitor::on_NtSecureConnectPort()
{
    LOG(INFO, "Break on NtSecureConnectPort");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortName        = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto SecurityQos     = nt::cast_to<nt::PSECURITY_QUALITY_OF_SERVICE>(args[2]);
    const auto ClientView      = nt::cast_to<nt::PPORT_VIEW>         (args[3]);
    const auto RequiredServerSid= nt::cast_to<nt::PSID>               (args[4]);
    const auto ServerView      = nt::cast_to<nt::PREMOTE_PORT_VIEW>  (args[5]);
    const auto MaxMessageLength= nt::cast_to<nt::PULONG>             (args[6]);
    const auto ConnectionInformation= nt::cast_to<nt::PVOID>              (args[7]);
    const auto ConnectionInformationLength= nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto& it : d_->observers_NtSecureConnectPort)
    {
        it(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetBootEntryOrder(const on_NtSetBootEntryOrder_fn& on_ntsetbootentryorder)
{
    d_->observers_NtSetBootEntryOrder.push_back(on_ntsetbootentryorder);
}

void monitor::GenericMonitor::on_NtSetBootEntryOrder()
{
    LOG(INFO, "Break on NtSetBootEntryOrder");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetBootEntryOrder)
    {
        it(Ids, Count);
    }
}

void monitor::GenericMonitor::register_NtSetBootOptions(const on_NtSetBootOptions_fn& on_ntsetbootoptions)
{
    d_->observers_NtSetBootOptions.push_back(on_ntsetbootoptions);
}

void monitor::GenericMonitor::on_NtSetBootOptions()
{
    LOG(INFO, "Break on NtSetBootOptions");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootOptions     = nt::cast_to<nt::PBOOT_OPTIONS>      (args[0]);
    const auto FieldsToChange  = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetBootOptions)
    {
        it(BootOptions, FieldsToChange);
    }
}

void monitor::GenericMonitor::register_NtSetContextThread(const on_NtSetContextThread_fn& on_ntsetcontextthread)
{
    d_->observers_NtSetContextThread.push_back(on_ntsetcontextthread);
}

void monitor::GenericMonitor::on_NtSetContextThread()
{
    LOG(INFO, "Break on NtSetContextThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadContext   = nt::cast_to<nt::PCONTEXT>           (args[1]);

    for(const auto& it : d_->observers_NtSetContextThread)
    {
        it(ThreadHandle, ThreadContext);
    }
}

void monitor::GenericMonitor::register_NtSetDebugFilterState(const on_NtSetDebugFilterState_fn& on_ntsetdebugfilterstate)
{
    d_->observers_NtSetDebugFilterState.push_back(on_ntsetdebugfilterstate);
}

void monitor::GenericMonitor::on_NtSetDebugFilterState()
{
    LOG(INFO, "Break on NtSetDebugFilterState");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ComponentId     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Level           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto State           = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtSetDebugFilterState)
    {
        it(ComponentId, Level, State);
    }
}

void monitor::GenericMonitor::register_NtSetDefaultHardErrorPort(const on_NtSetDefaultHardErrorPort_fn& on_ntsetdefaultharderrorport)
{
    d_->observers_NtSetDefaultHardErrorPort.push_back(on_ntsetdefaultharderrorport);
}

void monitor::GenericMonitor::on_NtSetDefaultHardErrorPort()
{
    LOG(INFO, "Break on NtSetDefaultHardErrorPort");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DefaultHardErrorPort= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetDefaultHardErrorPort)
    {
        it(DefaultHardErrorPort);
    }
}

void monitor::GenericMonitor::register_NtSetDefaultLocale(const on_NtSetDefaultLocale_fn& on_ntsetdefaultlocale)
{
    d_->observers_NtSetDefaultLocale.push_back(on_ntsetdefaultlocale);
}

void monitor::GenericMonitor::on_NtSetDefaultLocale()
{
    LOG(INFO, "Break on NtSetDefaultLocale");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto UserProfile     = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::LCID>               (args[1]);

    for(const auto& it : d_->observers_NtSetDefaultLocale)
    {
        it(UserProfile, DefaultLocaleId);
    }
}

void monitor::GenericMonitor::register_NtSetDefaultUILanguage(const on_NtSetDefaultUILanguage_fn& on_ntsetdefaultuilanguage)
{
    d_->observers_NtSetDefaultUILanguage.push_back(on_ntsetdefaultuilanguage);
}

void monitor::GenericMonitor::on_NtSetDefaultUILanguage()
{
    LOG(INFO, "Break on NtSetDefaultUILanguage");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DefaultUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtSetDefaultUILanguage)
    {
        it(DefaultUILanguageId);
    }
}

void monitor::GenericMonitor::register_NtSetDriverEntryOrder(const on_NtSetDriverEntryOrder_fn& on_ntsetdriverentryorder)
{
    d_->observers_NtSetDriverEntryOrder.push_back(on_ntsetdriverentryorder);
}

void monitor::GenericMonitor::on_NtSetDriverEntryOrder()
{
    LOG(INFO, "Break on NtSetDriverEntryOrder");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetDriverEntryOrder)
    {
        it(Ids, Count);
    }
}

void monitor::GenericMonitor::register_NtSetEaFile(const on_NtSetEaFile_fn& on_ntseteafile)
{
    d_->observers_NtSetEaFile.push_back(on_ntseteafile);
}

void monitor::GenericMonitor::on_NtSetEaFile()
{
    LOG(INFO, "Break on NtSetEaFile");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetEaFile)
    {
        it(FileHandle, IoStatusBlock, Buffer, Length);
    }
}

void monitor::GenericMonitor::register_NtSetEventBoostPriority(const on_NtSetEventBoostPriority_fn& on_ntseteventboostpriority)
{
    d_->observers_NtSetEventBoostPriority.push_back(on_ntseteventboostpriority);
}

void monitor::GenericMonitor::on_NtSetEventBoostPriority()
{
    LOG(INFO, "Break on NtSetEventBoostPriority");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetEventBoostPriority)
    {
        it(EventHandle);
    }
}

void monitor::GenericMonitor::register_NtSetEvent(const on_NtSetEvent_fn& on_ntsetevent)
{
    d_->observers_NtSetEvent.push_back(on_ntsetevent);
}

void monitor::GenericMonitor::on_NtSetEvent()
{
    LOG(INFO, "Break on NtSetEvent");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetEvent)
    {
        it(EventHandle, PreviousState);
    }
}

void monitor::GenericMonitor::register_NtSetHighEventPair(const on_NtSetHighEventPair_fn& on_ntsethigheventpair)
{
    d_->observers_NtSetHighEventPair.push_back(on_ntsethigheventpair);
}

void monitor::GenericMonitor::on_NtSetHighEventPair()
{
    LOG(INFO, "Break on NtSetHighEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetHighEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtSetHighWaitLowEventPair(const on_NtSetHighWaitLowEventPair_fn& on_ntsethighwaitloweventpair)
{
    d_->observers_NtSetHighWaitLowEventPair.push_back(on_ntsethighwaitloweventpair);
}

void monitor::GenericMonitor::on_NtSetHighWaitLowEventPair()
{
    LOG(INFO, "Break on NtSetHighWaitLowEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetHighWaitLowEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtSetInformationDebugObject(const on_NtSetInformationDebugObject_fn& on_ntsetinformationdebugobject)
{
    d_->observers_NtSetInformationDebugObject.push_back(on_ntsetinformationdebugobject);
}

void monitor::GenericMonitor::on_NtSetInformationDebugObject()
{
    LOG(INFO, "Break on NtSetInformationDebugObject");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectInformationClass= nt::cast_to<nt::DEBUGOBJECTINFOCLASS>(args[1]);
    const auto DebugInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto DebugInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtSetInformationDebugObject)
    {
        it(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationEnlistment(const on_NtSetInformationEnlistment_fn& on_ntsetinformationenlistment)
{
    d_->observers_NtSetInformationEnlistment.push_back(on_ntsetinformationenlistment);
}

void monitor::GenericMonitor::on_NtSetInformationEnlistment()
{
    LOG(INFO, "Break on NtSetInformationEnlistment");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentInformationClass= nt::cast_to<nt::ENLISTMENT_INFORMATION_CLASS>(args[1]);
    const auto EnlistmentInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EnlistmentInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationEnlistment)
    {
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationFile(const on_NtSetInformationFile_fn& on_ntsetinformationfile)
{
    d_->observers_NtSetInformationFile.push_back(on_ntsetinformationfile);
}

void monitor::GenericMonitor::on_NtSetInformationFile()
{
    LOG(INFO, "Break on NtSetInformationFile");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FileInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FileInformationClass= nt::cast_to<nt::FILE_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtSetInformationFile)
    {
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
    }
}

void monitor::GenericMonitor::register_NtSetInformationJobObject(const on_NtSetInformationJobObject_fn& on_ntsetinformationjobobject)
{
    d_->observers_NtSetInformationJobObject.push_back(on_ntsetinformationjobobject);
}

void monitor::GenericMonitor::on_NtSetInformationJobObject()
{
    LOG(INFO, "Break on NtSetInformationJobObject");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobObjectInformationClass= nt::cast_to<nt::JOBOBJECTINFOCLASS> (args[1]);
    const auto JobObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto JobObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationJobObject)
    {
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationKey(const on_NtSetInformationKey_fn& on_ntsetinformationkey)
{
    d_->observers_NtSetInformationKey.push_back(on_ntsetinformationkey);
}

void monitor::GenericMonitor::on_NtSetInformationKey()
{
    LOG(INFO, "Break on NtSetInformationKey");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeySetInformationClass= nt::cast_to<nt::KEY_SET_INFORMATION_CLASS>(args[1]);
    const auto KeySetInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto KeySetInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationKey)
    {
        it(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationObject(const on_NtSetInformationObject_fn& on_ntsetinformationobject)
{
    d_->observers_NtSetInformationObject.push_back(on_ntsetinformationobject);
}

void monitor::GenericMonitor::on_NtSetInformationObject()
{
    LOG(INFO, "Break on NtSetInformationObject");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ObjectInformationClass= nt::cast_to<nt::OBJECT_INFORMATION_CLASS>(args[1]);
    const auto ObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationObject)
    {
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationProcess(const on_NtSetInformationProcess_fn& on_ntsetinformationprocess)
{
    d_->observers_NtSetInformationProcess.push_back(on_ntsetinformationprocess);
}

void monitor::GenericMonitor::on_NtSetInformationProcess()
{
    LOG(INFO, "Break on NtSetInformationProcess");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessInformationClass= nt::cast_to<nt::PROCESSINFOCLASS>   (args[1]);
    const auto ProcessInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ProcessInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationProcess)
    {
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationResourceManager(const on_NtSetInformationResourceManager_fn& on_ntsetinformationresourcemanager)
{
    d_->observers_NtSetInformationResourceManager.push_back(on_ntsetinformationresourcemanager);
}

void monitor::GenericMonitor::on_NtSetInformationResourceManager()
{
    LOG(INFO, "Break on NtSetInformationResourceManager");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResourceManagerInformationClass= nt::cast_to<nt::RESOURCEMANAGER_INFORMATION_CLASS>(args[1]);
    const auto ResourceManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ResourceManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationResourceManager)
    {
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationThread(const on_NtSetInformationThread_fn& on_ntsetinformationthread)
{
    d_->observers_NtSetInformationThread.push_back(on_ntsetinformationthread);
}

void monitor::GenericMonitor::on_NtSetInformationThread()
{
    LOG(INFO, "Break on NtSetInformationThread");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadInformationClass= nt::cast_to<nt::THREADINFOCLASS>    (args[1]);
    const auto ThreadInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ThreadInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationThread)
    {
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationToken(const on_NtSetInformationToken_fn& on_ntsetinformationtoken)
{
    d_->observers_NtSetInformationToken.push_back(on_ntsetinformationtoken);
}

void monitor::GenericMonitor::on_NtSetInformationToken()
{
    LOG(INFO, "Break on NtSetInformationToken");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TokenInformationClass= nt::cast_to<nt::TOKEN_INFORMATION_CLASS>(args[1]);
    const auto TokenInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TokenInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationToken)
    {
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationTransaction(const on_NtSetInformationTransaction_fn& on_ntsetinformationtransaction)
{
    d_->observers_NtSetInformationTransaction.push_back(on_ntsetinformationtransaction);
}

void monitor::GenericMonitor::on_NtSetInformationTransaction()
{
    LOG(INFO, "Break on NtSetInformationTransaction");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionInformationClass= nt::cast_to<nt::TRANSACTION_INFORMATION_CLASS>(args[1]);
    const auto TransactionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationTransaction)
    {
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationTransactionManager(const on_NtSetInformationTransactionManager_fn& on_ntsetinformationtransactionmanager)
{
    d_->observers_NtSetInformationTransactionManager.push_back(on_ntsetinformationtransactionmanager);
}

void monitor::GenericMonitor::on_NtSetInformationTransactionManager()
{
    LOG(INFO, "Break on NtSetInformationTransactionManager");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionManagerInformationClass= nt::cast_to<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(args[1]);
    const auto TransactionManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationTransactionManager)
    {
        it(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetInformationWorkerFactory(const on_NtSetInformationWorkerFactory_fn& on_ntsetinformationworkerfactory)
{
    d_->observers_NtSetInformationWorkerFactory.push_back(on_ntsetinformationworkerfactory);
}

void monitor::GenericMonitor::on_NtSetInformationWorkerFactory()
{
    LOG(INFO, "Break on NtSetInformationWorkerFactory");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WorkerFactoryInformationClass= nt::cast_to<nt::WORKERFACTORYINFOCLASS>(args[1]);
    const auto WorkerFactoryInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto WorkerFactoryInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationWorkerFactory)
    {
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetIntervalProfile(const on_NtSetIntervalProfile_fn& on_ntsetintervalprofile)
{
    d_->observers_NtSetIntervalProfile.push_back(on_ntsetintervalprofile);
}

void monitor::GenericMonitor::on_NtSetIntervalProfile()
{
    LOG(INFO, "Break on NtSetIntervalProfile");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Interval        = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Source          = nt::cast_to<nt::KPROFILE_SOURCE>    (args[1]);

    for(const auto& it : d_->observers_NtSetIntervalProfile)
    {
        it(Interval, Source);
    }
}

void monitor::GenericMonitor::register_NtSetIoCompletionEx(const on_NtSetIoCompletionEx_fn& on_ntsetiocompletionex)
{
    d_->observers_NtSetIoCompletionEx.push_back(on_ntsetiocompletionex);
}

void monitor::GenericMonitor::on_NtSetIoCompletionEx()
{
    LOG(INFO, "Break on NtSetIoCompletionEx");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionReserveHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto KeyContext      = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatus        = nt::cast_to<nt::NTSTATUS>           (args[4]);
    const auto IoStatusInformation= nt::cast_to<nt::ULONG_PTR>          (args[5]);

    for(const auto& it : d_->observers_NtSetIoCompletionEx)
    {
        it(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }
}

void monitor::GenericMonitor::register_NtSetIoCompletion(const on_NtSetIoCompletion_fn& on_ntsetiocompletion)
{
    d_->observers_NtSetIoCompletion.push_back(on_ntsetiocompletion);
}

void monitor::GenericMonitor::on_NtSetIoCompletion()
{
    LOG(INFO, "Break on NtSetIoCompletion");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyContext      = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoStatus        = nt::cast_to<nt::NTSTATUS>           (args[3]);
    const auto IoStatusInformation= nt::cast_to<nt::ULONG_PTR>          (args[4]);

    for(const auto& it : d_->observers_NtSetIoCompletion)
    {
        it(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
    }
}

void monitor::GenericMonitor::register_NtSetLdtEntries(const on_NtSetLdtEntries_fn& on_ntsetldtentries)
{
    d_->observers_NtSetLdtEntries.push_back(on_ntsetldtentries);
}

void monitor::GenericMonitor::on_NtSetLdtEntries()
{
    LOG(INFO, "Break on NtSetLdtEntries");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Selector0       = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Entry0Low       = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Entry0Hi        = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Selector1       = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Entry1Low       = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Entry1Hi        = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtSetLdtEntries)
    {
        it(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
    }
}

void monitor::GenericMonitor::register_NtSetLowEventPair(const on_NtSetLowEventPair_fn& on_ntsetloweventpair)
{
    d_->observers_NtSetLowEventPair.push_back(on_ntsetloweventpair);
}

void monitor::GenericMonitor::on_NtSetLowEventPair()
{
    LOG(INFO, "Break on NtSetLowEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetLowEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtSetLowWaitHighEventPair(const on_NtSetLowWaitHighEventPair_fn& on_ntsetlowwaithigheventpair)
{
    d_->observers_NtSetLowWaitHighEventPair.push_back(on_ntsetlowwaithigheventpair);
}

void monitor::GenericMonitor::on_NtSetLowWaitHighEventPair()
{
    LOG(INFO, "Break on NtSetLowWaitHighEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetLowWaitHighEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtSetQuotaInformationFile(const on_NtSetQuotaInformationFile_fn& on_ntsetquotainformationfile)
{
    d_->observers_NtSetQuotaInformationFile.push_back(on_ntsetquotainformationfile);
}

void monitor::GenericMonitor::on_NtSetQuotaInformationFile()
{
    LOG(INFO, "Break on NtSetQuotaInformationFile");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetQuotaInformationFile)
    {
        it(FileHandle, IoStatusBlock, Buffer, Length);
    }
}

void monitor::GenericMonitor::register_NtSetSecurityObject(const on_NtSetSecurityObject_fn& on_ntsetsecurityobject)
{
    d_->observers_NtSetSecurityObject.push_back(on_ntsetsecurityobject);
}

void monitor::GenericMonitor::on_NtSetSecurityObject()
{
    LOG(INFO, "Break on NtSetSecurityObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecurityInformation= nt::cast_to<nt::SECURITY_INFORMATION>(args[1]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[2]);

    for(const auto& it : d_->observers_NtSetSecurityObject)
    {
        it(Handle, SecurityInformation, SecurityDescriptor);
    }
}

void monitor::GenericMonitor::register_NtSetSystemEnvironmentValueEx(const on_NtSetSystemEnvironmentValueEx_fn& on_ntsetsystemenvironmentvalueex)
{
    d_->observers_NtSetSystemEnvironmentValueEx.push_back(on_ntsetsystemenvironmentvalueex);
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValueEx()
{
    LOG(INFO, "Break on NtSetSystemEnvironmentValueEx");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VendorGuid      = nt::cast_to<nt::LPGUID>             (args[1]);
    const auto Value           = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ValueLength     = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Attributes      = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValueEx)
    {
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
    }
}

void monitor::GenericMonitor::register_NtSetSystemEnvironmentValue(const on_NtSetSystemEnvironmentValue_fn& on_ntsetsystemenvironmentvalue)
{
    d_->observers_NtSetSystemEnvironmentValue.push_back(on_ntsetsystemenvironmentvalue);
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValue()
{
    LOG(INFO, "Break on NtSetSystemEnvironmentValue");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VariableValue   = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValue)
    {
        it(VariableName, VariableValue);
    }
}

void monitor::GenericMonitor::register_NtSetSystemInformation(const on_NtSetSystemInformation_fn& on_ntsetsysteminformation)
{
    d_->observers_NtSetSystemInformation.push_back(on_ntsetsysteminformation);
}

void monitor::GenericMonitor::on_NtSetSystemInformation()
{
    LOG(INFO, "Break on NtSetSystemInformation");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSetSystemInformation)
    {
        it(SystemInformationClass, SystemInformation, SystemInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetSystemPowerState(const on_NtSetSystemPowerState_fn& on_ntsetsystempowerstate)
{
    d_->observers_NtSetSystemPowerState.push_back(on_ntsetsystempowerstate);
}

void monitor::GenericMonitor::on_NtSetSystemPowerState()
{
    LOG(INFO, "Break on NtSetSystemPowerState");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemAction    = nt::cast_to<nt::POWER_ACTION>       (args[0]);
    const auto MinSystemState  = nt::cast_to<nt::SYSTEM_POWER_STATE> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSetSystemPowerState)
    {
        it(SystemAction, MinSystemState, Flags);
    }
}

void monitor::GenericMonitor::register_NtSetSystemTime(const on_NtSetSystemTime_fn& on_ntsetsystemtime)
{
    d_->observers_NtSetSystemTime.push_back(on_ntsetsystemtime);
}

void monitor::GenericMonitor::on_NtSetSystemTime()
{
    LOG(INFO, "Break on NtSetSystemTime");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemTime      = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto PreviousTime    = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtSetSystemTime)
    {
        it(SystemTime, PreviousTime);
    }
}

void monitor::GenericMonitor::register_NtSetThreadExecutionState(const on_NtSetThreadExecutionState_fn& on_ntsetthreadexecutionstate)
{
    d_->observers_NtSetThreadExecutionState.push_back(on_ntsetthreadexecutionstate);
}

void monitor::GenericMonitor::on_NtSetThreadExecutionState()
{
    LOG(INFO, "Break on NtSetThreadExecutionState");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto esFlags         = nt::cast_to<nt::EXECUTION_STATE>    (args[0]);
    const auto STARPreviousFlags= nt::cast_to<nt::EXECUTION_STATE>    (args[1]);

    for(const auto& it : d_->observers_NtSetThreadExecutionState)
    {
        it(esFlags, STARPreviousFlags);
    }
}

void monitor::GenericMonitor::register_NtSetTimerEx(const on_NtSetTimerEx_fn& on_ntsettimerex)
{
    d_->observers_NtSetTimerEx.push_back(on_ntsettimerex);
}

void monitor::GenericMonitor::on_NtSetTimerEx()
{
    LOG(INFO, "Break on NtSetTimerEx");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TimerSetInformationClass= nt::cast_to<nt::TIMER_SET_INFORMATION_CLASS>(args[1]);
    const auto TimerSetInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TimerSetInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetTimerEx)
    {
        it(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
    }
}

void monitor::GenericMonitor::register_NtSetTimer(const on_NtSetTimer_fn& on_ntsettimer)
{
    d_->observers_NtSetTimer.push_back(on_ntsettimer);
}

void monitor::GenericMonitor::on_NtSetTimer()
{
    LOG(INFO, "Break on NtSetTimer");
    const auto nargs = 7;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DueTime         = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);
    const auto TimerApcRoutine = nt::cast_to<nt::PTIMER_APC_ROUTINE> (args[2]);
    const auto TimerContext    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto WakeTimer       = nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto Period          = nt::cast_to<nt::LONG>               (args[5]);
    const auto PreviousState   = nt::cast_to<nt::PBOOLEAN>           (args[6]);

    for(const auto& it : d_->observers_NtSetTimer)
    {
        it(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
    }
}

void monitor::GenericMonitor::register_NtSetTimerResolution(const on_NtSetTimerResolution_fn& on_ntsettimerresolution)
{
    d_->observers_NtSetTimerResolution.push_back(on_ntsettimerresolution);
}

void monitor::GenericMonitor::on_NtSetTimerResolution()
{
    LOG(INFO, "Break on NtSetTimerResolution");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DesiredTime     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto SetResolution   = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto ActualTime      = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtSetTimerResolution)
    {
        it(DesiredTime, SetResolution, ActualTime);
    }
}

void monitor::GenericMonitor::register_NtSetUuidSeed(const on_NtSetUuidSeed_fn& on_ntsetuuidseed)
{
    d_->observers_NtSetUuidSeed.push_back(on_ntsetuuidseed);
}

void monitor::GenericMonitor::on_NtSetUuidSeed()
{
    LOG(INFO, "Break on NtSetUuidSeed");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Seed            = nt::cast_to<nt::PCHAR>              (args[0]);

    for(const auto& it : d_->observers_NtSetUuidSeed)
    {
        it(Seed);
    }
}

void monitor::GenericMonitor::register_NtSetValueKey(const on_NtSetValueKey_fn& on_ntsetvaluekey)
{
    d_->observers_NtSetValueKey.push_back(on_ntsetvaluekey);
}

void monitor::GenericMonitor::on_NtSetValueKey()
{
    LOG(INFO, "Break on NtSetValueKey");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto TitleIndex      = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Type            = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Data            = nt::cast_to<nt::PVOID>              (args[4]);
    const auto DataSize        = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtSetValueKey)
    {
        it(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
    }
}

void monitor::GenericMonitor::register_NtSetVolumeInformationFile(const on_NtSetVolumeInformationFile_fn& on_ntsetvolumeinformationfile)
{
    d_->observers_NtSetVolumeInformationFile.push_back(on_ntsetvolumeinformationfile);
}

void monitor::GenericMonitor::on_NtSetVolumeInformationFile()
{
    LOG(INFO, "Break on NtSetVolumeInformationFile");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FsInformation   = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FsInformationClass= nt::cast_to<nt::FS_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtSetVolumeInformationFile)
    {
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
    }
}

void monitor::GenericMonitor::register_NtShutdownSystem(const on_NtShutdownSystem_fn& on_ntshutdownsystem)
{
    d_->observers_NtShutdownSystem.push_back(on_ntshutdownsystem);
}

void monitor::GenericMonitor::on_NtShutdownSystem()
{
    LOG(INFO, "Break on NtShutdownSystem");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Action          = nt::cast_to<nt::SHUTDOWN_ACTION>    (args[0]);

    for(const auto& it : d_->observers_NtShutdownSystem)
    {
        it(Action);
    }
}

void monitor::GenericMonitor::register_NtShutdownWorkerFactory(const on_NtShutdownWorkerFactory_fn& on_ntshutdownworkerfactory)
{
    d_->observers_NtShutdownWorkerFactory.push_back(on_ntshutdownworkerfactory);
}

void monitor::GenericMonitor::on_NtShutdownWorkerFactory()
{
    LOG(INFO, "Break on NtShutdownWorkerFactory");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPendingWorkerCount= nt::cast_to<nt::LONG>               (args[1]);

    for(const auto& it : d_->observers_NtShutdownWorkerFactory)
    {
        it(WorkerFactoryHandle, STARPendingWorkerCount);
    }
}

void monitor::GenericMonitor::register_NtSignalAndWaitForSingleObject(const on_NtSignalAndWaitForSingleObject_fn& on_ntsignalandwaitforsingleobject)
{
    d_->observers_NtSignalAndWaitForSingleObject.push_back(on_ntsignalandwaitforsingleobject);
}

void monitor::GenericMonitor::on_NtSignalAndWaitForSingleObject()
{
    LOG(INFO, "Break on NtSignalAndWaitForSingleObject");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SignalHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WaitHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtSignalAndWaitForSingleObject)
    {
        it(SignalHandle, WaitHandle, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtSinglePhaseReject(const on_NtSinglePhaseReject_fn& on_ntsinglephasereject)
{
    d_->observers_NtSinglePhaseReject.push_back(on_ntsinglephasereject);
}

void monitor::GenericMonitor::on_NtSinglePhaseReject()
{
    LOG(INFO, "Break on NtSinglePhaseReject");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtSinglePhaseReject)
    {
        it(EnlistmentHandle, TmVirtualClock);
    }
}

void monitor::GenericMonitor::register_NtStartProfile(const on_NtStartProfile_fn& on_ntstartprofile)
{
    d_->observers_NtStartProfile.push_back(on_ntstartprofile);
}

void monitor::GenericMonitor::on_NtStartProfile()
{
    LOG(INFO, "Break on NtStartProfile");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtStartProfile)
    {
        it(ProfileHandle);
    }
}

void monitor::GenericMonitor::register_NtStopProfile(const on_NtStopProfile_fn& on_ntstopprofile)
{
    d_->observers_NtStopProfile.push_back(on_ntstopprofile);
}

void monitor::GenericMonitor::on_NtStopProfile()
{
    LOG(INFO, "Break on NtStopProfile");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtStopProfile)
    {
        it(ProfileHandle);
    }
}

void monitor::GenericMonitor::register_NtSuspendProcess(const on_NtSuspendProcess_fn& on_ntsuspendprocess)
{
    d_->observers_NtSuspendProcess.push_back(on_ntsuspendprocess);
}

void monitor::GenericMonitor::on_NtSuspendProcess()
{
    LOG(INFO, "Break on NtSuspendProcess");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSuspendProcess)
    {
        it(ProcessHandle);
    }
}

void monitor::GenericMonitor::register_NtSuspendThread(const on_NtSuspendThread_fn& on_ntsuspendthread)
{
    d_->observers_NtSuspendThread.push_back(on_ntsuspendthread);
}

void monitor::GenericMonitor::on_NtSuspendThread()
{
    LOG(INFO, "Break on NtSuspendThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtSuspendThread)
    {
        it(ThreadHandle, PreviousSuspendCount);
    }
}

void monitor::GenericMonitor::register_NtSystemDebugControl(const on_NtSystemDebugControl_fn& on_ntsystemdebugcontrol)
{
    d_->observers_NtSystemDebugControl.push_back(on_ntsystemdebugcontrol);
}

void monitor::GenericMonitor::on_NtSystemDebugControl()
{
    LOG(INFO, "Break on NtSystemDebugControl");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Command         = nt::cast_to<nt::SYSDBG_COMMAND>     (args[0]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtSystemDebugControl)
    {
        it(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtTerminateJobObject(const on_NtTerminateJobObject_fn& on_ntterminatejobobject)
{
    d_->observers_NtTerminateJobObject.push_back(on_ntterminatejobobject);
}

void monitor::GenericMonitor::on_NtTerminateJobObject()
{
    LOG(INFO, "Break on NtTerminateJobObject");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateJobObject)
    {
        it(JobHandle, ExitStatus);
    }
}

void monitor::GenericMonitor::register_NtTerminateProcess(const on_NtTerminateProcess_fn& on_ntterminateprocess)
{
    d_->observers_NtTerminateProcess.push_back(on_ntterminateprocess);
}

void monitor::GenericMonitor::on_NtTerminateProcess()
{
    LOG(INFO, "Break on NtTerminateProcess");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateProcess)
    {
        it(ProcessHandle, ExitStatus);
    }
}

void monitor::GenericMonitor::register_NtTerminateThread(const on_NtTerminateThread_fn& on_ntterminatethread)
{
    d_->observers_NtTerminateThread.push_back(on_ntterminatethread);
}

void monitor::GenericMonitor::on_NtTerminateThread()
{
    LOG(INFO, "Break on NtTerminateThread");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateThread)
    {
        it(ThreadHandle, ExitStatus);
    }
}

void monitor::GenericMonitor::register_NtTraceControl(const on_NtTraceControl_fn& on_nttracecontrol)
{
    d_->observers_NtTraceControl.push_back(on_nttracecontrol);
}

void monitor::GenericMonitor::on_NtTraceControl()
{
    LOG(INFO, "Break on NtTraceControl");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FunctionCode    = nt::cast_to<nt::ULONG>              (args[0]);
    const auto InBuffer        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InBufferLen     = nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutBuffer       = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutBufferLen    = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtTraceControl)
    {
        it(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
    }
}

void monitor::GenericMonitor::register_NtTraceEvent(const on_NtTraceEvent_fn& on_nttraceevent)
{
    d_->observers_NtTraceEvent.push_back(on_nttraceevent);
}

void monitor::GenericMonitor::on_NtTraceEvent()
{
    LOG(INFO, "Break on NtTraceEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TraceHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto FieldSize       = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Fields          = nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtTraceEvent)
    {
        it(TraceHandle, Flags, FieldSize, Fields);
    }
}

void monitor::GenericMonitor::register_NtTranslateFilePath(const on_NtTranslateFilePath_fn& on_nttranslatefilepath)
{
    d_->observers_NtTranslateFilePath.push_back(on_nttranslatefilepath);
}

void monitor::GenericMonitor::on_NtTranslateFilePath()
{
    LOG(INFO, "Break on NtTranslateFilePath");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InputFilePath   = nt::cast_to<nt::PFILE_PATH>         (args[0]);
    const auto OutputType      = nt::cast_to<nt::ULONG>              (args[1]);
    const auto OutputFilePath  = nt::cast_to<nt::PFILE_PATH>         (args[2]);
    const auto OutputFilePathLength= nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtTranslateFilePath)
    {
        it(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
    }
}

void monitor::GenericMonitor::register_NtUnloadDriver(const on_NtUnloadDriver_fn& on_ntunloaddriver)
{
    d_->observers_NtUnloadDriver.push_back(on_ntunloaddriver);
}

void monitor::GenericMonitor::on_NtUnloadDriver()
{
    LOG(INFO, "Break on NtUnloadDriver");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverServiceName= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtUnloadDriver)
    {
        it(DriverServiceName);
    }
}

void monitor::GenericMonitor::register_NtUnloadKey2(const on_NtUnloadKey2_fn& on_ntunloadkey2)
{
    d_->observers_NtUnloadKey2.push_back(on_ntunloadkey2);
}

void monitor::GenericMonitor::on_NtUnloadKey2()
{
    LOG(INFO, "Break on NtUnloadKey2");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtUnloadKey2)
    {
        it(TargetKey, Flags);
    }
}

void monitor::GenericMonitor::register_NtUnloadKeyEx(const on_NtUnloadKeyEx_fn& on_ntunloadkeyex)
{
    d_->observers_NtUnloadKeyEx.push_back(on_ntunloadkeyex);
}

void monitor::GenericMonitor::on_NtUnloadKeyEx()
{
    LOG(INFO, "Break on NtUnloadKeyEx");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtUnloadKeyEx)
    {
        it(TargetKey, Event);
    }
}

void monitor::GenericMonitor::register_NtUnloadKey(const on_NtUnloadKey_fn& on_ntunloadkey)
{
    d_->observers_NtUnloadKey.push_back(on_ntunloadkey);
}

void monitor::GenericMonitor::on_NtUnloadKey()
{
    LOG(INFO, "Break on NtUnloadKey");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);

    for(const auto& it : d_->observers_NtUnloadKey)
    {
        it(TargetKey);
    }
}

void monitor::GenericMonitor::register_NtUnlockFile(const on_NtUnlockFile_fn& on_ntunlockfile)
{
    d_->observers_NtUnlockFile.push_back(on_ntunlockfile);
}

void monitor::GenericMonitor::on_NtUnlockFile()
{
    LOG(INFO, "Break on NtUnlockFile");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto Length          = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto Key             = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtUnlockFile)
    {
        it(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
    }
}

void monitor::GenericMonitor::register_NtUnlockVirtualMemory(const on_NtUnlockVirtualMemory_fn& on_ntunlockvirtualmemory)
{
    d_->observers_NtUnlockVirtualMemory.push_back(on_ntunlockvirtualmemory);
}

void monitor::GenericMonitor::on_NtUnlockVirtualMemory()
{
    LOG(INFO, "Break on NtUnlockVirtualMemory");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto MapType         = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtUnlockVirtualMemory)
    {
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
    }
}

void monitor::GenericMonitor::register_NtUnmapViewOfSection(const on_NtUnmapViewOfSection_fn& on_ntunmapviewofsection)
{
    d_->observers_NtUnmapViewOfSection.push_back(on_ntunmapviewofsection);
}

void monitor::GenericMonitor::on_NtUnmapViewOfSection()
{
    LOG(INFO, "Break on NtUnmapViewOfSection");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtUnmapViewOfSection)
    {
        it(ProcessHandle, BaseAddress);
    }
}

void monitor::GenericMonitor::register_NtVdmControl(const on_NtVdmControl_fn& on_ntvdmcontrol)
{
    d_->observers_NtVdmControl.push_back(on_ntvdmcontrol);
}

void monitor::GenericMonitor::on_NtVdmControl()
{
    LOG(INFO, "Break on NtVdmControl");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Service         = nt::cast_to<nt::VDMSERVICECLASS>    (args[0]);
    const auto ServiceData     = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtVdmControl)
    {
        it(Service, ServiceData);
    }
}

void monitor::GenericMonitor::register_NtWaitForDebugEvent(const on_NtWaitForDebugEvent_fn& on_ntwaitfordebugevent)
{
    d_->observers_NtWaitForDebugEvent.push_back(on_ntwaitfordebugevent);
}

void monitor::GenericMonitor::on_NtWaitForDebugEvent()
{
    LOG(INFO, "Break on NtWaitForDebugEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto WaitStateChange = nt::cast_to<nt::PDBGUI_WAIT_STATE_CHANGE>(args[3]);

    for(const auto& it : d_->observers_NtWaitForDebugEvent)
    {
        it(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
    }
}

void monitor::GenericMonitor::register_NtWaitForKeyedEvent(const on_NtWaitForKeyedEvent_fn& on_ntwaitforkeyedevent)
{
    d_->observers_NtWaitForKeyedEvent.push_back(on_ntwaitforkeyedevent);
}

void monitor::GenericMonitor::on_NtWaitForKeyedEvent()
{
    LOG(INFO, "Break on NtWaitForKeyedEvent");
    const auto nargs = 4;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyValue        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtWaitForKeyedEvent)
    {
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtWaitForMultipleObjects32(const on_NtWaitForMultipleObjects32_fn& on_ntwaitformultipleobjects32)
{
    d_->observers_NtWaitForMultipleObjects32.push_back(on_ntwaitformultipleobjects32);
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects32()
{
    LOG(INFO, "Break on NtWaitForMultipleObjects32");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Handles         = nt::cast_to<nt::LONG>               (args[1]);
    const auto WaitType        = nt::cast_to<nt::WAIT_TYPE>          (args[2]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects32)
    {
        it(Count, Handles, WaitType, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtWaitForMultipleObjects(const on_NtWaitForMultipleObjects_fn& on_ntwaitformultipleobjects)
{
    d_->observers_NtWaitForMultipleObjects.push_back(on_ntwaitformultipleobjects);
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects()
{
    LOG(INFO, "Break on NtWaitForMultipleObjects");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Handles         = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto WaitType        = nt::cast_to<nt::WAIT_TYPE>          (args[2]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects)
    {
        it(Count, Handles, WaitType, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtWaitForSingleObject(const on_NtWaitForSingleObject_fn& on_ntwaitforsingleobject)
{
    d_->observers_NtWaitForSingleObject.push_back(on_ntwaitforsingleobject);
}

void monitor::GenericMonitor::on_NtWaitForSingleObject()
{
    LOG(INFO, "Break on NtWaitForSingleObject");
    const auto nargs = 3;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);

    for(const auto& it : d_->observers_NtWaitForSingleObject)
    {
        it(Handle, Alertable, Timeout);
    }
}

void monitor::GenericMonitor::register_NtWaitForWorkViaWorkerFactory(const on_NtWaitForWorkViaWorkerFactory_fn& on_ntwaitforworkviaworkerfactory)
{
    d_->observers_NtWaitForWorkViaWorkerFactory.push_back(on_ntwaitforworkviaworkerfactory);
}

void monitor::GenericMonitor::on_NtWaitForWorkViaWorkerFactory()
{
    LOG(INFO, "Break on NtWaitForWorkViaWorkerFactory");
    const auto nargs = 2;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto MiniPacket      = nt::cast_to<nt::PFILE_IO_COMPLETION_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtWaitForWorkViaWorkerFactory)
    {
        it(WorkerFactoryHandle, MiniPacket);
    }
}

void monitor::GenericMonitor::register_NtWaitHighEventPair(const on_NtWaitHighEventPair_fn& on_ntwaithigheventpair)
{
    d_->observers_NtWaitHighEventPair.push_back(on_ntwaithigheventpair);
}

void monitor::GenericMonitor::on_NtWaitHighEventPair()
{
    LOG(INFO, "Break on NtWaitHighEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWaitHighEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtWaitLowEventPair(const on_NtWaitLowEventPair_fn& on_ntwaitloweventpair)
{
    d_->observers_NtWaitLowEventPair.push_back(on_ntwaitloweventpair);
}

void monitor::GenericMonitor::on_NtWaitLowEventPair()
{
    LOG(INFO, "Break on NtWaitLowEventPair");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWaitLowEventPair)
    {
        it(EventPairHandle);
    }
}

void monitor::GenericMonitor::register_NtWorkerFactoryWorkerReady(const on_NtWorkerFactoryWorkerReady_fn& on_ntworkerfactoryworkerready)
{
    d_->observers_NtWorkerFactoryWorkerReady.push_back(on_ntworkerfactoryworkerready);
}

void monitor::GenericMonitor::on_NtWorkerFactoryWorkerReady()
{
    LOG(INFO, "Break on NtWorkerFactoryWorkerReady");
    const auto nargs = 1;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWorkerFactoryWorkerReady)
    {
        it(WorkerFactoryHandle);
    }
}

void monitor::GenericMonitor::register_NtWriteFileGather(const on_NtWriteFileGather_fn& on_ntwritefilegather)
{
    d_->observers_NtWriteFileGather.push_back(on_ntwritefilegather);
}

void monitor::GenericMonitor::on_NtWriteFileGather()
{
    LOG(INFO, "Break on NtWriteFileGather");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto SegmentArray    = nt::cast_to<nt::PFILE_SEGMENT_ELEMENT>(args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);
    const auto Key             = nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto& it : d_->observers_NtWriteFileGather)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
    }
}

void monitor::GenericMonitor::register_NtWriteFile(const on_NtWriteFile_fn& on_ntwritefile)
{
    d_->observers_NtWriteFile.push_back(on_ntwritefile);
}

void monitor::GenericMonitor::on_NtWriteFile()
{
    LOG(INFO, "Break on NtWriteFile");
    const auto nargs = 9;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PIO_APC_ROUTINE>    (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[4]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[5]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[6]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[7]);
    const auto Key             = nt::cast_to<nt::PULONG>             (args[8]);

    for(const auto& it : d_->observers_NtWriteFile)
    {
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
    }
}

void monitor::GenericMonitor::register_NtWriteRequestData(const on_NtWriteRequestData_fn& on_ntwriterequestdata)
{
    d_->observers_NtWriteRequestData.push_back(on_ntwriterequestdata);
}

void monitor::GenericMonitor::on_NtWriteRequestData()
{
    LOG(INFO, "Break on NtWriteRequestData");
    const auto nargs = 6;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto DataEntryIndex  = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto NumberOfBytesWritten= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtWriteRequestData)
    {
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
    }
}

void monitor::GenericMonitor::register_NtWriteVirtualMemory(const on_NtWriteVirtualMemory_fn& on_ntwritevirtualmemory)
{
    d_->observers_NtWriteVirtualMemory.push_back(on_ntwritevirtualmemory);
}

void monitor::GenericMonitor::on_NtWriteVirtualMemory()
{
    LOG(INFO, "Break on NtWriteVirtualMemory");
    const auto nargs = 5;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto NumberOfBytesWritten= nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtWriteVirtualMemory)
    {
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
    }
}

void monitor::GenericMonitor::register_NtDisableLastKnownGood(const on_NtDisableLastKnownGood_fn& on_ntdisablelastknowngood)
{
    d_->observers_NtDisableLastKnownGood.push_back(on_ntdisablelastknowngood);
}

void monitor::GenericMonitor::on_NtDisableLastKnownGood()
{
    LOG(INFO, "Break on NtDisableLastKnownGood");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtDisableLastKnownGood)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtEnableLastKnownGood(const on_NtEnableLastKnownGood_fn& on_ntenablelastknowngood)
{
    d_->observers_NtEnableLastKnownGood.push_back(on_ntenablelastknowngood);
}

void monitor::GenericMonitor::on_NtEnableLastKnownGood()
{
    LOG(INFO, "Break on NtEnableLastKnownGood");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtEnableLastKnownGood)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtFlushProcessWriteBuffers(const on_NtFlushProcessWriteBuffers_fn& on_ntflushprocesswritebuffers)
{
    d_->observers_NtFlushProcessWriteBuffers.push_back(on_ntflushprocesswritebuffers);
}

void monitor::GenericMonitor::on_NtFlushProcessWriteBuffers()
{
    LOG(INFO, "Break on NtFlushProcessWriteBuffers");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtFlushProcessWriteBuffers)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtFlushWriteBuffer(const on_NtFlushWriteBuffer_fn& on_ntflushwritebuffer)
{
    d_->observers_NtFlushWriteBuffer.push_back(on_ntflushwritebuffer);
}

void monitor::GenericMonitor::on_NtFlushWriteBuffer()
{
    LOG(INFO, "Break on NtFlushWriteBuffer");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtFlushWriteBuffer)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtGetCurrentProcessorNumber(const on_NtGetCurrentProcessorNumber_fn& on_ntgetcurrentprocessornumber)
{
    d_->observers_NtGetCurrentProcessorNumber.push_back(on_ntgetcurrentprocessornumber);
}

void monitor::GenericMonitor::on_NtGetCurrentProcessorNumber()
{
    LOG(INFO, "Break on NtGetCurrentProcessorNumber");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtGetCurrentProcessorNumber)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtIsSystemResumeAutomatic(const on_NtIsSystemResumeAutomatic_fn& on_ntissystemresumeautomatic)
{
    d_->observers_NtIsSystemResumeAutomatic.push_back(on_ntissystemresumeautomatic);
}

void monitor::GenericMonitor::on_NtIsSystemResumeAutomatic()
{
    LOG(INFO, "Break on NtIsSystemResumeAutomatic");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtIsSystemResumeAutomatic)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtIsUILanguageComitted(const on_NtIsUILanguageComitted_fn& on_ntisuilanguagecomitted)
{
    d_->observers_NtIsUILanguageComitted.push_back(on_ntisuilanguagecomitted);
}

void monitor::GenericMonitor::on_NtIsUILanguageComitted()
{
    LOG(INFO, "Break on NtIsUILanguageComitted");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtIsUILanguageComitted)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtQueryPortInformationProcess(const on_NtQueryPortInformationProcess_fn& on_ntqueryportinformationprocess)
{
    d_->observers_NtQueryPortInformationProcess.push_back(on_ntqueryportinformationprocess);
}

void monitor::GenericMonitor::on_NtQueryPortInformationProcess()
{
    LOG(INFO, "Break on NtQueryPortInformationProcess");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtQueryPortInformationProcess)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtSerializeBoot(const on_NtSerializeBoot_fn& on_ntserializeboot)
{
    d_->observers_NtSerializeBoot.push_back(on_ntserializeboot);
}

void monitor::GenericMonitor::on_NtSerializeBoot()
{
    LOG(INFO, "Break on NtSerializeBoot");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtSerializeBoot)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtTestAlert(const on_NtTestAlert_fn& on_nttestalert)
{
    d_->observers_NtTestAlert.push_back(on_nttestalert);
}

void monitor::GenericMonitor::on_NtTestAlert()
{
    LOG(INFO, "Break on NtTestAlert");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtTestAlert)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtThawRegistry(const on_NtThawRegistry_fn& on_ntthawregistry)
{
    d_->observers_NtThawRegistry.push_back(on_ntthawregistry);
}

void monitor::GenericMonitor::on_NtThawRegistry()
{
    LOG(INFO, "Break on NtThawRegistry");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtThawRegistry)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtThawTransactions(const on_NtThawTransactions_fn& on_ntthawtransactions)
{
    d_->observers_NtThawTransactions.push_back(on_ntthawtransactions);
}

void monitor::GenericMonitor::on_NtThawTransactions()
{
    LOG(INFO, "Break on NtThawTransactions");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtThawTransactions)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtUmsThreadYield(const on_NtUmsThreadYield_fn& on_ntumsthreadyield)
{
    d_->observers_NtUmsThreadYield.push_back(on_ntumsthreadyield);
}

void monitor::GenericMonitor::on_NtUmsThreadYield()
{
    LOG(INFO, "Break on NtUmsThreadYield");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtUmsThreadYield)
    {
        it();
    }
}

void monitor::GenericMonitor::register_NtYieldExecution(const on_NtYieldExecution_fn& on_ntyieldexecution)
{
    d_->observers_NtYieldExecution.push_back(on_ntyieldexecution);
}

void monitor::GenericMonitor::on_NtYieldExecution()
{
    LOG(INFO, "Break on NtYieldExecution");
    const auto nargs = 0;

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtYieldExecution)
    {
        it();
    }
}
