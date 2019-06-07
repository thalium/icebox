include(AutoconfHelper)

ac_c_bigendian()
ac_check_headers(sys/types.h sys/stat.h stdlib.h string.h memory.h strings.h inttypes.h stdint.h unistd.h)

# libdwarf default-disabled shared
option(shared "build shared library libdwarf.so and use it if present" FALSE)

option(nonshared "build archive library libdwarf.a" TRUE)

#  This adds compiler option -Wall (gcc compiler warnings)
option(wall "Add -Wall" FALSE)
set(dwfwall $<$<BOOL:${wall}>:"-Wall -O0 -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes -Wdeclaration-after-statement -Wextra -Wcomment -Wformat -Wpedantic -Wuninitialized -Wno-long-long -Wshadow">)

option(nonstandardprintf "Use a special printf format for 64bit (default is NO)" FALSE)
set(HAVE_NONSTANDARD_PRINTF_64_FORMAT ${nonstandardprintf})
message(STATUS "Checking enable nonstandardprintf... ${HAVE_NONSTANDARD_PRINTF_64_FORMAT}")
