<xsl:stylesheet version = '1.0'
     xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
     xmlns:vbox="http://www.virtualbox.org/"
     xmlns:exsl="http://exslt.org/common"
     extension-element-prefixes="exsl">

<!--

    comimpl.xsl:
        XSLT stylesheet that generates COM C++ classes implementing
        interfaces described in VirtualBox.xidl.
        For now we generate implementation for events, as they are
        rather trivial container classes for their read-only attributes.
        Further extension to other interfaces is possible and anticipated.

    Copyright (C) 2010-2016 Oracle Corporation

    This file is part of VirtualBox Open Source Edition (OSE), as
    available from http://www.virtualbox.org. This file is free software;
    you can redistribute it and/or modify it under the terms of the GNU
    General Public License (GPL) as published by the Free Software
    Foundation, in version 2 as it comes in the "COPYING" file of the
    VirtualBox OSE distribution. VirtualBox OSE is distributed in the
    hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
-->

<xsl:output
  method="text"
  version="1.0"
  encoding="utf-8"
  indent="no"/>

<xsl:include href="typemap-shared.inc.xsl" />

<!-- $G_kind contains what kind of COM class implementation we generate -->
<xsl:variable name="G_xsltFilename" select="'autogen.xsl'" />


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of types.
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyEnumsByName" match="//enum[@name]" use="@name"/>
<xsl:key name="G_keyInterfacesByName" match="//interface[@name]" use="@name"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template name="fileheader">
  <xsl:param name="name" />
  <xsl:text>/** @file </xsl:text>
  <xsl:value-of select="$name"/>
  <xsl:text>
 * DO NOT EDIT! This is a generated file.
 * Generated from: src/VBox/Main/idl/VirtualBox.xidl (VirtualBox's interface definitions in XML)
 * Generator: src/VBox/Main/idl/comimpl.xsl
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

</xsl:text>
</xsl:template>

<xsl:template name="genComEntry">
  <xsl:param name="name" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY(', $name, ')&#10;')" />
  <xsl:choose>
    <xsl:when test="$extends='$unknown'">
      <!-- Reached base -->
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genComEntry">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $extends)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="typeIdl2Back">
  <xsl:param name="type" />
  <xsl:param name="safearray" />
  <xsl:param name="param" />
  <xsl:param name="dir" />
  <xsl:param name="mod" />

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$type" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="dir" select="'in'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$param and ($dir='in')">
          <xsl:value-of select="concat('ComSafeArrayIn(',$elemtype,',', $param,')')"/>
        </xsl:when>
        <xsl:when test="$param and ($dir='out')">
          <xsl:value-of select="concat('ComSafeArrayOut(',$elemtype,', ', $param, ')')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('com::SafeArray&lt;',$elemtype,'&gt;')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$mod='ptr'">
          <xsl:value-of select="'BYTE*'" />
        </xsl:when>
        <xsl:when test="(($type='wstring') or ($type='uuid'))">
          <xsl:choose>
            <xsl:when test="$param and ($dir='in')">
              <xsl:value-of select="'CBSTR'"/>
            </xsl:when>
            <xsl:when test="$param and ($dir='out')">
              <xsl:value-of select="'BSTR'"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="'Bstr'"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="count(key('G_keyEnumsByName', $type)) > 0">
          <xsl:value-of select="concat($type,'_T')"/>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:choose>
            <xsl:when test="$param">
              <xsl:value-of select="concat($type,'*')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat('ComPtr&lt;',$type,'&gt;')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$type='boolean'">
          <xsl:value-of select="'BOOL'" />
        </xsl:when>
         <xsl:when test="$type='octet'">
          <xsl:value-of select="'BYTE'" />
        </xsl:when>
        <xsl:when test="$type='unsigned short'">
          <xsl:value-of select="'USHORT'" />
        </xsl:when>
        <xsl:when test="$type='short'">
          <xsl:value-of select="'SHORT'" />
        </xsl:when>
        <xsl:when test="$type='unsigned long'">
          <xsl:value-of select="'ULONG'" />
        </xsl:when>
        <xsl:when test="$type='long'">
          <xsl:value-of select="'LONG'" />
        </xsl:when>
        <xsl:when test="$type='unsigned long long'">
          <xsl:value-of select="'ULONG64'" />
        </xsl:when>
        <xsl:when test="$type='long long'">
          <xsl:value-of select="'LONG64'" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="fatalError">
            <xsl:with-param name="msg" select="concat('Unhandled type: ', $type)" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="$dir='out'">
        <xsl:value-of select="'*'"/>
      </xsl:if>
      <xsl:if test="$param and not($param='_')">
        <xsl:value-of select="concat(' ', $param)"/>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>

</xsl:template>


<xsl:template name="genSetParam">
  <xsl:param name="member"/>
  <xsl:param name="param"/>
  <xsl:param name="type"/>
  <xsl:param name="safearray"/>

  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
          <xsl:with-param name="type" select="$type" />
          <xsl:with-param name="safearray" select="''" />
          <xsl:with-param name="dir" select="'in'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('         SafeArray&lt;', $elemtype, '&gt; aArr(ComSafeArrayInArg(',$param,'));&#10;')"/>
      <xsl:value-of select="concat('         ',$member, '.initFrom(aArr);&#10;')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat('         ', $member, ' = ', $param, ';&#10;')"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genRetParam">
  <xsl:param name="member"/>
  <xsl:param name="param"/>
  <xsl:param name="type"/>
  <xsl:param name="safearray"/>
  <xsl:choose>
    <xsl:when test="$safearray='yes'">
      <xsl:variable name="elemtype">
        <xsl:call-template name="typeIdl2Back">
             <xsl:with-param name="type" select="$type" />
             <xsl:with-param name="safearray" select="''" />
             <xsl:with-param name="dir" select="'in'" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat('         SafeArray&lt;', $elemtype,'&gt; result;&#10;')"/>
      <xsl:value-of select="concat('         ', $member, '.cloneTo(result);&#10;')"/>
      <xsl:value-of select="concat('         result.detachTo(ComSafeArrayOutArg(', $param, '));&#10;')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="($type='wstring') or ($type = 'uuid')">
          <xsl:value-of select="concat('         ', $member, '.cloneTo(', $param, ');&#10;')"/>
        </xsl:when>
        <xsl:when test="count(key('G_keyInterfacesByName', $type)) > 0">
          <xsl:value-of select="concat('         ', $member, '.queryInterfaceTo(', $param, ');&#10;')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('         *', $param, ' = ', $member, ';&#10;')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genAttrInitCode">
  <xsl:param name="name" />
  <xsl:param name="obj" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genAttrInitCode">
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="obj" select="$obj" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:variable name="aTypeName">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$aName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="aType">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="'_'" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="@safearray='yes'">
         <xsl:variable name="elemtype">
           <xsl:call-template name="typeIdl2Back">
             <xsl:with-param name="type" select="@type" />
             <xsl:with-param name="safearray" select="''" />
             <xsl:with-param name="dir" select="'in'" />
           </xsl:call-template>
         </xsl:variable>
         <xsl:value-of select="       '#ifdef RT_OS_WINDOWS&#10;'"/>
         <xsl:value-of select="concat('              SAFEARRAY *aPtr_', @name, ' = va_arg(args, SAFEARRAY *);&#10;')"/>
         <xsl:value-of select="concat('              com::SafeArray&lt;', $elemtype,'&gt;   aArr_', @name, '(aPtr_', @name, ');&#10;')"/>
         <xsl:value-of select="       '#else&#10;'"/>
         <xsl:value-of select="concat('              PRUint32 aArrSize_', @name, ' = va_arg(args, PRUint32);&#10;')"/>
         <xsl:value-of select="concat('              void*    aPtr_', @name, ' = va_arg(args, void*);&#10;')"/>
         <xsl:value-of select="concat('              com::SafeArray&lt;', $elemtype,'&gt;   aArr_', @name, '(aArrSize_', @name, ', (', $elemtype,'*)aPtr_', @name, ');&#10;')"/>
         <xsl:value-of select="       '#endif&#10;'"/>
         <xsl:value-of select="concat('              ',$obj, '->set_', @name, '(ComSafeArrayAsInParam(aArr_', @name, '));&#10;')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('              ',$aTypeName, ' = va_arg(args, ',$aType,');&#10;')"/>
        <xsl:value-of select="concat('              ',$obj, '->set_', @name, '(',$aName, ');&#10;')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genImplList">
  <xsl:param name="impl" />
  <xsl:param name="name" />
  <xsl:param name="depth" />
  <xsl:param name="parents" />

  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$name='IEvent'">
      <xsl:value-of select="       '#ifdef VBOX_WITH_XPCOM&#10;'" />
      <xsl:value-of select="concat('NS_DECL_CLASSINFO(', $impl, ')&#10;')" />
      <xsl:value-of select="concat('NS_IMPL_THREADSAFE_ISUPPORTS',$depth,'_CI(', $impl, $parents, ', IEvent)&#10;')" />
      <xsl:value-of select="       '#endif&#10;&#10;'"/>
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genImplList">
        <xsl:with-param name="impl" select="$impl" />
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="depth" select="$depth+1" />
        <xsl:with-param name="parents" select="concat($parents, ', ', $name)" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genAttrCode">
  <xsl:param name="name" />
  <xsl:param name="depth" />
  <xsl:param name="parents" />

  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute">
    <xsl:variable name="mName">
      <xsl:value-of select="concat('m_', @name)" />
    </xsl:variable>
    <xsl:variable name="mType">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="pName">
      <xsl:value-of select="concat('a_', @name)" />
    </xsl:variable>
    <xsl:variable name="pTypeNameOut">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="dir" select="'out'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="pTypeNameIn">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="capsName">
      <xsl:call-template name="capitalize">
        <xsl:with-param name="str" select="@name" />
      </xsl:call-template>
    </xsl:variable>

    <xsl:value-of select="       '&#10;'" />
    <xsl:value-of select="concat('    // attribute ', @name,'&#10;')" />
    <xsl:value-of select="       'private:&#10;'" />
    <xsl:value-of select="concat('    ', $mType, '    ', $mName,';&#10;')" />
    <xsl:value-of select="       'public:&#10;'" />
    <xsl:value-of select="concat('    STDMETHOD(COMGETTER(', $capsName,'))(',$pTypeNameOut,')&#10;    {&#10;')" />
    <xsl:call-template name="genRetParam">
      <xsl:with-param name="type" select="@type" />
      <xsl:with-param name="member" select="$mName" />
      <xsl:with-param name="param" select="$pName" />
      <xsl:with-param name="safearray" select="@safearray" />
    </xsl:call-template>
    <xsl:value-of select="       '         return S_OK;&#10;'" />
    <xsl:value-of select="       '    }&#10;'" />

    <xsl:if test="not(@readonly='yes')">
      <xsl:value-of select="concat('    STDMETHOD(COMSETTER(', $capsName,'))(',$pTypeNameIn,')&#10;    {&#10;')" />
      <xsl:call-template name="genSetParam">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="member" select="$mName" />
        <xsl:with-param name="param" select="$pName" />
        <xsl:with-param name="safearray" select="@safearray" />
      </xsl:call-template>
      <xsl:value-of select="       '         return S_OK;&#10;'" />
      <xsl:value-of select="       '    }&#10;'" />
    </xsl:if>

    <xsl:value-of select="       '    // purely internal setter&#10;'" />
    <xsl:value-of select="concat('    HRESULT set_', @name,'(',$pTypeNameIn, ')&#10;    {&#10;')" />
    <xsl:call-template name="genSetParam">
      <xsl:with-param name="type" select="@type" />
      <xsl:with-param name="member" select="$mName" />
      <xsl:with-param name="param" select="$pName" />
      <xsl:with-param name="safearray" select="@safearray" />
    </xsl:call-template>
    <xsl:value-of select="       '         return S_OK;&#10;'" />
    <xsl:value-of select="       '    }&#10;'" />
  </xsl:for-each>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
      <xsl:value-of select="   '    // skipping IEvent attributes &#10;'" />
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
      <xsl:value-of select="   '    // skipping IReusableEvent attributes &#10;'" />
    </xsl:when>
     <xsl:when test="$extends='IVetoEvent'">
      <xsl:value-of select="   '    // skipping IVetoEvent attributes &#10;'" />
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genAttrCode">
        <xsl:with-param name="name" select="$extends" />
        <xsl:with-param name="depth" select="$depth+1" />
        <xsl:with-param name="parents" select="concat($parents, ', ', @name)" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $extends)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="genEventImpl">
  <xsl:param name="implName" />
  <xsl:param name="isVeto" />
  <xsl:param name="isReusable" />

  <xsl:value-of select="concat('class ATL_NO_VTABLE ',$implName,
                        '&#10;    : public VirtualBoxBase,&#10;      VBOX_SCRIPTABLE_IMPL(',
                        @name, ')&#10;{&#10;')" />
  <xsl:value-of select="'public:&#10;'" />
  <xsl:value-of select="concat('    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(', $implName, ', ', @name, ')&#10;')" />
  <xsl:value-of select="concat('    DECLARE_NOT_AGGREGATABLE(', $implName, ')&#10;')" />
  <xsl:value-of select="       '    DECLARE_PROTECT_FINAL_CONSTRUCT()&#10;'" />
  <xsl:value-of select="concat('    BEGIN_COM_MAP(', $implName, ')&#10;')" />
  <xsl:value-of select="       '        COM_INTERFACE_ENTRY(ISupportErrorInfo)&#10;'" />
  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY(', @name, ')&#10;')" />
  <xsl:value-of select="concat('        COM_INTERFACE_ENTRY2(IDispatch, ', @name, ')&#10;')" />
  <xsl:value-of select="concat('        VBOX_TWEAK_INTERFACE_ENTRY(', @name, ')&#10;')" />

  <xsl:call-template name="genComEntry">
    <xsl:with-param name="name" select="@name" />
  </xsl:call-template>
  <xsl:value-of select="       '    END_COM_MAP()&#10;'" />
  <xsl:value-of select="concat('    ',$implName,'() { /*printf(&quot;',$implName,'\n&quot;)*/;}&#10;')" />
  <xsl:value-of select="concat('    virtual ~',$implName,'() { /*printf(&quot;~',$implName,'\n&quot;)*/; uninit(); }&#10;')" />
  <xsl:text><![CDATA[
    HRESULT FinalConstruct()
    {
        BaseFinalConstruct();
        return mEvent.createObject();
    }
    void FinalRelease()
    {
        uninit();
        BaseFinalRelease();
    }
    STDMETHOD(COMGETTER(Type))(VBoxEventType_T *aType)
    {
        return mEvent->COMGETTER(Type)(aType);
    }
    STDMETHOD(COMGETTER(Source))(IEventSource * *aSource)
    {
        return mEvent->COMGETTER(Source)(aSource);
    }
    STDMETHOD(COMGETTER(Waitable))(BOOL *aWaitable)
    {
        return mEvent->COMGETTER(Waitable)(aWaitable);
    }
    STDMETHOD(SetProcessed)()
    {
       return mEvent->SetProcessed();
    }
    STDMETHOD(WaitProcessed)(LONG aTimeout, BOOL *aResult)
    {
        return mEvent->WaitProcessed(aTimeout, aResult);
    }
    void uninit()
    {
        if (!mEvent.isNull())
        {
           mEvent->uninit();
           mEvent.setNull();
        }
    }
]]></xsl:text>
  <xsl:choose>
    <xsl:when test="$isVeto='yes'">
<xsl:text><![CDATA[
    HRESULT init(IEventSource* aSource, VBoxEventType_T aType, BOOL aWaitable = TRUE)
    {
        NOREF(aWaitable);
        return mEvent->init(aSource, aType);
    }
    STDMETHOD(AddVeto)(IN_BSTR aVeto)
    {
        return mEvent->AddVeto(aVeto);
    }
    STDMETHOD(IsVetoed)(BOOL *aResult)
    {
       return mEvent->IsVetoed(aResult);
    }
    STDMETHOD(GetVetos)(ComSafeArrayOut(BSTR, aVetos))
    {
       return mEvent->GetVetos(ComSafeArrayOutArg(aVetos));
    }
    STDMETHOD(AddApproval)(IN_BSTR aReason)
    {
        return mEvent->AddApproval(aReason);
    }
    STDMETHOD(IsApproved)(BOOL *aResult)
    {
       return mEvent->IsApproved(aResult);
    }
    STDMETHOD(GetApprovals)(ComSafeArrayOut(BSTR, aReasons))
    {
       return mEvent->GetApprovals(ComSafeArrayOutArg(aReasons));
    }
private:
    ComObjPtr<VBoxVetoEvent>      mEvent;
]]></xsl:text>
    </xsl:when>
    <xsl:when test="$isReusable='yes'">
      <xsl:text>
<![CDATA[
    HRESULT init(IEventSource* aSource, VBoxEventType_T aType, BOOL aWaitable = FALSE)
    {
        mGeneration = 1;
        return mEvent->init(aSource, aType, aWaitable);
    }
    STDMETHOD(COMGETTER(Generation))(ULONG *aGeneration)
    {
      *aGeneration = mGeneration;
      return S_OK;
    }
    STDMETHOD(Reuse)()
    {
       ASMAtomicIncU32((volatile uint32_t*)&mGeneration);
       return S_OK;
    }
private:
    volatile ULONG              mGeneration;
    ComObjPtr<VBoxEvent>        mEvent;
]]></xsl:text>
    </xsl:when>
    <xsl:otherwise>
<xsl:text><![CDATA[
    HRESULT init(IEventSource* aSource, VBoxEventType_T aType, BOOL aWaitable)
    {
        return mEvent->init(aSource, aType, aWaitable);
    }
private:
    ComObjPtr<VBoxEvent>      mEvent;
]]></xsl:text>
    </xsl:otherwise>
  </xsl:choose>

  <!-- Before we generate attribute code, we check and make sure there are attributes here. -->
  <xsl:if test="count(attribute) = 0 and @name != 'INATNetworkAlterEvent'">
    <xsl:call-template name="fatalError">
      <xsl:with-param name="msg">error: <xsl:value-of select="@name"/> has no attributes</xsl:with-param>
    </xsl:call-template>
  </xsl:if>

  <xsl:call-template name="genAttrCode">
    <xsl:with-param name="name" select="@name" />
  </xsl:call-template>
  <xsl:value-of select="'};&#10;'" />


  <xsl:call-template name="genImplList">
    <xsl:with-param name="impl" select="$implName" />
    <xsl:with-param name="name" select="@name" />
    <xsl:with-param name="depth" select="'1'" />
    <xsl:with-param name="parents" select="''" />
  </xsl:call-template>

</xsl:template>


<xsl:template name="genSwitchCase">
  <xsl:param name="ifaceName" />
  <xsl:param name="implName" />
  <xsl:param name="reinit" />
  <xsl:variable name="waitable">
    <xsl:choose>
      <xsl:when test="@waitable='yes'">
        <xsl:value-of select="'TRUE'"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="'FALSE'"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:value-of select="concat('         case VBoxEventType_', @id, ':&#10;')"/>
  <xsl:value-of select="       '         {&#10;'"/>
  <xsl:choose>
    <xsl:when test="$reinit='yes'">
      <xsl:value-of select="concat('              ComPtr&lt;', $ifaceName, '&gt; iobj;&#10;')"/>
      <xsl:value-of select="       '              iobj = mEvent;&#10;'"/>
      <xsl:value-of select="       '              Assert(!iobj.isNull());&#10;'"/>
      <xsl:value-of select="concat('              ',$implName, '* obj = (', $implName, '*)(', $ifaceName, '*)iobj;&#10;')"/>
      <xsl:value-of select="       '              obj->Reuse();&#10;'"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="concat('              ComObjPtr&lt;', $implName, '&gt; obj;&#10;')"/>
      <xsl:value-of select="       '              obj.createObject();&#10;'"/>
      <xsl:value-of select="concat('              obj->init(aSource, aType, ', $waitable, ');&#10;')"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:call-template name="genAttrInitCode">
    <xsl:with-param name="name" select="@name" />
    <xsl:with-param name="obj" select="'obj'" />
  </xsl:call-template>
  <xsl:if test="not($reinit='yes')">
    <xsl:value-of select="       '              obj.queryInterfaceTo(mEvent.asOutParam());&#10;'"/>
  </xsl:if>
  <xsl:value-of select="       '              break;&#10;'"/>
  <xsl:value-of select="       '         }&#10;'"/>
</xsl:template>

<xsl:template name="genCommonEventCode">
  <xsl:call-template name="fileheader">
    <xsl:with-param name="name" select="'VBoxEvents.cpp'" />
  </xsl:call-template>

<xsl:text><![CDATA[
#include <VBox/com/array.h>
#include <iprt/asm.h>
#include "EventImpl.h"
]]></xsl:text>

  <!-- Interfaces -->
  <xsl:for-each select="//interface[@autogen=$G_kind]">
    <xsl:value-of select="concat('// ', @name,  ' implementation code')" />
    <xsl:call-template name="xsltprocNewlineOutputHack"/>
    <xsl:variable name="implName">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$G_kind='VBoxEvent'">
        <xsl:variable name="isVeto">
          <xsl:if test="@extends='IVetoEvent'">
            <xsl:value-of select="'yes'" />
          </xsl:if>
        </xsl:variable>
        <xsl:variable name="isReusable">
          <xsl:if test="@extends='IReusableEvent'">
            <xsl:value-of select="'yes'" />
          </xsl:if>
        </xsl:variable>
        <xsl:call-template name="genEventImpl">
          <xsl:with-param name="implName" select="$implName" />
          <xsl:with-param name="isVeto" select="$isVeto" />
          <xsl:with-param name="isReusable" select="$isReusable" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:for-each>

  <xsl:text><![CDATA[
HRESULT VBoxEventDesc::init(IEventSource *aSource, VBoxEventType_T aType, ...)
{
    va_list args;

    mEventSource = aSource;
    va_start(args, aType);
    switch (aType)
    {
]]></xsl:text>

  <xsl:for-each select="//interface[@autogen=$G_kind]">
    <xsl:variable name="implName">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>
    <xsl:call-template name="genSwitchCase">
      <xsl:with-param name="ifaceName" select="@name" />
      <xsl:with-param name="implName" select="$implName" />
      <xsl:with-param name="reinit" select="'no'" />
    </xsl:call-template>
  </xsl:for-each>

  <xsl:text><![CDATA[
         default:
            AssertFailed();
    }
    va_end(args);

    return S_OK;
}
]]></xsl:text>

 <xsl:text><![CDATA[
HRESULT VBoxEventDesc::reinit(VBoxEventType_T aType, ...)
{
    va_list args;

    va_start(args, aType);
    switch (aType)
    {
]]></xsl:text>

  <xsl:for-each select="//interface[@autogen=$G_kind and @extends='IReusableEvent']">
    <xsl:variable name="implName">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>
    <xsl:call-template name="genSwitchCase">
      <xsl:with-param name="ifaceName" select="@name" />
      <xsl:with-param name="implName" select="$implName" />
      <xsl:with-param name="reinit" select="'yes'" />
    </xsl:call-template>
  </xsl:for-each>

  <xsl:text><![CDATA[
         default:
            AssertFailed();
    }
    va_end(args);

    return S_OK;
}
]]></xsl:text>

</xsl:template>

<xsl:template name="genFormalParams">
  <xsl:param name="name" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genFormalParams">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:variable name="aTypeName">
      <xsl:call-template name="typeIdl2Back">
        <xsl:with-param name="type" select="@type" />
        <xsl:with-param name="safearray" select="@safearray" />
        <xsl:with-param name="param" select="$aName" />
        <xsl:with-param name="dir" select="'in'" />
        <xsl:with-param name="mod" select="@mod" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat(', ',$aTypeName)"/>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genFactParams">
  <xsl:param name="name" />
  <xsl:variable name="extends">
    <xsl:value-of select="key('G_keyInterfacesByName', $name)/@extends" />
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$extends='IEvent'">
    </xsl:when>
    <xsl:when test="$extends='IReusableEvent'">
    </xsl:when>
    <xsl:when test="count(key('G_keyInterfacesByName', $extends)) > 0">
      <xsl:call-template name="genFactParams">
        <xsl:with-param name="name" select="$extends" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('No idea how to process it: ', $name)" />
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="key('G_keyInterfacesByName', $name)/attribute[@name != 'midlDoesNotLikeEmptyInterfaces']">
    <xsl:variable name="aName" select="concat('a_',@name)"/>
    <xsl:choose>
      <xsl:when test="@safearray='yes'">
        <xsl:value-of select="concat(', ComSafeArrayInArg(',$aName,')')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat(', ',$aName)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<xsl:template name="genCommonEventHeader">
  <xsl:call-template name="fileheader">
    <xsl:with-param name="name" select="'VBoxEvents.h'" />
  </xsl:call-template>

<xsl:text><![CDATA[
#include "EventImpl.h"
]]></xsl:text>

  <!-- Interfaces -->
  <xsl:for-each select="//interface[@autogen='VBoxEvent']">
    <xsl:value-of select="concat('// ', @name,  ' generation routine &#10;')" />
    <xsl:variable name="evname">
      <xsl:value-of select="substring(@name, 2)" />
    </xsl:variable>
    <xsl:variable name="evid">
      <xsl:value-of select="concat('On', substring(@name, 2, string-length(@name)-6))" />
    </xsl:variable>

    <xsl:variable name="ifname">
      <xsl:value-of select="@name" />
    </xsl:variable>

    <xsl:value-of select="concat('DECLINLINE(void) fire', $evname, '(IEventSource* aSource')"/>
    <xsl:call-template name="genFormalParams">
      <xsl:with-param name="name" select="$ifname" />
    </xsl:call-template>
    <xsl:value-of select="       ')&#10;{&#10;'"/>

    <xsl:value-of select="       '    VBoxEventDesc evDesc;&#10;'"/>
    <xsl:value-of select="concat('    evDesc.init(aSource, VBoxEventType_',$evid)"/>
    <xsl:call-template name="genFactParams">
      <xsl:with-param name="name" select="$ifname" />
    </xsl:call-template>
    <xsl:value-of select="');&#10;'"/>
    <xsl:value-of select="       '    evDesc.fire(/* do not wait for delivery */ 0);&#10;'"/>
    <xsl:value-of select="       '}&#10;'"/>
  </xsl:for-each>
</xsl:template>

<xsl:template match="/">
  <!-- Global code -->
   <xsl:choose>
      <xsl:when test="$G_kind='VBoxEvent'">
        <xsl:call-template name="genCommonEventCode">
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$G_kind='VBoxEventHeader'">
        <xsl:call-template name="genCommonEventHeader">
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="fatalError">
          <xsl:with-param name="msg" select="concat('Request unsupported: ', $G_kind)" />
        </xsl:call-template>
      </xsl:otherwise>
   </xsl:choose>
</xsl:template>

</xsl:stylesheet>
