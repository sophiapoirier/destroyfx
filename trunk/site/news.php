<?php
	include("dfx-common.php");
	PrintPageHeader(DFX_PAGE_TITLE_PREFIX . "news &amp; release infos", "#EF8431");
?>

<table class="container" style="background-color: #FF9431 ; width: 600px ; margin-left: auto ; margin-right: auto">
<tr><td style="width: 30px ; padding: 0px">&nbsp;&nbsp;</td>
<td style="width: 100% ; padding: 0px" valign="top">
<a href="./"><?php echo GetImageTag("title.gif", "Super Destroy FX", "right"); ?></a>
<br class="left" /><br class="right" />


<?php

function BeginNewsItem($inTitle, $inDate, $inType, $inAnchorSuffix = NULL)
{
	$dateTimestamp = strtotime($inDate);
	if ( ($dateTimestamp !== FALSE) && ($dateTimestamp != -1) )
	{
		$dateString = date("F jS Y", $dateTimestamp);
		$dateString_short = date("n/j/Y", $dateTimestamp);
	}
	else
	{
		$dateString = $inDate;
		$dateString_short = $inDate;
	}

	$anchorName = MakeNewsAnchor($inDate, $inAnchorSuffix);
	echo "<!-- news {$dateString_short} -->

<p></p>
<table class=\"infobox\" id=\"{$anchorName}\"><tr><td>
<table class=\"fullwidth\"><tr><td valign=\"top\">

<p class=\"rel\">{$dateString}</p>
<p class=\"{$inType}\">{$inTitle}</p>

";
}

function EndNewsItem()
{
	echo "</td></tr></table>
</td></tr></table>
";
}

function PrintNewsBlurb($inBlurbText)
{
	echo "<p class=\"desc\">
{$inBlurbText}
</p>

";
}

?>


<?php
BeginNewsItem("released RMS Buddy 1.1.5 (Audio Unit)", "2007-12-07", "announce");

PrintNewsBlurb("Fixed a problem that caused the value read-outs to not update in Mac OS X 10.5 (Leopard).");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.1.4 (Audio Unit)", "2007-11-06", "announce");

PrintNewsBlurb("Added support for &quot;full compatibility mode&quot; running on Logic Nodes.");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.1.3 (Audio Unit)", "2006-06-30", "announce");

PrintNewsBlurb("Fixed a bug when running on Intel-based Macs where all of the controls and text displays appeared as black rectangles.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Geometer 1.1.1 for Windows", "2005-09-09", "announce");

PrintNewsBlurb("Fixed a bug that lead to settings corruption when switching between &quot;programs&quot;.  The Mac VST versions also suffer from this problem, but they will not be updated since they have been discontinued in favor of the Audio Unit version, which has never had this bug.  (thank you PaPQH8 for <a href=\"http://www.kvraudio.com/forum/viewtopic.php?t=89715\">pointing out this problem</a>)");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.1.2 (Audio Unit)", "2005-08-21", "announce");

PrintNewsBlurb("Fixed an audio rendering bug that caused it to fail Apple's AUValidation.");

PrintNewsBlurb("It is now a &quot;universal binary&quot; (runs natively on PowerPC and Intel Macs, though note that this now means that <i>Mac OS X 10.3.9 is the minimum system requirement</i>).");

PrintNewsBlurb("Redesigned the interface slightly.");

PrintNewsBlurb("All of the text is now localizable.");

PrintNewsBlurb("Now using the modern notification mechanism for parameter change gestures.");

EndNewsItem();
?>



<?php
BeginNewsItem("URS A Series EQ and N Series EQ Audio Units released", "2004-11-01", "tidbit");

PrintNewsBlurb("Sophia did the version 1.2.1 updates of these <a href=\"http://ursplugins.com/\">URS EQs</a>, making them available as Audio Units now.  These are not a Destroy FX plugins, but something that Sophia did as a job.");

EndNewsItem();
?>



<?php
BeginNewsItem("Pluggo 3.1 released", "2004-01-08", "tidbit");

PrintNewsBlurb("<a href=\"http://www.cycling74.com/products/pluggo.html\">Pluggo 3.1</a> for Mac OS X has been released.  It's not a Destroy FX plugin, but Sophia did the Audio Unit implementation for it, and it may interest Destroy FX fans.");

EndNewsItem();
?>



<?php
BeginNewsItem("Smart Electronix interview at d-i-r-t-y.com", "2003-11-16", "tidbit");

PrintNewsBlurb("There's an interview with the entire Smart Electronix crew (including Tom and Sophia) at the French online music magazine <a href=\"http://www.d-i-r-t-y.com/textes/int_smartelectronix.html\">d-i-r-t-y.com</a> (though our interview is in English).");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.1.1 (Audio Unit)", "2003-11-12", "announce");

PrintNewsBlurb("The GUI now adapts itself automatically when the number of audio channels changes.");

PrintNewsBlurb("The GUI now works properly in compositing windows.");

PrintNewsBlurb("Fixed a bug where, in some circumstances, the GUI text displays showed bogus values when initially opened.");

EndNewsItem();
?>



<?php
BeginNewsItem("Destroy FX review in May issue of Electronic Musician", "2003-04-24", "tidbit");

PrintNewsBlurb("There is a review of the Destroy FX plugins collection (focusing on Buffer Override, Geometer, and Rez Synth) in the May 2003 issue of <a href=\"http://emusician.com/\">Electronic Musician</a> magazine.  It is a lovely review.  We got 4 out of 5 goodness points.  They even made <a href=\"http://emusician.com/ar/emusic_em_web_clips_8/index.htm\">sound clips</a> for the article.");

EndNewsItem();
?>



<?php
BeginNewsItem("SFX Machine RT released", "2003-04-18", "tidbit");

PrintNewsBlurb("SFX Machine RT is a modular effects processor plugin that comes with almost 300 effect configurations, from utilities to conventional effects to really crazy, unusual effects.  It's not a Destroy FX plugin, but it's the fruit of Sophia's past several months' labor (Sophia's day job).  You might like to check it out:  <a href=\"http://sfxmachine.com/\">sfxmachine.com</a>  It's available for Mac OS X, old Mac OS, and Windows, in Audio Unit and VST formats.");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.1", "2003-02-25", "announce");

PrintNewsBlurb("The most important improvement in this update is that the RMS values are now correct.  The prior versions of RMS Buddy did the RMS calculations backwards.  Woops.  (thank you Hans Sj&ouml;blad for pointing out this problem)");

PrintNewsBlurb("This update also adds a new analysis display field:  continual peak (thank you Maan Hamze for suggesting that feature)");
?>

<p class="desc">
The Audio Unit version also has some other nice improvements:
</p>
<ul>
	<li>The value read-outs are much more reliable and occur on a dependable periodic basis.  The behavior is  consistent between different host apps.</li>
	<li>Because the AU format allows for that reliability, I also added an analysis window size parameter that lets you adjust the rate at which calculations occur and get displayed.</li>
	<li>Also, the Audio Unit version can process any number of channels, not just stereo.  The graphical interface adapts to the number of channels being processed.</li>
	<li>The Audio Unit version is also a bit more efficient.</li>
</ul>

<p class="plain">
None of those improvements were possible using the VST plugin format, that's why only the Audio Unit version gets them.
</p>

<?php
EndNewsItem();
?>



<?php
BeginNewsItem("article about Audio Units in March issue of KEYBOARDS (including statement from Sophia)", "2003-02-04", "tidbit");

PrintNewsBlurb("There is an article about Audio Units in the March 2003 issue of the German magazine <a href=\"http://www.keyboards.de/\">KEYBOARDS</a>.  The article includes a statement from Sophia about Audio Units in general and Destroy FX Audio Units.  I don't think that there's a way to view the article online, but I did find <a href=\"http://www1.keyboards.de/magazine/m0303/303104.html\">this link</a> which says something about the article and has a couple photos of Sophia.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Geometer 1.1", "2003-01-08", "announce");

PrintNewsBlurb("Geometer VST now processes in stereo rather than mono.  There are no other new features, so if you prefer mono processing, don't update to version 1.1.  (Note:  The Audio Unit version can still process any number of channels.)");

EndNewsItem();
?>



<?php
BeginNewsItem("<a href=\"http://destroyfx.org/\">destroyfx.org</a>", "2002-12-02", "tidbit");

EndNewsItem();
?>



<?php
BeginNewsItem("Destroy FX externals for PD", "2002-09-11", "tidbit");

PrintNewsBlurb("Martin Pi has kindly taken the time to port a few of our plugins to PD externals.  They have been compiled for the GNU-Linux version of <a href=\"http://www-crca.ucsd.edu/~msp/software.html\">PD</a>.  So far the collection includes Transverb, Skidder, Buffer Override, and Polarizer.  The externals are available <a href=\"http://attacksyour.net/pi/pd/\">here</a>.  Thank you Martin!");

EndNewsItem();
?>



<?php
BeginNewsItem("Logic X review at osxAudio.com (including interview bit with Sophia)", "2002-09-10", "tidbit");

PrintNewsBlurb("There is a review of Emagic's Logic for Mac OS X at <a href=\"http://osxaudio.com/\">osxAudio.com</a>.  The review includes a little interview with Sophia about Logic X and music plugins in Mac OS X.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Geometer 1.0", "2002-08-09", "announce");

PrintNewsBlurb("Geometer is a new effect by Tom");

EndNewsItem();
?>



<?php
BeginNewsItem("released Polarizer 1.0.1 VST and Audio Unit", "2002-08-05", "announce");

PrintNewsBlurb("Polarizer changed a little bit since 1.0, but I can't remember what changed.  The main reason for releasing this update was to include an &quot;experimental&quot; Audio Unit version of Polarizer.  This is just for curious folks; there's currently no custom graphical interface and there may be weird bugs.  We'll try harder next time...");

EndNewsItem();
?>



<?php
BeginNewsItem("released Scrubby 1.0", "2002-07-08", "announce");
?>

<p class="announce">released Buffer Override 2.0</p>

<p class="announce">released Transverb 1.4, Skidder 1.4, and Rez Synth 1.2</p>

<?php
PrintNewsBlurb("Scrubby is a brand new effect plugin by Sophia");
?>

<p class="desc">
Buffer Override 2.0 is major update of Buffer Override.  The main improvements are:
</p>
<ul>
	<li>LFOs to modulate the buffer divisor and forced buffer size (thank you Arne Van Petegem for the idea)</li>
	<li>Buffer Interrupt&trade; technologiez</li>
	<li>beautiful new graphics by <a href="http://code404.com/">Justin Maxwell</a></li>
	<li>assignable MIDI control of every parameter</li>
</ul>

<?php
PrintNewsBlurb("Rez Synth, Skidder, and Transverb also now feature assignable MIDI control of parameters");

PrintNewsBlurb("Skidder has a new MIDI note control mode called &quot;MIDI apply&quot; and an option for having note velocity control the floor (thank you Sascha Kujawa for the ideas)");

PrintNewsBlurb("Skidder also has easier to use &quot;range slider&quot; controls for the pulsewidth and floor parameters");

PrintNewsBlurb("fixed the denormals problems in Transverb which could sometimes cause CPU spikes while processing a silent signal (thank you Christian Bei&szlig;wenger for pointing out that problem)");

PrintNewsBlurb("the dry/wet mix in Buffer Override 2 and Rez Synth is now &quot;even power&quot;");

PrintNewsBlurb("fixed a problem that Transverb, Skidder, and Rez Synth had with updating certain displays in response to automation");

PrintNewsBlurb("changed velocity influence in Rez Synth to not be exponentially scaled (because that was stupid)");

PrintNewsBlurb("Rez Synth can utilize MIDI sustain pedal (probably, although I don't have a sustain pedal so I haven't actually tested this)");

PrintNewsBlurb("We have discontinued the &quot;food&quot; plugins for the Mac versions of Rez Synth, Skidder, and Buffer Override because they made the plugins incompatible with dual-CPU processing mode in Mac OS 9 and they are also unnecessary now that Logic 5 supports routing MIDI to VST effects.");

EndNewsItem();
?>



<?php
BeginNewsItem("interview at osxAudio.com", "2002-06-19", "tidbit");

PrintNewsBlurb("Tom &amp; Sophia are interviewed at <a href=\"http://osxaudio.com/\">osxAudio.com</a>");

EndNewsItem();
?>



<?php
BeginNewsItem("released EQ Sync 1.0.1", "2002-05-02", "announce");

PrintNewsBlurb("fixed a problem where the CPU usage could dramatically increase when processing a silent audio signal (denormals problem)");

PrintNewsBlurb("fixed a bug that could cause messed up audio when using EQ Sync as a &quot;send&quot; effect in Cubase or other hosts that handle send effects in the way that Cubase does");

PrintNewsBlurb("the autonomous, moving faders now ought to update properly in more (if not all) hosts");

EndNewsItem();
?>



<?php
BeginNewsItem("released Rez Synth 1.1, Skidder 1.3, &amp; Transverb 1.3", "2002-03-16", "announce");

PrintNewsBlurb("Transverb, Rez Synth, and Skidder all support MIDI program change messages for switching between presets");

PrintNewsBlurb("Transverb has a random \"preset\" now.  Combined with the MIDI program support, this means that you can now automate randomization using MIDI program change messages.  See the updated documentation for more information about that.");

PrintNewsBlurb("I fixed a few little display bugs in Skidder (there were sometimes wrong displays when the host automated certain parameters)");

PrintNewsBlurb("Skidder's tempo syncing is more compatible with Cubase now");

PrintNewsBlurb("Rez Synth handles pitchbend LSB now");

PrintNewsBlurb("the fades parameter on Rez Synth has been reversed, which can affect presets or saved songs that you have made with Rez Synth");

PrintNewsBlurb("The classic Mac OS distributions of Rez Synth and Skidder now include separate versions of the plugins in the Logic 4 workarounds folder.  This is because I learned that some aspects of the workaround that I developed are not compatible with dual-processor Macs, so people not using Logic 4 might as well not use those special versions (thank you Carty Fox for telling me about this problem).  Also, these workarounds are no longer necessary in Logic 5.  See the updated documentation for more about how to use Rez Synth and Skidder (and also MIDI Gater and Buffer Override) in Logic 5.");

PrintNewsBlurb("all of these plugins now have slightly more complete support of the VST standard");

EndNewsItem();
?>



<?php
BeginNewsItem("released RMS Buddy 1.0.1", "2002-03-01", "announce");

PrintNewsBlurb("RMS Buddy didn't always update its displays well, if at all, in some hosts (Massiva, FXpansion adapter, BUZZ adapter), but now it does.  (thanks for telling me about this problem, Maan Hamze)");

PrintNewsBlurb("RMS Buddy's continuous RMS displays are now always accurate, even in hosts that give infrequent idle processor time to plugins");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.2.3", "2002-02-25", "announce");

PrintNewsBlurb("When Transverb uses a highpass filter in ultra hi-fi mode (to avoid aliasing noise), the filter cutoff is a little higher so that things sound less muffled.");

PrintNewsBlurb("It's now easier to click on the quality, randomize, and TOMSOUND buttons because their neighboring text labels are now also clickable.");

EndNewsItem();
?>



<?php
BeginNewsItem("released block test 1.0.1", "2002-02-13", "announce");

PrintNewsBlurb("The writing of log files is now gentler in the Mac OS X and Windows versions.");

PrintNewsBlurb("Block test's graphics should open properly in all hosts (particularly an unreleased host that may have previously not been able to open the graphics properly).");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.2.2", "2002-01-28", "announce");

PrintNewsBlurb("Transverb's performance with high delay speeds in ultra hi-fi mode has been hugely improved");

PrintNewsBlurb("the sound quality with high speeds in ultra hi-fi mode has been significantly improved, too");

PrintNewsBlurb("Transverb now uses highpass filters on the delay heads when their speeds are below 0 in ultra hi-fi mode.  This is done in order to cut out super-low sub-audio bass frequencies that can result from the speed down.  (thanks for the suggestion, Arne Van Petegem)");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.2.1 for Windows", "2002-01-21", "announce");

PrintNewsBlurb("Woops.  The new quality button was missing altogether from the Windows release of Transverb 1.2.  Sorry about that.  Here's a working version.");

EndNewsItem();
?>



<?php
BeginNewsItem("article in Sound On Sound", "2002-01-19", "tidbit", "b");

PrintNewsBlurb("The February 2002 issue of Sound On Sound magazine features an article about Destroy FX.  You can read it online <a href=\"http://www.sospubs.co.uk/sos/feb02/articles/plugin0202.asp\">here</a>.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.2", "2002-01-19", "announce");

PrintNewsBlurb("Transverb has a new quality mode that reduces aliasing noise when speeding up delay heads past 0.  This is the \"ultra hi-fi\" mode that you can turn on by clicking the quality button until you see an exclamation point (!).  An X indicates the old hi-fi mode (with no protection against aliasing) and an empty box indicates the old \"dirt-fi\" mode.  Note that using ultra hi-fi quality will impose an extra processing load as you speed up a delay head past 0.  You reach the maximum load at about +4 octaves.");

PrintNewsBlurb("Transverb doesn't \"blow out\" (i.e. big pop and then silence) from high feedback levels nearly as often.  It's still possible (that is just the nature of digital feedback), but it's a lot harder to do now.");

PrintNewsBlurb("Fixed a display bug with Transverb's speed parameters where semitone values higher than 11.5 would be displayed as 0.0.");

PrintNewsBlurb("Transverb's graphics should open properly in all hosts (particularly an unreleased host that may have previously not been able to open the graphics properly).");

PrintNewsBlurb("Fixed a little something in Transverb that could sometimes cause a crash in old versions of Logic for Windows if you switched the plugin to \"controls\" view.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Buffer Override 1.1.2", "2002-01-03", "announce");

PrintNewsBlurb("Buffer Override now properly responds to the \"all notes off\" MIDI message.  Previously, this could cause some problems (like stuck notes) when using \"MIDI trigger\" control of Buffer Override in Cubase and maybe other hosts.  (thank you John Audette for making me aware of this problem)");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder 1.2.2, Rez Synth 1.0.8, &amp; Monomaker 1.0.1", "2002-01-01", "announce");

PrintNewsBlurb("Woops, our first bad bug...  There was a problem with the graphics in the last versions of Skidder and Rez Synth that could cause crashes when you opened, closed, and then reopened their editor windows.  The crashes only seemed to happen under Windows, but I've updated the Mac versions, too.  (thanks for pointing that problem out to me, Mashino Jun)");

PrintNewsBlurb("I also changed a little something in Skidder that could sometimes cause a crash in old versions of Logic for Windows if you switched the plugin to \"controls\" view.");

PrintNewsBlurb("Fixed a little problem in Monomaker where the handle on the pan fader wouldn't move when that parameter got automated.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder 1.2.1 &amp; Rez Synth 1.0.7", "2001-12-12", "announce");

PrintNewsBlurb("Internet links in Skidder and Rez Synth are no longer based on VST parameters.  Previously this could cause problems for people using things like Pluggo or Max/MSP to randomize parameters.  (thanks for pointing that problem out to me, Greg Davis)");

PrintNewsBlurb("Rez Synth has more \"sensible\" gain parameters now.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Buffer Override 1.1.1", "2001-12-02", "announce");

PrintNewsBlurb("Buffer Override now has a \"memory\" of which MIDI notes are active and also the order in which they were played (i.e. which came first, which came after that, etc.).  This means that if, for example, you play one note, press another note while still holding down the first note, and then release the second note, Buffer Override will remember that the first note is still active and go back to that note.  Previously, Buffer Override only heeded notes when they first started.  Now you can play quick trills and such and Buffer Override will more accurately reproduce what you play.");

PrintNewsBlurb("Internet links are no longer based on VST parameters.  Previously this could cause problems for people using things like Pluggo or Max/MSP to randomize parameters.  (thanks for pointing that problem out to me, Greg Davis)");

EndNewsItem();
?>



<?php
BeginNewsItem("Destroy FX extras", "2001-11-28", "announce");

PrintNewsBlurb("We announced our <a href=\"extras/\">Destroy FX extras</a> collection with Monomaker, RMS Buddy, Polarizer, EQ Sync, block test, MIDI Gater, &amp; VST GUI Tester.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Buffer Override 1.1 for classic Mac OS, OS X, &amp; Windows &amp; Buffer Food 1.0.1 for classic Mac OS", "2001-11-27", "announce");
?>

<p class="announce">released Monomaker 1.0 for classic Mac OS, OS X, &amp; Windows</p>

<?php
PrintNewsBlurb("Buffer Override has a new MIDI note control mode called MIDI trigger.  In this mode, the effect turns on and off when you play notes.  This way you can use MIDI notes to not only change the buffer divisor, but also to apply the effect only at certain moments.  The old mode is now called \"MIDI nudge.\"  Check the updated documentation for more details about MIDI trigger mode.  (thank you Sascha Kujawa for suggesting this)");

PrintNewsBlurb("Again, as with the recent Rez Synth and Skidder updates, I enhanced the same unimportant little VST things and did the same thing with Buffer Food that didn't matter.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder 1.2 for classic Mac OS, OS X, &amp; Windows", "2001-11-25", "announce");

PrintNewsBlurb("Skidder has a new parameter:  floor random minimum.  This is a lot like the pulsewidth random minimum except that it randomizes the floor.  (thank you James Dashow for suggesting this feature)");

PrintNewsBlurb("The \"rate random factor range\" display now gets updated properly when you type in a tempo value.");

PrintNewsBlurb("The rupture parameter's fader doesn't act \"funny\" anymore.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Rez Synth 1.0.6 &amp; Skidder 1.1.2 for classic Mac OS, OS X, &amp; Windows", "2001-11-23", "announce");

PrintNewsBlurb("Rez Synth's dry/wet mix now works correctly when Rez Synth is used as a send effect.");

PrintNewsBlurb("Rez Synth now always properly clears the feedback buffers of new notes.");

PrintNewsBlurb("Note-offs with Skidder in MIDI trigger mode used to sometimes cause slight audio glitches, but not anymore.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Polarizer, MIDI Gater, RMS Buddy, EQ Sync, &amp; block test 1.0 for classic Mac OS, OS X, &amp; Windows &amp; VST GUI Tester 1.0 for classic Mac OS", "2001-11-19", "announce");

PrintNewsBlurb("released but not announced yet (deescreet)");

EndNewsItem();
?>



<?php
BeginNewsItem("released Rez Synth 1.0.5 for classic Mac OS, OS X, &amp; Windows &amp; Rez Food 1.0.1 for classic Mac OS", "2001-11-16", "announce");

PrintNewsBlurb("Rez Synth consumes slightly less processing power now.");

PrintNewsBlurb("The same mundane stuff as with the previous Skidder update:  enhanced some unimportant little VST things and did something with Rez Food that didn't matter.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder 1.1.1 for classic Mac OS, OS X, &amp; Windows &amp; Skidder Food 1.0.1 for classic Mac OS", "2001-11-13", "announce");

PrintNewsBlurb("Fixed a problem using Skidder with Logic for Mac where the \"connect to food\" button showed up in the wrong place and therefore wouldn't work.  (thank you Igor for informing me about this problem)");

PrintNewsBlurb("Enhanced some minor VST things a bit.  Programs are now more thoroughly supported and host communication about what the plugin can do is a little more thorough, too.  This stuff probably won't make any difference in hardly any hosts.");

PrintNewsBlurb("I can't remember what I changed about Skidder Food, but it didn't make any difference, I remember that much.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.1 for classic Mac OS, OS X, &amp; Windows", "2001-10-29", "announce");

PrintNewsBlurb("Transverb's speed parameters have been changed and greatly enhanced so that they function in a much more musically useful fashion.  Check the updated documentation for full details (you really ought to if you want to get the most out of Transverb).");

PrintNewsBlurb("Note that, because the speed parameters work very differently in version 1.1, old presets or saved song settings from version 1.0 or 1.0.1 of Transverb will not sound the same with version 1.1.  But don't worry because there's now a little button in the lower right corner that you can press to update your speed parameter settings from the old style to the new style.");

PrintNewsBlurb("Fixed the problem with the OS X version that prevented the internet links from working.");

PrintNewsBlurb("The Windows version uses the non-beta VST graphics library now (the Mac versions have all along).");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder 1.1, Rez Synth 1.0.4, &amp; Buffer Override 1.0.4 for classic Mac OS, OS X, &amp; Windows", "2001-10-26", "announce");

PrintNewsBlurb("Skidder has a new parameter called \"floor.\"  This is the gain level in between skids.  This way, you can make it so that Skidder doesn't switch your sound completely on and off; it can be quiet/loud instead (or even loud/loud if that's what you really want).  The documentation has been updated with more details about this new parameter.  (thank you James Dashow for suggesting this feature)");

PrintNewsBlurb("Fixed the problem with the OS X versions that prevented the internet links from working.");

PrintNewsBlurb("Beat-sync in Skidder and Buffer Override is slightly more reliable now.");

PrintNewsBlurb("The Windows versions all use the non-beta VST graphics library now (the Mac versions have all along).");

PrintNewsBlurb("I slightly enhanced Rez Synth's and Buffer Override's graphics (most significantly, you can now manually type in values for Rez Synth's band separation parameter, in case you need totally precise values).");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder, Rez Synth, &amp; Buffer Override 1.0.3 &amp; Transverb 1.0.1 for classic Mac OS, OS X, &amp; Windows", "2001-10-10", "announce");

PrintNewsBlurb("All of our plugins now support loading and saving preset banks.  (thank you Ian for the suggesting this)");

PrintNewsBlurb("Skidder and Buffer Override now, when syncing to host tempo, start their cycles at the beginnings of measures (in hosts that support sending tempo and time information to plugins).  (thank you Luke Skirenko for suggesting this)");

PrintNewsBlurb("The Mac versions of our plugins have improved icons for OS X.");

EndNewsItem();
?>



<?php
BeginNewsItem("released VST MIDI 1.1.1", "2001-10-06", "announce");

PrintNewsBlurb("This version adds support for loading, saving, and browsing preset bank files.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Transverb 1.0 for classic Mac OS, OS X, &amp; Windows", "2001-09-30", "announce");
?>

<p class="announce">released Skidder &amp; Rez Synth 1.0.2 for Mac OS X</p>

<?php
PrintNewsBlurb("Version 1.0.2 of Skidder and Rez Synth makes the words \"Destroy FX\" and \"Smart Electronix\" links that will take you to those web pages when you click on them.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder, Rez Synth, &amp; Buffer Override 1.0.2 for classic Mac OS &amp; Windows &amp; Buffer Override 1.0.2 for OS X", "2001-09-29", "announce");

PrintNewsBlurb("Version 1.0.2 makes the words \"Destroy FX\" and \"Smart Electronix\" links that will take you to those web pages when you click on them.");

PrintNewsBlurb("Version 1.0.2 of Skidder also makes the random panning more random.");

EndNewsItem();
?>



<?php
BeginNewsItem("released Buffer Override 1.0 for Mac OS X", "2001-09-26", "announce");
?>

<p class="announce">released Skidder, Rez Synth, &amp; Buffer Override 1.0.1 for classic Mac OS</p>

<?php
PrintNewsBlurb("Version 1.0.1 fixes a problem where under Mac OS where the plugins would not load if you didn't have the Internet Config Extension installed.  (thank you Craig for telling me about this problem)");

EndNewsItem();
?>



<?php
BeginNewsItem("released VST MIDI 1.1", "2001-09-20", "announce");

PrintNewsBlurb("This version can load and save plugin presets.");

EndNewsItem();
?>



<?php
BeginNewsItem("Hello.", "2001-09-19", "desc");

PrintNewsBlurb("(we publicly announced our web page and plugins)");

EndNewsItem();
?>



<?php
BeginNewsItem("released Skidder, Rez Synth, &amp; Buffer Override 1.0 for Mac &amp; Windows", "2001-09-18", "announce"); ?>

<?php EndNewsItem();
?>



<?php
BeginNewsItem("released VST MIDI 1.0", "2001-09-12", "announce"); ?>

<?php EndNewsItem();
?>



<!-- news (template) -->

<!--
<p><table class="infobox">
<tr><td>
<table class="fullwidth">
<tr><td valign="top">

<p class="rel">December 3rd 1978</p>

<p class="announce">");

PrintNewsBlurb("</p>

</td></tr>
</table>
</td></tr></table>
<p>
-->



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
