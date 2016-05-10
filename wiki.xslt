<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template name="escape_underscore">
	<xsl:param name="text" />
	<xsl:choose>
		<xsl:when test="contains($text, '_')">
			<xsl:value-of select="substring-before($text,'_')" />
			<xsl:value-of select="'\_'" />
			<xsl:call-template name="escape_underscore">
				<xsl:with-param name="text" select="substring-after($text,'_')" />
			</xsl:call-template>
		</xsl:when>
		<xsl:otherwise>
			<xsl:value-of select="$text" />
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template match="/classes">
<xsl:for-each select="class">
<xsl:text>&#xa;</xsl:text>
<xsl:variable name="class-name">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="@name" />
</xsl:call-template>
</xsl:variable>
<xsl:variable name="class-desc">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="description" />
</xsl:call-template>
</xsl:variable>
<xsl:if test="attributes/attribute">
***
#### <xsl:value-of select="$class-name"/>
<xsl:text>&#xa;</xsl:text>
<xsl:value-of select="$class-desc"/>
<xsl:text>&#xa;</xsl:text>
</xsl:if>
<xsl:for-each select="attributes/attribute">
<xsl:variable name="attr-name">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="@name" />
</xsl:call-template>
</xsl:variable>
<xsl:variable name="attr-type">
<!-- Don't need to escape this because we put it in code tags -->
<xsl:value-of select="@type"/>
</xsl:variable>
<xsl:variable name="attr-vars">
<!-- Don't need to escape this because we put it in code tags -->
<xsl:value-of select="@vars"/>
</xsl:variable>
<xsl:variable name="attr-desc">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="." />
</xsl:call-template>
</xsl:variable>
**<xsl:value-of select="$attr-name"/>** | `<xsl:value-of select="$attr-type"/><xsl:if test="$attr-vars != ''"> : <xsl:value-of select="$attr-vars"/></xsl:if>` | <xsl:value-of select="$attr-desc"/>
<xsl:text>&#xa;</xsl:text>
</xsl:for-each>
<xsl:text>&#xa;</xsl:text>
</xsl:for-each>
***
### Predefined Strings
<xsl:text>&#xa;</xsl:text>
Below are some strings that are used in various attributes.
<xsl:for-each select="class">
<xsl:text>&#xa;</xsl:text>
<xsl:variable name="class-name">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="@name" />
</xsl:call-template>
</xsl:variable>
<xsl:variable name="class-desc">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="description" />
</xsl:call-template>
</xsl:variable>
<xsl:if test="attributes/type">
***
#### <xsl:value-of select="$class-name"/>
<xsl:text>&#xa;</xsl:text>
<xsl:value-of select="$class-desc"/>
<xsl:text>&#xa;</xsl:text>
</xsl:if>
<xsl:for-each select="attributes/type">
<xsl:variable name="type-name">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="@name" />
</xsl:call-template>
</xsl:variable>
<xsl:variable name="type-desc">
<xsl:call-template name="escape_underscore">
<xsl:with-param name="text" select="." />
</xsl:call-template>
</xsl:variable>
**<xsl:value-of select="$type-name"/>** | <xsl:value-of select="$type-desc"/>
<xsl:text>&#xa;</xsl:text>
</xsl:for-each>
<xsl:text>&#xa;</xsl:text>
</xsl:for-each>
<xsl:text>&#xa;</xsl:text>
</xsl:template>

</xsl:stylesheet>

