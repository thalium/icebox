# -*- coding: utf-8 -*-
# $Id: config.py $

"""
Test Manager Configuration.
"""

__copyright__ = \
"""
Copyright (C) 2012-2017 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 118412 $"

import os;

## Test Manager version string.
g_ksVersion             = 'v0.1.0';
## Test Manager revision string.
g_ksRevision            = ('$Revision: 118412 $')[11:-2];

## Enable VBox specific stuff.
g_kfVBoxSpecific        = True;


## @name Used by the TMDatabaseConnection class.
# @{
g_ksDatabaseName        = 'testmanager';
g_ksDatabaseAddress     = None;
g_ksDatabasePort        = None;
g_ksDatabaseUser        = 'postgres';
g_ksDatabasePassword    = '';
## @}


## @name User handling.
## @{

## Whether login names are case insensitive (True) or case sensitive (False).
## @note Implemented by inserting lower case names into DB and lower case
##       bind variables in WHERE clauses.
g_kfLoginNameCaseInsensitive = True;

## @}


## @name File locations
## @{

## The TestManager directory.
g_ksTestManagerDir      = os.path.dirname(os.path.abspath(__file__));
## The Validation Kit directory.
g_ksValidationKitDir        = os.path.dirname(g_ksTestManagerDir);
## The TestManager htdoc directory.
g_ksTmHtDocDir          = os.path.join(g_ksTestManagerDir, 'htdocs');
## The TestManager download directory (under htdoc somewhere), for validationkit zips.
g_ksTmDownloadDir       = os.path.join(g_ksTmHtDocDir, 'download');
## The base URL relative path of the TM download directory (g_ksTmDownloadDir).
g_ksTmDownloadBaseUrlRel = 'htdocs/downloads';
## The root of the file area (referred to as TM_FILE_DIR in database docs).
g_ksFileAreaRootDir     = '/var/tmp/testmanager'
## The root of the file area with the zip files (best put on a big storage server).
g_ksZipFileAreaRootDir  = '/var/tmp/testmanager2'
## URL prefix for trac log viewer.
g_ksTracLogUrlPrefix    = 'https://linserv.de.oracle.com/vbox/log/'
## URL prefix for trac log viewer.
g_ksTracChangsetUrlFmt  = 'https://linserv.de.oracle.com/%(sRepository)s/changeset/%(iRevision)s'
## URL prefix for unprefixed build logs.
g_ksBuildLogUrlPrefix   = ''
## URL prefix for unprefixed build binaries.
g_ksBuildBinUrlPrefix   = '/builds/'
## The local path prefix for unprefixed build binaries. (Host file system, not web server.)
g_ksBuildBinRootDir     = '/mnt/builds/'
## File on the build binary share that can be used to check that it's mounted.
g_ksBuildBinRootFile    = 'builds.txt'
## @}


## @name Scheduling parameters
## @{

## The time to wait for a gang to gather (in seconds).
g_kcSecGangGathering                    = 600;
## The max time allowed to spend looking for a new task (in seconds).
g_kcSecMaxNewTask                       = 60;
## Minimum time since last task started.
g_kcSecMinSinceLastTask                 = 120; # (2 min)
## Minimum time since last failed task.
g_kcSecMinSinceLastFailedTask           = 180; # (3 min)

## @}



## @name Test result limits.
## In general, we will fail the test when reached and stop accepting further results.
## @{

## The max number of test results per test set.
g_kcMaxTestResultsPerTS = 4096;
## The max number of test results (children) per test result.
g_kcMaxTestResultsPerTR = 512;
## The max number of test result values per test set.
g_kcMaxTestValuesPerTS  = 4096;
## The max number of test result values per test result.
g_kcMaxTestValuesPerTR  = 256;
## The max number of test result message per test result.
g_kcMaxTestMsgsPerTR    = 4;
## The max test result nesting depth.
g_kcMaxTestResultDepth  = 10;

## The max length of a test result name.
g_kcchMaxTestResultName = 64;
## The max length of a test result value name.
g_kcchMaxTestValueName  = 56;
## The max length of a test result message.
g_kcchMaxTestMsg        = 128;

## The max size of the main log file.
g_kcMbMaxMainLog        = 32;
## The max size of an uploaded file (individual).
g_kcMbMaxUploadSingle   = 150;
## The max size of all uploaded file.
g_kcMbMaxUploadTotal    = 200;
## The max number of files that can be uploaded.
g_kcMaxUploads          = 256;
## @}

## @name Debug Features
## @{

## Enables extra DB exception information.
g_kfDebugDbXcpt         = True;

## Where to write the glue debug.
# None indicates apache error log, string indicates a file.
#g_ksSrcGlueDebugLogDst  = '/tmp/testmanager-srv-glue.log';
g_ksSrcGlueDebugLogDst  = None;
## Whether to enable CGI trace back in the server glue.
g_kfSrvGlueCgiTb        = False;
## Enables glue debug output.
g_kfSrvGlueDebug        = False;
## Timestamp and pid prefix the glue debug output.
g_kfSrvGlueDebugTS      = True;
## Enables task scheduler debug output to g_ksSrcGlueDebugLogDst.
g_kfSrvGlueDebugScheduler = False;

## Enables the SQL trace back.
g_kfWebUiSqlTrace       = False;
## Enables the explain in the SQL trace back.
g_kfWebUiSqlTraceExplain = False;
## Whether the postgresql version supports the TIMING option on EXPLAIN (>= 9.2).
g_kfWebUiSqlTraceExplainTiming = False;
## Display time spent processing the page.
g_kfWebUiProcessedIn    = True;
## Enables WebUI debug output.
g_kfWebUiDebug          = False;
## Enables WebUI SQL debug output print() calls (requires g_kfWebUiDebug).
g_kfWebUiSqlDebug       = False;
## Enables the debug panel at the bottom of the page.
g_kfWebUiDebugPanel     = True;

## Profile cgi/admin.py.
g_kfProfileAdmin        = False;
## Profile cgi/index.py.
g_kfProfileIndex        = False;

## When not None,
g_ksTestBoxDispXpctLog  = '/tmp/testmanager-testboxdisp-xcpt.log'
## @}

