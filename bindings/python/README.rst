PyFDP
=============


PyFDP is a Python extension used to communicate with the FDP (Fast Debugging Protocol) hypervisor-based debugging server used in the `Winbagility <https://winbagility.github.io/>`_ project.

Winbagility introduced an instrumented version of VirtualBox which can be used to implement a sthealth debugger via Virtual Machine introspection and runtime analysis. While Winbagility simply connect the FDP server to Windbg in order to debug a Windows VM as if the guest was launch with /DEBUG option activated, anyone can write a FDP client. PyFDP expose the FDP client side by wrapping the DLL's exports via ctypes, enabling any Python program to script a VM debugging session.

PyFDP is pretty barebone since it's only a thin wrapper over FDP.dll exports. For the moment, it can only : 

* Read/Write registers and MSR
* Read/Write memory (virtual or physical)
* Save/Restore VM state, and listening to state's changes
* Set/unset breakpoints

If you want a "real" debugger (break on specific process names, on functions exports or symbols, etc.) you will need to write your own OS helpers.

Installation
-------------------------

PyFDP is on Pypi, so you can just install via pip :

::
    
    pip install PyFDP

If you want to install by source, clone `Winbagility git repository <https://github.com/Winbagility/Winbagility.git>`_  and type the following command :

::

    python setup.py install

PyFDP rely on FDP.dll, so you will need CMake and Visual Studio installed to build it, otherwise it does not rely on any other Python package.

By default, it use Visual Studio 2017 : if you want to use another (VS2015 and 2012 supported) you can do it by setting the environment variable FDP_MSVC_VER.

::
    
    PS >$env:FDP_MSVC_VER="2015"
    PS >python setup.py install
    running install
    running bdist_egg
    running egg_info
    writing PyFDP.egg-info\PKG-INFO
    writing top-level names to PyFDP.egg-info\top_level.txt
    writing dependency_links to PyFDP.egg-info\dependency_links.txt
    reading manifest file 'PyFDP.egg-info\SOURCES.txt'
    writing manifest file 'PyFDP.egg-info\SOURCES.txt'
    installing library code to build\bdist.win-amd64\egg
    running build_clib
    running PyFDPCustomBuildClib
    building FPD library with visual studio 2015
    ...

Example
--------

The following example place a breakpoint on **NtWriteFile** (the kernel syscall writing a file on disk) and read the written buffer. If it detects the buffer to be a valid **PNG** image, it also write the image on the host : 

::

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
                    
                    # Support <1MB write operations since it's highly likely to be a full image write
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


Development and support
-------------------------

Please use the `github issues <https://github.com/Winbagility/Winbagility/issues>`_ to ask general questions, make feature requests, report issues and bugs, and to send in patches. 

Main documentation is at `winbagility.github.io <https://winbagility.github.io/>`_, and in this presentation paper (in FR) `Winbagility : Débogage furtif et introspection de
machine virtuelle <https://www.sstic.org/media/SSTIC2016/SSTIC-actes/debogage_furtif_et_introspection_de_machines_virtu/SSTIC2016-Article-debogage_furtif_et_introspection_de_machines_virtuelles-couffin.pdf>`_ and `associated slides <https://www.sstic.org/media/SSTIC2016/SSTIC-actes/debogage_furtif_et_introspection_de_machines_virtu/SSTIC2016-Slides-debogage_furtif_et_introspection_de_machines_virtuelles-couffin.pdf>`_ .


Requirements
--------------

PyFDP should run on any Python 2 and 3 interpreter running on Windows (No Linux port yet).