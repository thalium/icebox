import struct

from PyFDP.FDP import FDP
    
if __name__ == '__main__':
    #fdp = FDP("7_SP1_x64")
    #fdp = FDP("8_1_x64")
    #fdp = FDP("10_x64")
    fdp = FDP("10_RS1_64")
    
    fdp.Pause()
    fdp.UnsetAllBreakpoint()
    fdp.WriteRegister(FDP_CPU0, FDP_DR0_REGISTER, 0)
    fdp.WriteRegister(FDP_CPU0, FDP_DR1_REGISTER, 0)
    fdp.WriteRegister(FDP_CPU0, FDP_DR2_REGISTER, 0)
    fdp.WriteRegister(FDP_CPU0, FDP_DR7_REGISTER, 0x400)
    fdp.Resume()
        