import os
import sys
import ctypes

if not sys.platform == "win32":
    raise ValueError("The binding PyFDP cannot be used on a host that is not a native Windows machine")




# get __file__ path
import inspect
if not hasattr(sys.modules[__name__], '__file__'):
    __file__ = inspect.getfile(inspect.currentframe())


# Try to call LoadLibrary on the correct FDP dll file
_found = False
FDP_DLL_HANDLE = None
package_dir = os.path.dirname(__file__)
try:
    is_64bits = sys.maxsize > 2**32
    if is_64bits:
        _FDP_dll_name = "FDP_x64.dll"
    else:
        _FDP_dll_name = "FDP_x86.dll"

    
    _FDP_dll_path = os.path.join(package_dir, _FDP_dll_name)
    FDP_DLL_HANDLE = ctypes.cdll.LoadLibrary(_FDP_dll_path)
    _found = True
except OSError:
    pass


if _found == False or FDP_DLL_HANDLE == None:
    raise ImportError("ERROR: fail to load the dynamic library %s." % _FDP_dll_path)