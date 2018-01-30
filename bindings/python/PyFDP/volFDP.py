import urllib

import volatility.addrspace as addrspace

from PyFDP import FDP

class PyFDPAddressSpace(addrspace.BaseAddressSpace):
    """
    FDP AddressSpace for volatility
    """

    order = 1

    def __init__(self, base, config, layered=False, **kwargs):
        addrspace.BaseAddressSpace.__init__(self, base, config, **kwargs)
        self.as_assert(base == None or layered, "Must be first Address Space")
        self.as_assert(
                config.LOCATION.startswith("fdp://"),
                "Location doesn't start with fdp://")
        self.config = dict(inittype="partial")
        self.name = urllib.url2pathname(config.LOCATION[6:])
        self.config['name'] = self.name
        self.fdp = FDP.FDP(self.config['name'])
        self.as_assert(not self.fdp is None, "VM not found")
        self.dtb = self.fdp.ReadRegister(0, FDP_CR3_REGISTER)
        self.PhysicalMemorySize = self.fdp.GetPhysicalMemorySize()

    def read(self, PhysicalAddress, ReadSize):
        return self.fdp.ReadPhysicalMemory(PhysicalAddress, ReadSize)

    def zread(self, PhysicalAddress, ReadSize):
        Buffer = self.read(PhysicalAddress, ReadSize)
        if Buffer is None:
            Buffer = "\x00" * ReadSize
        elif len(Buffer) != ReadSize:
            Buffer += "\x00" * (ReadSize - len(Buffer))
        return Buffer

    def is_valid_address(self, PhysicalAddress):
        if PhysicalAddress == None:
            return False
        return 0 <= PhysicalAddress < self.PhysicalMemorySize

    def write(self, PhysicalAddress, Buffer):
        return False

    def get_cr3(self):
        return self.dtb

    def get_available_addresses(self):
        yield (0, self.PhysicalMemorySize)