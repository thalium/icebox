-- $Id: globalresource.pgsql $
--- @file
-- VBox Test Manager Database Stored Procedures.
--

--
-- Copyright (C) 2006-2017 Oracle Corporation
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

-- Args: uidAuthor, sName, sDescription, fEnabled
CREATE OR REPLACE function add_globalresource(integer, text, text, bool) RETURNS integer AS $$
    DECLARE
        _idGlobalRsrc integer;
        _uidAuthor ALIAS FOR $1;
        _sName ALIAS FOR $2;
        _sDescription ALIAS FOR $3;
        _fEnabled ALIAS FOR $4;
    BEGIN
        -- Check if Global Resource name is unique
        IF EXISTS(SELECT * FROM GlobalResources
                    WHERE sName=_sName AND
                          tsExpire='infinity'::timestamp) THEN
            RAISE EXCEPTION 'Duplicate Global Resource name';
        END IF;
        INSERT INTO GlobalResources (uidAuthor, sName, sDescription, fEnabled)
            VALUES (_uidAuthor, _sName, _sDescription, _fEnabled) RETURNING idGlobalRsrc INTO _idGlobalRsrc;
        RETURN _idGlobalRsrc;
    END;
$$ LANGUAGE plpgsql;

-- Args: uidAuthor, idGlobalRsrc
CREATE OR REPLACE function del_globalresource(integer, integer) RETURNS VOID AS $$
    DECLARE
        _uidAuthor ALIAS FOR $1;
        _idGlobalRsrc ALIAS FOR $2;
    BEGIN

        -- Check if record exist
        IF NOT EXISTS(SELECT * FROM GlobalResources WHERE idGlobalRsrc=_idGlobalRsrc AND tsExpire='infinity'::timestamp) THEN
            RAISE EXCEPTION 'Global resource (%) does not exist', _idGlobalRsrc;
        END IF;

        -- Historize record: GlobalResources
        UPDATE GlobalResources
          SET   tsExpire=CURRENT_TIMESTAMP,
                uidAuthor=_uidAuthor
          WHERE idGlobalRsrc=_idGlobalRsrc AND
                tsExpire='infinity'::timestamp;


        -- Delete record: GlobalResourceStatuses
        DELETE FROM GlobalResourceStatuses WHERE idGlobalRsrc=_idGlobalRsrc;

        -- Historize record: TestCaseGlobalRsrcDeps
        UPDATE TestCaseGlobalRsrcDeps
          SET   tsExpire=CURRENT_TIMESTAMP,
                uidAuthor=_uidAuthor
          WHERE idGlobalRsrc=_idGlobalRsrc AND
                tsExpire='infinity'::timestamp;

    END;
$$ LANGUAGE plpgsql;

-- Args: uidAuthor, idGlobalRsrc, sName, sDescription, fEnabled
CREATE OR REPLACE function update_globalresource(integer, integer, text, text, bool) RETURNS VOID AS $$
    DECLARE
        _uidAuthor ALIAS FOR $1;
        _idGlobalRsrc ALIAS FOR $2;
        _sName ALIAS FOR $3;
        _sDescription ALIAS FOR $4;
        _fEnabled ALIAS FOR $5;
    BEGIN
        -- Hostorize record
        UPDATE GlobalResources
          SET   tsExpire=CURRENT_TIMESTAMP
          WHERE idGlobalRsrc=_idGlobalRsrc AND
                tsExpire='infinity'::timestamp;
        -- Check if Global Resource name is unique
        IF EXISTS(SELECT * FROM GlobalResources
                    WHERE sName=_sName AND
                          tsExpire='infinity'::timestamp) THEN
            RAISE EXCEPTION 'Duplicate Global Resource name';
        END IF;
        -- Add new record
        INSERT INTO GlobalResources(uidAuthor, idGlobalRsrc, sName, sDescription, fEnabled)
            VALUES (_uidAuthor, _idGlobalRsrc, _sName, _sDescription, _fEnabled);
    END;
$$ LANGUAGE plpgsql;
