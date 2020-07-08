<?php
	include("dfx-common.php");
	PrintPageHeader(DFX_PAGE_TITLE_PREFIX . "documentation", "#171947");
?>

<table class="container" style="background-color: #333771 ; width: 600px ; margin-left: auto ; margin-right: auto">
<tr><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;</td>
<td style="width: 100% ; padding: 0px" valign="top">
<a href="./"><?php echo GetImageTag("docs-title.gif", "Super Destroy FX", "right"); ?></a>
<br class="left" /><br class="right" />

<!-- museum -->
<table class="infobox">
<tr><td>

<?php echo GetImageTag("dfx-documentation.gif", "documentation", "right") . "\n"; ?>

<br class="right" />

<p class="museum">Welcome to the <b>Destroy FX documentation</b>!  Here you can learn.  Welcome to the learning center.
</p>

<table class="fullwidth" style="border-bottom : 1px solid black ; border-right : 1px solid black ; border-top: none ; border-left : none" cellpadding="2" cellspacing="0">
<tr class="columnname">
	<th class="thinblack_columnname">name</th>
	<th class="thinblack_columnname">last updated</th>
</tr>



<?php

$docsDirectoryPath = "docs/";

function GetDocumentationFileDisplayName($inFileName)
{
	$inFileName = preg_replace("/\.([a-z0-9]+)$/i", " ", $inFileName);
	$inFileName = preg_replace("/([_-])/", " ", $inFileName);
	$inFileName = ucwords($inFileName);

	$allCapsWords = array("fx", "dfx", "midi", "eq", "rms", "vst");
	$inFileName = " " . $inFileName . " ";
	foreach ($allCapsWords as $specialWord)
	{
		$inFileName = preg_replace("/ ".$specialWord." /i", " ".strtoupper($specialWord)." ", $inFileName);
	}
	$inFileName = trim($inFileName);

	return $inFileName;
}

$gRowCount = 0;
function PrintDocumentationFileInfo($inFileName, $inFileModTime)
{
	global $docsDirectoryPath, $gRowCount;

	$rowClass = ($gRowCount % 2) ? "other_row" : "one_row";
	$gRowCount++;

	echo "<tr class=\"{$rowClass}\">\n";

	$fileURL = $docsDirectoryPath . rawurlencode($inFileName);
	$fileName_display = GetDocumentationFileDisplayName($inFileName);
	$fileLink = "<a href=\"{$fileURL}\" class=\"semiplain\">" . htmlentities($fileName_display) . "</a>";
	echo "	<td class=\"thinblack\">" . $fileLink . "</td>\n";

	$fileModTime_display = date("F jS Y", $inFileModTime);
	echo "	<td class=\"thinblack\" style=\"text-align: right\">" . $fileModTime_display . "</td>\n";

	echo "</tr>\n";
}



$directoryHandle = opendir($docsDirectoryPath);
if ($directoryHandle)
{
	while ( ($fileName = readdir($directoryHandle)) !== FALSE )
	{
		$filePath = $docsDirectoryPath . $fileName;
		if ($fileName[0] === ".")
			continue;
		if (! is_readable($filePath) )
			continue;
		if ( @is_dir($filePath) )
			continue;
		if (! FileNameExtensionMatches(array("html", "htm", "txt"), $fileName) )
			continue;

		$fileStream = OpenFileCautiously($filePath, "r");
		if ($fileStream)
		{
			$fileStats = fstat($fileStream);
			fclose($fileStream);
			PrintDocumentationFileInfo($fileName, $fileStats["mtime"]);
		}
	}
	closedir($directoryHandle);
}

?>

</table>
</td></tr></table>



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
