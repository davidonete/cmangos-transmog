#include "TransmogConfig.h"

#include "Globals/ObjectMgr.h"
#include "Log.h"
#include "SystemConfig.h"

TransmogConfig::TransmogConfig()
: enabled(false)
, presetsEnabled(false)
, maxPresets(0U)
, costMultiplier(1.0f)
, costFee(0U)
, tokenRequired(false)
, tokenEntry(0U)
, tokenAmount(0U)
{
    
}

INSTANTIATE_SINGLETON_1(TransmogConfig);

bool TransmogConfig::Initialize()
{
    sLog.outString("Initializing Transmog");

    if (!config.SetSource(SYSCONFDIR"transmog.conf"))
    {
        sLog.outError("Failed to open configuration file transmog.conf");
        return false;
    }

    enabled = config.GetBoolDefault("Transmog.Enable", false);
    presetsEnabled = config.GetBoolDefault("Transmog.Presets", false);
    maxPresets = config.GetIntDefault("Transmog.MaxPresets", 10U);
    costMultiplier = config.GetFloatDefault("Transmog.CostMultiplier", 1.0f);
    costFee = config.GetIntDefault("Transmog.CostFee", 0U);
    tokenRequired = config.GetBoolDefault("Transmog.TokenRequired", false);
    tokenEntry = config.GetIntDefault("Transmog.TokenEntry", 0U);
    tokenAmount = config.GetIntDefault("Transmog.TokenAmount", 1U);

    if (maxPresets > MAX_OPTIONS)
    {
        sLog.outError("Transmog.MaxPresets set to %u but the max allowed is 25.", maxPresets);
        maxPresets = MAX_OPTIONS;
    }

    if (tokenRequired && !sObjectMgr.GetItemPrototype(tokenEntry))
    {
        sLog.outError("Transmog.TokenEntry (%u) does not exist. Disabling token requirements", tokenRequired);
        tokenRequired = false;
    }

    if (tokenRequired && tokenAmount == 0)
    {
        sLog.outError("Transmog.TokenAmount set to %u but it needs a minimum of 1. Setting token amount to 1", tokenAmount);
        tokenAmount = 1;
    }

    sLog.outString("Transmog configuration loaded");
    return true;
}
