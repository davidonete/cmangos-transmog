#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
    class TransmogModuleConfig : public ModuleConfig
    {
    public:
        TransmogModuleConfig();
        bool OnLoad() override;

    public:
        bool enabled;
        float costMultiplier;
        uint32 costFee;
        bool tokenRequired;
        uint32 tokenEntry;
        uint32 tokenAmount;
    };
}