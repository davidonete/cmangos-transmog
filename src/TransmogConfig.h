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

private:
    Config config;
};

#define sTransmogConfig MaNGOS::Singleton<TransmogConfig>::Instance()

