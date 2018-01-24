import os
import sys
import shutil
import setuptools

from distutils import log
from distutils.command.build_clib import build_clib

VERSION_MAJOR = 0
VERSION_MINOR = 1
VERSION_VBOXBIN = "5-0-18-6667"
VERSION = "{major:d}.{minor:d}.{vbox:s}".format(
    major = VERSION_MAJOR,
    minor = VERSION_MINOR,
    vbox = VERSION_VBOXBIN,
)

BINDING_DIRECTORY =  os.path.abspath(os.path.dirname(__file__))
ROOT_DIRECTORY = os.path.abspath(os.path.join(BINDING_DIRECTORY, "../../"))

# path to compiled libraries for Windows
PATH_LIB64 = os.path.join(ROOT_DIRECTORY, "out_x64/Release/FDP_x64.dll")
PATH_LIB32 = os.path.join(ROOT_DIRECTORY, "out_x86/Release/FDP_x86.dll")

FDP_BUILD_SCRIPT = lambda VS_VERSION : os.path.join(ROOT_DIRECTORY, "buildVS%s.bat" % VS_VERSION)
VS_VERSIONS = [
    "2017",
    "2015",
    "2013"
]

class PyFDPCustomBuildClib(build_clib):
    """ Customized build_clib command. """
    description = "Custom build lib step used to compile the FDP client dll from sources."
    

    def run(self):
        log.info('running PyFDPCustomBuildClib')
        build_clib.run(self)

    def initialize_options(self):
        """ Initialize custom command line switches """
        build_clib.initialize_options(self)
        
        # Visual Studio version
        # Customize it by setting the env variable FDP_MSVC_VER
        self.vs = os.environ.get("FDP_MSVC_VER", "2017")


    def finalize_options(self):
        """ Validate custom command line switches values """
        build_clib.finalize_options(self)

    def build_libraries(self, libraries):
        
        # platform description refers at https://docs.python.org/2/library/sys.html#sys.platform
        if not sys.platform == "win32":
            raise ValueError("Can not build on a platform that is not native Windows")
        

        assert self.vs in VS_VERSIONS, 'Unrecognized visual studio compiler version. Supported versions : %s ' % ",".join(VS_VERSIONS)
        log.info("building FPD library with visual studio %s " % self.vs)

        _cwd = os.getcwd()
        os.chdir(ROOT_DIRECTORY)

        # Cleanup old build folders
        shutil.rmtree(os.path.join(ROOT_DIRECTORY, "out_x64"), ignore_errors=True)
        shutil.rmtree(os.path.join(ROOT_DIRECTORY, "out_x86"), ignore_errors=True)

        # Windows build: this process requires few things:
        #    - CMake + MSVC installed
        #    - Run this command in an environment setup for MSVC
        os.system(FDP_BUILD_SCRIPT(self.vs))
        if not os.path.exists(PATH_LIB64):
            raise FileNotFoundError("Could not find %s : compilation step went wrong", PATH_LIB64)
        if not os.path.exists(PATH_LIB32):
            raise FileNotFoundError("Could not find %s : compilation step went wrong", PATH_LIB32)

        # copy compiled dll files into source dir
        shutil.copy(PATH_LIB32, os.path.join(BINDING_DIRECTORY, "PyFDP", "FDP_x86.dll"))
        shutil.copy(PATH_LIB64, os.path.join(BINDING_DIRECTORY, "PyFDP", "FDP_x64.dll"))

        os.chdir(_cwd)

def dummy_src():
    return []

setuptools.setup(
    provides=['PyFDP'],
    packages = setuptools.find_packages(),
    name="PyFDP",
    version=VERSION,
    author='Winbagility',
    author_email='',
    description='FDP (Fast Debug Protocol) : KD client for no /DEBUG enabled Windows virtual machines.',
    url='https://winbagility.github.io',
    classifiers=[
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
    ],
    
    requires=[],
    
    # PyFDP need to compile the client FDP dlls
    # but there are not Python module compliant so
    # the standard build_clib command will raise errors. That's
    # why we hook build_clib in order to launch the cmake build script.
    libraries=[(
        'PyFDP', dict(
            package = 'PyFDP',
            sources = dummy_src()
        ),
    )],

    cmdclass=dict(
        build_clib=PyFDPCustomBuildClib,
    ),

    # Tell setuptools not to zip into an egg file
    # That's mandatory whenever there is a filepath involved
    # (in our case via the LoadLibrary)
    zip_safe=False,

    # We have two dlls to package with the python lib.
    include_package_data=True,
    package_data={
        "PyFDP": ["*.dll"],
    }
)