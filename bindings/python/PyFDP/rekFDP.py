import urllib

from rekall import addrspace
from rekall import yaml_utils
from rekall.plugins.addrspaces import amd64

from PyFDP import FDP


class PyFDPAddressSpace(amd64.AMD64PagedMemory):
    """
    FDP AddressSpace for Rekall
    """
    __name = "rekFDP"

    # We should be the AS of last resort
    order = 90

    # This address space handles images.
    __image = True

    def __init__(self, base = None, session = None, filename=None, **kwargs):
        self.as_assert(base == None, 'Must be first Address Space')
        path = filename or (session and session.GetParameter("filename"))
        self.as_assert(path, "Filename must be specified in session (e.g. "
                       "session.SetParameter('filename', 'MyFile.raw').")
        self.as_assert(path.startswith("fdp://"),
                        "Location doesn't start with fdp://")
        self.name = urllib.url2pathname(path[6:])
        self.fdp = FDP.FDP(self.name)
        self.as_assert(not self.fdp is None, "VM not found")
        Cr3 = self.fdp.ReadRegister(0, FDP_CR3_REGISTER)
        session.SetParameter("dtb", Cr3)
        self.PhysicalMemorySize = self.fdp.GetPhysicalMemorySize()
        super(PyFDPAddressSpace, self).__init__(base=self, dtb = Cr3, session = session, **kwargs)

    def iread(self, PhysicalAddress, ReadSize):
        return self.fdp.ReadPhysicalMemory(PhysicalAddress, ReadSize)

    def read(self, PhysicalAddress, ReadSize):
        Buffer = self.iread(PhysicalAddress, ReadSize)
        if Buffer is None:
            Buffer = "\x00" * ReadSize
        elif len(Buffer) != ReadSize:
            Buffer += "\x00" * (ReadSize - len(Buffer))
        return Buffer
        
    def is_valid_address(self, PhysicalAddress):
        if PhysicalAddress == None:
            return False
        return 0 <= PhysicalAddress < self.PhysicalMemorySize

    def get_address_ranges(self):
        yield Run(0, self.PhysicalMemorySize)