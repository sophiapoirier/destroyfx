This directory should contain a symlink to the directory vst2.x.
This is needed because vstgui wants to include paths like
public.sdk/source/vst2.x/
but the legacy VST2 headers what to include paths like
pluginterfaces/vst2.x/

In this directory, just type:
`ln -s ../../pluginterfaces/vst2.x`

(The symlink is not added to the repository because allegedly
Subversion has trouble dealing with this on windows.)

On windows, the following may be preferable (from cmd running as
administrator):

`mklink /D vst2.x ..\..\pluginterfaces\vst2.x`

The `ln` command from cygwin seems to create a "junction" that works fine
in cygwin but has problems with windows tools (supposedly junctions don't
support relative paths?).
