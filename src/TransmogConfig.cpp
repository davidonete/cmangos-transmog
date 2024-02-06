#include "TransmogConfig.h"

#include "Log.h"
#include "SystemConfig.h"

TransmogConfig::TransmogConfig()
: enabled(false)
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

    sLog.outString("Transmog configuration loaded");
    return true;
}
