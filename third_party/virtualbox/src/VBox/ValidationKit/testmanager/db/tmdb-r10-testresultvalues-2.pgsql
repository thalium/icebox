-- $Id: tmdb-r10-testresultvalues-2.pgsql $
--- @file
-- VBox Test Manager Database - Adds an idTestSet to TestResultValues.
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

--
-- Cleanup after failed runs.
--
DROP TABLE NewTestResultValues;

--
-- Drop all indexes (might already be dropped).
--
DROP INDEX TestResultValuesIdx;
DROP INDEX TestResultValuesNameIdx;

-- Die on error from now on.
\set ON_ERROR_STOP 1
\set AUTOCOMMIT 0

\d+ TestResultValues;

--
-- Create the new version of the table and filling with the content of the old.
--
CREATE TABLE NewTestResultValues (
    --- The ID of this value.
    idTestResultValue   INTEGER     DEFAULT NEXTVAL('TestResultValueIdSeq'), -- PRIMARY KEY
    --- The test result it was reported within.
    idTestResult        INTEGER     NOT NULL, -- REFERENCES TestResults(idTestResult)  NOT NULL,
    --- The test result it was reported within.
    idTestSet           INTEGER     NOT NULL, -- REFERENCES TestSets(idTestSet) NOT NULL,
    --- Creation time stamp.
    tsCreated           TIMESTAMP WITH TIME ZONE  DEFAULT current_timestamp  NOT NULL,
    --- The name.
    idStrName           INTEGER     NOT NULL, -- REFERENCES TestResultStrTab(idStr)  NOT NULL,
    --- The value.
    lValue              bigint      NOT NULL,
    --- The unit.
    -- @todo This is currently not defined properly. Will fix/correlate this
    --       with the other places we use unit (IPRT/testdriver/VMMDev).
    iUnit               smallint    NOT NULL --CHECK (iUnit >= 0 AND iUnit < 1024)
);
COMMIT;
\d+ NewTestResultValues

-- Note! Using left out join here to speed up things (no hashing).
SELECT COUNT(*) FROM TestResultValues a LEFT OUTER JOIN TestResults b ON (a.idTestResult = b.idTestResult);
SELECT COUNT(*) FROM TestResultValues;

INSERT INTO NewTestResultValues (idTestResultValue, idTestResult, idTestSet, tsCreated, idStrName, lValue, iUnit)
    SELECT a.idTestResultValue, a.idTestResult, b.idTestSet, a.tsCreated, a.idStrName, a.lValue, a.iUnit
    FROM   TestResultValues a LEFT OUTER JOIN TestResults b ON (a.idTestResult = b.idTestResult);
COMMIT;
SELECT COUNT(*) FROM NewTestResultValues;

-- Switch the tables.
ALTER TABLE TestResultValues RENAME TO OldTestResultValues;
ALTER TABLE NewTestResultValues RENAME TO TestResultValues;
COMMIT;

-- Index the table.
CREATE INDEX TestResultValuesIdx ON TestResultValues(idTestResult);
CREATE INDEX TestResultValuesNameIdx ON TestResultValues(idStrName, tsCreated);
COMMIT;

-- Drop the old table.
DROP TABLE OldTestResultValues;
COMMIT;

-- Add the constraints constraint.
ALTER TABLE TestResultValues ADD CONSTRAINT TestResultValues_iUnit_Check CHECK (iUnit >= 0 AND iUnit < 1024);
ALTER TABLE TestResultValues ADD PRIMARY KEY (idTestResultValue);
ALTER TABLE TestResultValues ADD FOREIGN KEY (idStrName)    REFERENCES TestResultstrtab(idStr);
ALTER TABLE TestResultValues ADD FOREIGN KEY (idTestResult) REFERENCES TestResults(idTestResult);
ALTER TABLE TestResultValues ADD FOREIGN KEY (idTestSet)    REFERENCES TestSets(idTestSet);
COMMIT;

\d+ TestResultValues;

