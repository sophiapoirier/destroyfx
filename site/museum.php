<?php
	include("dfx-common.php");
	PrintPageHeader(DFX_PAGE_TITLE_PREFIX . "Museum", "#1E581F", "green.gif");
?>

<table style="width: 640px" cellspacing="0">
<tr><td style="width: 36px">&nbsp;</td>
<td style="width: 600px" valign="top">
<?php echo GetImageTag("museum-title.gif", "Destroy FX Museum") . "\n"; ?>
<br class="left" /><br class="right" />

<!-- museum -->
<table class="infobox" style="width: 600px">
<tr><td>

<?php echo GetImageTag("dfx-museum.gif", "museum", "right") . "\n"; ?>

<br class="right" />

<p class="museum">Welcome to the <b>Destroy FX Museum</b>! Here you can find
archives of old versions of our plugins. These are sometimes useful
for reviving an ancient song that won't load (or sounds wrong) with
new versions, or to resolve specific issues with older hosts. We
welcome you to help yourself to the downloads, though we probably will
not be able to help you if you experience problems with these. If by
some mistake of fate you found yourself at this page of grimy old
plugins rather than our <b><a href="./">regular page of shiny
new plugins</a></b>, by all means, head there instead!
</p>
<p class="museum">
attention:  If you see something listed here that says <b>missing</b> 
and you happen to have that file archived in your own personal stash, 
please <a href="./#contact">let us know</a> and share it with us 
so that we can complete our archive.  Thanks!
</p>

<table class="fullwidth" style="border-bottom : 1px solid black ; border-right : 1px solid black ; border-top: none ; border-left : none" cellpadding="2" cellspacing="0">
<tr>
	<td>&nbsp;</td>
	<th class="thinblack_columnname">Windows</th>
	<th class="thinblack_columnname">Mac OS X</th>
	<th class="thinblack_columnname">Mac OS 8/9</th>
	<th class="thinblack_columnname">Source</th>
</tr>



<?php

$CURRENT_SOFTWARE_NAME = NULL;
function PrintSoftwareName($inSoftwareName)
{
	global $CURRENT_SOFTWARE_NAME;
	$CURRENT_SOFTWARE_NAME = $inSoftwareName;
	$idString = NameToAnchor($inSoftwareName);

	echo "


<!-- {$inSoftwareName} -->
<tr class=\"columnname\" id=\"{$idString}\"><th class=\"thinblack\" colspan=\"5\">{$inSoftwareName}</th></tr>

";
}

function PrintMuseumDownloadCell($inFileURL, $inPlatform, $inIsPlugin = TRUE)
{
	global $CURRENT_SOFTWARE_NAME;
	echo "	<td class=\"thinblack_icon\">";
	echo MakeDownloadLink($CURRENT_SOFTWARE_NAME, $inFileURL, $inPlatform, $inIsPlugin);
	echo "</td>\n";
}

$gRowCount = 0;
function PrintSoftwareVersion($inVersionNumber, $inDate, $inWindowsURL, $inMacURL, $inOldMacURL, $inSourceURL, $inIsPlugin = TRUE)
{
	global $CURRENT_SOFTWARE_NAME;
	global $gRowCount;
	$rowClass = ($gRowCount % 2) ? "other_row" : "one_row";
	$gRowCount++;

	$dateTimestamp = strtotime($inDate);
	if ( ($dateTimestamp !== FALSE) && ($dateTimestamp != -1) )
		$dateString = date("j M Y", $dateTimestamp);
	else
		$dateString = $inDate;
	$releaseNotesURL = "news.php#" . MakeNewsAnchor($inDate);

	echo "<tr class=\"{$rowClass}\">
	<td class=\"thinblack\"><a class=\"plain\" href=\"{$releaseNotesURL}\">{$inVersionNumber}</a><br /><span class=\"small\">{$dateString}</span></td>
";

	PrintMuseumDownloadCell($inWindowsURL, "Windows", $inIsPlugin);
	PrintMuseumDownloadCell($inMacURL, "Mac", $inIsPlugin);
	PrintMuseumDownloadCell($inOldMacURL, "old Mac", $inIsPlugin);
	PrintMuseumDownloadCell($inSourceURL, "source", $inIsPlugin);

	echo "</tr>
";
}



PrintSoftwareName("Geometer");

PrintSoftwareVersion("1.1", "2003-01-08", "museum/geometer-1.1-win.zip", "museum/geometer-1.1-carbon.sit", "museum/geometer-1.1-mac.sit", "museum/geometer-1.1-source.sit");

PrintSoftwareVersion("1.0", "2002-08-09", "museum/geometer-1.0-win.zip", "museum/geometer-1.0-carbon.sit", "museum/geometer-1.0-mac.sit", "museum/geometer-1.0-source.sit");



//PrintSoftwareName("Scrubby");

//PrintSoftwareVersion("1.0", "2002-07-08", "museum/scrubby-1.0-win.zip", "museum/scrubby-1.0-carbon.sit", "museum/scrubby-1.0-mac.sit", "museum/scrubby-1.0-source.sit");



PrintSoftwareName("Buffer Override");

//PrintSoftwareVersion("2.0", "2002-07-08", "museum/bufferoverride-2.0-win.zip", "museum/bufferoverride-2.0-carbon.sit", "museum/bufferoverride-2.0-mac.sit", "museum/bufferoverride-2.0-source.sit");

PrintSoftwareVersion("1.1.2", "2002-01-03", "museum/bufferoverride-1.1.2-win.zip", "museum/bufferoverride-1.1.2-carbon.sit", "museum/bufferoverride-1.1.2-mac.sit", "museum/bufferoverride-1.1.2-source.sit");

PrintSoftwareVersion("1.1.1", "2001-12-02", "museum/bufferoverride-1.1.1-win.zip", "museum/bufferoverride-1.1.1-carbon.sit", "museum/bufferoverride-1.1.1-mac.sit", "museum/bufferoverride-1.1.1-source.sit");

PrintSoftwareVersion("1.1", "2001-11-27", "museum/bufferoverride-1.1-win.zip", "museum/bufferoverride-1.1-carbon.sit", "museum/bufferoverride-1.1-mac.sit", "museum/bufferoverride-1.1-source.sit");

PrintSoftwareVersion("1.0.4", "2001-10-26", "museum/bufferoverride-1.0.4-win.zip", "museum/bufferoverride-1.0.4-carbon.sit", "museum/bufferoverride-1.0.4-mac.sit", "museum/bufferoverride-1.0.4-source.sit");

PrintSoftwareVersion("1.0.3", "2001-10-10", "museum/bufferoverride-1.0.3-win.zip", "museum/bufferoverride-1.0.3-carbon.sit", "museum/bufferoverride-1.0.3-mac.sit", "museum/bufferoverride-1.0.3-source.sit");

PrintSoftwareVersion("1.0.2", "2001-09-29", "museum/bufferoverride-1.0.2-win.zip", "museum/bufferoverride-1.0.2-carbon.sit", "museum/bufferoverride-1.0.2-mac.sit", "museum/bufferoverride-1.0.2-source.sit");

PrintSoftwareVersion("1.0.1", "2001-09-26", NULL, NULL, $MISSING_FILE_SYMBOL, NULL);

PrintSoftwareVersion("1.0", "2001-09-18", "museum/bufferoverride-1.0-win.zip", $MISSING_FILE_SYMBOL, $MISSING_FILE_SYMBOL, NULL);



PrintSoftwareName("Transverb");

PrintSoftwareVersion("1.3", "2002-03-16", "museum/transverb-1.3-win.zip", "museum/transverb-1.3-carbon.sit", "museum/transverb-1.3-mac.sit", "museum/transverb-1.3-source.sit");

PrintSoftwareVersion("1.2.3", "2002-02-25", "museum/transverb-1.2.3-win.zip", "museum/transverb-1.2.3-carbon.sit", "museum/transverb-1.2.3-mac.sit", "museum/transverb-1.2.3-source.sit");

PrintSoftwareVersion("1.2.2", "2002-01-28", "museum/transverb-1.2.2-win.zip", "museum/transverb-1.2.2-carbon.sit", "museum/transverb-1.2.2-mac.sit", "museum/transverb-1.2.2-source.sit");

PrintSoftwareVersion("1.2.1", "2002-01-21", "museum/transverb-1.2.1-win.zip", NULL, NULL, "museum/transverb-1.2.1-source.sit");

PrintSoftwareVersion("1.2", "2002-01-19", "museum/transverb-1.2-win.zip", "museum/transverb-1.2-carbon.sit", "museum/transverb-1.2-mac.sit", "museum/transverb-1.2-source.sit");

PrintSoftwareVersion("1.1", "2001-10-29", "museum/transverb-1.1-win.zip", "museum/transverb-1.1-carbon.sit", "museum/transverb-1.1-mac.sit", "museum/transverb-1.1-source.sit");

PrintSoftwareVersion("1.0.1", "2001-10-10", "museum/transverb-1.0.1-win.zip", "museum/transverb-1.0.1-carbon.sit", "museum/transverb-1.0.1-mac.sit", "museum/transverb-1.0.1-source.sit");

PrintSoftwareVersion("1.0", "2001-09-30", "museum/transverb-1.0-win.zip", "museum/transverb-1.0-carbon.sit", "museum/transverb-1.0-mac.sit", "museum/transverb-1.0-source.sit");



PrintSoftwareName("Rez Synth");

PrintSoftwareVersion("1.1", "2002-03-16", "museum/rezsynth-1.1-win.zip", "museum/rezsynth-1.1-carbon.sit", "museum/rezsynth-1.1-mac.sit", "museum/rezsynth-1.1-source.sit");

PrintSoftwareVersion("1.0.8", "2002-01-01", "museum/rezsynth-1.0.8-win.zip", "museum/rezsynth-1.0.8-carbon.sit", "museum/rezsynth-1.0.8-mac.sit", "museum/rezsynth-1.0.8-source.sit");

PrintSoftwareVersion("1.0.7", "2001-12-12", "museum/rezsynth-1.0.7-win.zip", "museum/rezsynth-1.0.7-carbon.sit", "museum/rezsynth-1.0.7-mac.sit", "museum/rezsynth-1.0.7-source.sit");

PrintSoftwareVersion("1.0.6", "2001-11-23", "museum/rezsynth-1.0.6-win.zip", "museum/rezsynth-1.0.6-carbon.sit", "museum/rezsynth-1.0.6-mac.sit", "museum/rezsynth-1.0.6-source.sit");

PrintSoftwareVersion("1.0.5", "2001-11-16", "museum/rezsynth-1.0.5-win.zip", "museum/rezsynth-1.0.5-carbon.sit", "museum/rezsynth-1.0.5-mac.sit", "museum/rezsynth-1.0.5-source.sit");

PrintSoftwareVersion("1.0.4", "2001-10-26", "museum/rezsynth-1.0.4-win.zip", "museum/rezsynth-1.0.4-carbon.sit", "museum/rezsynth-1.0.4-mac.sit", "museum/rezsynth-1.0.4-source.sit");

PrintSoftwareVersion("1.0.3", "2001-10-10", "museum/rezsynth-1.0.3-win.zip", "museum/rezsynth-1.0.3-carbon.sit", "museum/rezsynth-1.0.3-mac.sit", "museum/rezsynth-1.0.3-source.sit");

PrintSoftwareVersion("1.0.2", "2001-09-29", "museum/rezsynth-1.0.2-win.zip", "museum/rezsynth-1.0.2-carbon.sit", "museum/rezsynth-1.0.2-mac.sit", "museum/rezsynth-1.0.2-source.sit");

PrintSoftwareVersion("1.0.1", "2001-09-26", NULL, NULL, $MISSING_FILE_SYMBOL, NULL);

PrintSoftwareVersion("1.0", "2001-09-18", "museum/rezsynth-1.0-win.zip", NULL, $MISSING_FILE_SYMBOL, NULL);



PrintSoftwareName("Skidder");

PrintSoftwareVersion("1.3", "2002-03-16", "museum/skidder-1.3-win.zip", "museum/skidder-1.3-carbon.sit", "museum/skidder-1.3-mac.sit", "museum/skidder-1.3-source.sit");

PrintSoftwareVersion("1.2.2", "2002-01-01", "museum/skidder-1.2.2-win.zip", "museum/skidder-1.2.2-carbon.sit", "museum/skidder-1.2.2-mac.sit", "museum/skidder-1.2.2-source.sit");

PrintSoftwareVersion("1.2.1", "2001-12-12", "museum/skidder-1.2.1-win.zip", "museum/skidder-1.2.1-carbon.sit", "museum/skidder-1.2.1-mac.sit", "museum/skidder-1.2.1-source.sit");

PrintSoftwareVersion("1.2", "2001-11-25", "museum/skidder-1.2-win.zip", "museum/skidder-1.2-carbon.sit", "museum/skidder-1.2-mac.sit", "museum/skidder-1.2-source.sit");

PrintSoftwareVersion("1.1.2", "2001-11-23", "museum/skidder-1.1.2-win.zip", "museum/skidder-1.1.2-carbon.sit", "museum/skidder-1.1.2-mac.sit", "museum/skidder-1.1.2-source.sit");

PrintSoftwareVersion("1.1.1", "2001-11-13", "museum/skidder-1.1.1-win.zip", "museum/skidder-1.1.1-carbon.sit", "museum/skidder-1.1.1-mac.sit", "museum/skidder-1.1.1-source.sit");

PrintSoftwareVersion("1.1", "2001-10-26", "museum/skidder-1.1-win.zip", "museum/skidder-1.1-carbon.sit", "museum/skidder-1.1-mac.sit", "museum/skidder-1.1-source.sit");

PrintSoftwareVersion("1.0.3", "2001-10-10", "museum/skidder-1.0.3-win.zip", "museum/skidder-1.0.3-carbon.sit", "museum/skidder-1.0.3-mac.sit", "museum/skidder-1.0.3-source.sit");

PrintSoftwareVersion("1.0.2", "2001-09-29", "museum/skidder-1.0.2-win.zip", "museum/skidder-1.0.2-carbon.sit", "museum/skidder-1.0.2-mac.sit", "museum/skidder-1.0.2-source.sit");

PrintSoftwareVersion("1.0.1", "2001-09-26", NULL, NULL, $MISSING_FILE_SYMBOL, NULL);

PrintSoftwareVersion("1.0", "2001-09-18", "museum/skidder-1.0-win.zip", NULL, $MISSING_FILE_SYMBOL, NULL);



PrintSoftwareName("Monomaker");

PrintSoftwareVersion("1.0", "2001-11-27", "museum/monomaker-1.0-win.zip", "museum/monomaker-1.0-carbon.sit", "museum/monomaker-1.0-mac.sit", "museum/monomaker-1.0-source.sit");



PrintSoftwareName("RMS Buddy");

PrintSoftwareVersion("1.1.1", "2003-11-12", NULL, "museum/rmsbuddy-1.1.1-mac.tar.gz", NULL, "museum/rmsbuddy-1.1.1-source.tar.gz");

PrintSoftwareVersion("1.1", "2003-02-25", "museum/rmsbuddy-1.1-win.zip", "museum/rmsbuddy-1.1-mac.tar.gz", "museum/rmsbuddy-1.1-mac.sit", "museum/rmsbuddy-1.1-source.tar.gz");

PrintSoftwareVersion("1.0.1", "2002-03-01", "museum/rmsbuddy-1.0.1-win.zip", "museum/rmsbuddy-1.0.1-carbon.sit", "museum/rmsbuddy-1.0.1-mac.sit", "museum/rmsbuddy-1.0.1-source.sit");

PrintSoftwareVersion("1.0", "2001-11-19", "museum/rmsbuddy-1.0-win.zip", "museum/rmsbuddy-1.0-carbon.sit", "museum/rmsbuddy-1.0-mac.sit", "museum/rmsbuddy-1.0-source.sit");



PrintSoftwareName("EQ Sync");

PrintSoftwareVersion("1.0", "2001-11-19", "museum/eqsync-1.0-win.zip", "museum/eqsync-1.0-carbon.sit", "museum/eqsync-1.0-mac.sit", "museum/eqsync-1.0-source.sit");



PrintSoftwareName("Polarizer");

PrintSoftwareVersion("1.0", "2001-11-19", "museum/polarizer-1.0-win.zip", "museum/polarizer-1.0-carbon.sit", "museum/polarizer-1.0-mac.sit", "museum/polarizer-1.0-source.sit");



PrintSoftwareName("block test");

PrintSoftwareVersion("1.0", "2001-11-19", "museum/blocktest-1.0-win.zip", "museum/blocktest-1.0-carbon.sit", "museum/blocktest-1.0-mac.sit", "museum/blocktest-1.0-source.sit");



PrintSoftwareName("VST MIDI");

PrintSoftwareVersion("1.1", "2001-09-20", NULL, NULL, "museum/vstmidi-1.1-mac.sit", "museum/vstmidi-1.1-patch.sit");

PrintSoftwareVersion("1.0", "2001-09-12", NULL, NULL, $MISSING_FILE_SYMBOL, $MISSING_FILE_SYMBOL);

?>



</table>


</td></tr></table>
<a href="./"><?php echo GetImageTag("keep-on-downloading.png", "&lt; &lt; keep on downloading", "right"); ?></a>
<br /><p><br /></p>
</td><td style="width: 4px">&nbsp;</td></tr>
</table>
</body>
</html>
