# -*- coding: utf-8 -*-
# $Id: schedgroup.py $

"""
Test Manager - Scheduling Group.
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
import unittest;

# Validation Kit imports.
from testmanager.core.base          import ModelDataBase, ModelDataBaseTestCase, ModelLogicBase, TMExceptionBase, \
                                           TMRowInUse, TMInvalidData, TMRowAlreadyExists, TMRowNotFound;
from testmanager.core.buildsource   import BuildSourceData;
from testmanager.core.testcase      import TestCaseData;
from testmanager.core.testcaseargs  import TestCaseArgsData;
from testmanager.core.testbox       import TestBoxData, TestBoxLogic;
from testmanager.core.testgroup     import TestGroupData;



class SchedGroupMemberData(ModelDataBase):
    """
    SchedGroupMember Data.
    """

    ksIdAttr = 'idSchedGroup';

    ksParam_idSchedGroup        = 'SchedGroupMember_idSchedGroup';
    ksParam_idTestGroup         = 'SchedGroupMember_idTestGroup';
    ksParam_tsEffective         = 'SchedGroupMember_tsEffective';
    ksParam_tsExpire            = 'SchedGroupMember_tsExpire';
    ksParam_uidAuthor           = 'SchedGroupMember_uidAuthor';
    ksParam_iSchedPriority      = 'SchedGroupMember_iSchedPriority';
    ksParam_bmHourlySchedule    = 'SchedGroupMember_bmHourlySchedule';
    ksParam_idTestGroupPreReq   = 'SchedGroupMember_idTestGroupPreReq';

    kasAllowNullAttributes      = [ 'idSchedGroup', 'idTestGroup', 'tsEffective', 'tsExpire',
                                    'uidAuthor', 'bmHourlySchedule', 'idTestGroupPreReq' ];
    kiMin_iSchedPriority        = 0;
    kiMax_iSchedPriority        = 32;

    kcDbColumns                 = 8

    def __init__(self):
        ModelDataBase.__init__(self);

        #
        # Initialize with defaults.
        # See the database for explanations of each of these fields.
        #
        self.idSchedGroup        = None;
        self.idTestGroup         = None;
        self.tsEffective         = None;
        self.tsExpire            = None;
        self.uidAuthor           = None;
        self.iSchedPriority      = 16;
        self.bmHourlySchedule    = None;
        self.idTestGroupPreReq   = None;

    def initFromDbRow(self, aoRow):
        """
        Re-initializes the data with a row from a SELECT * FROM SchedGroupMembers.

        Returns self. Raises exception if the row is None or otherwise invalid.
        """

        if aoRow is None:
            raise TMRowNotFound('SchedGroupMember not found.');

        self.idSchedGroup        = aoRow[0];
        self.idTestGroup         = aoRow[1];
        self.tsEffective         = aoRow[2];
        self.tsExpire            = aoRow[3];
        self.uidAuthor           = aoRow[4];
        self.iSchedPriority      = aoRow[5];
        self.bmHourlySchedule    = aoRow[6]; ## @todo figure out how bitmaps are returned...
        self.idTestGroupPreReq   = aoRow[7];
        return self;


class SchedGroupMemberDataEx(SchedGroupMemberData):
    """
    Extended SchedGroupMember data class.
    This adds the testgroups.
    """

    def __init__(self):
        SchedGroupMemberData.__init__(self);
        self.oTestGroup = None;

    def initFromDbRow(self, aoRow):
        """
        Re-initializes the data with a row from a query like this:

            SELECT  SchedGroupMembers.*, TestGroups.*
            FROM    SchedGroupMembers
            JOIN    TestGroups
                ON  (SchedGroupMembers.idTestGroup = TestGroups.idTestGroup);

        Returns self. Raises exception if the row is None or otherwise invalid.
        """
        SchedGroupMemberData.initFromDbRow(self, aoRow);
        self.oTestGroup = TestGroupData().initFromDbRow(aoRow[SchedGroupMemberData.kcDbColumns:]);
        return self;

    def getDataAttributes(self):
        asAttributes = SchedGroupMemberData.getDataAttributes(self);
        asAttributes.remove('oTestGroup');
        return asAttributes;

    def _validateAndConvertWorker(self, asAllowNullAttributes, oDb, enmValidateFor = ModelDataBase.ksValidateFor_Other):
        dErrors = SchedGroupMemberData._validateAndConvertWorker(self, asAllowNullAttributes, oDb, enmValidateFor);
        if self.ksParam_idTestGroup not in dErrors:
            self.oTestGroup = TestGroupData();
            try:
                self.oTestGroup.initFromDbWithId(oDb, self.idTestGroup);
            except Exception as oXcpt:
                self.oTestGroup = TestGroupData()
                dErrors[self.ksParam_idTestGroup] = str(oXcpt);
        return dErrors;



class SchedGroupData(ModelDataBase):
    """
    SchedGroup Data.
    """

    ## @name TestBoxState_T
    # @{
    ksScheduler_BestEffortContinuousIntegration = 'bestEffortContinousItegration'; # sic*2
    ksScheduler_Reserved                        = 'reserved';
    ## @}


    ksIdAttr = 'idSchedGroup';

    ksParam_idSchedGroup        = 'SchedGroup_idSchedGroup';
    ksParam_tsEffective         = 'SchedGroup_tsEffective';
    ksParam_tsExpire            = 'SchedGroup_tsExpire';
    ksParam_uidAuthor           = 'SchedGroup_uidAuthor';
    ksParam_sName               = 'SchedGroup_sName';
    ksParam_sDescription        = 'SchedGroup_sDescription';
    ksParam_fEnabled            = 'SchedGroup_fEnabled';
    ksParam_enmScheduler        = 'SchedGroup_enmScheduler';
    ksParam_idBuildSrc          = 'SchedGroup_idBuildSrc';
    ksParam_idBuildSrcTestSuite = 'SchedGroup_idBuildSrcTestSuite';
    ksParam_sComment            = 'SchedGroup_sComment';

    kasAllowNullAttributes      = ['idSchedGroup', 'tsEffective', 'tsExpire', 'uidAuthor', 'sDescription',
                                   'idBuildSrc', 'idBuildSrcTestSuite', 'sComment' ];
    kasValidValues_enmScheduler = [ ksScheduler_BestEffortContinuousIntegration, ];

    kcDbColumns                 = 11;

    # Scheduler types
    kasSchedulerDesc            = \
    [
        ( ksScheduler_BestEffortContinuousIntegration,  'Best-Effort-Continuous-Integration (BECI) scheduler.', ''),
    ]

    def __init__(self):
        ModelDataBase.__init__(self);

        #
        # Initialize with defaults.
        # See the database for explanations of each of these fields.
        #
        self.idSchedGroup            = None;
        self.tsEffective             = None;
        self.tsExpire                = None;
        self.uidAuthor               = None;
        self.sName                   = None;
        self.sDescription            = None;
        self.fEnabled                = None;
        self.enmScheduler            = SchedGroupData.ksScheduler_BestEffortContinuousIntegration;
        self.idBuildSrc              = None;
        self.idBuildSrcTestSuite     = None;
        self.sComment                = None;

    def initFromDbRow(self, aoRow):
        """
        Re-initializes the data with a row from a SELECT * FROM SchedGroups.

        Returns self. Raises exception if the row is None or otherwise invalid.
        """

        if aoRow is None:
            raise TMRowNotFound('SchedGroup not found.');

        self.idSchedGroup            = aoRow[0];
        self.tsEffective             = aoRow[1];
        self.tsExpire                = aoRow[2];
        self.uidAuthor               = aoRow[3];
        self.sName                   = aoRow[4];
        self.sDescription            = aoRow[5];
        self.fEnabled                = aoRow[6];
        self.enmScheduler            = aoRow[7];
        self.idBuildSrc              = aoRow[8];
        self.idBuildSrcTestSuite     = aoRow[9];
        self.sComment                = aoRow[10];
        return self;

    def initFromDbWithId(self, oDb, idSchedGroup, tsNow = None, sPeriodBack = None):
        """
        Initialize the object from the database.
        """
        oDb.execute(self.formatSimpleNowAndPeriodQuery(oDb,
                                                       'SELECT *\n'
                                                       'FROM   SchedGroups\n'
                                                       'WHERE  idSchedGroup = %s\n'
                                                       , ( idSchedGroup,), tsNow, sPeriodBack));
        aoRow = oDb.fetchOne()
        if aoRow is None:
            raise TMRowNotFound('idSchedGroup=%s not found (tsNow=%s, sPeriodBack=%s)' % (idSchedGroup, tsNow, sPeriodBack));
        return self.initFromDbRow(aoRow);


class SchedGroupDataEx(SchedGroupData):
    """
    Extended scheduling group data.

    Note! Similar to TestGroupDataEx.
    """

    ksParam_aoMembers    = 'SchedGroup_aoMembers';
    kasAltArrayNull      = [ 'aoMembers', ];

    ## Helper parameter containing the comma separated list with the IDs of
    #  potential members found in the parameters.
    ksParam_aidTestGroups = 'TestGroupDataEx_aidTestGroups';


    def __init__(self):
        SchedGroupData.__init__(self);
        self.aoMembers          = [];       # type: SchedGroupMemberDataEx

        # Two build sources for convenience sake.
        self.oBuildSrc          = None;     # type: TestBoxData
        self.oBuildSrcValidationKit = None; # type: TestBoxData
        # List of test boxes that uses this group for convenience.
        self.aoTestBoxes        = None;     # type: list[TestBoxData]

    def _initExtraMembersFromDb(self, oDb, tsNow = None, sPeriodBack = None):
        """
        Worker shared by the initFromDb* methods.
        Returns self.  Raises exception if no row or database error.
        """
        #
        # It all upfront so the object has some kind of consistency if anything
        # below raises exceptions.
        #
        self.oBuildSrc    = None;
        self.oBuildSrcValidationKit = None;
        self.aoTestBoxes = [];
        self.aoMembers   = [];

        #
        # Build source.
        #
        if self.idBuildSrc:
            self.oBuildSrc = BuildSourceData().initFromDbWithId(oDb, self.idBuildSrc, tsNow, sPeriodBack);

        if self.idBuildSrcTestSuite:
            self.oBuildSrcValidationKit = BuildSourceData().initFromDbWithId(oDb, self.idBuildSrcTestSuite,
                                                                             tsNow, sPeriodBack);

        #
        # Test Boxes.
        #
        oDb.execute('SELECT TestBoxesWithStrings.*\n'
                    'FROM   TestBoxesWithStrings,\n'
                    '       TestBoxesInSchedGroups\n'
                    'WHERE  TestBoxesInSchedGroups.idSchedGroup = %s\n'
                    + self.formatSimpleNowAndPeriod(oDb, tsNow, sPeriodBack, sTablePrefix = 'TestBoxesInSchedGroups.') +
                    '   AND TestBoxesWithStrings.idTestBox      = TestBoxesInSchedGroups.idTestBox\n'
                    + self.formatSimpleNowAndPeriod(oDb, tsNow, sPeriodBack, sTablePrefix = 'TestBoxesWithStrings.') +
                    'ORDER BY TestBoxesWithStrings.sName, TestBoxesWithStrings.idTestBox\n'
                    , (self.idSchedGroup,));
        for aoRow in oDb.fetchAll():
            self.aoTestBoxes.append(TestBoxData().initFromDbRow(aoRow));

        #
        # Test groups.
        #
        oDb.execute('SELECT SchedGroupMembers.*, TestGroups.*\n'
                    'FROM   SchedGroupMembers\n'
                    'LEFT OUTER JOIN TestGroups ON (SchedGroupMembers.idTestGroup = TestGroups.idTestGroup)\n'
                    'WHERE  SchedGroupMembers.idSchedGroup = %s\n'
                    + self.formatSimpleNowAndPeriod(oDb, tsNow, sPeriodBack, sTablePrefix = 'SchedGroupMembers.')
                    + self.formatSimpleNowAndPeriod(oDb, tsNow, sPeriodBack, sTablePrefix = 'TestGroups.') +
                    'ORDER BY SchedGroupMembers.idTestGroupPreReq, SchedGroupMembers.idTestGroup\n'
                    , (self.idSchedGroup,));
        for aoRow in oDb.fetchAll():
            self.aoMembers.append(SchedGroupMemberDataEx().initFromDbRow(aoRow));
        return self;

    def initFromDbRowEx(self, aoRow, oDb, tsNow = None):
        """
        Reinitialize from a SELECT * FROM SchedGroups row.  Will query the
        necessary additional data from oDb using tsNow.
        Returns self.  Raises exception if no row or database error.
        """
        SchedGroupData.initFromDbRow(self, aoRow);
        return self._initExtraMembersFromDb(oDb, tsNow);

    def initFromDbWithId(self, oDb, idSchedGroup, tsNow = None, sPeriodBack = None):
        """
        Initialize the object from the database.
        """
        SchedGroupData.initFromDbWithId(self, oDb, idSchedGroup, tsNow, sPeriodBack);
        return self._initExtraMembersFromDb(oDb, tsNow, sPeriodBack);

    def getDataAttributes(self):
        asAttributes = SchedGroupData.getDataAttributes(self);
        asAttributes.remove('oBuildSrc');
        asAttributes.remove('oBuildSrcValidationKit');
        asAttributes.remove('aoTestBoxes');
        return asAttributes;

    def getAttributeParamNullValues(self, sAttr):
        if sAttr != 'aoMembers':
            return SchedGroupData.getAttributeParamNullValues(self, sAttr);
        return ['', [], None];

    def convertParamToAttribute(self, sAttr, sParam, oValue, oDisp, fStrict):
        if sAttr != 'aoMembers':
            return SchedGroupData.convertParamToAttribute(self, sAttr, sParam, oValue, oDisp, fStrict);

        aoNewValue  = [];
        aidSelected = oDisp.getListOfIntParams(sParam, iMin = 1, iMax = 0x7ffffffe, aiDefaults = [])
        sIds        = oDisp.getStringParam(self.ksParam_aidTestGroups, sDefault = '');
        for idTestGroup in sIds.split(','):
            try:    idTestGroup = int(idTestGroup);
            except: pass;
            oDispWrapper = self.DispWrapper(oDisp, '%s[%s][%%s]' % (SchedGroupDataEx.ksParam_aoMembers, idTestGroup,))
            oMember = SchedGroupMemberDataEx().initFromParams(oDispWrapper, fStrict = False);
            if idTestGroup in aidSelected:
                aoNewValue.append(oMember);
        return aoNewValue;

    def _validateAndConvertAttribute(self, sAttr, sParam, oValue, aoNilValues, fAllowNull, oDb):
        if sAttr != 'aoMembers':
            return SchedGroupData._validateAndConvertAttribute(self, sAttr, sParam, oValue, aoNilValues, fAllowNull, oDb);

        asErrors     = [];
        aoNewMembers = [];
        for oOldMember in oValue:
            oNewMember = SchedGroupMemberDataEx().initFromOther(oOldMember);
            aoNewMembers.append(oNewMember);

            dErrors = oNewMember.validateAndConvert(oDb, ModelDataBase.ksValidateFor_Other);
            if dErrors:
                asErrors.append(str(dErrors));

        if not asErrors:
            for i, _ in enumerate(aoNewMembers):
                idTestGroup = aoNewMembers[i];
                for j in range(i + 1, len(aoNewMembers)):
                    if aoNewMembers[j].idTestGroup == idTestGroup:
                        asErrors.append('Duplicate test group #%d!' % (idTestGroup, ));
                        break;

        return (aoNewMembers, None if not asErrors else '<br>\n'.join(asErrors));

    def _validateAndConvertWorker(self, asAllowNullAttributes, oDb, enmValidateFor = ModelDataBase.ksValidateFor_Other):
        dErrors = SchedGroupData._validateAndConvertWorker(self, asAllowNullAttributes, oDb, enmValidateFor);

        #
        # Fetch the extended build source bits.
        #
        if self.ksParam_idBuildSrc not in dErrors:
            if   self.idBuildSrc in self.getAttributeParamNullValues('idBuildSrc') \
              or self.idBuildSrc is None:
                self.oBuildSrc = None;
            else:
                try:
                    self.oBuildSrc = BuildSourceData().initFromDbWithId(oDb, self.idBuildSrc);
                except Exception as oXcpt:
                    self.oBuildSrc = BuildSourceData();
                    dErrors[self.ksParam_idBuildSrc] = str(oXcpt);

        if self.ksParam_idBuildSrcTestSuite not in dErrors:
            if   self.idBuildSrcTestSuite in self.getAttributeParamNullValues('idBuildSrcTestSuite') \
              or self.idBuildSrcTestSuite is None:
                self.oBuildSrcValidationKit = None;
            else:
                try:
                    self.oBuildSrcValidationKit = BuildSourceData().initFromDbWithId(oDb, self.idBuildSrcTestSuite);
                except Exception as oXcpt:
                    self.oBuildSrcValidationKit = BuildSourceData();
                    dErrors[self.ksParam_idBuildSrcTestSuite] = str(oXcpt);

        return dErrors;



class SchedGroupLogic(ModelLogicBase): # pylint: disable=R0903
    """
    SchedGroup logic.
    """

    def __init__(self, oDb):
        ModelLogicBase.__init__(self, oDb);
        self.dCache = None;

    #
    # Standard methods.
    #

    def fetchForListing(self, iStart, cMaxRows, tsNow, aiSortColumns = None):
        """
        Fetches build sources.

        Returns an array (list) of BuildSourceData items, empty list if none.
        Raises exception on error.
        """
        _ = aiSortColumns;

        if tsNow is None:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire = \'infinity\'::TIMESTAMP\n'
                              'ORDER BY idSchedGroup DESC\n'
                              'LIMIT %s OFFSET %s\n'
                              , (cMaxRows, iStart,));
        else:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire     > %s\n'
                              '     AND tsEffective <= %s\n'
                              'ORDER BY idSchedGroup DESC\n'
                              'LIMIT %s OFFSET %s\n'
                              , (tsNow, tsNow, cMaxRows, iStart,));

        aoRet = [];
        for aoRow in self._oDb.fetchAll():
            aoRet.append(SchedGroupDataEx().initFromDbRowEx(aoRow, self._oDb, tsNow));
        return aoRet;

    def addEntry(self, oData, uidAuthor, fCommit = False):
        """Add Scheduling Group record"""

        #
        # Validate.
        #
        dDataErrors = oData.validateAndConvert(self._oDb, oData.ksValidateFor_Add);
        if dDataErrors:
            raise TMInvalidData('Invalid data passed to addEntry: %s' % (dDataErrors,));
        if self.exists(oData.sName):
            raise TMRowAlreadyExists('Scheduling group "%s" already exists.' % (oData.sName,));

        #
        # Add it.
        #
        self._oDb.execute('INSERT INTO SchedGroups (\n'
                          '         uidAuthor,\n'
                          '         sName,\n'
                          '         sDescription,\n'
                          '         fEnabled,\n'
                          '         enmScheduler,\n'
                          '         idBuildSrc,\n'
                          '         idBuildSrcTestSuite,\n'
                          '         sComment)\n'
                          'VALUES (%s, %s, %s, %s, %s, %s, %s, %s)\n'
                          'RETURNING idSchedGroup\n'
                          , ( uidAuthor,
                              oData.sName,
                              oData.sDescription,
                              oData.fEnabled,
                              oData.enmScheduler,
                              oData.idBuildSrc,
                              oData.idBuildSrcTestSuite,
                              oData.sComment ));
        idSchedGroup = self._oDb.fetchOne()[0];
        oData.idSchedGroup = idSchedGroup;

        for oMember in oData.aoMembers:
            oMember.idSchedGroup = idSchedGroup;
            self._addSchedGroupMember(uidAuthor, oMember);

        self._oDb.maybeCommit(fCommit);
        return True;

    def editEntry(self, oData, uidAuthor, fCommit = False):
        """Edit Scheduling Group record"""

        #
        # Validate input and retrieve the old data.
        #
        dErrors = oData.validateAndConvert(self._oDb, oData.ksValidateFor_Edit);
        if dErrors:
            raise TMInvalidData('editEntry got invalid data: %s' % (dErrors,));
        self._assertUnique(oData.sName, oData.idSchedGroup);
        oOldData = SchedGroupDataEx().initFromDbWithId(self._oDb, oData.idSchedGroup);

        #
        # Make the changes.
        #
        if not oData.isEqualEx(oOldData, [ 'tsEffective', 'tsExpire', 'uidAuthor', 'aoMembers', 'aoTestBoxes',
                                           'oBuildSrc', 'oBuildSrcValidationKit', ]):
            self._historizeEntry(oData.idSchedGroup);
            self._readdEntry(uidAuthor, oData);

        # Remove groups.
        for oOld in oOldData.aoMembers:
            fRemove = True;
            for oNew in oData.aoMembers:
                if oNew.idTestGroup == oOld.idTestGroup:
                    fRemove = False;
                    break;
            if fRemove:
                self._removeSchedGroupMember(uidAuthor, oOld);

        # Add / modify groups.
        for oMember in oData.aoMembers:
            oOldMember = None;
            for oOld in oOldData.aoMembers:
                if oOld.idTestGroup == oMember.idTestGroup:
                    oOldMember = oOld;
                    break;

            oMember.idSchedGroup = oData.idSchedGroup;
            if oOldMember is None:
                self._addSchedGroupMember(uidAuthor, oMember);
            elif not oMember.isEqualEx(oOldMember, ['tsEffective', 'tsExpire', 'uidAuthor', 'oTestGroup']):
                self._historizeSchedGroupMember(oMember);
                self._addSchedGroupMember(uidAuthor, oMember);

        self._oDb.maybeCommit(fCommit);
        return True;

    def removeEntry(self, uidAuthor, idSchedGroup, fCascade = False, fCommit = False):
        """
        Deletes a scheduling group.
        """

        #
        # Input validation and retrival of current data.
        #
        if idSchedGroup == 1:
            raise TMRowInUse('Cannot remove the default scheduling group (id 1).');
        oData = SchedGroupDataEx().initFromDbWithId(self._oDb, idSchedGroup);

        #
        # We use cascade a little different here... We don't actually delete
        # associated testboxes or testgroups.
        #
        if oData.aoTestBoxes:
            if fCascade is not True:
                # Complain about there being associated testboxes.
                asTestBoxes = ['%s (#%d)' % (oTestBox.sName, oTestBox.idTestBox) for oTestBox in oData.aoTestBoxes];
                raise TMRowInUse('Scheduling group #%d is associated with one or more test boxes: %s'
                                 % (idSchedGroup, ', '.join(asTestBoxes),));
            else:
                # Reassign testboxes to scheduling group #1 (the default group).
                oTbLogic = TestBoxLogic(self._oDb);
                for oTestBox in oData.aoTestBoxes:
                    oTbCopy = TestBoxData().initFromOther(oTestBox);
                    oTbCopy.idSchedGroup = 1;
                    oTbLogic.editEntry(oTbCopy, uidAuthor, fCommit = False);

                oData = SchedGroupDataEx().initFromDbWithId(self._oDb, idSchedGroup);
                if oData.aoTestBoxes:
                    raise TMRowInUse('More testboxes was added to the scheduling group as we were trying to delete it.');

        #
        # Remove the group and all member records.
        #
        for oMember in oData.aoMembers:
            self._removeSchedGroupMember(uidAuthor, oMember);
        self._oDb.execute('UPDATE   SchedGroupMembers\n'
                          'SET      tsExpire     = CURRENT_TIMESTAMP\n'
                          'WHERE    idSchedGroup = %s\n'
                          '     AND tsExpire     = \'infinity\'::TIMESTAMP\n'
                          , (idSchedGroup,));

        (tsCur, tsCurMinusOne) = self._oDb.getCurrentTimestamps();
        if oData.tsEffective != tsCur and oData.tsEffective != tsCurMinusOne:
            self._historizeEntry(idSchedGroup, tsCurMinusOne);
            self._readdEntry(uidAuthor, oData, tsCurMinusOne);
            self._historizeEntry(idSchedGroup);
        self._oDb.execute('UPDATE   SchedGroups\n'
                          'SET      tsExpire     = CURRENT_TIMESTAMP\n'
                          'WHERE    idSchedGroup = %s\n'
                          '     AND tsExpire     = \'infinity\'::TIMESTAMP\n'
                          , (idSchedGroup,))

        self._oDb.maybeCommit(fCommit)
        return True;


    def cachedLookup(self, idSchedGroup):
        """
        Looks up the most recent SchedGroupData object for idSchedGroup
        via an object cache.

        Returns a shared SchedGroupData object.  None if not found.
        Raises exception on DB error.
        """
        if self.dCache is None:
            self.dCache = self._oDb.getCache('SchedGroup');

        oEntry = self.dCache.get(idSchedGroup, None);
        if oEntry is None:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    idSchedGroup = %s\n'
                              '     AND tsExpire = \'infinity\'::TIMESTAMP\n'
                              , (idSchedGroup, ));
            if self._oDb.getRowCount() == 0:
                # Maybe it was deleted, try get the last entry.
                self._oDb.execute('SELECT   *\n'
                                  'FROM     SchedGroups\n'
                                  'WHERE    idSchedGroup = %s\n'
                                  'ORDER BY tsExpire DESC\n'
                                  'LIMIT 1\n'
                                  , (idSchedGroup, ));
            elif self._oDb.getRowCount() > 1:
                raise self._oDb.integrityException('%s infinity rows for %s' % (self._oDb.getRowCount(), idSchedGroup));

            if self._oDb.getRowCount() == 1:
                oEntry = SchedGroupData().initFromDbRow(self._oDb.fetchOne());
                self.dCache[idSchedGroup] = oEntry;
        return oEntry;


    #
    # Other methods.
    #

    def fetchOrderedByName(self, tsNow = None):
        """
        Return list of objects of type SchedGroups ordered by name.
        May raise exception on database error.
        """
        if tsNow is None:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire = \'infinity\'::TIMESTAMP\n'
                              'ORDER BY sName ASC\n');
        else:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire     > %s\n'
                              '   AND   tsEffective <= %s\n'
                              'ORDER BY sName ASC\n'
                              , (tsNow, tsNow,));
        aoRet = []
        for _ in range(self._oDb.getRowCount()):
            aoRet.append(SchedGroupData().initFromDbRow(self._oDb.fetchOne()));
        return aoRet;


    def getAll(self, tsEffective = None):
        """
        Gets the list of all scheduling groups.
        Returns an array of SchedGroupData instances.
        """
        if tsEffective is None:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire = \'infinity\'::TIMESTAMP\n');
        else:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire     > %s\n'
                              '     AND tsEffective <= %s\n'
                              , (tsEffective, tsEffective));
        aoRet = [];
        for aoRow in self._oDb.fetchAll():
            aoRet.append(SchedGroupData().initFromDbRow(aoRow));
        return aoRet;

    def getSchedGroupsForCombo(self, tsEffective = None):
        """
        Gets the list of active scheduling groups for a combo box.
        Returns an array of (value [idSchedGroup], drop-down-name [sName],
        hover-text [sDescription]) tuples.
        """
        if tsEffective is None:
            self._oDb.execute('SELECT   idSchedGroup, sName, sDescription\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire = \'infinity\'::TIMESTAMP\n'
                              'ORDER BY sName');
        else:
            self._oDb.execute('SELECT   idSchedGroup, sName, sDescription\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire     > %s\n'
                              '     AND tsEffective <= %s\n'
                              'ORDER BY sName'
                              , (tsEffective, tsEffective));
        return self._oDb.fetchAll();


    def getMembers(self, idSchedGroup, tsEffective = None):
        """
        Gets the scheduling groups members for the given scheduling group.

        Returns an array of SchedGroupMemberDataEx instances (sorted by
        priority (descending) and idTestGroup).  May raise exception DB error.
        """

        if tsEffective is None:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroupMembers, TestGroups\n'
                              'WHERE    SchedGroupMembers.idSchedGroup = %s\n'
                              '     AND SchedGroupMembers.tsExpire     = \'infinity\'::TIMESTAMP\n'
                              '     AND TestGroups.idTestGroup         = SchedGroupMembers.idTestGroup\n'
                              '     AND TestGroups.tsExpire            = \'infinity\'::TIMESTAMP\n'
                              'ORDER BY SchedGroupMembers.iSchedPriority DESC, SchedGroupMembers.idTestGroup\n'
                              , (idSchedGroup,));
        else:
            self._oDb.execute('SELECT   *\n'
                              'FROM     SchedGroupMembers, TestGroups\n'
                              'WHERE    SchedGroupMembers.idSchedGroup = %s\n'
                              '     AND SchedGroupMembers.tsExpire     < %s\n'
                              '     AND SchedGroupMembers.tsEffective >= %s\n'
                              '     AND TestGroups.idTestGroup         = SchedGroupMembers.idTestGroup\n'
                              '     AND TestGroups.tsExpire            < %s\n'
                              '     AND TestGroups.tsEffective        >= %s\n'
                              'ORDER BY SchedGroupMembers.iSchedPriority DESC, SchedGroupMembers.idTestGroup\n'
                              , (idSchedGroup, tsEffective, tsEffective, tsEffective, tsEffective, ));
        aaoRows = self._oDb.fetchAll();
        aoRet = [];
        for aoRow in aaoRows:
            aoRet.append(SchedGroupMemberDataEx().initFromDbRow(aoRow));
        return aoRet;

    def getTestCasesForGroup(self, idSchedGroup, cMax = None):
        """
        Gets the enabled testcases w/ testgroup+priority for the given scheduling group.

        Returns an array of TestCaseData instances (ordered by group id, descending
        testcase priority, and testcase IDs) with an extra iSchedPriority member.
        May raise exception on DB error or if the result exceeds cMax.
        """

        self._oDb.execute('SELECT   TestGroupMembers.idTestGroup, TestGroupMembers.iSchedPriority, TestCases.*\n'
                          'FROM     SchedGroupMembers, TestGroups, TestGroupMembers, TestCases\n'
                          'WHERE    SchedGroupMembers.idSchedGroup = %s\n'
                          '     AND SchedGroupMembers.tsExpire     = \'infinity\'::TIMESTAMP\n'
                          '     AND TestGroups.idTestGroup         = SchedGroupMembers.idTestGroup\n'
                          '     AND TestGroups.tsExpire            = \'infinity\'::TIMESTAMP\n'
                          '     AND TestGroupMembers.idTestGroup   = TestGroups.idTestGroup\n'
                          '     AND TestGroupMembers.tsExpire      = \'infinity\'::TIMESTAMP\n'
                          '     AND TestCases.idTestCase           = TestGroupMembers.idTestCase\n'
                          '     AND TestCases.tsExpire             = \'infinity\'::TIMESTAMP\n'
                          '     AND TestCases.fEnabled             = TRUE\n'
                          'ORDER BY TestGroupMembers.idTestGroup, TestGroupMembers.iSchedPriority DESC, TestCases.idTestCase\n'
                          , (idSchedGroup,));

        if cMax is not None  and  self._oDb.getRowCount() > cMax:
            raise TMExceptionBase('Too many testcases for scheduling group %s: %s, max %s'
                                  % (idSchedGroup, cMax, self._oDb.getRowCount(),));

        aoRet = [];
        for aoRow in self._oDb.fetchAll():
            oTestCase = TestCaseData().initFromDbRow(aoRow[2:]);
            oTestCase.idTestGroup    = aoRow[0];
            oTestCase.iSchedPriority = aoRow[1];
            aoRet.append(oTestCase);
        return aoRet;

    def getTestCaseArgsForGroup(self, idSchedGroup, cMax = None):
        """
        Gets the testcase argument variation w/ testgroup+priority for the given scheduling group.

        Returns an array TestCaseArgsData instance (sorted by group and
        variation id) with an extra iSchedPriority member.
        May raise exception on DB error or if the result exceeds cMax.
        """

        self._oDb.execute('SELECT   TestGroupMembers.idTestGroup, TestGroupMembers.iSchedPriority, TestCaseArgs.*\n'
                          'FROM     SchedGroupMembers, TestGroups, TestGroupMembers, TestCaseArgs, TestCases\n'
                          'WHERE    SchedGroupMembers.idSchedGroup = %s\n'
                          '     AND SchedGroupMembers.tsExpire     = \'infinity\'::TIMESTAMP\n'
                          '     AND TestGroups.idTestGroup         = SchedGroupMembers.idTestGroup\n'
                          '     AND TestGroups.tsExpire            = \'infinity\'::TIMESTAMP\n'
                          '     AND TestGroupMembers.idTestGroup   = TestGroups.idTestGroup\n'
                          '     AND TestGroupMembers.tsExpire      = \'infinity\'::TIMESTAMP\n'
                          '     AND TestCaseArgs.idTestCase        = TestGroupMembers.idTestCase\n'
                          '     AND TestCaseArgs.tsExpire          = \'infinity\'::TIMESTAMP\n'
                          '     AND (   TestGroupMembers.aidTestCaseArgs is NULL\n'
                          '          OR TestCaseArgs.idTestCaseArgs = ANY(TestGroupMembers.aidTestCaseArgs) )\n'
                          '     AND TestCases.idTestCase           = TestCaseArgs.idTestCase\n'
                          '     AND TestCases.tsExpire             = \'infinity\'::TIMESTAMP\n'
                          '     AND TestCases.fEnabled             = TRUE\n'
                          'ORDER BY TestGroupMembers.idTestGroup, TestGroupMembers.idTestCase, TestCaseArgs.idTestCaseArgs\n'
                          , (idSchedGroup,));

        if cMax is not None  and  self._oDb.getRowCount() > cMax:
            raise TMExceptionBase('Too many argument variations for scheduling group %s: %s, max %s'
                                  % (idSchedGroup, cMax, self._oDb.getRowCount(),));

        aoRet = [];
        for aoRow in self._oDb.fetchAll():
            oVariation = TestCaseArgsData().initFromDbRow(aoRow[2:]);
            oVariation.idTestGroup    = aoRow[0];
            oVariation.iSchedPriority = aoRow[1];
            aoRet.append(oVariation);
        return aoRet;

    def exists(self, sName):
        """Checks if a group with the given name exists."""
        self._oDb.execute('SELECT idSchedGroup\n'
                          'FROM   SchedGroups\n'
                          'WHERE  tsExpire   = \'infinity\'::TIMESTAMP\n'
                          '   AND sName      = %s\n'
                          'LIMIT 1\n'
                          , (sName,));
        return self._oDb.getRowCount() > 0;

    def getById(self, idSchedGroup):
        """Get Scheduling Group data by idSchedGroup"""
        self._oDb.execute('SELECT   *\n'
                          'FROM     SchedGroups\n'
                          'WHERE    tsExpire     = \'infinity\'::timestamp\n'
                          '  AND    idSchedGroup = %s;', (idSchedGroup,))
        aRows = self._oDb.fetchAll()
        if len(aRows) not in (0, 1):
            raise self._oDb.integrityException(
                'Found more than one scheduling groups with the same credentials. Database structure is corrupted.')
        try:
            return SchedGroupData().initFromDbRow(aRows[0])
        except IndexError:
            return None


    #
    # Internal helpers.
    #

    def _assertUnique(self, sName, idSchedGroupIgnore = None):
        """
        Checks that the scheduling group name is unique.
        Raises exception if the name is already in use.
        """
        if idSchedGroupIgnore is None:
            self._oDb.execute('SELECT   idSchedGroup\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire = \'infinity\'::TIMESTAMP\n'
                              '   AND   sName    = %s\n'
                              , ( sName, ) );
        else:
            self._oDb.execute('SELECT   idSchedGroup\n'
                              'FROM     SchedGroups\n'
                              'WHERE    tsExpire      = \'infinity\'::TIMESTAMP\n'
                              '   AND   sName         = %s\n'
                              '   AND   idSchedGroup <> %s\n'
                              , ( sName, idSchedGroupIgnore, ) );
        if self._oDb.getRowCount() > 0:
            raise TMRowInUse('Scheduling group name (%s) is already in use.' % (sName,));
        return True;

    def _readdEntry(self, uidAuthor, oData, tsEffective = None):
        """
        Re-adds the SchedGroups entry. Used by editEntry and removeEntry.
        """
        if tsEffective is None:
            tsEffective = self._oDb.getCurrentTimestamp();
        self._oDb.execute('INSERT INTO SchedGroups (\n'
                          '         uidAuthor,\n'
                          '         tsEffective,\n'
                          '         idSchedGroup,\n'
                          '         sName,\n'
                          '         sDescription,\n'
                          '         fEnabled,\n'
                          '         enmScheduler,\n'
                          '         idBuildSrc,\n'
                          '         idBuildSrcTestSuite,\n'
                          '         sComment )\n'
                          'VALUES ( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s )\n'
                          , ( uidAuthor,
                              tsEffective,
                              oData.idSchedGroup,
                              oData.sName,
                              oData.sDescription,
                              oData.fEnabled,
                              oData.enmScheduler,
                              oData.idBuildSrc,
                              oData.idBuildSrcTestSuite,
                              oData.sComment, ));
        return True;

    def _historizeEntry(self, idSchedGroup, tsExpire = None):
        """
        Historizes the current entry for the given scheduling group.
        """
        if tsExpire is None:
            tsExpire = self._oDb.getCurrentTimestamp();
        self._oDb.execute('UPDATE SchedGroups\n'
                          'SET    tsExpire = %s\n'
                          'WHERE  idSchedGroup = %s\n'
                          '   AND tsExpire     = \'infinity\'::TIMESTAMP\n'
                          , ( tsExpire, idSchedGroup, ));
        return True;

    def _addSchedGroupMember(self, uidAuthor, oMember, tsEffective = None):
        """
        addEntry worker for adding a scheduling group member.
        """
        if tsEffective is None:
            tsEffective = self._oDb.getCurrentTimestamp();
        self._oDb.execute('INSERT INTO SchedGroupMembers(\n'
                          '         idSchedGroup,\n'
                          '         idTestGroup,\n'
                          '         tsEffective,\n'
                          '         uidAuthor,\n'
                          '         iSchedPriority,\n'
                          '         bmHourlySchedule,\n'
                          '         idTestGroupPreReq)\n'
                          'VALUES (%s, %s, %s, %s, %s, %s, %s)\n'
                          , ( oMember.idSchedGroup,
                              oMember.idTestGroup,
                              tsEffective,
                              uidAuthor,
                              oMember.iSchedPriority,
                              oMember.bmHourlySchedule,
                              oMember.idTestGroupPreReq, ));
        return True;

    def _removeSchedGroupMember(self, uidAuthor, oMember):
        """
        Removes a scheduling group member.
        """

        # Try record who removed it by adding an dummy entry that expires immediately.
        (tsCur, tsCurMinusOne) = self._oDb.getCurrentTimestamps();
        if oMember.tsEffective != tsCur and oMember.tsEffective != tsCurMinusOne:
            self._historizeSchedGroupMember(oMember, tsCurMinusOne);
            self._addSchedGroupMember(uidAuthor, oMember, tsCurMinusOne); # lazy bird.
            self._historizeSchedGroupMember(oMember);
        else:
            self._historizeSchedGroupMember(oMember);
        return True;

    def _historizeSchedGroupMember(self, oMember, tsExpire = None):
        """
        Historizes the current entry for the given scheduling group.
        """
        if tsExpire is None:
            tsExpire = self._oDb.getCurrentTimestamp();
        self._oDb.execute('UPDATE SchedGroupMembers\n'
                          'SET    tsExpire = %s\n'
                          'WHERE  idSchedGroup = %s\n'
                          '   AND idTestGroup  = %s\n'
                          '   AND tsExpire     = \'infinity\'::TIMESTAMP\n'
                          , ( tsExpire, oMember.idSchedGroup, oMember.idTestGroup, ));
        return True;




#
# Unit testing.
#

# pylint: disable=C0111
class SchedGroupMemberDataTestCase(ModelDataBaseTestCase):
    def setUp(self):
        self.aoSamples = [SchedGroupMemberData(),];

class SchedGroupDataTestCase(ModelDataBaseTestCase):
    def setUp(self):
        self.aoSamples = [SchedGroupData(),];

if __name__ == '__main__':
    unittest.main();
    # not reached.

