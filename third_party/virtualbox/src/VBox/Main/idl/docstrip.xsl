<?xml version="1.0"?>

<!--
    Copies the source XIDL file to the output, except that all <desc>
    tags are stripped in the process. This is to generate a copy
    of VirtualBox.xidl which is then used as a source for generating
    the COM/XPCOM headers, the webservice files, the Qt bindings and
    others. The idea is that updating the documentation tags in the
    original XIDL should not cause a full recompile of nearly all of
    VirtualBox.

    Copyright (C) 2009-2016 Oracle Corporation

    This file is part of VirtualBox Open Source Edition (OSE), as
    available from http://www.virtualbox.org. This file is free software;
    you can redistribute it and/or modify it under the terms of the GNU
    General Public License (GPL) as published by the Free Software
    Foundation, in version 2 as it comes in the "COPYING" file of the
    VirtualBox OSE distribution. VirtualBox OSE is distributed in the
    hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" indent="yes"/>

<!-- copy everything unless there's a more specific template -->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<!-- swallow desc -->
<xsl:template match="desc" />

<!-- swallow all comments -->
<xsl:template match="comment()" />

</xsl:stylesheet>

