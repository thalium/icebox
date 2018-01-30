from __future__ import print_function
import struct
import argparse
import logging

from PyFDP.FDP import FDP

# Warning : those offsets are only valid for Windows 7 SP1 x64
# see http://terminus.rewolf.pl/terminus/structures/ntdll/_EPROCESS_x64.html to extend this script for other platforms
# kd> dt _EPROCESS
# nt!_EPROCESS
#    +0x000 Pcb              : _KPROCESS
#    +0x160 ProcessLock      : _EX_PUSH_LOCK
#    +0x168 CreateTime       : _LARGE_INTEGER
#    +0x170 ExitTime         : _LARGE_INTEGER
#    +0x178 RundownProtect   : _EX_RUNDOWN_REF
#    +0x180 UniqueProcessId  : Ptr64 Void
#    +0x188 ActiveProcessLinks : _LIST_ENTRY <---- Next process
#    +0x198 ProcessQuotaUsage : [2] Uint8B
#    +0x1a8 ProcessQuotaPeak : [2] Uint8B
#    ...
#    ...
#    +0x2d8 Session          : Ptr64 Void
#    +0x2e0 ImageFileName    : [15] UChar <---- Process Name
#    +0x2ef PriorityClass    : UChar
#    +0x2f0 JobLinks         : _LIST_ENTRY
#    +0x300 LockedPagesList  : Ptr64 Void
EPROCESS_ACTIVEPROCESSLIST_OFF = 0x188
EPROCESS_PROCESSNAME_OFF = 0x2E0
EPROCESS_PROCESSNAME_SIZE = 15

def read_ptr(fdp, address):
    return struct.unpack("Q", fdp.ReadVirtualMemory(address, 8))[0]

if __name__ == '__main__':

    parser = argparse.ArgumentParser("NoCalc script : restore a particular VM to a safe state whenever the guest user tries to launch calc.exe :p")
    parser.add_argument("--vm", type=str, help="Name of target VM registered in VBox. Must be a Windows 7 SP1 x64 (rely on undocumented structures).")
    parser.add_argument("--nt-active-process-head", type=str, help="address for the head of process list (type 'x nt!PsActiveProcessHead' in windbg to get it).")

    args = parser.parse_args()
    fdp = FDP(args.vm)

    
    print ("Saving safe state...", end="")
    fdp.Pause()
    fdp.Save()
    fdp.Resume()
    print ("[OK]")
    

    try:
        while True:
            
            PsActiveProcessHead = int(args.nt_active_process_head,16)
            FirstProcess = read_ptr(fdp, PsActiveProcessHead)
            CurrentProcess = FirstProcess - EPROCESS_ACTIVEPROCESSLIST_OFF;

            while True:

                CurrentProcessName = fdp.ReadVirtualMemory(CurrentProcess + EPROCESS_PROCESSNAME_OFF, EPROCESS_PROCESSNAME_SIZE)
                if CurrentProcessName == None or CurrentProcessName[0] == 0:
                    break;

                if CurrentProcessName.startswith(b"calc.exe") == True:
                    print ("Restoring to safe state...", end="")
                    fdp.Pause()
                    fdp.Restore()
                    fdp.Resume()
                    print ("[OK]")
                    break;

                CurrentProcess = read_ptr(fdp, CurrentProcess + EPROCESS_ACTIVEPROCESSLIST_OFF)
                if CurrentProcess == None or CurrentProcess == FirstProcess:
                    break;

                CurrentProcess = CurrentProcess - EPROCESS_ACTIVEPROCESSLIST_OFF

    except KeyboardInterrupt as Ki:
        print("clearing every breakpoints")
        fdp.Pause()
        fdp.UnsetAllBreakpoint()
    finally:
        print("resuming the vm")
        fdp.Resume()