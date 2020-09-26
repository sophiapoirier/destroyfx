#include "AudioUnit.r"

#include "freeverb-au-def.h"

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#define FREEVERB_RES_ID					10000

// So you need to define these appropriately for your audio unit
// For the name the convention is to provide your company name and end it 
// with a ':' then provide the name of the AudioUnit
// The Description can be whatever you want
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Freeverb3
#define RES_ID		FREEVERB_RES_ID
#define COMP_TYPE	kAudioUnitType_Effect
#define COMP_SUBTYPE	'JzR3'
#define COMP_MANUF	'DreP'
#define VERSION		PLUGIN_VERSION
#define NAME		"Dreampoint: Freeverb3"
#define DESCRIPTION	"reverb"
#define ENTRY_POINT	"FreeverbEntry"

#include "AUResources.r"
