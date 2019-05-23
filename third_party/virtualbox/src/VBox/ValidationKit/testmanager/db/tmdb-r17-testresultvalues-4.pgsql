-- $Id: tmdb-r17-testresultvalues-4.pgsql $
--- @file
-- VBox Test Manager Database - Log viewer related optimizations for TestResultValues.
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

-- Create index for the log viewer
CREATE INDEX TestResultValuesLogIdx ON TestResultValues(idTestSet, tsCreated);
COMMIT;

\d+ TestResultValues

