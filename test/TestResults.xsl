<?xml version='1.0' ?>
<!--
    (C) 2010 by Argonne National Laboratory.
        See COPYRIGHT in top-level directory.
    (C)Copyright IBM Corp.  2010

     Thanks to Joe Ratterman @ IBM for providing many improvements to
     this style sheet.
  -->
<!-- <xsl:stylesheet  xmlns:xsl="http://www.w3.org/TR/WD-xsl"> -->
<xsl:stylesheet  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!-- <xsl:output method="html"/>-->

<xsl:template match="/">
<html>
    <head>
        <title>MPICH2 Error Report</title>
        <style type="text/css">
            table      { border-collapse:collapse; }
            th, td     { border:2px solid blue; padding:2px; }
            td         { vertical-align:top; }
            th         { background-color:#bbf; color:white; }
            tr.fail td { background-color:#fbb; }
            tr.pass td { background-color:#bfb; }
        </style>
    </head>
    <body>
        <h1>MPICH2 Error Report</h1>
        <xsl:apply-templates select="MPITESTRESULTS"/>
    </body>
</html>
</xsl:template>

<xsl:template match="MPITESTRESULTS">
    <p>
        <xsl:choose>
            <xsl:when test="count(MPITEST[STATUS ='fail']) = 0">
                No failures
            </xsl:when>
            <xsl:otherwise>
                <span style="color: red"><xsl:value-of select="count(MPITEST[STATUS ='fail'])"/> tests failed</span>
            </xsl:otherwise>
        </xsl:choose>
        out of <xsl:value-of select="count(MPITEST)"/> tests run.
        <xsl:apply-templates select="DATE"/>
        <xsl:apply-templates select="TOTALTIME"/>.
    </p>

    <table>
        <tr>
            <th>Dir</th>
            <th>Name</th>
            <th>np</th>
            <th>Status</th>
            <xsl:choose>
                <xsl:when test="count(MPITEST/RUNTIME) > 0">
                    <th>Time</th>
                </xsl:when>
            </xsl:choose>
            <th>Diff</th>
        </tr>
        <xsl:apply-templates select="MPITEST"/>
    </table>
</xsl:template>

<xsl:template match="DATE">The tests started at <xsl:value-of select="."/></xsl:template>
<xsl:template match="TOTALTIME"> and ran for <xsl:value-of select=". div 60"/> minutes</xsl:template>

<xsl:template match="MPITEST">
    <xsl:variable name="status">
        <xsl:choose>
            <xsl:when test="STATUS = 'pass'">
                <xsl:value-of select="'pass'"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="'fail'"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>

    <tr class="{$status}">
    <td><xsl:value-of select="WORKDIR"/></td>
    <td><xsl:value-of select="NAME"/></td>
    <td><xsl:value-of select="NP"/></td>
    <td><xsl:value-of select="STATUS"/></td>
    <xsl:choose>
        <xsl:when test="RUNTIME">
            <td><xsl:value-of select="RUNTIME"/></td>
        </xsl:when>
    </xsl:choose>
    <td><pre><xsl:value-of select="TESTDIFF"/></pre></td>
    </tr>
</xsl:template>

<xsl:template match="TRACEBACK">
    <a>
    <xsl:attribute name="HREF">
    <xsl:value-of select="."/>
    </xsl:attribute>
    Traceback
    </a>
</xsl:template>


</xsl:stylesheet>
