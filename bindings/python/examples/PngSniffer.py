from __future__ import print_function
import struct
import argparse
import logging

from PyFDP.FDP import FDP


FDP_1M = 1*1024*1024
FDP_ADDRESS_SIZE = 8 # assume x64 systems

PNG_HEADER = b'\x89PNG\r\n\x1a\n'

def read_ptr(fdp, address):
    return struct.unpack("Q", fdp.ReadVirtualMemory(address, FDP_ADDRESS_SIZE))[0]


if __name__ == '__main__':

    parser = argparse.ArgumentParser("PngSniffer script : exfil to the host any png file opened in the guest VM")
    parser.add_argument("--vm", type=str, help="Name of target VM registered in VBox. Must be a Windows 7 SP1 x64 (rely on hardcoded offsets).")
    parser.add_argument("--nt-writefile", type=str, help="address to nt!NtWriteFile.")
    parser.add_argument("--file", default = "./test.png", type=str, help="filepath where to write the png file sniffed.")
    parser.add_argument('-v', '--verbose', action='store_true', help='Add verbose output')

    args = parser.parse_args()
    fdp = FDP(args.vm)

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG, format="%(levelname)s:%(message)s")
        logging.debug("Debug log: ON")
    else:
        logging.basicConfig(level=logging.INFO, format="%(message)s")


    # works on 7_SP1_x64    
    #  kd> x nt!NtWriteFile
    #  fffff800`029ab9a0 nt!NtWriteFile (<no parameter info>)
    NtWriteFile = int (args.nt_writefile, 16)

    
    # Place breakpoint on NtWriteFile
    fdp.Pause()
    fdp.UnsetAllBreakpoint()
    # fdp.WriteRegister(FDP.FDP_DR7_REGISTER, 0x400)
    fdp.SetBreakpoint(FDP.FDP_SOFTHBP, 0, FDP.FDP_EXECUTE_BP, FDP.FDP_VIRTUAL_ADDRESS, NtWriteFile, 1, 0)
    fdp.Resume()
    
    try:
        while True:
             if fdp.WaitForStateChanged() & FDP.FDP_STATE_BREAKPOINT_HIT:
                logging.info(".")

                # Read 6 and 7th args placed on the stack
                # NTSTATUS ZwWriteFile(
                #   _In_     HANDLE           FileHandle,
                #   _In_opt_ HANDLE           Event,
                #   _In_opt_ PIO_APC_ROUTINE  ApcRoutine,
                #   _In_opt_ PVOID            ApcContext,
                #   _Out_    PIO_STATUS_BLOCK IoStatusBlock,
                #   _In_     PVOID            Buffer,
                #   _In_     ULONG            Length,
                #   _In_opt_ PLARGE_INTEGER   ByteOffset,
                #   _In_opt_ PULONG           Key
                # );
                BufferPtr = read_ptr(fdp, fdp.rsp+6*FDP_ADDRESS_SIZE)
                BufferSize = read_ptr(fdp, fdp.rsp+7*FDP_ADDRESS_SIZE)
                logging.debug("buffer %x (size %x)" % (BufferPtr, BufferSize))
                
                if BufferSize > len(PNG_HEADER) and BufferSize < FDP_1M:
                    Buffer = fdp.ReadVirtualMemory(BufferPtr, BufferSize)
                    if Buffer != None:
                        logging.debug(Buffer[0:min(BufferSize, 20)])
                        if Buffer.startswith(PNG_HEADER):
                            logging.info ("Write image to  %s" % args.file)
                            with  open(args.file, "wb") as out_png:
                                out_png.write(Buffer)
                            logging.info ("Image written to  %s" % args.file)
                
                fdp.SingleStep()
                fdp.Resume()
    except KeyboardInterrupt as Ki:
        logging.info("clearing every breakpoints")
        fdp.Pause()
        fdp.UnsetAllBreakpoint()
    finally:
        logging.info("resuming the vm")
        fdp.Resume()