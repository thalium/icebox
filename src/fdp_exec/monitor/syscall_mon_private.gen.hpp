#pragma once

#include "generic_mon.hpp"


bool monitor::GenericMonitor::register_NtAcceptConnectPort(proc_t proc, const on_NtAcceptConnectPort_fn& on_ntacceptconnectport)
{
    const auto ok = setup_func(proc, "NtAcceptConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAcceptConnectPort.push_back(on_ntacceptconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtAcceptConnectPort()
{
    //LOG(INFO, "Break on NtAcceptConnectPort");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortContext     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ConnectionRequest= nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto AcceptConnection= nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto ServerView      = nt::cast_to<nt::PPORT_VIEW>         (args[4]);
    const auto ClientView      = nt::cast_to<nt::PREMOTE_PORT_VIEW>  (args[5]);

    for(const auto& it : d_->observers_NtAcceptConnectPort)
        it(PortHandle, PortContext, ConnectionRequest, AcceptConnection, ServerView, ClientView);
}

bool monitor::GenericMonitor::register_NtAccessCheckAndAuditAlarm(proc_t proc, const on_NtAccessCheckAndAuditAlarm_fn& on_ntaccesscheckandauditalarm)
{
    const auto ok = setup_func(proc, "NtAccessCheckAndAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckAndAuditAlarm.push_back(on_ntaccesscheckandauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckAndAuditAlarm()
{
    //LOG(INFO, "Break on NtAccessCheckAndAuditAlarm");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeAndAuditAlarm_fn& on_ntaccesscheckbytypeandauditalarm)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeAndAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckByTypeAndAuditAlarm.push_back(on_ntaccesscheckbytypeandauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeAndAuditAlarm()
{
    //LOG(INFO, "Break on NtAccessCheckByTypeAndAuditAlarm");
    constexpr int nargs = 16;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByType(proc_t proc, const on_NtAccessCheckByType_fn& on_ntaccesscheckbytype)
{
    const auto ok = setup_func(proc, "NtAccessCheckByType");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckByType.push_back(on_ntaccesscheckbytype);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByType()
{
    //LOG(INFO, "Break on NtAccessCheckByType");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarmByHandle(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle_fn& on_ntaccesscheckbytyperesultlistandauditalarmbyhandle)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultListAndAuditAlarmByHandle");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarmByHandle.push_back(on_ntaccesscheckbytyperesultlistandauditalarmbyhandle);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarmByHandle()
{
    //LOG(INFO, "Break on NtAccessCheckByTypeResultListAndAuditAlarmByHandle");
    constexpr int nargs = 17;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultListAndAuditAlarm(proc_t proc, const on_NtAccessCheckByTypeResultListAndAuditAlarm_fn& on_ntaccesscheckbytyperesultlistandauditalarm)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultListAndAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckByTypeResultListAndAuditAlarm.push_back(on_ntaccesscheckbytyperesultlistandauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultListAndAuditAlarm()
{
    //LOG(INFO, "Break on NtAccessCheckByTypeResultListAndAuditAlarm");
    constexpr int nargs = 16;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtAccessCheckByTypeResultList(proc_t proc, const on_NtAccessCheckByTypeResultList_fn& on_ntaccesscheckbytyperesultlist)
{
    const auto ok = setup_func(proc, "NtAccessCheckByTypeResultList");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheckByTypeResultList.push_back(on_ntaccesscheckbytyperesultlist);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheckByTypeResultList()
{
    //LOG(INFO, "Break on NtAccessCheckByTypeResultList");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAccessCheck(proc_t proc, const on_NtAccessCheck_fn& on_ntaccesscheck)
{
    const auto ok = setup_func(proc, "NtAccessCheck");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAccessCheck.push_back(on_ntaccesscheck);
    return true;
}

void monitor::GenericMonitor::on_NtAccessCheck()
{
    //LOG(INFO, "Break on NtAccessCheck");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus);
}

bool monitor::GenericMonitor::register_NtAddAtom(proc_t proc, const on_NtAddAtom_fn& on_ntaddatom)
{
    const auto ok = setup_func(proc, "NtAddAtom");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAddAtom.push_back(on_ntaddatom);
    return true;
}

void monitor::GenericMonitor::on_NtAddAtom()
{
    //LOG(INFO, "Break on NtAddAtom");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto AtomName        = nt::cast_to<nt::PWSTR>              (args[0]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Atom            = nt::cast_to<nt::PRTL_ATOM>          (args[2]);

    for(const auto& it : d_->observers_NtAddAtom)
        it(AtomName, Length, Atom);
}

bool monitor::GenericMonitor::register_NtAddBootEntry(proc_t proc, const on_NtAddBootEntry_fn& on_ntaddbootentry)
{
    const auto ok = setup_func(proc, "NtAddBootEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAddBootEntry.push_back(on_ntaddbootentry);
    return true;
}

void monitor::GenericMonitor::on_NtAddBootEntry()
{
    //LOG(INFO, "Break on NtAddBootEntry");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootEntry       = nt::cast_to<nt::PBOOT_ENTRY>        (args[0]);
    const auto Id              = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAddBootEntry)
        it(BootEntry, Id);
}

bool monitor::GenericMonitor::register_NtAddDriverEntry(proc_t proc, const on_NtAddDriverEntry_fn& on_ntadddriverentry)
{
    const auto ok = setup_func(proc, "NtAddDriverEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAddDriverEntry.push_back(on_ntadddriverentry);
    return true;
}

void monitor::GenericMonitor::on_NtAddDriverEntry()
{
    //LOG(INFO, "Break on NtAddDriverEntry");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverEntry     = nt::cast_to<nt::PEFI_DRIVER_ENTRY>  (args[0]);
    const auto Id              = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAddDriverEntry)
        it(DriverEntry, Id);
}

bool monitor::GenericMonitor::register_NtAdjustGroupsToken(proc_t proc, const on_NtAdjustGroupsToken_fn& on_ntadjustgroupstoken)
{
    const auto ok = setup_func(proc, "NtAdjustGroupsToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAdjustGroupsToken.push_back(on_ntadjustgroupstoken);
    return true;
}

void monitor::GenericMonitor::on_NtAdjustGroupsToken()
{
    //LOG(INFO, "Break on NtAdjustGroupsToken");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResetToDefault  = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto NewState        = nt::cast_to<nt::PTOKEN_GROUPS>      (args[2]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[3]);
    const auto PreviousState   = nt::cast_to<nt::PTOKEN_GROUPS>      (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAdjustGroupsToken)
        it(TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAdjustPrivilegesToken(proc_t proc, const on_NtAdjustPrivilegesToken_fn& on_ntadjustprivilegestoken)
{
    const auto ok = setup_func(proc, "NtAdjustPrivilegesToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAdjustPrivilegesToken.push_back(on_ntadjustprivilegestoken);
    return true;
}

void monitor::GenericMonitor::on_NtAdjustPrivilegesToken()
{
    //LOG(INFO, "Break on NtAdjustPrivilegesToken");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DisableAllPrivileges= nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto NewState        = nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[2]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[3]);
    const auto PreviousState   = nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAdjustPrivilegesToken)
        it(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlertResumeThread(proc_t proc, const on_NtAlertResumeThread_fn& on_ntalertresumethread)
{
    const auto ok = setup_func(proc, "NtAlertResumeThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlertResumeThread.push_back(on_ntalertresumethread);
    return true;
}

void monitor::GenericMonitor::on_NtAlertResumeThread()
{
    //LOG(INFO, "Break on NtAlertResumeThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtAlertResumeThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtAlertThread(proc_t proc, const on_NtAlertThread_fn& on_ntalertthread)
{
    const auto ok = setup_func(proc, "NtAlertThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlertThread.push_back(on_ntalertthread);
    return true;
}

void monitor::GenericMonitor::on_NtAlertThread()
{
    //LOG(INFO, "Break on NtAlertThread");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtAlertThread)
        it(ThreadHandle);
}

bool monitor::GenericMonitor::register_NtAllocateLocallyUniqueId(proc_t proc, const on_NtAllocateLocallyUniqueId_fn& on_ntallocatelocallyuniqueid)
{
    const auto ok = setup_func(proc, "NtAllocateLocallyUniqueId");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAllocateLocallyUniqueId.push_back(on_ntallocatelocallyuniqueid);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateLocallyUniqueId()
{
    //LOG(INFO, "Break on NtAllocateLocallyUniqueId");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Luid            = nt::cast_to<nt::PLUID>              (args[0]);

    for(const auto& it : d_->observers_NtAllocateLocallyUniqueId)
        it(Luid);
}

bool monitor::GenericMonitor::register_NtAllocateReserveObject(proc_t proc, const on_NtAllocateReserveObject_fn& on_ntallocatereserveobject)
{
    const auto ok = setup_func(proc, "NtAllocateReserveObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAllocateReserveObject.push_back(on_ntallocatereserveobject);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateReserveObject()
{
    //LOG(INFO, "Break on NtAllocateReserveObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MemoryReserveHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Type            = nt::cast_to<nt::MEMORY_RESERVE_TYPE>(args[2]);

    for(const auto& it : d_->observers_NtAllocateReserveObject)
        it(MemoryReserveHandle, ObjectAttributes, Type);
}

bool monitor::GenericMonitor::register_NtAllocateUserPhysicalPages(proc_t proc, const on_NtAllocateUserPhysicalPages_fn& on_ntallocateuserphysicalpages)
{
    const auto ok = setup_func(proc, "NtAllocateUserPhysicalPages");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAllocateUserPhysicalPages.push_back(on_ntallocateuserphysicalpages);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateUserPhysicalPages()
{
    //LOG(INFO, "Break on NtAllocateUserPhysicalPages");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::PULONG_PTR>         (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtAllocateUserPhysicalPages)
        it(ProcessHandle, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtAllocateUuids(proc_t proc, const on_NtAllocateUuids_fn& on_ntallocateuuids)
{
    const auto ok = setup_func(proc, "NtAllocateUuids");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAllocateUuids.push_back(on_ntallocateuuids);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateUuids()
{
    //LOG(INFO, "Break on NtAllocateUuids");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Time            = nt::cast_to<nt::PULARGE_INTEGER>    (args[0]);
    const auto Range           = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Sequence        = nt::cast_to<nt::PULONG>             (args[2]);
    const auto Seed            = nt::cast_to<nt::PCHAR>              (args[3]);

    for(const auto& it : d_->observers_NtAllocateUuids)
        it(Time, Range, Sequence, Seed);
}

bool monitor::GenericMonitor::register_NtAllocateVirtualMemory(proc_t proc, const on_NtAllocateVirtualMemory_fn& on_ntallocatevirtualmemory)
{
    const auto ok = setup_func(proc, "NtAllocateVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAllocateVirtualMemory.push_back(on_ntallocatevirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtAllocateVirtualMemory()
{
    //LOG(INFO, "Break on NtAllocateVirtualMemory");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ZeroBits        = nt::cast_to<nt::ULONG_PTR>          (args[2]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[3]);
    const auto AllocationType  = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Protect         = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtAllocateVirtualMemory)
        it(ProcessHandle, STARBaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

bool monitor::GenericMonitor::register_NtAlpcAcceptConnectPort(proc_t proc, const on_NtAlpcAcceptConnectPort_fn& on_ntalpcacceptconnectport)
{
    const auto ok = setup_func(proc, "NtAlpcAcceptConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcAcceptConnectPort.push_back(on_ntalpcacceptconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcAcceptConnectPort()
{
    //LOG(INFO, "Break on NtAlpcAcceptConnectPort");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(PortHandle, ConnectionPortHandle, Flags, ObjectAttributes, PortAttributes, PortContext, ConnectionRequest, ConnectionMessageAttributes, AcceptConnection);
}

bool monitor::GenericMonitor::register_NtAlpcCancelMessage(proc_t proc, const on_NtAlpcCancelMessage_fn& on_ntalpccancelmessage)
{
    const auto ok = setup_func(proc, "NtAlpcCancelMessage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCancelMessage.push_back(on_ntalpccancelmessage);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCancelMessage()
{
    //LOG(INFO, "Break on NtAlpcCancelMessage");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto MessageContext  = nt::cast_to<nt::PALPC_CONTEXT_ATTR> (args[2]);

    for(const auto& it : d_->observers_NtAlpcCancelMessage)
        it(PortHandle, Flags, MessageContext);
}

bool monitor::GenericMonitor::register_NtAlpcConnectPort(proc_t proc, const on_NtAlpcConnectPort_fn& on_ntalpcconnectport)
{
    const auto ok = setup_func(proc, "NtAlpcConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcConnectPort.push_back(on_ntalpcconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcConnectPort()
{
    //LOG(INFO, "Break on NtAlpcConnectPort");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(PortHandle, PortName, ObjectAttributes, PortAttributes, Flags, RequiredServerSid, ConnectionMessage, BufferLength, OutMessageAttributes, InMessageAttributes, Timeout);
}

bool monitor::GenericMonitor::register_NtAlpcCreatePort(proc_t proc, const on_NtAlpcCreatePort_fn& on_ntalpccreateport)
{
    const auto ok = setup_func(proc, "NtAlpcCreatePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCreatePort.push_back(on_ntalpccreateport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreatePort()
{
    //LOG(INFO, "Break on NtAlpcCreatePort");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto PortAttributes  = nt::cast_to<nt::PALPC_PORT_ATTRIBUTES>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreatePort)
        it(PortHandle, ObjectAttributes, PortAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcCreatePortSection(proc_t proc, const on_NtAlpcCreatePortSection_fn& on_ntalpccreateportsection)
{
    const auto ok = setup_func(proc, "NtAlpcCreatePortSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCreatePortSection.push_back(on_ntalpccreateportsection);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreatePortSection()
{
    //LOG(INFO, "Break on NtAlpcCreatePortSection");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto SectionSize     = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto AlpcSectionHandle= nt::cast_to<nt::PALPC_HANDLE>       (args[4]);
    const auto ActualSectionSize= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtAlpcCreatePortSection)
        it(PortHandle, Flags, SectionHandle, SectionSize, AlpcSectionHandle, ActualSectionSize);
}

bool monitor::GenericMonitor::register_NtAlpcCreateResourceReserve(proc_t proc, const on_NtAlpcCreateResourceReserve_fn& on_ntalpccreateresourcereserve)
{
    const auto ok = setup_func(proc, "NtAlpcCreateResourceReserve");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCreateResourceReserve.push_back(on_ntalpccreateresourcereserve);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateResourceReserve()
{
    //LOG(INFO, "Break on NtAlpcCreateResourceReserve");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto MessageSize     = nt::cast_to<nt::SIZE_T>             (args[2]);
    const auto ResourceId      = nt::cast_to<nt::PALPC_HANDLE>       (args[3]);

    for(const auto& it : d_->observers_NtAlpcCreateResourceReserve)
        it(PortHandle, Flags, MessageSize, ResourceId);
}

bool monitor::GenericMonitor::register_NtAlpcCreateSectionView(proc_t proc, const on_NtAlpcCreateSectionView_fn& on_ntalpccreatesectionview)
{
    const auto ok = setup_func(proc, "NtAlpcCreateSectionView");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCreateSectionView.push_back(on_ntalpccreatesectionview);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateSectionView()
{
    //LOG(INFO, "Break on NtAlpcCreateSectionView");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ViewAttributes  = nt::cast_to<nt::PALPC_DATA_VIEW_ATTR>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreateSectionView)
        it(PortHandle, Flags, ViewAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcCreateSecurityContext(proc_t proc, const on_NtAlpcCreateSecurityContext_fn& on_ntalpccreatesecuritycontext)
{
    const auto ok = setup_func(proc, "NtAlpcCreateSecurityContext");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcCreateSecurityContext.push_back(on_ntalpccreatesecuritycontext);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcCreateSecurityContext()
{
    //LOG(INFO, "Break on NtAlpcCreateSecurityContext");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SecurityAttribute= nt::cast_to<nt::PALPC_SECURITY_ATTR>(args[2]);

    for(const auto& it : d_->observers_NtAlpcCreateSecurityContext)
        it(PortHandle, Flags, SecurityAttribute);
}

bool monitor::GenericMonitor::register_NtAlpcDeletePortSection(proc_t proc, const on_NtAlpcDeletePortSection_fn& on_ntalpcdeleteportsection)
{
    const auto ok = setup_func(proc, "NtAlpcDeletePortSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcDeletePortSection.push_back(on_ntalpcdeleteportsection);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeletePortSection()
{
    //LOG(INFO, "Break on NtAlpcDeletePortSection");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SectionHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeletePortSection)
        it(PortHandle, Flags, SectionHandle);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteResourceReserve(proc_t proc, const on_NtAlpcDeleteResourceReserve_fn& on_ntalpcdeleteresourcereserve)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteResourceReserve");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcDeleteResourceReserve.push_back(on_ntalpcdeleteresourcereserve);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteResourceReserve()
{
    //LOG(INFO, "Break on NtAlpcDeleteResourceReserve");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ResourceId      = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteResourceReserve)
        it(PortHandle, Flags, ResourceId);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteSectionView(proc_t proc, const on_NtAlpcDeleteSectionView_fn& on_ntalpcdeletesectionview)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteSectionView");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcDeleteSectionView.push_back(on_ntalpcdeletesectionview);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteSectionView()
{
    //LOG(INFO, "Break on NtAlpcDeleteSectionView");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ViewBase        = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteSectionView)
        it(PortHandle, Flags, ViewBase);
}

bool monitor::GenericMonitor::register_NtAlpcDeleteSecurityContext(proc_t proc, const on_NtAlpcDeleteSecurityContext_fn& on_ntalpcdeletesecuritycontext)
{
    const auto ok = setup_func(proc, "NtAlpcDeleteSecurityContext");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcDeleteSecurityContext.push_back(on_ntalpcdeletesecuritycontext);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDeleteSecurityContext()
{
    //LOG(INFO, "Break on NtAlpcDeleteSecurityContext");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcDeleteSecurityContext)
        it(PortHandle, Flags, ContextHandle);
}

bool monitor::GenericMonitor::register_NtAlpcDisconnectPort(proc_t proc, const on_NtAlpcDisconnectPort_fn& on_ntalpcdisconnectport)
{
    const auto ok = setup_func(proc, "NtAlpcDisconnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcDisconnectPort.push_back(on_ntalpcdisconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcDisconnectPort()
{
    //LOG(INFO, "Break on NtAlpcDisconnectPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtAlpcDisconnectPort)
        it(PortHandle, Flags);
}

bool monitor::GenericMonitor::register_NtAlpcImpersonateClientOfPort(proc_t proc, const on_NtAlpcImpersonateClientOfPort_fn& on_ntalpcimpersonateclientofport)
{
    const auto ok = setup_func(proc, "NtAlpcImpersonateClientOfPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcImpersonateClientOfPort.push_back(on_ntalpcimpersonateclientofport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcImpersonateClientOfPort()
{
    //LOG(INFO, "Break on NtAlpcImpersonateClientOfPort");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto Reserved        = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtAlpcImpersonateClientOfPort)
        it(PortHandle, PortMessage, Reserved);
}

bool monitor::GenericMonitor::register_NtAlpcOpenSenderProcess(proc_t proc, const on_NtAlpcOpenSenderProcess_fn& on_ntalpcopensenderprocess)
{
    const auto ok = setup_func(proc, "NtAlpcOpenSenderProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcOpenSenderProcess.push_back(on_ntalpcopensenderprocess);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderProcess()
{
    //LOG(INFO, "Break on NtAlpcOpenSenderProcess");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[5]);

    for(const auto& it : d_->observers_NtAlpcOpenSenderProcess)
        it(ProcessHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcOpenSenderThread(proc_t proc, const on_NtAlpcOpenSenderThread_fn& on_ntalpcopensenderthread)
{
    const auto ok = setup_func(proc, "NtAlpcOpenSenderThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcOpenSenderThread.push_back(on_ntalpcopensenderthread);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcOpenSenderThread()
{
    //LOG(INFO, "Break on NtAlpcOpenSenderThread");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[5]);

    for(const auto& it : d_->observers_NtAlpcOpenSenderThread)
        it(ThreadHandle, PortHandle, PortMessage, Flags, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtAlpcQueryInformation(proc_t proc, const on_NtAlpcQueryInformation_fn& on_ntalpcqueryinformation)
{
    const auto ok = setup_func(proc, "NtAlpcQueryInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcQueryInformation.push_back(on_ntalpcqueryinformation);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcQueryInformation()
{
    //LOG(INFO, "Break on NtAlpcQueryInformation");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::ALPC_PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtAlpcQueryInformation)
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlpcQueryInformationMessage(proc_t proc, const on_NtAlpcQueryInformationMessage_fn& on_ntalpcqueryinformationmessage)
{
    const auto ok = setup_func(proc, "NtAlpcQueryInformationMessage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcQueryInformationMessage.push_back(on_ntalpcqueryinformationmessage);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcQueryInformationMessage()
{
    //LOG(INFO, "Break on NtAlpcQueryInformationMessage");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortMessage     = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto MessageInformationClass= nt::cast_to<nt::ALPC_MESSAGE_INFORMATION_CLASS>(args[2]);
    const auto MessageInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtAlpcQueryInformationMessage)
        it(PortHandle, PortMessage, MessageInformationClass, MessageInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtAlpcRevokeSecurityContext(proc_t proc, const on_NtAlpcRevokeSecurityContext_fn& on_ntalpcrevokesecuritycontext)
{
    const auto ok = setup_func(proc, "NtAlpcRevokeSecurityContext");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcRevokeSecurityContext.push_back(on_ntalpcrevokesecuritycontext);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcRevokeSecurityContext()
{
    //LOG(INFO, "Break on NtAlpcRevokeSecurityContext");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextHandle   = nt::cast_to<nt::ALPC_HANDLE>        (args[2]);

    for(const auto& it : d_->observers_NtAlpcRevokeSecurityContext)
        it(PortHandle, Flags, ContextHandle);
}

bool monitor::GenericMonitor::register_NtAlpcSendWaitReceivePort(proc_t proc, const on_NtAlpcSendWaitReceivePort_fn& on_ntalpcsendwaitreceiveport)
{
    const auto ok = setup_func(proc, "NtAlpcSendWaitReceivePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcSendWaitReceivePort.push_back(on_ntalpcsendwaitreceiveport);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcSendWaitReceivePort()
{
    //LOG(INFO, "Break on NtAlpcSendWaitReceivePort");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(PortHandle, Flags, SendMessage, SendMessageAttributes, ReceiveMessage, BufferLength, ReceiveMessageAttributes, Timeout);
}

bool monitor::GenericMonitor::register_NtAlpcSetInformation(proc_t proc, const on_NtAlpcSetInformation_fn& on_ntalpcsetinformation)
{
    const auto ok = setup_func(proc, "NtAlpcSetInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAlpcSetInformation.push_back(on_ntalpcsetinformation);
    return true;
}

void monitor::GenericMonitor::on_NtAlpcSetInformation()
{
    //LOG(INFO, "Break on NtAlpcSetInformation");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::ALPC_PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtAlpcSetInformation)
        it(PortHandle, PortInformationClass, PortInformation, Length);
}

bool monitor::GenericMonitor::register_NtApphelpCacheControl(proc_t proc, const on_NtApphelpCacheControl_fn& on_ntapphelpcachecontrol)
{
    const auto ok = setup_func(proc, "NtApphelpCacheControl");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtApphelpCacheControl.push_back(on_ntapphelpcachecontrol);
    return true;
}

void monitor::GenericMonitor::on_NtApphelpCacheControl()
{
    //LOG(INFO, "Break on NtApphelpCacheControl");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto type            = nt::cast_to<nt::APPHELPCOMMAND>     (args[0]);
    const auto buf             = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtApphelpCacheControl)
        it(type, buf);
}

bool monitor::GenericMonitor::register_NtAreMappedFilesTheSame(proc_t proc, const on_NtAreMappedFilesTheSame_fn& on_ntaremappedfilesthesame)
{
    const auto ok = setup_func(proc, "NtAreMappedFilesTheSame");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAreMappedFilesTheSame.push_back(on_ntaremappedfilesthesame);
    return true;
}

void monitor::GenericMonitor::on_NtAreMappedFilesTheSame()
{
    //LOG(INFO, "Break on NtAreMappedFilesTheSame");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto File1MappedAsAnImage= nt::cast_to<nt::PVOID>              (args[0]);
    const auto File2MappedAsFile= nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtAreMappedFilesTheSame)
        it(File1MappedAsAnImage, File2MappedAsFile);
}

bool monitor::GenericMonitor::register_NtAssignProcessToJobObject(proc_t proc, const on_NtAssignProcessToJobObject_fn& on_ntassignprocesstojobobject)
{
    const auto ok = setup_func(proc, "NtAssignProcessToJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtAssignProcessToJobObject.push_back(on_ntassignprocesstojobobject);
    return true;
}

void monitor::GenericMonitor::on_NtAssignProcessToJobObject()
{
    //LOG(INFO, "Break on NtAssignProcessToJobObject");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtAssignProcessToJobObject)
        it(JobHandle, ProcessHandle);
}

bool monitor::GenericMonitor::register_NtCallbackReturn(proc_t proc, const on_NtCallbackReturn_fn& on_ntcallbackreturn)
{
    const auto ok = setup_func(proc, "NtCallbackReturn");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCallbackReturn.push_back(on_ntcallbackreturn);
    return true;
}

void monitor::GenericMonitor::on_NtCallbackReturn()
{
    //LOG(INFO, "Break on NtCallbackReturn");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[0]);
    const auto OutputLength    = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Status          = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtCallbackReturn)
        it(OutputBuffer, OutputLength, Status);
}

bool monitor::GenericMonitor::register_NtCancelIoFileEx(proc_t proc, const on_NtCancelIoFileEx_fn& on_ntcanceliofileex)
{
    const auto ok = setup_func(proc, "NtCancelIoFileEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCancelIoFileEx.push_back(on_ntcanceliofileex);
    return true;
}

void monitor::GenericMonitor::on_NtCancelIoFileEx()
{
    //LOG(INFO, "Break on NtCancelIoFileEx");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoRequestToCancel= nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[2]);

    for(const auto& it : d_->observers_NtCancelIoFileEx)
        it(FileHandle, IoRequestToCancel, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelIoFile(proc_t proc, const on_NtCancelIoFile_fn& on_ntcanceliofile)
{
    const auto ok = setup_func(proc, "NtCancelIoFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCancelIoFile.push_back(on_ntcanceliofile);
    return true;
}

void monitor::GenericMonitor::on_NtCancelIoFile()
{
    //LOG(INFO, "Break on NtCancelIoFile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);

    for(const auto& it : d_->observers_NtCancelIoFile)
        it(FileHandle, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelSynchronousIoFile(proc_t proc, const on_NtCancelSynchronousIoFile_fn& on_ntcancelsynchronousiofile)
{
    const auto ok = setup_func(proc, "NtCancelSynchronousIoFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCancelSynchronousIoFile.push_back(on_ntcancelsynchronousiofile);
    return true;
}

void monitor::GenericMonitor::on_NtCancelSynchronousIoFile()
{
    //LOG(INFO, "Break on NtCancelSynchronousIoFile");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoRequestToCancel= nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[2]);

    for(const auto& it : d_->observers_NtCancelSynchronousIoFile)
        it(ThreadHandle, IoRequestToCancel, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtCancelTimer(proc_t proc, const on_NtCancelTimer_fn& on_ntcanceltimer)
{
    const auto ok = setup_func(proc, "NtCancelTimer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCancelTimer.push_back(on_ntcanceltimer);
    return true;
}

void monitor::GenericMonitor::on_NtCancelTimer()
{
    //LOG(INFO, "Break on NtCancelTimer");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto CurrentState    = nt::cast_to<nt::PBOOLEAN>           (args[1]);

    for(const auto& it : d_->observers_NtCancelTimer)
        it(TimerHandle, CurrentState);
}

bool monitor::GenericMonitor::register_NtClearEvent(proc_t proc, const on_NtClearEvent_fn& on_ntclearevent)
{
    const auto ok = setup_func(proc, "NtClearEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtClearEvent.push_back(on_ntclearevent);
    return true;
}

void monitor::GenericMonitor::on_NtClearEvent()
{
    //LOG(INFO, "Break on NtClearEvent");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtClearEvent)
        it(EventHandle);
}

bool monitor::GenericMonitor::register_NtClose(proc_t proc, const on_NtClose_fn& on_ntclose)
{
    const auto ok = setup_func(proc, "NtClose");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtClose.push_back(on_ntclose);
    return true;
}

void monitor::GenericMonitor::on_NtClose()
{
    //LOG(INFO, "Break on NtClose");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtClose)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtCloseObjectAuditAlarm(proc_t proc, const on_NtCloseObjectAuditAlarm_fn& on_ntcloseobjectauditalarm)
{
    const auto ok = setup_func(proc, "NtCloseObjectAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCloseObjectAuditAlarm.push_back(on_ntcloseobjectauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtCloseObjectAuditAlarm()
{
    //LOG(INFO, "Break on NtCloseObjectAuditAlarm");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto GenerateOnClose = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtCloseObjectAuditAlarm)
        it(SubsystemName, HandleId, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtCommitComplete(proc_t proc, const on_NtCommitComplete_fn& on_ntcommitcomplete)
{
    const auto ok = setup_func(proc, "NtCommitComplete");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCommitComplete.push_back(on_ntcommitcomplete);
    return true;
}

void monitor::GenericMonitor::on_NtCommitComplete()
{
    //LOG(INFO, "Break on NtCommitComplete");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtCommitComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtCommitEnlistment(proc_t proc, const on_NtCommitEnlistment_fn& on_ntcommitenlistment)
{
    const auto ok = setup_func(proc, "NtCommitEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCommitEnlistment.push_back(on_ntcommitenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtCommitEnlistment()
{
    //LOG(INFO, "Break on NtCommitEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtCommitEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtCommitTransaction(proc_t proc, const on_NtCommitTransaction_fn& on_ntcommittransaction)
{
    const auto ok = setup_func(proc, "NtCommitTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCommitTransaction.push_back(on_ntcommittransaction);
    return true;
}

void monitor::GenericMonitor::on_NtCommitTransaction()
{
    //LOG(INFO, "Break on NtCommitTransaction");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Wait            = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtCommitTransaction)
        it(TransactionHandle, Wait);
}

bool monitor::GenericMonitor::register_NtCompactKeys(proc_t proc, const on_NtCompactKeys_fn& on_ntcompactkeys)
{
    const auto ok = setup_func(proc, "NtCompactKeys");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCompactKeys.push_back(on_ntcompactkeys);
    return true;
}

void monitor::GenericMonitor::on_NtCompactKeys()
{
    //LOG(INFO, "Break on NtCompactKeys");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto KeyArray        = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtCompactKeys)
        it(Count, KeyArray);
}

bool monitor::GenericMonitor::register_NtCompareTokens(proc_t proc, const on_NtCompareTokens_fn& on_ntcomparetokens)
{
    const auto ok = setup_func(proc, "NtCompareTokens");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCompareTokens.push_back(on_ntcomparetokens);
    return true;
}

void monitor::GenericMonitor::on_NtCompareTokens()
{
    //LOG(INFO, "Break on NtCompareTokens");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FirstTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecondTokenHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Equal           = nt::cast_to<nt::PBOOLEAN>           (args[2]);

    for(const auto& it : d_->observers_NtCompareTokens)
        it(FirstTokenHandle, SecondTokenHandle, Equal);
}

bool monitor::GenericMonitor::register_NtCompleteConnectPort(proc_t proc, const on_NtCompleteConnectPort_fn& on_ntcompleteconnectport)
{
    const auto ok = setup_func(proc, "NtCompleteConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCompleteConnectPort.push_back(on_ntcompleteconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtCompleteConnectPort()
{
    //LOG(INFO, "Break on NtCompleteConnectPort");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtCompleteConnectPort)
        it(PortHandle);
}

bool monitor::GenericMonitor::register_NtCompressKey(proc_t proc, const on_NtCompressKey_fn& on_ntcompresskey)
{
    const auto ok = setup_func(proc, "NtCompressKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCompressKey.push_back(on_ntcompresskey);
    return true;
}

void monitor::GenericMonitor::on_NtCompressKey()
{
    //LOG(INFO, "Break on NtCompressKey");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Key             = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtCompressKey)
        it(Key);
}

bool monitor::GenericMonitor::register_NtConnectPort(proc_t proc, const on_NtConnectPort_fn& on_ntconnectport)
{
    const auto ok = setup_func(proc, "NtConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtConnectPort.push_back(on_ntconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtConnectPort()
{
    //LOG(INFO, "Break on NtConnectPort");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(PortHandle, PortName, SecurityQos, ClientView, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
}

bool monitor::GenericMonitor::register_NtContinue(proc_t proc, const on_NtContinue_fn& on_ntcontinue)
{
    const auto ok = setup_func(proc, "NtContinue");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtContinue.push_back(on_ntcontinue);
    return true;
}

void monitor::GenericMonitor::on_NtContinue()
{
    //LOG(INFO, "Break on NtContinue");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ContextRecord   = nt::cast_to<nt::PCONTEXT>           (args[0]);
    const auto TestAlert       = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtContinue)
        it(ContextRecord, TestAlert);
}

bool monitor::GenericMonitor::register_NtCreateDebugObject(proc_t proc, const on_NtCreateDebugObject_fn& on_ntcreatedebugobject)
{
    const auto ok = setup_func(proc, "NtCreateDebugObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateDebugObject.push_back(on_ntcreatedebugobject);
    return true;
}

void monitor::GenericMonitor::on_NtCreateDebugObject()
{
    //LOG(INFO, "Break on NtCreateDebugObject");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateDebugObject)
        it(DebugObjectHandle, DesiredAccess, ObjectAttributes, Flags);
}

bool monitor::GenericMonitor::register_NtCreateDirectoryObject(proc_t proc, const on_NtCreateDirectoryObject_fn& on_ntcreatedirectoryobject)
{
    const auto ok = setup_func(proc, "NtCreateDirectoryObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateDirectoryObject.push_back(on_ntcreatedirectoryobject);
    return true;
}

void monitor::GenericMonitor::on_NtCreateDirectoryObject()
{
    //LOG(INFO, "Break on NtCreateDirectoryObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateDirectoryObject)
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateEnlistment(proc_t proc, const on_NtCreateEnlistment_fn& on_ntcreateenlistment)
{
    const auto ok = setup_func(proc, "NtCreateEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateEnlistment.push_back(on_ntcreateenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEnlistment()
{
    //LOG(INFO, "Break on NtCreateEnlistment");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, TransactionHandle, ObjectAttributes, CreateOptions, NotificationMask, EnlistmentKey);
}

bool monitor::GenericMonitor::register_NtCreateEvent(proc_t proc, const on_NtCreateEvent_fn& on_ntcreateevent)
{
    const auto ok = setup_func(proc, "NtCreateEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateEvent.push_back(on_ntcreateevent);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEvent()
{
    //LOG(INFO, "Break on NtCreateEvent");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto EventType       = nt::cast_to<nt::EVENT_TYPE>         (args[3]);
    const auto InitialState    = nt::cast_to<nt::BOOLEAN>            (args[4]);

    for(const auto& it : d_->observers_NtCreateEvent)
        it(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
}

bool monitor::GenericMonitor::register_NtCreateEventPair(proc_t proc, const on_NtCreateEventPair_fn& on_ntcreateeventpair)
{
    const auto ok = setup_func(proc, "NtCreateEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateEventPair.push_back(on_ntcreateeventpair);
    return true;
}

void monitor::GenericMonitor::on_NtCreateEventPair()
{
    //LOG(INFO, "Break on NtCreateEventPair");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateEventPair)
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateFile(proc_t proc, const on_NtCreateFile_fn& on_ntcreatefile)
{
    const auto ok = setup_func(proc, "NtCreateFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateFile.push_back(on_ntcreatefile);
    return true;
}

void monitor::GenericMonitor::on_NtCreateFile()
{
    //LOG(INFO, "Break on NtCreateFile");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

bool monitor::GenericMonitor::register_NtCreateIoCompletion(proc_t proc, const on_NtCreateIoCompletion_fn& on_ntcreateiocompletion)
{
    const auto ok = setup_func(proc, "NtCreateIoCompletion");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateIoCompletion.push_back(on_ntcreateiocompletion);
    return true;
}

void monitor::GenericMonitor::on_NtCreateIoCompletion()
{
    //LOG(INFO, "Break on NtCreateIoCompletion");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateIoCompletion)
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes, Count);
}

bool monitor::GenericMonitor::register_NtCreateJobObject(proc_t proc, const on_NtCreateJobObject_fn& on_ntcreatejobobject)
{
    const auto ok = setup_func(proc, "NtCreateJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateJobObject.push_back(on_ntcreatejobobject);
    return true;
}

void monitor::GenericMonitor::on_NtCreateJobObject()
{
    //LOG(INFO, "Break on NtCreateJobObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtCreateJobObject)
        it(JobHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtCreateJobSet(proc_t proc, const on_NtCreateJobSet_fn& on_ntcreatejobset)
{
    const auto ok = setup_func(proc, "NtCreateJobSet");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateJobSet.push_back(on_ntcreatejobset);
    return true;
}

void monitor::GenericMonitor::on_NtCreateJobSet()
{
    //LOG(INFO, "Break on NtCreateJobSet");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NumJob          = nt::cast_to<nt::ULONG>              (args[0]);
    const auto UserJobSet      = nt::cast_to<nt::PJOB_SET_ARRAY>     (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtCreateJobSet)
        it(NumJob, UserJobSet, Flags);
}

bool monitor::GenericMonitor::register_NtCreateKeyedEvent(proc_t proc, const on_NtCreateKeyedEvent_fn& on_ntcreatekeyedevent)
{
    const auto ok = setup_func(proc, "NtCreateKeyedEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateKeyedEvent.push_back(on_ntcreatekeyedevent);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKeyedEvent()
{
    //LOG(INFO, "Break on NtCreateKeyedEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreateKeyedEvent)
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes, Flags);
}

bool monitor::GenericMonitor::register_NtCreateKey(proc_t proc, const on_NtCreateKey_fn& on_ntcreatekey)
{
    const auto ok = setup_func(proc, "NtCreateKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateKey.push_back(on_ntcreatekey);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKey()
{
    //LOG(INFO, "Break on NtCreateKey");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TitleIndex      = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Class           = nt::cast_to<nt::PUNICODE_STRING>    (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto Disposition     = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtCreateKey)
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);
}

bool monitor::GenericMonitor::register_NtCreateKeyTransacted(proc_t proc, const on_NtCreateKeyTransacted_fn& on_ntcreatekeytransacted)
{
    const auto ok = setup_func(proc, "NtCreateKeyTransacted");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateKeyTransacted.push_back(on_ntcreatekeytransacted);
    return true;
}

void monitor::GenericMonitor::on_NtCreateKeyTransacted()
{
    //LOG(INFO, "Break on NtCreateKeyTransacted");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, TransactionHandle, Disposition);
}

bool monitor::GenericMonitor::register_NtCreateMailslotFile(proc_t proc, const on_NtCreateMailslotFile_fn& on_ntcreatemailslotfile)
{
    const auto ok = setup_func(proc, "NtCreateMailslotFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateMailslotFile.push_back(on_ntcreatemailslotfile);
    return true;
}

void monitor::GenericMonitor::on_NtCreateMailslotFile()
{
    //LOG(INFO, "Break on NtCreateMailslotFile");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, CreateOptions, MailslotQuota, MaximumMessageSize, ReadTimeout);
}

bool monitor::GenericMonitor::register_NtCreateMutant(proc_t proc, const on_NtCreateMutant_fn& on_ntcreatemutant)
{
    const auto ok = setup_func(proc, "NtCreateMutant");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateMutant.push_back(on_ntcreatemutant);
    return true;
}

void monitor::GenericMonitor::on_NtCreateMutant()
{
    //LOG(INFO, "Break on NtCreateMutant");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto InitialOwner    = nt::cast_to<nt::BOOLEAN>            (args[3]);

    for(const auto& it : d_->observers_NtCreateMutant)
        it(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
}

bool monitor::GenericMonitor::register_NtCreateNamedPipeFile(proc_t proc, const on_NtCreateNamedPipeFile_fn& on_ntcreatenamedpipefile)
{
    const auto ok = setup_func(proc, "NtCreateNamedPipeFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateNamedPipeFile.push_back(on_ntcreatenamedpipefile);
    return true;
}

void monitor::GenericMonitor::on_NtCreateNamedPipeFile()
{
    //LOG(INFO, "Break on NtCreateNamedPipeFile");
    constexpr int nargs = 14;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, CreateDisposition, CreateOptions, NamedPipeType, ReadMode, CompletionMode, MaximumInstances, InboundQuota, OutboundQuota, DefaultTimeout);
}

bool monitor::GenericMonitor::register_NtCreatePagingFile(proc_t proc, const on_NtCreatePagingFile_fn& on_ntcreatepagingfile)
{
    const auto ok = setup_func(proc, "NtCreatePagingFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreatePagingFile.push_back(on_ntcreatepagingfile);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePagingFile()
{
    //LOG(INFO, "Break on NtCreatePagingFile");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PageFileName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto MinimumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);
    const auto MaximumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto Priority        = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtCreatePagingFile)
        it(PageFileName, MinimumSize, MaximumSize, Priority);
}

bool monitor::GenericMonitor::register_NtCreatePort(proc_t proc, const on_NtCreatePort_fn& on_ntcreateport)
{
    const auto ok = setup_func(proc, "NtCreatePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreatePort.push_back(on_ntcreateport);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePort()
{
    //LOG(INFO, "Break on NtCreatePort");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto MaxConnectionInfoLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto MaxMessageLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto MaxPoolUsage    = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtCreatePort)
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
}

bool monitor::GenericMonitor::register_NtCreatePrivateNamespace(proc_t proc, const on_NtCreatePrivateNamespace_fn& on_ntcreateprivatenamespace)
{
    const auto ok = setup_func(proc, "NtCreatePrivateNamespace");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreatePrivateNamespace.push_back(on_ntcreateprivatenamespace);
    return true;
}

void monitor::GenericMonitor::on_NtCreatePrivateNamespace()
{
    //LOG(INFO, "Break on NtCreatePrivateNamespace");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto BoundaryDescriptor= nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtCreatePrivateNamespace)
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
}

bool monitor::GenericMonitor::register_NtCreateProcessEx(proc_t proc, const on_NtCreateProcessEx_fn& on_ntcreateprocessex)
{
    const auto ok = setup_func(proc, "NtCreateProcessEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateProcessEx.push_back(on_ntcreateprocessex);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProcessEx()
{
    //LOG(INFO, "Break on NtCreateProcessEx");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, Flags, SectionHandle, DebugPort, ExceptionPort, JobMemberLevel);
}

bool monitor::GenericMonitor::register_NtCreateProcess(proc_t proc, const on_NtCreateProcess_fn& on_ntcreateprocess)
{
    const auto ok = setup_func(proc, "NtCreateProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateProcess.push_back(on_ntcreateprocess);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProcess()
{
    //LOG(INFO, "Break on NtCreateProcess");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ParentProcess, InheritObjectTable, SectionHandle, DebugPort, ExceptionPort);
}

bool monitor::GenericMonitor::register_NtCreateProfileEx(proc_t proc, const on_NtCreateProfileEx_fn& on_ntcreateprofileex)
{
    const auto ok = setup_func(proc, "NtCreateProfileEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateProfileEx.push_back(on_ntcreateprofileex);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProfileEx()
{
    //LOG(INFO, "Break on NtCreateProfileEx");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ProfileHandle, Process, ProfileBase, ProfileSize, BucketSize, Buffer, BufferSize, ProfileSource, GroupAffinityCount, GroupAffinity);
}

bool monitor::GenericMonitor::register_NtCreateProfile(proc_t proc, const on_NtCreateProfile_fn& on_ntcreateprofile)
{
    const auto ok = setup_func(proc, "NtCreateProfile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateProfile.push_back(on_ntcreateprofile);
    return true;
}

void monitor::GenericMonitor::on_NtCreateProfile()
{
    //LOG(INFO, "Break on NtCreateProfile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ProfileHandle, Process, RangeBase, RangeSize, BucketSize, Buffer, BufferSize, ProfileSource, Affinity);
}

bool monitor::GenericMonitor::register_NtCreateResourceManager(proc_t proc, const on_NtCreateResourceManager_fn& on_ntcreateresourcemanager)
{
    const auto ok = setup_func(proc, "NtCreateResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateResourceManager.push_back(on_ntcreateresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtCreateResourceManager()
{
    //LOG(INFO, "Break on NtCreateResourceManager");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto RmGuid          = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[5]);
    const auto Description     = nt::cast_to<nt::PUNICODE_STRING>    (args[6]);

    for(const auto& it : d_->observers_NtCreateResourceManager)
        it(ResourceManagerHandle, DesiredAccess, TmHandle, RmGuid, ObjectAttributes, CreateOptions, Description);
}

bool monitor::GenericMonitor::register_NtCreateSection(proc_t proc, const on_NtCreateSection_fn& on_ntcreatesection)
{
    const auto ok = setup_func(proc, "NtCreateSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateSection.push_back(on_ntcreatesection);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSection()
{
    //LOG(INFO, "Break on NtCreateSection");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto MaximumSize     = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto SectionPageProtection= nt::cast_to<nt::ULONG>              (args[4]);
    const auto AllocationAttributes= nt::cast_to<nt::ULONG>              (args[5]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[6]);

    for(const auto& it : d_->observers_NtCreateSection)
        it(SectionHandle, DesiredAccess, ObjectAttributes, MaximumSize, SectionPageProtection, AllocationAttributes, FileHandle);
}

bool monitor::GenericMonitor::register_NtCreateSemaphore(proc_t proc, const on_NtCreateSemaphore_fn& on_ntcreatesemaphore)
{
    const auto ok = setup_func(proc, "NtCreateSemaphore");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateSemaphore.push_back(on_ntcreatesemaphore);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSemaphore()
{
    //LOG(INFO, "Break on NtCreateSemaphore");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto InitialCount    = nt::cast_to<nt::LONG>               (args[3]);
    const auto MaximumCount    = nt::cast_to<nt::LONG>               (args[4]);

    for(const auto& it : d_->observers_NtCreateSemaphore)
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
}

bool monitor::GenericMonitor::register_NtCreateSymbolicLinkObject(proc_t proc, const on_NtCreateSymbolicLinkObject_fn& on_ntcreatesymboliclinkobject)
{
    const auto ok = setup_func(proc, "NtCreateSymbolicLinkObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateSymbolicLinkObject.push_back(on_ntcreatesymboliclinkobject);
    return true;
}

void monitor::GenericMonitor::on_NtCreateSymbolicLinkObject()
{
    //LOG(INFO, "Break on NtCreateSymbolicLinkObject");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LinkTarget      = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);

    for(const auto& it : d_->observers_NtCreateSymbolicLinkObject)
        it(LinkHandle, DesiredAccess, ObjectAttributes, LinkTarget);
}

bool monitor::GenericMonitor::register_NtCreateThreadEx(proc_t proc, const on_NtCreateThreadEx_fn& on_ntcreatethreadex)
{
    const auto ok = setup_func(proc, "NtCreateThreadEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateThreadEx.push_back(on_ntcreatethreadex);
    return true;
}

void monitor::GenericMonitor::on_NtCreateThreadEx()
{
    //LOG(INFO, "Break on NtCreateThreadEx");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, StartRoutine, Argument, CreateFlags, ZeroBits, StackSize, MaximumStackSize, AttributeList);
}

bool monitor::GenericMonitor::register_NtCreateThread(proc_t proc, const on_NtCreateThread_fn& on_ntcreatethread)
{
    const auto ok = setup_func(proc, "NtCreateThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateThread.push_back(on_ntcreatethread);
    return true;
}

void monitor::GenericMonitor::on_NtCreateThread()
{
    //LOG(INFO, "Break on NtCreateThread");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
}

bool monitor::GenericMonitor::register_NtCreateTimer(proc_t proc, const on_NtCreateTimer_fn& on_ntcreatetimer)
{
    const auto ok = setup_func(proc, "NtCreateTimer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateTimer.push_back(on_ntcreatetimer);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTimer()
{
    //LOG(INFO, "Break on NtCreateTimer");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TimerType       = nt::cast_to<nt::TIMER_TYPE>         (args[3]);

    for(const auto& it : d_->observers_NtCreateTimer)
        it(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
}

bool monitor::GenericMonitor::register_NtCreateToken(proc_t proc, const on_NtCreateToken_fn& on_ntcreatetoken)
{
    const auto ok = setup_func(proc, "NtCreateToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateToken.push_back(on_ntcreatetoken);
    return true;
}

void monitor::GenericMonitor::on_NtCreateToken()
{
    //LOG(INFO, "Break on NtCreateToken");
    constexpr int nargs = 13;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(TokenHandle, DesiredAccess, ObjectAttributes, TokenType, AuthenticationId, ExpirationTime, User, Groups, Privileges, Owner, PrimaryGroup, DefaultDacl, TokenSource);
}

bool monitor::GenericMonitor::register_NtCreateTransactionManager(proc_t proc, const on_NtCreateTransactionManager_fn& on_ntcreatetransactionmanager)
{
    const auto ok = setup_func(proc, "NtCreateTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateTransactionManager.push_back(on_ntcreatetransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTransactionManager()
{
    //LOG(INFO, "Break on NtCreateTransactionManager");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[4]);
    const auto CommitStrength  = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtCreateTransactionManager)
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, CreateOptions, CommitStrength);
}

bool monitor::GenericMonitor::register_NtCreateTransaction(proc_t proc, const on_NtCreateTransaction_fn& on_ntcreatetransaction)
{
    const auto ok = setup_func(proc, "NtCreateTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateTransaction.push_back(on_ntcreatetransaction);
    return true;
}

void monitor::GenericMonitor::on_NtCreateTransaction()
{
    //LOG(INFO, "Break on NtCreateTransaction");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle, CreateOptions, IsolationLevel, IsolationFlags, Timeout, Description);
}

bool monitor::GenericMonitor::register_NtCreateUserProcess(proc_t proc, const on_NtCreateUserProcess_fn& on_ntcreateuserprocess)
{
    const auto ok = setup_func(proc, "NtCreateUserProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateUserProcess.push_back(on_ntcreateuserprocess);
    return true;
}

void monitor::GenericMonitor::on_NtCreateUserProcess()
{
    //LOG(INFO, "Break on NtCreateUserProcess");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
}

bool monitor::GenericMonitor::register_NtCreateWaitablePort(proc_t proc, const on_NtCreateWaitablePort_fn& on_ntcreatewaitableport)
{
    const auto ok = setup_func(proc, "NtCreateWaitablePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateWaitablePort.push_back(on_ntcreatewaitableport);
    return true;
}

void monitor::GenericMonitor::on_NtCreateWaitablePort()
{
    //LOG(INFO, "Break on NtCreateWaitablePort");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto MaxConnectionInfoLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto MaxMessageLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto MaxPoolUsage    = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtCreateWaitablePort)
        it(PortHandle, ObjectAttributes, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
}

bool monitor::GenericMonitor::register_NtCreateWorkerFactory(proc_t proc, const on_NtCreateWorkerFactory_fn& on_ntcreateworkerfactory)
{
    const auto ok = setup_func(proc, "NtCreateWorkerFactory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtCreateWorkerFactory.push_back(on_ntcreateworkerfactory);
    return true;
}

void monitor::GenericMonitor::on_NtCreateWorkerFactory()
{
    //LOG(INFO, "Break on NtCreateWorkerFactory");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(WorkerFactoryHandleReturn, DesiredAccess, ObjectAttributes, CompletionPortHandle, WorkerProcessHandle, StartRoutine, StartParameter, MaxThreadCount, StackReserve, StackCommit);
}

bool monitor::GenericMonitor::register_NtDebugActiveProcess(proc_t proc, const on_NtDebugActiveProcess_fn& on_ntdebugactiveprocess)
{
    const auto ok = setup_func(proc, "NtDebugActiveProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDebugActiveProcess.push_back(on_ntdebugactiveprocess);
    return true;
}

void monitor::GenericMonitor::on_NtDebugActiveProcess()
{
    //LOG(INFO, "Break on NtDebugActiveProcess");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtDebugActiveProcess)
        it(ProcessHandle, DebugObjectHandle);
}

bool monitor::GenericMonitor::register_NtDebugContinue(proc_t proc, const on_NtDebugContinue_fn& on_ntdebugcontinue)
{
    const auto ok = setup_func(proc, "NtDebugContinue");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDebugContinue.push_back(on_ntdebugcontinue);
    return true;
}

void monitor::GenericMonitor::on_NtDebugContinue()
{
    //LOG(INFO, "Break on NtDebugContinue");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[1]);
    const auto ContinueStatus  = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtDebugContinue)
        it(DebugObjectHandle, ClientId, ContinueStatus);
}

bool monitor::GenericMonitor::register_NtDelayExecution(proc_t proc, const on_NtDelayExecution_fn& on_ntdelayexecution)
{
    const auto ok = setup_func(proc, "NtDelayExecution");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDelayExecution.push_back(on_ntdelayexecution);
    return true;
}

void monitor::GenericMonitor::on_NtDelayExecution()
{
    //LOG(INFO, "Break on NtDelayExecution");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DelayInterval   = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtDelayExecution)
        it(Alertable, DelayInterval);
}

bool monitor::GenericMonitor::register_NtDeleteAtom(proc_t proc, const on_NtDeleteAtom_fn& on_ntdeleteatom)
{
    const auto ok = setup_func(proc, "NtDeleteAtom");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteAtom.push_back(on_ntdeleteatom);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteAtom()
{
    //LOG(INFO, "Break on NtDeleteAtom");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Atom            = nt::cast_to<nt::RTL_ATOM>           (args[0]);

    for(const auto& it : d_->observers_NtDeleteAtom)
        it(Atom);
}

bool monitor::GenericMonitor::register_NtDeleteBootEntry(proc_t proc, const on_NtDeleteBootEntry_fn& on_ntdeletebootentry)
{
    const auto ok = setup_func(proc, "NtDeleteBootEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteBootEntry.push_back(on_ntdeletebootentry);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteBootEntry()
{
    //LOG(INFO, "Break on NtDeleteBootEntry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Id              = nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtDeleteBootEntry)
        it(Id);
}

bool monitor::GenericMonitor::register_NtDeleteDriverEntry(proc_t proc, const on_NtDeleteDriverEntry_fn& on_ntdeletedriverentry)
{
    const auto ok = setup_func(proc, "NtDeleteDriverEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteDriverEntry.push_back(on_ntdeletedriverentry);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteDriverEntry()
{
    //LOG(INFO, "Break on NtDeleteDriverEntry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Id              = nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtDeleteDriverEntry)
        it(Id);
}

bool monitor::GenericMonitor::register_NtDeleteFile(proc_t proc, const on_NtDeleteFile_fn& on_ntdeletefile)
{
    const auto ok = setup_func(proc, "NtDeleteFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteFile.push_back(on_ntdeletefile);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteFile()
{
    //LOG(INFO, "Break on NtDeleteFile");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);

    for(const auto& it : d_->observers_NtDeleteFile)
        it(ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtDeleteKey(proc_t proc, const on_NtDeleteKey_fn& on_ntdeletekey)
{
    const auto ok = setup_func(proc, "NtDeleteKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteKey.push_back(on_ntdeletekey);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteKey()
{
    //LOG(INFO, "Break on NtDeleteKey");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtDeleteKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtDeleteObjectAuditAlarm(proc_t proc, const on_NtDeleteObjectAuditAlarm_fn& on_ntdeleteobjectauditalarm)
{
    const auto ok = setup_func(proc, "NtDeleteObjectAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteObjectAuditAlarm.push_back(on_ntdeleteobjectauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteObjectAuditAlarm()
{
    //LOG(INFO, "Break on NtDeleteObjectAuditAlarm");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto GenerateOnClose = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtDeleteObjectAuditAlarm)
        it(SubsystemName, HandleId, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtDeletePrivateNamespace(proc_t proc, const on_NtDeletePrivateNamespace_fn& on_ntdeleteprivatenamespace)
{
    const auto ok = setup_func(proc, "NtDeletePrivateNamespace");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeletePrivateNamespace.push_back(on_ntdeleteprivatenamespace);
    return true;
}

void monitor::GenericMonitor::on_NtDeletePrivateNamespace()
{
    //LOG(INFO, "Break on NtDeletePrivateNamespace");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtDeletePrivateNamespace)
        it(NamespaceHandle);
}

bool monitor::GenericMonitor::register_NtDeleteValueKey(proc_t proc, const on_NtDeleteValueKey_fn& on_ntdeletevaluekey)
{
    const auto ok = setup_func(proc, "NtDeleteValueKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeleteValueKey.push_back(on_ntdeletevaluekey);
    return true;
}

void monitor::GenericMonitor::on_NtDeleteValueKey()
{
    //LOG(INFO, "Break on NtDeleteValueKey");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtDeleteValueKey)
        it(KeyHandle, ValueName);
}

bool monitor::GenericMonitor::register_NtDeviceIoControlFile(proc_t proc, const on_NtDeviceIoControlFile_fn& on_ntdeviceiocontrolfile)
{
    const auto ok = setup_func(proc, "NtDeviceIoControlFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDeviceIoControlFile.push_back(on_ntdeviceiocontrolfile);
    return true;
}

void monitor::GenericMonitor::on_NtDeviceIoControlFile()
{
    //LOG(INFO, "Break on NtDeviceIoControlFile");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtDisplayString(proc_t proc, const on_NtDisplayString_fn& on_ntdisplaystring)
{
    const auto ok = setup_func(proc, "NtDisplayString");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDisplayString.push_back(on_ntdisplaystring);
    return true;
}

void monitor::GenericMonitor::on_NtDisplayString()
{
    //LOG(INFO, "Break on NtDisplayString");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto String          = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtDisplayString)
        it(String);
}

bool monitor::GenericMonitor::register_NtDrawText(proc_t proc, const on_NtDrawText_fn& on_ntdrawtext)
{
    const auto ok = setup_func(proc, "NtDrawText");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDrawText.push_back(on_ntdrawtext);
    return true;
}

void monitor::GenericMonitor::on_NtDrawText()
{
    //LOG(INFO, "Break on NtDrawText");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Text            = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtDrawText)
        it(Text);
}

bool monitor::GenericMonitor::register_NtDuplicateObject(proc_t proc, const on_NtDuplicateObject_fn& on_ntduplicateobject)
{
    const auto ok = setup_func(proc, "NtDuplicateObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDuplicateObject.push_back(on_ntduplicateobject);
    return true;
}

void monitor::GenericMonitor::on_NtDuplicateObject()
{
    //LOG(INFO, "Break on NtDuplicateObject");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SourceProcessHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SourceHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto TargetProcessHandle= nt::cast_to<nt::HANDLE>             (args[2]);
    const auto TargetHandle    = nt::cast_to<nt::PHANDLE>            (args[3]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[4]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[5]);
    const auto Options         = nt::cast_to<nt::ULONG>              (args[6]);

    for(const auto& it : d_->observers_NtDuplicateObject)
        it(SourceProcessHandle, SourceHandle, TargetProcessHandle, TargetHandle, DesiredAccess, HandleAttributes, Options);
}

bool monitor::GenericMonitor::register_NtDuplicateToken(proc_t proc, const on_NtDuplicateToken_fn& on_ntduplicatetoken)
{
    const auto ok = setup_func(proc, "NtDuplicateToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDuplicateToken.push_back(on_ntduplicatetoken);
    return true;
}

void monitor::GenericMonitor::on_NtDuplicateToken()
{
    //LOG(INFO, "Break on NtDuplicateToken");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExistingTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto EffectiveOnly   = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto TokenType       = nt::cast_to<nt::TOKEN_TYPE>         (args[4]);
    const auto NewTokenHandle  = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtDuplicateToken)
        it(ExistingTokenHandle, DesiredAccess, ObjectAttributes, EffectiveOnly, TokenType, NewTokenHandle);
}

bool monitor::GenericMonitor::register_NtEnumerateBootEntries(proc_t proc, const on_NtEnumerateBootEntries_fn& on_ntenumeratebootentries)
{
    const auto ok = setup_func(proc, "NtEnumerateBootEntries");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateBootEntries.push_back(on_ntenumeratebootentries);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateBootEntries()
{
    //LOG(INFO, "Break on NtEnumerateBootEntries");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[0]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtEnumerateBootEntries)
        it(Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateDriverEntries(proc_t proc, const on_NtEnumerateDriverEntries_fn& on_ntenumeratedriverentries)
{
    const auto ok = setup_func(proc, "NtEnumerateDriverEntries");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateDriverEntries.push_back(on_ntenumeratedriverentries);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateDriverEntries()
{
    //LOG(INFO, "Break on NtEnumerateDriverEntries");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[0]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtEnumerateDriverEntries)
        it(Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateKey(proc_t proc, const on_NtEnumerateKey_fn& on_ntenumeratekey)
{
    const auto ok = setup_func(proc, "NtEnumerateKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateKey.push_back(on_ntenumeratekey);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateKey()
{
    //LOG(INFO, "Break on NtEnumerateKey");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto KeyInformationClass= nt::cast_to<nt::KEY_INFORMATION_CLASS>(args[2]);
    const auto KeyInformation  = nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtEnumerateKey)
        it(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtEnumerateSystemEnvironmentValuesEx(proc_t proc, const on_NtEnumerateSystemEnvironmentValuesEx_fn& on_ntenumeratesystemenvironmentvaluesex)
{
    const auto ok = setup_func(proc, "NtEnumerateSystemEnvironmentValuesEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateSystemEnvironmentValuesEx.push_back(on_ntenumeratesystemenvironmentvaluesex);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateSystemEnvironmentValuesEx()
{
    //LOG(INFO, "Break on NtEnumerateSystemEnvironmentValuesEx");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InformationClass= nt::cast_to<nt::ULONG>              (args[0]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[1]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtEnumerateSystemEnvironmentValuesEx)
        it(InformationClass, Buffer, BufferLength);
}

bool monitor::GenericMonitor::register_NtEnumerateTransactionObject(proc_t proc, const on_NtEnumerateTransactionObject_fn& on_ntenumeratetransactionobject)
{
    const auto ok = setup_func(proc, "NtEnumerateTransactionObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateTransactionObject.push_back(on_ntenumeratetransactionobject);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateTransactionObject()
{
    //LOG(INFO, "Break on NtEnumerateTransactionObject");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto RootObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto QueryType       = nt::cast_to<nt::KTMOBJECT_TYPE>     (args[1]);
    const auto ObjectCursor    = nt::cast_to<nt::PKTMOBJECT_CURSOR>  (args[2]);
    const auto ObjectCursorLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtEnumerateTransactionObject)
        it(RootObjectHandle, QueryType, ObjectCursor, ObjectCursorLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtEnumerateValueKey(proc_t proc, const on_NtEnumerateValueKey_fn& on_ntenumeratevaluekey)
{
    const auto ok = setup_func(proc, "NtEnumerateValueKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnumerateValueKey.push_back(on_ntenumeratevaluekey);
    return true;
}

void monitor::GenericMonitor::on_NtEnumerateValueKey()
{
    //LOG(INFO, "Break on NtEnumerateValueKey");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto KeyValueInformationClass= nt::cast_to<nt::KEY_VALUE_INFORMATION_CLASS>(args[2]);
    const auto KeyValueInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtEnumerateValueKey)
        it(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtExtendSection(proc_t proc, const on_NtExtendSection_fn& on_ntextendsection)
{
    const auto ok = setup_func(proc, "NtExtendSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtExtendSection.push_back(on_ntextendsection);
    return true;
}

void monitor::GenericMonitor::on_NtExtendSection()
{
    //LOG(INFO, "Break on NtExtendSection");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NewSectionSize  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtExtendSection)
        it(SectionHandle, NewSectionSize);
}

bool monitor::GenericMonitor::register_NtFilterToken(proc_t proc, const on_NtFilterToken_fn& on_ntfiltertoken)
{
    const auto ok = setup_func(proc, "NtFilterToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFilterToken.push_back(on_ntfiltertoken);
    return true;
}

void monitor::GenericMonitor::on_NtFilterToken()
{
    //LOG(INFO, "Break on NtFilterToken");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExistingTokenHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto SidsToDisable   = nt::cast_to<nt::PTOKEN_GROUPS>      (args[2]);
    const auto PrivilegesToDelete= nt::cast_to<nt::PTOKEN_PRIVILEGES>  (args[3]);
    const auto RestrictedSids  = nt::cast_to<nt::PTOKEN_GROUPS>      (args[4]);
    const auto NewTokenHandle  = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtFilterToken)
        it(ExistingTokenHandle, Flags, SidsToDisable, PrivilegesToDelete, RestrictedSids, NewTokenHandle);
}

bool monitor::GenericMonitor::register_NtFindAtom(proc_t proc, const on_NtFindAtom_fn& on_ntfindatom)
{
    const auto ok = setup_func(proc, "NtFindAtom");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFindAtom.push_back(on_ntfindatom);
    return true;
}

void monitor::GenericMonitor::on_NtFindAtom()
{
    //LOG(INFO, "Break on NtFindAtom");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto AtomName        = nt::cast_to<nt::PWSTR>              (args[0]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Atom            = nt::cast_to<nt::PRTL_ATOM>          (args[2]);

    for(const auto& it : d_->observers_NtFindAtom)
        it(AtomName, Length, Atom);
}

bool monitor::GenericMonitor::register_NtFlushBuffersFile(proc_t proc, const on_NtFlushBuffersFile_fn& on_ntflushbuffersfile)
{
    const auto ok = setup_func(proc, "NtFlushBuffersFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushBuffersFile.push_back(on_ntflushbuffersfile);
    return true;
}

void monitor::GenericMonitor::on_NtFlushBuffersFile()
{
    //LOG(INFO, "Break on NtFlushBuffersFile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);

    for(const auto& it : d_->observers_NtFlushBuffersFile)
        it(FileHandle, IoStatusBlock);
}

bool monitor::GenericMonitor::register_NtFlushInstallUILanguage(proc_t proc, const on_NtFlushInstallUILanguage_fn& on_ntflushinstalluilanguage)
{
    const auto ok = setup_func(proc, "NtFlushInstallUILanguage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushInstallUILanguage.push_back(on_ntflushinstalluilanguage);
    return true;
}

void monitor::GenericMonitor::on_NtFlushInstallUILanguage()
{
    //LOG(INFO, "Break on NtFlushInstallUILanguage");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InstallUILanguage= nt::cast_to<nt::LANGID>             (args[0]);
    const auto SetComittedFlag = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtFlushInstallUILanguage)
        it(InstallUILanguage, SetComittedFlag);
}

bool monitor::GenericMonitor::register_NtFlushInstructionCache(proc_t proc, const on_NtFlushInstructionCache_fn& on_ntflushinstructioncache)
{
    const auto ok = setup_func(proc, "NtFlushInstructionCache");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushInstructionCache.push_back(on_ntflushinstructioncache);
    return true;
}

void monitor::GenericMonitor::on_NtFlushInstructionCache()
{
    //LOG(INFO, "Break on NtFlushInstructionCache");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Length          = nt::cast_to<nt::SIZE_T>             (args[2]);

    for(const auto& it : d_->observers_NtFlushInstructionCache)
        it(ProcessHandle, BaseAddress, Length);
}

bool monitor::GenericMonitor::register_NtFlushKey(proc_t proc, const on_NtFlushKey_fn& on_ntflushkey)
{
    const auto ok = setup_func(proc, "NtFlushKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushKey.push_back(on_ntflushkey);
    return true;
}

void monitor::GenericMonitor::on_NtFlushKey()
{
    //LOG(INFO, "Break on NtFlushKey");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtFlushKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtFlushVirtualMemory(proc_t proc, const on_NtFlushVirtualMemory_fn& on_ntflushvirtualmemory)
{
    const auto ok = setup_func(proc, "NtFlushVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushVirtualMemory.push_back(on_ntflushvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtFlushVirtualMemory()
{
    //LOG(INFO, "Break on NtFlushVirtualMemory");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto IoStatus        = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);

    for(const auto& it : d_->observers_NtFlushVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, IoStatus);
}

bool monitor::GenericMonitor::register_NtFreeUserPhysicalPages(proc_t proc, const on_NtFreeUserPhysicalPages_fn& on_ntfreeuserphysicalpages)
{
    const auto ok = setup_func(proc, "NtFreeUserPhysicalPages");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFreeUserPhysicalPages.push_back(on_ntfreeuserphysicalpages);
    return true;
}

void monitor::GenericMonitor::on_NtFreeUserPhysicalPages()
{
    //LOG(INFO, "Break on NtFreeUserPhysicalPages");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::PULONG_PTR>         (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtFreeUserPhysicalPages)
        it(ProcessHandle, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtFreeVirtualMemory(proc_t proc, const on_NtFreeVirtualMemory_fn& on_ntfreevirtualmemory)
{
    const auto ok = setup_func(proc, "NtFreeVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFreeVirtualMemory.push_back(on_ntfreevirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtFreeVirtualMemory()
{
    //LOG(INFO, "Break on NtFreeVirtualMemory");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto FreeType        = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtFreeVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, FreeType);
}

bool monitor::GenericMonitor::register_NtFreezeRegistry(proc_t proc, const on_NtFreezeRegistry_fn& on_ntfreezeregistry)
{
    const auto ok = setup_func(proc, "NtFreezeRegistry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFreezeRegistry.push_back(on_ntfreezeregistry);
    return true;
}

void monitor::GenericMonitor::on_NtFreezeRegistry()
{
    //LOG(INFO, "Break on NtFreezeRegistry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimeOutInSeconds= nt::cast_to<nt::ULONG>              (args[0]);

    for(const auto& it : d_->observers_NtFreezeRegistry)
        it(TimeOutInSeconds);
}

bool monitor::GenericMonitor::register_NtFreezeTransactions(proc_t proc, const on_NtFreezeTransactions_fn& on_ntfreezetransactions)
{
    const auto ok = setup_func(proc, "NtFreezeTransactions");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFreezeTransactions.push_back(on_ntfreezetransactions);
    return true;
}

void monitor::GenericMonitor::on_NtFreezeTransactions()
{
    //LOG(INFO, "Break on NtFreezeTransactions");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FreezeTimeout   = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto ThawTimeout     = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtFreezeTransactions)
        it(FreezeTimeout, ThawTimeout);
}

bool monitor::GenericMonitor::register_NtFsControlFile(proc_t proc, const on_NtFsControlFile_fn& on_ntfscontrolfile)
{
    const auto ok = setup_func(proc, "NtFsControlFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFsControlFile.push_back(on_ntfscontrolfile);
    return true;
}

void monitor::GenericMonitor::on_NtFsControlFile()
{
    //LOG(INFO, "Break on NtFsControlFile");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtGetContextThread(proc_t proc, const on_NtGetContextThread_fn& on_ntgetcontextthread)
{
    const auto ok = setup_func(proc, "NtGetContextThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetContextThread.push_back(on_ntgetcontextthread);
    return true;
}

void monitor::GenericMonitor::on_NtGetContextThread()
{
    //LOG(INFO, "Break on NtGetContextThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadContext   = nt::cast_to<nt::PCONTEXT>           (args[1]);

    for(const auto& it : d_->observers_NtGetContextThread)
        it(ThreadHandle, ThreadContext);
}

bool monitor::GenericMonitor::register_NtGetDevicePowerState(proc_t proc, const on_NtGetDevicePowerState_fn& on_ntgetdevicepowerstate)
{
    const auto ok = setup_func(proc, "NtGetDevicePowerState");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetDevicePowerState.push_back(on_ntgetdevicepowerstate);
    return true;
}

void monitor::GenericMonitor::on_NtGetDevicePowerState()
{
    //LOG(INFO, "Break on NtGetDevicePowerState");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Device          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARState       = nt::cast_to<nt::DEVICE_POWER_STATE> (args[1]);

    for(const auto& it : d_->observers_NtGetDevicePowerState)
        it(Device, STARState);
}

bool monitor::GenericMonitor::register_NtGetMUIRegistryInfo(proc_t proc, const on_NtGetMUIRegistryInfo_fn& on_ntgetmuiregistryinfo)
{
    const auto ok = setup_func(proc, "NtGetMUIRegistryInfo");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetMUIRegistryInfo.push_back(on_ntgetmuiregistryinfo);
    return true;
}

void monitor::GenericMonitor::on_NtGetMUIRegistryInfo()
{
    //LOG(INFO, "Break on NtGetMUIRegistryInfo");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Flags           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto DataSize        = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Data            = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_NtGetMUIRegistryInfo)
        it(Flags, DataSize, Data);
}

bool monitor::GenericMonitor::register_NtGetNextProcess(proc_t proc, const on_NtGetNextProcess_fn& on_ntgetnextprocess)
{
    const auto ok = setup_func(proc, "NtGetNextProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetNextProcess.push_back(on_ntgetnextprocess);
    return true;
}

void monitor::GenericMonitor::on_NtGetNextProcess()
{
    //LOG(INFO, "Break on NtGetNextProcess");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[3]);
    const auto NewProcessHandle= nt::cast_to<nt::PHANDLE>            (args[4]);

    for(const auto& it : d_->observers_NtGetNextProcess)
        it(ProcessHandle, DesiredAccess, HandleAttributes, Flags, NewProcessHandle);
}

bool monitor::GenericMonitor::register_NtGetNextThread(proc_t proc, const on_NtGetNextThread_fn& on_ntgetnextthread)
{
    const auto ok = setup_func(proc, "NtGetNextThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetNextThread.push_back(on_ntgetnextthread);
    return true;
}

void monitor::GenericMonitor::on_NtGetNextThread()
{
    //LOG(INFO, "Break on NtGetNextThread");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[2]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[3]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[4]);
    const auto NewThreadHandle = nt::cast_to<nt::PHANDLE>            (args[5]);

    for(const auto& it : d_->observers_NtGetNextThread)
        it(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
}

bool monitor::GenericMonitor::register_NtGetNlsSectionPtr(proc_t proc, const on_NtGetNlsSectionPtr_fn& on_ntgetnlssectionptr)
{
    const auto ok = setup_func(proc, "NtGetNlsSectionPtr");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetNlsSectionPtr.push_back(on_ntgetnlssectionptr);
    return true;
}

void monitor::GenericMonitor::on_NtGetNlsSectionPtr()
{
    //LOG(INFO, "Break on NtGetNlsSectionPtr");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionType     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto SectionData     = nt::cast_to<nt::ULONG>              (args[1]);
    const auto ContextData     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto STARSectionPointer= nt::cast_to<nt::PVOID>              (args[3]);
    const auto SectionSize     = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtGetNlsSectionPtr)
        it(SectionType, SectionData, ContextData, STARSectionPointer, SectionSize);
}

bool monitor::GenericMonitor::register_NtGetNotificationResourceManager(proc_t proc, const on_NtGetNotificationResourceManager_fn& on_ntgetnotificationresourcemanager)
{
    const auto ok = setup_func(proc, "NtGetNotificationResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetNotificationResourceManager.push_back(on_ntgetnotificationresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtGetNotificationResourceManager()
{
    //LOG(INFO, "Break on NtGetNotificationResourceManager");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionNotification= nt::cast_to<nt::PTRANSACTION_NOTIFICATION>(args[1]);
    const auto NotificationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);
    const auto Asynchronous    = nt::cast_to<nt::ULONG>              (args[5]);
    const auto AsynchronousContext= nt::cast_to<nt::ULONG_PTR>          (args[6]);

    for(const auto& it : d_->observers_NtGetNotificationResourceManager)
        it(ResourceManagerHandle, TransactionNotification, NotificationLength, Timeout, ReturnLength, Asynchronous, AsynchronousContext);
}

bool monitor::GenericMonitor::register_NtGetPlugPlayEvent(proc_t proc, const on_NtGetPlugPlayEvent_fn& on_ntgetplugplayevent)
{
    const auto ok = setup_func(proc, "NtGetPlugPlayEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetPlugPlayEvent.push_back(on_ntgetplugplayevent);
    return true;
}

void monitor::GenericMonitor::on_NtGetPlugPlayEvent()
{
    //LOG(INFO, "Break on NtGetPlugPlayEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Context         = nt::cast_to<nt::PVOID>              (args[1]);
    const auto EventBlock      = nt::cast_to<nt::PPLUGPLAY_EVENT_BLOCK>(args[2]);
    const auto EventBufferSize = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtGetPlugPlayEvent)
        it(EventHandle, Context, EventBlock, EventBufferSize);
}

bool monitor::GenericMonitor::register_NtGetWriteWatch(proc_t proc, const on_NtGetWriteWatch_fn& on_ntgetwritewatch)
{
    const auto ok = setup_func(proc, "NtGetWriteWatch");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetWriteWatch.push_back(on_ntgetwritewatch);
    return true;
}

void monitor::GenericMonitor::on_NtGetWriteWatch()
{
    //LOG(INFO, "Break on NtGetWriteWatch");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto RegionSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto STARUserAddressArray= nt::cast_to<nt::PVOID>              (args[4]);
    const auto EntriesInUserAddressArray= nt::cast_to<nt::PULONG_PTR>         (args[5]);
    const auto Granularity     = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtGetWriteWatch)
        it(ProcessHandle, Flags, BaseAddress, RegionSize, STARUserAddressArray, EntriesInUserAddressArray, Granularity);
}

bool monitor::GenericMonitor::register_NtImpersonateAnonymousToken(proc_t proc, const on_NtImpersonateAnonymousToken_fn& on_ntimpersonateanonymoustoken)
{
    const auto ok = setup_func(proc, "NtImpersonateAnonymousToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtImpersonateAnonymousToken.push_back(on_ntimpersonateanonymoustoken);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateAnonymousToken()
{
    //LOG(INFO, "Break on NtImpersonateAnonymousToken");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtImpersonateAnonymousToken)
        it(ThreadHandle);
}

bool monitor::GenericMonitor::register_NtImpersonateClientOfPort(proc_t proc, const on_NtImpersonateClientOfPort_fn& on_ntimpersonateclientofport)
{
    const auto ok = setup_func(proc, "NtImpersonateClientOfPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtImpersonateClientOfPort.push_back(on_ntimpersonateclientofport);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateClientOfPort()
{
    //LOG(INFO, "Break on NtImpersonateClientOfPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtImpersonateClientOfPort)
        it(PortHandle, Message);
}

bool monitor::GenericMonitor::register_NtImpersonateThread(proc_t proc, const on_NtImpersonateThread_fn& on_ntimpersonatethread)
{
    const auto ok = setup_func(proc, "NtImpersonateThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtImpersonateThread.push_back(on_ntimpersonatethread);
    return true;
}

void monitor::GenericMonitor::on_NtImpersonateThread()
{
    //LOG(INFO, "Break on NtImpersonateThread");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ServerThreadHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ClientThreadHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto SecurityQos     = nt::cast_to<nt::PSECURITY_QUALITY_OF_SERVICE>(args[2]);

    for(const auto& it : d_->observers_NtImpersonateThread)
        it(ServerThreadHandle, ClientThreadHandle, SecurityQos);
}

bool monitor::GenericMonitor::register_NtInitializeNlsFiles(proc_t proc, const on_NtInitializeNlsFiles_fn& on_ntinitializenlsfiles)
{
    const auto ok = setup_func(proc, "NtInitializeNlsFiles");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtInitializeNlsFiles.push_back(on_ntinitializenlsfiles);
    return true;
}

void monitor::GenericMonitor::on_NtInitializeNlsFiles()
{
    //LOG(INFO, "Break on NtInitializeNlsFiles");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::PLCID>              (args[1]);
    const auto DefaultCasingTableSize= nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);

    for(const auto& it : d_->observers_NtInitializeNlsFiles)
        it(STARBaseAddress, DefaultLocaleId, DefaultCasingTableSize);
}

bool monitor::GenericMonitor::register_NtInitializeRegistry(proc_t proc, const on_NtInitializeRegistry_fn& on_ntinitializeregistry)
{
    const auto ok = setup_func(proc, "NtInitializeRegistry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtInitializeRegistry.push_back(on_ntinitializeregistry);
    return true;
}

void monitor::GenericMonitor::on_NtInitializeRegistry()
{
    //LOG(INFO, "Break on NtInitializeRegistry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootCondition   = nt::cast_to<nt::USHORT>             (args[0]);

    for(const auto& it : d_->observers_NtInitializeRegistry)
        it(BootCondition);
}

bool monitor::GenericMonitor::register_NtInitiatePowerAction(proc_t proc, const on_NtInitiatePowerAction_fn& on_ntinitiatepoweraction)
{
    const auto ok = setup_func(proc, "NtInitiatePowerAction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtInitiatePowerAction.push_back(on_ntinitiatepoweraction);
    return true;
}

void monitor::GenericMonitor::on_NtInitiatePowerAction()
{
    //LOG(INFO, "Break on NtInitiatePowerAction");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemAction    = nt::cast_to<nt::POWER_ACTION>       (args[0]);
    const auto MinSystemState  = nt::cast_to<nt::SYSTEM_POWER_STATE> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Asynchronous    = nt::cast_to<nt::BOOLEAN>            (args[3]);

    for(const auto& it : d_->observers_NtInitiatePowerAction)
        it(SystemAction, MinSystemState, Flags, Asynchronous);
}

bool monitor::GenericMonitor::register_NtIsProcessInJob(proc_t proc, const on_NtIsProcessInJob_fn& on_ntisprocessinjob)
{
    const auto ok = setup_func(proc, "NtIsProcessInJob");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtIsProcessInJob.push_back(on_ntisprocessinjob);
    return true;
}

void monitor::GenericMonitor::on_NtIsProcessInJob()
{
    //LOG(INFO, "Break on NtIsProcessInJob");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtIsProcessInJob)
        it(ProcessHandle, JobHandle);
}

bool monitor::GenericMonitor::register_NtListenPort(proc_t proc, const on_NtListenPort_fn& on_ntlistenport)
{
    const auto ok = setup_func(proc, "NtListenPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtListenPort.push_back(on_ntlistenport);
    return true;
}

void monitor::GenericMonitor::on_NtListenPort()
{
    //LOG(INFO, "Break on NtListenPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ConnectionRequest= nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtListenPort)
        it(PortHandle, ConnectionRequest);
}

bool monitor::GenericMonitor::register_NtLoadDriver(proc_t proc, const on_NtLoadDriver_fn& on_ntloaddriver)
{
    const auto ok = setup_func(proc, "NtLoadDriver");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLoadDriver.push_back(on_ntloaddriver);
    return true;
}

void monitor::GenericMonitor::on_NtLoadDriver()
{
    //LOG(INFO, "Break on NtLoadDriver");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverServiceName= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtLoadDriver)
        it(DriverServiceName);
}

bool monitor::GenericMonitor::register_NtLoadKey2(proc_t proc, const on_NtLoadKey2_fn& on_ntloadkey2)
{
    const auto ok = setup_func(proc, "NtLoadKey2");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLoadKey2.push_back(on_ntloadkey2);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKey2()
{
    //LOG(INFO, "Break on NtLoadKey2");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtLoadKey2)
        it(TargetKey, SourceFile, Flags);
}

bool monitor::GenericMonitor::register_NtLoadKeyEx(proc_t proc, const on_NtLoadKeyEx_fn& on_ntloadkeyex)
{
    const auto ok = setup_func(proc, "NtLoadKeyEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLoadKeyEx.push_back(on_ntloadkeyex);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKeyEx()
{
    //LOG(INFO, "Break on NtLoadKeyEx");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto TrustClassKey   = nt::cast_to<nt::HANDLE>             (args[3]);

    for(const auto& it : d_->observers_NtLoadKeyEx)
        it(TargetKey, SourceFile, Flags, TrustClassKey);
}

bool monitor::GenericMonitor::register_NtLoadKey(proc_t proc, const on_NtLoadKey_fn& on_ntloadkey)
{
    const auto ok = setup_func(proc, "NtLoadKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLoadKey.push_back(on_ntloadkey);
    return true;
}

void monitor::GenericMonitor::on_NtLoadKey()
{
    //LOG(INFO, "Break on NtLoadKey");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto SourceFile      = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[1]);

    for(const auto& it : d_->observers_NtLoadKey)
        it(TargetKey, SourceFile);
}

bool monitor::GenericMonitor::register_NtLockFile(proc_t proc, const on_NtLockFile_fn& on_ntlockfile)
{
    const auto ok = setup_func(proc, "NtLockFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLockFile.push_back(on_ntlockfile);
    return true;
}

void monitor::GenericMonitor::on_NtLockFile()
{
    //LOG(INFO, "Break on NtLockFile");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, ByteOffset, Length, Key, FailImmediately, ExclusiveLock);
}

bool monitor::GenericMonitor::register_NtLockProductActivationKeys(proc_t proc, const on_NtLockProductActivationKeys_fn& on_ntlockproductactivationkeys)
{
    const auto ok = setup_func(proc, "NtLockProductActivationKeys");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLockProductActivationKeys.push_back(on_ntlockproductactivationkeys);
    return true;
}

void monitor::GenericMonitor::on_NtLockProductActivationKeys()
{
    //LOG(INFO, "Break on NtLockProductActivationKeys");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARpPrivateVer = nt::cast_to<nt::ULONG>              (args[0]);
    const auto STARpSafeMode   = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtLockProductActivationKeys)
        it(STARpPrivateVer, STARpSafeMode);
}

bool monitor::GenericMonitor::register_NtLockRegistryKey(proc_t proc, const on_NtLockRegistryKey_fn& on_ntlockregistrykey)
{
    const auto ok = setup_func(proc, "NtLockRegistryKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLockRegistryKey.push_back(on_ntlockregistrykey);
    return true;
}

void monitor::GenericMonitor::on_NtLockRegistryKey()
{
    //LOG(INFO, "Break on NtLockRegistryKey");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtLockRegistryKey)
        it(KeyHandle);
}

bool monitor::GenericMonitor::register_NtLockVirtualMemory(proc_t proc, const on_NtLockVirtualMemory_fn& on_ntlockvirtualmemory)
{
    const auto ok = setup_func(proc, "NtLockVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtLockVirtualMemory.push_back(on_ntlockvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtLockVirtualMemory()
{
    //LOG(INFO, "Break on NtLockVirtualMemory");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto MapType         = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtLockVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
}

bool monitor::GenericMonitor::register_NtMakePermanentObject(proc_t proc, const on_NtMakePermanentObject_fn& on_ntmakepermanentobject)
{
    const auto ok = setup_func(proc, "NtMakePermanentObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMakePermanentObject.push_back(on_ntmakepermanentobject);
    return true;
}

void monitor::GenericMonitor::on_NtMakePermanentObject()
{
    //LOG(INFO, "Break on NtMakePermanentObject");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtMakePermanentObject)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtMakeTemporaryObject(proc_t proc, const on_NtMakeTemporaryObject_fn& on_ntmaketemporaryobject)
{
    const auto ok = setup_func(proc, "NtMakeTemporaryObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMakeTemporaryObject.push_back(on_ntmaketemporaryobject);
    return true;
}

void monitor::GenericMonitor::on_NtMakeTemporaryObject()
{
    //LOG(INFO, "Break on NtMakeTemporaryObject");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtMakeTemporaryObject)
        it(Handle);
}

bool monitor::GenericMonitor::register_NtMapCMFModule(proc_t proc, const on_NtMapCMFModule_fn& on_ntmapcmfmodule)
{
    const auto ok = setup_func(proc, "NtMapCMFModule");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMapCMFModule.push_back(on_ntmapcmfmodule);
    return true;
}

void monitor::GenericMonitor::on_NtMapCMFModule()
{
    //LOG(INFO, "Break on NtMapCMFModule");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto What            = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Index           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto CacheIndexOut   = nt::cast_to<nt::PULONG>             (args[2]);
    const auto CacheFlagsOut   = nt::cast_to<nt::PULONG>             (args[3]);
    const auto ViewSizeOut     = nt::cast_to<nt::PULONG>             (args[4]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[5]);

    for(const auto& it : d_->observers_NtMapCMFModule)
        it(What, Index, CacheIndexOut, CacheFlagsOut, ViewSizeOut, STARBaseAddress);
}

bool monitor::GenericMonitor::register_NtMapUserPhysicalPages(proc_t proc, const on_NtMapUserPhysicalPages_fn& on_ntmapuserphysicalpages)
{
    const auto ok = setup_func(proc, "NtMapUserPhysicalPages");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMapUserPhysicalPages.push_back(on_ntmapuserphysicalpages);
    return true;
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPages()
{
    //LOG(INFO, "Break on NtMapUserPhysicalPages");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VirtualAddress  = nt::cast_to<nt::PVOID>              (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::ULONG_PTR>          (args[1]);
    const auto UserPfnArra     = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtMapUserPhysicalPages)
        it(VirtualAddress, NumberOfPages, UserPfnArra);
}

bool monitor::GenericMonitor::register_NtMapUserPhysicalPagesScatter(proc_t proc, const on_NtMapUserPhysicalPagesScatter_fn& on_ntmapuserphysicalpagesscatter)
{
    const auto ok = setup_func(proc, "NtMapUserPhysicalPagesScatter");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMapUserPhysicalPagesScatter.push_back(on_ntmapuserphysicalpagesscatter);
    return true;
}

void monitor::GenericMonitor::on_NtMapUserPhysicalPagesScatter()
{
    //LOG(INFO, "Break on NtMapUserPhysicalPagesScatter");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARVirtualAddresses= nt::cast_to<nt::PVOID>              (args[0]);
    const auto NumberOfPages   = nt::cast_to<nt::ULONG_PTR>          (args[1]);
    const auto UserPfnArray    = nt::cast_to<nt::PULONG_PTR>         (args[2]);

    for(const auto& it : d_->observers_NtMapUserPhysicalPagesScatter)
        it(STARVirtualAddresses, NumberOfPages, UserPfnArray);
}

bool monitor::GenericMonitor::register_NtMapViewOfSection(proc_t proc, const on_NtMapViewOfSection_fn& on_ntmapviewofsection)
{
    const auto ok = setup_func(proc, "NtMapViewOfSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtMapViewOfSection.push_back(on_ntmapviewofsection);
    return true;
}

void monitor::GenericMonitor::on_NtMapViewOfSection()
{
    //LOG(INFO, "Break on NtMapViewOfSection");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SectionHandle, ProcessHandle, STARBaseAddress, ZeroBits, CommitSize, SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
}

bool monitor::GenericMonitor::register_NtModifyBootEntry(proc_t proc, const on_NtModifyBootEntry_fn& on_ntmodifybootentry)
{
    const auto ok = setup_func(proc, "NtModifyBootEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtModifyBootEntry.push_back(on_ntmodifybootentry);
    return true;
}

void monitor::GenericMonitor::on_NtModifyBootEntry()
{
    //LOG(INFO, "Break on NtModifyBootEntry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootEntry       = nt::cast_to<nt::PBOOT_ENTRY>        (args[0]);

    for(const auto& it : d_->observers_NtModifyBootEntry)
        it(BootEntry);
}

bool monitor::GenericMonitor::register_NtModifyDriverEntry(proc_t proc, const on_NtModifyDriverEntry_fn& on_ntmodifydriverentry)
{
    const auto ok = setup_func(proc, "NtModifyDriverEntry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtModifyDriverEntry.push_back(on_ntmodifydriverentry);
    return true;
}

void monitor::GenericMonitor::on_NtModifyDriverEntry()
{
    //LOG(INFO, "Break on NtModifyDriverEntry");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverEntry     = nt::cast_to<nt::PEFI_DRIVER_ENTRY>  (args[0]);

    for(const auto& it : d_->observers_NtModifyDriverEntry)
        it(DriverEntry);
}

bool monitor::GenericMonitor::register_NtNotifyChangeDirectoryFile(proc_t proc, const on_NtNotifyChangeDirectoryFile_fn& on_ntnotifychangedirectoryfile)
{
    const auto ok = setup_func(proc, "NtNotifyChangeDirectoryFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtNotifyChangeDirectoryFile.push_back(on_ntnotifychangedirectoryfile);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeDirectoryFile()
{
    //LOG(INFO, "Break on NtNotifyChangeDirectoryFile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, CompletionFilter, WatchTree);
}

bool monitor::GenericMonitor::register_NtNotifyChangeKey(proc_t proc, const on_NtNotifyChangeKey_fn& on_ntnotifychangekey)
{
    const auto ok = setup_func(proc, "NtNotifyChangeKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtNotifyChangeKey.push_back(on_ntnotifychangekey);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeKey()
{
    //LOG(INFO, "Break on NtNotifyChangeKey");
    constexpr int nargs = 10;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(KeyHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

bool monitor::GenericMonitor::register_NtNotifyChangeMultipleKeys(proc_t proc, const on_NtNotifyChangeMultipleKeys_fn& on_ntnotifychangemultiplekeys)
{
    const auto ok = setup_func(proc, "NtNotifyChangeMultipleKeys");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtNotifyChangeMultipleKeys.push_back(on_ntnotifychangemultiplekeys);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeMultipleKeys()
{
    //LOG(INFO, "Break on NtNotifyChangeMultipleKeys");
    constexpr int nargs = 12;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(MasterKeyHandle, Count, SlaveObjects, Event, ApcRoutine, ApcContext, IoStatusBlock, CompletionFilter, WatchTree, Buffer, BufferSize, Asynchronous);
}

bool monitor::GenericMonitor::register_NtNotifyChangeSession(proc_t proc, const on_NtNotifyChangeSession_fn& on_ntnotifychangesession)
{
    const auto ok = setup_func(proc, "NtNotifyChangeSession");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtNotifyChangeSession.push_back(on_ntnotifychangesession);
    return true;
}

void monitor::GenericMonitor::on_NtNotifyChangeSession()
{
    //LOG(INFO, "Break on NtNotifyChangeSession");
    constexpr int nargs = 8;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(Session, IoStateSequence, Reserved, Action, IoState, IoState2, Buffer, BufferSize);
}

bool monitor::GenericMonitor::register_NtOpenDirectoryObject(proc_t proc, const on_NtOpenDirectoryObject_fn& on_ntopendirectoryobject)
{
    const auto ok = setup_func(proc, "NtOpenDirectoryObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenDirectoryObject.push_back(on_ntopendirectoryobject);
    return true;
}

void monitor::GenericMonitor::on_NtOpenDirectoryObject()
{
    //LOG(INFO, "Break on NtOpenDirectoryObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenDirectoryObject)
        it(DirectoryHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEnlistment(proc_t proc, const on_NtOpenEnlistment_fn& on_ntopenenlistment)
{
    const auto ok = setup_func(proc, "NtOpenEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenEnlistment.push_back(on_ntopenenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEnlistment()
{
    //LOG(INFO, "Break on NtOpenEnlistment");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[2]);
    const auto EnlistmentGuid  = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);

    for(const auto& it : d_->observers_NtOpenEnlistment)
        it(EnlistmentHandle, DesiredAccess, ResourceManagerHandle, EnlistmentGuid, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEvent(proc_t proc, const on_NtOpenEvent_fn& on_ntopenevent)
{
    const auto ok = setup_func(proc, "NtOpenEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenEvent.push_back(on_ntopenevent);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEvent()
{
    //LOG(INFO, "Break on NtOpenEvent");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenEvent)
        it(EventHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenEventPair(proc_t proc, const on_NtOpenEventPair_fn& on_ntopeneventpair)
{
    const auto ok = setup_func(proc, "NtOpenEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenEventPair.push_back(on_ntopeneventpair);
    return true;
}

void monitor::GenericMonitor::on_NtOpenEventPair()
{
    //LOG(INFO, "Break on NtOpenEventPair");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenEventPair)
        it(EventPairHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenFile(proc_t proc, const on_NtOpenFile_fn& on_ntopenfile)
{
    const auto ok = setup_func(proc, "NtOpenFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenFile.push_back(on_ntopenfile);
    return true;
}

void monitor::GenericMonitor::on_NtOpenFile()
{
    //LOG(INFO, "Break on NtOpenFile");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto ShareAccess     = nt::cast_to<nt::ULONG>              (args[4]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtOpenFile)
        it(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenIoCompletion(proc_t proc, const on_NtOpenIoCompletion_fn& on_ntopeniocompletion)
{
    const auto ok = setup_func(proc, "NtOpenIoCompletion");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenIoCompletion.push_back(on_ntopeniocompletion);
    return true;
}

void monitor::GenericMonitor::on_NtOpenIoCompletion()
{
    //LOG(INFO, "Break on NtOpenIoCompletion");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenIoCompletion)
        it(IoCompletionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenJobObject(proc_t proc, const on_NtOpenJobObject_fn& on_ntopenjobobject)
{
    const auto ok = setup_func(proc, "NtOpenJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenJobObject.push_back(on_ntopenjobobject);
    return true;
}

void monitor::GenericMonitor::on_NtOpenJobObject()
{
    //LOG(INFO, "Break on NtOpenJobObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenJobObject)
        it(JobHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyedEvent(proc_t proc, const on_NtOpenKeyedEvent_fn& on_ntopenkeyedevent)
{
    const auto ok = setup_func(proc, "NtOpenKeyedEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenKeyedEvent.push_back(on_ntopenkeyedevent);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyedEvent()
{
    //LOG(INFO, "Break on NtOpenKeyedEvent");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenKeyedEvent)
        it(KeyedEventHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyEx(proc_t proc, const on_NtOpenKeyEx_fn& on_ntopenkeyex)
{
    const auto ok = setup_func(proc, "NtOpenKeyEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenKeyEx.push_back(on_ntopenkeyex);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyEx()
{
    //LOG(INFO, "Break on NtOpenKeyEx");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtOpenKeyEx)
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenKey(proc_t proc, const on_NtOpenKey_fn& on_ntopenkey)
{
    const auto ok = setup_func(proc, "NtOpenKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenKey.push_back(on_ntopenkey);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKey()
{
    //LOG(INFO, "Break on NtOpenKey");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenKey)
        it(KeyHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenKeyTransactedEx(proc_t proc, const on_NtOpenKeyTransactedEx_fn& on_ntopenkeytransactedex)
{
    const auto ok = setup_func(proc, "NtOpenKeyTransactedEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenKeyTransactedEx.push_back(on_ntopenkeytransactedex);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyTransactedEx()
{
    //LOG(INFO, "Break on NtOpenKeyTransactedEx");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[3]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[4]);

    for(const auto& it : d_->observers_NtOpenKeyTransactedEx)
        it(KeyHandle, DesiredAccess, ObjectAttributes, OpenOptions, TransactionHandle);
}

bool monitor::GenericMonitor::register_NtOpenKeyTransacted(proc_t proc, const on_NtOpenKeyTransacted_fn& on_ntopenkeytransacted)
{
    const auto ok = setup_func(proc, "NtOpenKeyTransacted");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenKeyTransacted.push_back(on_ntopenkeytransacted);
    return true;
}

void monitor::GenericMonitor::on_NtOpenKeyTransacted()
{
    //LOG(INFO, "Break on NtOpenKeyTransacted");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[3]);

    for(const auto& it : d_->observers_NtOpenKeyTransacted)
        it(KeyHandle, DesiredAccess, ObjectAttributes, TransactionHandle);
}

bool monitor::GenericMonitor::register_NtOpenMutant(proc_t proc, const on_NtOpenMutant_fn& on_ntopenmutant)
{
    const auto ok = setup_func(proc, "NtOpenMutant");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenMutant.push_back(on_ntopenmutant);
    return true;
}

void monitor::GenericMonitor::on_NtOpenMutant()
{
    //LOG(INFO, "Break on NtOpenMutant");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenMutant)
        it(MutantHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenObjectAuditAlarm(proc_t proc, const on_NtOpenObjectAuditAlarm_fn& on_ntopenobjectauditalarm)
{
    const auto ok = setup_func(proc, "NtOpenObjectAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenObjectAuditAlarm.push_back(on_ntopenobjectauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtOpenObjectAuditAlarm()
{
    //LOG(INFO, "Break on NtOpenObjectAuditAlarm");
    constexpr int nargs = 12;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose);
}

bool monitor::GenericMonitor::register_NtOpenPrivateNamespace(proc_t proc, const on_NtOpenPrivateNamespace_fn& on_ntopenprivatenamespace)
{
    const auto ok = setup_func(proc, "NtOpenPrivateNamespace");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenPrivateNamespace.push_back(on_ntopenprivatenamespace);
    return true;
}

void monitor::GenericMonitor::on_NtOpenPrivateNamespace()
{
    //LOG(INFO, "Break on NtOpenPrivateNamespace");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NamespaceHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto BoundaryDescriptor= nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtOpenPrivateNamespace)
        it(NamespaceHandle, DesiredAccess, ObjectAttributes, BoundaryDescriptor);
}

bool monitor::GenericMonitor::register_NtOpenProcess(proc_t proc, const on_NtOpenProcess_fn& on_ntopenprocess)
{
    const auto ok = setup_func(proc, "NtOpenProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenProcess.push_back(on_ntopenprocess);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcess()
{
    //LOG(INFO, "Break on NtOpenProcess");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[3]);

    for(const auto& it : d_->observers_NtOpenProcess)
        it(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

bool monitor::GenericMonitor::register_NtOpenProcessTokenEx(proc_t proc, const on_NtOpenProcessTokenEx_fn& on_ntopenprocesstokenex)
{
    const auto ok = setup_func(proc, "NtOpenProcessTokenEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenProcessTokenEx.push_back(on_ntopenprocesstokenex);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcessTokenEx()
{
    //LOG(INFO, "Break on NtOpenProcessTokenEx");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[3]);

    for(const auto& it : d_->observers_NtOpenProcessTokenEx)
        it(ProcessHandle, DesiredAccess, HandleAttributes, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenProcessToken(proc_t proc, const on_NtOpenProcessToken_fn& on_ntopenprocesstoken)
{
    const auto ok = setup_func(proc, "NtOpenProcessToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenProcessToken.push_back(on_ntopenprocesstoken);
    return true;
}

void monitor::GenericMonitor::on_NtOpenProcessToken()
{
    //LOG(INFO, "Break on NtOpenProcessToken");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[2]);

    for(const auto& it : d_->observers_NtOpenProcessToken)
        it(ProcessHandle, DesiredAccess, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenResourceManager(proc_t proc, const on_NtOpenResourceManager_fn& on_ntopenresourcemanager)
{
    const auto ok = setup_func(proc, "NtOpenResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenResourceManager.push_back(on_ntopenresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtOpenResourceManager()
{
    //LOG(INFO, "Break on NtOpenResourceManager");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto ResourceManagerGuid= nt::cast_to<nt::LPGUID>             (args[3]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[4]);

    for(const auto& it : d_->observers_NtOpenResourceManager)
        it(ResourceManagerHandle, DesiredAccess, TmHandle, ResourceManagerGuid, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSection(proc_t proc, const on_NtOpenSection_fn& on_ntopensection)
{
    const auto ok = setup_func(proc, "NtOpenSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenSection.push_back(on_ntopensection);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSection()
{
    //LOG(INFO, "Break on NtOpenSection");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSection)
        it(SectionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSemaphore(proc_t proc, const on_NtOpenSemaphore_fn& on_ntopensemaphore)
{
    const auto ok = setup_func(proc, "NtOpenSemaphore");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenSemaphore.push_back(on_ntopensemaphore);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSemaphore()
{
    //LOG(INFO, "Break on NtOpenSemaphore");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSemaphore)
        it(SemaphoreHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSession(proc_t proc, const on_NtOpenSession_fn& on_ntopensession)
{
    const auto ok = setup_func(proc, "NtOpenSession");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenSession.push_back(on_ntopensession);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSession()
{
    //LOG(INFO, "Break on NtOpenSession");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SessionHandle   = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSession)
        it(SessionHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenSymbolicLinkObject(proc_t proc, const on_NtOpenSymbolicLinkObject_fn& on_ntopensymboliclinkobject)
{
    const auto ok = setup_func(proc, "NtOpenSymbolicLinkObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenSymbolicLinkObject.push_back(on_ntopensymboliclinkobject);
    return true;
}

void monitor::GenericMonitor::on_NtOpenSymbolicLinkObject()
{
    //LOG(INFO, "Break on NtOpenSymbolicLinkObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenSymbolicLinkObject)
        it(LinkHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenThread(proc_t proc, const on_NtOpenThread_fn& on_ntopenthread)
{
    const auto ok = setup_func(proc, "NtOpenThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenThread.push_back(on_ntopenthread);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThread()
{
    //LOG(INFO, "Break on NtOpenThread");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto ClientId        = nt::cast_to<nt::PCLIENT_ID>         (args[3]);

    for(const auto& it : d_->observers_NtOpenThread)
        it(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);
}

bool monitor::GenericMonitor::register_NtOpenThreadTokenEx(proc_t proc, const on_NtOpenThreadTokenEx_fn& on_ntopenthreadtokenex)
{
    const auto ok = setup_func(proc, "NtOpenThreadTokenEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenThreadTokenEx.push_back(on_ntopenthreadtokenex);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThreadTokenEx()
{
    //LOG(INFO, "Break on NtOpenThreadTokenEx");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto OpenAsSelf      = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto HandleAttributes= nt::cast_to<nt::ULONG>              (args[3]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[4]);

    for(const auto& it : d_->observers_NtOpenThreadTokenEx)
        it(ThreadHandle, DesiredAccess, OpenAsSelf, HandleAttributes, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenThreadToken(proc_t proc, const on_NtOpenThreadToken_fn& on_ntopenthreadtoken)
{
    const auto ok = setup_func(proc, "NtOpenThreadToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenThreadToken.push_back(on_ntopenthreadtoken);
    return true;
}

void monitor::GenericMonitor::on_NtOpenThreadToken()
{
    //LOG(INFO, "Break on NtOpenThreadToken");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto OpenAsSelf      = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto TokenHandle     = nt::cast_to<nt::PHANDLE>            (args[3]);

    for(const auto& it : d_->observers_NtOpenThreadToken)
        it(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle);
}

bool monitor::GenericMonitor::register_NtOpenTimer(proc_t proc, const on_NtOpenTimer_fn& on_ntopentimer)
{
    const auto ok = setup_func(proc, "NtOpenTimer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenTimer.push_back(on_ntopentimer);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTimer()
{
    //LOG(INFO, "Break on NtOpenTimer");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtOpenTimer)
        it(TimerHandle, DesiredAccess, ObjectAttributes);
}

bool monitor::GenericMonitor::register_NtOpenTransactionManager(proc_t proc, const on_NtOpenTransactionManager_fn& on_ntopentransactionmanager)
{
    const auto ok = setup_func(proc, "NtOpenTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenTransactionManager.push_back(on_ntopentransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTransactionManager()
{
    //LOG(INFO, "Break on NtOpenTransactionManager");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[3]);
    const auto TmIdentity      = nt::cast_to<nt::LPGUID>             (args[4]);
    const auto OpenOptions     = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtOpenTransactionManager)
        it(TmHandle, DesiredAccess, ObjectAttributes, LogFileName, TmIdentity, OpenOptions);
}

bool monitor::GenericMonitor::register_NtOpenTransaction(proc_t proc, const on_NtOpenTransaction_fn& on_ntopentransaction)
{
    const auto ok = setup_func(proc, "NtOpenTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtOpenTransaction.push_back(on_ntopentransaction);
    return true;
}

void monitor::GenericMonitor::on_NtOpenTransaction()
{
    //LOG(INFO, "Break on NtOpenTransaction");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::PHANDLE>            (args[0]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[1]);
    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);
    const auto Uow             = nt::cast_to<nt::LPGUID>             (args[3]);
    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[4]);

    for(const auto& it : d_->observers_NtOpenTransaction)
        it(TransactionHandle, DesiredAccess, ObjectAttributes, Uow, TmHandle);
}

bool monitor::GenericMonitor::register_NtPlugPlayControl(proc_t proc, const on_NtPlugPlayControl_fn& on_ntplugplaycontrol)
{
    const auto ok = setup_func(proc, "NtPlugPlayControl");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPlugPlayControl.push_back(on_ntplugplaycontrol);
    return true;
}

void monitor::GenericMonitor::on_NtPlugPlayControl()
{
    //LOG(INFO, "Break on NtPlugPlayControl");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PnPControlClass = nt::cast_to<nt::PLUGPLAY_CONTROL_CLASS>(args[0]);
    const auto PnPControlData  = nt::cast_to<nt::PVOID>              (args[1]);
    const auto PnPControlDataLength= nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtPlugPlayControl)
        it(PnPControlClass, PnPControlData, PnPControlDataLength);
}

bool monitor::GenericMonitor::register_NtPowerInformation(proc_t proc, const on_NtPowerInformation_fn& on_ntpowerinformation)
{
    const auto ok = setup_func(proc, "NtPowerInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPowerInformation.push_back(on_ntpowerinformation);
    return true;
}

void monitor::GenericMonitor::on_NtPowerInformation()
{
    //LOG(INFO, "Break on NtPowerInformation");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InformationLevel= nt::cast_to<nt::POWER_INFORMATION_LEVEL>(args[0]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtPowerInformation)
        it(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

bool monitor::GenericMonitor::register_NtPrepareComplete(proc_t proc, const on_NtPrepareComplete_fn& on_ntpreparecomplete)
{
    const auto ok = setup_func(proc, "NtPrepareComplete");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrepareComplete.push_back(on_ntpreparecomplete);
    return true;
}

void monitor::GenericMonitor::on_NtPrepareComplete()
{
    //LOG(INFO, "Break on NtPrepareComplete");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrepareComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrepareEnlistment(proc_t proc, const on_NtPrepareEnlistment_fn& on_ntprepareenlistment)
{
    const auto ok = setup_func(proc, "NtPrepareEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrepareEnlistment.push_back(on_ntprepareenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtPrepareEnlistment()
{
    //LOG(INFO, "Break on NtPrepareEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrepareEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrePrepareComplete(proc_t proc, const on_NtPrePrepareComplete_fn& on_ntprepreparecomplete)
{
    const auto ok = setup_func(proc, "NtPrePrepareComplete");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrePrepareComplete.push_back(on_ntprepreparecomplete);
    return true;
}

void monitor::GenericMonitor::on_NtPrePrepareComplete()
{
    //LOG(INFO, "Break on NtPrePrepareComplete");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrePrepareComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrePrepareEnlistment(proc_t proc, const on_NtPrePrepareEnlistment_fn& on_ntpreprepareenlistment)
{
    const auto ok = setup_func(proc, "NtPrePrepareEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrePrepareEnlistment.push_back(on_ntpreprepareenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtPrePrepareEnlistment()
{
    //LOG(INFO, "Break on NtPrePrepareEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtPrePrepareEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtPrivilegeCheck(proc_t proc, const on_NtPrivilegeCheck_fn& on_ntprivilegecheck)
{
    const auto ok = setup_func(proc, "NtPrivilegeCheck");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrivilegeCheck.push_back(on_ntprivilegecheck);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegeCheck()
{
    //LOG(INFO, "Break on NtPrivilegeCheck");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequiredPrivileges= nt::cast_to<nt::PPRIVILEGE_SET>     (args[1]);
    const auto Result          = nt::cast_to<nt::PBOOLEAN>           (args[2]);

    for(const auto& it : d_->observers_NtPrivilegeCheck)
        it(ClientToken, RequiredPrivileges, Result);
}

bool monitor::GenericMonitor::register_NtPrivilegedServiceAuditAlarm(proc_t proc, const on_NtPrivilegedServiceAuditAlarm_fn& on_ntprivilegedserviceauditalarm)
{
    const auto ok = setup_func(proc, "NtPrivilegedServiceAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrivilegedServiceAuditAlarm.push_back(on_ntprivilegedserviceauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegedServiceAuditAlarm()
{
    //LOG(INFO, "Break on NtPrivilegedServiceAuditAlarm");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto ServiceName     = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto Privileges      = nt::cast_to<nt::PPRIVILEGE_SET>     (args[3]);
    const auto AccessGranted   = nt::cast_to<nt::BOOLEAN>            (args[4]);

    for(const auto& it : d_->observers_NtPrivilegedServiceAuditAlarm)
        it(SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted);
}

bool monitor::GenericMonitor::register_NtPrivilegeObjectAuditAlarm(proc_t proc, const on_NtPrivilegeObjectAuditAlarm_fn& on_ntprivilegeobjectauditalarm)
{
    const auto ok = setup_func(proc, "NtPrivilegeObjectAuditAlarm");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPrivilegeObjectAuditAlarm.push_back(on_ntprivilegeobjectauditalarm);
    return true;
}

void monitor::GenericMonitor::on_NtPrivilegeObjectAuditAlarm()
{
    //LOG(INFO, "Break on NtPrivilegeObjectAuditAlarm");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SubsystemName   = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto HandleId        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ClientToken     = nt::cast_to<nt::HANDLE>             (args[2]);
    const auto DesiredAccess   = nt::cast_to<nt::ACCESS_MASK>        (args[3]);
    const auto Privileges      = nt::cast_to<nt::PPRIVILEGE_SET>     (args[4]);
    const auto AccessGranted   = nt::cast_to<nt::BOOLEAN>            (args[5]);

    for(const auto& it : d_->observers_NtPrivilegeObjectAuditAlarm)
        it(SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted);
}

bool monitor::GenericMonitor::register_NtPropagationComplete(proc_t proc, const on_NtPropagationComplete_fn& on_ntpropagationcomplete)
{
    const auto ok = setup_func(proc, "NtPropagationComplete");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPropagationComplete.push_back(on_ntpropagationcomplete);
    return true;
}

void monitor::GenericMonitor::on_NtPropagationComplete()
{
    //LOG(INFO, "Break on NtPropagationComplete");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestCookie   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtPropagationComplete)
        it(ResourceManagerHandle, RequestCookie, BufferLength, Buffer);
}

bool monitor::GenericMonitor::register_NtPropagationFailed(proc_t proc, const on_NtPropagationFailed_fn& on_ntpropagationfailed)
{
    const auto ok = setup_func(proc, "NtPropagationFailed");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPropagationFailed.push_back(on_ntpropagationfailed);
    return true;
}

void monitor::GenericMonitor::on_NtPropagationFailed()
{
    //LOG(INFO, "Break on NtPropagationFailed");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestCookie   = nt::cast_to<nt::ULONG>              (args[1]);
    const auto PropStatus      = nt::cast_to<nt::NTSTATUS>           (args[2]);

    for(const auto& it : d_->observers_NtPropagationFailed)
        it(ResourceManagerHandle, RequestCookie, PropStatus);
}

bool monitor::GenericMonitor::register_NtProtectVirtualMemory(proc_t proc, const on_NtProtectVirtualMemory_fn& on_ntprotectvirtualmemory)
{
    const auto ok = setup_func(proc, "NtProtectVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtProtectVirtualMemory.push_back(on_ntprotectvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtProtectVirtualMemory()
{
    //LOG(INFO, "Break on NtProtectVirtualMemory");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto NewProtectWin32 = nt::cast_to<nt::WIN32_PROTECTION_MASK>(args[3]);
    const auto OldProtect      = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtProtectVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, NewProtectWin32, OldProtect);
}

bool monitor::GenericMonitor::register_NtPulseEvent(proc_t proc, const on_NtPulseEvent_fn& on_ntpulseevent)
{
    const auto ok = setup_func(proc, "NtPulseEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtPulseEvent.push_back(on_ntpulseevent);
    return true;
}

void monitor::GenericMonitor::on_NtPulseEvent()
{
    //LOG(INFO, "Break on NtPulseEvent");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtPulseEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtQueryAttributesFile(proc_t proc, const on_NtQueryAttributesFile_fn& on_ntqueryattributesfile)
{
    const auto ok = setup_func(proc, "NtQueryAttributesFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryAttributesFile.push_back(on_ntqueryattributesfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryAttributesFile()
{
    //LOG(INFO, "Break on NtQueryAttributesFile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto FileInformation = nt::cast_to<nt::PFILE_BASIC_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtQueryAttributesFile)
        it(ObjectAttributes, FileInformation);
}

bool monitor::GenericMonitor::register_NtQueryBootEntryOrder(proc_t proc, const on_NtQueryBootEntryOrder_fn& on_ntquerybootentryorder)
{
    const auto ok = setup_func(proc, "NtQueryBootEntryOrder");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryBootEntryOrder.push_back(on_ntquerybootentryorder);
    return true;
}

void monitor::GenericMonitor::on_NtQueryBootEntryOrder()
{
    //LOG(INFO, "Break on NtQueryBootEntryOrder");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryBootEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtQueryBootOptions(proc_t proc, const on_NtQueryBootOptions_fn& on_ntquerybootoptions)
{
    const auto ok = setup_func(proc, "NtQueryBootOptions");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryBootOptions.push_back(on_ntquerybootoptions);
    return true;
}

void monitor::GenericMonitor::on_NtQueryBootOptions()
{
    //LOG(INFO, "Break on NtQueryBootOptions");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootOptions     = nt::cast_to<nt::PBOOT_OPTIONS>      (args[0]);
    const auto BootOptionsLength= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryBootOptions)
        it(BootOptions, BootOptionsLength);
}

bool monitor::GenericMonitor::register_NtQueryDebugFilterState(proc_t proc, const on_NtQueryDebugFilterState_fn& on_ntquerydebugfilterstate)
{
    const auto ok = setup_func(proc, "NtQueryDebugFilterState");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDebugFilterState.push_back(on_ntquerydebugfilterstate);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDebugFilterState()
{
    //LOG(INFO, "Break on NtQueryDebugFilterState");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ComponentId     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Level           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtQueryDebugFilterState)
        it(ComponentId, Level);
}

bool monitor::GenericMonitor::register_NtQueryDefaultLocale(proc_t proc, const on_NtQueryDefaultLocale_fn& on_ntquerydefaultlocale)
{
    const auto ok = setup_func(proc, "NtQueryDefaultLocale");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDefaultLocale.push_back(on_ntquerydefaultlocale);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDefaultLocale()
{
    //LOG(INFO, "Break on NtQueryDefaultLocale");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto UserProfile     = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::PLCID>              (args[1]);

    for(const auto& it : d_->observers_NtQueryDefaultLocale)
        it(UserProfile, DefaultLocaleId);
}

bool monitor::GenericMonitor::register_NtQueryDefaultUILanguage(proc_t proc, const on_NtQueryDefaultUILanguage_fn& on_ntquerydefaultuilanguage)
{
    const auto ok = setup_func(proc, "NtQueryDefaultUILanguage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDefaultUILanguage.push_back(on_ntquerydefaultuilanguage);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDefaultUILanguage()
{
    //LOG(INFO, "Break on NtQueryDefaultUILanguage");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARDefaultUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtQueryDefaultUILanguage)
        it(STARDefaultUILanguageId);
}

bool monitor::GenericMonitor::register_NtQueryDirectoryFile(proc_t proc, const on_NtQueryDirectoryFile_fn& on_ntquerydirectoryfile)
{
    const auto ok = setup_func(proc, "NtQueryDirectoryFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDirectoryFile.push_back(on_ntquerydirectoryfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDirectoryFile()
{
    //LOG(INFO, "Break on NtQueryDirectoryFile");
    constexpr int nargs = 11;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FileInformation, Length, FileInformationClass, ReturnSingleEntry, FileName, RestartScan);
}

bool monitor::GenericMonitor::register_NtQueryDirectoryObject(proc_t proc, const on_NtQueryDirectoryObject_fn& on_ntquerydirectoryobject)
{
    const auto ok = setup_func(proc, "NtQueryDirectoryObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDirectoryObject.push_back(on_ntquerydirectoryobject);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDirectoryObject()
{
    //LOG(INFO, "Break on NtQueryDirectoryObject");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DirectoryHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[2]);
    const auto ReturnSingleEntry= nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto RestartScan     = nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto Context         = nt::cast_to<nt::PULONG>             (args[5]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[6]);

    for(const auto& it : d_->observers_NtQueryDirectoryObject)
        it(DirectoryHandle, Buffer, Length, ReturnSingleEntry, RestartScan, Context, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryDriverEntryOrder(proc_t proc, const on_NtQueryDriverEntryOrder_fn& on_ntquerydriverentryorder)
{
    const auto ok = setup_func(proc, "NtQueryDriverEntryOrder");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryDriverEntryOrder.push_back(on_ntquerydriverentryorder);
    return true;
}

void monitor::GenericMonitor::on_NtQueryDriverEntryOrder()
{
    //LOG(INFO, "Break on NtQueryDriverEntryOrder");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryDriverEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtQueryEaFile(proc_t proc, const on_NtQueryEaFile_fn& on_ntqueryeafile)
{
    const auto ok = setup_func(proc, "NtQueryEaFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryEaFile.push_back(on_ntqueryeafile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryEaFile()
{
    //LOG(INFO, "Break on NtQueryEaFile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
}

bool monitor::GenericMonitor::register_NtQueryEvent(proc_t proc, const on_NtQueryEvent_fn& on_ntqueryevent)
{
    const auto ok = setup_func(proc, "NtQueryEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryEvent.push_back(on_ntqueryevent);
    return true;
}

void monitor::GenericMonitor::on_NtQueryEvent()
{
    //LOG(INFO, "Break on NtQueryEvent");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EventInformationClass= nt::cast_to<nt::EVENT_INFORMATION_CLASS>(args[1]);
    const auto EventInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EventInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryEvent)
        it(EventHandle, EventInformationClass, EventInformation, EventInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryFullAttributesFile(proc_t proc, const on_NtQueryFullAttributesFile_fn& on_ntqueryfullattributesfile)
{
    const auto ok = setup_func(proc, "NtQueryFullAttributesFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryFullAttributesFile.push_back(on_ntqueryfullattributesfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryFullAttributesFile()
{
    //LOG(INFO, "Break on NtQueryFullAttributesFile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ObjectAttributes= nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto FileInformation = nt::cast_to<nt::PFILE_NETWORK_OPEN_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtQueryFullAttributesFile)
        it(ObjectAttributes, FileInformation);
}

bool monitor::GenericMonitor::register_NtQueryInformationAtom(proc_t proc, const on_NtQueryInformationAtom_fn& on_ntqueryinformationatom)
{
    const auto ok = setup_func(proc, "NtQueryInformationAtom");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationAtom.push_back(on_ntqueryinformationatom);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationAtom()
{
    //LOG(INFO, "Break on NtQueryInformationAtom");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Atom            = nt::cast_to<nt::RTL_ATOM>           (args[0]);
    const auto InformationClass= nt::cast_to<nt::ATOM_INFORMATION_CLASS>(args[1]);
    const auto AtomInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto AtomInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationAtom)
        it(Atom, InformationClass, AtomInformation, AtomInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationEnlistment(proc_t proc, const on_NtQueryInformationEnlistment_fn& on_ntqueryinformationenlistment)
{
    const auto ok = setup_func(proc, "NtQueryInformationEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationEnlistment.push_back(on_ntqueryinformationenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationEnlistment()
{
    //LOG(INFO, "Break on NtQueryInformationEnlistment");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentInformationClass= nt::cast_to<nt::ENLISTMENT_INFORMATION_CLASS>(args[1]);
    const auto EnlistmentInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EnlistmentInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationEnlistment)
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationFile(proc_t proc, const on_NtQueryInformationFile_fn& on_ntqueryinformationfile)
{
    const auto ok = setup_func(proc, "NtQueryInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationFile.push_back(on_ntqueryinformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationFile()
{
    //LOG(INFO, "Break on NtQueryInformationFile");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FileInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FileInformationClass= nt::cast_to<nt::FILE_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtQueryInformationFile)
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

bool monitor::GenericMonitor::register_NtQueryInformationJobObject(proc_t proc, const on_NtQueryInformationJobObject_fn& on_ntqueryinformationjobobject)
{
    const auto ok = setup_func(proc, "NtQueryInformationJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationJobObject.push_back(on_ntqueryinformationjobobject);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationJobObject()
{
    //LOG(INFO, "Break on NtQueryInformationJobObject");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobObjectInformationClass= nt::cast_to<nt::JOBOBJECTINFOCLASS> (args[1]);
    const auto JobObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto JobObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationJobObject)
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationPort(proc_t proc, const on_NtQueryInformationPort_fn& on_ntqueryinformationport)
{
    const auto ok = setup_func(proc, "NtQueryInformationPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationPort.push_back(on_ntqueryinformationport);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationPort()
{
    //LOG(INFO, "Break on NtQueryInformationPort");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PortInformationClass= nt::cast_to<nt::PORT_INFORMATION_CLASS>(args[1]);
    const auto PortInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationPort)
        it(PortHandle, PortInformationClass, PortInformation, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationProcess(proc_t proc, const on_NtQueryInformationProcess_fn& on_ntqueryinformationprocess)
{
    const auto ok = setup_func(proc, "NtQueryInformationProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationProcess.push_back(on_ntqueryinformationprocess);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationProcess()
{
    //LOG(INFO, "Break on NtQueryInformationProcess");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessInformationClass= nt::cast_to<nt::PROCESSINFOCLASS>   (args[1]);
    const auto ProcessInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ProcessInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationProcess)
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationResourceManager(proc_t proc, const on_NtQueryInformationResourceManager_fn& on_ntqueryinformationresourcemanager)
{
    const auto ok = setup_func(proc, "NtQueryInformationResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationResourceManager.push_back(on_ntqueryinformationresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationResourceManager()
{
    //LOG(INFO, "Break on NtQueryInformationResourceManager");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResourceManagerInformationClass= nt::cast_to<nt::RESOURCEMANAGER_INFORMATION_CLASS>(args[1]);
    const auto ResourceManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ResourceManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationResourceManager)
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationThread(proc_t proc, const on_NtQueryInformationThread_fn& on_ntqueryinformationthread)
{
    const auto ok = setup_func(proc, "NtQueryInformationThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationThread.push_back(on_ntqueryinformationthread);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationThread()
{
    //LOG(INFO, "Break on NtQueryInformationThread");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadInformationClass= nt::cast_to<nt::THREADINFOCLASS>    (args[1]);
    const auto ThreadInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ThreadInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationThread)
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationToken(proc_t proc, const on_NtQueryInformationToken_fn& on_ntqueryinformationtoken)
{
    const auto ok = setup_func(proc, "NtQueryInformationToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationToken.push_back(on_ntqueryinformationtoken);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationToken()
{
    //LOG(INFO, "Break on NtQueryInformationToken");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TokenInformationClass= nt::cast_to<nt::TOKEN_INFORMATION_CLASS>(args[1]);
    const auto TokenInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TokenInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationToken)
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationTransaction(proc_t proc, const on_NtQueryInformationTransaction_fn& on_ntqueryinformationtransaction)
{
    const auto ok = setup_func(proc, "NtQueryInformationTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationTransaction.push_back(on_ntqueryinformationtransaction);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationTransaction()
{
    //LOG(INFO, "Break on NtQueryInformationTransaction");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionInformationClass= nt::cast_to<nt::TRANSACTION_INFORMATION_CLASS>(args[1]);
    const auto TransactionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationTransaction)
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationTransactionManager(proc_t proc, const on_NtQueryInformationTransactionManager_fn& on_ntqueryinformationtransactionmanager)
{
    const auto ok = setup_func(proc, "NtQueryInformationTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationTransactionManager.push_back(on_ntqueryinformationtransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationTransactionManager()
{
    //LOG(INFO, "Break on NtQueryInformationTransactionManager");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionManagerInformationClass= nt::cast_to<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(args[1]);
    const auto TransactionManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationTransactionManager)
        it(TransactionManagerHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInformationWorkerFactory(proc_t proc, const on_NtQueryInformationWorkerFactory_fn& on_ntqueryinformationworkerfactory)
{
    const auto ok = setup_func(proc, "NtQueryInformationWorkerFactory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInformationWorkerFactory.push_back(on_ntqueryinformationworkerfactory);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInformationWorkerFactory()
{
    //LOG(INFO, "Break on NtQueryInformationWorkerFactory");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WorkerFactoryInformationClass= nt::cast_to<nt::WORKERFACTORYINFOCLASS>(args[1]);
    const auto WorkerFactoryInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto WorkerFactoryInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryInformationWorkerFactory)
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryInstallUILanguage(proc_t proc, const on_NtQueryInstallUILanguage_fn& on_ntqueryinstalluilanguage)
{
    const auto ok = setup_func(proc, "NtQueryInstallUILanguage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryInstallUILanguage.push_back(on_ntqueryinstalluilanguage);
    return true;
}

void monitor::GenericMonitor::on_NtQueryInstallUILanguage()
{
    //LOG(INFO, "Break on NtQueryInstallUILanguage");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto STARInstallUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtQueryInstallUILanguage)
        it(STARInstallUILanguageId);
}

bool monitor::GenericMonitor::register_NtQueryIntervalProfile(proc_t proc, const on_NtQueryIntervalProfile_fn& on_ntqueryintervalprofile)
{
    const auto ok = setup_func(proc, "NtQueryIntervalProfile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryIntervalProfile.push_back(on_ntqueryintervalprofile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryIntervalProfile()
{
    //LOG(INFO, "Break on NtQueryIntervalProfile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileSource   = nt::cast_to<nt::KPROFILE_SOURCE>    (args[0]);
    const auto Interval        = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryIntervalProfile)
        it(ProfileSource, Interval);
}

bool monitor::GenericMonitor::register_NtQueryIoCompletion(proc_t proc, const on_NtQueryIoCompletion_fn& on_ntqueryiocompletion)
{
    const auto ok = setup_func(proc, "NtQueryIoCompletion");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryIoCompletion.push_back(on_ntqueryiocompletion);
    return true;
}

void monitor::GenericMonitor::on_NtQueryIoCompletion()
{
    //LOG(INFO, "Break on NtQueryIoCompletion");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionInformationClass= nt::cast_to<nt::IO_COMPLETION_INFORMATION_CLASS>(args[1]);
    const auto IoCompletionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoCompletionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryIoCompletion)
        it(IoCompletionHandle, IoCompletionInformationClass, IoCompletionInformation, IoCompletionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryKey(proc_t proc, const on_NtQueryKey_fn& on_ntquerykey)
{
    const auto ok = setup_func(proc, "NtQueryKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryKey.push_back(on_ntquerykey);
    return true;
}

void monitor::GenericMonitor::on_NtQueryKey()
{
    //LOG(INFO, "Break on NtQueryKey");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyInformationClass= nt::cast_to<nt::KEY_INFORMATION_CLASS>(args[1]);
    const auto KeyInformation  = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryKey)
        it(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtQueryLicenseValue(proc_t proc, const on_NtQueryLicenseValue_fn& on_ntquerylicensevalue)
{
    const auto ok = setup_func(proc, "NtQueryLicenseValue");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryLicenseValue.push_back(on_ntquerylicensevalue);
    return true;
}

void monitor::GenericMonitor::on_NtQueryLicenseValue()
{
    //LOG(INFO, "Break on NtQueryLicenseValue");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Name            = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto Type            = nt::cast_to<nt::PULONG>             (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnedLength  = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryLicenseValue)
        it(Name, Type, Buffer, Length, ReturnedLength);
}

bool monitor::GenericMonitor::register_NtQueryMultipleValueKey(proc_t proc, const on_NtQueryMultipleValueKey_fn& on_ntquerymultiplevaluekey)
{
    const auto ok = setup_func(proc, "NtQueryMultipleValueKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryMultipleValueKey.push_back(on_ntquerymultiplevaluekey);
    return true;
}

void monitor::GenericMonitor::on_NtQueryMultipleValueKey()
{
    //LOG(INFO, "Break on NtQueryMultipleValueKey");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueEntries    = nt::cast_to<nt::PKEY_VALUE_ENTRY>   (args[1]);
    const auto EntryCount      = nt::cast_to<nt::ULONG>              (args[2]);
    const auto ValueBuffer     = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferLength    = nt::cast_to<nt::PULONG>             (args[4]);
    const auto RequiredBufferLength= nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQueryMultipleValueKey)
        it(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
}

bool monitor::GenericMonitor::register_NtQueryMutant(proc_t proc, const on_NtQueryMutant_fn& on_ntquerymutant)
{
    const auto ok = setup_func(proc, "NtQueryMutant");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryMutant.push_back(on_ntquerymutant);
    return true;
}

void monitor::GenericMonitor::on_NtQueryMutant()
{
    //LOG(INFO, "Break on NtQueryMutant");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto MutantInformationClass= nt::cast_to<nt::MUTANT_INFORMATION_CLASS>(args[1]);
    const auto MutantInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto MutantInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryMutant)
        it(MutantHandle, MutantInformationClass, MutantInformation, MutantInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryObject(proc_t proc, const on_NtQueryObject_fn& on_ntqueryobject)
{
    const auto ok = setup_func(proc, "NtQueryObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryObject.push_back(on_ntqueryobject);
    return true;
}

void monitor::GenericMonitor::on_NtQueryObject()
{
    //LOG(INFO, "Break on NtQueryObject");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ObjectInformationClass= nt::cast_to<nt::OBJECT_INFORMATION_CLASS>(args[1]);
    const auto ObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryObject)
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryOpenSubKeysEx(proc_t proc, const on_NtQueryOpenSubKeysEx_fn& on_ntqueryopensubkeysex)
{
    const auto ok = setup_func(proc, "NtQueryOpenSubKeysEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryOpenSubKeysEx.push_back(on_ntqueryopensubkeysex);
    return true;
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeysEx()
{
    //LOG(INFO, "Break on NtQueryOpenSubKeysEx");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto BufferLength    = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto RequiredSize    = nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtQueryOpenSubKeysEx)
        it(TargetKey, BufferLength, Buffer, RequiredSize);
}

bool monitor::GenericMonitor::register_NtQueryOpenSubKeys(proc_t proc, const on_NtQueryOpenSubKeys_fn& on_ntqueryopensubkeys)
{
    const auto ok = setup_func(proc, "NtQueryOpenSubKeys");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryOpenSubKeys.push_back(on_ntqueryopensubkeys);
    return true;
}

void monitor::GenericMonitor::on_NtQueryOpenSubKeys()
{
    //LOG(INFO, "Break on NtQueryOpenSubKeys");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto HandleCount     = nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtQueryOpenSubKeys)
        it(TargetKey, HandleCount);
}

bool monitor::GenericMonitor::register_NtQueryPerformanceCounter(proc_t proc, const on_NtQueryPerformanceCounter_fn& on_ntqueryperformancecounter)
{
    const auto ok = setup_func(proc, "NtQueryPerformanceCounter");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryPerformanceCounter.push_back(on_ntqueryperformancecounter);
    return true;
}

void monitor::GenericMonitor::on_NtQueryPerformanceCounter()
{
    //LOG(INFO, "Break on NtQueryPerformanceCounter");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PerformanceCounter= nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto PerformanceFrequency= nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtQueryPerformanceCounter)
        it(PerformanceCounter, PerformanceFrequency);
}

bool monitor::GenericMonitor::register_NtQueryQuotaInformationFile(proc_t proc, const on_NtQueryQuotaInformationFile_fn& on_ntqueryquotainformationfile)
{
    const auto ok = setup_func(proc, "NtQueryQuotaInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryQuotaInformationFile.push_back(on_ntqueryquotainformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryQuotaInformationFile()
{
    //LOG(INFO, "Break on NtQueryQuotaInformationFile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, SidList, SidListLength, StartSid, RestartScan);
}

bool monitor::GenericMonitor::register_NtQuerySection(proc_t proc, const on_NtQuerySection_fn& on_ntquerysection)
{
    const auto ok = setup_func(proc, "NtQuerySection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySection.push_back(on_ntquerysection);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySection()
{
    //LOG(INFO, "Break on NtQuerySection");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SectionHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SectionInformationClass= nt::cast_to<nt::SECTION_INFORMATION_CLASS>(args[1]);
    const auto SectionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto SectionInformationLength= nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtQuerySection)
        it(SectionHandle, SectionInformationClass, SectionInformation, SectionInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySecurityAttributesToken(proc_t proc, const on_NtQuerySecurityAttributesToken_fn& on_ntquerysecurityattributestoken)
{
    const auto ok = setup_func(proc, "NtQuerySecurityAttributesToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySecurityAttributesToken.push_back(on_ntquerysecurityattributestoken);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySecurityAttributesToken()
{
    //LOG(INFO, "Break on NtQuerySecurityAttributesToken");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Attributes      = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto NumberOfAttributes= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQuerySecurityAttributesToken)
        it(TokenHandle, Attributes, NumberOfAttributes, Buffer, Length, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySecurityObject(proc_t proc, const on_NtQuerySecurityObject_fn& on_ntquerysecurityobject)
{
    const auto ok = setup_func(proc, "NtQuerySecurityObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySecurityObject.push_back(on_ntquerysecurityobject);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySecurityObject()
{
    //LOG(INFO, "Break on NtQuerySecurityObject");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecurityInformation= nt::cast_to<nt::SECURITY_INFORMATION>(args[1]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto LengthNeeded    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySecurityObject)
        it(Handle, SecurityInformation, SecurityDescriptor, Length, LengthNeeded);
}

bool monitor::GenericMonitor::register_NtQuerySemaphore(proc_t proc, const on_NtQuerySemaphore_fn& on_ntquerysemaphore)
{
    const auto ok = setup_func(proc, "NtQuerySemaphore");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySemaphore.push_back(on_ntquerysemaphore);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySemaphore()
{
    //LOG(INFO, "Break on NtQuerySemaphore");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SemaphoreInformationClass= nt::cast_to<nt::SEMAPHORE_INFORMATION_CLASS>(args[1]);
    const auto SemaphoreInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto SemaphoreInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySemaphore)
        it(SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, SemaphoreInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySymbolicLinkObject(proc_t proc, const on_NtQuerySymbolicLinkObject_fn& on_ntquerysymboliclinkobject)
{
    const auto ok = setup_func(proc, "NtQuerySymbolicLinkObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySymbolicLinkObject.push_back(on_ntquerysymboliclinkobject);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySymbolicLinkObject()
{
    //LOG(INFO, "Break on NtQuerySymbolicLinkObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LinkHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto LinkTarget      = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto ReturnedLength  = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtQuerySymbolicLinkObject)
        it(LinkHandle, LinkTarget, ReturnedLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemEnvironmentValueEx(proc_t proc, const on_NtQuerySystemEnvironmentValueEx_fn& on_ntquerysystemenvironmentvalueex)
{
    const auto ok = setup_func(proc, "NtQuerySystemEnvironmentValueEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySystemEnvironmentValueEx.push_back(on_ntquerysystemenvironmentvalueex);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValueEx()
{
    //LOG(INFO, "Break on NtQuerySystemEnvironmentValueEx");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VendorGuid      = nt::cast_to<nt::LPGUID>             (args[1]);
    const auto Value           = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ValueLength     = nt::cast_to<nt::PULONG>             (args[3]);
    const auto Attributes      = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValueEx)
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
}

bool monitor::GenericMonitor::register_NtQuerySystemEnvironmentValue(proc_t proc, const on_NtQuerySystemEnvironmentValue_fn& on_ntquerysystemenvironmentvalue)
{
    const auto ok = setup_func(proc, "NtQuerySystemEnvironmentValue");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySystemEnvironmentValue.push_back(on_ntquerysystemenvironmentvalue);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemEnvironmentValue()
{
    //LOG(INFO, "Break on NtQuerySystemEnvironmentValue");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VariableValue   = nt::cast_to<nt::PWSTR>              (args[1]);
    const auto ValueLength     = nt::cast_to<nt::USHORT>             (args[2]);
    const auto ReturnLength    = nt::cast_to<nt::PUSHORT>            (args[3]);

    for(const auto& it : d_->observers_NtQuerySystemEnvironmentValue)
        it(VariableName, VariableValue, ValueLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemInformationEx(proc_t proc, const on_NtQuerySystemInformationEx_fn& on_ntquerysysteminformationex)
{
    const auto ok = setup_func(proc, "NtQuerySystemInformationEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySystemInformationEx.push_back(on_ntquerysysteminformationex);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemInformationEx()
{
    //LOG(INFO, "Break on NtQuerySystemInformationEx");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto QueryInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto QueryInformationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQuerySystemInformationEx)
        it(SystemInformationClass, QueryInformation, QueryInformationLength, SystemInformation, SystemInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemInformation(proc_t proc, const on_NtQuerySystemInformation_fn& on_ntquerysysteminformation)
{
    const auto ok = setup_func(proc, "NtQuerySystemInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySystemInformation.push_back(on_ntquerysysteminformation);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemInformation()
{
    //LOG(INFO, "Break on NtQuerySystemInformation");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtQuerySystemInformation)
        it(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQuerySystemTime(proc_t proc, const on_NtQuerySystemTime_fn& on_ntquerysystemtime)
{
    const auto ok = setup_func(proc, "NtQuerySystemTime");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQuerySystemTime.push_back(on_ntquerysystemtime);
    return true;
}

void monitor::GenericMonitor::on_NtQuerySystemTime()
{
    //LOG(INFO, "Break on NtQuerySystemTime");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemTime      = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);

    for(const auto& it : d_->observers_NtQuerySystemTime)
        it(SystemTime);
}

bool monitor::GenericMonitor::register_NtQueryTimer(proc_t proc, const on_NtQueryTimer_fn& on_ntquerytimer)
{
    const auto ok = setup_func(proc, "NtQueryTimer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryTimer.push_back(on_ntquerytimer);
    return true;
}

void monitor::GenericMonitor::on_NtQueryTimer()
{
    //LOG(INFO, "Break on NtQueryTimer");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TimerInformationClass= nt::cast_to<nt::TIMER_INFORMATION_CLASS>(args[1]);
    const auto TimerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TimerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtQueryTimer)
        it(TimerHandle, TimerInformationClass, TimerInformation, TimerInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryTimerResolution(proc_t proc, const on_NtQueryTimerResolution_fn& on_ntquerytimerresolution)
{
    const auto ok = setup_func(proc, "NtQueryTimerResolution");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryTimerResolution.push_back(on_ntquerytimerresolution);
    return true;
}

void monitor::GenericMonitor::on_NtQueryTimerResolution()
{
    //LOG(INFO, "Break on NtQueryTimerResolution");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MaximumTime     = nt::cast_to<nt::PULONG>             (args[0]);
    const auto MinimumTime     = nt::cast_to<nt::PULONG>             (args[1]);
    const auto CurrentTime     = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtQueryTimerResolution)
        it(MaximumTime, MinimumTime, CurrentTime);
}

bool monitor::GenericMonitor::register_NtQueryValueKey(proc_t proc, const on_NtQueryValueKey_fn& on_ntqueryvaluekey)
{
    const auto ok = setup_func(proc, "NtQueryValueKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryValueKey.push_back(on_ntqueryvaluekey);
    return true;
}

void monitor::GenericMonitor::on_NtQueryValueKey()
{
    //LOG(INFO, "Break on NtQueryValueKey");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto KeyValueInformationClass= nt::cast_to<nt::KEY_VALUE_INFORMATION_CLASS>(args[2]);
    const auto KeyValueInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ResultLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtQueryValueKey)
        it(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

bool monitor::GenericMonitor::register_NtQueryVirtualMemory(proc_t proc, const on_NtQueryVirtualMemory_fn& on_ntqueryvirtualmemory)
{
    const auto ok = setup_func(proc, "NtQueryVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryVirtualMemory.push_back(on_ntqueryvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtQueryVirtualMemory()
{
    //LOG(INFO, "Break on NtQueryVirtualMemory");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto MemoryInformationClass= nt::cast_to<nt::MEMORY_INFORMATION_CLASS>(args[2]);
    const auto MemoryInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto MemoryInformationLength= nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtQueryVirtualMemory)
        it(ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtQueryVolumeInformationFile(proc_t proc, const on_NtQueryVolumeInformationFile_fn& on_ntqueryvolumeinformationfile)
{
    const auto ok = setup_func(proc, "NtQueryVolumeInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryVolumeInformationFile.push_back(on_ntqueryvolumeinformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtQueryVolumeInformationFile()
{
    //LOG(INFO, "Break on NtQueryVolumeInformationFile");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FsInformation   = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FsInformationClass= nt::cast_to<nt::FS_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtQueryVolumeInformationFile)
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

bool monitor::GenericMonitor::register_NtQueueApcThreadEx(proc_t proc, const on_NtQueueApcThreadEx_fn& on_ntqueueapcthreadex)
{
    const auto ok = setup_func(proc, "NtQueueApcThreadEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueueApcThreadEx.push_back(on_ntqueueapcthreadex);
    return true;
}

void monitor::GenericMonitor::on_NtQueueApcThreadEx()
{
    //LOG(INFO, "Break on NtQueueApcThreadEx");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto UserApcReserveHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto ApcRoutine      = nt::cast_to<nt::PPS_APC_ROUTINE>    (args[2]);
    const auto ApcArgument1    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto ApcArgument2    = nt::cast_to<nt::PVOID>              (args[4]);
    const auto ApcArgument3    = nt::cast_to<nt::PVOID>              (args[5]);

    for(const auto& it : d_->observers_NtQueueApcThreadEx)
        it(ThreadHandle, UserApcReserveHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

bool monitor::GenericMonitor::register_NtQueueApcThread(proc_t proc, const on_NtQueueApcThread_fn& on_ntqueueapcthread)
{
    const auto ok = setup_func(proc, "NtQueueApcThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueueApcThread.push_back(on_ntqueueapcthread);
    return true;
}

void monitor::GenericMonitor::on_NtQueueApcThread()
{
    //LOG(INFO, "Break on NtQueueApcThread");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ApcRoutine      = nt::cast_to<nt::PPS_APC_ROUTINE>    (args[1]);
    const auto ApcArgument1    = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ApcArgument2    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto ApcArgument3    = nt::cast_to<nt::PVOID>              (args[4]);

    for(const auto& it : d_->observers_NtQueueApcThread)
        it(ThreadHandle, ApcRoutine, ApcArgument1, ApcArgument2, ApcArgument3);
}

bool monitor::GenericMonitor::register_NtRaiseException(proc_t proc, const on_NtRaiseException_fn& on_ntraiseexception)
{
    const auto ok = setup_func(proc, "NtRaiseException");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRaiseException.push_back(on_ntraiseexception);
    return true;
}

void monitor::GenericMonitor::on_NtRaiseException()
{
    //LOG(INFO, "Break on NtRaiseException");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ExceptionRecord = nt::cast_to<nt::PEXCEPTION_RECORD>  (args[0]);
    const auto ContextRecord   = nt::cast_to<nt::PCONTEXT>           (args[1]);
    const auto FirstChance     = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtRaiseException)
        it(ExceptionRecord, ContextRecord, FirstChance);
}

bool monitor::GenericMonitor::register_NtRaiseHardError(proc_t proc, const on_NtRaiseHardError_fn& on_ntraiseharderror)
{
    const auto ok = setup_func(proc, "NtRaiseHardError");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRaiseHardError.push_back(on_ntraiseharderror);
    return true;
}

void monitor::GenericMonitor::on_NtRaiseHardError()
{
    //LOG(INFO, "Break on NtRaiseHardError");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ErrorStatus     = nt::cast_to<nt::NTSTATUS>           (args[0]);
    const auto NumberOfParameters= nt::cast_to<nt::ULONG>              (args[1]);
    const auto UnicodeStringParameterMask= nt::cast_to<nt::ULONG>              (args[2]);
    const auto Parameters      = nt::cast_to<nt::PULONG_PTR>         (args[3]);
    const auto ValidResponseOptions= nt::cast_to<nt::ULONG>              (args[4]);
    const auto Response        = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtRaiseHardError)
        it(ErrorStatus, NumberOfParameters, UnicodeStringParameterMask, Parameters, ValidResponseOptions, Response);
}

bool monitor::GenericMonitor::register_NtReadFile(proc_t proc, const on_NtReadFile_fn& on_ntreadfile)
{
    const auto ok = setup_func(proc, "NtReadFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReadFile.push_back(on_ntreadfile);
    return true;
}

void monitor::GenericMonitor::on_NtReadFile()
{
    //LOG(INFO, "Break on NtReadFile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtReadFileScatter(proc_t proc, const on_NtReadFileScatter_fn& on_ntreadfilescatter)
{
    const auto ok = setup_func(proc, "NtReadFileScatter");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReadFileScatter.push_back(on_ntreadfilescatter);
    return true;
}

void monitor::GenericMonitor::on_NtReadFileScatter()
{
    //LOG(INFO, "Break on NtReadFileScatter");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtReadOnlyEnlistment(proc_t proc, const on_NtReadOnlyEnlistment_fn& on_ntreadonlyenlistment)
{
    const auto ok = setup_func(proc, "NtReadOnlyEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReadOnlyEnlistment.push_back(on_ntreadonlyenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtReadOnlyEnlistment()
{
    //LOG(INFO, "Break on NtReadOnlyEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtReadOnlyEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtReadRequestData(proc_t proc, const on_NtReadRequestData_fn& on_ntreadrequestdata)
{
    const auto ok = setup_func(proc, "NtReadRequestData");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReadRequestData.push_back(on_ntreadrequestdata);
    return true;
}

void monitor::GenericMonitor::on_NtReadRequestData()
{
    //LOG(INFO, "Break on NtReadRequestData");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto DataEntryIndex  = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto NumberOfBytesRead= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtReadRequestData)
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesRead);
}

bool monitor::GenericMonitor::register_NtReadVirtualMemory(proc_t proc, const on_NtReadVirtualMemory_fn& on_ntreadvirtualmemory)
{
    const auto ok = setup_func(proc, "NtReadVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReadVirtualMemory.push_back(on_ntreadvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtReadVirtualMemory()
{
    //LOG(INFO, "Break on NtReadVirtualMemory");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto NumberOfBytesRead= nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtReadVirtualMemory)
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead);
}

bool monitor::GenericMonitor::register_NtRecoverEnlistment(proc_t proc, const on_NtRecoverEnlistment_fn& on_ntrecoverenlistment)
{
    const auto ok = setup_func(proc, "NtRecoverEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRecoverEnlistment.push_back(on_ntrecoverenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverEnlistment()
{
    //LOG(INFO, "Break on NtRecoverEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentKey   = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtRecoverEnlistment)
        it(EnlistmentHandle, EnlistmentKey);
}

bool monitor::GenericMonitor::register_NtRecoverResourceManager(proc_t proc, const on_NtRecoverResourceManager_fn& on_ntrecoverresourcemanager)
{
    const auto ok = setup_func(proc, "NtRecoverResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRecoverResourceManager.push_back(on_ntrecoverresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverResourceManager()
{
    //LOG(INFO, "Break on NtRecoverResourceManager");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRecoverResourceManager)
        it(ResourceManagerHandle);
}

bool monitor::GenericMonitor::register_NtRecoverTransactionManager(proc_t proc, const on_NtRecoverTransactionManager_fn& on_ntrecovertransactionmanager)
{
    const auto ok = setup_func(proc, "NtRecoverTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRecoverTransactionManager.push_back(on_ntrecovertransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtRecoverTransactionManager()
{
    //LOG(INFO, "Break on NtRecoverTransactionManager");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRecoverTransactionManager)
        it(TransactionManagerHandle);
}

bool monitor::GenericMonitor::register_NtRegisterProtocolAddressInformation(proc_t proc, const on_NtRegisterProtocolAddressInformation_fn& on_ntregisterprotocoladdressinformation)
{
    const auto ok = setup_func(proc, "NtRegisterProtocolAddressInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRegisterProtocolAddressInformation.push_back(on_ntregisterprotocoladdressinformation);
    return true;
}

void monitor::GenericMonitor::on_NtRegisterProtocolAddressInformation()
{
    //LOG(INFO, "Break on NtRegisterProtocolAddressInformation");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManager = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProtocolId      = nt::cast_to<nt::PCRM_PROTOCOL_ID>   (args[1]);
    const auto ProtocolInformationSize= nt::cast_to<nt::ULONG>              (args[2]);
    const auto ProtocolInformation= nt::cast_to<nt::PVOID>              (args[3]);
    const auto CreateOptions   = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtRegisterProtocolAddressInformation)
        it(ResourceManager, ProtocolId, ProtocolInformationSize, ProtocolInformation, CreateOptions);
}

bool monitor::GenericMonitor::register_NtRegisterThreadTerminatePort(proc_t proc, const on_NtRegisterThreadTerminatePort_fn& on_ntregisterthreadterminateport)
{
    const auto ok = setup_func(proc, "NtRegisterThreadTerminatePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRegisterThreadTerminatePort.push_back(on_ntregisterthreadterminateport);
    return true;
}

void monitor::GenericMonitor::on_NtRegisterThreadTerminatePort()
{
    //LOG(INFO, "Break on NtRegisterThreadTerminatePort");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtRegisterThreadTerminatePort)
        it(PortHandle);
}

bool monitor::GenericMonitor::register_NtReleaseKeyedEvent(proc_t proc, const on_NtReleaseKeyedEvent_fn& on_ntreleasekeyedevent)
{
    const auto ok = setup_func(proc, "NtReleaseKeyedEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReleaseKeyedEvent.push_back(on_ntreleasekeyedevent);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseKeyedEvent()
{
    //LOG(INFO, "Break on NtReleaseKeyedEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyValue        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtReleaseKeyedEvent)
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtReleaseMutant(proc_t proc, const on_NtReleaseMutant_fn& on_ntreleasemutant)
{
    const auto ok = setup_func(proc, "NtReleaseMutant");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReleaseMutant.push_back(on_ntreleasemutant);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseMutant()
{
    //LOG(INFO, "Break on NtReleaseMutant");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto MutantHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousCount   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtReleaseMutant)
        it(MutantHandle, PreviousCount);
}

bool monitor::GenericMonitor::register_NtReleaseSemaphore(proc_t proc, const on_NtReleaseSemaphore_fn& on_ntreleasesemaphore)
{
    const auto ok = setup_func(proc, "NtReleaseSemaphore");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReleaseSemaphore.push_back(on_ntreleasesemaphore);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseSemaphore()
{
    //LOG(INFO, "Break on NtReleaseSemaphore");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SemaphoreHandle = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReleaseCount    = nt::cast_to<nt::LONG>               (args[1]);
    const auto PreviousCount   = nt::cast_to<nt::PLONG>              (args[2]);

    for(const auto& it : d_->observers_NtReleaseSemaphore)
        it(SemaphoreHandle, ReleaseCount, PreviousCount);
}

bool monitor::GenericMonitor::register_NtReleaseWorkerFactoryWorker(proc_t proc, const on_NtReleaseWorkerFactoryWorker_fn& on_ntreleaseworkerfactoryworker)
{
    const auto ok = setup_func(proc, "NtReleaseWorkerFactoryWorker");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReleaseWorkerFactoryWorker.push_back(on_ntreleaseworkerfactoryworker);
    return true;
}

void monitor::GenericMonitor::on_NtReleaseWorkerFactoryWorker()
{
    //LOG(INFO, "Break on NtReleaseWorkerFactoryWorker");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtReleaseWorkerFactoryWorker)
        it(WorkerFactoryHandle);
}

bool monitor::GenericMonitor::register_NtRemoveIoCompletionEx(proc_t proc, const on_NtRemoveIoCompletionEx_fn& on_ntremoveiocompletionex)
{
    const auto ok = setup_func(proc, "NtRemoveIoCompletionEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRemoveIoCompletionEx.push_back(on_ntremoveiocompletionex);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveIoCompletionEx()
{
    //LOG(INFO, "Break on NtRemoveIoCompletionEx");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionInformation= nt::cast_to<nt::PFILE_IO_COMPLETION_INFORMATION>(args[1]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[2]);
    const auto NumEntriesRemoved= nt::cast_to<nt::PULONG>             (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[5]);

    for(const auto& it : d_->observers_NtRemoveIoCompletionEx)
        it(IoCompletionHandle, IoCompletionInformation, Count, NumEntriesRemoved, Timeout, Alertable);
}

bool monitor::GenericMonitor::register_NtRemoveIoCompletion(proc_t proc, const on_NtRemoveIoCompletion_fn& on_ntremoveiocompletion)
{
    const auto ok = setup_func(proc, "NtRemoveIoCompletion");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRemoveIoCompletion.push_back(on_ntremoveiocompletion);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveIoCompletion()
{
    //LOG(INFO, "Break on NtRemoveIoCompletion");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARKeyContext  = nt::cast_to<nt::PVOID>              (args[1]);
    const auto STARApcContext  = nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtRemoveIoCompletion)
        it(IoCompletionHandle, STARKeyContext, STARApcContext, IoStatusBlock, Timeout);
}

bool monitor::GenericMonitor::register_NtRemoveProcessDebug(proc_t proc, const on_NtRemoveProcessDebug_fn& on_ntremoveprocessdebug)
{
    const auto ok = setup_func(proc, "NtRemoveProcessDebug");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRemoveProcessDebug.push_back(on_ntremoveprocessdebug);
    return true;
}

void monitor::GenericMonitor::on_NtRemoveProcessDebug()
{
    //LOG(INFO, "Break on NtRemoveProcessDebug");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtRemoveProcessDebug)
        it(ProcessHandle, DebugObjectHandle);
}

bool monitor::GenericMonitor::register_NtRenameKey(proc_t proc, const on_NtRenameKey_fn& on_ntrenamekey)
{
    const auto ok = setup_func(proc, "NtRenameKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRenameKey.push_back(on_ntrenamekey);
    return true;
}

void monitor::GenericMonitor::on_NtRenameKey()
{
    //LOG(INFO, "Break on NtRenameKey");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto NewName         = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtRenameKey)
        it(KeyHandle, NewName);
}

bool monitor::GenericMonitor::register_NtRenameTransactionManager(proc_t proc, const on_NtRenameTransactionManager_fn& on_ntrenametransactionmanager)
{
    const auto ok = setup_func(proc, "NtRenameTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRenameTransactionManager.push_back(on_ntrenametransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtRenameTransactionManager()
{
    //LOG(INFO, "Break on NtRenameTransactionManager");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto LogFileName     = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto ExistingTransactionManagerGuid= nt::cast_to<nt::LPGUID>             (args[1]);

    for(const auto& it : d_->observers_NtRenameTransactionManager)
        it(LogFileName, ExistingTransactionManagerGuid);
}

bool monitor::GenericMonitor::register_NtReplaceKey(proc_t proc, const on_NtReplaceKey_fn& on_ntreplacekey)
{
    const auto ok = setup_func(proc, "NtReplaceKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplaceKey.push_back(on_ntreplacekey);
    return true;
}

void monitor::GenericMonitor::on_NtReplaceKey()
{
    //LOG(INFO, "Break on NtReplaceKey");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto NewFile         = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto TargetHandle    = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto OldFile         = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[2]);

    for(const auto& it : d_->observers_NtReplaceKey)
        it(NewFile, TargetHandle, OldFile);
}

bool monitor::GenericMonitor::register_NtReplacePartitionUnit(proc_t proc, const on_NtReplacePartitionUnit_fn& on_ntreplacepartitionunit)
{
    const auto ok = setup_func(proc, "NtReplacePartitionUnit");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplacePartitionUnit.push_back(on_ntreplacepartitionunit);
    return true;
}

void monitor::GenericMonitor::on_NtReplacePartitionUnit()
{
    //LOG(INFO, "Break on NtReplacePartitionUnit");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetInstancePath= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto SpareInstancePath= nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtReplacePartitionUnit)
        it(TargetInstancePath, SpareInstancePath, Flags);
}

bool monitor::GenericMonitor::register_NtReplyPort(proc_t proc, const on_NtReplyPort_fn& on_ntreplyport)
{
    const auto ok = setup_func(proc, "NtReplyPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplyPort.push_back(on_ntreplyport);
    return true;
}

void monitor::GenericMonitor::on_NtReplyPort()
{
    //LOG(INFO, "Break on NtReplyPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtReplyPort)
        it(PortHandle, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtReplyWaitReceivePortEx(proc_t proc, const on_NtReplyWaitReceivePortEx_fn& on_ntreplywaitreceiveportex)
{
    const auto ok = setup_func(proc, "NtReplyWaitReceivePortEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplyWaitReceivePortEx.push_back(on_ntreplywaitreceiveportex);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePortEx()
{
    //LOG(INFO, "Break on NtReplyWaitReceivePortEx");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPortContext = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto ReceiveMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtReplyWaitReceivePortEx)
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage, Timeout);
}

bool monitor::GenericMonitor::register_NtReplyWaitReceivePort(proc_t proc, const on_NtReplyWaitReceivePort_fn& on_ntreplywaitreceiveport)
{
    const auto ok = setup_func(proc, "NtReplyWaitReceivePort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplyWaitReceivePort.push_back(on_ntreplywaitreceiveport);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReceivePort()
{
    //LOG(INFO, "Break on NtReplyWaitReceivePort");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPortContext = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);
    const auto ReceiveMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[3]);

    for(const auto& it : d_->observers_NtReplyWaitReceivePort)
        it(PortHandle, STARPortContext, ReplyMessage, ReceiveMessage);
}

bool monitor::GenericMonitor::register_NtReplyWaitReplyPort(proc_t proc, const on_NtReplyWaitReplyPort_fn& on_ntreplywaitreplyport)
{
    const auto ok = setup_func(proc, "NtReplyWaitReplyPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtReplyWaitReplyPort.push_back(on_ntreplywaitreplyport);
    return true;
}

void monitor::GenericMonitor::on_NtReplyWaitReplyPort()
{
    //LOG(INFO, "Break on NtReplyWaitReplyPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtReplyWaitReplyPort)
        it(PortHandle, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtRequestPort(proc_t proc, const on_NtRequestPort_fn& on_ntrequestport)
{
    const auto ok = setup_func(proc, "NtRequestPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRequestPort.push_back(on_ntrequestport);
    return true;
}

void monitor::GenericMonitor::on_NtRequestPort()
{
    //LOG(INFO, "Break on NtRequestPort");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);

    for(const auto& it : d_->observers_NtRequestPort)
        it(PortHandle, RequestMessage);
}

bool monitor::GenericMonitor::register_NtRequestWaitReplyPort(proc_t proc, const on_NtRequestWaitReplyPort_fn& on_ntrequestwaitreplyport)
{
    const auto ok = setup_func(proc, "NtRequestWaitReplyPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRequestWaitReplyPort.push_back(on_ntrequestwaitreplyport);
    return true;
}

void monitor::GenericMonitor::on_NtRequestWaitReplyPort()
{
    //LOG(INFO, "Break on NtRequestWaitReplyPort");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto RequestMessage  = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto ReplyMessage    = nt::cast_to<nt::PPORT_MESSAGE>      (args[2]);

    for(const auto& it : d_->observers_NtRequestWaitReplyPort)
        it(PortHandle, RequestMessage, ReplyMessage);
}

bool monitor::GenericMonitor::register_NtResetEvent(proc_t proc, const on_NtResetEvent_fn& on_ntresetevent)
{
    const auto ok = setup_func(proc, "NtResetEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtResetEvent.push_back(on_ntresetevent);
    return true;
}

void monitor::GenericMonitor::on_NtResetEvent()
{
    //LOG(INFO, "Break on NtResetEvent");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtResetEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtResetWriteWatch(proc_t proc, const on_NtResetWriteWatch_fn& on_ntresetwritewatch)
{
    const auto ok = setup_func(proc, "NtResetWriteWatch");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtResetWriteWatch.push_back(on_ntresetwritewatch);
    return true;
}

void monitor::GenericMonitor::on_NtResetWriteWatch()
{
    //LOG(INFO, "Break on NtResetWriteWatch");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::SIZE_T>             (args[2]);

    for(const auto& it : d_->observers_NtResetWriteWatch)
        it(ProcessHandle, BaseAddress, RegionSize);
}

bool monitor::GenericMonitor::register_NtRestoreKey(proc_t proc, const on_NtRestoreKey_fn& on_ntrestorekey)
{
    const auto ok = setup_func(proc, "NtRestoreKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRestoreKey.push_back(on_ntrestorekey);
    return true;
}

void monitor::GenericMonitor::on_NtRestoreKey()
{
    //LOG(INFO, "Break on NtRestoreKey");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtRestoreKey)
        it(KeyHandle, FileHandle, Flags);
}

bool monitor::GenericMonitor::register_NtResumeProcess(proc_t proc, const on_NtResumeProcess_fn& on_ntresumeprocess)
{
    const auto ok = setup_func(proc, "NtResumeProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtResumeProcess.push_back(on_ntresumeprocess);
    return true;
}

void monitor::GenericMonitor::on_NtResumeProcess()
{
    //LOG(INFO, "Break on NtResumeProcess");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtResumeProcess)
        it(ProcessHandle);
}

bool monitor::GenericMonitor::register_NtResumeThread(proc_t proc, const on_NtResumeThread_fn& on_ntresumethread)
{
    const auto ok = setup_func(proc, "NtResumeThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtResumeThread.push_back(on_ntresumethread);
    return true;
}

void monitor::GenericMonitor::on_NtResumeThread()
{
    //LOG(INFO, "Break on NtResumeThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtResumeThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtRollbackComplete(proc_t proc, const on_NtRollbackComplete_fn& on_ntrollbackcomplete)
{
    const auto ok = setup_func(proc, "NtRollbackComplete");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRollbackComplete.push_back(on_ntrollbackcomplete);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackComplete()
{
    //LOG(INFO, "Break on NtRollbackComplete");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollbackComplete)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtRollbackEnlistment(proc_t proc, const on_NtRollbackEnlistment_fn& on_ntrollbackenlistment)
{
    const auto ok = setup_func(proc, "NtRollbackEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRollbackEnlistment.push_back(on_ntrollbackenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackEnlistment()
{
    //LOG(INFO, "Break on NtRollbackEnlistment");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollbackEnlistment)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtRollbackTransaction(proc_t proc, const on_NtRollbackTransaction_fn& on_ntrollbacktransaction)
{
    const auto ok = setup_func(proc, "NtRollbackTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRollbackTransaction.push_back(on_ntrollbacktransaction);
    return true;
}

void monitor::GenericMonitor::on_NtRollbackTransaction()
{
    //LOG(INFO, "Break on NtRollbackTransaction");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Wait            = nt::cast_to<nt::BOOLEAN>            (args[1]);

    for(const auto& it : d_->observers_NtRollbackTransaction)
        it(TransactionHandle, Wait);
}

bool monitor::GenericMonitor::register_NtRollforwardTransactionManager(proc_t proc, const on_NtRollforwardTransactionManager_fn& on_ntrollforwardtransactionmanager)
{
    const auto ok = setup_func(proc, "NtRollforwardTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtRollforwardTransactionManager.push_back(on_ntrollforwardtransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtRollforwardTransactionManager()
{
    //LOG(INFO, "Break on NtRollforwardTransactionManager");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtRollforwardTransactionManager)
        it(TransactionManagerHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtSaveKeyEx(proc_t proc, const on_NtSaveKeyEx_fn& on_ntsavekeyex)
{
    const auto ok = setup_func(proc, "NtSaveKeyEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSaveKeyEx.push_back(on_ntsavekeyex);
    return true;
}

void monitor::GenericMonitor::on_NtSaveKeyEx()
{
    //LOG(INFO, "Break on NtSaveKeyEx");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Format          = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSaveKeyEx)
        it(KeyHandle, FileHandle, Format);
}

bool monitor::GenericMonitor::register_NtSaveKey(proc_t proc, const on_NtSaveKey_fn& on_ntsavekey)
{
    const auto ok = setup_func(proc, "NtSaveKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSaveKey.push_back(on_ntsavekey);
    return true;
}

void monitor::GenericMonitor::on_NtSaveKey()
{
    //LOG(INFO, "Break on NtSaveKey");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtSaveKey)
        it(KeyHandle, FileHandle);
}

bool monitor::GenericMonitor::register_NtSaveMergedKeys(proc_t proc, const on_NtSaveMergedKeys_fn& on_ntsavemergedkeys)
{
    const auto ok = setup_func(proc, "NtSaveMergedKeys");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSaveMergedKeys.push_back(on_ntsavemergedkeys);
    return true;
}

void monitor::GenericMonitor::on_NtSaveMergedKeys()
{
    //LOG(INFO, "Break on NtSaveMergedKeys");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HighPrecedenceKeyHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto LowPrecedenceKeyHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[2]);

    for(const auto& it : d_->observers_NtSaveMergedKeys)
        it(HighPrecedenceKeyHandle, LowPrecedenceKeyHandle, FileHandle);
}

bool monitor::GenericMonitor::register_NtSecureConnectPort(proc_t proc, const on_NtSecureConnectPort_fn& on_ntsecureconnectport)
{
    const auto ok = setup_func(proc, "NtSecureConnectPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSecureConnectPort.push_back(on_ntsecureconnectport);
    return true;
}

void monitor::GenericMonitor::on_NtSecureConnectPort()
{
    //LOG(INFO, "Break on NtSecureConnectPort");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(PortHandle, PortName, SecurityQos, ClientView, RequiredServerSid, ServerView, MaxMessageLength, ConnectionInformation, ConnectionInformationLength);
}

bool monitor::GenericMonitor::register_NtSetBootEntryOrder(proc_t proc, const on_NtSetBootEntryOrder_fn& on_ntsetbootentryorder)
{
    const auto ok = setup_func(proc, "NtSetBootEntryOrder");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetBootEntryOrder.push_back(on_ntsetbootentryorder);
    return true;
}

void monitor::GenericMonitor::on_NtSetBootEntryOrder()
{
    //LOG(INFO, "Break on NtSetBootEntryOrder");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetBootEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtSetBootOptions(proc_t proc, const on_NtSetBootOptions_fn& on_ntsetbootoptions)
{
    const auto ok = setup_func(proc, "NtSetBootOptions");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetBootOptions.push_back(on_ntsetbootoptions);
    return true;
}

void monitor::GenericMonitor::on_NtSetBootOptions()
{
    //LOG(INFO, "Break on NtSetBootOptions");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto BootOptions     = nt::cast_to<nt::PBOOT_OPTIONS>      (args[0]);
    const auto FieldsToChange  = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetBootOptions)
        it(BootOptions, FieldsToChange);
}

bool monitor::GenericMonitor::register_NtSetContextThread(proc_t proc, const on_NtSetContextThread_fn& on_ntsetcontextthread)
{
    const auto ok = setup_func(proc, "NtSetContextThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetContextThread.push_back(on_ntsetcontextthread);
    return true;
}

void monitor::GenericMonitor::on_NtSetContextThread()
{
    //LOG(INFO, "Break on NtSetContextThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadContext   = nt::cast_to<nt::PCONTEXT>           (args[1]);

    for(const auto& it : d_->observers_NtSetContextThread)
        it(ThreadHandle, ThreadContext);
}

bool monitor::GenericMonitor::register_NtSetDebugFilterState(proc_t proc, const on_NtSetDebugFilterState_fn& on_ntsetdebugfilterstate)
{
    const auto ok = setup_func(proc, "NtSetDebugFilterState");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetDebugFilterState.push_back(on_ntsetdebugfilterstate);
    return true;
}

void monitor::GenericMonitor::on_NtSetDebugFilterState()
{
    //LOG(INFO, "Break on NtSetDebugFilterState");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ComponentId     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Level           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto State           = nt::cast_to<nt::BOOLEAN>            (args[2]);

    for(const auto& it : d_->observers_NtSetDebugFilterState)
        it(ComponentId, Level, State);
}

bool monitor::GenericMonitor::register_NtSetDefaultHardErrorPort(proc_t proc, const on_NtSetDefaultHardErrorPort_fn& on_ntsetdefaultharderrorport)
{
    const auto ok = setup_func(proc, "NtSetDefaultHardErrorPort");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetDefaultHardErrorPort.push_back(on_ntsetdefaultharderrorport);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultHardErrorPort()
{
    //LOG(INFO, "Break on NtSetDefaultHardErrorPort");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DefaultHardErrorPort= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetDefaultHardErrorPort)
        it(DefaultHardErrorPort);
}

bool monitor::GenericMonitor::register_NtSetDefaultLocale(proc_t proc, const on_NtSetDefaultLocale_fn& on_ntsetdefaultlocale)
{
    const auto ok = setup_func(proc, "NtSetDefaultLocale");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetDefaultLocale.push_back(on_ntsetdefaultlocale);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultLocale()
{
    //LOG(INFO, "Break on NtSetDefaultLocale");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto UserProfile     = nt::cast_to<nt::BOOLEAN>            (args[0]);
    const auto DefaultLocaleId = nt::cast_to<nt::LCID>               (args[1]);

    for(const auto& it : d_->observers_NtSetDefaultLocale)
        it(UserProfile, DefaultLocaleId);
}

bool monitor::GenericMonitor::register_NtSetDefaultUILanguage(proc_t proc, const on_NtSetDefaultUILanguage_fn& on_ntsetdefaultuilanguage)
{
    const auto ok = setup_func(proc, "NtSetDefaultUILanguage");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetDefaultUILanguage.push_back(on_ntsetdefaultuilanguage);
    return true;
}

void monitor::GenericMonitor::on_NtSetDefaultUILanguage()
{
    //LOG(INFO, "Break on NtSetDefaultUILanguage");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DefaultUILanguageId= nt::cast_to<nt::LANGID>             (args[0]);

    for(const auto& it : d_->observers_NtSetDefaultUILanguage)
        it(DefaultUILanguageId);
}

bool monitor::GenericMonitor::register_NtSetDriverEntryOrder(proc_t proc, const on_NtSetDriverEntryOrder_fn& on_ntsetdriverentryorder)
{
    const auto ok = setup_func(proc, "NtSetDriverEntryOrder");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetDriverEntryOrder.push_back(on_ntsetdriverentryorder);
    return true;
}

void monitor::GenericMonitor::on_NtSetDriverEntryOrder()
{
    //LOG(INFO, "Break on NtSetDriverEntryOrder");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Ids             = nt::cast_to<nt::PULONG>             (args[0]);
    const auto Count           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetDriverEntryOrder)
        it(Ids, Count);
}

bool monitor::GenericMonitor::register_NtSetEaFile(proc_t proc, const on_NtSetEaFile_fn& on_ntseteafile)
{
    const auto ok = setup_func(proc, "NtSetEaFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetEaFile.push_back(on_ntseteafile);
    return true;
}

void monitor::GenericMonitor::on_NtSetEaFile()
{
    //LOG(INFO, "Break on NtSetEaFile");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetEaFile)
        it(FileHandle, IoStatusBlock, Buffer, Length);
}

bool monitor::GenericMonitor::register_NtSetEventBoostPriority(proc_t proc, const on_NtSetEventBoostPriority_fn& on_ntseteventboostpriority)
{
    const auto ok = setup_func(proc, "NtSetEventBoostPriority");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetEventBoostPriority.push_back(on_ntseteventboostpriority);
    return true;
}

void monitor::GenericMonitor::on_NtSetEventBoostPriority()
{
    //LOG(INFO, "Break on NtSetEventBoostPriority");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetEventBoostPriority)
        it(EventHandle);
}

bool monitor::GenericMonitor::register_NtSetEvent(proc_t proc, const on_NtSetEvent_fn& on_ntsetevent)
{
    const auto ok = setup_func(proc, "NtSetEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetEvent.push_back(on_ntsetevent);
    return true;
}

void monitor::GenericMonitor::on_NtSetEvent()
{
    //LOG(INFO, "Break on NtSetEvent");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousState   = nt::cast_to<nt::PLONG>              (args[1]);

    for(const auto& it : d_->observers_NtSetEvent)
        it(EventHandle, PreviousState);
}

bool monitor::GenericMonitor::register_NtSetHighEventPair(proc_t proc, const on_NtSetHighEventPair_fn& on_ntsethigheventpair)
{
    const auto ok = setup_func(proc, "NtSetHighEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetHighEventPair.push_back(on_ntsethigheventpair);
    return true;
}

void monitor::GenericMonitor::on_NtSetHighEventPair()
{
    //LOG(INFO, "Break on NtSetHighEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetHighWaitLowEventPair(proc_t proc, const on_NtSetHighWaitLowEventPair_fn& on_ntsethighwaitloweventpair)
{
    const auto ok = setup_func(proc, "NtSetHighWaitLowEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetHighWaitLowEventPair.push_back(on_ntsethighwaitloweventpair);
    return true;
}

void monitor::GenericMonitor::on_NtSetHighWaitLowEventPair()
{
    //LOG(INFO, "Break on NtSetHighWaitLowEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetHighWaitLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetInformationDebugObject(proc_t proc, const on_NtSetInformationDebugObject_fn& on_ntsetinformationdebugobject)
{
    const auto ok = setup_func(proc, "NtSetInformationDebugObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationDebugObject.push_back(on_ntsetinformationdebugobject);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationDebugObject()
{
    //LOG(INFO, "Break on NtSetInformationDebugObject");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DebugObjectInformationClass= nt::cast_to<nt::DEBUGOBJECTINFOCLASS>(args[1]);
    const auto DebugInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto DebugInformationLength= nt::cast_to<nt::ULONG>              (args[3]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_NtSetInformationDebugObject)
        it(DebugObjectHandle, DebugObjectInformationClass, DebugInformation, DebugInformationLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtSetInformationEnlistment(proc_t proc, const on_NtSetInformationEnlistment_fn& on_ntsetinformationenlistment)
{
    const auto ok = setup_func(proc, "NtSetInformationEnlistment");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationEnlistment.push_back(on_ntsetinformationenlistment);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationEnlistment()
{
    //LOG(INFO, "Break on NtSetInformationEnlistment");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto EnlistmentInformationClass= nt::cast_to<nt::ENLISTMENT_INFORMATION_CLASS>(args[1]);
    const auto EnlistmentInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto EnlistmentInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationEnlistment)
        it(EnlistmentHandle, EnlistmentInformationClass, EnlistmentInformation, EnlistmentInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationFile(proc_t proc, const on_NtSetInformationFile_fn& on_ntsetinformationfile)
{
    const auto ok = setup_func(proc, "NtSetInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationFile.push_back(on_ntsetinformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationFile()
{
    //LOG(INFO, "Break on NtSetInformationFile");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FileInformation = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FileInformationClass= nt::cast_to<nt::FILE_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtSetInformationFile)
        it(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
}

bool monitor::GenericMonitor::register_NtSetInformationJobObject(proc_t proc, const on_NtSetInformationJobObject_fn& on_ntsetinformationjobobject)
{
    const auto ok = setup_func(proc, "NtSetInformationJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationJobObject.push_back(on_ntsetinformationjobobject);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationJobObject()
{
    //LOG(INFO, "Break on NtSetInformationJobObject");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto JobObjectInformationClass= nt::cast_to<nt::JOBOBJECTINFOCLASS> (args[1]);
    const auto JobObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto JobObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationJobObject)
        it(JobHandle, JobObjectInformationClass, JobObjectInformation, JobObjectInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationKey(proc_t proc, const on_NtSetInformationKey_fn& on_ntsetinformationkey)
{
    const auto ok = setup_func(proc, "NtSetInformationKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationKey.push_back(on_ntsetinformationkey);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationKey()
{
    //LOG(INFO, "Break on NtSetInformationKey");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeySetInformationClass= nt::cast_to<nt::KEY_SET_INFORMATION_CLASS>(args[1]);
    const auto KeySetInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto KeySetInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationKey)
        it(KeyHandle, KeySetInformationClass, KeySetInformation, KeySetInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationObject(proc_t proc, const on_NtSetInformationObject_fn& on_ntsetinformationobject)
{
    const auto ok = setup_func(proc, "NtSetInformationObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationObject.push_back(on_ntsetinformationobject);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationObject()
{
    //LOG(INFO, "Break on NtSetInformationObject");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ObjectInformationClass= nt::cast_to<nt::OBJECT_INFORMATION_CLASS>(args[1]);
    const auto ObjectInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ObjectInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationObject)
        it(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationProcess(proc_t proc, const on_NtSetInformationProcess_fn& on_ntsetinformationprocess)
{
    const auto ok = setup_func(proc, "NtSetInformationProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationProcess.push_back(on_ntsetinformationprocess);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationProcess()
{
    //LOG(INFO, "Break on NtSetInformationProcess");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ProcessInformationClass= nt::cast_to<nt::PROCESSINFOCLASS>   (args[1]);
    const auto ProcessInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ProcessInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationProcess)
        it(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationResourceManager(proc_t proc, const on_NtSetInformationResourceManager_fn& on_ntsetinformationresourcemanager)
{
    const auto ok = setup_func(proc, "NtSetInformationResourceManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationResourceManager.push_back(on_ntsetinformationresourcemanager);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationResourceManager()
{
    //LOG(INFO, "Break on NtSetInformationResourceManager");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ResourceManagerHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ResourceManagerInformationClass= nt::cast_to<nt::RESOURCEMANAGER_INFORMATION_CLASS>(args[1]);
    const auto ResourceManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ResourceManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationResourceManager)
        it(ResourceManagerHandle, ResourceManagerInformationClass, ResourceManagerInformation, ResourceManagerInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationThread(proc_t proc, const on_NtSetInformationThread_fn& on_ntsetinformationthread)
{
    const auto ok = setup_func(proc, "NtSetInformationThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationThread.push_back(on_ntsetinformationthread);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationThread()
{
    //LOG(INFO, "Break on NtSetInformationThread");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ThreadInformationClass= nt::cast_to<nt::THREADINFOCLASS>    (args[1]);
    const auto ThreadInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto ThreadInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationThread)
        it(ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationToken(proc_t proc, const on_NtSetInformationToken_fn& on_ntsetinformationtoken)
{
    const auto ok = setup_func(proc, "NtSetInformationToken");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationToken.push_back(on_ntsetinformationtoken);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationToken()
{
    //LOG(INFO, "Break on NtSetInformationToken");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TokenHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TokenInformationClass= nt::cast_to<nt::TOKEN_INFORMATION_CLASS>(args[1]);
    const auto TokenInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TokenInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationToken)
        it(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationTransaction(proc_t proc, const on_NtSetInformationTransaction_fn& on_ntsetinformationtransaction)
{
    const auto ok = setup_func(proc, "NtSetInformationTransaction");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationTransaction.push_back(on_ntsetinformationtransaction);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationTransaction()
{
    //LOG(INFO, "Break on NtSetInformationTransaction");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TransactionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionInformationClass= nt::cast_to<nt::TRANSACTION_INFORMATION_CLASS>(args[1]);
    const auto TransactionInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationTransaction)
        it(TransactionHandle, TransactionInformationClass, TransactionInformation, TransactionInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationTransactionManager(proc_t proc, const on_NtSetInformationTransactionManager_fn& on_ntsetinformationtransactionmanager)
{
    const auto ok = setup_func(proc, "NtSetInformationTransactionManager");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationTransactionManager.push_back(on_ntsetinformationtransactionmanager);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationTransactionManager()
{
    //LOG(INFO, "Break on NtSetInformationTransactionManager");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TmHandle        = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TransactionManagerInformationClass= nt::cast_to<nt::TRANSACTIONMANAGER_INFORMATION_CLASS>(args[1]);
    const auto TransactionManagerInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TransactionManagerInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationTransactionManager)
        it(TmHandle, TransactionManagerInformationClass, TransactionManagerInformation, TransactionManagerInformationLength);
}

bool monitor::GenericMonitor::register_NtSetInformationWorkerFactory(proc_t proc, const on_NtSetInformationWorkerFactory_fn& on_ntsetinformationworkerfactory)
{
    const auto ok = setup_func(proc, "NtSetInformationWorkerFactory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetInformationWorkerFactory.push_back(on_ntsetinformationworkerfactory);
    return true;
}

void monitor::GenericMonitor::on_NtSetInformationWorkerFactory()
{
    //LOG(INFO, "Break on NtSetInformationWorkerFactory");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WorkerFactoryInformationClass= nt::cast_to<nt::WORKERFACTORYINFOCLASS>(args[1]);
    const auto WorkerFactoryInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto WorkerFactoryInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetInformationWorkerFactory)
        it(WorkerFactoryHandle, WorkerFactoryInformationClass, WorkerFactoryInformation, WorkerFactoryInformationLength);
}

bool monitor::GenericMonitor::register_NtSetIntervalProfile(proc_t proc, const on_NtSetIntervalProfile_fn& on_ntsetintervalprofile)
{
    const auto ok = setup_func(proc, "NtSetIntervalProfile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetIntervalProfile.push_back(on_ntsetintervalprofile);
    return true;
}

void monitor::GenericMonitor::on_NtSetIntervalProfile()
{
    //LOG(INFO, "Break on NtSetIntervalProfile");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Interval        = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Source          = nt::cast_to<nt::KPROFILE_SOURCE>    (args[1]);

    for(const auto& it : d_->observers_NtSetIntervalProfile)
        it(Interval, Source);
}

bool monitor::GenericMonitor::register_NtSetIoCompletionEx(proc_t proc, const on_NtSetIoCompletionEx_fn& on_ntsetiocompletionex)
{
    const auto ok = setup_func(proc, "NtSetIoCompletionEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetIoCompletionEx.push_back(on_ntsetiocompletionex);
    return true;
}

void monitor::GenericMonitor::on_NtSetIoCompletionEx()
{
    //LOG(INFO, "Break on NtSetIoCompletionEx");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoCompletionReserveHandle= nt::cast_to<nt::HANDLE>             (args[1]);
    const auto KeyContext      = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[3]);
    const auto IoStatus        = nt::cast_to<nt::NTSTATUS>           (args[4]);
    const auto IoStatusInformation= nt::cast_to<nt::ULONG_PTR>          (args[5]);

    for(const auto& it : d_->observers_NtSetIoCompletionEx)
        it(IoCompletionHandle, IoCompletionReserveHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
}

bool monitor::GenericMonitor::register_NtSetIoCompletion(proc_t proc, const on_NtSetIoCompletion_fn& on_ntsetiocompletion)
{
    const auto ok = setup_func(proc, "NtSetIoCompletion");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetIoCompletion.push_back(on_ntsetiocompletion);
    return true;
}

void monitor::GenericMonitor::on_NtSetIoCompletion()
{
    //LOG(INFO, "Break on NtSetIoCompletion");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto IoCompletionHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyContext      = nt::cast_to<nt::PVOID>              (args[1]);
    const auto ApcContext      = nt::cast_to<nt::PVOID>              (args[2]);
    const auto IoStatus        = nt::cast_to<nt::NTSTATUS>           (args[3]);
    const auto IoStatusInformation= nt::cast_to<nt::ULONG_PTR>          (args[4]);

    for(const auto& it : d_->observers_NtSetIoCompletion)
        it(IoCompletionHandle, KeyContext, ApcContext, IoStatus, IoStatusInformation);
}

bool monitor::GenericMonitor::register_NtSetLdtEntries(proc_t proc, const on_NtSetLdtEntries_fn& on_ntsetldtentries)
{
    const auto ok = setup_func(proc, "NtSetLdtEntries");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetLdtEntries.push_back(on_ntsetldtentries);
    return true;
}

void monitor::GenericMonitor::on_NtSetLdtEntries()
{
    //LOG(INFO, "Break on NtSetLdtEntries");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Selector0       = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Entry0Low       = nt::cast_to<nt::ULONG>              (args[1]);
    const auto Entry0Hi        = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Selector1       = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Entry1Low       = nt::cast_to<nt::ULONG>              (args[4]);
    const auto Entry1Hi        = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtSetLdtEntries)
        it(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);
}

bool monitor::GenericMonitor::register_NtSetLowEventPair(proc_t proc, const on_NtSetLowEventPair_fn& on_ntsetloweventpair)
{
    const auto ok = setup_func(proc, "NtSetLowEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetLowEventPair.push_back(on_ntsetloweventpair);
    return true;
}

void monitor::GenericMonitor::on_NtSetLowEventPair()
{
    //LOG(INFO, "Break on NtSetLowEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetLowWaitHighEventPair(proc_t proc, const on_NtSetLowWaitHighEventPair_fn& on_ntsetlowwaithigheventpair)
{
    const auto ok = setup_func(proc, "NtSetLowWaitHighEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetLowWaitHighEventPair.push_back(on_ntsetlowwaithigheventpair);
    return true;
}

void monitor::GenericMonitor::on_NtSetLowWaitHighEventPair()
{
    //LOG(INFO, "Break on NtSetLowWaitHighEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSetLowWaitHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtSetQuotaInformationFile(proc_t proc, const on_NtSetQuotaInformationFile_fn& on_ntsetquotainformationfile)
{
    const auto ok = setup_func(proc, "NtSetQuotaInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetQuotaInformationFile.push_back(on_ntsetquotainformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtSetQuotaInformationFile()
{
    //LOG(INFO, "Break on NtSetQuotaInformationFile");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetQuotaInformationFile)
        it(FileHandle, IoStatusBlock, Buffer, Length);
}

bool monitor::GenericMonitor::register_NtSetSecurityObject(proc_t proc, const on_NtSetSecurityObject_fn& on_ntsetsecurityobject)
{
    const auto ok = setup_func(proc, "NtSetSecurityObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSecurityObject.push_back(on_ntsetsecurityobject);
    return true;
}

void monitor::GenericMonitor::on_NtSetSecurityObject()
{
    //LOG(INFO, "Break on NtSetSecurityObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto SecurityInformation= nt::cast_to<nt::SECURITY_INFORMATION>(args[1]);
    const auto SecurityDescriptor= nt::cast_to<nt::PSECURITY_DESCRIPTOR>(args[2]);

    for(const auto& it : d_->observers_NtSetSecurityObject)
        it(Handle, SecurityInformation, SecurityDescriptor);
}

bool monitor::GenericMonitor::register_NtSetSystemEnvironmentValueEx(proc_t proc, const on_NtSetSystemEnvironmentValueEx_fn& on_ntsetsystemenvironmentvalueex)
{
    const auto ok = setup_func(proc, "NtSetSystemEnvironmentValueEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSystemEnvironmentValueEx.push_back(on_ntsetsystemenvironmentvalueex);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValueEx()
{
    //LOG(INFO, "Break on NtSetSystemEnvironmentValueEx");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VendorGuid      = nt::cast_to<nt::LPGUID>             (args[1]);
    const auto Value           = nt::cast_to<nt::PVOID>              (args[2]);
    const auto ValueLength     = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Attributes      = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValueEx)
        it(VariableName, VendorGuid, Value, ValueLength, Attributes);
}

bool monitor::GenericMonitor::register_NtSetSystemEnvironmentValue(proc_t proc, const on_NtSetSystemEnvironmentValue_fn& on_ntsetsystemenvironmentvalue)
{
    const auto ok = setup_func(proc, "NtSetSystemEnvironmentValue");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSystemEnvironmentValue.push_back(on_ntsetsystemenvironmentvalue);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemEnvironmentValue()
{
    //LOG(INFO, "Break on NtSetSystemEnvironmentValue");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto VariableName    = nt::cast_to<nt::PUNICODE_STRING>    (args[0]);
    const auto VariableValue   = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);

    for(const auto& it : d_->observers_NtSetSystemEnvironmentValue)
        it(VariableName, VariableValue);
}

bool monitor::GenericMonitor::register_NtSetSystemInformation(proc_t proc, const on_NtSetSystemInformation_fn& on_ntsetsysteminformation)
{
    const auto ok = setup_func(proc, "NtSetSystemInformation");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSystemInformation.push_back(on_ntsetsysteminformation);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemInformation()
{
    //LOG(INFO, "Break on NtSetSystemInformation");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemInformationClass= nt::cast_to<nt::SYSTEM_INFORMATION_CLASS>(args[0]);
    const auto SystemInformation= nt::cast_to<nt::PVOID>              (args[1]);
    const auto SystemInformationLength= nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSetSystemInformation)
        it(SystemInformationClass, SystemInformation, SystemInformationLength);
}

bool monitor::GenericMonitor::register_NtSetSystemPowerState(proc_t proc, const on_NtSetSystemPowerState_fn& on_ntsetsystempowerstate)
{
    const auto ok = setup_func(proc, "NtSetSystemPowerState");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSystemPowerState.push_back(on_ntsetsystempowerstate);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemPowerState()
{
    //LOG(INFO, "Break on NtSetSystemPowerState");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemAction    = nt::cast_to<nt::POWER_ACTION>       (args[0]);
    const auto MinSystemState  = nt::cast_to<nt::SYSTEM_POWER_STATE> (args[1]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[2]);

    for(const auto& it : d_->observers_NtSetSystemPowerState)
        it(SystemAction, MinSystemState, Flags);
}

bool monitor::GenericMonitor::register_NtSetSystemTime(proc_t proc, const on_NtSetSystemTime_fn& on_ntsetsystemtime)
{
    const auto ok = setup_func(proc, "NtSetSystemTime");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetSystemTime.push_back(on_ntsetsystemtime);
    return true;
}

void monitor::GenericMonitor::on_NtSetSystemTime()
{
    //LOG(INFO, "Break on NtSetSystemTime");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SystemTime      = nt::cast_to<nt::PLARGE_INTEGER>     (args[0]);
    const auto PreviousTime    = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtSetSystemTime)
        it(SystemTime, PreviousTime);
}

bool monitor::GenericMonitor::register_NtSetThreadExecutionState(proc_t proc, const on_NtSetThreadExecutionState_fn& on_ntsetthreadexecutionstate)
{
    const auto ok = setup_func(proc, "NtSetThreadExecutionState");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetThreadExecutionState.push_back(on_ntsetthreadexecutionstate);
    return true;
}

void monitor::GenericMonitor::on_NtSetThreadExecutionState()
{
    //LOG(INFO, "Break on NtSetThreadExecutionState");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto esFlags         = nt::cast_to<nt::EXECUTION_STATE>    (args[0]);
    const auto STARPreviousFlags= nt::cast_to<nt::EXECUTION_STATE>    (args[1]);

    for(const auto& it : d_->observers_NtSetThreadExecutionState)
        it(esFlags, STARPreviousFlags);
}

bool monitor::GenericMonitor::register_NtSetTimerEx(proc_t proc, const on_NtSetTimerEx_fn& on_ntsettimerex)
{
    const auto ok = setup_func(proc, "NtSetTimerEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetTimerEx.push_back(on_ntsettimerex);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimerEx()
{
    //LOG(INFO, "Break on NtSetTimerEx");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TimerSetInformationClass= nt::cast_to<nt::TIMER_SET_INFORMATION_CLASS>(args[1]);
    const auto TimerSetInformation= nt::cast_to<nt::PVOID>              (args[2]);
    const auto TimerSetInformationLength= nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtSetTimerEx)
        it(TimerHandle, TimerSetInformationClass, TimerSetInformation, TimerSetInformationLength);
}

bool monitor::GenericMonitor::register_NtSetTimer(proc_t proc, const on_NtSetTimer_fn& on_ntsettimer)
{
    const auto ok = setup_func(proc, "NtSetTimer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetTimer.push_back(on_ntsettimer);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimer()
{
    //LOG(INFO, "Break on NtSetTimer");
    constexpr int nargs = 7;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TimerHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto DueTime         = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);
    const auto TimerApcRoutine = nt::cast_to<nt::PTIMER_APC_ROUTINE> (args[2]);
    const auto TimerContext    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto WakeTimer       = nt::cast_to<nt::BOOLEAN>            (args[4]);
    const auto Period          = nt::cast_to<nt::LONG>               (args[5]);
    const auto PreviousState   = nt::cast_to<nt::PBOOLEAN>           (args[6]);

    for(const auto& it : d_->observers_NtSetTimer)
        it(TimerHandle, DueTime, TimerApcRoutine, TimerContext, WakeTimer, Period, PreviousState);
}

bool monitor::GenericMonitor::register_NtSetTimerResolution(proc_t proc, const on_NtSetTimerResolution_fn& on_ntsettimerresolution)
{
    const auto ok = setup_func(proc, "NtSetTimerResolution");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetTimerResolution.push_back(on_ntsettimerresolution);
    return true;
}

void monitor::GenericMonitor::on_NtSetTimerResolution()
{
    //LOG(INFO, "Break on NtSetTimerResolution");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DesiredTime     = nt::cast_to<nt::ULONG>              (args[0]);
    const auto SetResolution   = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto ActualTime      = nt::cast_to<nt::PULONG>             (args[2]);

    for(const auto& it : d_->observers_NtSetTimerResolution)
        it(DesiredTime, SetResolution, ActualTime);
}

bool monitor::GenericMonitor::register_NtSetUuidSeed(proc_t proc, const on_NtSetUuidSeed_fn& on_ntsetuuidseed)
{
    const auto ok = setup_func(proc, "NtSetUuidSeed");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetUuidSeed.push_back(on_ntsetuuidseed);
    return true;
}

void monitor::GenericMonitor::on_NtSetUuidSeed()
{
    //LOG(INFO, "Break on NtSetUuidSeed");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Seed            = nt::cast_to<nt::PCHAR>              (args[0]);

    for(const auto& it : d_->observers_NtSetUuidSeed)
        it(Seed);
}

bool monitor::GenericMonitor::register_NtSetValueKey(proc_t proc, const on_NtSetValueKey_fn& on_ntsetvaluekey)
{
    const auto ok = setup_func(proc, "NtSetValueKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetValueKey.push_back(on_ntsetvaluekey);
    return true;
}

void monitor::GenericMonitor::on_NtSetValueKey()
{
    //LOG(INFO, "Break on NtSetValueKey");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ValueName       = nt::cast_to<nt::PUNICODE_STRING>    (args[1]);
    const auto TitleIndex      = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Type            = nt::cast_to<nt::ULONG>              (args[3]);
    const auto Data            = nt::cast_to<nt::PVOID>              (args[4]);
    const auto DataSize        = nt::cast_to<nt::ULONG>              (args[5]);

    for(const auto& it : d_->observers_NtSetValueKey)
        it(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
}

bool monitor::GenericMonitor::register_NtSetVolumeInformationFile(proc_t proc, const on_NtSetVolumeInformationFile_fn& on_ntsetvolumeinformationfile)
{
    const auto ok = setup_func(proc, "NtSetVolumeInformationFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSetVolumeInformationFile.push_back(on_ntsetvolumeinformationfile);
    return true;
}

void monitor::GenericMonitor::on_NtSetVolumeInformationFile()
{
    //LOG(INFO, "Break on NtSetVolumeInformationFile");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto FsInformation   = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Length          = nt::cast_to<nt::ULONG>              (args[3]);
    const auto FsInformationClass= nt::cast_to<nt::FS_INFORMATION_CLASS>(args[4]);

    for(const auto& it : d_->observers_NtSetVolumeInformationFile)
        it(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
}

bool monitor::GenericMonitor::register_NtShutdownSystem(proc_t proc, const on_NtShutdownSystem_fn& on_ntshutdownsystem)
{
    const auto ok = setup_func(proc, "NtShutdownSystem");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtShutdownSystem.push_back(on_ntshutdownsystem);
    return true;
}

void monitor::GenericMonitor::on_NtShutdownSystem()
{
    //LOG(INFO, "Break on NtShutdownSystem");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Action          = nt::cast_to<nt::SHUTDOWN_ACTION>    (args[0]);

    for(const auto& it : d_->observers_NtShutdownSystem)
        it(Action);
}

bool monitor::GenericMonitor::register_NtShutdownWorkerFactory(proc_t proc, const on_NtShutdownWorkerFactory_fn& on_ntshutdownworkerfactory)
{
    const auto ok = setup_func(proc, "NtShutdownWorkerFactory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtShutdownWorkerFactory.push_back(on_ntshutdownworkerfactory);
    return true;
}

void monitor::GenericMonitor::on_NtShutdownWorkerFactory()
{
    //LOG(INFO, "Break on NtShutdownWorkerFactory");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARPendingWorkerCount= nt::cast_to<nt::LONG>               (args[1]);

    for(const auto& it : d_->observers_NtShutdownWorkerFactory)
        it(WorkerFactoryHandle, STARPendingWorkerCount);
}

bool monitor::GenericMonitor::register_NtSignalAndWaitForSingleObject(proc_t proc, const on_NtSignalAndWaitForSingleObject_fn& on_ntsignalandwaitforsingleobject)
{
    const auto ok = setup_func(proc, "NtSignalAndWaitForSingleObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSignalAndWaitForSingleObject.push_back(on_ntsignalandwaitforsingleobject);
    return true;
}

void monitor::GenericMonitor::on_NtSignalAndWaitForSingleObject()
{
    //LOG(INFO, "Break on NtSignalAndWaitForSingleObject");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto SignalHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto WaitHandle      = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtSignalAndWaitForSingleObject)
        it(SignalHandle, WaitHandle, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtSinglePhaseReject(proc_t proc, const on_NtSinglePhaseReject_fn& on_ntsinglephasereject)
{
    const auto ok = setup_func(proc, "NtSinglePhaseReject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSinglePhaseReject.push_back(on_ntsinglephasereject);
    return true;
}

void monitor::GenericMonitor::on_NtSinglePhaseReject()
{
    //LOG(INFO, "Break on NtSinglePhaseReject");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EnlistmentHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto TmVirtualClock  = nt::cast_to<nt::PLARGE_INTEGER>     (args[1]);

    for(const auto& it : d_->observers_NtSinglePhaseReject)
        it(EnlistmentHandle, TmVirtualClock);
}

bool monitor::GenericMonitor::register_NtStartProfile(proc_t proc, const on_NtStartProfile_fn& on_ntstartprofile)
{
    const auto ok = setup_func(proc, "NtStartProfile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtStartProfile.push_back(on_ntstartprofile);
    return true;
}

void monitor::GenericMonitor::on_NtStartProfile()
{
    //LOG(INFO, "Break on NtStartProfile");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtStartProfile)
        it(ProfileHandle);
}

bool monitor::GenericMonitor::register_NtStopProfile(proc_t proc, const on_NtStopProfile_fn& on_ntstopprofile)
{
    const auto ok = setup_func(proc, "NtStopProfile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtStopProfile.push_back(on_ntstopprofile);
    return true;
}

void monitor::GenericMonitor::on_NtStopProfile()
{
    //LOG(INFO, "Break on NtStopProfile");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProfileHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtStopProfile)
        it(ProfileHandle);
}

bool monitor::GenericMonitor::register_NtSuspendProcess(proc_t proc, const on_NtSuspendProcess_fn& on_ntsuspendprocess)
{
    const auto ok = setup_func(proc, "NtSuspendProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSuspendProcess.push_back(on_ntsuspendprocess);
    return true;
}

void monitor::GenericMonitor::on_NtSuspendProcess()
{
    //LOG(INFO, "Break on NtSuspendProcess");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtSuspendProcess)
        it(ProcessHandle);
}

bool monitor::GenericMonitor::register_NtSuspendThread(proc_t proc, const on_NtSuspendThread_fn& on_ntsuspendthread)
{
    const auto ok = setup_func(proc, "NtSuspendThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSuspendThread.push_back(on_ntsuspendthread);
    return true;
}

void monitor::GenericMonitor::on_NtSuspendThread()
{
    //LOG(INFO, "Break on NtSuspendThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto PreviousSuspendCount= nt::cast_to<nt::PULONG>             (args[1]);

    for(const auto& it : d_->observers_NtSuspendThread)
        it(ThreadHandle, PreviousSuspendCount);
}

bool monitor::GenericMonitor::register_NtSystemDebugControl(proc_t proc, const on_NtSystemDebugControl_fn& on_ntsystemdebugcontrol)
{
    const auto ok = setup_func(proc, "NtSystemDebugControl");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSystemDebugControl.push_back(on_ntsystemdebugcontrol);
    return true;
}

void monitor::GenericMonitor::on_NtSystemDebugControl()
{
    //LOG(INFO, "Break on NtSystemDebugControl");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Command         = nt::cast_to<nt::SYSDBG_COMMAND>     (args[0]);
    const auto InputBuffer     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InputBufferLength= nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutputBuffer    = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutputBufferLength= nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtSystemDebugControl)
        it(Command, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ReturnLength);
}

bool monitor::GenericMonitor::register_NtTerminateJobObject(proc_t proc, const on_NtTerminateJobObject_fn& on_ntterminatejobobject)
{
    const auto ok = setup_func(proc, "NtTerminateJobObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTerminateJobObject.push_back(on_ntterminatejobobject);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateJobObject()
{
    //LOG(INFO, "Break on NtTerminateJobObject");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto JobHandle       = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateJobObject)
        it(JobHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTerminateProcess(proc_t proc, const on_NtTerminateProcess_fn& on_ntterminateprocess)
{
    const auto ok = setup_func(proc, "NtTerminateProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTerminateProcess.push_back(on_ntterminateprocess);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateProcess()
{
    //LOG(INFO, "Break on NtTerminateProcess");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateProcess)
        it(ProcessHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTerminateThread(proc_t proc, const on_NtTerminateThread_fn& on_ntterminatethread)
{
    const auto ok = setup_func(proc, "NtTerminateThread");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTerminateThread.push_back(on_ntterminatethread);
    return true;
}

void monitor::GenericMonitor::on_NtTerminateThread()
{
    //LOG(INFO, "Break on NtTerminateThread");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ThreadHandle    = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto ExitStatus      = nt::cast_to<nt::NTSTATUS>           (args[1]);

    for(const auto& it : d_->observers_NtTerminateThread)
        it(ThreadHandle, ExitStatus);
}

bool monitor::GenericMonitor::register_NtTraceControl(proc_t proc, const on_NtTraceControl_fn& on_nttracecontrol)
{
    const auto ok = setup_func(proc, "NtTraceControl");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTraceControl.push_back(on_nttracecontrol);
    return true;
}

void monitor::GenericMonitor::on_NtTraceControl()
{
    //LOG(INFO, "Break on NtTraceControl");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FunctionCode    = nt::cast_to<nt::ULONG>              (args[0]);
    const auto InBuffer        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto InBufferLen     = nt::cast_to<nt::ULONG>              (args[2]);
    const auto OutBuffer       = nt::cast_to<nt::PVOID>              (args[3]);
    const auto OutBufferLen    = nt::cast_to<nt::ULONG>              (args[4]);
    const auto ReturnLength    = nt::cast_to<nt::PULONG>             (args[5]);

    for(const auto& it : d_->observers_NtTraceControl)
        it(FunctionCode, InBuffer, InBufferLen, OutBuffer, OutBufferLen, ReturnLength);
}

bool monitor::GenericMonitor::register_NtTraceEvent(proc_t proc, const on_NtTraceEvent_fn& on_nttraceevent)
{
    const auto ok = setup_func(proc, "NtTraceEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTraceEvent.push_back(on_nttraceevent);
    return true;
}

void monitor::GenericMonitor::on_NtTraceEvent()
{
    //LOG(INFO, "Break on NtTraceEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TraceHandle     = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto FieldSize       = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Fields          = nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_NtTraceEvent)
        it(TraceHandle, Flags, FieldSize, Fields);
}

bool monitor::GenericMonitor::register_NtTranslateFilePath(proc_t proc, const on_NtTranslateFilePath_fn& on_nttranslatefilepath)
{
    const auto ok = setup_func(proc, "NtTranslateFilePath");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTranslateFilePath.push_back(on_nttranslatefilepath);
    return true;
}

void monitor::GenericMonitor::on_NtTranslateFilePath()
{
    //LOG(INFO, "Break on NtTranslateFilePath");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto InputFilePath   = nt::cast_to<nt::PFILE_PATH>         (args[0]);
    const auto OutputType      = nt::cast_to<nt::ULONG>              (args[1]);
    const auto OutputFilePath  = nt::cast_to<nt::PFILE_PATH>         (args[2]);
    const auto OutputFilePathLength= nt::cast_to<nt::PULONG>             (args[3]);

    for(const auto& it : d_->observers_NtTranslateFilePath)
        it(InputFilePath, OutputType, OutputFilePath, OutputFilePathLength);
}

bool monitor::GenericMonitor::register_NtUnloadDriver(proc_t proc, const on_NtUnloadDriver_fn& on_ntunloaddriver)
{
    const auto ok = setup_func(proc, "NtUnloadDriver");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnloadDriver.push_back(on_ntunloaddriver);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadDriver()
{
    //LOG(INFO, "Break on NtUnloadDriver");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DriverServiceName= nt::cast_to<nt::PUNICODE_STRING>    (args[0]);

    for(const auto& it : d_->observers_NtUnloadDriver)
        it(DriverServiceName);
}

bool monitor::GenericMonitor::register_NtUnloadKey2(proc_t proc, const on_NtUnloadKey2_fn& on_ntunloadkey2)
{
    const auto ok = setup_func(proc, "NtUnloadKey2");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnloadKey2.push_back(on_ntunloadkey2);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKey2()
{
    //LOG(INFO, "Break on NtUnloadKey2");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);

    for(const auto& it : d_->observers_NtUnloadKey2)
        it(TargetKey, Flags);
}

bool monitor::GenericMonitor::register_NtUnloadKeyEx(proc_t proc, const on_NtUnloadKeyEx_fn& on_ntunloadkeyex)
{
    const auto ok = setup_func(proc, "NtUnloadKeyEx");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnloadKeyEx.push_back(on_ntunloadkeyex);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKeyEx()
{
    //LOG(INFO, "Break on NtUnloadKeyEx");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);
    const auto Event           = nt::cast_to<nt::HANDLE>             (args[1]);

    for(const auto& it : d_->observers_NtUnloadKeyEx)
        it(TargetKey, Event);
}

bool monitor::GenericMonitor::register_NtUnloadKey(proc_t proc, const on_NtUnloadKey_fn& on_ntunloadkey)
{
    const auto ok = setup_func(proc, "NtUnloadKey");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnloadKey.push_back(on_ntunloadkey);
    return true;
}

void monitor::GenericMonitor::on_NtUnloadKey()
{
    //LOG(INFO, "Break on NtUnloadKey");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto TargetKey       = nt::cast_to<nt::POBJECT_ATTRIBUTES> (args[0]);

    for(const auto& it : d_->observers_NtUnloadKey)
        it(TargetKey);
}

bool monitor::GenericMonitor::register_NtUnlockFile(proc_t proc, const on_NtUnlockFile_fn& on_ntunlockfile)
{
    const auto ok = setup_func(proc, "NtUnlockFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnlockFile.push_back(on_ntunlockfile);
    return true;
}

void monitor::GenericMonitor::on_NtUnlockFile()
{
    //LOG(INFO, "Break on NtUnlockFile");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto FileHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto IoStatusBlock   = nt::cast_to<nt::PIO_STATUS_BLOCK>   (args[1]);
    const auto ByteOffset      = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto Length          = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);
    const auto Key             = nt::cast_to<nt::ULONG>              (args[4]);

    for(const auto& it : d_->observers_NtUnlockFile)
        it(FileHandle, IoStatusBlock, ByteOffset, Length, Key);
}

bool monitor::GenericMonitor::register_NtUnlockVirtualMemory(proc_t proc, const on_NtUnlockVirtualMemory_fn& on_ntunlockvirtualmemory)
{
    const auto ok = setup_func(proc, "NtUnlockVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnlockVirtualMemory.push_back(on_ntunlockvirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtUnlockVirtualMemory()
{
    //LOG(INFO, "Break on NtUnlockVirtualMemory");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto STARBaseAddress = nt::cast_to<nt::PVOID>              (args[1]);
    const auto RegionSize      = nt::cast_to<nt::PSIZE_T>            (args[2]);
    const auto MapType         = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_NtUnlockVirtualMemory)
        it(ProcessHandle, STARBaseAddress, RegionSize, MapType);
}

bool monitor::GenericMonitor::register_NtUnmapViewOfSection(proc_t proc, const on_NtUnmapViewOfSection_fn& on_ntunmapviewofsection)
{
    const auto ok = setup_func(proc, "NtUnmapViewOfSection");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUnmapViewOfSection.push_back(on_ntunmapviewofsection);
    return true;
}

void monitor::GenericMonitor::on_NtUnmapViewOfSection()
{
    //LOG(INFO, "Break on NtUnmapViewOfSection");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtUnmapViewOfSection)
        it(ProcessHandle, BaseAddress);
}

bool monitor::GenericMonitor::register_NtVdmControl(proc_t proc, const on_NtVdmControl_fn& on_ntvdmcontrol)
{
    const auto ok = setup_func(proc, "NtVdmControl");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtVdmControl.push_back(on_ntvdmcontrol);
    return true;
}

void monitor::GenericMonitor::on_NtVdmControl()
{
    //LOG(INFO, "Break on NtVdmControl");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Service         = nt::cast_to<nt::VDMSERVICECLASS>    (args[0]);
    const auto ServiceData     = nt::cast_to<nt::PVOID>              (args[1]);

    for(const auto& it : d_->observers_NtVdmControl)
        it(Service, ServiceData);
}

bool monitor::GenericMonitor::register_NtWaitForDebugEvent(proc_t proc, const on_NtWaitForDebugEvent_fn& on_ntwaitfordebugevent)
{
    const auto ok = setup_func(proc, "NtWaitForDebugEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForDebugEvent.push_back(on_ntwaitfordebugevent);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForDebugEvent()
{
    //LOG(INFO, "Break on NtWaitForDebugEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto DebugObjectHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);
    const auto WaitStateChange = nt::cast_to<nt::PDBGUI_WAIT_STATE_CHANGE>(args[3]);

    for(const auto& it : d_->observers_NtWaitForDebugEvent)
        it(DebugObjectHandle, Alertable, Timeout, WaitStateChange);
}

bool monitor::GenericMonitor::register_NtWaitForKeyedEvent(proc_t proc, const on_NtWaitForKeyedEvent_fn& on_ntwaitforkeyedevent)
{
    const auto ok = setup_func(proc, "NtWaitForKeyedEvent");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForKeyedEvent.push_back(on_ntwaitforkeyedevent);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForKeyedEvent()
{
    //LOG(INFO, "Break on NtWaitForKeyedEvent");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto KeyedEventHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto KeyValue        = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[2]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[3]);

    for(const auto& it : d_->observers_NtWaitForKeyedEvent)
        it(KeyedEventHandle, KeyValue, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForMultipleObjects32(proc_t proc, const on_NtWaitForMultipleObjects32_fn& on_ntwaitformultipleobjects32)
{
    const auto ok = setup_func(proc, "NtWaitForMultipleObjects32");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForMultipleObjects32.push_back(on_ntwaitformultipleobjects32);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects32()
{
    //LOG(INFO, "Break on NtWaitForMultipleObjects32");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Handles         = nt::cast_to<nt::LONG>               (args[1]);
    const auto WaitType        = nt::cast_to<nt::WAIT_TYPE>          (args[2]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects32)
        it(Count, Handles, WaitType, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForMultipleObjects(proc_t proc, const on_NtWaitForMultipleObjects_fn& on_ntwaitformultipleobjects)
{
    const auto ok = setup_func(proc, "NtWaitForMultipleObjects");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForMultipleObjects.push_back(on_ntwaitformultipleobjects);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForMultipleObjects()
{
    //LOG(INFO, "Break on NtWaitForMultipleObjects");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Count           = nt::cast_to<nt::ULONG>              (args[0]);
    const auto Handles         = nt::cast_to<nt::HANDLE>             (args[1]);
    const auto WaitType        = nt::cast_to<nt::WAIT_TYPE>          (args[2]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[3]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[4]);

    for(const auto& it : d_->observers_NtWaitForMultipleObjects)
        it(Count, Handles, WaitType, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForSingleObject(proc_t proc, const on_NtWaitForSingleObject_fn& on_ntwaitforsingleobject)
{
    const auto ok = setup_func(proc, "NtWaitForSingleObject");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForSingleObject.push_back(on_ntwaitforsingleobject);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForSingleObject()
{
    //LOG(INFO, "Break on NtWaitForSingleObject");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto Handle          = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Alertable       = nt::cast_to<nt::BOOLEAN>            (args[1]);
    const auto Timeout         = nt::cast_to<nt::PLARGE_INTEGER>     (args[2]);

    for(const auto& it : d_->observers_NtWaitForSingleObject)
        it(Handle, Alertable, Timeout);
}

bool monitor::GenericMonitor::register_NtWaitForWorkViaWorkerFactory(proc_t proc, const on_NtWaitForWorkViaWorkerFactory_fn& on_ntwaitforworkviaworkerfactory)
{
    const auto ok = setup_func(proc, "NtWaitForWorkViaWorkerFactory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitForWorkViaWorkerFactory.push_back(on_ntwaitforworkviaworkerfactory);
    return true;
}

void monitor::GenericMonitor::on_NtWaitForWorkViaWorkerFactory()
{
    //LOG(INFO, "Break on NtWaitForWorkViaWorkerFactory");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);
    const auto MiniPacket      = nt::cast_to<nt::PFILE_IO_COMPLETION_INFORMATION>(args[1]);

    for(const auto& it : d_->observers_NtWaitForWorkViaWorkerFactory)
        it(WorkerFactoryHandle, MiniPacket);
}

bool monitor::GenericMonitor::register_NtWaitHighEventPair(proc_t proc, const on_NtWaitHighEventPair_fn& on_ntwaithigheventpair)
{
    const auto ok = setup_func(proc, "NtWaitHighEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitHighEventPair.push_back(on_ntwaithigheventpair);
    return true;
}

void monitor::GenericMonitor::on_NtWaitHighEventPair()
{
    //LOG(INFO, "Break on NtWaitHighEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWaitHighEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtWaitLowEventPair(proc_t proc, const on_NtWaitLowEventPair_fn& on_ntwaitloweventpair)
{
    const auto ok = setup_func(proc, "NtWaitLowEventPair");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWaitLowEventPair.push_back(on_ntwaitloweventpair);
    return true;
}

void monitor::GenericMonitor::on_NtWaitLowEventPair()
{
    //LOG(INFO, "Break on NtWaitLowEventPair");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto EventPairHandle = nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWaitLowEventPair)
        it(EventPairHandle);
}

bool monitor::GenericMonitor::register_NtWorkerFactoryWorkerReady(proc_t proc, const on_NtWorkerFactoryWorkerReady_fn& on_ntworkerfactoryworkerready)
{
    const auto ok = setup_func(proc, "NtWorkerFactoryWorkerReady");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWorkerFactoryWorkerReady.push_back(on_ntworkerfactoryworkerready);
    return true;
}

void monitor::GenericMonitor::on_NtWorkerFactoryWorkerReady()
{
    //LOG(INFO, "Break on NtWorkerFactoryWorkerReady");
    constexpr int nargs = 1;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto WorkerFactoryHandle= nt::cast_to<nt::HANDLE>             (args[0]);

    for(const auto& it : d_->observers_NtWorkerFactoryWorkerReady)
        it(WorkerFactoryHandle);
}

bool monitor::GenericMonitor::register_NtWriteFileGather(proc_t proc, const on_NtWriteFileGather_fn& on_ntwritefilegather)
{
    const auto ok = setup_func(proc, "NtWriteFileGather");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWriteFileGather.push_back(on_ntwritefilegather);
    return true;
}

void monitor::GenericMonitor::on_NtWriteFileGather()
{
    //LOG(INFO, "Break on NtWriteFileGather");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, SegmentArray, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtWriteFile(proc_t proc, const on_NtWriteFile_fn& on_ntwritefile)
{
    const auto ok = setup_func(proc, "NtWriteFile");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWriteFile.push_back(on_ntwritefile);
    return true;
}

void monitor::GenericMonitor::on_NtWriteFile()
{
    //LOG(INFO, "Break on NtWriteFile");
    constexpr int nargs = 9;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
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
        it(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
}

bool monitor::GenericMonitor::register_NtWriteRequestData(proc_t proc, const on_NtWriteRequestData_fn& on_ntwriterequestdata)
{
    const auto ok = setup_func(proc, "NtWriteRequestData");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWriteRequestData.push_back(on_ntwriterequestdata);
    return true;
}

void monitor::GenericMonitor::on_NtWriteRequestData()
{
    //LOG(INFO, "Break on NtWriteRequestData");
    constexpr int nargs = 6;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto PortHandle      = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto Message         = nt::cast_to<nt::PPORT_MESSAGE>      (args[1]);
    const auto DataEntryIndex  = nt::cast_to<nt::ULONG>              (args[2]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[3]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[4]);
    const auto NumberOfBytesWritten= nt::cast_to<nt::PSIZE_T>            (args[5]);

    for(const auto& it : d_->observers_NtWriteRequestData)
        it(PortHandle, Message, DataEntryIndex, Buffer, BufferSize, NumberOfBytesWritten);
}

bool monitor::GenericMonitor::register_NtWriteVirtualMemory(proc_t proc, const on_NtWriteVirtualMemory_fn& on_ntwritevirtualmemory)
{
    const auto ok = setup_func(proc, "NtWriteVirtualMemory");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtWriteVirtualMemory.push_back(on_ntwritevirtualmemory);
    return true;
}

void monitor::GenericMonitor::on_NtWriteVirtualMemory()
{
    //LOG(INFO, "Break on NtWriteVirtualMemory");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto ProcessHandle   = nt::cast_to<nt::HANDLE>             (args[0]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[1]);
    const auto Buffer          = nt::cast_to<nt::PVOID>              (args[2]);
    const auto BufferSize      = nt::cast_to<nt::SIZE_T>             (args[3]);
    const auto NumberOfBytesWritten= nt::cast_to<nt::PSIZE_T>            (args[4]);

    for(const auto& it : d_->observers_NtWriteVirtualMemory)
        it(ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesWritten);
}

bool monitor::GenericMonitor::register_NtDisableLastKnownGood(proc_t proc, const on_NtDisableLastKnownGood_fn& on_ntdisablelastknowngood)
{
    const auto ok = setup_func(proc, "NtDisableLastKnownGood");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtDisableLastKnownGood.push_back(on_ntdisablelastknowngood);
    return true;
}

void monitor::GenericMonitor::on_NtDisableLastKnownGood()
{
    //LOG(INFO, "Break on NtDisableLastKnownGood");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtDisableLastKnownGood)
        it();
}

bool monitor::GenericMonitor::register_NtEnableLastKnownGood(proc_t proc, const on_NtEnableLastKnownGood_fn& on_ntenablelastknowngood)
{
    const auto ok = setup_func(proc, "NtEnableLastKnownGood");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtEnableLastKnownGood.push_back(on_ntenablelastknowngood);
    return true;
}

void monitor::GenericMonitor::on_NtEnableLastKnownGood()
{
    //LOG(INFO, "Break on NtEnableLastKnownGood");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtEnableLastKnownGood)
        it();
}

bool monitor::GenericMonitor::register_NtFlushProcessWriteBuffers(proc_t proc, const on_NtFlushProcessWriteBuffers_fn& on_ntflushprocesswritebuffers)
{
    const auto ok = setup_func(proc, "NtFlushProcessWriteBuffers");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushProcessWriteBuffers.push_back(on_ntflushprocesswritebuffers);
    return true;
}

void monitor::GenericMonitor::on_NtFlushProcessWriteBuffers()
{
    //LOG(INFO, "Break on NtFlushProcessWriteBuffers");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtFlushProcessWriteBuffers)
        it();
}

bool monitor::GenericMonitor::register_NtFlushWriteBuffer(proc_t proc, const on_NtFlushWriteBuffer_fn& on_ntflushwritebuffer)
{
    const auto ok = setup_func(proc, "NtFlushWriteBuffer");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtFlushWriteBuffer.push_back(on_ntflushwritebuffer);
    return true;
}

void monitor::GenericMonitor::on_NtFlushWriteBuffer()
{
    //LOG(INFO, "Break on NtFlushWriteBuffer");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtFlushWriteBuffer)
        it();
}

bool monitor::GenericMonitor::register_NtGetCurrentProcessorNumber(proc_t proc, const on_NtGetCurrentProcessorNumber_fn& on_ntgetcurrentprocessornumber)
{
    const auto ok = setup_func(proc, "NtGetCurrentProcessorNumber");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtGetCurrentProcessorNumber.push_back(on_ntgetcurrentprocessornumber);
    return true;
}

void monitor::GenericMonitor::on_NtGetCurrentProcessorNumber()
{
    //LOG(INFO, "Break on NtGetCurrentProcessorNumber");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtGetCurrentProcessorNumber)
        it();
}

bool monitor::GenericMonitor::register_NtIsSystemResumeAutomatic(proc_t proc, const on_NtIsSystemResumeAutomatic_fn& on_ntissystemresumeautomatic)
{
    const auto ok = setup_func(proc, "NtIsSystemResumeAutomatic");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtIsSystemResumeAutomatic.push_back(on_ntissystemresumeautomatic);
    return true;
}

void monitor::GenericMonitor::on_NtIsSystemResumeAutomatic()
{
    //LOG(INFO, "Break on NtIsSystemResumeAutomatic");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtIsSystemResumeAutomatic)
        it();
}

bool monitor::GenericMonitor::register_NtIsUILanguageComitted(proc_t proc, const on_NtIsUILanguageComitted_fn& on_ntisuilanguagecomitted)
{
    const auto ok = setup_func(proc, "NtIsUILanguageComitted");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtIsUILanguageComitted.push_back(on_ntisuilanguagecomitted);
    return true;
}

void monitor::GenericMonitor::on_NtIsUILanguageComitted()
{
    //LOG(INFO, "Break on NtIsUILanguageComitted");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtIsUILanguageComitted)
        it();
}

bool monitor::GenericMonitor::register_NtQueryPortInformationProcess(proc_t proc, const on_NtQueryPortInformationProcess_fn& on_ntqueryportinformationprocess)
{
    const auto ok = setup_func(proc, "NtQueryPortInformationProcess");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtQueryPortInformationProcess.push_back(on_ntqueryportinformationprocess);
    return true;
}

void monitor::GenericMonitor::on_NtQueryPortInformationProcess()
{
    //LOG(INFO, "Break on NtQueryPortInformationProcess");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtQueryPortInformationProcess)
        it();
}

bool monitor::GenericMonitor::register_NtSerializeBoot(proc_t proc, const on_NtSerializeBoot_fn& on_ntserializeboot)
{
    const auto ok = setup_func(proc, "NtSerializeBoot");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtSerializeBoot.push_back(on_ntserializeboot);
    return true;
}

void monitor::GenericMonitor::on_NtSerializeBoot()
{
    //LOG(INFO, "Break on NtSerializeBoot");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtSerializeBoot)
        it();
}

bool monitor::GenericMonitor::register_NtTestAlert(proc_t proc, const on_NtTestAlert_fn& on_nttestalert)
{
    const auto ok = setup_func(proc, "NtTestAlert");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtTestAlert.push_back(on_nttestalert);
    return true;
}

void monitor::GenericMonitor::on_NtTestAlert()
{
    //LOG(INFO, "Break on NtTestAlert");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtTestAlert)
        it();
}

bool monitor::GenericMonitor::register_NtThawRegistry(proc_t proc, const on_NtThawRegistry_fn& on_ntthawregistry)
{
    const auto ok = setup_func(proc, "NtThawRegistry");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtThawRegistry.push_back(on_ntthawregistry);
    return true;
}

void monitor::GenericMonitor::on_NtThawRegistry()
{
    //LOG(INFO, "Break on NtThawRegistry");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtThawRegistry)
        it();
}

bool monitor::GenericMonitor::register_NtThawTransactions(proc_t proc, const on_NtThawTransactions_fn& on_ntthawtransactions)
{
    const auto ok = setup_func(proc, "NtThawTransactions");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtThawTransactions.push_back(on_ntthawtransactions);
    return true;
}

void monitor::GenericMonitor::on_NtThawTransactions()
{
    //LOG(INFO, "Break on NtThawTransactions");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtThawTransactions)
        it();
}

bool monitor::GenericMonitor::register_NtUmsThreadYield(proc_t proc, const on_NtUmsThreadYield_fn& on_ntumsthreadyield)
{
    const auto ok = setup_func(proc, "NtUmsThreadYield");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtUmsThreadYield.push_back(on_ntumsthreadyield);
    return true;
}

void monitor::GenericMonitor::on_NtUmsThreadYield()
{
    //LOG(INFO, "Break on NtUmsThreadYield");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtUmsThreadYield)
        it();
}

bool monitor::GenericMonitor::register_NtYieldExecution(proc_t proc, const on_NtYieldExecution_fn& on_ntyieldexecution)
{
    const auto ok = setup_func(proc, "NtYieldExecution");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_NtYieldExecution.push_back(on_ntyieldexecution);
    return true;
}

void monitor::GenericMonitor::on_NtYieldExecution()
{
    //LOG(INFO, "Break on NtYieldExecution");
    constexpr int nargs = 0;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    

    for(const auto& it : d_->observers_NtYieldExecution)
        it();
}

bool monitor::GenericMonitor::register_RtlpAllocateHeapInternal(proc_t proc, const on_RtlpAllocateHeapInternal_fn& on_rtlpallocateheapinternal)
{
    const auto ok = setup_func(proc, "RtlpAllocateHeapInternal");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlpAllocateHeapInternal.push_back(on_rtlpallocateheapinternal);
    return true;
}

void monitor::GenericMonitor::on_RtlpAllocateHeapInternal()
{
    //LOG(INFO, "Break on RtlpAllocateHeapInternal");
    constexpr int nargs = 2;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Size            = nt::cast_to<nt::SIZE_T>             (args[1]);

    for(const auto& it : d_->observers_RtlpAllocateHeapInternal)
        it(HeapHandle, Size);
}

bool monitor::GenericMonitor::register_RtlFreeHeap(proc_t proc, const on_RtlFreeHeap_fn& on_rtlfreeheap)
{
    const auto ok = setup_func(proc, "RtlFreeHeap");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlFreeHeap.push_back(on_rtlfreeheap);
    return true;
}

void monitor::GenericMonitor::on_RtlFreeHeap()
{
    //LOG(INFO, "Break on RtlFreeHeap");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_RtlFreeHeap)
        it(HeapHandle, Flags, BaseAddress);
}

bool monitor::GenericMonitor::register_RtlpReAllocateHeapInternal(proc_t proc, const on_RtlpReAllocateHeapInternal_fn& on_rtlpreallocateheapinternal)
{
    const auto ok = setup_func(proc, "RtlpReAllocateHeapInternal");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlpReAllocateHeapInternal.push_back(on_rtlpreallocateheapinternal);
    return true;
}

void monitor::GenericMonitor::on_RtlpReAllocateHeapInternal()
{
    //LOG(INFO, "Break on RtlpReAllocateHeapInternal");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto Size            = nt::cast_to<nt::ULONG>              (args[3]);

    for(const auto& it : d_->observers_RtlpReAllocateHeapInternal)
        it(HeapHandle, Flags, BaseAddress, Size);
}

bool monitor::GenericMonitor::register_RtlSizeHeap(proc_t proc, const on_RtlSizeHeap_fn& on_rtlsizeheap)
{
    const auto ok = setup_func(proc, "RtlSizeHeap");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlSizeHeap.push_back(on_rtlsizeheap);
    return true;
}

void monitor::GenericMonitor::on_RtlSizeHeap()
{
    //LOG(INFO, "Break on RtlSizeHeap");
    constexpr int nargs = 3;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);

    for(const auto& it : d_->observers_RtlSizeHeap)
        it(HeapHandle, Flags, BaseAddress);
}

bool monitor::GenericMonitor::register_RtlSetUserValueHeap(proc_t proc, const on_RtlSetUserValueHeap_fn& on_rtlsetuservalueheap)
{
    const auto ok = setup_func(proc, "RtlSetUserValueHeap");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlSetUserValueHeap.push_back(on_rtlsetuservalueheap);
    return true;
}

void monitor::GenericMonitor::on_RtlSetUserValueHeap()
{
    //LOG(INFO, "Break on RtlSetUserValueHeap");
    constexpr int nargs = 4;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto UserValue       = nt::cast_to<nt::PVOID>              (args[3]);

    for(const auto& it : d_->observers_RtlSetUserValueHeap)
        it(HeapHandle, Flags, BaseAddress, UserValue);
}

bool monitor::GenericMonitor::register_RtlGetUserInfoHeap(proc_t proc, const on_RtlGetUserInfoHeap_fn& on_rtlgetuserinfoheap)
{
    const auto ok = setup_func(proc, "RtlGetUserInfoHeap");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_RtlGetUserInfoHeap.push_back(on_rtlgetuserinfoheap);
    return true;
}

void monitor::GenericMonitor::on_RtlGetUserInfoHeap()
{
    //LOG(INFO, "Break on RtlGetUserInfoHeap");
    constexpr int nargs = 5;

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) { args.push_back(arg); return WALK_NEXT; });

    const auto HeapHandle      = nt::cast_to<nt::PVOID>              (args[0]);
    const auto Flags           = nt::cast_to<nt::ULONG>              (args[1]);
    const auto BaseAddress     = nt::cast_to<nt::PVOID>              (args[2]);
    const auto UserValue       = nt::cast_to<nt::PVOID>              (args[3]);
    const auto UserFlags       = nt::cast_to<nt::PULONG>             (args[4]);

    for(const auto& it : d_->observers_RtlGetUserInfoHeap)
        it(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
}
