#ifndef CMANGOS_MODULE_TRANSMOG_H
#define CMANGOS_MODULE_TRANSMOG_H

#include "Module.h"
#include "TransmogModuleConfig.h"

#include <unordered_map>
#include <map>

namespace cmangos_module
{
    typedef std::unordered_map<ObjectGuid, uint32> Transmog2Data;
    typedef std::unordered_map<uint32, Transmog2Data> TransmogMap;
    typedef std::unordered_map<ObjectGuid, uint32> TransmogData;

    struct TransmogItem
    {
        uint8 slots[4];
        uint8 itemClass;
        uint8 itemSubclass;
        uint32 itemID;
        uint32 displayID;
    };

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
        void OnSetVisibleItemSlot(Player* player, uint8 slot, Item* item) override;
        void OnMoveItemFromInventory(Player* player, Item* item) override;
        void OnEquipItem(Player* player, Item* item) override;

        // Commands
        std::vector<ModuleChatCommand>* GetCommandTable() override;
        const char* GetChatCommandPrefix() const override { return "transmog"; }
        bool HandleTransmogStatus(WorldSession* session, const std::string& args);
        bool HandleGetAvailableTransmogs(WorldSession* session, const std::string& args);
        bool HandleCalculateTransmogCost(WorldSession* session, const std::string& args);
        bool HandleApplyTransmog(WorldSession* session, const std::string& args);

    private:
        void UpdateItemAppearance(Player* player, Item* item) const;
        uint32 GetTransmogAppearance(const Item* item) const;
        
        bool ApplyTransmog(Player* player, Item* item, uint32 transmogItemID, bool updateVisibility);
        bool RemoveTransmog(Player* player, Item* item, bool updateVisibility);

        bool IsItemTransmogrified(const Item* item) const;
        std::vector<std::pair<Item*, uint32>> GetTransmogrifiedItems(const Player* player, bool equipped = false) const;

        bool IsValidTransmog(const Player* player, const ItemPrototype* itemPrototype) const;
        bool IsValidTransmog(const Player* player, uint32 itemEntry) const;

        void LoadActiveTransmogs(Player* player);
        void SendActiveTransmogs(const Player* player);

        void LoadDiscoveredTransmogs(const Player* player);
        void AddDiscoveredTransmog(const Player* player, uint32 itemEntry, bool sendToClient, bool addToDB);
        void SendDiscoveredTransmogs(const Player* player, int8 slot = -1, int8 itemClass = -1, int8 itemSubclass = -1);

        std::pair<uint32, uint32> CalculateTransmogCost(uint32 itemEntry) const;
        void SendTransmogCost(const Player* player, const std::vector<std::pair<uint32, uint32>>& slots) const;

    private:
        TransmogMap entryMap;
        TransmogData dataMap;

        std::unordered_map<uint32, std::map<uint32, TransmogItem>> playerDiscoveredTransmogs;
    };
}
#endif