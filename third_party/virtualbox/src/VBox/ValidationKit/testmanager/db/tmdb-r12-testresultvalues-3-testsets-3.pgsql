-- $Id: tmdb-r12-testresultvalues-3-testsets-3.pgsql $
--- @file
-- VBox Test Manager Database - Graph related optimizations for TestResultValues and TestSets.
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

\d+ TestResultValues

-- Rename index to better show it's purpose.
ALTER INDEX TestResultValuesNameIdx RENAME TO TestResultValuesGraphIdx;
COMMIT;

-- Combine the tsCreated and tsDone indexes.
DROP INDEX TestSetsCreated;
DROP INDEX TestSetsDone;
CREATE INDEX TestSetsCreatedDoneIdx ON TestSets (tsCreated, tsDone);
COMMIT;

-- Create index for graph.
CREATE INDEX TestSetsGraphBoxIdx ON TestSets (idTestBox, tsCreated, tsDone, idBuildCategory, idTestCase);
COMMIT;

\d+ TestResultValues

