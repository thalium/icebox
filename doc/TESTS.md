## Common statements

After building, tests can be launch with the _icebox_tests_ binary.

<br>

### Linux Guest

To launch linux tests specifically, use command line _icebox_tests --gtest_filter=Linux*_

You must have have a linux guest running in the modified VirtualBox. The VM must be named "linux".

Icebox should have the right profil of the linux kernel running as guest. See doc/make_profil_linux for more details.

#### VM prerequisites
None