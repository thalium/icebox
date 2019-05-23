/* $Id: VBoxGuestDrvInst.cpp $ */
/** @file
 * instdrvmain - Install guest drivers on NT4
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/**
 * @todo
 * The spacing in this file is horrible! Here are some simple rules:
 * - tabs are forbidden
 * - indentation is 4 spaces
 */


#include <iprt/win/windows.h>
#include <iprt/win/setupapi.h>
#include <regstr.h>
#include <DEVGUID.h>
#include <stdio.h>

#include "tchar.h"
#include "string.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** The video service name. */
#define  VBOXGUEST_VIDEO_NAME    "VBoxVideo"

/** The video inf file name */
#define  VBOXGUEST_VIDEO_INF_NAME    "VBoxVideo.inf"

/////////////////////////////////////////////////////////////////////////////



/**
 * Do some cleanup of data we used. Called by installVideoDriver()
 */
void closeAndDestroy(HDEVINFO hDevInfo, HINF hInf)
{
  SetupDiDestroyDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
  SetupDiDestroyDeviceInfoList(hDevInfo);
  SetupCloseInfFile(hInf);
}


/**
 * Install the VBox video driver.
 *
 * @returns TRUE on success.
 * @returns FALSE on failure.
 * @param   szDriverDir     The base directory where we find the INF.
 */
BOOL installVideoDriver(TCHAR* szDriverDir)
{
  HDEVINFO hDevInfo;
  SP_DEVINSTALL_PARAMS DeviceInstallParams={0};
  SP_DRVINFO_DATA drvInfoData={0};
  SP_DRVINFO_DETAIL_DATA DriverInfoDetailData={0};

  DWORD cbReqSize;

  /* Vars used for reading the INF */
  HINF hInf;
  TCHAR szServiceSection[LINE_LEN];
  INFCONTEXT serviceContext;
  TCHAR szServiceData[LINE_LEN];
  TCHAR deviceRegStr[1000];//I'm lazy here. 1000 ought to be enough for everybody...

  SP_DEVINFO_DATA deviceInfoData;
  DWORD configFlags;

  HKEY hkey;
  DWORD disp;
  TCHAR regKeyName[LINE_LEN];

  BOOL rc;

  /* Create an empty list */
  hDevInfo = SetupDiCreateDeviceInfoList((LPGUID) &GUID_DEVCLASS_DISPLAY,
                                         NULL);

  if (hDevInfo == INVALID_HANDLE_VALUE)
    return FALSE;

  memset(&DeviceInstallParams, 0, sizeof(SP_DEVINSTALL_PARAMS));
  DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

  rc=SetupDiGetDeviceInstallParams(hDevInfo,
                                   NULL,
                                   &DeviceInstallParams);

  if(!rc)
    return FALSE;

  DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
  DeviceInstallParams.Flags |= DI_NOFILECOPY | /* We did our own file copying */
    DI_DONOTCALLCONFIGMG |
    DI_ENUMSINGLEINF; /* .DriverPath specifies an inf file */


  /* Path to inf file */
  wsprintf(DeviceInstallParams.DriverPath,
           TEXT("%ws\\%ws"),
           szDriverDir, TEXT(VBOXGUEST_VIDEO_INF_NAME));

  rc=SetupDiSetDeviceInstallParams(hDevInfo,
                                   NULL,
                                   &DeviceInstallParams);
  if(!rc)
    return FALSE;

  /* Read the drivers from the inf file */
  if (!SetupDiBuildDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER))
    {
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return FALSE;
    }

  /* Get the first found driver.
     Our Inf file only contains one so this is fine  */
  drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
  if(FALSE==SetupDiEnumDriverInfo(hDevInfo, NULL, SPDIT_CLASSDRIVER,
                                  0, &drvInfoData)){
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FALSE;
  }

  /* Get necessary driver details */
  DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
  if (!(!SetupDiGetDriverInfoDetail(hDevInfo,
                                    NULL,
                                    &drvInfoData,
                                    &DriverInfoDetailData,
                                    DriverInfoDetailData.cbSize,
                                    &cbReqSize)
        &&GetLastError()== ERROR_INSUFFICIENT_BUFFER) )
    {
      SetupDiDestroyDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return FALSE;
    }

  hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                          NULL, INF_STYLE_WIN4, NULL);

  if (hInf == INVALID_HANDLE_VALUE)
    {
      SetupDiDestroyDriverInfoList(hDevInfo, NULL, SPDIT_CLASSDRIVER);
      SetupDiDestroyDeviceInfoList(hDevInfo);
      return FALSE;
    }

  /* First install the service */
  wsprintf(szServiceSection, TEXT("%ws.Services"),
           DriverInfoDetailData.SectionName);

  if(!SetupFindFirstLine(hInf, szServiceSection, NULL, &serviceContext))
    {
      /* No service line?? Can't be... */
      closeAndDestroy(hDevInfo, hInf);
      return FALSE;
    }

  /* Get the name */
  SetupGetStringField(&serviceContext,
                      1,
                      szServiceData,
                      sizeof(szServiceData),
                      NULL);

  wsprintf(deviceRegStr, TEXT("Root\\LEGACY_%ws\\0000"), szServiceData);

  memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
  deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

  if (SetupDiOpenDeviceInfo(hDevInfo, deviceRegStr, NULL, 0, &deviceInfoData) //Check for existing
      ||(SetupDiCreateDeviceInfo(hDevInfo, deviceRegStr,                      //Create new
                                 (LPGUID) &GUID_DEVCLASS_DISPLAY,
                                 NULL, //Do we need a description here?
                                 NULL, //No user interface
                                 0,
                                 &deviceInfoData) &&
         SetupDiRegisterDeviceInfo(hDevInfo,
                                   &deviceInfoData,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL)) )
    {
      /* We created a new key in the registry */

      memset(&DeviceInstallParams, 0,sizeof(SP_DEVINSTALL_PARAMS));
      DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

      SetupDiGetDeviceInstallParams(hDevInfo,
                                    &deviceInfoData,
                                    &DeviceInstallParams);

      DeviceInstallParams.Flags |= DI_NOFILECOPY | //We already copied the files
        DI_DONOTCALLCONFIGMG |
        DI_ENUMSINGLEINF; //Use our INF file only

      /* Path to inf file */
      wsprintf(DeviceInstallParams.DriverPath,
               TEXT("%ws\\%ws"),
               szDriverDir, TEXT(VBOXGUEST_VIDEO_INF_NAME));

      SetupDiSetDeviceInstallParams(hDevInfo,
                                    &deviceInfoData,
                                    &DeviceInstallParams);


      if(!SetupDiBuildDriverInfoList(hDevInfo,
                                     &deviceInfoData,
                                     SPDIT_CLASSDRIVER))
        {
          closeAndDestroy(hDevInfo, hInf);
          return FALSE;
        }

      drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
      if (!SetupDiEnumDriverInfo(hDevInfo,
                                 &deviceInfoData,
                                 SPDIT_CLASSDRIVER,
                                 0,
                                 &drvInfoData))
        {
          closeAndDestroy(hDevInfo, hInf);
          return FALSE;
        }

      if(!SetupDiSetSelectedDriver(hDevInfo,
                                   &deviceInfoData,
                                   &drvInfoData))
        {
          closeAndDestroy(hDevInfo, hInf);
          return FALSE;
        }

      if(!SetupDiInstallDevice(hDevInfo,
                               &deviceInfoData))
        {
          closeAndDestroy(hDevInfo, hInf);
          return FALSE;
        }
    }

  /* Make sure the device is enabled */
  if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                       &deviceInfoData, SPDRP_CONFIGFLAGS,
                                       NULL, (LPBYTE) &configFlags,
                                       sizeof(DWORD),
                                       NULL)
      && (configFlags & CONFIGFLAG_DISABLED))
    {
      configFlags &= ~CONFIGFLAG_DISABLED;

      SetupDiSetDeviceRegistryProperty(hDevInfo,
                                       &deviceInfoData,
                                       SPDRP_CONFIGFLAGS,
                                       (LPBYTE) &configFlags,
                                       sizeof(DWORD));
    }

  wsprintf(regKeyName,
           TEXT("System\\CurrentControlSet\\Services\\%ws\\Device%d"),
           szServiceData, 0); //We only have one device

  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     regKeyName,
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_READ | KEY_WRITE,
                     NULL,
                     &hkey,
                     &disp) == ERROR_SUCCESS)
    {
      /* Insert description */
      RegSetValueEx(hkey,
                    TEXT("Device Description"),
                    0,
                    REG_SZ,
                    (LPBYTE) DriverInfoDetailData.DrvDescription,
                    (lstrlen(DriverInfoDetailData.DrvDescription) + 1) *
                    sizeof(TCHAR) );

      TCHAR szSoftwareSection[LINE_LEN];

      wsprintf(szSoftwareSection,
               TEXT("%ws.SoftwareSettings"),
               szServiceData);

      if (!SetupInstallFromInfSection(NULL,
                                      hInf,
                                      szSoftwareSection,
                                      SPINST_REGISTRY,
                                      hkey,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL))
        {
          RegCloseKey(hkey);
          closeAndDestroy(hDevInfo, hInf);
          return FALSE;
        }

      RegCloseKey(hkey);
    }

  /* Install OpenGL stuff */
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"),
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_READ | KEY_WRITE,
                     NULL,
                     &hkey,
                     &disp) == ERROR_SUCCESS)
    {
      /* Do installation here if ever necessary. Currently there is no OpenGL stuff */

      RegCloseKey(hkey);
    }


  /* Cleanup */
  closeAndDestroy(hDevInfo, hInf);

#if 0
  /* If this key is inserted into the registry, windows will show the desktop
     applet on next boot. We decide in the installer if we want that so the code
     is disabled here. */
  /* Set registry keys so windows picks up the changes */
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay"),
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_READ | KEY_WRITE,
                     NULL,
                     &hkey,
                     &disp) == ERROR_SUCCESS)
    {
      RegCloseKey(hkey);
    }
#endif

  /* We must reboot at some point */
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\RebootNecessary"),
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_READ | KEY_WRITE,
                     NULL,
                     &hkey,
                     &disp) == ERROR_SUCCESS)
    {
      RegCloseKey(hkey);
    }

  return TRUE;
}


void displayHelpAndExit(char *programName)
{
    printf("Installs VirtualBox Guest Additions for Windows NT 4.0\n\n");
    printf("Syntax: %s [/i|/u]\n", programName);
    printf("\n\t/i  - perform Guest Additions installation\n");
    printf("\t/u:  - perform Guest Additions uninstallation\n");
    exit(1);
}

/**
 * Check if we are running on NT4.
 *
 * @returns TRUE if NT 4.
 * @returns FALSE otherwise.
 */
BOOL isNT4(void)
{
    OSVERSIONINFO osVersion;

    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osVersion);

    switch (osVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32s:
        case VER_PLATFORM_WIN32_WINDOWS:
            return FALSE;
        case VER_PLATFORM_WIN32_NT:
            if (osVersion.dwMajorVersion == 4)
                return TRUE;
            else
                return FALSE;
        default:
            break;
    }
    return FALSE;
}

/**
 *  Video driver uninstallation will be added later if necessary.
 */
int uninstallDrivers(void)
{
    return 0;
}


int main(int argc, char **argv)
{
    BOOL rc=FALSE;
    TCHAR szInstallDir[MAX_PATH];
    HMODULE hExe = GetModuleHandle(NULL);

  /** @todo r=bird:
   * And by the way, we're only missing the coding style dmik uses now
   * and this file would contain the complete set.  */
  if(2!=argc || (stricmp(argv[1], "/i") && stricmp(argv[1], "/u")))
    displayHelpAndExit(argv[0]);

  /* This program is only for installing drivers on NT4 */
  if(!isNT4()){
    printf("This program only runs on NT4\n");
    return -1;
  }

  if (!GetModuleFileName(hExe, &szInstallDir[0], sizeof(szInstallDir)))
    {
      printf("GetModuleFileName failed! rc = %d\n", GetLastError());
      return -1;
    }

  TCHAR *lastBackslash = wcsrchr(szInstallDir, L'\\');

  if (!lastBackslash)
    {
      printf("Invalid path name!\n");
      return -1;
    }

  *lastBackslash=L'\0';

  /* Install video driver. Mouse driver installation is done by overwriting
     the image path in the setup script. */
  if(!stricmp(argv[1], "/i")){
    rc=installVideoDriver(szInstallDir);
  }
  else{
    /* No video driver uninstallation yet. Do we need it? */
  }

  if(FALSE==rc)
    printf("Some failure occurred during driver installation.\n");
  return !rc; /* Return != 0 on failure. */
}
