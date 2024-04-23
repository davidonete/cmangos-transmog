#include "TransmogModuleConfig.h"
#include "Globals/ObjectMgr.h"
#include "Log/Log.h"

namespace cmangos_module
{
    TransmogModuleConfig::TransmogModuleConfig()
    : ModuleConfig("transmog.conf")
    , enabled(false)
    , costMultiplier(1.0f)
    , costFee(0U)
    , tokenRequired(false)
    , tokenEntry(0U)
    , tokenAmount(0U)
    {
    
    }

    bool TransmogModuleConfig::OnLoad()
    {
        enabled = config.GetBoolDefault("Transmog.Enable", false);
        costMultiplier = config.GetFloatDefault("Transmog.CostMultiplier", 1.0f);
        costFee = config.GetIntDefault("Transmog.CostFee", 0U);
        tokenRequired = config.GetBoolDefault("Transmog.TokenRequired", false);
        tokenEntry = config.GetIntDefault("Transmog.TokenEntry", 0U);
        tokenAmount = config.GetIntDefault("Transmog.TokenAmount", 1U);

        if (tokenRequired)
        {
            auto result = WorldDatabase.PQuery("SELECT COUNT(*) FROM `item_template` WHERE `entry` = %u", tokenEntry);
            if (!result)
            {
                sLog.outError("Transmog.TokenEntry (%u) does not exist. Disabling token requirements", tokenRequired);
                tokenRequired = false;
            }
        }

        if (tokenRequired && tokenAmount == 0)
        {
            sLog.outError("Transmog.TokenAmount set to %u but it needs a minimum of 1. Setting token amount to 1", tokenAmount);
            tokenAmount = 1;
        }

        return true;
    }
}