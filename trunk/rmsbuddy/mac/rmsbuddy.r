#include "AudioUnit.r"
#include "rmsbuddydef.h"


// define both of these so that we produce the ComponentResources that indicate 
// "universal binaries" (executables with code for PowerPC and x86)
#define ppc_YES		1
#define i386_YES	1


#define RES_ID		3000
#define COMP_TYPE	kAudioUnitType_Effect
#define COMP_SUBTYPE	RMS_BUDDY_PLUGIN_ID
#define COMP_MANUF	RMS_BUDDY_MANUFACTURER_ID
#define VERSION		RMS_BUDDY_VERSION
#define NAME		"Destroy FX: RMS Buddy"
#define DESCRIPTION	"modular multi-fx thingie"
#define ENTRY_POINT	"RMSBuddyEntry"

#include "AUResources.r"


#define RES_ID		9000
#define COMP_TYPE	kAudioUnitCarbonViewComponentType
#define COMP_SUBTYPE	RMS_BUDDY_PLUGIN_ID
#define COMP_MANUF	RMS_BUDDY_MANUFACTURER_ID
#define VERSION		RMS_BUDDY_VERSION
#define NAME		"Destroy FX: RMS Buddy editor"
#define DESCRIPTION	"compact and informative interface for DFX RMS Buddy"
#define ENTRY_POINT	"RMSBuddyEditorEntry"

#include "AUResources.r"
