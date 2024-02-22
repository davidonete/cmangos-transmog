#pragma once

#include "Config/Config.h"

#define MAX_OPTIONS 25

#define TRANSMOG_NPC_ENTRY 190010

#define TRANSMOG_INFO_NPC_TEXT 601083
#define TRANSMOG_PRESET_INFO_NPC_TEXT 601084
#define TRANSMOG_SELECT_LOOK_NPC_TEXT 601085
#define TRANSMOG_CONFIRM_NPC_TEXT 601086
#define TRANSMOG_ALREADY_NPC_TEXT 601087
#define TRANSMOG_ALREADY_ALT_NPC_TEXT 601088

#define TRANSMOG_SELF_VISUAL_SPELL 24085
#define TRANSMOG_NPC_VISUAL_SPELL 14867

class TransmogConfig
{
public:
    TransmogConfig();

    bool Initialize();

public:
    bool enabled;
    float costMultiplier;
    uint32 costFee;
    bool tokenRequired;
    uint32 tokenEntry;
    uint32 tokenAmount;

private:
    Config config;
};

#define sTransmogConfig MaNGOS::Singleton<TransmogConfig>::Instance()

