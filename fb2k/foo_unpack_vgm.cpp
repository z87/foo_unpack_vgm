#include <foobar2000/SDK/foobar2000.h>

#include "foo_unpack_vgm.h"

#define PLUGIN_NAME  "foo_unpack_vgm"
#define PLUGIN_VERSION  "0.1"
#define PLUGIN_INFO  PLUGIN_NAME " " PLUGIN_VERSION " (" __DATE__ ")"
#define PLUGIN_DESCRIPTION  PLUGIN_INFO "\n" \
	"Video game archive reader\n" \
    "Supported formats:\n" \
    "- Cyberpunk 2077: .archive (limited to .wem contents)\n" \
    "- GTA IV: .rpf\n" \
    "- GTA V: .rpf\n" \
    "- TDU 2: .big + .map, .bnk"

#define PLUGIN_FILENAME "foo_unpack_vgm.dll"

cfg_string cfg_rpf3key({ 0xbd3dcf2a,0x5173,0x4477,{0x8f,0x6,0xcc,0x5,0xd1,0x63,0x1b,0xa1} }, "");
cfg_string cfg_rpf7key({ 0xbd3dcf2a,0x5173,0x4477,{0x8f,0x6,0xcc,0x5,0xd1,0x63,0x1b,0xa2} }, "");

bool read_keystring(const char* str, uint8_t* key) {
    if (strlen(str) != 64) return false;
    try {
        for (uint8_t i = 0; i < 32; i++) {
            key[i] = pfc::atohex<uint8_t>(str + i * 2, 2);
        }
        return true;
    }
    catch (const pfc::exception_overflow) {
        return false;
    }
}

DECLARE_COMPONENT_VERSION(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_DESCRIPTION);
VALIDATE_COMPONENT_FILENAME(PLUGIN_FILENAME);
