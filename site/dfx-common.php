<?php

// XXX this is a crummy hack
$weblibPath = "../weblib/weblib-utilities.php";
if ( file_exists($weblibPath) )
	include_once($weblibPath);
else
	include_once("../" . $weblibPath);

//-------------------------------------------------------------------
define("DFX_PAGE_TITLE_PREFIX", "Destroy FX : ");
$gResourcesRootPath = "";
define("VST_URL", "http://www.steinberg.net/");
define("AUDIOUNIT_URL", "http://www.apple.com/");

//-------------------------------------------------------------------
function PrintPageHeader($inTitle, $inBackgroundColor, $inBackgroundImage = NULL, $inIncludeKeywords = FALSE, $inIncludeJavascript = FALSE, $inRootPath = "")
{
	global $gResourcesRootPath;
	$gResourcesRootPath = $inRootPath;

	$metaItems = array( "author" => "Tom 7 and Sophia" );
	if ($inIncludeKeywords)
	{
		$metaItems["description"] = "A collection of free Audio Units and free VST plugins for destroying your music.";
		$metaItems["keywords"] = "Destroy FX, DestroyFX, destroy fx, destroyfx, Audio Unit, AudioUnit, audio unit, audiotunit, AU, free Audio Units, free AudioUnits, free audio units, free audiounits, VST, free, vst, plugins, VST plugins, vst plugins, free VST plugins, free vst plugins, universal binary, CoreAudio, coreaudio, coremidi, CoreMIDI, brokenfft, rez synth, buffer override, skidder, intercom, sonic decimatatronifier, stuck synth, thrush, fft, fuct soundz, source code, open source, Tom Murphy 7, Tom 7, Sophia Poirier, Sophia 3, Hrvatski, Keith Fullerton Whitman, xcode, project builder, visual c++, codewarrior, windows, win32, mac, mac os x, macintosh, apple, wavelab, logic audio, logic, spark, cubase, peak, max, msp, midi, noise, loud, transformation, transverb, polarizer, eq sync, vst gui tester, rms buddy, monomaker, midi gater, block test, broken fft, stucksynth, rezsynth, bufferoverride, midigater, rmsbuddy, eqsync, vst midi, vstmidi, vst gui tester, vstguitester, geometer, scrubby, waveform geometry";
	}

	$javascriptIncludes = NULL;
	$inlineStyle = NULL;
	if ($inIncludeJavascript)
	{
		$javascriptIncludes = "dfx.js";
		$inlineStyle = "table.infobox
{
	width: 450px ;
}";
	}

	PrintDocumentHead($inTitle, "dfx.css", $inlineStyle, $metaItems, $javascriptIncludes, NULL, NULL, $inRootPath);

	echo "
<body style=\"background-color: {$inBackgroundColor}";

	if ($inBackgroundImage)
		echo " ; background-image: url({$inBackgroundImage})";

	echo "\">
";
}


//-------------------------------------------------------------------
function NameToAnchor($inName, $inPreserveMore = FALSE)
{
	$inName = strtolower($inName);
	$extraExclusions = "";
	if ($inPreserveMore)
	{
		$extraExclusions = "-";
		$inName = preg_replace("/ /", $extraExclusions, $inName);
	}
	return preg_replace("/([^a-z0-9".$extraExclusions."])/", "", $inName);
}

//-------------------------------------------------------------------
function MakeNewsAnchor($inDate, $inAnchorSuffix = NULL)
{
	$resultString = "news" . $inDate;
	if ($inAnchorSuffix)
		$resultString .= $inAnchorSuffix;
	return $resultString;
}


$MISSING_FILE_SYMBOL = "?";

//-------------------------------------------------------------------
function MakeDownloadLink($inSoftwareName, $inFileURL, $inPlatform, $inIsPlugin = TRUE)
{
	global $MISSING_FILE_SYMBOL;
	global $gResourcesRootPath;

	switch ($inPlatform)
	{
		case 'Mac':
			$osName = "Mac OS X";
			$imageFile = "x";
			$pluginFormat = "AU";
			if ( ($inFileURL) && ($inFileURL != $MISSING_FILE_SYMBOL) )
			{
				$fileNameExtension = strrchr($inFileURL, ".");
				if ($fileNameExtension == ".sit")
					$pluginFormat = "VST";
			}
			break;

		case 'old Mac':
			$osName = "old Mac OS";
			$imageFile = "mac";
			$pluginFormat = "VST";
			break;

		case 'Windows':
			$osName = "Windows";
			$imageFile = "win32";
			$pluginFormat = "VST";
			break;

		case 'source':
		default:
			$osName = NULL;
			$imageFile = "source";
			$pluginFormat = NULL;
			break;
	}

	$fileExists = TRUE;
	if (! $inFileURL )
	{
		$imageFile .= "-no";
		$fileExists = FALSE;
	}
	$imageFile .= ".gif";
	if ($inFileURL == $MISSING_FILE_SYMBOL)
	{
		$imageFile = NULL;
		$fileExists = FALSE;
	}

	$fileSizeString = NULL;
	if ($fileExists)
		$fileSizeString = GetNicelyFormatedFileSize($inFileURL);

	if ($fileExists)
		$resultString = "<a href=\"{$inFileURL}\">";
	if ($imageFile)
	{
		if ($fileExists)
		{
			$altText = "download ";
			if ($inSoftwareName)
				$altText .= "{$inSoftwareName} ";
			if ($osName)
				$altText .= "for {$osName}";
			else
				$altText .= "source code";
			if ($inIsPlugin && $pluginFormat)
				$altText .= " ({$pluginFormat})";

			$titleText = "download ";
			if ($osName)
				$titleText .= $osName;
			else
				$titleText .= "source code";
			if ($inIsPlugin && $pluginFormat)
				$titleText .= " - {$pluginFormat}";
			if ($fileSizeString)
				$titleText .= " [{$fileSizeString}]";
		}
		else
		{
			$altText = "no ";
			if ($osName)
				$altText .= "{$osName} version ";
			else
				$altText .= "source code ";
			$altText .= "available";
			$titleText = $altText;
		}
		$resultString .= GetImageTag($gResourcesRootPath . $imageFile, $altText, NULL, $titleText);
	}
	else
		$resultString .= "<span class=\"small\"><b>missing</b><br /><a class=\"plain\" href=\"./#contact\">(do you have it?)</a></span>";
	if ($fileExists)
		$resultString .= "</a>";

	return $resultString;
}

?>
