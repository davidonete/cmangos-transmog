#ifndef MANGOS_TRANSMOG_MGR_H
#define MANGOS_TRANSMOG_MGR_H

#include "World/World.h"

#include <unordered_map>

#define MAX_OPTIONS 25
#define TRANSMOG_NPC_ENTRY 190010

class Item;
class Player;

struct ItemPrototype;

//class WorldSession;

enum TransmogAcoreStrings
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
    LANG_MENU_BACK,
    LANG_MENU_MAIN_MENU,
    LANG_MENU_BEFORE,
    LANG_MENU_AFTER,
    LANG_MENU_COST_IS,
    LANG_MENU_CONFIRM,
    LANG_MENU_YOUR_ITEM,
    LANG_MENU_TRANSMOG,
    LANG_MENU_POSSIBLE_LOOK,
    LANG_MENU_OPTIONS,
};

typedef std::unordered_map<ObjectGuid, uint32> Transmog2Data;
typedef std::unordered_map<ObjectGuid, Transmog2Data> TransmogMap;
typedef std::unordered_map<ObjectGuid, ObjectGuid> TransmogData;

typedef std::map<uint8, uint32> SlotMap;
typedef std::map<uint8, SlotMap> PresetData;
typedef std::map<uint8, std::string> PresetIdMap;
typedef std::unordered_map<ObjectGuid, PresetData> PresetDataMap;
typedef std::unordered_map<ObjectGuid, PresetIdMap> PresetNameMap;

class TransmogMgr
{
public:
    TransmogMgr() {}

    void Init();

    // Player hooks
    void OnPlayerLogout(Player* player);
    bool OnPlayerGossipHello(Player* player, Creature* creature);
    bool OnPlayerGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);

private:
    // Presets
    void LoadPlayerPresets(Player* player);
    void UnloadPlayerPresets(Player* player);

    bool CanTransmogrifyItemWithItem(Player* player, const ItemPrototype* target, const ItemPrototype* source) const;
    bool SuitableForTransmogrification(Player* player, const ItemPrototype* proto) const;

    void ShowTransmogItems(Player* player, Creature* creature, uint8 slot);
    void UpdateTransmogItem(Player* player, Item* item) const;

    uint32 GetFakeEntry(const ObjectGuid& itemGUID) const;
    void SetFakeEntry(Player* player, uint32 newEntry, Item* itemTransmogrified);
    void DeleteFakeEntry(Player* player, uint8, Item* itemTransmogrified);
    void DeleteFakeEntryFromDB(const ObjectGuid& itemGUID);

private:
    TransmogMap entryMap;
    TransmogData dataMap;

    // Presets
    PresetDataMap presetById;
    PresetNameMap presetByName;

/*
#ifdef PRESETS
    bool EnableSetInfo;
    uint32 SetNpcText;


    
    

    
    

    void PresetTransmog(Player* player, Item* itemTransmogrified, uint32 fakeEntry, uint8 slot);

    bool EnableSets;
    uint8 MaxSets;
    float SetCostModifier;
    int32 SetCopperCost;

    bool GetEnableSets() const;
    uint8 GetMaxSets() const;
    float GetSetCostModifier() const;
    int32 GetSetCopperCost() const;

    void LoadPlayerSets(ObjectGuid pGUID);
    void UnloadPlayerSets(ObjectGuid pGUID);
#endif

    bool EnableTransmogInfo;
    uint32 TransmogNpcText;
    uint32 TransmogNpcSelectLookText;
    uint32 TransmogNpcConfirmText;
    uint32 TransmogNpcAlreadyText;
    uint32 TransmogNpcAlreadyAltText;

    /*
    std::set<uint32> Allowed;
    std::set<uint32> NotAllowed;

    float ScaledCostModifier;
    int32 CopperCost;

    bool RequireToken;
    uint32 TokenEntry;
    uint32 TokenAmount;

    bool AllowPoor;
    bool AllowCommon;
    bool AllowUncommon;
    bool AllowRare;
    bool AllowEpic;
    bool AllowLegendary;
    bool AllowArtifact;
    bool AllowHeirloom;

    bool AllowMixedArmorTypes;
    bool AllowMixedWeaponTypes;
    bool AllowFishingPoles;

    bool IgnoreReqRace;
    bool IgnoreReqClass;
    bool IgnoreReqSkill;
    bool IgnoreReqSpell;
    bool IgnoreReqLevel;
    bool IgnoreReqEvent;
    bool IgnoreReqStats;
    */

    /*
    bool IsAllowed(uint32 entry) const;
    bool IsNotAllowed(uint32 entry) const;
    bool IsAllowedQuality(uint32 quality) const;
    bool IsRangedWeapon(uint32 Class, uint32 SubClass) const;

    void LoadConfig(bool reload);

    std::string GetItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y) const;
    std::string GetSlotIcon(uint8 slot, uint32 width, uint32 height, int x, int y) const;
    const char* GetSlotName(uint8 slot, WorldSession* session) const;
    std::string GetItemLink(Item* item, WorldSession* session) const;
    std::string GetItemLink(uint32 entry, WorldSession* session) const;
    uint32 GetFakeEntry(ObjectGuid itemGUID) const;
    void UpdateItem(Player* player, Item* item) const;
    void DeleteFakeEntry(Player* player, uint8 slot, Item* itemTransmogrified);
    void SetFakeEntry(Player* player, uint32 newEntry, Item* itemTransmogrified);

    TransmogAcoreStrings Transmogrify(Player* player, ObjectGuid itemTransmogrifier, uint8 slot, bool no_cost = false);
    bool CanTransmogrifyItemWithItem(Player* player, ItemPrototype const* target, ItemPrototype const* source) const;
    static bool SuitableForTransmogrification(Player* player, ItemPrototype const* proto);
    // bool CanBeTransmogrified(Item const* item);
    // bool CanTransmogrify(Item const* item);
    uint32 GetSpecialPrice(ItemPrototype const* proto) const;
    std::string FormatPrice(uint32 copper) const;

    void DeleteFakeFromDB(ObjectGuid itemLowGuid);
    static void CleanUp(Player* pPlayer);
    void BuildTransmogMap(Player* pPlayer);
    bool Refresh(Player* pPlayer, Item* pEquippedItem);
    static bool RevertAll(Player* pPlayer);
    bool ApplyAll(Player* pPlayer);
    void OnLogout(Player* player);
    float GetScaledCostModifier() const;
    int32 GetCopperCost() const;

    bool GetRequireToken() const;
    uint32 GetTokenEntry() const;
    uint32 GetTokenAmount() const;

    bool GetAllowMixedArmorTypes() const;
    bool GetAllowMixedWeaponTypes() const;

    // Config
    bool GetEnableTransmogInfo() const;
    uint32 GetTransmogNpcText() const;
    bool GetEnableSetInfo() const;
    uint32 GetSetNpcText() const;
    uint32 GetNpcSelectLookText() const;
    uint32 GetSetNpcConfirmText() const;
    uint32 GetSetNpcAlreadyText() const;
    uint32 GetSetNpcAlreadyAltText() const;
*/
};

#define sTransmogMgr MaNGOS::Singleton<TransmogMgr>::Instance()
#endif