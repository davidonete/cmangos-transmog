#ifndef MANGOS_TRANSMOG_MGR_H
#define MANGOS_TRANSMOG_MGR_H

#include "Entities/ObjectGuid.h"
#include "Platform/Define.h"

#include <unordered_map>
#include <map>

class Creature;
class Item;
class Player;

struct ItemPrototype;

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
    void OnPlayerLogin(Player* player);
    void OnPlayerLogout(Player* player);
    void OnPlayerCharacterDeletedFromDB(uint32 playerId);
    bool OnPlayerGossipHello(Player* player, Creature* creature);
    bool OnPlayerGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    void OnPlayerSetVisibleItemSlot(Player* player, uint8 slot, Item* item);
    void OnPlayerMoveItemFromInventory(Player* player, Item* item);

private:
    bool CanApplyTransmog(Player* player, const Item* target, const Item* source) const;
    bool SuitableForTransmog(Player* player, const Item* item) const;

    void ShowTransmogItems(Player* player, Creature* creature, uint8 slot);
    void UpdateTransmogItem(Player* player, Item* item) const;
    TransmogLanguage ApplyTransmog(Player* player, Item* sourceItem, Item* targetItem);
    uint32 GetTransmogPrice(const Item* item) const;

    uint32 GetFakeEntry(Item* item) const;
    void SetFakeEntry(Player* player, uint32 newEntry, Item* item);
    void DeleteFakeEntry(Player* player, uint8, Item* item);
    void DeleteFakeEntryFromDB(Item* item);

private:
    TransmogMap entryMap;
    TransmogData dataMap;
};

#define sTransmogMgr MaNGOS::Singleton<TransmogMgr>::Instance()
#endif