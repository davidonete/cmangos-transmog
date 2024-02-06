#pragma once

#include "Config/Config.h"

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

private:
    Config config;
};

#define sTransmogConfig MaNGOS::Singleton<TransmogConfig>::Instance()

