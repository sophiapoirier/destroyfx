# DFX GUI

This is our graphics library for our Destroy FX plugins.  The library
assumes cooperation with DfxPlugin plugins and not any specific plugin
format, so that we can write our GUI code in a way that will work
specifically and nicely with DfxPlugin and will work with all of the
plugin formats that DfxPlugin supports (currently Audio Unit for Mac
and VST for Windows).

The implementation is VSTGUI-based.  This provides a foundation that
abstracts away from platform-specific GUI APIs and handles a lot of
the general GUI toolkit duties, enabling our layer on top of that to
manage features and functionality particular to our plugins.
