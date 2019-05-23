#
# Copyright (C) 2008-2010 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

all: run

run: 
	@echo !!!! Don\'t forget to start webserver with \"vboxwebsrv -t 10000\" !!!
	PYTHONPATH=../lib python ../../../glue/python/sample/vboxshell.py -w 

server:
	nohup vboxwebsrv -t 10000 &
