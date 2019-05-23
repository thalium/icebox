-- $Id: tmdb-r20-testcases-1-testgroups-1-schedgroups-1.pgsql $
--- @file
-- VBox Test Manager Database - Adds sComment to TestCases, TestGroups
--                              and SchedGroups.
--

--
-- Copyright (C) 2013-2017 Oracle Corporation
--
-- This file is part of VirtualBox Open Source Edition (OSE), as
-- available from http://www.virtualbox.org. This file is free software;
-- you can redistribute it and/or modify it under the terms of the GNU
-- General Public License (GPL) as published by the Free Software
-- Foundation, in version 2 as it comes in the "COPYING" file of the
-- VirtualBox OSE distribution. VirtualBox OSE is distributed in the
-- hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
--
-- The contents of this file may alternatively be used under the terms
-- of the Common Development and Distribution License Version 1.0
-- (CDDL) only, as it comes in the "COPYING.CDDL" file of the
-- VirtualBox OSE distribution, in which case the provisions of the
-- CDDL are applicable instead of those of the GPL.
--
-- You may elect to license modified versions of this file under the
-- terms and conditions of either the GPL or the CDDL or both.
--



\set ON_ERROR_STOP 1
\set AUTOCOMMIT 0

LOCK TABLE TestBoxes          IN ACCESS EXCLUSIVE MODE;
LOCK TABLE TestBoxStatuses    IN ACCESS EXCLUSIVE MODE;
LOCK TABLE TestCases          IN ACCESS EXCLUSIVE MODE;
LOCK TABLE TestGroups         IN ACCESS EXCLUSIVE MODE;
LOCK TABLE SchedGroups        IN ACCESS EXCLUSIVE MODE;

--
-- All the changes are rather simple and we'll just add the sComment column last.
--
\d TestCases;
\d TestGroups;
\d SchedGroups;

ALTER TABLE TestCases   ADD COLUMN sComment TEXT DEFAULT NULL;
ALTER TABLE TestGroups  ADD COLUMN sComment TEXT DEFAULT NULL;
ALTER TABLE SchedGroups ADD COLUMN sComment TEXT DEFAULT NULL;

\d TestCases;
\d TestGroups;
\d SchedGroups;

\prompt "Update python files while everything is locked. Hurry!"  dummy

COMMIT;

