<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/classes">
  <xsl:for-each select="class">
<xsl:text>&#xa;</xsl:text>
***
#### <xsl:value-of select="@name"/>
<xsl:text>&#xa;</xsl:text>
<xsl:value-of select="description"/>
<xsl:text>&#xa;</xsl:text>
    <xsl:for-each select="attributes/attribute">
**<xsl:value-of select="@name"/>**, _<xsl:value-of select="@type"/>_, <xsl:value-of select="."/>
<xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="attributes/type">
**<xsl:value-of select="@name"/>**, <xsl:value-of select="."/>
<xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
<xsl:text>&#xa;</xsl:text>
  </xsl:for-each>
<xsl:text>&#xa;</xsl:text>
</xsl:template>

</xsl:stylesheet>

