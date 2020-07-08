<?php
	include("dfx-common.php");
	include("dfx-software.php");
	PrintPageHeader(DFX_PAGE_TITLE_PREFIX . "source code", "#cb4496");
?>

<table class="container" style="background-color: #ef7cc2 ; width: 600px ; margin-left: auto ; margin-right: auto">
<tr><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;</td>
<td style="width: 100% ; padding: 0px" valign="top">
<h1 style="text-align: right ; font-size: 210% ; font-family: 'Trebuchet MS', Verdana, Arial, sans-serif ; margin-bottom: 0px">Destroy FX source code</h1>
<br class="left" /><br class="right" />


<?php

function BeginSourceCodeItem($inTitle)
{
	echo "<!-- {$inTitle} -->

<p></p>
<table class=\"infobox\"><tr><td>
<table class=\"fullwidth\"><tr><td valign=\"top\">

<p class=\"minititle\">{$inTitle}</p>";
	echo "

";
}

function PrintSourceCodeBlurb($inBlurbText)
{
	echo "<p class=\"desc\">
{$inBlurbText}
</p>

";
}

function GetSourceCodeFileDisplayName($inFileName)
{
	$inFileName = preg_replace("/\-source\.([a-z0-9\.]+)$/i", "", $inFileName);
	$inFileName = preg_replace("/([_-])/", " ", $inFileName);
//	$inFileName = ucwords($inFileName);
$inFileName = strtoupper($inFileName);
	return $inFileName;
}

function PrintSourceCodeFileLink($inFileName)
{
	global $downloadsDirectoryPath;

	echo "	<li>";
	$fileName_display = GetSourceCodeFileDisplayName($inFileName);
	$fileURL = $downloadsDirectoryPath . rawurlencode($inFileName);
	$fileLink = GetLinkTag(htmlentities($fileName_display), $fileURL);
	echo $fileLink;
	echo "</li>\n";
}

?>


<?php
BeginSourceCodeItem("source code repository");
PrintSourceCodeBlurb("We have a " . GetLinkTag("source code repository", "http://destroyfx.sourceforge.net/") . " available for your perusal and modification.  There you will find the latest state of our development efforts, including some unfinished projects.");
EndInfoBox();
?>



<?php
BeginSourceCodeItem("Destroy FX Audio Unit utilities library");
PrintSourceCodeBlurb("We also have a page dedicated to our " . GetLinkTag("Destroy FX Audio Unit utilities library", "dfx-au-utilities.html") . ", which Audio Unit plugin and host developers may find useful.");
EndInfoBox();
?>



<?php
BeginSourceCodeItem("source code packages");
PrintSourceCodeBlurb("If you just want the source code for specific projects and don't want to navigate our entire source code repository, we also make source code packages available for each of our software releases.  You can find download links in every software info box on our main pages, or crudely cataloged below:");
echo "<ul>
";
$downloadsDirectoryPath = "software/";
$directoryHandle = opendir($downloadsDirectoryPath);
if ($directoryHandle)
{
	while ( ($fileName = readdir($directoryHandle)) !== FALSE )
	{
		$filePath = $downloadsDirectoryPath . $fileName;
		if ($fileName[0] === ".")
			continue;
		if (! is_readable($filePath) )
			continue;
		if ( @is_dir($filePath) )
			continue;
		if (strstr($fileName, "-source.") === FALSE)
			continue;

		PrintSourceCodeFileLink($fileName, $filePath);
	}
	closedir($directoryHandle);
}
echo "</ul>

";
EndInfoBox();
?>



<!-- bottom -->

<table class="fullwidth">
<tr>
<td>&nbsp;</td>
<td style="width: 250px">
<a href="./"><?php echo GetImageTag("smel1.gif", "Destroy FX: a "); ?></a><a href="http://smartelectronix.com/"><?php echo GetImageTag("smel2.gif", "Smartelectronix"); ?></a><?php echo GetImageTag("smel3.gif", " member"); ?>
</td>
</tr>
</table>

</td><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;&nbsp;</td></tr>
</table>

</body>

</html>
