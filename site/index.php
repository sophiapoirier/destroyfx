<?php
//	session_start();
	define("PAGE_TITLE", "Destroy FX : free VST plugins, free Audio Units");
	include("dfx-common.php");
	include("dfx-software.php");
	PrintPageHeader(PAGE_TITLE, "#E88331", "orange.gif", TRUE, TRUE);
?>

<table class="container" style="width: 550px">
<tr><td style="width: 28px">&nbsp;</td>
<td style="width: 375px" valign="top">
<?php echo GetImageTag("title.gif", "Super Destroy FX", "right") . "\n"; ?>
<br class="left" /><br class="right" />



<?php
$sessionFormTokenExtraID = "dfx_contact";

function ProcessEmailContactForm()
{
	$emailsenderaddress = $_POST['emailsenderaddress'];
	$emailsendername = DecodeFormText( $_POST['emailsendername'] );
	$emailsubject = DecodeFormText( $_POST['emailsubject'] );
	$emailmessage = DecodeFormText( $_POST['emailmessage'] );
	$emailsenderurl = DecodeFormText( $_POST['emailsenderurl'] );
	$emailrecipientaddress = "REDACTED, REDACTED";
	$spamMessage = "We suspect this to be a spam attempt and will not send your message.";

	$errorstring = NULL;
	if (! $emailsenderaddress )
	{
		$errorstring = "You did not type in your email address!";
		$suggestion = "Please go back and enter your email address into the correct field.";
		$suggestion .= "\nWe can't reply to your message without knowing where to send our reply.";
	}
	elseif (! IsValidEmailAddressSyntax($emailsenderaddress) )
	{
		$errorstring = "<span class=\"emailaddress\">{$emailsenderaddress}</span> is not a valid email address!";
		$suggestion = "Please make sure that you typed your email address correctly.";
	}
	elseif (IsValidEmailAddressSyntax($emailmessage))
	{
		$errorstring = "You entered nothing but an email address in the message field!";
		$suggestion = "Try writing a message that is a message.";
	}
	elseif ( ContainsLineEndings($emailsubject) || ContainsLineEndings($emailsendername) )
	{
		$errorstring = "Line-endings are not allowed in single-line fields!";
		$suggestion = "Please cut it out.";
	}
	elseif (IsValidEmailAddressSyntax($emailsubject))
	{
		$errorstring = "You entered an email address as the subject field!";
		$suggestion = "We will not send a message that has an email address as its subject because those are usually junk mail.";
	}
	elseif (! $emailmessage )
	{
		$errorstring = "You left the message field blank!";
		$suggestion = "Please go back and enter your message into the correct field.";
	}
	elseif ($emailsubject === PAGE_TITLE)
	{
		$errorstring = "Your subject field is the title of this webpage!";
		$suggestion = "We will not send a message that uses this webpage's title as its subject because those are usually junk mail.";
	}
/*
	elseif ( !CheckFormToken($sessionFormTokenExtraID) )
	{
		$errorstring = "We could not send your message.";
		$suggestion = "Please make sure that you have cookies enabled in your browser.";
	}
*/
	elseif (strip_tags($emailmessage) !== $emailmessage)
	{
		$errorstring = $spamMessage;
		$suggestion = "We will not send messages that contain HTML in them because those are usually junk mail.  Go back and remove the HTML from your message and if you still want to try and reach us.";
	}
	else
	{
		$refererURL = $_SERVER['HTTP_REFERER'];
		if ($refererURL)
		{
			$refererComponents = parse_url($refererURL);
			if ($refererComponents)
			{
				$refererName = $refererComponents['host'];
				if ($refererName)
				{
					$refererIP = gethostbyname($refererName);
					$serverIP = $_SERVER['SERVER_ADDR'];
//					$serverName = $_SERVER['HTTP_HOST'];
					if ($refererIP && $serverIP)
					{
						if ($refererIP != $serverIP)
//						if ($refererName != $serverName)
						{
							$errorstring = $spamMessage;
$errorstring = NULL;
$extraFooter = "
WARNING!!!
referer IP = {$refererIP}
server  IP = {$serverIP}";
						}
					}
				}
			}
		}
	}

	BeginInfoBox(NULL, NULL);
	echo "<p>\n";

	if (! $errorstring )
	{
		$emailmessage .= "\n\n\n";
		if ($emailsenderurl)
			$emailmessage .= "\nURL:  " . $emailsenderurl;
		$emailmessage .= "\nIP:  " . gethostbyaddr( $_SERVER['REMOTE_ADDR'] );
if ($extraFooter) $emailmessage .= "\n" . $extraFooter;

		$emailsubject_common = "Destroy FX web contact";
		if ($emailsubject)
			$emailsubject .= " ({$emailsubject_common})";
		else
			$emailsubject = "-- {$emailsubject_common} --";

		if ($emailsendername)
		{
			$emailsendername = str_replace("\"", "\\\"", $emailsendername);
//			$emailsendername = strtr( $emailsendername, array("\"" => "") );
			$fromfield = "\"{$emailsendername}\" <{$emailsenderaddress}>";
		}
		else
			$fromfield = $emailsenderaddress;

		$success = mail($emailrecipientaddress, $emailsubject, $emailmessage, "From: ".$fromfield);
		if ($success)
		{
			echo "Thank you for sending your message!\n";
		}
		else
		{
			$errorstring = "Your message failed to send!";
			$suggestion = "Maybe you need to make sure that you typed your email address correctly?  (Does {$emailsenderaddress} seem right?)  Or we might just be having some system error, sorry.";
		}
	}

	if ($errorstring)
	{
		echo "<em class=\"emailerror\">BZZZT!!!</em>\n";
		echo "</p>\n\n";
		echo "<p>\n" . $errorstring . "\n";
		if ($suggestion)
		{
			echo "</p>\n\n";
			echo "<p>\n" . $suggestion . "\n";
		}
	}

	echo "</p>\n\n";

	$returnURL = $_SERVER['HTTP_REFERER'];
	if ($returnURL)
		echo "<p>\n<a href=\"{$returnURL}\">&larr; go back</a>\n</p>\n\n";

	EndInfoBox();
	echo "</td><td style=\"width: 151px\"></td></tr>\n</table>\n\n";
	echo "</body>\n</html>\n";
	exit;
}

if ( isset($_POST['sendemail']) )
{
	ProcessEmailContactForm();
}
?>



<!-- informations -->

<?php BeginInfoBox("introducing Destroy FX", "dfx-info.gif"); ?>

<p class="intro">
<b>Hello.</b>  Super Destroy FX is a collection of 
<a href="<?php echo AUDIOUNIT_URL; ?>">Audio Units</a> and 
<a href="<?php echo VST_URL; ?>">VST plugins</a> 
by Sophia and 
<a href="http://tom7.org/">Tom 7</a>. 
</p>

<p class="intro">
These plugins are free for you and your friends to use.  You can use them for whatever you like, but we'd appreciate it if you:
</p>
<ul>
	<li><a href="http://msg.spacebar.org/f/a/s/msg/disp/10">tell us</a> if you use them in a song that you release</li>
	<li>let others know where they can find this website</li>
<!--	<li><a href="donate.php">make a donation</a>, if you feel like these plugins are worth something to you</li>-->
</ul>

<p class="intro">
The <a
href="source.php">source code</a> for some of these plugins (and many unfinished plugins) is available for your perusal and modification.  You can also download complete project archives on this page.
</p>

<p class="intro">
Please direct your comments, suggestions, and questions to the <a href="http://msg.spacebar.org/f/a/s/msg/disp/10">guestbook / message board</a>, or <a href="#contact">email us</a>.
</p>

<p class="intro">
News and release infos are <a href="news.php">here</a>.  
Installation instructions are <a href="faq.html#installation">here</a> (in our FAQ).
Answers to recurring questions can be found at our <a href="faq.html">FAQ</a> page.
Outdated versions of our software are archived in our <a href="museum.php">museum</a>.
</p>

<?php EndInfoBox(); ?>



<!-- quick links -->

<p></p>
<table class="infobox" id="quicklinks">
<tr><td>

<p class="quicklinks">
<a href="news.php">news &amp; release infos</a> | 
<a href="faq.html">FAQ</a> | 
<a href="docs.html">documentation</a>
<br />
<a href="extras/">Extras</a> | 
<a href="museum.php">Museum</a> | 
<!--<a href="donate.php">donate</a> | -->
<a href="source.php">source code</a>
<br />
<a href="#contact">contact</a> | 
<a href="http://msg.spacebar.org/f/a/s/msg/disp/10">message board</a> | 
<a href="audiounits.html">Audio Units</a>
<br />
<a href="faq.html#installation">installation instructions</a> | 
<a href="sysreq.php">system requirements</a>
<br />
<a href="#biz">for hire</a> | 
<a href="http://smartelectronix.com/">Smartelectronix</a>
</p>

</td></tr></table>



<!-- Audio Units -->

<?php BeginInfoBox("Audio Units", "dfx-audio-units.gif", NULL, TRUE); ?>

<a href="audiounits.html"><?php echo GetImageTag("au-logo.gif", "Destroy FX Audio Units web page", "center"); ?></a>
</td>

<td valign="top">
<p class="desc">
We have Audio Unit versions of our plugins at our <a href="audiounits.html">Audio Units web page</a>.  They are mostly complete, but until they are 100% complete, we're going to keep them on a separate web page.  They are universal binaries.
</p>

<?php EndInfoBox(); ?>






<?php	// Geometer
BeginInfoBox("Geometer", "dfx-geometer.gif", "geometer", TRUE);
PrintCurrentSoftwareDownloads("Geometer");
PrintCurrentSoftwareVersion("1.1.1", "2005-09-09", "Windows");
PrintCurrentSoftwareVersion("1.1", "2003-01-08", "Mac");

PrintSoftwareDescription("Geometer is a visually oriented waveform geometry plugin.  It works by generating some \"points\" or \"landmarks\" on the waveform, moving them around and messing them up, and then reassembling the wave from the points.  Each of these three stages can be done in a variety of ways, combining to make millions of different effects.  However, Geometer has a real-time display of what your settings are doing to the sound, and a prominent built-in help system to explain what each setting does.");

BeginAudioSamples();
	PrintAudioSample("Theme from shaskell", "tom7=shaskell.mp3", "tom7=shaskellgeometer-8-10-02.mp3");
	PrintAudioSample("Mainframes", "mainframes-dry.mp3", "mainframes-excerpt.mp3");
EndAudioSamples();

EndInfoBox();
?>



<?php	// Scrubby
BeginInfoBox("Scrubby", "dfx-scrubby.gif", "scrubby", TRUE);
PrintCurrentSoftwareDownloads("Scrubby");
PrintCurrentSoftwareVersion("1.0", "2002-07-08");

PrintSoftwareDescription("Scrubby goes zipping around through time.  Speeding up or slowing down playback, moving backwards or forwards, Scrubby does whatever it takes to reach its destinations on time.  If necessary, you can discipline Scrubby by using pitch constraint control, and of course you have a choice between robot mode and DJ mode.");

BeginAudioSamples();
	PrintAudioSample("nighttime", "nighttime-clean-sounds.mp3", "nighttime-excerpt.mp3");
	PrintAudioSample("Bigger Or Not", "bigger-or-not-dry.mp3", "bigger-or-not-excerpt.mp3");
EndAudioSamples();

EndInfoBox();
?>



<?php	// Buffer Override
BeginInfoBox("Buffer Override", "dfx-bufferoverride.gif", "bufferoverride", TRUE);
PrintCurrentSoftwareDownloads("Buffer Override");
PrintCurrentSoftwareVersion("2.0", "2002-07-08");

PrintSoftwareDescription("Buffer Override can overcome your host app's audio processing buffer size and then (unsuccessfully) override that new buffer size to be a smaller buffer size.  It makes a lot more sense if you just try it out and hear what it does.  It can sound like a stuttery vocoder or a stuck beat shuffler or many other delightful things.  In certain hosts, you can also \"play\" Buffer Override via MIDI notes and even sync it to song tempo.");

BeginAudioSamples();
	PrintAudioSample("The Ballad Of Buffer Override", NULL, "ballad-of-buffer-override.mp3");
	PrintAudioSample("Skeletor Pro|24", "skeletor-pro24-dry.mp3", "skeletor-pro24-bo.mp3");
	PrintAudioSample("BO beat sync", "bo-beat-sync-example-dry.mp3", "bo-beat-sync-example-wet.mp3");
EndAudioSamples();

EndInfoBox();
?>



<?php	// Transverb
BeginInfoBox("Transverb", "dfx-transverb.gif", "transverb", TRUE);
PrintCurrentSoftwareDownloads("Transverb");
PrintCurrentSoftwareVersion("1.4", "2002-07-08");

PrintSoftwareDescription("Transverb is like a delay plugin, but it can play back the delay buffer at different speeds.  Think of it like a tape loop with two independently-moving read heads.  There are lots of parameters to control and a parameter randomizer for the impatient.  Tom's first \"released\" plugin. Fun!");

BeginAudioSamples();
	PrintAudioSample("Glup Drums", "unglup-beat.mp3", "glup-beat.mp3");
	PrintAudioSample("St. Thomas Aquinas", "aquinas-dry.mp3", "transverb-demo-aquinas.mp3");
	PrintAudioSample("I, Transverb", "i-resign.mp3", "tom7=09-02-01-i-transverb.mp3");
EndAudioSamples();

EndInfoBox();
?>



<?php	// Rez Synth
BeginInfoBox("Rez Synth", "dfx-rezsynth.gif", "rezsynth", TRUE);
PrintCurrentSoftwareDownloads("Rez Synth");
PrintCurrentSoftwareVersion("1.2", "2002-07-08");

PrintSoftwareDescription("Rez Synth allows you to \"play\" resonant bandpass filter banks that process your sound.  In the right hosts, MIDI notes can trigger individual filters or banks of filters (up to 30 per note) with controllable frequency separation.");

BeginAudioSamples();
	PrintAudioSample("Hrvatski Break", "hrvatski.break.original.mp3", "hrvatski.break.rezsynth.mp3");
EndAudioSamples();

EndInfoBox();
?>



<?php	// Skidder
BeginInfoBox("Skidder", "dfx-skidder.gif", "skidder", TRUE);
PrintCurrentSoftwareDownloads("Skidder");
PrintCurrentSoftwareVersion("1.4", "2002-07-08");

PrintSoftwareDescription("Skidder turns your sound on and off.  While that may not sound very rewarding, you have a great deal of control over the rate, pulsewidth, on/off slope, panning width, etc.  You can also relinquish some control to randomization.  Skidder can also sync with your song tempo and can be triggered by MIDI notes.");

BeginAudioSamples();
	PrintAudioSample("Ultra-Corrosion", "ultra-corrosion-excerpt.mp3", "ultra-corrosion-skidded.mp3");
EndAudioSamples();

EndInfoBox();
?>



<!-- Extras -->

<br class="left" />
<?php BeginInfoBox("Extras", "dfx-extras.gif", "extras"); ?>

<p class="desc">We also have more plugins.  They're on 
<a href="extras/">another page</a>.  They're not as exciting as the stuff here on our main page.  They're for people looking for <a href="extras/">extra stuff</a> 
(handy little things, stupid gimmicks, etc.).
</p>

<?php EndInfoBox(); ?>



<!-- biz -->

<br class="left" />
<?php BeginInfoBox("consulting", "consulting.gif", "biz"); ?>

<!--
<p align="center" class="tidbit"><b>$ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $</b></p>
-->
<p class="desc">
<!--
The Destroy FX team is available for paid gigs.  Here are projects that we have done:
-->
The Destroy FX team is not currently available for hire, but here are projects that we have done:
</p>
</td></tr>
</table>

<br class="left" />

<table class="fullwidth">
<tr>
<td class="iconcolumn" valign="top">
<a href="http://sfxmachine.com/"><?php echo GetImageTag("sfx-small.gif", "SFX Machine RT"); ?></a>
</td>
<td valign="top">
<p class="plain">
<a href="http://sfxmachine.com/">SFX Machine RT</a> - A cool multi-effects audio suite capable of very unique, bizarre, and wonderful sound mangling.  Sophia did the real-time update of this plugin for Audio Unit and VST formats.  Sophia also developed the Backwards Machine plugin.
</p>
</td></tr></table>

<br class="left" />

<table class="fullwidth">
<tr>
<td class="iconcolumn" valign="top">
<a href="http://cycling74.com/"><?php echo GetImageTag("pluggo-small.jpg", "Pluggo"); ?></a>
</td>
<td valign="top">
<p class="plain">
<a href="http://cycling74.com/">Pluggo</a> - The never ending plugin.  Pluggo is a collection of 100+ unusual and great effects and instruments.  Plus, it also lets you turn your Max/MSP creations into plugins that you can use in other apps.  Sophia did the Audio Unit update for Pluggo (plus a bit of work on Max/MSP).
</p>
</td></tr></table>

<br class="left" />

<table class="fullwidth">
<tr>
<td class="iconcolumn" valign="top">
<a href="http://ursplugins.com/"><?php echo GetImageTag("urs-logo-small.jpg", "URS"); ?></a>
</td>
<td valign="top">
<p class="plain">
<a href="http://ursplugins.com/">URS Classic Console Plug-Ins</a> - These are faithful, high-end models of classic hardware equalizers and compressors.  Sophia did the Audio Unit and VST versions of all of their plugins as well as the RTAS/AudioSuite versions of a some of them.
</p>
</td></tr></table>

<br class="left" />

<table class="fullwidth">
<tr>
<td class="iconcolumn" valign="top">
<a href="http://abbeyroadplugins.com/"><?php echo GetImageTag("abbeyroad.gif", "Abbey Road"); ?></a>
</td>
<td valign="top">
<p class="plain">
<a href="http://abbeyroadplugins.com/">Abbey Road Plug-Ins</a> - These are authentic recreations of Abbey Road Studios equipment.  Sophia did the Audio Unit and VST versions of most of their plugins.
</p>
</td></tr></table>

<br class="left" />

<table class="fullwidth">
<tr>
<td class="iconcolumn" valign="top">
<a href="http://refusesoftware.com/"><?php echo GetImageTag("refuse-logo.gif", "reFuse Software"); ?></a>
</td>
<td valign="top">
<p class="plain">
<a href="http://refusesoftware.com/">reFuse Software</a> - Sophia did the native Audio Unit, VST, and RTAS versions of their Lowender subharmonic bass synthesizer plugin.
</p>
<!--
</td></tr></table>

<br class="left" />

<table class="fullwidth">
<tr><td>
<p class="desc">If you would like to buy us, please <a href="#contact">get in touch.</a></p>
-->
<!--
<p align="center" class="tidbit"><b>$ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $ $</b></p>
-->

<?php EndInfoBox(); ?>



<!-- contact form -->

<br class="left" />
<?php BeginInfoBox("contact us", "dfx-contact.gif", "contact"); ?>

<p class="desc">
You can use this form to send us an email.
Please consider looking at our <a href="faq.html">Frequently Asked Questions</a> page first, though.
</p>

<br class="left" />
<!--
<form action="http://dfx.spacebar.org/f/a/form/doform" method="post">
-->
<form action="./" method="post">
<!--
<div><input type="hidden" name="n" value="6" /></div>
<table class="fullwidth">
<tr><td><input class="singleline" type="text" name="name" /></td><th>name</th></tr>
<tr><td><input class="singleline" type="text" name="email" /></td><th>email</th></tr>
<tr><td><input class="singleline" type="text" name="url" /></td><th>url</th></tr>
<tr><td colspan="2"><textarea style="width:321px" name="comments" rows="15" cols="22"></textarea></td><th>&nbsp;</th></tr>
-->
<div>
	<input type="hidden" name="sendemail" value="1" />
<?php
	InitializeFormToken($sessionFormTokenExtraID);
	echo "\t" . GetFormTokenTag() . "\n";
?>
</div>
<table class="fullwidth">
	<tr><td><input class="singleline" type="text" name="emailsendername" /></td><th>name</th></tr>
	<tr><td><input class="singleline" type="text" name="emailsenderaddress" /></td><th>email</th></tr>
	<tr><td><input class="singleline" type="text" name="emailsubject" /></td><th>subject</th></tr>
	<tr><td><input class="singleline" type="text" name="emailsenderurl" /></td><th>url</th></tr>
	<tr><td colspan="2"><textarea style="width:321px" name="emailmessage" rows="15" cols="22"></textarea></td><th>&nbsp;</th></tr>
	<tr><td><input type="submit" style="border-style:solid ; border-color:black ; width:100px ; height:22px" value="Send" /></td><th>send</th></tr>
</table>
</form>

<?php EndInfoBox(); ?>



<!-- email list subscribe -->

<?php BeginInfoBox("email list", "dfx-list.gif", "join"); ?>

<form class="small" action="http://spacebar.org/f/a/list/add" method="post">
<p class="plain">
<input type="hidden" name="list" value="1" />
your name:
<br /><input class="singleline" type="text" name="name" maxlength="128" />
<br />your e-mail:
<br /><input class="singleline" type="text" name="email" maxlength="128" />
<br /><input type="checkbox" name="lists" value="2" checked="checked" /> (check to also join Smartelectronix plugins announcement list)
<br /><input type="submit" value="Join!" style="border-style:solid ; border-color:black" />
</p>
</form>
</td>

<td valign="top">
<p class="desc">Join the Destroy FX mailing list to be alerted of new plugin releases.  
It's low traffic, easy to sign up, and we'll never give your name or email address out to anyone, for any reason.</p>

<?php EndInfoBox(); ?>



<br class="left" />
<p></p>
<table class="infobox" id="smexlinks">
<tr><td>

<p class="plain">
Say hello to the other <a href="http://smartelectronix.com/">Smartelectronix</a> members:<br />
<a href="http://alex.smartelectronix.com/">Alex</a>, 
<a href="http://antti.smartelectronix.com/">Antti</a>, 
<a href="http://bram.smartelectronix.com/">Bram</a>, 
<a href="http://dmi.smartelectronix.com/">dmi</a>, 
<a href="http://dunk.smartelectronix.com/">dunk</a>, 
<a href="http://andreas.smartelectronix.com/">ioplong</a>, 
<a href="http://jaha.smartelectronix.com/">Jaha</a>, 
<a href="http://koen.smartelectronix.com/">Koen</a>, 
<a href="http://magnus.smartelectronix.com/">Magnus</a>, 
<a href="http://mda.smartelectronix.com/">mda</a>, 
<a href="http://mdsp.smartelectronix.com/">mdsp</a>, 
<a href="http://deadskinboy.com/">Sean</a>
</p>


</td></tr>
</table>



<!-- bottom -->

<br class="left" />
<table class="fullwidth">
<tr>
<td>&nbsp;</td>
<td style="width: 250px">
<?php echo GetImageTag("smel1.gif", "Destroy FX: a "); ?><a href="http://smartelectronix.com/"><?php echo GetImageTag("smel2.gif", "Smartelectronix"); ?></a><?php echo GetImageTag("smel3.gif", " member"); ?>
</td>
</tr>
</table>



</td><td style="width: 151px"></td></tr>
</table>



</body>
</html>
