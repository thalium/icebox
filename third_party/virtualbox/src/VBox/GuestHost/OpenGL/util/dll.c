/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_error.h"
#include "cr_dll.h"
#include "cr_string.h"
#include "stdio.h"

#ifndef IN_GUEST
#include <string.h>
#endif

#if defined(IRIX) || defined(IRIX64) || defined(Linux) || defined(FreeBSD) || defined(AIX) || defined(DARWIN) || defined(SunOS) || defined(OSF1)
# include <iprt/assert.h>
# include <iprt/err.h>
# include <iprt/log.h>
# include <iprt/path.h>
#include <dlfcn.h>
#endif

#ifdef WINDOWS
# ifdef VBOX
#  include <iprt/win/shlwapi.h>
# else
#include <Shlwapi.h>
# endif
#endif

#ifdef DARWIN

#include <Carbon/Carbon.h>
#include <mach-o/dyld.h>

char *__frameworkErr=NULL;

CFBundleRef LoadFramework( const char *frameworkName ) {
	CFBundleRef bundle;
	CFURLRef bundleURL;
	char fullfile[8096];

	if( frameworkName[0] != '/' ) {
		/* load a system framework */
		/* XXX \todo should this folder be retrieved from somewhere else? */
		crStrcpy( fullfile, "/System/Library/Frameworks/" );
		crStrcat( fullfile, frameworkName );
	} else {
		/* load any framework */
		crStrcpy( fullfile, frameworkName );
	}

	bundleURL = CFURLCreateWithString( NULL, CFStringCreateWithCStringNoCopy(NULL, fullfile, CFStringGetSystemEncoding(), NULL), NULL );
	if( !bundleURL ) {
		__frameworkErr = "Could not create OpenGL Framework bundle URL";
		return NULL;
	}

	bundle = CFBundleCreate( kCFAllocatorDefault, bundleURL );
	CFRelease( bundleURL );

	if( !bundle ) {
		__frameworkErr = "Could not create OpenGL Framework bundle";
		return NULL;
	}

	if( !CFBundleLoadExecutable(bundle) ) {
		__frameworkErr = "Could not load MachO executable";
		return NULL;
	}

	return bundle;
}

char *__bundleErr=NULL;

void *LoadBundle( const char *filename ) {
	NSObjectFileImage fileImage;
	NSModule handle = NULL;
	char _filename[PATH_MAX];

	__bundleErr = NULL;

	if( filename[0] != '/' ) {
		/* default to a chromium bundle */
		crStrcpy( _filename, "/cr/lib/Darwin/" );
		crStrcat( _filename, filename );
	} else {
		crStrcpy( _filename, filename );
	}

	switch( NSCreateObjectFileImageFromFile(_filename, &fileImage) ) {
	default:
	case NSObjectFileImageFailure:
		__bundleErr = "NSObjectFileImageFailure: Failure.";
		break;

	case NSObjectFileImageInappropriateFile:
		__bundleErr = "NSObjectFileImageInappropriateFile: The specified file is not of a valid type.";
		break;

	case NSObjectFileImageArch:
		__bundleErr = "NSObjectFileImageArch: The specified file is for a different CPU architecture.";
		break;

	case NSObjectFileImageFormat:
		__bundleErr = "NSObjectFileImageFormat: The specified file does not appear to be a Mach-O file";
		break;

	case NSObjectFileImageAccess:
		__bundleErr = "NSObjectFileImageAccess: Permission to create image denied.";
		break;

	case NSObjectFileImageSuccess:
		handle = NSLinkModule( fileImage, _filename,
							   NSLINKMODULE_OPTION_RETURN_ON_ERROR |
							   NSLINKMODULE_OPTION_PRIVATE );
		NSDestroyObjectFileImage( fileImage );
		if( !handle ) {
			NSLinkEditErrors c;
			int n;
			const char *name;
			NSLinkEditError(&c, &n, &name, (const char**)&__bundleErr);
		}
		break;
	}

	return handle;
}

int check_extension( const char *name, const char *extension ) {
	int nam_len = crStrlen( name );
	int ext_len = crStrlen( extension );
	char *pos = crStrstr( name, extension );
	return ( pos == &(name[nam_len-ext_len]) );
}

enum {
	CR_DLL_NONE,
	CR_DLL_FRAMEWORK,
	CR_DLL_DYLIB,
	CR_DLL_BUNDLE,
	CR_DLL_UNKNOWN
};

#define NS_ADD 0

int get_dll_type( const char *name ) {
	if( check_extension(name, ".framework") )
		return CR_DLL_FRAMEWORK;
	if( check_extension(name, ".bundle") )
		return CR_DLL_BUNDLE;
	if( check_extension(name, ".dylib") )
		return CR_DLL_DYLIB;
	return CR_DLL_DYLIB;
}

#endif

/*
 * Open the named shared library.
 * If resolveGlobal is non-zero, unresolved symbols can be satisfied by
 * any matching symbol already defined globally.  Otherwise, if resolveGlobal
 * is zero, unresolved symbols should be resolved using symbols in that
 * object (in preference to global symbols).
 * NOTE: this came about because we found that for libGL, we need the
 * global-resolve option but for SPU's we need the non-global option (consider
 * the state tracker duplicated in the array, tilesort, etc. SPUs).
 */
CRDLL *crDLLOpen( const char *dllname, int resolveGlobal )
{
	CRDLL *dll;
	char *dll_err;
#if defined(WINDOWS)
    WCHAR   szwPath[MAX_PATH];
    UINT   cwcPath = 0;

    (void) resolveGlobal;

# ifndef CR_NO_GL_SYSTEM_PATH
    if (PathIsRelative(dllname))
    {
        size_t cName  = strlen(dllname) + 1;
#  ifdef IN_GUEST
        cwcPath = GetSystemDirectoryW(szwPath, RT_ELEMENTS(szwPath));
        if (!cwcPath || cwcPath >= MAX_PATH)
        {
            DWORD winEr = GetLastError();
            crError("GetSystemDirectoryW failed err %d", winEr);
            SetLastError(winEr);
            return NULL;
        }
#  else
        WCHAR * pszwSlashFile;
        cwcPath = GetModuleFileNameW(NULL, szwPath, RT_ELEMENTS(szwPath));
        if (!cwcPath || cwcPath >= MAX_PATH)
        {
            DWORD winEr = GetLastError();
            crError("GetModuleFileNameW failed err %d", winEr);
            SetLastError(winEr);
            return NULL;
        }

        pszwSlashFile = wcsrchr(szwPath, L'\\');
        if (!pszwSlashFile)
        {
            crError("failed to match file name");
            SetLastError(ERROR_PATH_NOT_FOUND);
            return NULL;
        }

        cwcPath = pszwSlashFile - szwPath;
#  endif

        if (cwcPath + 1 + cName > MAX_PATH)
        {
            crError("invalid path specified");
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return NULL;
        }
        szwPath[cwcPath] = '\\';
        ++cwcPath;
    }
# endif /* CR_NO_GL_SYSTEM_PATH */
	if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, dllname, -1, &szwPath[cwcPath], MAX_PATH - cwcPath))
	{
		DWORD winEr = GetLastError();
		crError("MultiByteToWideChar failed err %d", winEr);
		SetLastError(winEr);
		return NULL;
	}
#endif

	dll = (CRDLL *) crAlloc( sizeof( CRDLL ) );
	dll->name = crStrdup( dllname );

#if defined(WINDOWS)
    dll->hinstLib = LoadLibraryW( szwPath );
    if (!dll->hinstLib)
    {
        crError("failed to load dll %s", dllname);
    }
    dll_err = NULL;
#elif defined(DARWIN)
	/* XXX \todo Get better error handling in here */
	dll->type = get_dll_type( dllname );
	dll_err = NULL;

	switch( dll->type ) {
	case CR_DLL_FRAMEWORK:
		dll->hinstLib = LoadFramework( dllname );
		dll_err = __frameworkErr;
		break;

	case CR_DLL_BUNDLE:
		dll->hinstLib = LoadBundle( dllname );
		dll_err = __bundleErr;
		break;

	case CR_DLL_DYLIB:
#if NS_ADD
		dll->hinstLib = (void*)NSAddImage( dllname, NSADDIMAGE_OPTION_RETURN_ON_ERROR );
#else
		if( resolveGlobal )
			dll->hinstLib = dlopen( dllname, RTLD_LAZY | RTLD_GLOBAL );
		else
			dll->hinstLib = dlopen( dllname, RTLD_LAZY | RTLD_LOCAL );
		dll_err = (char*) dlerror();
#endif
		break;

	default:
		dll->hinstLib = NULL;
		dll_err = "Unknown DLL type";
		break;
	};
#elif defined(IRIX) || defined(IRIX64) || defined(Linux) || defined(FreeBSD) || defined(AIX) || defined(SunOS) || defined(OSF1)
	{
		int flags = RTLD_LAZY;
		if (resolveGlobal)
		   flags |= RTLD_GLOBAL;
		dll->hinstLib = dlopen( dllname, flags );
# ifndef IN_GUEST
		/* GCC address sanitiser breaks DT_RPATH. */
		if (!dll->hinstLib) do {
			char szPath[RTPATH_MAX];
			int rc = RTPathSharedLibs(szPath, sizeof(szPath));
			AssertLogRelMsgRCBreak(rc, ("RTPathSharedLibs() failed: %Rrc\n", rc));
			rc = RTPathAppend(szPath, sizeof(szPath), dllname);
			AssertLogRelMsgRCBreak(rc, ("RTPathAppend() failed: %Rrc\n", rc));
			dll->hinstLib = dlopen( szPath, flags );
		} while(0);
# endif
		dll_err = (char*) dlerror();
	}
#else
#error DSO
#endif

	if (!dll->hinstLib)
	{
		if (dll_err)
		{
			crDebug( "DLL_ERROR(%s): %s", dllname, dll_err );
		}
		crError( "DLL Loader couldn't find/open %s", dllname );
                crFree(dll);
                dll = NULL;
	}
	return dll;
}

CRDLLFunc crDLLGetNoError( CRDLL *dll, const char *symname )
{
#if defined(WINDOWS)
	return (CRDLLFunc) GetProcAddress( dll->hinstLib, symname );
#elif defined(DARWIN)
	NSSymbol nssym;

	if( dll->type == CR_DLL_FRAMEWORK )
		return (CRDLLFunc) CFBundleGetFunctionPointerForName( (CFBundleRef) dll->hinstLib, CFStringCreateWithCStringNoCopy(NULL, symname, CFStringGetSystemEncoding(), NULL) );

	if( dll->type == CR_DLL_DYLIB )
#if NS_ADD
		nssym = NSLookupSymbolInImage( dll->hinstLib, symname, NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR );
#else
		return (CRDLLFunc) dlsym( dll->hinstLib, symname );
#endif
	else
		nssym = NSLookupSymbolInModule( dll->hinstLib, symname );

	if( !nssym ) {
		char name[PATH_MAX];
		crStrcpy( name, "_" );
		crStrcat( name, symname );

		if( dll->type == CR_DLL_DYLIB )
			nssym = NSLookupSymbolInImage( dll->hinstLib, name, NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR );
		else
			nssym = NSLookupSymbolInModule( dll->hinstLib, name );
	}

	return (CRDLLFunc) NSAddressOfSymbol( nssym );

#elif defined(IRIX) || defined(IRIX64) || defined(Linux) || defined(FreeBSD) || defined(AIX) || defined(SunOS) || defined(OSF1)
	return (CRDLLFunc)(uintptr_t)dlsym( dll->hinstLib, symname );
#else
#error CR DLL ARCHITETECTURE
#endif
}

CRDLLFunc crDLLGet( CRDLL *dll, const char *symname )
{
	CRDLLFunc data = crDLLGetNoError( dll, symname );
	if (!data)
	{
		/* Are you sure there isn't some C++ mangling messing you up? */
		crWarning( "Couldn't get symbol \"%s\" in \"%s\"", symname, dll->name );
	}
	return data;
}

void crDLLClose( CRDLL *dll )
{
	int dll_err = 0;

	if (!dll) return;

#if defined(WINDOWS)
	FreeLibrary( dll->hinstLib );
#elif defined(DARWIN)
	switch( dll->type ) {
	case CR_DLL_FRAMEWORK:
		CFBundleUnloadExecutable( dll->hinstLib );
		CFRelease(dll->hinstLib);
		dll->hinstLib = NULL;
		break;

	case CR_DLL_DYLIB:
#if !NS_ADD
		dlclose( dll->hinstLib );
#endif
		break;

	case CR_DLL_BUNDLE:
		NSUnLinkModule( (NSModule) dll->hinstLib, 0L );
		break;
	}
#elif defined(IRIX) || defined(IRIX64) || defined(Linux) || defined(FreeBSD) || defined(AIX) || defined(SunOS) || defined(OSF1)
	/*
	 * Unloading Nvidia's libGL will crash VirtualBox later during shutdown.
	 * Therefore we will skip unloading it. It will be unloaded later anway
	 * because we are already freeing all resources and VirtualBox will terminate
	 * soon.
	 */
#ifndef IN_GUEST
	if (strncmp(dll->name, "libGL", 5))
#endif
		dll_err = dlclose( dll->hinstLib );
#else
#error DSO
#endif

	if (dll_err)
		crWarning("Error closing DLL %s\n",dll->name);

	crFree( dll->name );
	crFree( dll );
}
