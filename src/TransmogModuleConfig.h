#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
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

    enum TransmogLanguage
    {
        LANG_ERR_TRANSMOG_OK = 11100,
        LANG_ERR_TRANSMOG_INVALID_SLOT,
        LANG_ERR_TRANSMOG_INVALID_SRC_ENTRY,
        LANG_ERR_TRANSMOG_MISSING_SRC_ITEM,
        LANG_ERR_TRANSMOG_MISSING_DEST_ITEM,
        LANG_ERR_TRANSMOG_INVALID_ITEMS,
        LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY,
        LANG_ERR_TRANSMOG_NOT_ENOUGH_TOKENS,
        LANG_ERR_TRANSMOG_SAME_DISPLAYID,

        LANG_ERR_UNTRANSMOG_OK,
        LANG_ERR_UNTRANSMOG_SINGLE_OK,
        LANG_ERR_UNTRANSMOG_NO_TRANSMOGS,
        LANG_ERR_UNTRANSMOG_SINGLE_NO_TRANSMOGS,

        LANG_PRESET_ERR_INVALID_NAME,
        LANG_CMD_TRANSMOG_SHOW,
        LANG_CMD_TRANSMOG_HIDE,
        LANG_CMD_TRANSMOG_ADD_UNSUITABLE,
        LANG_CMD_TRANSMOG_ADD_FORBIDDEN,
        LANG_MENU_NO_SUITABLE_ITEMS,
        LANG_MENU_REMOVE,
        LANG_MENU_GO_BACK,
        LANG_MENU_MAIN_MENU,
        LANG_MENU_BEFORE,
        LANG_MENU_AFTER,
        LANG_MENU_COST_IS,
        LANG_MENU_CONFIRM,
        LANG_MENU_YOUR_ITEM,
        LANG_MENU_TRANSMOG,
        LANG_MENU_POSSIBLE_LOOK,
        LANG_MENU_OPTIONS,
        LANG_MENU_HOW,
        LANG_MENU_HEAD,
        LANG_MENU_SHOULDERS,
        LANG_MENU_SHIRT,
        LANG_MENU_CHEST,
        LANG_MENU_WAIST,
        LANG_MENU_LEGS,
        LANG_MENU_FEET,
        LANG_MENU_WRISTS,
        LANG_MENU_HANDS,
        LANG_MENU_BACK,
        LANG_MENU_MAIN_HAND,
        LANG_MENU_OFF_HAND,
        LANG_MENU_RANGED,
        LANG_MENU_TABARD,
        LANG_MENU_REMOVE_ALL,
        LANG_MENU_TRANSMOGRIFY
    };

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