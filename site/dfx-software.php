<?php

$SHOW_DONATE_LINKS = TRUE;

//-------------------------------------------------------------------
function BeginInfoBox($inTitle, $inTitleImage = NULL, $inAnchorName = NULL, $inHasIconColumn = FALSE, $inAnchorName_Section = NULL)
{
	if ($inTitleImage)
		$titleString = GetImageTag($inTitleImage, $inTitle, "right");
	else
		$titleString = $inTitle;
	echo "<p";
	if ($inAnchorName_Section)
		echo " id=\"{$inAnchorName_Section}\"";
	echo "></p>
<table class=\"infobox\"";
	if ($inAnchorName)
		echo " id=\"{$inAnchorName}\"";
	echo ">
<tr><td>

<table class=\"fullwidth\">
";
	if ($titleString)
		echo "<tr><td colspan=\"2\">{$titleString}</td></tr>
";
	echo "<tr><td";
	if ($inHasIconColumn)
		echo " class=\"iconcolumn\"";
	echo " valign=\"top\">
";
}

//-------------------------------------------------------------------
function EndInfoBox()
{
	echo "
</td></tr>
</table>

</td></tr></table>
";
}

//-------------------------------------------------------------------
function PrintCurrentSoftwareVersion($inVersionNumber, $inDate, $inTypeSpecification = NULL)
{
	global $gResourcesRootPath;

	$dateTimestamp = strtotime($inDate);
	if ( ($dateTimestamp !== FALSE) && ($dateTimestamp != -1) )
		$dateString = date("j M Y", $dateTimestamp);
	else
		$dateString = $inDate;

	$releaseNotesURL = $gResourcesRootPath . "news.php#" . MakeNewsAnchor($inDate);

	echo "
<p class=\"rel\"><a class=\"plain\" href=\"{$releaseNotesURL}\">";
	if ($inTypeSpecification)
		echo "{$inTypeSpecification}: ";
	echo "Version {$inVersionNumber} - {$dateString}</a></p>
";
}

//-------------------------------------------------------------------
function PrintCurrentSoftwareDownloads($inSoftwareName, $inShowAULink = TRUE, $inIsPlugin = TRUE)
{
	global $gResourcesRootPath;
	global $SHOW_DONATE_LINKS;

	$softwareFileName = NameToAnchor($inSoftwareName);
	$softwareImage_thumbnail = "{$softwareFileName}-small.png";
	$softwareImage_full = "{$softwareFileName}.png";
	if (! file_exists($softwareImage_full) )
		$softwareImage_full = "{$softwareFileName}.jpg";

	$softwareImageWidth = 100;
	$softwareImageHeight = 100;
	if ( file_exists($softwareImage_full) )
	{
//		RemoveErrorLevel(E_WARNING);
		$imageSizeInfo = @getimagesize($softwareImage_full);
//		RestoreErrorLevel();
		if ($imageSizeInfo != FALSE)
		{
			$softwareImageWidth = $imageSizeInfo[0];
			$softwareImageHeight = $imageSizeInfo[1];
		}
		$thumbnailTitleString .= "see full-size [" . GetNicelyFormatedFileSize($softwareImage_full) . "]";
	}
	else
		$thumbnailTitleString = NULL;
	$thumbnailImageTag = GetImageTag($softwareImage_thumbnail, 
			"view {$inSoftwareName} interface (full size)", 
			NULL, $thumbnailTitleString);


	// image thumbnail pop-up link
	echo "
<a href=\"{$softwareImage_full}\" onclick=\"return openPicture('{$softwareImage_full}','{$inSoftwareName}','{$softwareImageWidth}','{$softwareImageHeight}');\">{$thumbnailImageTag}</a>

";


	// software download links
	echo "<table class=\"filler\"><tr>
";

	// Mac OS X release
	$releaseVersionAU = FALSE;
	$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-mac.dmg";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-carbon.sit";
	else
		$releaseVersionAU = TRUE;
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = NULL;
	echo "	<td rowspan=\"2\">";
	echo MakeDownloadLink($inSoftwareName, $softwareFilePath, "Mac", $inIsPlugin);
	echo "</td>
";

	// Windows release
	$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-win.zip";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = NULL;
	echo "	<td rowspan=\"2\">";
	echo MakeDownloadLink($inSoftwareName, $softwareFilePath, "Windows", $inIsPlugin);
	echo "</td>
";

	// old Mac OS release
	$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-mac.sit";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = NULL;
	echo "	<td rowspan=\"2\">";
	echo MakeDownloadLink($inSoftwareName, $softwareFilePath, "old Mac", $inIsPlugin);
	echo "</td>
";

	echo "</tr></table>

";


	// software download links (2nd row)
	echo "<br /><table class=\"filler\"><tr>
";

	// Audio Units page link
	if ( !($releaseVersionAU) && ($inShowAULink) )
	{
		echo "	<td class=\"icons\"><a href=\"{$gResourcesRootPath}audiounits.html#{$softwareFileName}\">";
		echo GetImageTag($gResourcesRootPath . "x-au.png", "download {$inSoftwareName} for Mac OS X (AU)", NULL, "download Mac OS X - AU");
		echo "</a></td>
";
	}

	// source code
	$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-source.tar.gz";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-source.sit";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = $gResourcesRootPath . "software/{$softwareFileName}-patch.sit";
	if (! file_exists($softwareFilePath) )
		$softwareFilePath = NULL;
	echo "\t<td class=\"icons\">" . MakeDownloadLink($inSoftwareName, $softwareFilePath, "source", $inIsPlugin) . "</td>\n";

	echo "</tr></table>

";


	// documentation link
	$softwareFilePath = $gResourcesRootPath . "docs/" . NameToAnchor($inSoftwareName, TRUE) . ".html";
	if ( file_exists($softwareFilePath) )
	{
		echo "<br /><table class=\"filler\"><tr><td class=\"icons\">
	<a href=\"{$softwareFilePath}\">";
		echo GetImageTag($gResourcesRootPath . "docs.png", "manual", NULL, "view documentation for {$inSoftwareName}");
		echo "</a>
</td></tr></table>

";
	}


	// donate link
	if ($SHOW_DONATE_LINKS)
	{
		echo "<br /><table class=\"filler\"><tr><td class=\"icons\">
	<a href=\"{$gResourcesRootPath}donate.php\">";
		echo GetImageTag($gResourcesRootPath . "donate-button.png", "donate");
		echo "</a>
</td></tr></table>

";
	}


	echo "</td>
<td valign=\"top\">
";
}

//-------------------------------------------------------------------
function PrintSoftwareDescription($inDescriptionText)
{
	echo "
<p class=\"desc\">
{$inDescriptionText}
</p>
";
}

//-------------------------------------------------------------------
function BeginAudioSamples()
{
	echo "
<table class=\"fullwidth\">
<tr><th class=\"demo\">example sound name</th><th class=\"demolabel\">dry</th><th class=\"demolabel\">wet</th></tr>
";
}

//-------------------------------------------------------------------
function EndAudioSamples()
{
	echo "</table>
";
}

//-------------------------------------------------------------------
function PrintAudioSample($inDemoTitle, $inDryFile, $inWetFile)
{
	global $gResourcesRootPath;

	$audioSamplesDirectory = $gResourcesRootPath . "audio";
	$inWetFile = $audioSamplesDirectory . "/" . $inWetFile;
	if ($inDryFile)
		$inDryFile = $audioSamplesDirectory . "/" . $inDryFile;

	echo "<tr>
";
	echo "	<td class=\"demoname\">{$inDemoTitle}</td>
";

	echo "	<td class=\"icons\">";
	if ($inDryFile)
	{
		echo "<a href=\"{$inDryFile}\">";
		echo GetImageTag("dry.png", "dry audio sample", NULL, GetNicelyFormatedFileSize($inDryFile));
		echo "</a>";
	}
	else
		echo GetImageTag("dry-no.png", "no dry audio sample available");
	echo "</td>
";

	echo "	<td class=\"icons\"><a href=\"{$inWetFile}\">";
	echo GetImageTag("wet.png", "wet audio sample", NULL, GetNicelyFormatedFileSize($inDryFile));
	echo "</a></td>
";

	echo "</tr>
";
}

?>
