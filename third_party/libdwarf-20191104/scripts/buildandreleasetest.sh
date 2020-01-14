#!/bin/sh
#  A script verifying the distribution gets all needed files
#  for building, including 'make check'
# First, get the current configure.ac version into v:

# if stdint.h does not define uintptr_t and intptr_t
# Then dwarfgen (being c++) will not build
# Use --disable-libelf to disable reliance on libelf
# and dwarfgen.
# To just eliminate dwarfgen build/test/install use --disable-dwarfgen.
genopta="--enable-dwarfgen"
genoptb="-DBUILD_DWARFGEN=ON"
libelfopt=''
wd=`pwd`
if [ $# -gt 0 ]
then
  case $1 in
   --disable-libelf ) genopta='' ; genoptb='' 
        libelfopt=$1 ; shift ;;
   --enable-libelf )  shift  ;;
   --disable-dwarfgen ) genopta='' ; genoptb='' ; shift  ;;
   * ) echo "Unknown option $v. Error." ; exit 1 ;;
  esac
fi

if [ -f ./configure.ac ]
then
  f=./configure.ac
else
  if [ -f ../configure.ac ]
  then 
    f=../configure.ac
  else
    echo "FAIL Running distribution test from the wrong place."
    exit 
  fi
fi
v=`grep -o '201[0-9][0-9][0-9][0-9][0-9]'< $f | head -n 1`

if [ x$v = "x" ]
then
   echo FAIL did not get configure.ac version
   exit 1
fi
configloc=$wd/configure


rm -rf /tmp/dwbld
rm -rf /tmp/dwinstall
rm -rf /tmp/dwinstallrel
rm -rf /tmp/dwinstallrelbld
rm -rf /tmp/dwbigendianbld
rm -rf /tmp/dwinstallrelbldall
rm -f /tmp/dwrelease.tar.gz
rm -rf /tmp/dwreleasebld
rm -rf /tmp/cmakebld
mkdir  /tmp/dwbld
if [ $? -ne 0 ] 
then
   echo FAIL A1 mkdir
   exit 1
fi
mkdir /tmp/dwinstall
if [ $? -ne 0 ] 
then
   echo FAIL A2 mkdir
   exit 1
fi
mkdir /tmp/dwinstallrel
if [ $? -ne 0 ] 
then
   echo FAIL A3 mkdir
   exit 1
fi
mkdir /tmp/dwinstallrelbld
if [ $? -ne 0 ] 
then
   echo FAIL A3b mkdir
   exit 1
fi
mkdir /tmp/dwinstallrelbldall
if [ $? -ne 0 ] 
then
   echo FAIL A3c mkdir /tmp/dwinstallrelbldall
   exit 1
fi

mkdir /tmp/dwbigendianbld
if [ $? -ne 0 ] 
then
   echo FAIL A3bigend mkdir /tmp/dwbigendianbld
   exit 1
fi

mkdir /tmp/cmakebld
if [ $? -ne 0 ] 
then
   echo FAIL A3d mkdir /tmp/cmakebld
   exit 1
fi

echo "dirs created empty"
echo cd /tmp/dwbld
cd /tmp/dwbld
if [ $? -ne 0 ]
then
  echo FAIL A cd $v
      exit 1
fi
echo "now: $configloc --prefix=/tmp/dwinstall $libelfopt"
$configloc --prefix=/tmp/dwinstall $libelfopt
if [ $? -ne 0 ]
then
  echo FAIL A4a configure fail
  exit 1
fi
echo "TEST: initial (dwinstall) make install"
make install
if [ $? -ne 0 ]
then
  echo FAIL A4b make install 
  exit 1
fi
ls -lR /tmp/dwinstall
make dist
ls -1 *tar.gz >/tmp/dwrelset
ct=`wc < /tmp/dwrelset`
echo "count of gz files $ct"
cp *.tar.gz /tmp/dwrelease.tar.gz
cd /tmp
if [ $? -ne 0 ]
then
  echo FAIL B2  cd /tmp
  exit 1
fi
tar -zxf /tmp/dwrelease.tar.gz
ls -d *dw*
################
echo "TEST: now cd libdwarf-$v for second build install"
cd /tmp/dwinstallrelbld
if [ $? -ne 0 ]
then
  echo FAIL C cd /tmp/dwinstallrelbld
      exit 1
fi
echo "TEST: now second install install, prefix /tmp/dwinstallrel"
echo "TEST: Expecting src in /tmp/libdwarf-$v"
/tmp/libdwarf-$v/configure --enable-wall --prefix=/tmp/dwinstallrel $libelfopt
if [ $? -ne 0 ]
then
  echo FAIL C2  configure fail
  exit 1
fi
echo "TEST: In dwinstallrelbld make install from /tmp/libdwarf-$v/configure"
make install
if [ $? -ne 0 ]
then
  echo FAIL C3  final install fail
  exit 1
fi
ls -lR /tmp/dwinstallrel
echo "TEST: Now lets see if make check works"
make check
if [ $? -ne 0 ]
then
  echo FAIL make check C4 
  exit 1
fi
################

################
echo "TEST: now cd libdwarf-$v for big-endian build (not runnable) "

cd /tmp/dwbigendianbld
if [ $? -ne 0 ]
then
  echo FAIL C be1 /tmp/dwbigendianbld
  exit 1
fi
echo "TEST: now second install install, prefix /tmp/dwinstallrel"
echo "TEST: Expecting src in /tmp/libdwarf-$v"
echo "TEST: /tmp/libdwarf-$v/configure $genopta --enable-wall --enable-dwarfexample --prefix=/tmp/dwinstallrel $libelfopt"
/tmp/libdwarf-$v/configure $genopta --enable-wall --enable-dwarfexample --prefix=/tmp/dwinstallrel $libelfopt
if [ $? -ne 0 ]
then
  echo FAIL be2  configure fail
  exit 1
fi
echo "#define WORDS_BIGENDIAN 1" >> config.h
echo "TEST: Compile In dwbigendianbld make from /tmp/libdwarf-$v/configure"
make
if [ $? -ne 0 ]
then
  echo FAIL be3  Build failed
  exit 1
fi
################



cd /tmp/dwinstallrelbldall
if [ $? -ne 0 ]
then
  echo FAIL Ca cd /dwinstallrelbldall
  exit 1
fi
echo "TEST: Now configure from source dir /tmp/libdwarf-$v/ in build dir /tmp/dwinstallrelbldall"
/tmp/libdwarf-$v/configure --enable-wall --enable-dwarfexample $genopta
if [ $? -ne 0 ]
then
  echo FAIL C9  /tmp/libdwarf-$v/configure 
  exit 1
fi
make
if [ $? -ne 0 ]
then
  echo FAIL C9  /tmp/libdwarf-$v/configure  make
  exit 1
fi
cd /tmp/cmakebld
if [ $? -ne 0 ]
then
  echo FAIL C10  cd /tmp/cmakebld
  exit 1
fi

havecmake=n
which cmake >/dev/null
if [ $? -eq 0 ]
then
  havecmake=y
  echo "We have cmake and can test it."
fi

if [ $havecmake = "y" ]
then
  echo "TEST: Now cmake from source dir /tmp/libdwarf-$v/ in build dir /tmp/cmakebld"
  cmake $genoptb -DWALL=ON -DBUILD_DWARFEXAMPLE=ON -DDO_TESTING=ON /tmp/libdwarf-$v/
  if [ $? -ne 0 ]
  then
    echo "FAIL C10b  cmake in /tmp/cmakebld"
    exit 1
  fi
  make
  if [ $? -ne 0 ]
  then
    echo "FAIL C10c  cmake make in /tmp/cmakebld"
    exit 1
  fi
  make test
  if [ $? -ne 0 ]
  then
    echo "FAIL C10d  cmake make test in /tmp/cmakebld"
    exit 1
  fi
  ctest -R self
  if [ $? -ne 0 ]
  then
    echo "FAIL C10e  ctest -R self in /tmp/cmakebld"
    exit 1
  fi
else
  echo "cmake is not installed so not tested."
fi
exit 0
