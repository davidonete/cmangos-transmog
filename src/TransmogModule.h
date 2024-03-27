#ifndef CMANGOS_MODULE_TRANSMOG_H
#define CMANGOS_MODULE_TRANSMOG_H

#include "Module.h"
#include "TransmogModuleConfig.h"

#include <unordered_map>
#include <map>

namespace cmangos_module
{
    typedef std::unordered_map<ObjectGuid, uint32> Transmog2Data;
    typedef std::unordered_map<ObjectGuid, Transmog2Data> TransmogMap;
    typedef std::unordered_map<ObjectGuid, ObjectGuid> TransmogData;

    class TransmogModule : public Module
    {
    public:
        TransmogModule();
        const TransmogModuleConfig* GetConfig() const override;

        // Module Hooks
        void OnInitialize() override;

        // Player hooks
        void OnLoadFromDB(Player* player) override;
        void OnLogOut(Player* player) override;
        void OnDeleteFromDB(uint32 playerId) override;
        bool OnPreGossipHello(Player* player, Creature* creature) override;
        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId) override;
        void OnSetVisibleItemSlot(Player* player, uint8 slot, Item* item) override;
        void OnMoveItemFromInventory(Player* player, Item* item) override;

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
}
#endif