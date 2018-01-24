from FDP import *
import struct

#7_SP1_x64

EPROCESS_ACTIVEPROCESSLIST_OFF = 0x188
EPROCESS_PROCESSNAME_OFF = 0x2E0
EPROCESS_PROCESSNAME_SIZE = 15

if __name__ == '__main__':
    fdp = FDP("7_SP1_x64")
    
    print "Saving safe state...",
    fdp.Pause()
    fdp.Save()
    fdp.Resume()
    print "[OK]"
    
    while True:
        PsActiveProcessHead = 0xfffff80002883b90
        FirstProcess = fdp.ReadVirtualMemory64(FDP_CPU0, PsActiveProcessHead)
        CurrentProcess = FirstProcess - EPROCESS_ACTIVEPROCESSLIST_OFF;
        while True:
            CurrentProcessName = fdp.ReadVirtualMemory(FDP_CPU0, CurrentProcess + EPROCESS_PROCESSNAME_OFF, EPROCESS_PROCESSNAME_SIZE)
            if CurrentProcessName == None or CurrentProcessName[0] == 0:
                break;
            
            if CurrentProcessName.startswith("calc.exe") == True:
                print "Restoring to safe state...",
                fdp.Pause()
                fdp.Restore()
                fdp.Resume()
                print "[OK]"
                break;

            CurrentProcess = fdp.ReadVirtualMemory64(FDP_CPU0, CurrentProcess + EPROCESS_ACTIVEPROCESSLIST_OFF)
            if CurrentProcess == None or CurrentProcess == FirstProcess:
                break;
            CurrentProcess = CurrentProcess - EPROCESS_ACTIVEPROCESSLIST_OFF