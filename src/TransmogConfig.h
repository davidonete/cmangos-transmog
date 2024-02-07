#pragma once

#include "Config/Config.h"

#define MAX_OPTIONS 25
#define TRANSMOG_NPC_ENTRY 190010
#define TRANSMOG_INFO_NPC_TEXT 601083
#define TRANSMOG_PRESET_INFO_NPC_TEXT 601084

class TransmogConfig
{
public:
    TransmogConfig();

    static TransmogConfig& instance()
    {
        static TransmogConfig instance;
        return instance;
    }

    bool Initialize();

public:
    bool enabled;
    bool presetsEnabled;
    uint32 maxPresets;
    float costMultiplier;
    uint32 costFee;
    bool tokenRequired;
    uint32 tokenEntry;
    uint32 tokenAmount;

private:
    Config config;
};

#define sTransmogConfig MaNGOS::Singleton<TransmogConfig>::Instance()

