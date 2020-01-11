' $Id: configure.vbs $
'' @file
' The purpose of this script is to check for all external tools, headers, and
' libraries VBox OSE depends on.
'
' The script generates the build configuration file 'AutoConfig.kmk' and the
' environment setup script 'env.bat'. A log of what has been done is
' written to 'configure.log'.
'

'
' Copyright (C) 2006-2017 Oracle Corporation
'
' This file is part of VirtualBox Open Source Edition (OSE), as
' available from http://www.virtualbox.org. This file is free software;
' you can redistribute it and/or modify it under the terms of the GNU
' General Public License (GPL) as published by the Free Software
' Foundation, in version 2 as it comes in the "COPYING" file of the
' VirtualBox OSE distribution. VirtualBox OSE is distributed in the
' hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
'


'*****************************************************************************
'* Global Variables                                                          *
'*****************************************************************************
dim g_strPath, g_strEnvFile, g_strLogFile, g_strCfgFile, g_strShellOutput
g_strPath = Left(Wscript.ScriptFullName, Len(Wscript.ScriptFullName) - Len("\configure.vbs"))
g_strEnvFile = g_strPath & "\env.bat"
g_strCfgFile = g_strPath & "\AutoConfig.kmk"
g_strLogFile = g_strPath & "\configure.log"
'g_strTmpFile = g_strPath & "\configure.tmp"

dim g_objShell, g_objFileSys
Set g_objShell = WScript.CreateObject("WScript.Shell")
Set g_objFileSys = WScript.CreateObject("Scripting.FileSystemObject")

dim g_strPathkBuild, g_strPathkBuildBin, g_strPathDev, g_strPathVCC, g_strPathPSDK, g_strVerPSDK, g_strPathDDK, g_strSubOutput
g_strPathkBuild = ""
g_strPathDev = ""
g_strPathVCC = ""
g_strPathPSDK = ""
g_strPathDDK = ""

dim g_strTargetArch
g_strTargetArch = ""

dim g_blnDisableCOM, g_strDisableCOM
g_blnDisableCOM = False
g_strDisableCOM = ""

' The internal mode is primarily for skipping some large libraries.
dim g_blnInternalMode
g_blnInternalMode = False

' Whether to try the internal stuff first or last.
dim g_blnInternalFirst
g_blnInternalFirst = True



''
' Converts to unix slashes
function UnixSlashes(str)
   UnixSlashes = replace(str, "\", "/")
end function


''
' Converts to dos slashes
function DosSlashes(str)
   DosSlashes = replace(str, "/", "\")
end function


''
' Read a file (typically the tmp file) into a string.
function FileToString(strFilename)
   const ForReading = 1, TristateFalse = 0
   dim objLogFile, str

   set objFile = g_objFileSys.OpenTextFile(DosSlashes(strFilename), ForReading, False, TristateFalse)
   str = objFile.ReadAll()
   objFile.Close()

   FileToString = str
end function


''
' Deletes a file
sub FileDelete(strFilename)
   if g_objFileSys.FileExists(DosSlashes(strFilename)) then
      g_objFileSys.DeleteFile(DosSlashes(strFilename))
   end if
end sub


''
' Appends a line to an ascii file.
sub FileAppendLine(strFilename, str)
   const ForAppending = 8, TristateFalse = 0
   dim objFile

   set objFile = g_objFileSys.OpenTextFile(DosSlashes(strFilename), ForAppending, True, TristateFalse)
   objFile.WriteLine(str)
   objFile.Close()
end sub


''
' Checks if the file exists.
function FileExists(strFilename)
   FileExists = g_objFileSys.FileExists(DosSlashes(strFilename))
end function


''
' Checks if the directory exists.
function DirExists(strDirectory)
   DirExists = g_objFileSys.FolderExists(DosSlashes(strDirectory))
end function


''
' Checks if this is a WOW64 process.
function IsWow64()
   if g_objShell.Environment("PROCESS")("PROCESSOR_ARCHITEW6432") <> "" then
      IsWow64 = 1
   else
      IsWow64 = 0
   end if
end function


''
' Returns a reverse sorted array (strings).
function ArraySortStrings(arrStrings)
   for i = LBound(arrStrings) to UBound(arrStrings)
      str1 = arrStrings(i)
      for j = i + 1 to UBound(arrStrings)
         str2 = arrStrings(j)
         if StrComp(str2, str1) < 0 then
            arrStrings(j) = str1
            str1 = str2
         end if
      next
      arrStrings(i) = str1
   next
   ArraySortStrings = arrStrings
end function


''
' Prints a string array.
sub ArrayPrintStrings(arrStrings, strPrefix)
   for i = LBound(arrStrings) to UBound(arrStrings)
      Print strPrefix & "arrStrings(" & i & ") = '" & arrStrings(i) & "'"
   next
end sub


''
' Returns a reverse sorted array (strings).
function ArrayRSortStrings(arrStrings)
   ' Sort it.
   arrStrings = ArraySortStrings(arrStrings)

   ' Reverse the array.
   cnt = UBound(arrStrings) - LBound(arrStrings) + 1
   if cnt > 0 then
      j   = UBound(arrStrings)
      iHalf = Fix(LBound(arrStrings) + cnt / 2)
      for i = LBound(arrStrings) to iHalf - 1
         strTmp = arrStrings(i)
         arrStrings(i) = arrStrings(j)
         arrStrings(j) = strTmp
         j = j - 1
      next
   end if
   ArrayRSortStrings = arrStrings
end function


''
' Returns the input array with the string appended.
' Note! There must be some better way of doing this...
function ArrayAppend(arr, str)
   dim i, cnt
   cnt = UBound(arr) - LBound(arr) + 1
   redim arrRet(cnt)
   for i = LBound(arr) to UBound(arr)
      arrRet(i) = arr(i)
   next
   arrRet(UBound(arr) + 1) = str
   ArrayAppend = arrRet
end function



''
' Translates a register root name to a value
function RegTransRoot(strRoot)
   const HKEY_LOCAL_MACHINE = &H80000002
   const HKEY_CURRENT_USER  = &H80000001
   select case strRoot
      case "HKLM"
         RegTransRoot = HKEY_LOCAL_MACHINE
      case "HKCU"
         RegTransRoot = HKEY_CURRENT_USER
      case else
         MsgFatal "RegTransRoot: Unknown root: '" & strRoot & "'"
         RegTransRoot = 0
   end select
end function


'' The registry globals
dim g_objReg, g_objRegCtx
dim g_blnRegistry
g_blnRegistry = false


''
' Init the register provider globals.
function RegInit()
   RegInit = false
   On Error Resume Next
   if g_blnRegistry = false then
      set g_objRegCtx = CreateObject("WbemScripting.SWbemNamedValueSet")
      ' Comment out the following for lines if the cause trouble on your windows version.
      if IsWow64() then
         g_objRegCtx.Add "__ProviderArchitecture", 64
         g_objRegCtx.Add "__RequiredArchitecture", true
      end if
      set objLocator = CreateObject("Wbemscripting.SWbemLocator")
      set objServices = objLocator.ConnectServer("", "root\default", "", "", , , , g_objRegCtx)
      set g_objReg = objServices.Get("StdRegProv")
      g_blnRegistry = true
   end if
   RegInit = true
end function


''
' Gets a value from the registry. Returns "" if string wasn't found / valid.
function RegGetString(strName)
   RegGetString = ""
   if RegInit() then
      dim strRoot, strKey, strValue
      dim iRoot

      ' split up into root, key and value parts.
      strRoot = left(strName, instr(strName, "\") - 1)
      strKey = mid(strName, instr(strName, "\") + 1, instrrev(strName, "\") - instr(strName, "\"))
      strValue = mid(strName, instrrev(strName, "\") + 1)

      ' Must use ExecMethod to call the GetStringValue method because of the context.
      Set InParms = g_objReg.Methods_("GetStringValue").Inparameters
      InParms.hDefKey     = RegTransRoot(strRoot)
      InParms.sSubKeyName = strKey
      InParms.sValueName  = strValue
      On Error Resume Next
      set OutParms = g_objReg.ExecMethod_("GetStringValue", InParms, , g_objRegCtx)
      if OutParms.ReturnValue = 0 then
         RegGetString = OutParms.sValue
      end if
   else
      ' fallback mode
      On Error Resume Next
      RegGetString = g_objShell.RegRead(strName)
   end if
end function


''
' Returns an array of subkey strings.
function RegEnumSubKeys(strRoot, strKeyPath)
   dim iRoot
   iRoot = RegTransRoot(strRoot)
   RegEnumSubKeys = Array()

   if RegInit() then
      ' Must use ExecMethod to call the EnumKey method because of the context.
      Set InParms = g_objReg.Methods_("EnumKey").Inparameters
      InParms.hDefKey     = RegTransRoot(strRoot)
      InParms.sSubKeyName = strKeyPath
      On Error Resume Next
      set OutParms = g_objReg.ExecMethod_("EnumKey", InParms, , g_objRegCtx)
      if OutParms.ReturnValue = 0 then
         RegEnumSubKeys = OutParms.sNames
      end if
   else
      ' fallback mode
      dim objReg, rc, arrSubKeys
      set objReg = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\default:StdRegProv")
      On Error Resume Next
      rc = objReg.EnumKey(iRoot, strKeyPath, arrSubKeys)
      if rc = 0 then
         RegEnumSubKeys = arrSubKeys
      end if
   end if
end function


''
' Returns an array of full path subkey strings.
function RegEnumSubKeysFull(strRoot, strKeyPath)
   dim arrTmp
   arrTmp = RegEnumSubKeys(strRoot, strKeyPath)
   for i = LBound(arrTmp) to UBound(arrTmp)
      arrTmp(i) = strKeyPath & "\" & arrTmp(i)
   next
   RegEnumSubKeysFull = arrTmp
end function


''
' Returns an rsorted array of subkey strings.
function RegEnumSubKeysRSort(strRoot, strKeyPath)
   RegEnumSubKeysRSort = ArrayRSortStrings(RegEnumSubKeys(strRoot, strKeyPath))
end function


''
' Returns an rsorted array of subkey strings.
function RegEnumSubKeysFullRSort(strRoot, strKeyPath)
   RegEnumSubKeysFullRSort = ArrayRSortStrings(RegEnumSubKeysFull(strRoot, strKeyPath))
end function


''
' Gets the commandline used to invoke the script.
function GetCommandline()
   dim str, i

   '' @todo find an api for querying it instead of reconstructing it like this...
   GetCommandline = "cscript configure.vbs"
   for i = 1 to WScript.Arguments.Count
      str = WScript.Arguments.Item(i - 1)
      if str = "" then
         str = """"""
      elseif (InStr(1, str, " ")) then
         str = """" & str & """"
      end if
      GetCommandline = GetCommandline & " " & str
   next
end function


''
' Gets an environment variable.
function EnvGet(strName)
   EnvGet = g_objShell.Environment("PROCESS")(strName)
end function


''
' Sets an environment variable.
sub EnvSet(strName, strValue)
   g_objShell.Environment("PROCESS")(strName) = strValue
   LogPrint "EnvSet: " & strName & "=" & strValue
end sub


''
' Appends a string to an environment variable
sub EnvAppend(strName, strValue)
   dim str
   str = g_objShell.Environment("PROCESS")(strName)
   g_objShell.Environment("PROCESS")(strName) =  str & strValue
   LogPrint "EnvAppend: " & strName & "=" & str & strValue
end sub


''
' Prepends a string to an environment variable
sub EnvPrepend(strName, strValue)
   dim str
   str = g_objShell.Environment("PROCESS")(strName)
   g_objShell.Environment("PROCESS")(strName) =  strValue & str
   LogPrint "EnvPrepend: " & strName & "=" & strValue & str
end sub

''
' Gets the first non-empty environment variable of the given two.
function EnvGetFirst(strName1, strName2)
   EnvGetFirst = g_objShell.Environment("PROCESS")(strName1)
   if EnvGetFirst = "" then
      EnvGetFirst = g_objShell.Environment("PROCESS")(strName2)
   end if
end function


''
' Get the path of the parent directory. Returns root if root was specified.
' Expects abs path.
function PathParent(str)
   PathParent = g_objFileSys.GetParentFolderName(DosSlashes(str))
end function


''
' Strips the filename from at path.
function PathStripFilename(str)
   PathStripFilename = g_objFileSys.GetParentFolderName(DosSlashes(str))
end function


''
' Get the abs path, use the short version if necessary.
function PathAbs(str)
   strAbs    = g_objFileSys.GetAbsolutePathName(DosSlashes(str))
   strParent = g_objFileSys.GetParentFolderName(strAbs)
   if strParent = "" then
      PathAbs = strAbs
   else
      strParent = PathAbs(strParent)  ' Recurse to resolve parent paths.
      PathAbs   = g_objFileSys.BuildPath(strParent, g_objFileSys.GetFileName(strAbs))

      dim obj
      set obj = Nothing
      if FileExists(PathAbs) then
         set obj = g_objFileSys.GetFile(PathAbs)
      elseif DirExists(PathAbs) then
         set obj = g_objFileSys.GetFolder(PathAbs)
      end if

      if not (obj is nothing) then
         for each objSub in obj.ParentFolder.SubFolders
            if obj.Name = objSub.Name  or  obj.ShortName = objSub.ShortName then
               if  InStr(1, objSub.Name, " ") > 0 _
                Or InStr(1, objSub.Name, "&") > 0 _
                Or InStr(1, objSub.Name, "$") > 0 _
                  then
                  PathAbs = g_objFileSys.BuildPath(strParent, objSub.ShortName)
                  if  InStr(1, PathAbs, " ") > 0 _
                   Or InStr(1, PathAbs, "&") > 0 _
                   Or InStr(1, PathAbs, "$") > 0 _
                     then
                     MsgFatal "PathAbs(" & str & ") attempted to return filename with problematic " _
                      & "characters in it (" & PathAbs & "). The tool/sdk referenced will probably " _
                      & "need to be copied or reinstalled to a location without 'spaces', '$', ';' " _
                      & "or '&' in the path name. (Unless it's a problem with this script of course...)"
                  end if
               else
                  PathAbs = g_objFileSys.BuildPath(strParent, objSub.Name)
               end if
               exit for
            end if
         next
      end if
   end if
end function


''
' Get the abs path, use the long version.
function PathAbsLong(str)
   strAbs    = g_objFileSys.GetAbsolutePathName(DosSlashes(str))
   strParent = g_objFileSys.GetParentFolderName(strAbs)
   if strParent = "" then
      PathAbsLong = strAbs
   else
      strParent = PathAbsLong(strParent)  ' Recurse to resolve parent paths.
      PathAbsLong = g_objFileSys.BuildPath(strParent, g_objFileSys.GetFileName(strAbs))

      dim obj
      set obj = Nothing
      if FileExists(PathAbsLong) then
         set obj = g_objFileSys.GetFile(PathAbsLong)
      elseif DirExists(PathAbsLong) then
         set obj = g_objFileSys.GetFolder(PathAbsLong)
      end if

      if not (obj is nothing) then
         for each objSub in obj.ParentFolder.SubFolders
            if obj.Name = objSub.Name  or  obj.ShortName = objSub.ShortName then
               PathAbsLong = g_objFileSys.BuildPath(strParent, objSub.Name)
               exit for
            end if
         next
      end if
   end if
end function


''
' Executes a command in the shell catching output in g_strShellOutput
function Shell(strCommand, blnBoth)
   dim strShell, strCmdline, objExec, str

   strShell = g_objShell.ExpandEnvironmentStrings("%ComSpec%")
   if blnBoth = true then
      strCmdline = strShell & " /c " & strCommand & " 2>&1"
   else
      strCmdline = strShell & " /c " & strCommand & " 2>nul"
   end if

   LogPrint "# Shell: " & strCmdline
   Set objExec = g_objShell.Exec(strCmdLine)
   g_strShellOutput = objExec.StdOut.ReadAll()
   objExec.StdErr.ReadAll()
   do while objExec.Status = 0
      Wscript.Sleep 20
      g_strShellOutput = g_strShellOutput & objExec.StdOut.ReadAll()
      objExec.StdErr.ReadAll()
   loop

   LogPrint "# Status: " & objExec.ExitCode
   LogPrint "# Start of Output"
   LogPrint g_strShellOutput
   LogPrint "# End of Output"

   Shell = objExec.ExitCode
end function


''
' Try find the specified file in the path.
function Which(strFile)
   dim strPath, iStart, iEnd, str

   ' the path
   strPath = EnvGet("Path")
   iStart = 1
   do while iStart <= Len(strPath)
      iEnd = InStr(iStart, strPath, ";")
      if iEnd <= 0 then iEnd = Len(strPath) + 1
      if iEnd > iStart then
         str = Mid(strPath, iStart, iEnd - iStart) & "/" & strFile
         if FileExists(str) then
            Which = str
            exit function
         end if
      end if
      iStart = iEnd + 1
   loop

   ' registry or somewhere?

   Which = ""
end function


''
' Append text to the log file and echo it to stdout
sub Print(str)
   LogPrint str
   Wscript.Echo str
end sub


''
' Prints a test header
sub PrintHdr(strTest)
   LogPrint "***** Checking for " & strTest & " *****"
   Wscript.Echo "Checking for " & StrTest & "..."
end sub


''
' Prints a success message
sub PrintResultMsg(strTest, strResult)
   LogPrint "** " & strTest & ": " & strResult
   Wscript.Echo " Found "& strTest & ": " & strResult
end sub


''
' Prints a successfully detected path
sub PrintResult(strTest, strPath)
   strLongPath = PathAbsLong(strPath)
   if PathAbs(strPath) <> strLongPath then
      LogPrint         "** " & strTest & ": " & strPath & " (" & UnixSlashes(strLongPath) & ")"
      Wscript.Echo " Found " & strTest & ": " & strPath & " (" & UnixSlashes(strLongPath) & ")"
   else
      LogPrint         "** " & strTest & ": " & strPath
      Wscript.Echo " Found " & strTest & ": " & strPath
   end if
end sub


''
' Warning message.
sub MsgWarning(strMsg)
   Print "warning: " & strMsg
end sub


''
' Fatal error.
sub MsgFatal(strMsg)
   Print "fatal error: " & strMsg
   Wscript.Quit
end sub


''
' Error message, fatal unless flag to ignore errors is given.
sub MsgError(strMsg)
   Print "error: " & strMsg
   if g_blnInternalMode = False then
      Wscript.Quit
   end if
end sub


''
' Write a log header with some basic info.
sub LogInit
   FileDelete g_strLogFile
   LogPrint "# Log file generated by " & Wscript.ScriptFullName
   for i = 1 to WScript.Arguments.Count
      LogPrint "# Arg #" & i & ": " & WScript.Arguments.Item(i - 1)
   next
   if Wscript.Arguments.Count = 0 then
      LogPrint "# No arguments given"
   end if
   LogPrint "# Reconstructed command line: " & GetCommandline()

   ' some Wscript stuff
   LogPrint "# Wscript properties:"
   LogPrint "#   ScriptName: " & Wscript.ScriptName
   LogPrint "#   Version:    " & Wscript.Version
   LogPrint "#   Build:      " & Wscript.BuildVersion
   LogPrint "#   Name:       " & Wscript.Name
   LogPrint "#   Full Name:  " & Wscript.FullName
   LogPrint "#   Path:       " & Wscript.Path
   LogPrint "#"


   ' the environment
   LogPrint "# Environment:"
   dim objEnv
   for each strVar in g_objShell.Environment("PROCESS")
      LogPrint "#   " & strVar
   next
   LogPrint "#"
end sub


''
' Append text to the log file.
sub LogPrint(str)
   FileAppendLine g_strLogFile, str
   'Wscript.Echo "dbg: " & str
end sub


''
' Checks if the file exists and logs failures.
function LogFileExists(strPath, strFilename)
   LogFileExists = FileExists(strPath & "/" & strFilename)
   if LogFileExists = False then
      LogPrint "Testing '" & strPath & "': " & strFilename & " not found"
   end if

end function


''
' Finds the first file matching the pattern.
' If no file is found, log the failure.
function LogFindFile(strPath, strPattern)
   dim str

   '
   ' Yes, there are some facy database kinda interface to the filesystem
   ' however, breaking down the path and constructing a usable query is
   ' too much hassle. So, we'll do it the unix way...
   '
   if Shell("dir /B """ & DosSlashes(strPath) & "\" & DosSlashes(strPattern) & """", True) = 0 _
    And InStr(1, g_strShellOutput, Chr(13)) > 1 _
      then
      ' return the first word.
      LogFindFile = Left(g_strShellOutput, InStr(1, g_strShellOutput, Chr(13)) - 1)
   else
      LogPrint "Testing '" & strPath & "': " & strPattern & " not found"
      LogFindFile = ""
   end if
end function


''
' Finds the first directory matching the pattern.
' If no directory is found, log the failure,
' else return the complete path to the found directory.
function LogFindDir(strPath, strPattern)
   dim str

   '
   ' Yes, there are some facy database kinda interface to the filesystem
   ' however, breaking down the path and constructing a usable query is
   ' too much hassle. So, we'll do it the unix way...
   '

   ' List the alphabetically last names as first entries (with /O-N).
   if Shell("dir /B /AD /O-N """ & DosSlashes(strPath) & "\" & DosSlashes(strPattern) & """", True) = 0 _
    And InStr(1, g_strShellOutput, Chr(13)) > 1 _
      then
      ' return the first word.
      LogFindDir = strPath & "/" & Left(g_strShellOutput, InStr(1, g_strShellOutput, Chr(13)) - 1)
   else
      LogPrint "Testing '" & strPath & "': " & strPattern & " not found"
      LogFindDir = ""
   end if
end function


''
' Initializes the config file.
sub CfgInit
   FileDelete g_strCfgFile
   CfgPrint "# -*- Makefile -*-"
   CfgPrint "#"
   CfgPrint "# Build configuration generated by " & GetCommandline()
   CfgPrint "#"
   if g_blnInternalMode = False then
      CfgPrint "VBOX_OSE := 1"
      CfgPrint "VBOX_VCC_WERR = $(NO_SUCH_VARIABLE)"
   end if
end sub


''
' Prints a string to the config file.
sub CfgPrint(str)
   FileAppendLine g_strCfgFile, str
end sub


''
' Initializes the environment batch script.
sub EnvInit
   FileDelete g_strEnvFile
   EnvPrint "@echo off"
   EnvPrint "rem"
   EnvPrint "rem Environment setup script generated by " & GetCommandline()
   EnvPrint "rem"
end sub


''
' Prints a string to the environment batch script.
sub EnvPrint(str)
   FileAppendLine g_strEnvFile, str
end sub


''
' No COM
sub DisableCOM(strReason)
   if g_blnDisableCOM = False then
      LogPrint "Disabled COM components: " & strReason
      g_blnDisableCOM = True
      g_strDisableCOM = strReason
      CfgPrint "VBOX_WITH_MAIN="
      CfgPrint "VBOX_WITH_QTGUI="
      CfgPrint "VBOX_WITH_VBOXSDL="
      CfgPrint "VBOX_WITH_DEBUGGER_GUI="
   end if
end sub


''
' No UDPTunnel
sub DisableUDPTunnel(strReason)
   if g_blnDisableUDPTunnel = False then
      LogPrint "Disabled UDPTunnel network transport: " & strReason
      g_blnDisableUDPTunnel = True
      g_strDisableUDPTunnel = strReason
      CfgPrint "VBOX_WITH_UDPTUNNEL="
   end if
end sub


''
' No SDL
sub DisableSDL(strReason)
   if g_blnDisableSDL = False then
      LogPrint "Disabled SDL frontend: " & strReason
      g_blnDisableSDL = True
      g_strDisableSDL = strReason
      CfgPrint "VBOX_WITH_VBOXSDL="
   end if
end sub


''
' Checks the the path doesn't contain characters the tools cannot deal with.
sub CheckSourcePath
   dim sPwd

   sPwd = PathAbs(g_strPath)
   if InStr(1, sPwd, " ") > 0 then
      MsgError "Source path contains spaces! Please move it. (" & sPwd & ")"
   end if
   if InStr(1, sPwd, "$") > 0 then
      MsgError "Source path contains the '$' char! Please move it. (" & sPwd & ")"
   end if
   if InStr(1, sPwd, "%") > 0 then
      MsgError "Source path contains the '%' char! Please move it. (" & sPwd & ")"
   end if
   if  InStr(1, sPwd, Chr(10)) > 0 _
    Or InStr(1, sPwd, Chr(13)) > 0 _
    Or InStr(1, sPwd, Chr(9)) > 0 _
    then
      MsgError "Source path contains control characters! Please move it. (" & sPwd & ")"
   end if
   Print "Source path: OK"
end sub


''
' Checks for kBuild - very simple :)
sub CheckForkBuild(strOptkBuild)
   PrintHdr "kBuild"

   '
   ' Check if there is a 'kmk' in the path somewhere without
   ' any KBUILD_*PATH stuff around.
   '
   blnNeedEnvVars = True
   g_strPathkBuild = strOptkBuild
   g_strPathkBuildBin = ""
   if   (g_strPathkBuild = "") _
    And (EnvGetFirst("KBUILD_PATH", "PATH_KBUILD") = "") _
    And (EnvGetFirst("KBUILD_BIN_PATH", "PATH_KBUILD_BIN") = "") _
    And (Shell("kmk.exe --version", True) = 0) _
    And (InStr(1,g_strShellOutput, "kBuild Make 0.1") > 0) _
    And (InStr(1,g_strShellOutput, "KBUILD_PATH") > 0) _
    And (InStr(1,g_strShellOutput, "KBUILD_BIN_PATH") > 0) then
      '' @todo Need to parse out the KBUILD_PATH and KBUILD_BIN_PATH values to complete the other tests.
      'blnNeedEnvVars = False
      MsgWarning "You've installed kBuild it seems. configure.vbs hasn't been updated to " _
         & "deal with that yet and will use the one it ships with. Sorry."
   end if

   '
   ' Check for the KBUILD_PATH env.var. and fall back on root/kBuild otherwise.
   '
   if g_strPathkBuild = "" then
      g_strPathkBuild = EnvGetFirst("KBUILD_PATH", "PATH_KBUILD")
      if (g_strPathkBuild <> "") and (FileExists(g_strPathkBuild & "/footer.kmk") = False) then
         MsgWarning "Ignoring incorrect kBuild path (KBUILD_PATH=" & g_strPathkBuild & ")"
         g_strPathkBuild = ""
      end if

      if g_strPathkBuild = "" then
         g_strPathkBuild = g_strPath & "/kBuild"
      end if
   end if

   g_strPathkBuild = UnixSlashes(PathAbs(g_strPathkBuild))

   '
   ' Check for env.vars that kBuild uses (do this early to set g_strTargetArch).
   '
   str = EnvGetFirst("KBUILD_TYPE", "BUILD_TYPE")
   if   (str <> "") _
    And (InStr(1, "|release|debug|profile|kprofile", str) <= 0) then
      EnvPrint "set KBUILD_TYPE=release"
      EnvSet "KBUILD_TYPE", "release"
      MsgWarning "Found unknown KBUILD_TYPE value '" & str &"' in your environment. Setting it to 'release'."
   end if

   str = EnvGetFirst("KBUILD_TARGET", "BUILD_TARGET")
   if   (str <> "") _
    And (InStr(1, "win|win32|win64", str) <= 0) then '' @todo later only 'win' will be valid. remember to fix this check!
      EnvPrint "set KBUILD_TARGET=win"
      EnvSet "KBUILD_TARGET", "win"
      MsgWarning "Found unknown KBUILD_TARGET value '" & str &"' in your environment. Setting it to 'win32'."
   end if

   str = EnvGetFirst("KBUILD_TARGET_ARCH", "BUILD_TARGET_ARCH")
   if   (str <> "") _
    And (InStr(1, "x86|amd64", str) <= 0) then
      EnvPrint "set KBUILD_TARGET_ARCH=x86"
      EnvSet "KBUILD_TARGET_ARCH", "x86"
      MsgWarning "Found unknown KBUILD_TARGET_ARCH value '" & str &"' in your environment. Setting it to 'x86'."
      str = "x86"
   end if
   if g_strTargetArch = "" then '' command line parameter --target-arch=x86|amd64 has priority
      if str <> "" then
         g_strTargetArch = str
      elseif (EnvGet("PROCESSOR_ARCHITEW6432") = "AMD64" ) _
          Or (EnvGet("PROCESSOR_ARCHITECTURE") = "AMD64" ) then
         g_strTargetArch = "amd64"
      else
         g_strTargetArch = "x86"
      end if
   else
      if InStr(1, "x86|amd64", g_strTargetArch) <= 0 then
         EnvPrint "set KBUILD_TARGET_ARCH=x86"
         EnvSet "KBUILD_TARGET_ARCH", "x86"
         MsgWarning "Unknown --target-arch=" & str &". Setting it to 'x86'."
      end if
   end if
   LogPrint " Target architecture: " & g_strTargetArch & "."
   Wscript.Echo " Target architecture: " & g_strTargetArch & "."
   EnvPrint "set KBUILD_TARGET_ARCH=" & g_strTargetArch

   str = EnvGetFirst("KBUILD_TARGET_CPU", "BUILD_TARGET_CPU")
    ' perhaps a bit pedantic this since this isn't clearly define nor used much...
   if   (str <> "") _
    And (InStr(1, "i386|i486|i686|i786|i868|k5|k6|k7|k8", str) <= 0) then
      EnvPrint "set BUILD_TARGET_CPU=i386"
      EnvSet "KBUILD_TARGET_CPU", "i386"
      MsgWarning "Found unknown KBUILD_TARGET_CPU value '" & str &"' in your environment. Setting it to 'i386'."
   end if

   str = EnvGetFirst("KBUILD_HOST", "BUILD_PLATFORM")
   if   (str <> "") _
    And (InStr(1, "win|win32|win64", str) <= 0) then '' @todo later only 'win' will be valid. remember to fix this check!
      EnvPrint "set KBUILD_HOST=win"
      EnvSet "KBUILD_HOST", "win"
      MsgWarning "Found unknown KBUILD_HOST value '" & str &"' in your environment. Setting it to 'win32'."
   end if

   str = EnvGetFirst("KBUILD_HOST_ARCH", "BUILD_PLATFORM_ARCH")
   if str <> "" then
      if InStr(1, "x86|amd64", str) <= 0 then
         str = "x86"
         MsgWarning "Found unknown KBUILD_HOST_ARCH value '" & str &"' in your environment. Setting it to 'x86'."
      end if
   elseif (EnvGet("PROCESSOR_ARCHITEW6432") = "AMD64" ) _
       Or (EnvGet("PROCESSOR_ARCHITECTURE") = "AMD64" ) then
      str = "amd64"
   else
      str = "x86"
   end if
   LogPrint " Host architecture: " & str & "."
   Wscript.Echo " Host architecture: " & str & "."
   EnvPrint "set KBUILD_HOST_ARCH=" & str

   str = EnvGetFirst("KBUILD_HOST_CPU", "BUILD_PLATFORM_CPU")
    ' perhaps a bit pedantic this since this isn't clearly define nor used much...
   if   (str <> "") _
    And (InStr(1, "i386|i486|i686|i786|i868|k5|k6|k7|k8", str) <= 0) then
      EnvPrint "set KBUILD_HOST_CPU=i386"
      EnvSet "KBUILD_HOST_CPU", "i386"
      MsgWarning "Found unknown KBUILD_HOST_CPU value '" & str &"' in your environment. Setting it to 'i386'."
   end if

   '
   ' Determin the location of the kBuild binaries.
   '
   if g_strPathkBuildBin = "" then
      g_strPathkBuildBin = g_strPathkBuild & "/bin/win." & g_strTargetArch
      if FileExists(g_strPathkBuild & "/kmk.exe") = False then
         g_strPathkBuildBin = g_strPathkBuild & "/bin/win.x86"
      end if
   end if

   '
   ' Perform basic validations of the kBuild installation.
   '
   if  (FileExists(g_strPathkBuild & "/footer.kmk") = False) _
    Or (FileExists(g_strPathkBuild & "/header.kmk") = False) _
    Or (FileExists(g_strPathkBuild & "/rules.kmk") = False) then
      MsgFatal "Can't find valid kBuild at '" & g_strPathkBuild & "'. Either there is an " _
         & "incorrect KBUILD_PATH in the environment or the checkout didn't succeed."
      exit sub
   end if
   if  (FileExists(g_strPathkBuildBin & "/kmk.exe") = False) _
    Or (FileExists(g_strPathkBuildBin & "/kmk_ash.exe") = False) then
      MsgFatal "Can't find valid kBuild binaries at '" & g_strPathkBuildBin & "'. Either there is an " _
         & "incorrect KBUILD_PATH in the environment or the checkout didn't succeed."
      exit sub
   end if

   if (Shell(DosSlashes(g_strPathkBuildBin & "/kmk.exe") & " --version", True) <> 0) Then
      MsgFatal "Can't execute '" & g_strPathkBuildBin & "/kmk.exe --version'. check configure.log for the out."
      exit sub
   end if

   '
   ' If PATH_DEV is set, check that it's pointing to something useful.
   '
   str = EnvGet("PATH_DEV")
   g_strPathDev = str
   if (str <> "") _
    And False then '' @todo add some proper tests here.
      strNew = UnixSlashes(g_strPath & "/tools")
      EnvPrint "set PATH_DEV=" & strNew
      EnvSet "PATH_DEV", strNew
      MsgWarning "Found PATH_DEV='" & str &"' in your environment. Setting it to '" & strNew & "'."
      g_strPathDev = strNew
   end if
   if g_strPathDev = "" then g_strPathDev = UnixSlashes(g_strPath & "/tools")

   '
   ' Write KBUILD_PATH to the environment script if necessary.
   '
   if blnNeedEnvVars = True then
      EnvPrint "set KBUILD_PATH=" & g_strPathkBuild
      EnvSet "KBUILD_PATH", g_strPathkBuild
      EnvPrint "set PATH=" & g_strPathkBuildBin & ";%PATH%"
      EnvPrepend "PATH", g_strPathkBuildBin & ";"
   end if

   PrintResult "kBuild", g_strPathkBuild
   PrintResult "kBuild binaries", g_strPathkBuildBin
end sub


''
' Checks for Visual C++ version 10 (2010).
sub CheckForVisualCPP(strOptVC, strOptVCCommon, blnOptVCExpressEdition)
   dim strPathVC, strPathVCCommon, str, str2, blnNeedMsPDB
   PrintHdr "Visual C++"

   '
   ' Try find it...
   '
   strPathVC = ""
   strPathVCCommon = ""
   if (strPathVC = "") And (strOptVC <> "") then
      if CheckForVisualCPPSub(strOptVC, strOptVCCommon, blnOptVCExpressEdition) then
         strPathVC = strOptVC
         strPathVCCommon = strOptVCCommon
      end if
   end if

   if (strPathVC = "") And (g_blnInternalFirst = True) Then
      strPathVC = g_strPathDev & "/win.x86/vcc/v10sp1"
      if CheckForVisualCPPSub(strPathVC, "", blnOptVCExpressEdition) = False then
         strPathVC = ""
      end if
   end if

   if   (strPathVC = "") _
    And (Shell("cl.exe", True) = 0) then
      str = Which("cl.exe")
      if FileExists(PathStripFilename(strClExe) & "/build.exe") then
         ' don't know how to deal with this cl.
         Warning "Ignoring DDK cl.exe (" & str & ")."
      else
         strPathVC = PathParent(PathStripFilename(str))
         strPathVCCommon = PathParent(strPathVC) & "/Common7"
      end if
   end if

   if (strPathVC = "") then
      str = RegGetString("HKLM\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\10.0\Setup\VS\ProductDir")
      if str <> "" Then
         str2 = str & "Common7"
         str = str & "VC"
         if CheckForVisualCPPSub(str, str2, blnOptVCExpressEdition) then
            strPathVC = str
            strPathVCCommon = str2
         end if
      end if
   end if

   if (strPathVC = "") then
      str = RegGetString("HKLM\SOFTWARE\Microsoft\VisualStudio\10.0\Setup\VS\ProductDir")
      if str <> "" Then
         str2 = str & "Common7"
         str = str & "VC"
         if CheckForVisualCPPSub(str, str2, blnOptVCExpressEdition) then
            strPathVC = str
            strPathVCCommon = str2
         end if
      end if
   end if

   if (strPathVC = "") And (g_blnInternalFirst = False) Then
      strPathVC = g_strPathDev & "/win.x86/vcc/v10sp1"
      if CheckForVisualCPPSub(strPathVC, "", blnOptVCExpressEdition) = False then
         strPathVC = ""
      end if
   end if

   if strPathVC = "" then
      MsgError "Cannot find cl.exe (Visual C++) anywhere on your system. Check the build requirements."
      exit sub
   end if

   '
   ' Clean up the path and determin the VC directory.
   '
   strPathVC = UnixSlashes(PathAbs(strPathVC))
   g_strPathVCC = strPathVC

   '
   ' Check the version.
   ' We'll have to make sure mspdbXX.dll is somewhere in the PATH.
   '
   if (strPathVCCommon <> "") Then
      EnvAppend "PATH", ";" & strPathVCCommon & "/IDE"
   end if
   if Shell(DosSlashes(strPathVC & "/bin/cl.exe"), True) <> 0 then
      MsgError "Executing '" & strClExe & "' (which we believe to be the Visual C++ compiler driver) failed."
      exit sub
   end if

   if   (InStr(1, g_strShellOutput, "Version 16.") <= 0) _
    And (InStr(1, g_strShellOutput, "Version 17.") <= 0) then
      MsgError "The Visual C++ compiler we found ('" & strPathVC & "') isn't 10.0 or 11.0. Check the build requirements."
      exit sub
   end if

   '
   ' Ok, emit build config variables.
   '
   if InStr(1, g_strShellOutput, "Version 16.") > 0 then
      CfgPrint "PATH_TOOL_VCC100      := " & g_strPathVCC
      CfgPrint "PATH_TOOL_VCC100X86   := $(PATH_TOOL_VCC100)"
      CfgPrint "PATH_TOOL_VCC100AMD64 := $(PATH_TOOL_VCC100)"
      if LogFileExists(strPathVC, "atlmfc/include/atlbase.h") then
         PrintResult "Visual C++ v10 with ATL", g_strPathVCC
      elseif   LogFileExists(g_strPathDDK, "inc/atl71/atlbase.h") _
           And LogFileExists(g_strPathDDK, "lib/ATL/i386/atls.lib") then
         CfgPrint "VBOX_WITHOUT_COMPILER_REDIST=1"
         CfgPrint "PATH_TOOL_VCC100_ATLMFC_INC       = " & g_strPathDDK & "/inc/atl71"
         CfgPrint "PATH_TOOL_VCC100_ATLMFC_LIB.amd64 = " & g_strPathDDK & "/lib/ATL/amd64"
         CfgPrint "PATH_TOOL_VCC100_ATLMFC_LIB.x86   = " & g_strPathDDK & "/lib/ATL/i386"
         CfgPrint "PATH_TOOL_VCC100AMD64_ATLMFC_INC  = " & g_strPathDDK & "/inc/atl71"
         CfgPrint "PATH_TOOL_VCC100AMD64_ATLMFC_LIB  = " & g_strPathDDK & "/lib/ATL/amd64"
         CfgPrint "PATH_TOOL_VCC100X86_ATLMFC_INC    = " & g_strPathDDK & "/inc/atl71"
         CfgPrint "PATH_TOOL_VCC100X86_ATLMFC_LIB    = " & g_strPathDDK & "/lib/ATL/i386"
         PrintResult "Visual C++ v10 with DDK ATL", g_strPathVCC
      else
         CfgPrint "VBOX_WITHOUT_COMPILER_REDIST=1"
         DisableCOM "No ATL"
         PrintResult "Visual C++ v10 (or later) without ATL", g_strPathVCC
      end if

   elseif InStr(1, g_strShellOutput, "Version 17.") > 0 then
      CfgPrint "PATH_TOOL_VCC110      := " & g_strPathVCC
      CfgPrint "PATH_TOOL_VCC110X86   := $(PATH_TOOL_VCC110)"
      CfgPrint "PATH_TOOL_VCC110AMD64 := $(PATH_TOOL_VCC110)"
      PrintResult "Visual C++ v11", g_strPathVCC
      MsgWarning "The support for Visual C++ v11 (aka 2012) is experimental"

   else
      MsgError "The Visual C++ compiler we found ('" & strPathVC & "') isn't 10.0 or 11.0. Check the build requirements."
      exit sub
   end if

   ' and the env.bat path fix.
   if strPathVCCommon <> "" then
      EnvPrint "set PATH=%PATH%;" & strPathVCCommon & "/IDE;"
   end if
end sub

''
' Checks if the specified path points to a usable PSDK.
function CheckForVisualCPPSub(strPathVC, strPathVCCommon, blnOptVCExpressEdition)
   strPathVC = UnixSlashes(PathAbs(strPathVC))
   CheckForVisualCPPSub = False
   LogPrint "trying: strPathVC=" & strPathVC & " strPathVCCommon=" & strPathVCCommon & " blnOptVCExpressEdition=" & blnOptVCExpressEdition
   if   LogFileExists(strPathVC, "bin/cl.exe") _
    And LogFileExists(strPathVC, "bin/link.exe") _
    And LogFileExists(strPathVC, "include/string.h") _
    And LogFileExists(strPathVC, "lib/libcmt.lib") _
    And LogFileExists(strPathVC, "lib/msvcrt.lib") _
      then
      if blnOptVCExpressEdition _
       Or (    LogFileExists(strPathVC, "atlmfc/include/atlbase.h") _
           And LogFileExists(strPathVC, "atlmfc/lib/atls.lib")) _
       Or (    LogFileExists(g_strPathDDK, "inc/atl71/atlbase.h") _
           And LogFileExists(g_strPathDDK, "lib/ATL/i386/atls.lib")) _
         Then
         '' @todo figure out a way we can verify the version/build!
         CheckForVisualCPPSub = True
      end if
   end if
end function


''
' Checks for a platform SDK that works with the compiler
sub CheckForPlatformSDK(strOptSDK)
   dim strPathPSDK, str
   PrintHdr "Windows Platform SDK (recent)"

   strPathPSDK = ""

   ' Check the supplied argument first.
   str = strOptSDK
   if str <> "" then
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   ' The tools location (first).
   if strPathPSDK = "" And g_blnInternalFirst then
      str = g_strPathDev & "/win.x86/sdk/v7.1"
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   if strPathPSDK = "" And g_blnInternalFirst then
      str = g_strPathDev & "/win.x86/sdk/v8.0"
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   ' Look for it in the environment
   str = EnvGet("MSSdk")
   if strPathPSDK = "" And str <> "" then
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   str = EnvGet("Mstools")
   if strPathPSDK = "" And str <> "" then
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   ' Check if there is one installed with the compiler.
   if strPathPSDK = "" And str <> "" then
      str = g_strPathVCC & "/PlatformSDK"
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   ' Check the registry next (ASSUMES sorting).
   arrSubKeys = RegEnumSubKeysRSort("HKLM", "SOFTWARE\Microsoft\Microsoft SDKs\Windows")
   for each strSubKey in arrSubKeys
      str = RegGetString("HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\" & strSubKey & "\InstallationFolder")
      if strPathPSDK = "" And str <> "" then
         if CheckForPlatformSDKSub(str) then strPathPSDK = str
      end if
   Next
   arrSubKeys = RegEnumSubKeysRSort("HKCU", "SOFTWARE\Microsoft\Microsoft SDKs\Windows")
   for each strSubKey in arrSubKeys
      str = RegGetString("HKCU\SOFTWARE\Microsoft\Microsoft SDKs\Windows\" & strSubKey & "\InstallationFolder")
      if strPathPSDK = "" And str <> "" then
         if CheckForPlatformSDKSub(str) then strPathPSDK = str
      end if
   Next

   ' The tools location (post).
   if (strPathPSDK = "") And (g_blnInternalFirst = False) then
      str = g_strPathDev & "/win.x86/sdk/v7.1"
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   if (strPathPSDK = "") And (g_blnInternalFirst = False) then
      str = g_strPathDev & "/win.x86/sdk/v8.0"
      if CheckForPlatformSDKSub(str) then strPathPSDK = str
   end if

   ' Give up.
   if strPathPSDK = "" then
      MsgError "Cannot find a suitable Platform SDK. Check configure.log and the build requirements."
      exit sub
   end if

   '
   ' Emit the config.
   '
   strPathPSDK = UnixSlashes(PathAbs(strPathPSDK))
   CfgPrint "PATH_SDK_WINPSDK" & g_strVerPSDK & "    := " & strPathPSDK
   CfgPrint "VBOX_WINPSDK          := WINPSDK" & g_strVerPSDK

   PrintResult "Windows Platform SDK (v" & g_strVerPSDK & ")", strPathPSDK
   g_strPathPSDK = strPathPSDK
end sub

''
' Checks if the specified path points to a usable PSDK.
function CheckForPlatformSDKSub(strPathPSDK)
   CheckForPlatformSDKSub = False
   LogPrint "trying: strPathPSDK=" & strPathPSDK
   if    LogFileExists(strPathPSDK, "include/Windows.h") _
    And  LogFileExists(strPathPSDK, "lib/Kernel32.Lib") _
    And  LogFileExists(strPathPSDK, "lib/User32.Lib") _
    And  LogFileExists(strPathPSDK, "bin/rc.exe") _
    And  Shell("""" & DosSlashes(strPathPSDK & "/bin/rc.exe") & """" , True) <> 0 _
      then
      if InStr(1, g_strShellOutput, "Resource Compiler Version 6.2.") > 0 then
         g_strVerPSDK = "80"
         CheckForPlatformSDKSub = True
      elseif InStr(1, g_strShellOutput, "Resource Compiler Version 6.1.") > 0 then
         g_strVerPSDK = "71"
         CheckForPlatformSDKSub = True
      end if
   end if
end function


''
' Checks for a Windows 7 Driver Kit.
sub CheckForWinDDK(strOptDDK)
   dim strPathDDK, str, strSubKeys
   PrintHdr "Windows DDK v7.1"

   '
   ' Find the DDK.
   '
   strPathDDK = ""
   ' The specified path.
   if strPathDDK = "" And strOptDDK <> "" then
      if CheckForWinDDKSub(strOptDDK, True) then strPathDDK = strOptDDK
   end if

   ' The tools location (first).
   if strPathDDK = "" And g_blnInternalFirst then
      str = g_strPathDev & "/win.x86/ddk/7600.16385.1"
      if CheckForWinDDKSub(str, False) then strPathDDK = str
   end if

   ' Check the environment
   str = EnvGet("DDK_INC_PATH")
   if strPathDDK = "" And str <> "" then
      str = PathParent(PathParent(str))
      if CheckForWinDDKSub(str, True) then strPathDDK = str
   end if

   str = EnvGet("BASEDIR")
   if strPathDDK = "" And str <> "" then
      if CheckForWinDDKSub(str, True) then strPathDDK = str
   end if

   ' Some array constants to ease the work.
   arrSoftwareKeys = array("SOFTWARE", "SOFTWARE\Wow6432Node")
   arrRoots        = array("HKLM", "HKCU")

   ' Windows 7 WDK.
   arrLocations = array()
   for each strSoftwareKey in arrSoftwareKeys
      for each strSubKey in RegEnumSubKeysFull("HKLM", strSoftwareKey & "\Microsoft\KitSetup\configured-kits")
         for each strSubKey2 in RegEnumSubKeysFull("HKLM", strSubKey)
            str = RegGetString("HKLM\" & strSubKey2 & "\setup-install-location")
            if str <> "" then
               arrLocations = ArrayAppend(arrLocations, PathAbsLong(str))
            end if
         next
      next
   next
   arrLocations = ArrayRSortStrings(arrLocations)

   ' Check the locations we've gathered.
   for each str in arrLocations
      if strPathDDK = "" then
         if CheckForWinDDKSub(str, True) then strPathDDK = str
      end if
   next

   ' The tools location (post).
   if (strPathDDK = "") And (g_blnInternalFirst = False) then
      str = g_strPathDev & "/win.x86/ddk/7600.16385.1"
      if CheckForWinDDKSub(str, False) then strPathDDK = str
   end if

   ' Give up.
   if strPathDDK = "" then
      MsgError "Cannot find the Windows DDK v7.1. Check configure.log and the build requirements."
      exit sub
   end if

   '
   ' Emit the config.
   '
   strPathDDK = UnixSlashes(PathAbs(strPathDDK))
   CfgPrint "PATH_SDK_WINDDK71     := " & strPathDDK

   PrintResult "Windows DDK v7.1", strPathDDK
   g_strPathDDK = strPathDDK
end sub

'' Quick check if the DDK is in the specified directory or not.
function CheckForWinDDKSub(strPathDDK, blnCheckBuild)
   CheckForWinDDKSub = False
   LogPrint "trying: strPathDDK=" & strPathDDK & " blnCheckBuild=" & blnCheckBuild
   if   LogFileExists(strPathDDK, "inc/api/ntdef.h") _
    And LogFileExists(strPathDDK, "lib/win7/i386/int64.lib") _
    And LogFileExists(strPathDDK, "lib/wlh/i386/int64.lib") _
    And LogFileExists(strPathDDK, "lib/wnet/i386/int64.lib") _
    And LogFileExists(strPathDDK, "lib/wxp/i386/int64.lib") _
    And Not LogFileExists(strPathDDK, "lib/win8/i386/int64.lib") _
    And LogFileExists(strPathDDK, "bin/x86/rc.exe") _
      then
      if Not blnCheckBuild then
         CheckForWinDDKSub = True
      '' @todo Find better build check.
      elseif Shell("""" & DosSlashes(strPathDDK & "/bin/x86/rc.exe") & """" , True) <> 0 _
         And InStr(1, g_strShellOutput, "Resource Compiler Version 6.1.") > 0 then
         CheckForWinDDKSub = True
      end if
   end if
end function


''
' Finds midl.exe
sub CheckForMidl()
   dim strMidl
   PrintHdr "Midl.exe"

   ' Skip if no COM/ATL.
   if g_blnDisableCOM then
      PrintResultMsg "Midl", "Skipped (" & g_strDisableCOM & ")"
      exit sub
   end if

   if LogFileExists(g_strPathPSDK, "bin/Midl.exe") then
      strMidl = g_strPathPSDK & "/bin/Midl.exe"
   elseif LogFileExists(g_strPathVCC, "Common7/Tools/Bin/Midl.exe") then
      strMidl = g_strPathVCC & "/Common7/Tools/Bin/Midl.exe"
   elseif LogFileExists(g_strPathDDK, "bin/x86/Midl.exe") then
      strMidl = g_strPathDDK & "/bin/x86/Midl.exe"
   elseif LogFileExists(g_strPathDDK, "bin/Midl.exe") then
      strMidl = g_strPathDDK & "/bin/Midl.exe"
   elseif LogFileExists(g_strPathDev, "win.x86/bin/Midl.exe") then
      strMidl = g_strPathDev & "/win.x86/bin/Midl.exe"
   else
      MsgWarning "Midl.exe not found!"
      exit sub
   end if

   CfgPrint "VBOX_MAIN_IDL         := " & strMidl
   PrintResult "Midl.exe", strMidl
end sub


''
' Checks for a MinGW32 suitable for building the recompiler.
'
' strOptW32API is currently ignored.
'
sub CheckForMinGW32(strOptMinGW32, strOptW32API)
   dim strPathMingW32, strPathW32API, str
   PrintHdr "MinGW32 GCC v3.3.x + Binutils + Runtime + W32API"

   '
   ' Find the MinGW and W32API tools.
   '
   strPathMingW32 = ""
   strPathW32API = ""

   ' The specified path.
   if (strPathMingW32 = "") And (strOptMinGW32 <> "") then
      if CheckForMinGW32Sub(strOptMinGW32, strOptW32API) then
         strPathMingW32 = strOptMinGW32
         strPathW32API = strOptW32API
      end if
   end if

   ' The tools location (first).
   if (strPathMingW32 = "") And (g_blnInternalFirst = True) then
      str = g_strPathDev & "/win.x86/mingw32/v3.3.3"
      str2 = g_strPathDev & "/win.x86/w32api/v2.5"
      if CheckForMinGW32Sub(str, str2) then
         strPathMingW32 = str
         strPathW32API = str2
      end if
   end if

   ' See if there is any gcc around.
   if strPathMingW32 = "" then
      str = Which("mingw32-gcc.exe")
      if (str <> "") then
         str = PathParent(PathStripFilename(str))
         if CheckForMinGW32Sub(str, str) then strPathMingW32 = str
      end if
   end if

   if strPathMingW32 = "" then
      str = Which("gcc.exe")
      if (str <> "") then
         str = PathParent(PathStripFilename(str))
         if CheckForMinGW32Sub(str, str) then strPathMingW32 = str
      end if
   end if

   ' The tools location (post).
   if (strPathMingW32 = "") And (g_blnInternalFirst = False) then
      str = g_strPathDev & "/win.x86/mingw32/v3.3.3"
      str2 = g_strPathDev & "/win.x86/w32api/v2.5"
      if CheckForMinGW32Sub(str, str2) then
         strPathMingW32 = str
         strPathW32API = str2
      end if
   end if

   ' Success?
   if strPathMingW32 = "" then
      if g_strTargetArch = "amd64" then
         MsgWarning "Can't locate a suitable MinGW32 installation, ignoring since we're targeting AMD64 and won't need it."
      elseif strOptMinGW32 = "" then
         MsgError "Can't locate a suitable MinGW32 installation. Try specify the path with " _
            & "the --with-MinGW32=<path> argument. If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate a suitable MinGW32 installation. Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   '
   ' Emit the config.
   '
   strPathMingW32 = UnixSlashes(PathAbs(strPathMingW32))
   CfgPrint "PATH_TOOL_MINGW32     := " & strPathMingW32
   PrintResult "MinGW32 (GCC v" & g_strSubOutput & ")", strPathMingW32
   if (strPathMingW32 = strPathW32API) Or strPathW32API = "" then
      CfgPrint "PATH_SDK_W32API       := $(PATH_TOOL_MINGW32)"
   else
      CfgPrint "PATH_SDK_W32API       := " & strPathW32API
      PrintResult "W32API", strPathW32API
   end if
end sub

''
' Checks if the specified path points to an usable MinGW or not.
function CheckForMinGW32Sub(strPathMingW32, strPathW32API)
   g_strSubOutput = ""
   if strPathW32API = "" then strPathW32API = strPathMingW32
   LogPrint "trying: strPathMingW32="  &strPathMingW32 & " strPathW32API=" & strPathW32API

   if   LogFileExists(strPathMingW32, "bin/mingw32-gcc.exe") _
    And LogFileExists(strPathMingW32, "bin/ld.exe") _
    And LogFileExists(strPathMingW32, "bin/objdump.exe") _
    And LogFileExists(strPathMingW32, "bin/dllwrap.exe") _
    And LogFileExists(strPathMingW32, "bin/as.exe") _
    And LogFileExists(strPathMingW32, "include/string.h") _
    And LogFileExists(strPathMingW32, "include/_mingw.h") _
    And LogFileExists(strPathMingW32, "lib/dllcrt1.o") _
    And LogFileExists(strPathMingW32, "lib/dllcrt2.o") _
    And LogFileExists(strPathMingW32, "lib/libmsvcrt.a") _
    _
    And LogFileExists(strPathW32API, "lib/libkernel32.a") _
    And LogFileExists(strPathW32API, "include/windows.h") _
      then
      if Shell(DosSlashes(strPathMingW32 & "/bin/gcc.exe") & " --version", True) = 0 then
         dim offVer, iMajor, iMinor, iPatch, strVer

         ' extract the version.
         strVer = ""
         offVer = InStr(1, g_strShellOutput, "(GCC) ")
         if offVer > 0 then
            strVer = LTrim(Mid(g_strShellOutput, offVer + Len("(GCC) ")))
            strVer = RTrim(Left(strVer, InStr(1, strVer, " ")))
            if   (Mid(strVer, 2, 1) = ".") _
             And (Mid(strVer, 4, 1) = ".") then
               iMajor = Int(Left(strVer, 1)) ' Is Int() the right thing here? I want atoi()!!!
               iMinor = Int(Mid(strVer, 3, 1))
               iPatch = Int(Mid(strVer, 5))
            else
               LogPrint "Malformed version: '" & strVer & "'"
               strVer = ""
            end if
         end if
         if strVer <> "" then
            if (iMajor = 3) And (iMinor = 3) then
               CheckForMinGW32Sub = True
               g_strSubOutput = strVer
            else
               LogPrint "MinGW32 version '" & iMajor & "." & iMinor & "." & iPatch & "' is not supported (or configure.vbs failed to parse it correctly)."
            end if
         else
            LogPrint "Couldn't locate the GCC version in the output!"
         end if

      else
         LogPrint "Failed to run gcc.exe!"
      end if
   end if
end function


''
' Checks for a MinGW-w64 suitable for building the recompiler.
sub CheckForMinGWw64(strOptMinGWw64)
   dim strPathMingWw64, str
   PrintHdr "MinGW-w64 GCC (unprefixed)"

   '
   ' Find the MinGW-w64 tools.
   '
   strPathMingWw64 = ""

   ' The specified path.
   if (strPathMingWw64 = "") And (strOptMinGWw64 <> "") then
      if CheckForMinGWw64Sub(strOptMinGWw64) then
         strPathMingWw64 = strOptMinGWw64
      end if
   end if

   ' The tools location (first).
   if (strPathMinGWw64 = "") And (g_blnInternalFirst = True) then
      str = g_strPathDev & "/win.amd64/mingw-w64/r1"
      if CheckForMinGWw64Sub(str) then
         strPathMinGWw64 = str
      end if
   end if

   ' See if there is any gcc around.
   if strPathMinGWw64 = "" then
      str = Which("x86_64-w64-mingw32-gcc.exe")
      if (str <> "") then
         str = PathParent(PathStripFilename(str))
         if CheckForMinGWw64Sub(str) then strPathMinGWw64 = str
      end if
   end if

   if strPathMinGWw64 = "" then
      str = Which("gcc.exe")
      if (str <> "") then
         str = PathParent(PathStripFilename(str))
         if CheckForMinGWw64Sub(str) then strPathMinGWw64 = str
      end if
   end if

   ' The tools location (post).
   if (strPathMinGWw64 = "") And (g_blnInternalFirst = False) then
      str = g_strPathDev & "/win.amd64/mingw-w64/r1"
      if CheckForMinGWw64Sub(str) then
         strPathMinGWw64 = str
      end if
   end if

   ' Success?
   if strPathMinGWw64 = "" then
      if g_strTargetArch = "x86" then
         MsgWarning "Can't locate a suitable MinGW-w64 installation, ignoring since we're targeting x86 and won't need it."
      elseif strOptMinGWw64 = "" then
         MsgError "Can't locate a suitable MinGW-w64 installation. Try specify the path with " _
            & "the --with-MinGW-w64=<path> argument. If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate a suitable MinGW-w64 installation. Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   '
   ' Emit the config.
   '
   strPathMinGWw64 = UnixSlashes(PathAbs(strPathMinGWw64))
   CfgPrint "PATH_TOOL_MINGWW64    := " & strPathMinGWw64
   PrintResult "MinGW-w64 (GCC v" & g_strSubOutput & ")", strPathMinGWw64
end sub

''
' Checks if the specified path points to an usable MinGW-w64 or not.
function CheckForMinGWw64Sub(strPathMinGWw64)
   g_strSubOutput = ""
   LogPrint "trying: strPathMinGWw64="  &strPathMinGWw64

   if   LogFileExists(strPathMinGWw64, "bin/gcc.exe") _
    And LogFileExists(strPathMinGWw64, "bin/ld.exe") _
    And LogFileExists(strPathMinGWw64, "bin/objdump.exe") _
    And LogFileExists(strPathMinGWw64, "bin/dllwrap.exe") _
    And LogFileExists(strPathMinGWw64, "bin/dlltool.exe") _
    And LogFileExists(strPathMinGWw64, "bin/as.exe") _
    And LogFileExists(strPathMinGWw64, "include/bfd.h") _
    And LogFileExists(strPathMinGWw64, "lib64/libgcc_s.a") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/lib/dllcrt1.o") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/lib/dllcrt2.o") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/lib/libmsvcrt.a") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/lib/libmsvcr100.a") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/include/_mingw.h") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/include/stdint.h") _
    And LogFileExists(strPathMinGWw64, "x86_64-w64-mingw32/include/windows.h") _
      then
      if Shell(DosSlashes(strPathMinGWw64 & "/bin/gcc.exe") & " -dumpversion", True) = 0 then
         dim offVer, iMajor, iMinor, iPatch, strVer

         ' extract the version.
         strVer = Trim(Replace(Replace(g_strShellOutput, vbCr, ""), vbLf, ""))
         if   (Mid(strVer, 2, 1) = ".") _
          And (Mid(strVer, 4, 1) = ".") then
            iMajor = Int(Left(strVer, 1)) ' Is Int() the right thing here? I want atoi()!!!
            iMinor = Int(Mid(strVer, 3, 1))
            iPatch = Int(Mid(strVer, 5))
         else
            LogPrint "Malformed version: '" & strVer & "'"
            strVer = ""
         end if
         if strVer <> "" then
            if (iMajor = 4) And (iMinor >= 4) then
               CheckForMinGWw64Sub = True
               g_strSubOutput = strVer
            else
               LogPrint "MinGW-w64 version '" & iMajor & "." & iMinor & "." & iPatch & "' is not supported (or configure.vbs failed to parse it correctly)."
            end if
         else
            LogPrint "Couldn't locate the GCC version in the output!"
         end if

      else
         LogPrint "Failed to run gcc.exe!"
      end if
   end if
end function


''
' Checks for any libSDL binaries.
sub CheckForlibSDL(strOptlibSDL)
   dim strPathlibSDL, str
   PrintHdr "libSDL"

   '
   ' Try find some SDL library.
   '

   ' First, the specific location.
   strPathlibSDL = ""
   if (strPathlibSDL = "") And (strOptlibSDL <> "") then
      if CheckForlibSDLSub(strOptlibSDL) then strPathlibSDL = strOptlibSDL
   end if

   ' The tools location (first).
   if (strPathlibSDL = "") And (g_blnInternalFirst = True) Then
      str = g_strPathDev & "/win.x86/libsdl/v1.2.11"
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   if (strPathlibSDL = "") And (g_blnInternalFirst = True) Then
      str = g_strPathDev & "/win.x86/libsdl/v1.2.7-InnoTek"
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   ' Poke about in the path.
   str = Which("SDLmain.lib")
   if (strPathlibSDL = "") And (str <> "") Then
      str = PathParent(PathStripFilename(str))
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   str = Which("SDL.dll")
   if (strPathlibSDL = "") And (str <> "") Then
      str = PathParent(PathStripFilename(str))
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   ' The tools location (post).
   if (strPathlibSDL = "") And (g_blnInternalFirst = False) Then
      str = g_strPathDev & "/win.x86/libsdl/v1.2.11"
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   if (strPathlibSDL = "") And (g_blnInternalFirst = False) Then
      str = g_strPathDev & "/win.x86/libsdl/v1.2.7-InnoTek"
      if CheckForlibSDLSub(str) then strPathlibSDL = str
   end if

   ' Success?
   if strPathlibSDL = "" then
      if strOptlibSDL = "" then
         MsgError "Can't locate libSDL. Try specify the path with the --with-libSDL=<path> argument. " _
                & "If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate libSDL. Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   strPathLibSDL = UnixSlashes(PathAbs(strPathLibSDL))
   CfgPrint "PATH_SDK_LIBSDL       := " & strPathlibSDL

   PrintResult "libSDL", strPathlibSDL
end sub

''
' Checks if the specified path points to an usable libSDL or not.
function CheckForlibSDLSub(strPathlibSDL)
   CheckForlibSDLSub = False
   LogPrint "trying: strPathlibSDL=" & strPathlibSDL
   if   LogFileExists(strPathlibSDL, "lib/SDL.lib") _
    And LogFileExists(strPathlibSDL, "lib/SDLmain.lib") _
    And LogFileExists(strPathlibSDL, "lib/SDL.dll") _
    And LogFileExists(strPathlibSDL, "include/SDL.h") _
    And LogFileExists(strPathlibSDL, "include/SDL_syswm.h") _
    And LogFileExists(strPathlibSDL, "include/SDL_version.h") _
      then
      CheckForlibSDLSub = True
   end if
end function


''
' Checks for libxml2.
sub CheckForXml2(strOptXml2)
   dim strPathXml2, str
   PrintHdr "libxml2"

   ' Skip if no COM/ATL.
   if g_blnDisableCOM then
      PrintResultMsg "libxml2", "Skipped (" & g_strDisableCOM & ")"
      exit sub
   end if

   '
   ' Try find some xml2 dll/lib.
   '
   strPathXml2 = ""
   if (strPathXml2 = "") And (strOptXml2 <> "") then
      if CheckForXml2Sub(strOptXml2) then strPathXml2 = strOptXml2
   end if

   if strPathXml2 = "" Then
      str = Which("libxml2.lib")
      if str <> "" Then
         str = PathParent(PathStripFilename(str))
         if CheckForXml2Sub(str) then strPathXml2 = str
      end if
   end if

   ' Ignore failure if we're in 'internal' mode.
   if (strPathXml2 = "") and g_blnInternalMode then
      PrintResultMsg "libxml2", "ignored (internal mode)"
      exit sub
   end if

   ' Success?
   if strPathXml2 = "" then
      if strOptXml2 = "" then
         MsgError "Can't locate libxml2. Try specify the path with the --with-libxml2=<path> argument. " _
                & "If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate libxml2. Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   strPathXml2 = UnixSlashes(PathAbs(strPathXml2))
   CfgPrint "SDK_VBOX_LIBXML2_INCS  := " & strPathXml2 & "/include"
   CfgPrint "SDK_VBOX_LIBXML2_LIBS  := " & strPathXml2 & "/lib/libxml2.lib"

   PrintResult "libxml2", strPathXml2
end sub

''
' Checks if the specified path points to an usable libxml2 or not.
function CheckForXml2Sub(strPathXml2)
   dim str

   CheckForXml2Sub = False
   LogPrint "trying: strPathXml2=" & strPathXml2
   if   LogFileExists(strPathXml2, "include/libxml/xmlexports.h") _
    And LogFileExists(strPathXml2, "include/libxml/xmlreader.h") _
      then
      str = LogFindFile(strPathXml2, "bin/libxml2.dll")
      if str <> "" then
         if LogFindFile(strPathXml2, "lib/libxml2.lib") <> "" then
            CheckForXml2Sub = True
         end if
      end if
   end if
end function


''
' Checks for openssl
sub CheckForSsl(strOptSsl, bln32Bit)
   dim strPathSsl, str
   PrintHdr "openssl"

   strOpenssl = "openssl"
   if bln32Bit = True then
       strOpenssl = "openssl32"
   end if

   '
   ' Try find some openssl dll/lib.
   '
   strPathSsl = ""
   if (strPathSsl = "") And (strOptSsl <> "") then
      if CheckForSslSub(strOptSsl) then strPathSsl = strOptSsl
   end if

   if strPathSsl = "" Then
      str = Which("libssl.lib")
      if str <> "" Then
         str = PathParent(PathStripFilename(str))
         if CheckForSslSub(str) then strPathSsl = str
      end if
   end if

   ' Ignore failure if we're in 'internal' mode.
   if (strPathSsl = "") and g_blnInternalMode then
      PrintResultMsg strOpenssl, "ignored (internal mode)"
      exit sub
   end if

   ' Success?
   if strPathSsl = "" then
      if strOptSsl = "" then
         MsgError "Can't locate " & strOpenssl & ". " _
                & "Try specify the path with the --with-" & strOpenssl & "=<path> argument. " _
                & "If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate " & strOpenssl & ". " _
                & "Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   strPathSsl = UnixSlashes(PathAbs(strPathSsl))
   if bln32Bit = True then
      CfgPrint "SDK_VBOX_OPENSSL-x86_INCS := " & strPathSsl & "/include"
      CfgPrint "SDK_VBOX_OPENSSL-x86_LIBS := " & strPathSsl & "/lib/libcrypto.lib" & " " & strPathSsl & "/lib/libssl.lib"
      CfgPrint "SDK_VBOX_BLD_OPENSSL-x86_LIBS := " & strPathSsl & "/lib/libcrypto.lib" & " " & strPathSsl & "/lib/libssl.lib"
   else
      CfgPrint "SDK_VBOX_OPENSSL_INCS := " & strPathSsl & "/include"
      CfgPrint "SDK_VBOX_OPENSSL_LIBS := " & strPathSsl & "/lib/libcrypto.lib" & " " & strPathSsl & "/lib/libssl.lib"
      CfgPrint "SDK_VBOX_BLD_OPENSSL_LIBS := " & strPathSsl & "/lib/libcrypto.lib" & " " & strPathSsl & "/lib/libssl.lib"
   end if

   PrintResult strOpenssl, strPathSsl
end sub

''
' Checks if the specified path points to an usable openssl or not.
function CheckForSslSub(strPathSsl)

   CheckForSslSub = False
   LogPrint "trying: strPathSsl=" & strPathSsl
   if   LogFileExists(strPathSsl, "include/openssl/md5.h") _
    And LogFindFile(strPathSsl, "lib/libssl.lib") <> "" _
      then
         CheckForSslSub = True
      end if
end function


''
' Checks for libcurl
sub CheckForCurl(strOptCurl, bln32Bit)
   dim strPathCurl, str
   PrintHdr "libcurl"

   strCurl = "libcurl"
   if bln32Bit = True then
       strCurl = "libcurl32"
   end if

   '
   ' Try find some cURL dll/lib.
   '
   strPathCurl = ""
   if (strPathCurl = "") And (strOptCurl <> "") then
      if CheckForCurlSub(strOptCurl) then strPathCurl = strOptCurl
   end if

   if strPathCurl = "" Then
      str = Which("libcurl.lib")
      if str <> "" Then
         str = PathParent(PathStripFilename(str))
         if CheckForCurlSub(str) then strPathCurl = str
      end if
   end if

   ' Ignore failure if we're in 'internal' mode.
   if (strPathCurl = "") and g_blnInternalMode then
      PrintResultMsg strCurl, "ignored (internal mode)"
      exit sub
   end if

   ' Success?
   if strPathCurl = "" then
      if strOptCurl = "" then
         MsgError "Can't locate " & strCurl & ". " _
                & "Try specify the path with the --with-" & strCurl & "=<path> argument. " _
                & "If still no luck, consult the configure.log and the build requirements."
      else
         MsgError "Can't locate " & strCurl & ". " _
                & "Please consult the configure.log and the build requirements."
      end if
      exit sub
   end if

   strPathCurl = UnixSlashes(PathAbs(strPathCurl))
   if bln32Bit = True then
      CfgPrint "SDK_VBOX_LIBCURL-x86_INCS := " & strPathCurl & "/include"
      CfgPrint "SDK_VBOX_LIBCURL-x86_LIBS.x86 := " & strPathCurl & "/libcurl.lib"
   else
      CfgPrint "SDK_VBOX_LIBCURL_INCS := " & strPathCurl & "/include"
      CfgPrint "SDK_VBOX_LIBCURL_LIBS := " & strPathCurl & "/libcurl.lib"
   end if

   PrintResult strCurl, strPathCurl
end sub

''
' Checks if the specified path points to an usable libcurl or not.
function CheckForCurlSub(strPathCurl)

   CheckForCurlSub = False
   LogPrint "trying: strPathCurl=" & strPathCurl
   if   LogFileExists(strPathCurl, "include/curl/curl.h") _
    And LogFindFile(strPathCurl, "libcurl.dll") <> "" _
    And LogFindFile(strPathCurl, "libcurl.lib") <> "" _
      then
         CheckForCurlSub = True
      end if
end function



''
' Checks for any Qt5 binaries.
sub CheckForQt(strOptQt5)
   PrintHdr "Qt5"

   '
   ' Try to find the Qt5 installation (user specified path with --with-qt5)
   '
   strPathQt5 = ""

   LogPrint "Checking for user specified path of Qt5 ... "
   if (strPathQt5 = "") And (strOptQt5 <> "") then
      strOptQt5 = UnixSlashes(strOptQt5)
      if CheckForQt5Sub(strOptQt5) then strPathQt5 = strOptQt5
   end if

   ' Check the dev tools
   if (strPathQt5 = "") Then
      strPathQt5 = g_strPathDev & "/win." & g_strTargetArch & "/qt/v5.5.1-r138"
      if CheckForQt5Sub(strPathQt5) = False then strPathQt5 = ""
   end if

   ' Display the result.
   if strPathQt5 = "" then
      PrintResultMsg "Qt5", "not found"
   else
      PrintResult "Qt5", strPathQt5
   end if

   if strPathQt5 <> "" then
      CfgPrint "PATH_SDK_QT5          := " & strPathQt5
      CfgPrint "PATH_TOOL_QT5         := $(PATH_SDK_QT5)"
      CfgPrint "VBOX_PATH_QT          := $(PATH_SDK_QT5)"
   end if
   if strPathQt5 = "" then
      CfgPrint "VBOX_WITH_QTGUI       :="
   end if
end sub


''
' Checks if the specified path points to an usable Qt5 library.
function CheckForQt5Sub(strPathQt5)

   CheckForQt5Sub = False
   LogPrint "trying: strPathQt5=" & strPathQt5

   if   LogFileExists(strPathQt5, "bin/moc.exe") _
    And LogFileExists(strPathQt5, "bin/uic.exe") _
    And LogFileExists(strPathQt5, "include/QtWidgets/qwidget.h") _
    And LogFileExists(strPathQt5, "include/QtWidgets/QApplication") _
    And LogFileExists(strPathQt5, "include/QtGui/QImage") _
    And LogFileExists(strPathQt5, "include/QtNetwork/QHostAddress") _
    And (   LogFileExists(strPathQt5, "lib/Qt5Core.lib") _
         Or LogFileExists(strPathQt5, "lib/Qt5CoreVBox.lib")) _
    And (   LogFileExists(strPathQt5, "lib/Qt5Network.lib") _
         Or LogFileExists(strPathQt5, "lib/Qt5NetworkVBox.lib")) _
      then
         CheckForQt5Sub = True
   end if

end function


'
'
function CheckForPython(strPathPython)

   PrintHdr "Python"

   CheckForPython = False
   LogPrint "trying: strPathPython=" & strPathPython

   if LogFileExists(strPathPython, "python.exe") then
      CfgPrint "VBOX_BLD_PYTHON       := " & strPathPython & "\python.exe"
      CheckForPython = True
   end if

   PrintResult "Python ", strPathPython
end function


'
'
function CheckForMkisofs(strFnameMkisofs)

   PrintHdr "mkisofs"

   CheckForMkisofs = False
   LogPrint "trying: strFnameMkisofs=" & strFnameMkisofs

   if FileExists(strFnameMkisofs) = false then
      LogPrint "Testing '" & strFnameMkisofs & " not found"
   else
      CfgPrint "VBOX_MKISOFS          := " & strFnameMkisofs
      CheckForMkisofs = True
   end if

   PrintResult "mkisofs ", strFnameMkisofs
end function


''
' Show usage.
sub usage
   Print "Usage: cscript configure.vbs [options]"
   Print ""
   Print "Configuration:"
   Print "  -h, --help"
   Print "  --internal"
   Print "  --internal-last"
   Print "  --target-arch=x86|amd64"
   Print ""
   Print "Components:"
   Print "  --disable-COM"
   Print "  --disable-UDPTunnel"
   Print "  --disable-SDL"
   Print ""
   Print "Locations:"
   Print "  --with-kBuild=PATH    "
   Print "  --with-libSDL=PATH    "
   Print "  --with-MinGW32=PATH   "
   Print "  --with-MinGW-w64=PATH "
   Print "  --with-Qt5=PATH       "
   Print "  --with-DDK=PATH       "
   Print "  --with-SDK=PATH       "
   Print "  --with-VC=PATH        "
   Print "  --with-VC-Common=PATH "
   Print "  --with-VC-Express-Edition"
   Print "  --with-W32API=PATH    "
   Print "  --with-libxml2=PATH   "
   Print "  --with-openssl=PATH   "
   Print "  --with-openssl32=PATH (only for 64-bit targets)"
   Print "  --with-libcurl=PATH   "
   Print "  --with-libcurl32=PATH (only for 64-bit targets)"
   Print "  --with-python=PATH    "
   Print "  --with-mkisofs=PATH   "
end sub


''
' The main() like function.
'
Sub Main
   '
   ' Write the log header and check that we're not using wscript.
   '
   LogInit
   If UCase(Right(Wscript.FullName, 11)) = "WSCRIPT.EXE" Then
      Wscript.Echo "This script must be run under CScript."
      Wscript.Quit(1)
   End If

   '
   ' Parse arguments.
   '
   strOptDDK = ""
   strOptDXDDK = ""
   strOptkBuild = ""
   strOptlibSDL = ""
   strOptMinGW32 = ""
   strOptMinGWw64 = ""
   strOptQt5 = ""
   strOptSDK = ""
   strOptVC = ""
   strOptVCCommon = ""
   blnOptVCExpressEdition = False
   strOptW32API = ""
   strOptXml2 = ""
   strOptSsl = ""
   strOptSsl32 = ""
   strOptCurl = ""
   strOptCurl32 = ""
   strOptPython = ""
   strOptMkisofs = ""
   blnOptDisableCOM = False
   blnOptDisableUDPTunnel = False
   blnOptDisableSDL = False
   for i = 1 to Wscript.Arguments.Count
      dim str, strArg, strPath

      ' Separate argument and path value
      str = Wscript.Arguments.item(i - 1)
      if InStr(1, str, "=") > 0 then
         strArg = Mid(str, 1, InStr(1, str, "=") - 1)
         strPath = Mid(str, InStr(1, str, "=") + 1)
         if strPath = "" then MsgFatal "Syntax error! Argument #" & i & " is missing the path."
      else
         strArg = str
         strPath = ""
      end if

      ' Process the argument
      select case LCase(strArg)
         case "--with-ddk"
            strOptDDK = strPath
         case "--with-dxsdk"
            MsgWarning "Ignoring --with-dxsdk (the DirectX SDK is no longer required)."
         case "--with-kbuild"
            strOptkBuild = strPath
         case "--with-libsdl"
            strOptlibSDL = strPath
         case "--with-mingw32"
            strOptMinGW32 = strPath
         case "--with-mingw-w64"
            strOptMinGWw64 = strPath
         case "--with-qt5"
            strOptQt5 = strPath
         case "--with-sdk"
            strOptSDK = strPath
         case "--with-vc"
            strOptVC = strPath
         case "--with-vc-common"
            strOptVCCommon = strPath
         case "--with-vc-express-edition"
            blnOptVCExpressEdition = True
         case "--with-w32api"
            strOptW32API = strPath
         case "--with-libxml2"
            strOptXml2 = strPath
         case "--with-openssl"
            strOptSsl = strPath
         case "--with-openssl32"
            strOptSsl32 = strPath
         case "--with-libcurl"
            strOptCurl = strPath
         case "--with-libcurl32"
            strOptCurl32 = strPath
         case "--with-python"
            strOptPython = strPath
         case "--with-mkisofs"
            strOptMkisofs = strPath
         case "--disable-com"
            blnOptDisableCOM = True
         case "--enable-com"
            blnOptDisableCOM = False
         case "--disable-udptunnel"
            blnOptDisableUDPTunnel = True
         case "--disable-sdl"
            blnOptDisableSDL = True
         case "--internal"
            g_blnInternalMode = True
         case "--internal-last"
            g_blnInternalFirst = False
         case "--target-arch"
            g_strTargetArch = strPath
         case "-h", "--help", "-?"
            usage
            Wscript.Quit(0)
         case else
            Wscript.echo "syntax error: Unknown option '" & str &"'."
            usage
            Wscript.Quit(1)
      end select
   next

   '
   ' Initialize output files.
   '
   CfgInit
   EnvInit

   '
   ' Check that the Shell function is sane.
   '
   g_objShell.Environment("PROCESS")("TESTING_ENVIRONMENT_INHERITANCE") = "This works"
   if Shell("set TESTING_ENVIRONMENT_INHERITANC", False) <> 0 then ' The 'E' is missing on purpose (4nt).
      MsgFatal "shell execution test failed!"
   end if
   if g_strShellOutput <> "TESTING_ENVIRONMENT_INHERITANCE=This works" & CHR(13) & CHR(10)  then
      Print "Shell test Test -> '" & g_strShellOutput & "'"
      MsgFatal "shell inheritance or shell execution isn't working right. Make sure you use cmd.exe."
   end if
   g_objShell.Environment("PROCESS")("TESTING_ENVIRONMENT_INHERITANCE") = ""
   Print "Shell inheritance test: OK"

   '
   ' Do the checks.
   '
   if blnOptDisableCOM = True then
      DisableCOM "--disable-com"
   end if
   if blnOptDisableUDPTunnel = True then
      DisableUDPTunnel "--disable-udptunnel"
   end if
   CheckSourcePath
   CheckForkBuild strOptkBuild
   CheckForWinDDK strOptDDK
   CfgPrint "VBOX_WITH_WDDM_W8     := " '' @todo look for WinDDKv8
   CheckForVisualCPP strOptVC, strOptVCCommon, blnOptVCExpressEdition
   CheckForPlatformSDK strOptSDK
   CheckForMidl
   CheckForMinGW32 strOptMinGW32, strOptW32API
   CheckForMinGWw64 strOptMinGWw64
   CfgPrint "VBOX_WITH_OPEN_WATCOM := " '' @todo look for openwatcom 1.9+
   EnvPrint "set PATH=%PATH%;" & g_strPath& "/tools/win." & g_strTargetArch & "/bin;" '' @todo look for yasm
   if blnOptDisableSDL = True then
      DisableSDL "--disable-sdl"
   else
      CheckForlibSDL strOptlibSDL
   end if
   ' Don't check for this library by default as it's part of the tarball
   ' Using an external library can add a dependency to iconv
   if (strOptXml2 <> "") then
      CheckForXml2 strOptXml2
   end if
   CheckForSsl strOptSsl, False
   if g_strTargetArch = "amd64" then
       ' 32-bit openssl required as well
       CheckForSsl strOptSsl32, True
   end if
   CheckForCurl strOptCurl, False
   if g_strTargetArch = "amd64" then
       ' 32-bit Curl required as well
       CheckForCurl strOptCurl32, True
   end if
   CheckForQt strOptQt5
   if (strOptPython <> "") then
     CheckForPython strOptPython
   end if
   if (strOptMkisofs <> "") then
     CheckForMkisofs strOptMkisofs
   end if
   if g_blnInternalMode then
      EnvPrint "call " & g_strPathDev & "/env.cmd %1 %2 %3 %4 %5 %6 %7 %8 %9"
   end if

   Print ""
   Print "Execute env.bat once before you start to build VBox:"
   Print ""
   Print "  env.bat"
   Print "  kmk"
   Print ""

End Sub


Main

