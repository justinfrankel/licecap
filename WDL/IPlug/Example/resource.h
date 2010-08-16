// Double quotes, spaces OK.
#define PLUG_MFR "Schwa"
#define PLUG_NAME "IPlug Example"

// No quotes or spaces.
#define PLUG_CLASS_NAME PlugExample

// OSX crap.
// - Manually edit the info.plist file to set the CFBundleIdentifier to the either the string 
// "com.BUNDLE_MFR.audiounit.BUNDLE_NAME" or "com.BUNDLE_MFR.vst.BUNDLE_NAME".
// Double quotes, no spaces.
#define BUNDLE_MFR "Schwa"
#define BUNDLE_NAME "IPlugExample"
// - Manually create a PLUG_CLASS_NAME.exp file with two entries: _PLUG_ENTRY and _PLUG_VIEW_ENTRY
// (these two defines, each with a leading underscore).
// No quotes or spaces.
#define PLUG_ENTRY PlugExample_Entry
#define PLUG_VIEW_ENTRY PlugExample_ViewEntry
// The same strings, with double quotes.  There's no other way, trust me.
#define PLUG_ENTRY_STR "PlugExample_Entry"
#define PLUG_VIEW_ENTRY_STR "PlugExample_ViewEntry"
// This is the exported cocoa view class, some hosts display this string.
// No quotes or spaces.
#define VIEW_CLASS PlugExample_View
#define VIEW_CLASS_STR "PlugExample_View"

// This is interpreted as 0xMAJR.MN.BG
#define PLUG_VER 0x00010000

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes.
#define PLUG_UNIQUE_ID 'Iplg'
#define PLUG_MFR_ID 'Schw'

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0
#define PLUG_DOES_MIDI 0
#define PLUG_DOES_STATE_CHUNKS 0

// Unique IDs for each image resource.
#define TOGGLE_ID	100
#define KNOB_ID	  101
#define FADER_ID	102
#define BG_ID		  103
#define METER_ID	104

// Image resource locations for this plug.
#define TOGGLE_FN	"img/toggle-switch.png"
#define KNOB_FN	  "img/knob_sm.png"
#define FADER_FN	"img/fader-cap_sm.png"
#define BG_FN		  "img/BG_400x200.png"
#define METER_FN	"img/VU-meter_sm.png"
