# -*- coding: utf-8 -*-
# $Id: webservergluecgi.py $

"""
Test Manager Core - Web Server Abstraction Base Class.
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


# Standard python imports.
import cgitb;
import os;
import sys;
import cgi;

# Validation Kit imports.
from testmanager.core.webservergluebase import WebServerGlueBase;
from testmanager                        import config;


class WebServerGlueCgi(WebServerGlueBase):
    """
    CGI glue.
    """
    def __init__(self, sValidationKitDir, fHtmlOutput=True):
        WebServerGlueBase.__init__(self, sValidationKitDir, fHtmlOutput);

        if config.g_kfSrvGlueCgiTb is True:
            cgitb.enable(format=('html' if fHtmlOutput else 'text'));

    def getParameters(self):
        return cgi.parse(keep_blank_values=True);

    def getClientAddr(self):
        return os.environ.get('REMOTE_ADDR');

    def getMethod(self):
        return os.environ.get('REQUEST_METHOD', 'POST');

    def getLoginName(self):
        return os.environ.get('REMOTE_USER', WebServerGlueBase.ksUnknownUser);

    def getUrlScheme(self):
        return 'https' if 'HTTPS' in os.environ else 'http';

    def getUrlNetLoc(self):
        return os.environ['HTTP_HOST'];

    def getUrlPath(self):
        return os.environ['REQUEST_URI'];

    def getUserAgent(self):
        return os.getenv('HTTP_USER_AGENT', '');

    def getContentType(self):
        return cgi.parse_header(os.environ.get('CONTENT_TYPE', 'text/html'));

    def getContentLength(self):
        return int(os.environ.get('CONTENT_LENGTH', 0));

    def getBodyIoStream(self):
        return sys.stdin;

