<?php
echo "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
echo "<?xml-stylesheet href=\"#internalstyle\" type=\"text/css\"?>\n";
?>
<!DOCTYPE html 
	PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
<title>Destroy FX : music plugin host softwares</title>
<style type="text/css" id="internalstyle">
	body
	{
		background: black ;
		color: black ;
		font-family: "Lucida Grande", Verdana, Arial, Helvetica, sans-serif ;
	}
	table
	{
		background-color: white ;
		border-width: 2px ;
		border-style: solid ;
		border-color: #424242 ;
		border-collapse: collapse ;
		margin-left: auto ;
		margin-right: auto ;
	}
	td, tr
	{
		border-width: 1px ;
		border-color: #919599 ;
		border-style: solid ;
		padding: 3px ;
	}
	p.footer
	{
		font-size: 90% ;
		text-align: center ;
		color: white ;
	}
	td
	{
		text-align: center ;
	}
	td.columnname
	{
		background-color: white ;
	}
	td.columnname_selected
	{
		background-color: #FFFF9C ;
	}
	tr.one_row
	{
		background-color: #EEFFFF ;
	}
	tr.other_row
	{
		background-color: #E7EEE7 ;
	}
	span.columnname
	{
		font-weight: bold ;
	}
	a.columnname:link
	{
		color: inherit ;
	}
	a:hover, a.columnname:hover
	{
		color: green ;
	}
	a:active, a.columnname:active
	{
		color: red ;
	}
	img
	{
		border-width: 0px ;
	}
</style>
</head>


<body>

<table>

<?php

$scriptname = basename($_SERVER['PHP_SELF']);

$symbol_yes = "&bull;";
$symbol_no = "&nbsp;";
$symbol_free = "&loz;";
$symbol_tempo = "*";
$symbol_midi = "&dagger;";
$symbol_instruments = "&sect;";
//$symbol_forthcoming = "&#9733;";	// solid star
$symbol_forthcoming = "&#9734;";	// outline star
$symbol_unsure = "?";

$filterOn = FALSE;
$numDescriptions = 0;
$hostDescriptionsArray = array();  


//-------------------------------------------------------------------
function GetPostValue($inPostKey)
{
	global $filterOn;

	if ( isset($_GET[$inPostKey]) )
	{
		$filterOn = TRUE;
		return (boolean) $_GET[$inPostKey];
	}
	else
		return FALSE;
}

$filtersArray = array(
	"au" => GetPostValue("au"), 
	"vst" => GetPostValue("vst"), 
	"tempo" => GetPostValue("tempo"), 
	"midi" => GetPostValue("midi"), 
	"instruments" => GetPostValue("instruments"), 
	"mac" => GetPostValue("mac"), 
	"windows" => GetPostValue("windows"), 
);

//-------------------------------------------------------------------
function IsVapor($inColumnValue)
{
	global $symbol_unsure, $symbol_forthcoming;

	if ( strcmp($inColumnValue, $symbol_unsure) == 0 )
		return TRUE;
	if ( strcmp($inColumnValue, $symbol_forthcoming) == 0 )
		return TRUE;
	return FALSE;
}

//-------------------------------------------------------------------
function PrintPseudoBoolean($inValue)
{
	global $symbol_yes, $symbol_no;

	echo "\t<td>";
	if ( is_bool($inValue) )
		echo ( $inValue ? $symbol_yes : $symbol_no );
	else
		echo $inValue;
	echo "</td>\n";
}

//-------------------------------------------------------------------
function IsFilterEnabled($inFilterKey)
{
	global $filtersArray;

	if ($inFilterKey)
	{
		if ( isset($filtersArray[$inFilterKey]) )
			return $filtersArray[$inFilterKey];
	}
	return FALSE;
}

//-------------------------------------------------------------------
function CreateUrlOptions($inFilterKey)
{
	global $filtersArray;

	$outputString = "";
	if ($inFilterKey)
	{
		if ( isset($filtersArray[$inFilterKey]) )
		{
			$entryState = $filtersArray[$inFilterKey];
			$filtersArray[$inFilterKey] = ! $filtersArray[$inFilterKey];
			foreach ($filtersArray as $filterKey => $filterValue)
			{
				if ($filterValue)
				{
					if (strlen($outputString) > 0)
						$outputString .= "&amp;";
					else
						$outputString .= "?";
					$outputString .= $filterKey . "=1";
				}
			}
			$filtersArray[$inFilterKey] = $entryState;
		}
	}
	return $outputString;
}

//-------------------------------------------------------------------
function PrintColumnName($inColumnName, $inColumnKey = NULL, $inFootnoteSymbol = NULL)
{
	global $scriptname;

	$rowClassName = IsFilterEnabled($inColumnKey) ? "columnname_selected" : "columnname";
	echo "\t";
	echo "<td class=\"{$rowClassName}\" scope=\"col\">";
	if ($inColumnKey)
		echo "<a class=\"columnname\" href=\"{$scriptname}" . CreateUrlOptions($inColumnKey) . "\">";
	echo "<span class=\"columnname\">{$inColumnName}</span>";
	if ($inColumnKey)
		echo "</a>";
	if ($inFootnoteSymbol != NULL)
		echo " " . $inFootnoteSymbol;
	echo "</td>";
	echo "\n";
}

//-------------------------------------------------------------------
class HostDescription
{
	var $name;
	var $url;
	var $au;
	var $vst;
	var $tempo;
	var $midi;
	var $instruments;
	var $mac;
	var $windows;
	var $free;

	function HostDescription($inName, $inURL, $inAU, $inVST, $inTempo, $inMIDI, $inInstruments, $inMac, $inWindows, $inFree = FALSE)
	{
		$this->name = $inName;
		$this->url = $inURL;
		$this->au = $inAU;
		$this->vst = $inVST;
		$this->tempo = $inTempo;
		$this->midi = $inMIDI;
		$this->instruments = $inInstruments;
		$this->mac = $inMac;
		$this->windows = $inWindows;
		$this->free = $inFree;
	}

	function getHostName()
	{
		return $this->name;
	}

	function printRow($inShowTime = FALSE)
	{
		global $symbol_free;
		static $rowToggle = FALSE;
		$rowClassName = $rowToggle ? "other_row" : "one_row";
		$rowToggle = !$rowToggle;

		echo "<tr class=\"{$rowClassName}\">\n";

		// host name
		echo "\t<td>";
		if ($this->url)
			echo "<a href=\"{$this->url}\">";
		echo $this->name;
		if ($this->url)
			echo "</a>";
		if ($this->free)
			echo " " . $symbol_free;
		echo "</td>\n";

		PrintPseudoBoolean($this->au);
		PrintPseudoBoolean($this->vst);
		PrintPseudoBoolean($this->tempo);
		PrintPseudoBoolean($this->midi);
		PrintPseudoBoolean($this->instruments);
		PrintPseudoBoolean($this->mac);
		PrintPseudoBoolean($this->windows);

		echo "</tr>";
		echo "\n\n";
	}
}

//-------------------------------------------------------------------
function HostDescriptionCompare($a, $b)
{
	$value1 = $a->getHostName();
	$value2 = $b->getHostName();
	return strnatcasecmp($value1, $value2);
}

//-------------------------------------------------------------------
function AddHostDescription($inName, $inURL, $inAU, $inVST, $inTempo, $inMIDI, $inInstruments, $inMac, $inWindows, $inFree = FALSE)
{
	global $hostDescriptionsArray;
	global $filtersArray;

	if ( $filtersArray["au"] && (!$inAU || IsVapor($inAU)) )
		return;
	if ( $filtersArray["vst"] && (!$inVST || IsVapor($inVST)) )
		return;
	if ( $filtersArray["tempo"] && (!$inTempo || IsVapor($inTempo)) )
		return;
	if ( $filtersArray["midi"] && (!$inMIDI || IsVapor($inMIDI)) )
		return;
	if ( $filtersArray["instruments"] && (!$inInstruments || IsVapor($inInstruments)) )
		return;
	if ( $filtersArray["mac"] && !$inMac )
		return;
	if ( $filtersArray["windows"] && !$inWindows )
		return;

	$hostDescriptionsArray[] = new HostDescription($inName, $inURL, $inAU, $inVST, $inTempo, $inMIDI, $inInstruments, $inMac, $inWindows, $inFree);
}

?>


<?php
if ($filterOn)
{
	echo "<p style=\"text-align: center ; font-weight: bold\">";
	echo "<a href=\"{$scriptname}\">";
	echo "view all";
	echo "</a>";
	echo "</p>";
}
?>


<tr class="footer">
<?php
PrintColumnName("host name");
PrintColumnName("AU support", "au");
PrintColumnName("VST support", "vst");
PrintColumnName("tempo sync", "tempo", $symbol_tempo);
PrintColumnName("MIDI for effects", "midi", $symbol_midi);
PrintColumnName("instruments", "instruments", $symbol_instruments);
PrintColumnName("Mac OS X", "mac");
//PrintColumnName("PC-DOS", "windows");
PrintColumnName("Windows", "windows");
?>
</tr>


<?php

AddHostDescription("Logic", "http://www.apple.com/logic/", "5.4", FALSE, "4.7", "5.0", "4.2", "5.3", FALSE);

AddHostDescription("Digital Performer", "http://www.motu.com/", "4.1", "8.0", TRUE, "4.5", TRUE, "4.0", "8.?");

AddHostDescription("Ableton Live", "http://www.ableton.com/", "4.0", TRUE, "2.0", "4.0", "4.0", TRUE, TRUE);

AddHostDescription("Max/MSP", "http://www.cycling74.com/", FALSE, TRUE, FALSE, TRUE, TRUE, "4.2", "4.3");

AddHostDescription("Plogue Bidule", "http://www.plogue.com/", TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

AddHostDescription("Wave Editor", "http://audiofile-engineering.com/", TRUE, "1.4", FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Peak", "http://www.bias-inc.com/", "4.0", "2.5", "5.0", "5.0", "5.0", "3.0", FALSE);

AddHostDescription("Deck", "http://www.bias-inc.com/", FALSE, "3.0", FALSE, FALSE, FALSE, "3.5", FALSE);

AddHostDescription("Cubase", "http://www.steinberg.net/", FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

AddHostDescription("Nuendo", "http://www.steinberg.net/", FALSE, TRUE, TRUE, TRUE, TRUE, "2.0", TRUE);

AddHostDescription("WaveLab", "http://www.steinberg.net/", FALSE, TRUE, FALSE, FALSE, FALSE, "7.0", TRUE);

AddHostDescription("Final Cut Pro", "http://www.apple.com/finalcutpro/", "4.0", FALSE, FALSE, FALSE, FALSE, "3.0", FALSE);

AddHostDescription("Final Cut Express", "http://www.apple.com/finalcutexpress/", "2.0", FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Soundtrack", "http://www.apple.com/soundtrack/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("GarageBand", "http://www.apple.com/ilife/garageband/", TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE);

AddHostDescription("Ardour", "http://ardour.org/", "2.2", FALSE, FALSE, FALSE, FALSE, "2.2", FALSE, TRUE);

AddHostDescription("Melodyne", "http://www.celemony.com/", "2.0", "2.0", FALSE, FALSE, TRUE, TRUE, TRUE);

AddHostDescription("Metro", "http://www.sagantech.biz/", "5.8", "4.0", TRUE, TRUE, "6.0", "5.8", FALSE);

AddHostDescription("WaveBurner", "http://www.apple.com/logicstudio/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Studio One", "http://www.presonus.com/", TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE);

AddHostDescription("Impromptu", "http://impromptu.moso.com.au/", TRUE, FALSE, $symbol_unsure, TRUE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("Sound Studio", "http://felttip.com/ss/", "3.0", FALSE, $symbol_unsure, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Intuem", "http://www.intuem.com/", "2.0", FALSE, FALSE, FALSE, TRUE, TRUE, FALSE);

AddHostDescription("SONASPHERE", "http://www.sonasphere.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE);

AddHostDescription("Numerology", "http://www.five12.com/", "1.2", FALSE, TRUE, TRUE, TRUE, TRUE, FALSE);

AddHostDescription("DSP-Quattro", "http://www.dsp-quattro.com/", "1.2", "3.1", FALSE, FALSE, TRUE, TRUE, FALSE);

AddHostDescription("Amadeus Pro", "http://www.hairersoft.com/", TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Amadeus II", "http://www.hairersoft.com/", "3.7.2", "3.2", FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("Audacity", "http://audacity.sourceforge.net/", "1.3", TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE);

AddHostDescription("Toast", "http://www.roxio.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("TwistedWave", "http://twistedwave.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("soundBlade", "http://www.sonicstudio.com/", TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("XO Wave", "http://www.xowave.com/", TRUE, FALSE, $symbol_unsure, $symbol_unsure, FALSE, TRUE, FALSE);

AddHostDescription("AU Lab", "http://developer.apple.com/audio/", TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE);

AddHostDescription("SonicBirth", "http://sonicbirth.sourceforge.net/", TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("Sample Manager", "http://audiofile-engineering.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("WireTap Studio", "http://www.ambrosiasw.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("CPS", "http://cps.bonneville.nl/", $symbol_forthcoming, "1.3 (Windows)", FALSE, TRUE, TRUE, "1.4", TRUE, TRUE);

AddHostDescription("Critters", "http://www.grain-brain.com/critters.html", "2.0", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("djay", "http://www.algoriddim.net/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE);

AddHostDescription("Neutrino", "http://machinecodex.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);

AddHostDescription("AudioMulch", "http://audiomulch.com/", "2.1", TRUE, TRUE, TRUE, TRUE, "2.0", TRUE);

AddHostDescription("Audiohive", "http://openstudionetworks.com/", TRUE, FALSE, $symbol_unsure, $symbol_unsure, $symbol_unsure, TRUE, FALSE);

AddHostDescription("Rax", "http://audiofile-engineering.com/rax/", TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE);

AddHostDescription("SynthTest", "http://www.manyetas.com/creed/synthtest.html", TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("Audio Hijack Pro", "http://www.rogueamoeba.com/audiohijack/", TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE);

AddHostDescription("AudioXplorer", "http://www.arizona-software.ch/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE);

AddHostDescription("Ouroboros", "http://illposed.com/", TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE);

AddHostDescription("Sound Forge", "http://www.sonycreativesoftware.com/", FALSE, "8.0", $symbol_unsure, FALSE, FALSE, FALSE, TRUE);

AddHostDescription("ACID Pro", "http://www.sonycreativesoftware.com/", FALSE, TRUE, TRUE, $symbol_unsure, "4.0", FALSE, TRUE);

AddHostDescription("Vegas", "http://www.sonycreativesoftware.com/", FALSE, "6.0", $symbol_unsure, FALSE, FALSE, FALSE, TRUE);

AddHostDescription("Reaper", "http://reaper.fm/", TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

AddHostDescription("BUZZ", "http://www.buzzmachines.com/", FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE);

AddHostDescription("FL Studio", "http://www.flstudio.com/", FALSE, "3.0", FALSE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("Tracktion", "http://www.rawmaterialsoftware.com/", FALSE, TRUE, TRUE, TRUE, TRUE, "1.4", TRUE);

AddHostDescription("SynthEdit", "http://www.synthedit.com/", FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE);

AddHostDescription("Orion", "http://www.synapse-audio.com/", FALSE, TRUE, "2.7", "2.7", TRUE, FALSE, TRUE);

AddHostDescription("Spark", "http://www.tcelectronic.com/", "2.8", TRUE, "2.8", "2.0", "2.0", "2.1", FALSE);

AddHostDescription("Spark ME", "http://www.tcelectronic.com/", "2.8", TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE);

usort($hostDescriptionsArray, "HostDescriptionCompare");
foreach ($hostDescriptionsArray as $desc)
	$desc->printRow();

?>

</table>


<p class="footer">

<?php echo $symbol_tempo; ?> Buffer Override, EQ Sync, Scrubby, and Skidder have tempo sync features
<br />
<?php echo $symbol_midi; ?> Buffer Override, Geometer, MIDI Gater, Rez Synth, Scrubby, Skidder, and Transverb are effects that process audio and MIDI
<br />
<?php echo $symbol_instruments; ?> Turntablist is an &ldquo;instrument&rdquo; plugin
<br />
<?php echo $symbol_free; ?> free
<br />
<?php echo $symbol_forthcoming; ?> forthcoming version (not available yet)
</p>

<p class="footer">
<a href="./"><img src="flaming-dfx3.gif" alt="back" /></a>
<br />
<a href="./">back</a>
</p>


</body>

</html>
