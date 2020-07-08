<?php
	include("../dfx-common.php");
	include("../dfx-software.php");
	$SHOW_DONATE_LINKS = FALSE;
	PrintPageHeader("Destroy FX Extras  [handies &amp; stupids]", "#60418E", "purple.gif", FALSE, TRUE, "../");
?>

<table class="container" style="width: 550px">
<tr><td style="width: 30px">&nbsp;</td>
<td style="width: 375px" valign="top">
<?php echo GetImageTag("handy-title.gif", "Handy Destroy FX", "right") . "\n"; ?>
<br class="left" /><br class="right" />



<!-- informations -->

<?php BeginInfoBox("Extras", "dfx-extras.gif"); ?>

<p class="intro">
Hi.  This web page has more stuff by Sophia and <a href="http://tom7.org/">Tom 7</a>.  This stuff is not at our <a href="../">main web page</a> because it is not as exciting as that stuff.  We like to keep this stuff a little more tucked away because it's not our really good work.  Here you'll find <a href="<?php echo AUDIOUNIT_URL; ?>">Audio Units</a> and <a href="<?php echo VST_URL; ?>">VST plugins</a> that are either <a href="#handies">useful for musicians</a>, <a href="#devtoolz">useful for plugin developers</a>, or just <a href="#stupids">stupid and gimmicky</a>.
</p>

<p class="intro">
These plugins are free for you and your friends to use. You can use them for whatever you like, but we'd appreciate it if you:
</p>

<ul>
	<li><a href="http://msg.spacebar.org/f/a/s/msg/disp?n=10">tell us</a> if you use them in a song that you release</li>
	<li>let others know where they can find this website</li>
</ul>

<p class="intro">More information and better plugins are available at <a href="../">our main page</a>.
</p>

<?php EndInfoBox(); ?>



<p></p>
<p class="center_image"><a href="../"><?php echo GetImageTag("see-our-real-plugins.gif", "please see our real plugins"); ?></a></p>



<!-- RMS Buddy -->
<?php
BeginInfoBox("RMS Buddy", "dfx-rmsbuddy.gif", "rmsbuddy", TRUE, "handies");
PrintCurrentSoftwareDownloads("RMS Buddy", FALSE);
PrintCurrentSoftwareVersion("1.1.5", "2007-12-07", "AU");
PrintCurrentSoftwareVersion("1.1", "2003-02-25", "VST");

PrintSoftwareDescription("RMS Buddy is a handy tool for tracking RMS and peak volume information.  It provides a cumulative, running average RMS of the audio signal, momentary RMS values, and it keeps track of the peak volume levels.  I find it very useful when I'm mastering stuff.");

EndInfoBox();
?>



<!-- Monomaker -->
<?php
BeginInfoBox("Monomaker", "dfx-monomaker.gif", "monomaker", TRUE);
PrintCurrentSoftwareDownloads("Monomaker");
PrintCurrentSoftwareVersion("1.0.1", "2002-01-01");

PrintSoftwareDescription("Monomaker is a simple mono-merging and stereo-recentering utility.  You can use it to progressively mix down a stereo signal to mono and you can use it for equal power, center-shifting pans.");

EndInfoBox();
?>


<!-- MIDI Gater -->
<?php
BeginInfoBox("MIDI Gater", "dfx-midigater.gif", "midigater", TRUE);
PrintCurrentSoftwareDownloads("MIDI Gater");
PrintCurrentSoftwareVersion("1.0", "2001-11-19");

PrintSoftwareDescription("MIDI Gater is a simple, handy, MIDI-controlled gate.  
When you play a MIDI note, audio turns on.  
When you release the note, audio turns off.");
PrintSoftwareDescription("MIDI Gater is a little unusual, though, because it is \"polyphonic.\"  What that means is that a copy of the audio signal is created for every MIDI note, and the volume of each copy is sensitive to note velocity.  The level of sensitivity is adjustable.  The volume between notes is also adjustable, in case you don't want your audio signal turning off completely.");

EndInfoBox();
?>



<!-- EQ Sync -->
<?php
BeginInfoBox("EQ Sync", "dfx-eqsync.gif", "eqsync", TRUE, "stupids");
PrintCurrentSoftwareDownloads("EQ Sync");
PrintCurrentSoftwareVersion("1.0.1", "2002-05-02");

PrintSoftwareDescription("EQ Sync is a ridiculous equalizer.  You have no control over the EQ curve, if you try to adjust the faders they will escape, and the EQ parameters don't make any sense anyway.  The EQ setting changes on its own according to your song's tempo.");

EndInfoBox();
?>



<!-- Polarizer -->
<?php
BeginInfoBox("Polarizer", "dfx-polarizer.gif", "polarizer", TRUE);
PrintCurrentSoftwareDownloads("Polarizer");
PrintCurrentSoftwareVersion("1.0.1", "2002-08-05");

PrintSoftwareDescription("Polarizer takes every n<span style=\"font-size: smaller\"><sup>th</sup></span> sample and polarizes it.  The number of samples skipped and the strength of the polarization are adjustable.  You can also implode your sound.");

EndInfoBox();
?>



<!-- block test -->
<?php
BeginInfoBox("block test", "dfx-blocktest.gif", "blocktest", TRUE, "devtoolz");
PrintCurrentSoftwareDownloads("block test", FALSE);
PrintCurrentSoftwareVersion("1.0.1", "2002-02-13");

PrintSoftwareDescription("Block test is a pair of VST plugins:  
block test effect and block test synth.  
They provide useful information for VST plugin developers.  
Both of them display the current processing block size and responses to a couple  of hostCanDos.
Block test effect also writes a file that logs all of the 
host's responses to hostCanDos and product inquiries.  
It logs VstTimeInfo to a file, too.  Block test synth logs VstMidiEvents.");

EndInfoBox();
?>



<!-- VST MIDI -->
<?php
BeginInfoBox("VST MIDI", "dfx-vstmidi.gif", "vstmidi", TRUE);
PrintCurrentSoftwareDownloads("VST MIDI", FALSE, FALSE);
PrintCurrentSoftwareVersion("1.1.1", "2001-10-06");

PrintSoftwareDescription("VST MIDI is a little program that lets you explore the wonders of MIDI-controlled effects.  Unfortunately, most VST softwares do not support sending MIDI notes and such to effects plugins, so this is an easy way for you to use our fancy, MIDI-hungry effects if your software can't handle that kind of thing.  With VST MIDI, you can play a sound file, load a VST effect, control the effect with a MIDI instrument or the on-screen keyboard, and save the results to a sound file.");
PrintSoftwareDescription("VST MIDI was built with <a
href=\"http://www.cycling74.com\">Max/MSP</a>. The uncompiled patch is
also available here if you'd like to play with that.");

EndInfoBox();
?>



<!-- VST GUI Tester -->
<?php
BeginInfoBox("VST GUI Tester", "dfx-vstguitester.gif", "vstguitester", TRUE);
PrintCurrentSoftwareDownloads("VST GUI Tester", FALSE, FALSE);
PrintCurrentSoftwareVersion("1.0", "2001-11-19");

PrintSoftwareDescription("VST GUI Tester handy little program for VST plugin developers who are working on designing the graphical interfaces for their plugins.  It makes testing out new builds very, very easy.  You don't need to quit and restart the app, just close your plugin's editor window and then re-open by hitting any key.  Easy.");
PrintSoftwareDescription("VST GUI Tester was built with <a
href=\"http://www.cycling74.com\">Max/MSP</a>.  The uncompiled patch is
also available here if you'd like to play with that.");

EndInfoBox();
?>



<!-- Please go to our other web page. -->
<!--
<p class="center"><a href="../"><blink><marquee>
<font size="+2" color="#38aj83">p</font><font size="+3" color="#38sk38">L</font><font size="+4" color="#d8k39s">e</font><font size="+1" color="#832s72">a</font><font size="+4" color="#38sj38">s</font><font size="+3" color="#dj28sk">e</font> &nbsp;&nbsp;&nbsp;
<font size="+2" color="#d83k3c">v</font><font size="+3" color="#38s837">i</font><font size="+1" color="#27js82">s</font><font size="+0" color="#27s8sk">1</font><font size="+3" color="#38sj38">+</font> &nbsp;&nbsp;
<font size="+0" color="#38sj38">o</font><font size="+4" color="#38su38">U</font><font size="+2" color="#38e93j">r</font> &nbsp;&nbsp;&nbsp;
<font size="+1" color="#d6shwu">m</font><font size="+2" color="#38s73j">a</font><font size="+0" color="#dj93js">i</font><font size="+4" color="#38sk27">n</font> &nbsp;&nbsp;&nbsp;
<font size="+3" color="#08f3a1">w</font><font size="+1" color="#8af3b3">e</font><font size="+2" color="#s8dj32">b</font> &nbsp;&nbsp;&nbsp;
<font size="+4" color="#393472">p</font><font size="+3" color="#239sj3">a</font><font size="+0" color="#3928sk">g</font><font size="+1" color="#s93jf8">e</font><font size="+3" color="#32efa7">.</font>
</marquee></blink></a></p>
<p>
-->

<br />
<p class="center_image"><a href="../"><?php echo GetImageTag("see-our-real-plugins.gif", "please see our real plugins"); ?></a></p>



<!-- email list -->

<?php BeginInfoBox("email list", "dfx-list-purple.gif", "join"); ?>

<form class="small" action="http://spacebar.org/f/a/list/add" method="post">
<p class="plain">
<input type="hidden" name="list" value="1" />
your name:
<br /><input type="text" class="singleline" name="name" maxlength="128" />
<br />your e-mail:
<br /><input type="text" class="singleline" name="email" maxlength="128" />
<br /><input type="checkbox" name="lists" value="2" checked="checked" /> (check to also join Smartelectronix plugins announcement list)
<br /><input type="submit" value="Join!" style="border-style:solid ; border-color:black" />
</p>
</form>
</td>

<td valign="top">
<p class="desc">Join the Destroy FX mailing list to be alerted of
new plugin releases. It's low traffic, easy to sign up, and we'll
never give your name or email address out to anyone, for any reason.</p>

<?php EndInfoBox(); ?>


<!-- bottom -->

<br class="left" />
<table class="fullwidth">
<tr>
<td>&nbsp;</td>
<td style="width: 250px">
<a href="../"><?php echo GetImageTag("../smel1.gif", "Destroy FX: a "); ?></a><a href="http://smartelectronix.com/"><?php echo GetImageTag("../smel2.gif", "Smartelectronix"); ?></a><?php echo GetImageTag("../smel3.gif", " member"); ?>
</td>
</tr>
</table>

</td><td style="width: 151px"></td></tr>
</table>

</body>

</html>
