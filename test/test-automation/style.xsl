<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
	
<html>
<head>
<title>Test report</title>

<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js"/>

<script type="text/javascript">

$(document).ready(function() {
	$("span.show-tests").click(function() {
		var content = $(this).html();
		var id = $(this).attr('uid');
	
		var searchString = "div.tests[uid="+id+"]";
	
		if(content == '[Hide tests]') {
			$(searchString).hide("fast");
			$(this).text('[Show tests]');
		} else {
			$(searchString).show("fast");
			$(this).text('[Hide tests]');
		}
	});

	$("span.show-asserts").click(function() {
		var content = $(this).html();
		var id = $(this).attr('uid');
		
		console.log("assert uid" + id);
		
		var searchString = "div.asserts[uid="+id+"]";
		
		if(content == '[Hide Assert Summary]') {
			console.log("hide now");
		
			$(searchString).hide("fast");
			$(this).text('[Show Assert Summary]');
		} else {
			console.log("show now");
		
			$(searchString).show("fast");
			$(this).text('[Hide Assert Summary]');
		}
	});
	
	$("span.show-all-tests").click(function() {
		var content = $(this).html();
 		
		var searchString = "div.tests";
		
		if(content == '[Hide All Tests]') {
			console.log("hide now");
		
			$(searchString).hide("fast");
			$(this).text('[Show All Tests]');
			
			/* handle the individual '[show tests]' switcher */
			$("span.show-tests[uid]").text('[Show tests]');
			
			
		} else {
			console.log("show now");
		
			$(searchString).show("fast");
			$(this).text('[Hide All Tests]');
			
			/* handle the individual '[show tests]' switcher */
			$("span.show-tests[uid]").text('[Hide tests]');
		}
	});
	
	$("span.show-everything").click(function() {
		var content = $(this).html();
 		
		var searchString = "div.tests";
		
		if(content == '[Hide Everything]') {
			console.log("hide now");
		
			$("div.tests").hide("fast");
			$("div.asserts").hide("fast");
			$(this).text('[Show Everything]');
			
			/* handle the individual switchers */
			$("span.show-tests[uid]").text('[Show tests]');
			$("span.show-asserts[uid]").text('[Show Assert Summary]');
		} else {
			console.log("show now");
		
			$("div.tests").show("fast");
			$("div.asserts").show("fast");
			$(this).text('[Hide Everything]');
			
			/* handle the individual switchers */
			$("span.show-tests[uid]").text('[Hide tests]');
			$("span.show-asserts[uid]").text('[Hide Assert Summary]');
			
		}
	});

	/* Initially everything is hidden */
	$("div.tests").hide();
	$("div.asserts").hide();
	
	$("div.test[result='passed']").addClass('passedTest');
	$("div.test[result='failed']").addClass('failedTest');
});

</script>
<style>

div, h1 {
  padding: 3px 10px 2px 10px;
  margin: 5px 0px 5px 0px;
}

.document {
  font-family: Arial;
  font-size: 11pt;
  background-color: #EDEDED;
}
	
.description {
  font-style: italic;
}	
	
.bigtitle {
  font-size: 14pt;
  font-weight: bold;
}
	
.title {
  font-weight: bold;
}
	
.switch {
  font-style: italic;
  color: rgb(10, 10, 200); /*#024A68;*/
  font-size: 10pt;
  cursor: pointer;
}	
	
.passedTest {
 background-color: #64AA2B;
}

.failedTest {
 background-color: #FF6E40;
}
	
.statistics {
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
      <span>Executed </span> <xsl:value-of select="testlog/numSuites"/> test suites. <br/>
      <span>Tests in total: </span> <xsl:value-of select="testlog/numTests"/> (passed: <xsl:value-of select="testlog/numPassedTests"/> , failed: <xsl:value-of select="testlog/numFailedTests"/>)
    </div>
  </div>

  <div>
	<br/>
	<span class="bigtitle">Test Results</span><br/>
    <span class="switch show-all-tests">[Show All Tests] </span> | 
    <span class="switch show-everything">[Show Everything]</span>
  </div>
	<br/>
	
  <xsl:for-each select="testlog/suite">
    <div id="suite">
    Suite: <span class="title"> <xsl:value-of select="name"/></span> (<xsl:value-of select="startTime"/>)
	<div class="suiteInfo">
		Tests: passed <xsl:value-of select="testsPassed"/>, failed <xsl:value-of select="testsFailed"/>, skipped <xsl:value-of select="testsSkipped"/>.<br/>
		Total runtime: <xsl:value-of select="totalRuntime"/> seconds. <br/>
		<span class="show-tests switch" uid="{generate-id(test)}">[Show tests]</span>
        <div class="tests" uid="{generate-id(test)}">
	      <xsl:for-each select="test">
			<div class="test">
			  <xsl:attribute name="result">
			    <xsl:value-of select="result"/>
	          </xsl:attribute>
		      
		      Name:  <xsl:value-of select="name"/> (<xsl:value-of select="startTime"/>  - <xsl:value-of select="endTime"/>  ) <br/>
		      Description: <span class="description"> <xsl:value-of select="description"/> </span><br/> 
		
	          Total runtime: <xsl:value-of select="totalRuntime"/> seconds  <br/>
		      Result: <xsl:value-of select="result"/>  <br/>
		      
		      <span class="switch show-asserts" uid="{generate-id(assertSummary)}">[Show Assert Summary]</span><br/>
			  <div class="asserts" uid="{generate-id(assertSummary)}">
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

