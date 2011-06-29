<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
	
<html>
<head>
<title>Test report</title>
<style>

div {
  padding: 3px 10px 2px 10px;
}

.document {
  font-family: Arial;
  font-size: 11pt;
  background-color: #EEEEEE;
}
	
.description {
  font-style: italic;
}	
	
.title {
  font-weight: bold;
}
	
.aligner {
  width: 100px;
  float: left;
}
	
.statistics {
}

.ident {
  position: relative;
  left: 100px;
}	
	
</style>

</head>
<body class="document">
  <h1>Test Report</h1>
  <div>
    <span class="title">Start time: </span><xsl:value-of select="testlog/startTime"/><br/>
    <!-- and ended at <xsl:value-of select="testlog/endTime"/>.<br/>-->
    <span class="title">Total runtime: </span><xsl:value-of select="testlog/totalRuntime"/> seconds.<br/>
    <span class="title">Harness parameters: </span>
    <xsl:for-each select="testlog/parameters/parameter">
      <xsl:value-of select="."/>	 
    </xsl:for-each>
	<br/>
    <span class="title">Statistics:</span><br/>
    <div class="statistics">
      <span class="aligner">Suites: </span> <xsl:value-of select="testlog/numSuites"/> <br/>
      <span class="aligner">Tests in total: </span> <xsl:value-of select="testlog/numTests"/> <br/>
      <div>
        <span class="aligner">Passed tests: </span><xsl:value-of select="testlog/numPassedTests"/> <br/>
        <span class="aligner">Failed tests: </span><xsl:value-of select="testlog/numFailedTests"/> <br/>
      </div>
    </div>
  </div>

  <h3>Test results:</h3>
  <xsl:for-each select="testlog/suite">
    <div id="suite">
    Suite: <xsl:value-of select="name"/> (<xsl:value-of select="startTime"/>)
	<div id="suiteInfo">
		Tests: passed <xsl:value-of select="testsPassed"/>, failed <xsl:value-of select="testsFailed"/>, skipped <xsl:value-of select="testsSkipped"/>.<br/>
		Total runtime: <xsl:value-of select="totalRuntime"/> seconds. <br/>
		Show/hide tests.
		<div id="tests">
		    <xsl:for-each select="test">
			<div>
		      Name:  <xsl:value-of select="name"/> (<xsl:value-of select="startTime"/>  - <xsl:value-of select="endTime"/>  ) <br/>
		      Total runtime: <xsl:value-of select="totalRuntime"/>  <br/>
		      Result: <xsl:value-of select="result"/>  <br/>
		      Show/hide assert info <br/>
			  <div id="asserts">
		      <xsl:for-each select="assert"> 
			   <div id="assert">
		        Assert name: <xsl:value-of select="name"/> <br/>
		        Result: <xsl:value-of select="result"/> <br/>
		        Message <xsl:value-of select="message"/> <br/>
		        Time <xsl:value-of select="time"/> <br/>
		      </div>
		      </xsl:for-each>
		      AssertSummary:
		      Assert count: <xsl:value-of select="assertSummary/assertCount"/> <br/>
		      Asserts Passed <xsl:value-of select="assertSummary/assertsPassed"/> <br/>
		      Asserts Failed <xsl:value-of select="assertSummary/assertsFailed"/> <br/>
		    </div>
    		</div>
		    </xsl:for-each>
		</div>
	</div>
  </div>
  </xsl:for-each>
  <br/>
</body>
</html>

</xsl:template>
</xsl:stylesheet>

