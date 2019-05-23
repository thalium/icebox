-- $Id: st1-unload.pgsql $
--- @file
-- VBox Test Manager - Self Test #1 Database Unload File.
--

--
-- Copyright (C) 2012-2017 Oracle Corporation
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
\connect testmanager;

BEGIN WORK;

DELETE FROM TestBoxStatuses;
DELETE FROM SchedQueues;

DELETE FROM SchedGroupMembers   WHERE uidAuthor = 1112223331;
UPDATE TestBoxes SET idSchedGroup = 1 WHERE idSchedGroup IN ( SELECT idSchedGroup FROM SchedGroups WHERE uidAuthor = 1112223331 );
DELETE FROM SchedGroups         WHERE uidAuthor = 1112223331 OR sName = 'st1-group';

UPDATE TestSets SET idTestResult = NULL
    WHERE idTestCase IN ( SELECT idTestCase FROM TestCases WHERE uidAuthor = 1112223331 );

DELETE FROM TestResultValues
    WHERE idTestResult IN ( SELECT idTestResult FROM TestResults
                            WHERE idTestSet IN (  SELECT idTestSet  FROM TestSets
                                                  WHERE idTestCase IN ( SELECT idTestCase FROM TestCases
                                                                        WHERE uidAuthor = 1112223331 ) ) );
DELETE FROM TestResultFiles
    WHERE idTestResult IN ( SELECT idTestResult FROM TestResults
                            WHERE idTestSet IN (  SELECT idTestSet  FROM TestSets
                                                  WHERE idTestCase IN ( SELECT idTestCase FROM TestCases
                                                                        WHERE uidAuthor = 1112223331 ) ) );
DELETE FROM TestResultMsgs
    WHERE idTestResult IN ( SELECT idTestResult FROM TestResults
                            WHERE idTestSet IN (  SELECT idTestSet  FROM TestSets
                                                  WHERE idTestCase IN ( SELECT idTestCase FROM TestCases
                                                                        WHERE uidAuthor = 1112223331 ) ) );
DELETE FROM TestResults
    WHERE idTestSet IN (  SELECT idTestSet  FROM TestSets
                          WHERE idTestCase IN ( SELECT idTestCase FROM TestCases WHERE uidAuthor = 1112223331 ) );
DELETE FROM TestSets
    WHERE idTestCase IN ( SELECT idTestCase FROM TestCases WHERE uidAuthor = 1112223331 );

DELETE FROM TestCases           WHERE uidAuthor = 1112223331;
DELETE FROM TestCaseArgs        WHERE uidAuthor = 1112223331;
DELETE FROM TestGroups          WHERE uidAuthor = 1112223331 OR sName = 'st1-testgroup';
DELETE FROM TestGroupMembers    WHERE uidAuthor = 1112223331;

DELETE FROM BuildSources        WHERE uidAuthor = 1112223331;
DELETE FROM Builds              WHERE uidAuthor = 1112223331;
DELETE FROM BuildCategories     WHERE sProduct  = 'st1';

DELETE FROM Users               WHERE uid       = 1112223331;

COMMIT WORK;

