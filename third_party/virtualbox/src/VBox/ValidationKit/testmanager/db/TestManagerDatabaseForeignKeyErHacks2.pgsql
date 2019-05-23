-- $Id: TestManagerDatabaseForeignKeyErHacks2.pgsql $
--- @file
-- VBox Test Manager Database Addendum that adds non-unique foreign keys to Users.
--
-- This is for getting better visualization in reverse engeering ER tools,
-- it is not for production databases.
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
\connect testmanager

ALTER TABLE GlobalResources
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE BuildSources
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE RequirementSets
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsCreated) REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestCases
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestCaseArgs
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestcaseDeps
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestCaseGlobalRsrcDeps
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestGroups
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestGroupMembers
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE SchedGroups
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH SIMPLE;
ALTER TABLE SchedGroupMembers
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestBoxes
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE FailureCategories
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE FailureReasons
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE TestResultFailures
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE BuildBlacklist
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsExpire)  REFERENCES Users(uid, tsExpire) MATCH FULL;
ALTER TABLE Builds
   ADD CONSTRAINT non_unique_fk9 FOREIGN KEY (uidAuthor, tsCreated) REFERENCES Users(uid, tsExpire) MATCH FULL;

