import struct
import time
import sys

from PyFDP.FDP import FDP

if __name__ == '__main__':
    fdp = FDP("10_RS1_64")
    
    fdp.Pause()
    fdp.UnsetAllBreakpoint()
    fdp.WriteRegister(FDP_CPU0, FDP_DR7_REGISTER, 0x400)
    
    fdp.SetBreakpoint(FDP_CPU0, FDP_CRHBP, 0, FDP_WRITE_BP, FDP_VIRTUAL_ADDRESS, 3, 1, FDP_NO_CR3)
    fdp.Resume()
    
    SyscallCount = 0
    OldTime = time.time()
    while True:
         if fdp.WaitForStateChanged() & FDP_STATE_BREAKPOINT_HIT:
            #DO STUFF
            Cr3 = fdp.ReadRegister(FDP_CPU0, FDP_CR3_REGISTER)
            print "%016x" % (Cr3)
            #print "."
            SyscallCount = SyscallCount + 1
            if SyscallCount > 100000:
                NewTime = time.time()
                print int(SyscallCount / (NewTime -OldTime))
                OldTime = NewTime
                SyscallCount = 0
            fdp.SingleStep(FDP_CPU0)
            fdp.Resume()