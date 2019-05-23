# -*- coding: utf-8 -*-
# $Id: wuihlpgraphgooglechart.py $

"""
Test Manager Web-UI - Graph Helpers - Implemented using Google Charts.
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
from testmanager.webui.wuihlpgraphbase  import WuiHlpGraphBase;
from testmanager.webui                  import wuihlpgraphsimple;


#*******************************************************************************
#*  Global Variables                                                           *
#*******************************************************************************
g_cGraphs = 0;

class WuiHlpGraphGoogleChartsBase(WuiHlpGraphBase):
    """ Base class for the Google Charts graphs. """
    pass;


## @todo bar graphs later.
WuiHlpBarGraph = wuihlpgraphsimple.WuiHlpBarGraph;


class WuiHlpLineGraph(WuiHlpGraphGoogleChartsBase):
    """
    Line graph.
    """

    ## @todo implement error bars.
    kfNoErrorBarsSupport = True;

    def __init__(self, sId, oData, oDisp = None, fErrorBarY = False):
        # oData must be a WuiHlpGraphDataTableEx like object.
        WuiHlpGraphGoogleChartsBase.__init__(self, sId, oData, oDisp);
        self._cMaxErrorBars = 12;
        self._fErrorBarY    = fErrorBarY;

    def setErrorBarY(self, fEnable):
        """ Enables or Disables error bars, making this work like a line graph. """
        self._fErrorBarY = fEnable;
        return True;

    def renderGraph(self): # pylint: disable=R0914
        fSlideFilter = True;

        # Tooltips?
        cTooltips = 0;
        for oSeries in self._oData.aoSeries:
            cTooltips += oSeries.asHtmlTooltips is not None;

        # Unique on load function.
        global g_cGraphs;
        iGraph = g_cGraphs;
        g_cGraphs += 1;

        sHtml  = '<div id="%s">\n' % ( self._sId, );
        if fSlideFilter:
            sHtml += ' <table><tr><td><div id="%s_graph"/></td></tr><tr><td><div id="%s_filter"/></td></tr></table>\n' \
                   % ( self._sId, self._sId, );

        sHtml += '<script type="text/javascript" src="https://www.google.com/jsapi"></script>\n' \
                 '<script type="text/javascript">\n' \
                 'google.load("visualization", "1.0", { packages: ["corechart"%s] });\n' \
                 'google.setOnLoadCallback(tmDrawLineGraph%u);\n' \
                 'function tmDrawLineGraph%u()\n' \
                 '{\n' \
                 '    var fnResize;\n' \
                 '    var fnRedraw;\n' \
                 '    var idRedrawTimer = null;\n' \
                 '    var cxCur = getElementWidthById("%s") - 20;\n' \
                 '    var oGraph;\n' \
                 '    var oData = new google.visualization.DataTable();\n' \
                 '    var fpXYRatio = %u / %u;\n' \
                 '    var dGraphOptions = \n' \
                 '    {\n' \
                 '         "title":     "%s",\n' \
                 '         "width":     cxCur,\n' \
                 '         "height":    Math.round(cxCur / fpXYRatio),\n' \
                 '         "pointSize": 2,\n' \
                 '         "fontSize": %u,\n' \
                 '         "hAxis":     { "title": "%s", "minorGridlines": { count: 5 }},\n' \
                 '         "vAxis":     { "title": "%s", "minorGridlines": { count: 5 }},\n' \
                 '         "theme":     "maximized",\n' \
                 '         "tooltip":   { "isHtml": %s }\n' \
                 '    };\n' \
                % ( ', "controls"' if fSlideFilter else '',
                    iGraph,
                    iGraph,
                    self._sId,
                    self._cxGraph, self._cyGraph,
                    self._sTitle if self._sTitle is not None else '',
                    self._cPtFont * self._cDpiGraph / 72, # fudge
                    self._oData.sXUnit if self._oData.sXUnit else '',
                    self._oData.sYUnit if self._oData.sYUnit else '',
                    'true' if cTooltips > 0 else 'false',
                    );
        if fSlideFilter:
            sHtml += '    var oDashboard = new google.visualization.Dashboard(document.getElementById("%s"));\n' \
                     '    var oSlide = new google.visualization.ControlWrapper({\n' \
                     '        "controlType": "NumberRangeFilter",\n' \
                     '        "containerId": "%s_filter",\n' \
                     '        "options": {\n' \
                     '            "filterColumnIndex": 0,\n' \
                     '            "ui": { "width": getElementWidthById("%s") / 2 }, \n' \
                     '        }\n' \
                     '     });\n' \
                   % ( self._sId,
                       self._sId,
                       self._sId,);

        # Data variables.
        for iSeries, oSeries in enumerate(self._oData.aoSeries):
            sHtml += '    var aSeries%u = [\n' % (iSeries,);
            if oSeries.asHtmlTooltips is None:
                sHtml += '[%s,%s]' % ( oSeries.aoXValues[0], oSeries.aoYValues[0],);
                for i in range(1, len(oSeries.aoXValues)):
                    if (i & 16) == 0:   sHtml += '\n';
                    sHtml += ',[%s,%s]' % ( oSeries.aoXValues[i], oSeries.aoYValues[i], );
            else:
                sHtml += '[%s,%s,"%s"]' \
                       % ( oSeries.aoXValues[0], oSeries.aoYValues[0],
                           webutils.escapeAttrJavaScriptStringDQ(oSeries.asHtmlTooltips[0]),);
                for i in range(1, len(oSeries.aoXValues)):
                    if (i & 16) == 0:   sHtml += '\n';
                    sHtml += ',[%s,%s,"%s"]' \
                           % ( oSeries.aoXValues[i], oSeries.aoYValues[i],
                               webutils.escapeAttrJavaScriptStringDQ(oSeries.asHtmlTooltips[i]),);

            sHtml += '];\n'

        sHtml += '    oData.addColumn("number", "%s");\n' % (self._oData.sXUnit if self._oData.sXUnit else '',);
        cVColumns = 0;
        for oSeries in self._oData.aoSeries:
            sHtml += '    oData.addColumn("number", "%s");\n' % (oSeries.sName,);
            if oSeries.asHtmlTooltips:
                sHtml += '    oData.addColumn({"type": "string", "role": "tooltip", "p": {"html": true}});\n';
                cVColumns += 1;
            cVColumns += 1;
        sHtml += 'var i;\n'

        cVColumsDone = 0;
        for iSeries, oSeries in enumerate(self._oData.aoSeries):
            sVar = 'aSeries%u' % (iSeries,);
            sHtml += '    for (i = 0; i < %s.length; i++)\n' \
                     '    {\n' \
                     '        oData.addRow([%s[i][0]%s,%s[i][1]%s%s]);\n' \
                   % ( sVar,
                       sVar,
                       ',null' * cVColumsDone,
                       sVar,
                       '' if oSeries.asHtmlTooltips is None else ',%s[i][2]' % (sVar,),
                       ',null' * (cVColumns - cVColumsDone - 1 - (oSeries.asHtmlTooltips is not None)),
                     );
            sHtml += '    }\n' \
                     '    %s = null\n' \
                   % (sVar,);
            cVColumsDone += 1 + (oSeries.asHtmlTooltips is not None);

        # Create and draw.
        if fSlideFilter:
            sHtml += '    oGraph = new google.visualization.ChartWrapper({\n' \
                     '        "chartType": "LineChart",\n' \
                     '        "containerId": "%s_graph",\n' \
                     '        "options": dGraphOptions\n' \
                     '    });\n' \
                     '    oDashboard.bind(oSlide, oGraph);\n' \
                     '    oDashboard.draw(oData);\n' \
                   % ( self._sId, );
        else:
            sHtml += '    oGraph = new google.visualization.LineChart(document.getElementById("%s"));\n' \
                     '    oGraph.draw(oData, dGraphOptions);\n' \
                   % ( self._sId, );

        # Register a resize handler for redrawing the graph, using a timer to delay it.
        sHtml += '    fnRedraw = function() {\n' \
                 '        var cxNew = getElementWidthById("%s") - 6;\n' \
                 '        if (Math.abs(cxNew - cxCur) > 8)\n' \
                 '        {\n' \
                 '            cxCur = cxNew;\n' \
                 '            dGraphOptions["width"]  = cxNew;\n' \
                 '            dGraphOptions["height"] = Math.round(cxNew / fpXYRatio);\n' \
                 '            oGraph.draw(oData, dGraphOptions);\n' \
                 '        }\n' \
                 '        clearTimeout(idRedrawTimer);\n' \
                 '        idRedrawTimer = null;\n' \
                 '        return true;\n' \
                 '    };\n' \
                 '    fnResize = function() {\n' \
                 '        if (idRedrawTimer != null) { clearTimeout(idRedrawTimer); } \n' \
                 '        idRedrawTimer = setTimeout(fnRedraw, 512);\n' \
                 '        return true;\n' \
                 '    };\n' \
                 '    if (window.attachEvent)\n' \
                 '    { window.attachEvent("onresize", fnResize); }\n' \
                 '    else if (window.addEventListener)\n' \
                 '    { window.addEventListener("resize", fnResize, true); }\n' \
               % ( self._sId, );

        # clean up what the callbacks don't need.
        sHtml += '    oData = null;\n' \
                 '    aaaSeries = null;\n';

        # done;
        sHtml += '    return true;\n' \
                 '};\n';

        sHtml += '</script>\n' \
                 '</div>\n';
        return sHtml;


class WuiHlpLineGraphErrorbarY(WuiHlpLineGraph):
    """
    Line graph with an errorbar for the Y axis.
    """

    def __init__(self, sId, oData, oDisp = None):
        WuiHlpLineGraph.__init__(self, sId, oData, fErrorBarY = True);

