# -*- coding: utf-8 -*-
# $Id: wuiadminbuildcategory.py $

"""
Test Manager WUI - Build categories.
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


# Validation Kit imports.
from common                             import webutils;
from testmanager.webui.wuicontentbase   import WuiListContentBase, WuiFormContentBase, WuiRawHtml, WuiTmLink;
from testmanager.core.build             import BuildCategoryData
from testmanager.core                   import coreconsts;


class WuiAdminBuildCatList(WuiListContentBase):
    """
    WUI Build Category List Content Generator.
    """

    def __init__(self, aoEntries, iPage, cItemsPerPage, tsEffective, fnDPrint, oDisp, aiSelectedSortColumns = None):
        WuiListContentBase.__init__(self, aoEntries, iPage, cItemsPerPage, tsEffective,
                                    sTitle = 'Build Categories', sId = 'buildcategories',
                                    fnDPrint = fnDPrint, oDisp = oDisp, aiSelectedSortColumns = aiSelectedSortColumns);
        self._asColumnHeaders = ([ 'ID', 'Product', 'Repository', 'Branch', 'Build Type', 'OS/Architectures', 'Actions' ]);
        self._asColumnAttribs = (['align="right"', '', '', '', '', 'align="center"' ]);

    def _formatListEntry(self, iEntry):
        from testmanager.webui.wuiadmin import WuiAdmin;
        oEntry  = self._aoEntries[iEntry];

        aoActions = [
            WuiTmLink('Details', WuiAdmin.ksScriptName,
                      { WuiAdmin.ksParamAction: WuiAdmin.ksActionBuildCategoryDetails,
                        BuildCategoryData.ksParam_idBuildCategory: oEntry.idBuildCategory, }),
        ];
        if self._oDisp is None or not self._oDisp.isReadOnlyUser():
            aoActions += [
                WuiTmLink('Clone', WuiAdmin.ksScriptName,
                          { WuiAdmin.ksParamAction: WuiAdmin.ksActionBuildCategoryClone,
                            BuildCategoryData.ksParam_idBuildCategory: oEntry.idBuildCategory, }),
                WuiTmLink('Try Remove', WuiAdmin.ksScriptName,
                          { WuiAdmin.ksParamAction: WuiAdmin.ksActionBuildCategoryDoRemove,
                            BuildCategoryData.ksParam_idBuildCategory: oEntry.idBuildCategory, }),
            ];

        sHtml = '<ul class="tmshowall">\n';
        for sOsArch in oEntry.asOsArches:
            sHtml += '  <li class="tmshowall">%s</li>\n' % (webutils.escapeElem(sOsArch),);
        sHtml += '</ul>\n'

        return [ oEntry.idBuildCategory,
                 oEntry.sRepository,
                 oEntry.sProduct,
                 oEntry.sBranch,
                 oEntry.sType,
                 WuiRawHtml(sHtml),
                 aoActions,
        ];


class WuiAdminBuildCat(WuiFormContentBase):
    """
    WUI Build Category Form Content Generator.
    """
    def __init__(self, oData, sMode, oDisp):
        if sMode == WuiFormContentBase.ksMode_Add:
            sTitle = 'Create Build Category';
        elif sMode == WuiFormContentBase.ksMode_Edit:
            assert False, 'not possible'
        else:
            assert sMode == WuiFormContentBase.ksMode_Show;
            sTitle = 'Build Category- %s' % (oData.idBuildCategory,);
        WuiFormContentBase.__init__(self, oData, sMode, 'BuildCategory', oDisp, sTitle, fEditable = False);

    def _populateForm(self, oForm, oData):
        oForm.addIntRO( BuildCategoryData.ksParam_idBuildCategory,      oData.idBuildCategory,  'Build Category ID')
        oForm.addText(  BuildCategoryData.ksParam_sRepository,          oData.sRepository,      'VCS repository name');
        oForm.addText(  BuildCategoryData.ksParam_sProduct,             oData.sProduct,         'Product name')
        oForm.addText(  BuildCategoryData.ksParam_sBranch,              oData.sBranch,          'Branch name')
        oForm.addText(  BuildCategoryData.ksParam_sType,                oData.sType,            'Build type')

        aoOsArches = [[sOsArch, sOsArch in oData.asOsArches, sOsArch] for sOsArch in coreconsts.g_kasOsDotCpusAll];
        oForm.addListOfOsArches(BuildCategoryData.ksParam_asOsArches,   aoOsArches,             'Target architectures');

        if self._sMode != WuiFormContentBase.ksMode_Show:
            oForm.addSubmit();
        return True;

