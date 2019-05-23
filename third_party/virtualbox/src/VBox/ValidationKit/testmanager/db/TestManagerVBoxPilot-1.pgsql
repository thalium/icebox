-- $Id: TestManagerVBoxPilot-1.pgsql $
--- @file
-- VBox Test Manager - Setup for the 1st VBox Pilot.
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

--
-- The user we assign all the changes too.
--
INSERT INTO Users (sUsername, sEmail, sFullName, sLoginName)
    VALUES ('vbox-pilot-config', 'pilot1@example.org', 'VBox Pilot Configurator', 'vbox-pilot-config');
\set idUserQuery '(SELECT uid FROM Users WHERE sUsername = \'vbox-pilot-config\')'

--
-- Configure a scheduling group with build sources.
--
INSERT INTO BuildSources (uidAuthor, sName, sProduct, sBranch, asTypes, asOsArches)
    VALUES (:idUserQuery, 'VBox trunk builds', 'VirtualBox', 'trunk', ARRAY['release', 'strict'],  NULL);

INSERT INTO BuildSources (uidAuthor, sName, sProduct, sBranch, asTypes, asOsArches)
    VALUES (:idUserQuery, 'VBox TestSuite trunk builds', 'VBox TestSuite', 'trunk', ARRAY['release'], NULL);

INSERT INTO SchedGroups (sName, sDescription, fEnabled, idBuildSrc, idBuildSrcTestSuite)
    VALUES ('VirtualBox Trunk', NULL, TRUE,
            (SELECT idBuildSrc FROM BuildSources WHERE sName = 'VBox trunk builds'),
            (SELECT idBuildSrc FROM BuildSources WHERE sName = 'VBox TestSuite trunk builds') );
\set idSchedGroupQuery '(SELECT idSchedGroup FROM SchedGroups WHERE sName = \'VirtualBox Trunk\')'

--
-- Configure three test groups.
--
INSERT INTO TestGroups (uidAuthor, sName)
    VALUES (:idUserQuery, 'VBox smoketests');
\set idGrpSmokeQuery        '(SELECT idTestGroup FROM TestGroups WHERE sName = \'VBox smoketests\')'
INSERT INTO SchedGroupMembers (idSchedGroup, idTestGroup, uidAuthor, idTestGroupPreReq)
    VALUES (:idSchedGroupQuery, :idGrpSmokeQuery, :idUserQuery, NULL);

INSERT INTO TestGroups (uidAuthor, sName)
    VALUES (:idUserQuery, 'VBox general');
\set idGrpGeneralQuery      '(SELECT idTestGroup FROM TestGroups WHERE sName = \'VBox general\')'
INSERT INTO SchedGroupMembers (idSchedGroup, idTestGroup, uidAuthor, idTestGroupPreReq)
    VALUES (:idSchedGroupQuery, :idGrpGeneralQuery, :idUserQuery, :idGrpSmokeQuery);

INSERT INTO TestGroups (uidAuthor, sName)
    VALUES (:idUserQuery, 'VBox benchmarks');
\set idGrpBenchmarksQuery   '(SELECT idTestGroup FROM TestGroups WHERE sName = \'VBox benchmarks\')'
INSERT INTO SchedGroupMembers (idSchedGroup, idTestGroup, uidAuthor, idTestGroupPreReq)
    VALUES (:idSchedGroupQuery, :idGrpBenchmarksQuery, :idUserQuery, :idGrpGeneralQuery);


--
-- Testcases
--
INSERT INTO TestCases (uidAuthor, sName, fEnabled, cSecTimeout, sBaseCmd, sTestSuiteZips)
    VALUES (:idUserQuery, 'VBox install', TRUE, 600,
            'validationkit/testdriver/vboxinstaller.py --vbox-build @BUILD_BINARIES@ @ACTION@ -- testdriver/base.py @ACTION@',
            '@VALIDATIONKIT_ZIP@');
INSERT INTO TestCaseArgs (idTestCase, uidAuthor, sArgs)
    VALUES ((SELECT idTestCase FROM TestCases WHERE sName = 'VBox install'), :idUserQuery, '');
INSERT INTO TestGroupMembers (idTestGroup, idTestCase, uidAuthor)
    VALUES (:idGrpSmokeQuery, (SELECT idTestCase FROM TestCases WHERE sName = 'VBox install'), :idUserQuery);

COMMIT WORK;

