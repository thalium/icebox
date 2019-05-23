<?xml version="1.0"?>
<!--
    docbook-refentry-to-manpage-preprocessing.xsl:
        XSLT stylesheet preprocessing remarks elements before
        turning a refentry (manpage) into a unix manual page and
        VBoxManage built-in help.

    Copyright (C) 2006-2015 Oracle Corporation

    This file is part of VirtualBox Open Source Edition (OSE), as
    available from http://www.virtualbox.org. This file is free software;
    you can redistribute it and/or modify it under the terms of the GNU
    General Public License (GPL) as published by the Free Software
    Foundation, in version 2 as it comes in the "COPYING" file of the
    VirtualBox OSE distribution. VirtualBox OSE is distributed in the
    hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
-->

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:str="http://xsltsl.org/string"
  >

  <xsl:output method="xml" version="1.0" encoding="utf-8" indent="no"/>

  <!--
    The default action is to copy everything.
    -->
  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <!--
    Execute synopsis copy remark (avoids duplication for complicated xml).
    We strip the attributes off it.
    -->
  <xsl:template match="remark[@role = 'help-copy-synopsis']">
    <xsl:choose>
      <xsl:when test="parent::refsect2"/>
      <xsl:otherwise>
        <xsl:message terminate="yes">Misplaced remark/@role=help-copy-synopsis element.
Only supported on: refsect2</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:variable name="sSrcId">
      <xsl:choose>
        <xsl:when test="@condition"><xsl:value-of select="concat('synopsis-', @condition)"/></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('synopsis-', ../@id)"/></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="CmdSynopsis" select="/refentry/refsynopsisdiv/cmdsynopsis[@id = $sSrcId]"/>
    <xsl:if test="not($CmdSynopsis)">
      <xsl:message terminate="yes">Could not find any cmdsynopsis with id=<xsl:value-of select="$sSrcId"/> in refsynopsisdiv.</xsl:message>
    </xsl:if>

    <xsl:element name="cmdsynopsis">
      <xsl:apply-templates select="$CmdSynopsis/node()"/>
    </xsl:element>

  </xsl:template>

  <!--
    Remove bits only intended for the manual.
    -->
  <xsl:template match="remark[@role='help-manual']"/>

</xsl:stylesheet>

