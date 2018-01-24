import struct

from PyFDP.FDP import FDP

if __name__ == '__main__':
    fdp = FDP("7_SP1_x64")
    
    NtWriteFile = 0xfffff800029ee9a0
    
    fdp.Pause()
    fdp.UnsetAllBreakpoint()
    fdp.WriteRegister(FDP_CPU0, FDP_DR7_REGISTER, 0x400)
    fdp.SetBreakpoint(FDP_CPU0, FDP_SOFTHBP, 0, FDP_EXECUTE_BP, FDP_VIRTUAL_ADDRESS, NtWriteFile, 1)
    fdp.Resume()
    
    while True:
         if fdp.WaitForStateChanged() & FDP_STATE_BREAKPOINT_HIT:
            print ".",
            Rsp = fdp.ReadRegister(FDP_CPU0, FDP_RSP_REGISTER)
            BufferPtr = fdp.ReadVirtualMemory64(FDP_CPU0, Rsp+(6*8))
            #print "%x" % (BufferPtr)

            BufferSize = fdp.ReadVirtualMemory32(FDP_CPU0, Rsp+(7*8))
            #print "%x" % (BufferSize)
            
            if BufferSize > 3 and BufferSize < FDP_1M:
                Buffer = fdp.ReadVirtualMemory(FDP_CPU0, BufferPtr, BufferSize)
                if Buffer != None and Buffer[1] == 'P' and Buffer[2] == 'N' and Buffer[3] == 'G': 
                    f = open("./test.png", "wb")
                    f.write(Buffer)
                    f.close()
                    print "\nFile Written"
            
            fdp.SingleStep(FDP_CPU0)
            fdp.Resume()