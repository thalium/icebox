[![Travis Build Status](https://travis-ci.org/dvirtz/libdwarf.svg?branch=cmake)](https://travis-ci.org/dvirtz/libdwarf)
[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/oxh8pg7hsuav2jrl?svg=true)](https://ci.appveyor.com/project/dvirtz/libdwarf)

# This is README.md
## BUILDING

To just build libdwarf and dwarfdump, if the source tree is in `/a/b/libdwarf-1`

### Using CMake

August 23 2018 and the following seems to work:
   rm -rf /tmp/bld
   mkdir /tmp/bld
   cd /tmp/bld
   cmake /a/b/libdwarf-1

# The following does not necessarily work. ?
To build using CMake one might do
* `cd /a/b/libdwarf-1`
* configure: `cmake . -B_Release -DCMAKE_BUILD_TYPE=Release`
* build: `cmake --build _Release --target dd`
* (optionally install): `sudo cmake --build _Release --target install`

# for autotools builds, see README


