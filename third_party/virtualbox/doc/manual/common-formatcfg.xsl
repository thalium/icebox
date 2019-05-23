<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- General formatting settings. -->
<xsl:variable name="section.autolabel">1</xsl:variable>
<xsl:variable name="section.label.includes.component.label">1</xsl:variable>
<xsl:attribute-set name="monospace.properties">
  <xsl:attribute name="font-size">90%</xsl:attribute>
</xsl:attribute-set>
<xsl:param name="draft.mode" select="'no'"/>

<!-- Shift down section sizes one magstep. -->
<xsl:attribute-set name="section.title.level1.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.728"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level2.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.44"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level3.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.2"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level4.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level5.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level6.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>

<!-- Shift down chapter font size one magstep. -->
<xsl:attribute-set name="component.title.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 2.0736"></xsl:value-of>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>

<!-- command synopsis -->
<xsl:variable name="arg.choice.opt.open.str">[</xsl:variable>
<xsl:variable name="arg.choice.opt.close.str">]</xsl:variable>
<xsl:variable name="arg.choice.req.open.str">&lt;</xsl:variable>
<xsl:variable name="arg.choice.req.close.str">&gt;</xsl:variable>
<xsl:variable name="arg.choice.plain.open.str"><xsl:text> </xsl:text></xsl:variable>
<xsl:variable name="arg.choice.plain.close.str"><xsl:text> </xsl:text></xsl:variable>
<xsl:variable name="arg.choice.def.open.str">[</xsl:variable>
<xsl:variable name="arg.choice.def.close.str">]</xsl:variable>
<xsl:variable name="arg.rep.repeat.str">...</xsl:variable>
<xsl:variable name="arg.rep.norepeat.str"></xsl:variable>
<xsl:variable name="arg.rep.def.str"></xsl:variable>
<xsl:variable name="arg.or.sep"> | </xsl:variable>
<xsl:variable name="cmdsynopsis.hanging.indent">4pi</xsl:variable>

<!--
  refentry related layout tweaks.

  Note! While we could save us all this work by using refsect1..3 and
        refsynopsisdiv docbook-refentry-to-manual-sect1.xsl, we'd like to have
        a valid XML document and thus do do some extra markup using the role
        and condition attributes.  We catch some of it here.  But the XSLT
        for specific targets (html, latex, etc) have a few more tweaks
        related to this.

        The @role has only one special trick 'not-in-toc' that excludes sections
        like 'Synopsis' and 'Description' from the TOCs.

        The @condition records the original refentry element name, i.e. it will
        have values like refentry, refsynopsisdiv, refsect1, refsect2 and refsect3.
  -->

<!-- This removes the not-in-toc bits from the toc. -->
<xsl:template match="sect2[@role = 'not-in-toc']"      mode="toc" />
<xsl:template match="sect3[@role = 'not-in-toc']"      mode="toc" />
<xsl:template match="sect4[@role = 'not-in-toc']"      mode="toc" />
<xsl:template match="sect5[@role = 'not-in-toc']"      mode="toc" />
<xsl:template match="section[@role = 'not-in-toc']"    mode="toc" />
<xsl:template match="simplesect[@role = 'not-in-toc']" mode="toc" />

<!-- This removes unnecessary <dd><dl> stuff caused by the above. -->
<xsl:template match="sect1[sect2/@role = 'not-in-toc']" mode="toc">
  <xsl:param name="toc-context" select="."/>
  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect2[@role != 'not-in-toc'] | bridgehead[$bridgehead.in.toc != 0]"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect2[sect3/@role = 'not-in-toc']" mode="toc">
  <xsl:param name="toc-context" select="."/>
  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect3[@role != 'not-in-toc'] | bridgehead[$bridgehead.in.toc != 0]"/>
  </xsl:call-template>
</xsl:template>

<!-- This make the refsect* and refsynopsisdiv unnumbered like the default refentry rendering. -->
<xsl:template match="sect2[@condition = 'refsynopsisdiv']
                   | sect2[starts-with(@condition, 'refsect')]
                   | sect3[starts-with(@condition, 'refsect')]
                   | sect4[starts-with(@condition, 'refsect')]
                   | sect5[starts-with(@condition, 'refsect')]
                   | section[starts-with(@condition, 'refsect')]
                   | simplesect[starts-with(@condition, 'refsect')]"
  mode="object.title.template"
  >
    <xsl:call-template name="gentext.template">
      <xsl:with-param name="context" select="'title-unnumbered'"/>
      <xsl:with-param name="name">
        <xsl:call-template name="xpath.location"/>
      </xsl:with-param>
    </xsl:call-template>
</xsl:template>


</xsl:stylesheet>
