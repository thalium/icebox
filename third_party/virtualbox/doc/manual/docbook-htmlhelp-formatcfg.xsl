<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="@VBOX_PATH_DOCBOOK@/htmlhelp/htmlhelp.xsl"/>
<xsl:import href="@VBOX_PATH_MANUAL_SRC@/common-formatcfg.xsl"/>
<xsl:import href="@VBOX_PATH_MANUAL_SRC@/common-html-formatcfg.xsl"/>

<xsl:include href="@VBOX_PATH_MANUAL_OUT_LANG@/titlepage-htmlhelp.xsl"/>

<!-- Override the style sheet stuff from common-html-formatcfg.xsl, we don't
     the same as the html-chunks and html-one-page.  Also, the microsoft
     help viewer may have limited CSS support, depending on which browser
     version it emulated, so keep it simple. -->
<xsl:template name="user.head.content">
 <style type="text/css">
  <xsl:comment>
   .cmdsynopsis p
   {
     padding-left: 3.4em;
     text-indent: -2.2em;
   }
   p.nextcommand
   {
     margin-top:    0px;
     margin-bottom: 0px;
   }
   p.lastcommand
   {
     margin-top:    0px;
   }
  </xsl:comment>
 </style>
</xsl:template>


<!-- for some reason, the default docbook stuff doesn't wrap simple <arg> elements
     into HTML <code>, so with a default CSS a cmdsynopsis ends up with a mix of
     monospace and proportional fonts.  Elsewhere we hack that in the CSS, here
     that turned out to be harded, so we just wrap things in <code>, risking
     nested <code> elements, but who cares as long as it works... -->
<xsl:template match="group|arg">
    <xsl:choose>
        <xsl:when test="name(..) = 'arg' or name(..) = 'group'">
            <xsl:apply-imports/>
        </xsl:when>
        <xsl:otherwise>
            <code><xsl:apply-imports/></code>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

</xsl:stylesheet>

