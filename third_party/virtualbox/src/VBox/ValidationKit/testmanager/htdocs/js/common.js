/* $Id: common.js $ */
/** @file
 * Common JavaScript functions
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Same as WuiDispatcherBase.ksParamRedirectTo. */
var g_ksParamRedirectTo = 'RedirectTo';


/**
 * Checks if the given value is a decimal integer value.
 *
 * @returns true if it is, false if it's isn't.
 * @param   sValue              The value to inspect.
 */
function isInteger(sValue)
{
    if (typeof sValue != 'undefined')
    {
        var intRegex = /^\d+$/;
        if (intRegex.test(sValue))
        {
            return true;
        }
    }
    return false;
}

/**
 * Checks if @a oMemmber is present in aoArray.
 *
 * @returns true/false.
 * @param   aoArray             The array to check.
 * @param   oMember             The member to check for.
 */
function isMemberOfArray(aoArray, oMember)
{
    var i;
    for (i = 0; i < aoArray.length; i++)
        if (aoArray[i] == oMember)
            return true;
    return false;
}

/**
 * Removes the element with the specified ID.
 */
function removeHtmlNode(sContainerId)
{
    var oElement = document.getElementById(sContainerId);
    if (oElement)
    {
        oElement.parentNode.removeChild(oElement);
    }
}

/**
 * Sets the value of the element with id @a sInputId to the keys of aoItems
 * (comma separated).
 */
function setElementValueToKeyList(sInputId, aoItems)
{
    var sKey;
    var oElement = document.getElementById(sInputId);
    oElement.value = '';

    for (sKey in aoItems)
    {
        if (oElement.value.length > 0)
        {
            oElement.value += ',';
        }

        oElement.value += sKey;
    }
}

/**
 * Get the Window.devicePixelRatio in a safe way.
 *
 * @returns Floating point ratio. 1.0 means it's a 1:1 ratio.
 */
function getDevicePixelRatio()
{
    var fpRatio = 1.0;
    if (window.devicePixelRatio)
    {
        fpRatio = window.devicePixelRatio;
        if (fpRatio < 0.5 || fpRatio > 10.0)
            fpRatio = 1.0;
    }
    return fpRatio;
}

/**
 * Tries to figure out the DPI of the device in the X direction.
 *
 * @returns DPI on success, null on failure.
 */
function getDeviceXDotsPerInch()
{
    if (window.deviceXDPI && window.deviceXDPI > 48 && window.deviceXDPI < 2048)
    {
        return window.deviceXDPI;
    }
    else if (window.devicePixelRatio && window.devicePixelRatio >= 0.5 && window.devicePixelRatio <= 10.0)
    {
        cDotsPerInch = Math.round(96 * window.devicePixelRatio);
    }
    else
    {
        cDotsPerInch = null;
    }
    return cDotsPerInch;
}

/**
 * Gets the width of the given element (downscaled).
 *
 * Useful when using the element to figure the size of a image
 * or similar.
 *
 * @returns Number of pixels.  null if oElement is bad.
 * @param   oElement        The element (not ID).
 */
function getElementWidth(oElement)
{
    if (oElement && oElement.offsetWidth)
        return oElement.offsetWidth;
    return null;
}

/** By element ID version of getElementWidth. */
function getElementWidthById(sElementId)
{
    return getElementWidth(document.getElementById(sElementId));
}

/**
 * Gets the real unscaled width of the given element.
 *
 * Useful when using the element to figure the size of a image
 * or similar.
 *
 * @returns Number of screen pixels.  null if oElement is bad.
 * @param   oElement        The element (not ID).
 */
function getUnscaledElementWidth(oElement)
{
    if (oElement && oElement.offsetWidth)
        return Math.round(oElement.offsetWidth * getDevicePixelRatio());
    return null;
}

/** By element ID version of getUnscaledElementWidth. */
function getUnscaledElementWidthById(sElementId)
{
    return getUnscaledElementWidth(document.getElementById(sElementId));
}

/**
 * Gets the part of the URL needed for a RedirectTo parameter.
 *
 * @returns URL string.
 */
function getCurrentBrowerUrlPartForRedirectTo()
{
    var sWhere = window.location.href;
    var offTmp;
    var offPathKeep;

    /* Find the end of that URL 'path' component. */
    var offPathEnd = sWhere.indexOf('?');
    if (offPathEnd < 0)
        offPathEnd = sWhere.indexOf('#');
    if (offPathEnd < 0)
        offPathEnd = sWhere.length;

    /* Go backwards from the end of the and find the start of the last component. */
    offPathKeep = sWhere.lastIndexOf("/", offPathEnd);
    offTmp = sWhere.lastIndexOf(":", offPathEnd);
    if (offPathKeep < offTmp)
        offPathKeep = offTmp;
    offTmp = sWhere.lastIndexOf("\\", offPathEnd);
    if (offPathKeep < offTmp)
        offPathKeep = offTmp;

    return sWhere.substring(offPathKeep + 1);
}

/**
 * Adds the given sorting options to the URL and reloads.
 *
 * This will preserve previous sorting columns except for those
 * given in @a aiColumns.
 *
 * @param   sParam              Sorting parameter.
 * @param   aiColumns           Array of sorting columns.
 */
function ahrefActionSortByColumns(sParam, aiColumns)
{
    var sWhere = window.location.href;

    var offHash = sWhere.indexOf('#');
    if (offHash < 0)
        offHash = sWhere.length;

    var offQm = sWhere.indexOf('?');
    if (offQm > offHash)
        offQm = -1;

    var sNew = '';
    if (offQm > 0)
        sNew = sWhere.substring(0, offQm);

    sNew += '?' + sParam + '=' + aiColumns[0];
    var i;
    for (i = 1; i < aiColumns.length; i++)
        sNew += '&' + sParam + '=' + aiColumns[i];

    if (offQm >= 0 && offQm + 1 < offHash)
    {
        var sArgs = '&' + sWhere.substring(offQm + 1, offHash);
        var off   = 0;
        while (off < sArgs.length)
        {
            var offMatch = sArgs.indexOf('&' + sParam + '=', off);
            if (offMatch >= 0)
            {
                if (off < offMatch)
                    sNew += sArgs.substring(off, offMatch);

                var offValue = offMatch + 1 + sParam.length + 1;
                offEnd = sArgs.indexOf('&', offValue);
                if (offEnd < offValue)
                    offEnd = sArgs.length;

                var iColumn = parseInt(sArgs.substring(offValue, offEnd));
                if (!isMemberOfArray(aiColumns, iColumn) && !isMemberOfArray(aiColumns, -iColumn))
                    sNew += sArgs.substring(offMatch, offEnd);

                off = offEnd;
            }
            else
            {
                sNew += sArgs.substring(off);
                break;
            }
        }
    }

    if (offHash < sWhere.length)
        sNew = sWhere.substr(offHash);

    window.location.href = sNew;
}

/**
 * Sets the value of an input field element (give by ID).
 *
 * @returns Returns success indicator (true/false).
 * @param   sFieldId            The field ID (required for updating).
 * @param   sValue              The field value.
 */
function setInputFieldValue(sFieldId, sValue)
{
    var oInputElement = document.getElementById(sFieldId);
    if (oInputElement)
    {
        oInputElement.value = sValue;
        return true;
    }
    return false;
}

/**
 * Adds a hidden input field to a form.
 *
 * @returns The new input field element.
 * @param   oFormElement        The form to append it to.
 * @param   sName               The field name.
 * @param   sValue              The field value.
 * @param   sFieldId            The field ID (optional).
 */
function addHiddenInputFieldToForm(oFormElement, sName, sValue, sFieldId)
{
    var oNew = document.createElement('input');
    oNew.type  = 'hidden';
    oNew.name  = sName;
    oNew.value = sValue;
    if (sFieldId)
        oNew.id = sFieldId;
    oFormElement.appendChild(oNew);
    return oNew;
}

/** By element ID version of addHiddenInputFieldToForm. */
function addHiddenInputFieldToFormById(sFormId, sName, sValue, sFieldId)
{
    return addHiddenInputFieldToForm(document.getElementById(sFormId), sName, sValue, sFieldId);
}

/**
 * Adds or updates a hidden input field to/on a form.
 *
 * @returns The new input field element.
 * @param   sFormId             The ID of the form to amend.
 * @param   sName               The field name.
 * @param   sValue              The field value.
 * @param   sFieldId            The field ID (required for updating).
 */
function addUpdateHiddenInputFieldToFormById(sFormId, sName, sValue, sFieldId)
{
    var oInputElement = null;
    if (sFieldId)
    {
        oInputElement = document.getElementById(sFieldId);
    }
    if (oInputElement)
    {
        oInputElement.name  = sName;
        oInputElement.value = sValue;
    }
    else
    {
        oInputElement = addHiddenInputFieldToFormById(sFormId, sName, sValue, sFieldId);
    }
    return oInputElement;
}

/**
 * Adds a width and a dpi input to the given form element if possible to
 * determine the values.
 *
 * This is normally employed in an onlick hook, but then you must specify IDs or
 * the browser may end up adding it several times.
 *
 * @param   sFormId             The ID of the form to amend.
 * @param   sWidthSrcId         The ID of the element to calculate the width
 *                              value from.
 * @param   sWidthName          The name of the width value.
 * @param   sDpiName            The name of the dpi value.
 */
function addDynamicGraphInputs(sFormId, sWidthSrcId, sWidthName, sDpiName)
{
    var cx            = getUnscaledElementWidthById(sWidthSrcId);
    var cDotsPerInch  = getDeviceXDotsPerInch();

    if (cx)
    {
        addUpdateHiddenInputFieldToFormById(sFormId, sWidthName, cx, sFormId + '-' + sWidthName + '-id');
    }

    if (cDotsPerInch)
    {
        addUpdateHiddenInputFieldToFormById(sFormId, sDpiName, cDotsPerInch, sFormId + '-' + sDpiName + '-id');
    }

}

/**
 * Adds the RedirecTo field with the current URL to the form.
 *
 * This is a 'onsubmit' action.
 *
 * @returns Returns success indicator (true/false).
 * @param   oForm               The form being submitted.
 */
function addRedirectToInputFieldWithCurrentUrl(oForm)
{
    /* Constant used here is duplicated in WuiDispatcherBase.ksParamRedirectTo */
    return addHiddenInputFieldToForm(oForm, 'RedirectTo', getCurrentBrowerUrlPartForRedirectTo(), null);
}

/**
 * Adds the RedirecTo parameter to the href of the given anchor.
 *
 * This is a 'onclick' action.
 *
 * @returns Returns success indicator (true/false).
 * @param   oAnchor         The anchor element being clicked on.
 */
function addRedirectToAnchorHref(oAnchor)
{
    var sRedirectToParam = g_ksParamRedirectTo + '=' + encodeURIComponent(getCurrentBrowerUrlPartForRedirectTo());
    var sHref = oAnchor.href;
    if (sHref.indexOf(sRedirectToParam) < 0)
    {
        var sHash;
        var offHash = sHref.indexOf('#');
        if (offHash >= 0)
            sHash = sHref.substring(offHash);
        else
        {
            sHash   = '';
            offHash = sHref.length;
        }
        sHref = sHref.substring(0, offHash)
        if (sHref.indexOf('?') >= 0)
            sHref += '&';
        else
            sHref += '?';
        sHref += sRedirectToParam;
        sHref += sHash;
        oAnchor.href = sHref;
    }
    return true;
}



/**
 * Clears one input element.
 *
 * @param   oInput      The input to clear.
 */
function resetInput(oInput)
{
    switch (oInput.type)
    {
        case 'checkbox':
        case 'radio':
            oInput.checked = false;
            break;

        case 'text':
            oInput.value = 0;
            break;
    }
}


/**
 * Clears a form.
 *
 * @param   sIdForm     The ID of the form
 */
function clearForm(sIdForm)
{
    var oForm = document.getElementById(sIdForm);
    if (oForm)
    {
        var aoInputs = oForm.getElementsByTagName('INPUT');
        var i;
        for (i = 0; i < aoInputs.length; i++)
            resetInput(aoInputs[i])

        /* HTML5 allows inputs outside <form>, so scan the document. */
        aoInputs = document.getElementsByTagName('INPUT');
        for (i = 0; i < aoInputs.length; i++)
            if (aoInputs.hasOwnProperty("form"))
                if (aoInputs.form == sIdForm)
                    resetInput(aoInputs[i])
    }

    return true;
}


/** @name Collapsible / Expandable items
 * @{
 */


/**
 * Toggles the collapsible / expandable state of a parent DD and DT uncle.
 *
 * @returns true
 * @param   oAnchor             The anchor object.
 */
function toggleCollapsibleDtDd(oAnchor)
{
    var oParent = oAnchor.parentElement;
    var sClass  = oParent.className;

    /* Find the DD sibling tag */
    var oDdElement = oParent.nextSibling;
    while (oDdElement != null && oDdElement.tagName != 'DD')
        oDdElement = oDdElement.nextSibling;

    /* Determin the new class and arrow char. */
    var sNewClass;
    var sNewChar;
    if (     sClass.substr(-11) == 'collapsible')
    {
        sNewClass = sClass.substr(0, sClass.length - 11) + 'expandable';
        sNewChar  = '\u25B6'; /* black right-pointing triangle */
    }
    else if (sClass.substr(-10) == 'expandable')
    {
        sNewClass = sClass.substr(0, sClass.length - 10) + 'collapsible';
        sNewChar  = '\u25BC'; /* black down-pointing triangle */
    }
    else
    {
        console.log('toggleCollapsibleParent: Invalid class: ' + sClass);
        return true;
    }

    /* Update the parent (DT) class and anchor text. */
    oParent.className   = sNewClass;
    oAnchor.firstChild.textContent = sNewChar + oAnchor.firstChild.textContent.substr(1);

    /* Update the uncle (DD) class. */
    if (oDdElement)
        oDdElement.className = sNewClass;
    return true;
}

/**
 * Shows/hides a sub-category UL according to checkbox status.
 *
 * The checkbox is expected to be within a label element or something.
 *
 * @returns true
 * @param   oInput          The input checkbox.
 */
function toggleCollapsibleCheckbox(oInput)
{
    var oParent = oInput.parentElement;

    /* Find the UL sibling element. */
    var oUlElement = oParent.nextSibling;
    while (oUlElement != null && oUlElement.tagName != 'UL')
        oUlElement = oUlElement.nextSibling;

    /* Change the visibility. */
    if (oInput.checked)
        oUlElement.className = oUlElement.className.replace('expandable', 'collapsible');
    else
    {
        oUlElement.className = oUlElement.className.replace('collapsible', 'expandable');

        /* Make sure all sub-checkboxes are now unchecked. */
        var aoSubInputs = oUlElement.getElementsByTagName('input');
        var i;
        for (i = 0; i < aoSubInputs.length; i++)
            aoSubInputs[i].checked = false;
    }
    return true;
}

/**
 * Toggles the sidebar size so filters can more easily manipulated.
 */
function toggleSidebarSize()
{
    var sLinkText;
    if (document.body.className != 'tm-wide-side-menu')
    {
        document.body.className = 'tm-wide-side-menu';
        sLinkText = '\u00ab\u00ab';
    }
    else
    {
        document.body.className = '';
        sLinkText = '\u00bb\u00bb';
    }

    var aoToggleLink = document.getElementsByClassName('tm-sidebar-size-link');
    var i;
    for (i = 0; i < aoToggleLink.length; i++)
        if (   aoToggleLink[i].textContent.indexOf('\u00bb') >= 0
            || aoToggleLink[i].textContent.indexOf('\u00ab') >= 0)
            aoToggleLink[i].textContent = sLinkText;
}

/** @} */


/** @name Custom Tooltips
 * @{
 */

/** Where we keep tooltip elements when not displayed. */
var g_dTooltips          = {};
var g_oCurrentTooltip    = null;
var g_idTooltipShowTimer = null;
var g_idTooltipHideTimer = null;
var g_cTooltipSvnRevisions = 12;

/**
 * Cancel showing/replacing/repositing a tooltip.
 */
function tooltipResetShowTimer()
{
    if (g_idTooltipShowTimer)
    {
        clearTimeout(g_idTooltipShowTimer);
        g_idTooltipShowTimer = null;
    }
}

/**
 * Cancel hiding of the current tooltip.
 */
function tooltipResetHideTimer()
{
    if (g_idTooltipHideTimer)
    {
        clearTimeout(g_idTooltipHideTimer);
        g_idTooltipHideTimer = null;
    }
}

/**
 * Really hide the tooltip.
 */
function tooltipReallyHide()
{
    if (g_oCurrentTooltip)
    {
        //console.log('tooltipReallyHide: ' + g_oCurrentTooltip);
        g_oCurrentTooltip.oElm.style.display = 'none';
        g_oCurrentTooltip = null;
    }
}

/**
 * Schedule the tooltip for hiding.
 */
function tooltipHide()
{
    function tooltipDelayedHide()
    {
        tooltipResetHideTimer();
        tooltipReallyHide();
    }

    /*
     * Cancel any pending show and schedule hiding if necessary.
     */
    tooltipResetShowTimer();
    if (g_oCurrentTooltip && !g_idTooltipHideTimer)
    {
        g_idTooltipHideTimer = setTimeout(tooltipDelayedHide, 700);
    }

    return true;
}

/**
 * Function that is repositions the tooltip when it's shown.
 *
 * Used directly, via onload, and hackish timers to catch all browsers and
 * whatnot.
 *
 * Will set several tooltip member variables related to position and space.
 */
function tooltipRepositionOnLoad()
{
    if (g_oCurrentTooltip)
    {
        var oRelToRect = g_oCurrentTooltip.oRelToRect;
        var cxNeeded   = g_oCurrentTooltip.oElm.offsetWidth  + 8;
        var cyNeeded   = g_oCurrentTooltip.oElm.offsetHeight + 8;

        var yScroll         = window.pageYOffset || document.documentElement.scrollTop;
        var yScrollBottom   = yScroll + window.innerHeight;
        var xScroll         = window.pageXOffset || document.documentElement.scrollLeft;
        var xScrollRight    = xScroll + window.innerWidth;

        var cyAbove    = Math.max(oRelToRect.top  - yScroll, 0);
        var cyBelow    = Math.max(yScrollBottom - oRelToRect.bottom, 0);
        var cxLeft     = Math.max(oRelToRect.left - xScroll, 0);
        var cxRight    = Math.max(xScrollRight  - oRelToRect.right, 0);

        var xPos;
        var yPos;

        /*
         * Decide where to put the thing.
         */
        if (cyNeeded < cyBelow)
        {
            yPos = oRelToRect.bottom;
            g_oCurrentTooltip.cyMax = cyBelow;
        }
        else if (cyBelow >= cyAbove)
        {
            yPos = yScrollBottom - cyNeeded;
            g_oCurrentTooltip.cyMax = yScrollBottom - yPos;
        }
        else
        {
            yPos = oRelToRect.top - cyNeeded;
            g_oCurrentTooltip.cyMax = yScrollBottom - yPos;
        }
        if (yPos < yScroll)
        {
            yPos = yScroll;
            g_oCurrentTooltip.cyMax = yScrollBottom - yPos;
        }
        g_oCurrentTooltip.yPos    = yPos;
        g_oCurrentTooltip.yScroll = yScroll;
        g_oCurrentTooltip.cyMaxUp = yPos - yScroll;

        if (cxNeeded < cxRight || cxNeeded > cxRight)
        {
            xPos = oRelToRect.right;
            g_oCurrentTooltip.cxMax = cxRight;
        }
        else
        {
            xPos = oRelToRect.left - cxNeeded;
            g_oCurrentTooltip.cxMax = cxNeeded;
        }
        g_oCurrentTooltip.xPos    = xPos;
        g_oCurrentTooltip.xScroll = xScroll;

        g_oCurrentTooltip.oElm.style.top  = yPos + 'px';
        g_oCurrentTooltip.oElm.style.left = xPos + 'px';
    }
    return true;
}


/**
 * Really show the tooltip.
 *
 * @param   oTooltip            The tooltip object.
 * @param   oRelTo              What to put the tooltip adjecent to.
 */
function tooltipReallyShow(oTooltip, oRelTo)
{
    var oRect;

    tooltipResetShowTimer();
    tooltipResetHideTimer();

    if (g_oCurrentTooltip == oTooltip)
    {
        //console.log('moving tooltip');
    }
    else if (g_oCurrentTooltip)
    {
        //console.log('removing current tooltip and showing new');
        tooltipReallyHide();
    }
    else
    {
        //console.log('showing tooltip');
    }

    oTooltip.oElm.style.display  = 'block';
    oTooltip.oElm.style.position = 'absolute';
    oRect = oRelTo.getBoundingClientRect();
    oTooltip.oRelToRect = oRect;
    oTooltip.oElm.style.left     = oRect.right + 'px';
    oTooltip.oElm.style.top      = oRect.bottom + 'px';

    g_oCurrentTooltip = oTooltip;

    /*
     * This function does the repositioning at some point.
     */
    tooltipRepositionOnLoad();
    if (oTooltip.oElm.onload === null)
    {
        oTooltip.oElm.onload = function(){ tooltipRepositionOnLoad(); setTimeout(tooltipRepositionOnLoad, 0); };
    }
}

/**
 * Tooltip onmouseenter handler .
 */
function tooltipElementOnMouseEnter()
{
    //console.log('tooltipElementOnMouseEnter: arguments.length='+arguments.length+' [0]='+arguments[0]);
    //console.log('ENT: currentTarget='+arguments[0].currentTarget);
    tooltipResetShowTimer();
    tooltipResetHideTimer();
    return true;
}

/**
 * Tooltip onmouseout handler.
 *
 * @remarks We only use this and onmouseenter for one tooltip element (iframe
 *          for svn, because chrome is sending onmouseout events after
 *          onmouseneter for the next element, which would confuse this simple
 *          code.
 */
function tooltipElementOnMouseOut()
{
    //console.log('tooltipElementOnMouseOut: arguments.length='+arguments.length+' [0]='+arguments[0]);
    //console.log('OUT: currentTarget='+arguments[0].currentTarget);
    tooltipHide();
    return true;
}

/**
 * iframe.onload hook that repositions and resizes the tooltip.
 *
 * This is a little hacky and we're calling it one or three times too many to
 * work around various browser differences too.
 */
function svnHistoryTooltipOnLoad()
{
    //console.log('svnHistoryTooltipOnLoad');

    /*
     * Resize the tooltip to better fit the content.
     */
    tooltipRepositionOnLoad(); /* Sets cxMax and cyMax. */
    if (g_oCurrentTooltip && g_oCurrentTooltip.oIFrame.contentWindow)
    {
        var oSubElement = g_oCurrentTooltip.oIFrame;
        var cxSpace  = Math.max(oSubElement.offsetLeft * 2, 0); /* simplified */
        var cySpace  = Math.max(oSubElement.offsetTop  * 2, 0); /* simplified */
        var cxNeeded = oSubElement.contentWindow.document.body.scrollWidth  + cxSpace;
        var cyNeeded = oSubElement.contentWindow.document.body.scrollHeight + cySpace;
        var cx = Math.min(cxNeeded, g_oCurrentTooltip.cxMax);
        var cy;

        g_oCurrentTooltip.oElm.width = cx + 'px';
        oSubElement.width            = (cx - cxSpace) + 'px';
        if (cx >= cxNeeded)
        {
            //console.log('svnHistoryTooltipOnLoad: overflowX -> hidden');
            oSubElement.style.overflowX = 'hidden';
        }
        else
        {
            oSubElement.style.overflowX = 'scroll';
        }

        cy = Math.min(cyNeeded, g_oCurrentTooltip.cyMax);
        if (cyNeeded > g_oCurrentTooltip.cyMax && g_oCurrentTooltip.cyMaxUp > 0)
        {
            var cyMove = Math.min(cyNeeded - g_oCurrentTooltip.cyMax, g_oCurrentTooltip.cyMaxUp);
            g_oCurrentTooltip.cyMax += cyMove;
            g_oCurrentTooltip.yPos  -= cyMove;
            g_oCurrentTooltip.oElm.style.top = g_oCurrentTooltip.yPos + 'px';
            cy = Math.min(cyNeeded, g_oCurrentTooltip.cyMax);
        }

        g_oCurrentTooltip.oElm.height = cy + 'px';
        oSubElement.height            = (cy - cySpace) + 'px';
        if (cy >= cyNeeded)
        {
            //console.log('svnHistoryTooltipOnLoad: overflowY -> hidden');
            oSubElement.style.overflowY = 'hidden';
        }
        else
        {
            oSubElement.style.overflowY = 'scroll';
        }

        //console.log('cyNeeded='+cyNeeded+' cyMax='+g_oCurrentTooltip.cyMax+' cySpace='+cySpace+' cy='+cy);
        //console.log('oSubElement.offsetTop='+oSubElement.offsetTop);
        //console.log('svnHistoryTooltipOnLoad: cx='+cx+'cxMax='+g_oCurrentTooltip.cxMax+' cxNeeded='+cxNeeded+' cy='+cy+' cyMax='+g_oCurrentTooltip.cyMax);

        tooltipRepositionOnLoad();
    }
    return true;
}

/**
 * Calculates the last revision to get when showing a tooltip for @a iRevision.
 *
 * A tooltip covers several change log entries, both to limit the number of
 * tooltips to load and to give context.  The exact number is defined by
 * g_cTooltipSvnRevisions.
 *
 * @returns Last revision in a tooltip.
 * @param   iRevision   The revision number.
 */
function svnHistoryTooltipCalcLastRevision(iRevision)
{
    var iFirstRev = Math.floor(iRevision / g_cTooltipSvnRevisions) * g_cTooltipSvnRevisions;
    return iFirstRev + g_cTooltipSvnRevisions - 1;
}

/**
 * Calculates a unique ID for the tooltip element.
 *
 * This is also used as dictionary index.
 *
 * @returns tooltip ID value (string).
 * @param   sRepository The repository name.
 * @param   iRevision   The revision number.
 */
function svnHistoryTooltipCalcId(sRepository, iRevision)
{
    return 'svnHistoryTooltip_' + sRepository + '_' + svnHistoryTooltipCalcLastRevision(iRevision);
}

/**
 * The onmouseenter event handler for creating the tooltip.
 *
 * @param   oEvt        The event.
 * @param   sRepository The repository name.
 * @param   iRevision   The revision number.
 *
 * @remarks onmouseout must be set to call tooltipHide.
 */
function svnHistoryTooltipShow(oEvt, sRepository, iRevision)
{
    var sKey        = svnHistoryTooltipCalcId(sRepository, iRevision);
    var oTooltip    = g_dTooltips[sKey];
    var oParent     = oEvt.currentTarget;
    //console.log('svnHistoryTooltipShow ' + sRepository);

    function svnHistoryTooltipDelayedShow()
    {
        var oSubElement;
        var sSrc;

        oTooltip = g_dTooltips[sKey];
        //console.log('svnHistoryTooltipDelayedShow ' + sRepository + ' ' + oTooltip);
        if (!oTooltip)
        {
            /*
             * Create a new tooltip element.
             */
            //console.log('creating ' + sKey);
            oTooltip = {};
            oTooltip.oElm = document.createElement('div');
            oTooltip.oElm.setAttribute('id', sKey);
            oTooltip.oElm.setAttribute('class', 'tmvcstooltip');
            oTooltip.oElm.style.position = 'absolute';
            oTooltip.oElm.style.zIndex = 6001;
            oTooltip.xPos    = 0;
            oTooltip.yPos    = 0;
            oTooltip.cxMax   = 0;
            oTooltip.cyMax   = 0;
            oTooltip.cyMaxUp = 0;
            oTooltip.xScroll = 0;
            oTooltip.yScroll = 0;

            oSubElement = document.createElement('iframe');
            oSubElement.setAttribute('id', sKey + '_iframe');
            oSubElement.setAttribute('style', 'position: relative;"');
            oSubElement.onload       = function() {svnHistoryTooltipOnLoad(); setTimeout(svnHistoryTooltipOnLoad,0);};
            oSubElement.onmouseenter = tooltipElementOnMouseEnter;
            oSubElement.onmouseout   = tooltipElementOnMouseOut;
            oTooltip.oElm.appendChild(oSubElement);
            oTooltip.oIFrame = oSubElement;
            g_dTooltips[sKey] = oTooltip;

            document.body.appendChild(oTooltip.oElm);
        }
        else
        {
            oSubElement = oTooltip.oIFrame;
        }

        oSubElement.setAttribute('src', 'index.py?Action=VcsHistoryTooltip&repo=' + sRepository
                                 + '&rev=' + svnHistoryTooltipCalcLastRevision(iRevision)
                                 + '&cEntries=' + g_cTooltipSvnRevisions
                                 + '#r' + iRevision);
        tooltipReallyShow(oTooltip, oParent);
        /* Resize and repositioning hacks. */
        svnHistoryTooltipOnLoad();
        setTimeout(svnHistoryTooltipOnLoad, 0);
    }

    /*
     * Delay the change.
     */
    tooltipResetShowTimer();
    g_idTooltipShowTimer = setTimeout(svnHistoryTooltipDelayedShow, 512);
}

/** @} */


/** @name Debugging and Introspection
 * @{
 */

/**
 * Python-like dir() implementation.
 *
 * @returns Array of names associated with oObj.
 * @param   oObj        The object under inspection.  If not specified we'll
 *                      look at the window object.
 */
function pythonlikeDir(oObj, fDeep)
{
    var aRet = [];
    var dTmp = {};

    if (!oObj)
    {
        oObj = window;
    }

    for (var oCur = oObj; oCur; oCur = Object.getPrototypeOf(oCur))
    {
        var aThis = Object.getOwnPropertyNames(oCur);
        for (var i = 0; i < aThis.length; i++)
        {
            if (!(aThis[i] in dTmp))
            {
                dTmp[aThis[i]] = 1;
                aRet.push(aThis[i]);
            }
        }
    }

    return aRet;
}


/**
 * Python-like dir() implementation, shallow version.
 *
 * @returns Array of names associated with oObj.
 * @param   oObj        The object under inspection.  If not specified we'll
 *                      look at the window object.
 */
function pythonlikeShallowDir(oObj, fDeep)
{
    var aRet = [];
    var dTmp = {};

    if (oObj)
    {
        for (var i in oObj)
        {
            aRet.push(i);
        }
    }

    return aRet;
}



function dbgGetObjType(oObj)
{
    var sType = typeof oObj;
    if (sType == "object" && oObj !== null)
    {
        if (oObj.constructor && oObj.constructor.name)
        {
            sType = oObj.constructor.name;
        }
        else
        {
            var fnToString = Object.prototype.toString;
            var sTmp = fnToString.call(oObj);
            if (sTmp.indexOf('[object ') === 0)
            {
                sType = sTmp.substring(8, sTmp.length);
            }
        }
    }
    return sType;
}


/**
 * Dumps the given object to the console.
 *
 * @param   oObj        The object under inspection.
 * @param   sPrefix     What to prefix the log output with.
 */
function dbgDumpObj(oObj, sName, sPrefix)
{
    var aMembers;
    var sType;

    /*
     * Defaults
     */
    if (!oObj)
    {
        oObj = window;
    }

    if (!sPrefix)
    {
        if (sName)
        {
            sPrefix = sName + ':';
        }
        else
        {
            sPrefix = 'dbgDumpObj:';
        }
    }

    if (!sName)
    {
        sName = '';
    }

    /*
     * The object itself.
     */
    sPrefix = sPrefix + ' ';
    console.log(sPrefix + sName + ' ' + dbgGetObjType(oObj));

    /*
     * The members.
     */
    sPrefix = sPrefix + ' ';
    aMembers = pythonlikeDir(oObj);
    for (i = 0; i < aMembers.length; i++)
    {
        console.log(sPrefix + aMembers[i]);
    }

    return true;
}

function dbgDumpObjWorker(sType, sName, oObj, sPrefix)
{
    var sRet;
    switch (sType)
    {
        case 'function':
        {
            sRet = sPrefix + 'function ' + sName + '()' + '\n';
            break;
        }

        case 'object':
        {
            sRet = sPrefix + 'var ' + sName + '(' + dbgGetObjType(oObj) + ') =';
            if (oObj !== null)
            {
                sRet += '\n';
            }
            else
            {
                sRet += ' null\n';
            }
            break;
        }

        case 'string':
        {
            sRet = sPrefix + 'var ' + sName + '(string, ' + oObj.length + ')';
            if (oObj.length < 80)
            {
                sRet += ' = "' + oObj + '"\n';
            }
            else
            {
                sRet += '\n';
            }
            break;
        }

        case 'Oops!':
            sRet = sPrefix + sName + '(??)\n';
            break;

        default:
            sRet = sPrefix + 'var ' + sName + '(' + sType + ')\n';
            break;
    }
    return sRet;
}


function dbgObjInArray(aoObjs, oObj)
{
    var i = aoObjs.length;
    while (i > 0)
    {
        i--;
        if (aoObjs[i] === oObj)
        {
            return true;
        }
    }
    return false;
}

function dbgDumpObjTreeWorker(oObj, sPrefix, aParentObjs, cMaxDepth)
{
    var sRet     = '';
    var aMembers = pythonlikeShallowDir(oObj);
    var i;

    for (i = 0; i < aMembers.length; i++)
    {
        //var sName = i;
        var sName = aMembers[i];
        var oMember;
        var sType;
        var oEx;

        try
        {
            oMember = oObj[sName];
            sType = typeof oObj[sName];
        }
        catch (oEx)
        {
            oMember = null;
            sType = 'Oops!';
        }

        //sRet += '[' + i + '/' + aMembers.length + ']';
        sRet += dbgDumpObjWorker(sType, sName, oMember, sPrefix);

        if (   sType == 'object'
            && oObj !== null)
        {

            if (dbgObjInArray(aParentObjs, oMember))
            {
                sRet += sPrefix + '! parent recursion\n';
            }
            else if (   sName == 'previousSibling'
                     || sName == 'previousElement'
                     || sName == 'lastChild'
                     || sName == 'firstElementChild'
                     || sName == 'lastElementChild'
                     || sName == 'nextElementSibling'
                     || sName == 'prevElementSibling'
                     || sName == 'parentElement'
                     || sName == 'ownerDocument')
            {
                sRet += sPrefix + '! potentially dangerous element name\n';
            }
            else if (aParentObjs.length >= cMaxDepth)
            {
                sRet = sRet.substring(0, sRet.length - 1);
                sRet += ' <too deep>!\n';
            }
            else
            {

                aParentObjs.push(oMember);
                if (i + 1 < aMembers.length)
                {
                    sRet += dbgDumpObjTreeWorker(oMember, sPrefix + '| ', aParentObjs, cMaxDepth);
                }
                else
                {
                    sRet += dbgDumpObjTreeWorker(oMember, sPrefix.substring(0, sPrefix.length - 2) + '  | ', aParentObjs, cMaxDepth);
                }
                aParentObjs.pop();
            }
        }
    }
    return sRet;
}

/**
 * Dumps the given object and all it's subobjects to the console.
 *
 * @returns String dump of the object.
 * @param   oObj        The object under inspection.
 * @param   sName       The object name (optional).
 * @param   sPrefix     What to prefix the log output with (optional).
 * @param   cMaxDepth   The max depth, optional.
 */
function dbgDumpObjTree(oObj, sName, sPrefix, cMaxDepth)
{
    var sType;
    var sRet;
    var oEx;

    /*
     * Defaults
     */
    if (!sPrefix)
    {
        sPrefix = '';
    }

    if (!sName)
    {
        sName = '??';
    }

    if (!cMaxDepth)
    {
        cMaxDepth = 2;
    }

    /*
     * The object itself.
     */
    try
    {
        sType = typeof oObj;
    }
    catch (oEx)
    {
        sType = 'Oops!';
    }
    sRet = dbgDumpObjWorker(sType, sName, oObj, sPrefix);
    if (sType == 'object' && oObj !== null)
    {
        var aParentObjs = Array();
        aParentObjs.push(oObj);
        sRet += dbgDumpObjTreeWorker(oObj, sPrefix + '| ', aParentObjs, cMaxDepth);
    }

    return sRet;
}

function dbgLogString(sLongString)
{
    var aStrings = sLongString.split("\n");
    var i;
    for (i = 0; i < aStrings.length; i++)
    {
        console.log(aStrings[i]);
    }
    console.log('dbgLogString - end - ' + aStrings.length + '/' + sLongString.length);
    return true;
}

function dbgLogObjTree(oObj, sName, sPrefix, cMaxDepth)
{
    return dbgLogString(dbgDumpObjTree(oObj, sName, sPrefix, cMaxDepth));
}

/** @}  */

