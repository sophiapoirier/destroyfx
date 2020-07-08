<?php
	include("dfx-common.php");
	PrintPageHeader(DFX_PAGE_TITLE_PREFIX . "system requirements", "#FCC8ED");
?>

<table class="container" style="background-color: #F9FAD3 ; width: 600px ; margin-left: auto ; margin-right: auto">
<tr><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;</td>
<td style="width: 100% ; padding: 0px" valign="top">
<h1 style="text-align: right ; font-size: 210% ; font-family: 'Trebuchet MS', Verdana, Arial, sans-serif ; margin-bottom: 0px">Destroy FX system requirements</h1>
<br class="left" /><br class="right" />


<?php

function BeginSystemRequirementsItem($inTitle, $inAnchorName, $inIconFilePath, $inIconFilePath2 = NULL)
{
	echo "<!-- {$inTitle} -->

<p></p>
<table class=\"infobox\" id=\"{$inAnchorName}\"><tr><td>
<table class=\"fullwidth\"><tr><td valign=\"top\">

<p class=\"minititle\">{$inTitle}</p>
&nbsp; " . GetImageTag($inIconFilePath, "{$inAnchorName} icon");
	if ($inIconFilePath2)
		echo "&nbsp;&nbsp;&nbsp;" . GetImageTag($inIconFilePath2, "{$inAnchorName} icon 2");
	echo "

";
}

function EndSystemRequirementsItem()
{
	echo "</td></tr></table>
</td></tr></table>
";
}

function PrintSystemRequirementsBlurb($inBlurbText)
{
	echo "<p class=\"desc\">
{$inBlurbText}
</p>

";
}

?>


<?php
BeginSystemRequirementsItem("Audio Unit plugins for Mac OS X", "mac_au", "x.gif", "x-au.gif");
PrintSystemRequirementsBlurb("Our Audio Unit plugins for Mac OS X require Mac OS X 10.3.9 (&quot;Panther&quot;) or higher.  They run natively on PowerPC-based Macs and Intel-based Macs.  They are currently only available for 32-bit environments.");
PrintSystemRequirementsBlurb("The only exceptions are RMS Buddy versions 1.1 and 1.1.1, which run in Mac OS X 10.2 (&quot;Jaguar&quot;) or higher (and do not contain executable code for Intel processors), and Turntablist, which requires Mac OS X 10.4 (&quot;Tiger&quot;) or higher.  Skidder and Freeverb will also run in 64-bit environments.");
EndSystemRequirementsItem();
?>



<?php
BeginSystemRequirementsItem("VST plugins for Windows", "windows", "win32.gif");
PrintSystemRequirementsBlurb("Our VST plugins for Windows require Windows 95 or higher.  They are currently only available for 32-bit environments.");
EndSystemRequirementsItem();
?>



<?php
BeginSystemRequirementsItem("VST plugins for Mac OS X", "mac_vst", "x.gif");
PrintSystemRequirementsBlurb("Our VST plugins for Mac OS X require Mac OS X 10.1 (&quot;Puma&quot;) or higher.  They are CFM code format for PowerPC processors.  They may run under the &quot;Rosetta&quot; environment for Intel processors, but they do not contain native x86 executable code, nor are they available in the native Mac OS X code format Mach-O (we made the decision to discontinue VST development for Mac before Steinberg released their woefully belated VST SDK for Mach-O).");
EndSystemRequirementsItem();
?>



<?php
BeginSystemRequirementsItem("VST plugins &amp; applications for old Mac OS", "oldmac_vst", "mac.gif");
PrintSystemRequirementsBlurb("Our software for old Mac OS will run in Mac OS versions 8.5 through 9.2.2.");
EndSystemRequirementsItem();
?>



<!-- bottom -->

<table class="fullwidth">
<tr>
<td>&nbsp;</td>
<td style="width: 250px ; background: black">
<a href="./"><?php echo GetImageTag("smel1.gif", "Destroy FX: a "); ?></a><a href="http://smartelectronix.com/"><?php echo GetImageTag("smel2.gif", "Smartelectronix"); ?></a><?php echo GetImageTag("smel3.gif", " member"); ?>
</td>
</tr>
</table>

</td><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;&nbsp;</td></tr>
</table>

</body>

</html>
