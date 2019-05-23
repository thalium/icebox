<?xml version="1.0"?>

<!--
    A template to generate a C header that will contain all result code
    definitions as entires of the const RTCOMERRMSG array (for use in the
    %Rhrc format specifier) as they are defined in the VirtualBox interface
    definition file (src/VBox/Main/idl/VirtualBox.xidl).

    Copyright (C) 2006-2017 Oracle Corporation

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
-->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
>
<xsl:output method="text"/>

<xsl:strip-space elements="*"/>

<!--
//  helpers
////////////////////////////////////////////////////////////////////////////////
-->

<!--
//  templates
////////////////////////////////////////////////////////////////////////////////
-->

<!--
 *  shut down all implicit templates
-->
<xsl:template match="*"/>

<xsl:template match="idl">
  <xsl:for-each select="library/application/result">
    <xsl:text>{ "</xsl:text>
    <xsl:choose>
      <xsl:when test="contains(normalize-space(desc/text()), '. ')">
        <xsl:value-of select="normalize-space(substring-before(desc/text(), '. '))"/>
      </xsl:when>
      <xsl:when test="contains(normalize-space(desc/text()), '.')">
        <xsl:value-of select="normalize-space(substring-before(desc/text(), '.'))"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="normalize-space(desc/text())"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>", "</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>", </xsl:text>
    <xsl:value-of select="@value"/>
    <xsl:text> },&#x0A;</xsl:text>
  </xsl:for-each>
</xsl:template>

<!--
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *  END
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-->

</xsl:stylesheet>

