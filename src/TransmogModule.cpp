#include "TransmogModule.h"

#include "Entities/GossipDef.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/PlayerbotAI.h"
#endif

namespace cmangos_module
{
    void SendAddOnMessage(const Player* player, const char* prefix, const char* message)
    {
        WorldPacket data;

        std::ostringstream out;
        if (strstr(prefix, "") != NULL)
        {
            out << prefix << "\t" << message;
        }
        else
        {
            out << message;
        }

        char* buf = mangos_strdup(out.str().c_str());
        char* pos = buf;

        while (char* line = ChatHandler::LineFromMessage(pos))
        {
#if EXPANSION == 0
            ChatHandler::BuildChatPacket(data, CHAT_MSG_ADDON, line, LANG_ADDON);
#else
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, line, LANG_ADDON);
#endif
            player->GetSession()->SendPacket(data);
        }

        delete[] buf;
    }

    void SendAddOnMessage(const Player* player, const char* prefix, const std::string& message)
    {
        SendAddOnMessage(player, prefix, message.c_str());
    }

    TransmogModule::TransmogModule()
    : Module("Transmog", new TransmogModuleConfig())
    {

    }

    const cmangos_module::TransmogModuleConfig* TransmogModule::GetConfig() const
    {
        return (TransmogModuleConfig*)Module::GetConfig();
    }

    void TransmogModule::OnInitialize()
    {
	    if (GetConfig()->enabled)
	    {
            // Cleanup non existent characters
            CharacterDatabase.PExecute("DELETE FROM `custom_transmog_active` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_transmog_active`.`player`);");
            CharacterDatabase.PExecute("DELETE FROM `custom_transmog_discovered` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_transmog_discovered`.`player`);");
		    
            // Delete corrupted transmog items
		    CharacterDatabase.Execute("DELETE FROM `custom_transmog_active` WHERE NOT EXISTS (SELECT 1 FROM `item_instance` WHERE `item_instance`.`guid` = `custom_transmog_active`.`item_guid`)");
	    }
    }

    void TransmogModule::OnLoadFromDB(Player* player)
    {
        if (GetConfig()->enabled)
        {
		    if (player)
		    {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

                LoadActiveTransmogs(player);
                LoadDiscoveredTransmogs(player);
		    }
	    }
    }

    void TransmogModule::OnLogOut(Player* player)
    {
	    if (GetConfig()->enabled)
	    {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

			    // Unload transmog config
                const uint32 playerID = player->GetObjectGuid().GetCounter();
                for (auto it = entryMap[playerID].begin(); it != entryMap[playerID].end(); ++it)
                {
                    dataMap.erase(it->first);
                }

                entryMap.erase(playerID);
                playerDiscoveredTransmogs.erase(playerID);
            }
	    }
    }

    void TransmogModule::OnDeleteFromDB(uint32 playerId)
    {
        if (GetConfig()->enabled)
        {
		    CharacterDatabase.PExecute("DELETE FROM `custom_transmog_active` WHERE `player` = %u", playerId);
            CharacterDatabase.PExecute("DELETE FROM `custom_transmog_discovered` WHERE `player` = %u", playerId);

            // Unload transmog config
            for (auto it = entryMap[playerId].begin(); it != entryMap[playerId].end(); ++it)
            {
                dataMap.erase(it->first);
            }

            entryMap.erase(playerId);
            playerDiscoveredTransmogs.erase(playerId);
	    }
    }

    void TransmogModule::OnSetVisibleItemSlot(Player* player, uint8 slot, Item* item)
    {
	    if (GetConfig()->enabled)
	    {
		    if (player && item)
		    {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

                if (uint32 entry = GetTransmogAppearance(item))
			    {
#if EXPANSION == 2
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + item->GetSlot() * 2, entry);
#else
                    player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_0 + item->GetSlot() * MAX_VISIBLE_ITEM_OFFSET, entry);
#endif
			    }
		    }
	    }
    }

    void TransmogModule::OnEquipItem(Player* player, Item* item)
    {
        if (GetConfig()->enabled)
        {
            if (player && item)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif
                // Don't consider items if the player has not finished loading from DB
                if (playerDiscoveredTransmogs.find(player->GetObjectGuid().GetCounter()) != playerDiscoveredTransmogs.end())
                {
                    const uint32 itemEntry = item->GetEntry();
                    if (IsValidTransmog(player, itemEntry))
                    {
                        AddDiscoveredTransmog(player, itemEntry, true, true);
                    }
                }
            }
        }
    }

    void TransmogModule::OnMoveItemFromInventory(Player* player, Item* item)
    {
        if (GetConfig()->enabled)
        {
            if (player && item)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

			    RemoveTransmog(player, item, false);
		    }
	    }
    }

    std::vector<ModuleChatCommand>* TransmogModule::GetCommandTable()
    {
        static std::vector<ModuleChatCommand> commandTable =
        {
            { "GetTransmogStatus", std::bind(&TransmogModule::HandleTransmogStatus, this, std::placeholders::_1, std::placeholders::_2), SEC_PLAYER },
            { "GetAvailableTransmogs", std::bind(&TransmogModule::HandleGetAvailableTransmogs, this, std::placeholders::_1, std::placeholders::_2), SEC_PLAYER },
            { "CalculateTransmogCost", std::bind(&TransmogModule::HandleCalculateTransmogCost, this, std::placeholders::_1, std::placeholders::_2), SEC_PLAYER },
            { "ApplyTransmog", std::bind(&TransmogModule::HandleApplyTransmog, this, std::placeholders::_1, std::placeholders::_2), SEC_PLAYER }
        };

        return &commandTable;
    }

    bool TransmogModule::HandleTransmogStatus(WorldSession* session, const std::string& args)
    {
        if (GetConfig()->enabled)
        {
            Player* player = session->GetPlayer();
            if (player)
            {
                SendActiveTransmogs(player);
                return true;
            }
        }

        return false;
    }

    bool TransmogModule::HandleGetAvailableTransmogs(WorldSession* session, const std::string& args)
    {
        if (GetConfig()->enabled)
        {
            Player* player = session->GetPlayer();
            if (player)
            {
                SendDiscoveredTransmogs(player);
                return true;
            }
        }

        return false;
    }

    bool TransmogModule::HandleCalculateTransmogCost(WorldSession* session, const std::string& args)
    {
        if (GetConfig()->enabled)
        {
            Player* player = session->GetPlayer();
            if (player)
            {
                std::vector<std::pair<uint32, uint32>> slots;
                std::vector<std::string> slotsStr = helper::SplitString(args, ",");
                for (const auto& slotStr : slotsStr)
                {
                    std::vector<std::string> slotPair = helper::SplitString(slotStr, ":");
                    if (slotPair.size() == 2)
                    {
                        if (helper::IsValidNumberString(slotPair[0]) && helper::IsValidNumberString(slotPair[1]))
                        {
                            const uint32 slot = std::stoi(slotPair[0]);
                            const uint32 itemID = std::stoi(slotPair[1]);
                            slots.push_back(std::make_pair(slot, itemID));
                        }
                    }
                }

                SendTransmogCost(player, slots);
                return true;
            }
        }

        return false;
    }

    bool TransmogModule::HandleApplyTransmog(WorldSession* session, const std::string& args)
    {
        if (GetConfig()->enabled)
        {
            Player* player = session->GetPlayer();
            if (player)
            {
                std::vector<std::pair<uint32, uint32>> slots;
                std::vector<std::string> slotsStr = helper::SplitString(args, ",");
                for (const auto& slotStr : slotsStr)
                {
                    std::vector<std::string> slotPair = helper::SplitString(slotStr, ":");
                    if (slotPair.size() == 2)
                    {
                        if (helper::IsValidNumberString(slotPair[0]) && helper::IsValidNumberString(slotPair[1]))
                        {
                            const uint32 slot = std::stoi(slotPair[0]);
                            const uint32 itemID = std::stoi(slotPair[1]);
                            slots.push_back(std::make_pair(slot, itemID));
                        }
                    }
                }

                uint32 cost = 0;
                uint32 tokenID = 0;
                bool succeeded = false;

                if (slots.size() > 0)
                {
                    for (auto& pair : slots)
                    {
                        const uint32 slot = pair.first;
                        const uint32 itemID = pair.second;
                        if (const Item* slotItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                        {
                            std::pair<uint32, uint32> itemCost = CalculateTransmogCost(slotItem->GetEntry());
                            cost += itemCost.first;
                            tokenID = itemCost.second;
                        }
                    }

                    succeeded = tokenID ? player->HasItemCount(tokenID, cost) : player->GetMoney() >= cost;
                }

                if (succeeded)
                {
                    for (auto& pair : slots)
                    {
                        const uint32 slot = pair.first;
                        const uint32 itemID = pair.second;
                        if (Item* slotItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                        {
                            if (itemID > 0)
                            {
                                if (!ApplyTransmog(player, slotItem, itemID, true))
                                {
                                    succeeded = false;
                                    break;
                                }
                            }
                            else
                            {
                                if (!RemoveTransmog(player, slotItem, true))
                                {
                                    succeeded = false;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (succeeded)
                {
                    if (tokenID)
                    {
                        player->DestroyItemCount(tokenID, cost, true);
                    }
                    else
                    {
                        player->ModifyMoney(-(int32)cost);
                    }

                    player->GetPlayerMenu();

                    std::ostringstream out;
                    bool first = true;
                    for (auto& pair : slots)
                    {
                        if (first)
                        {
                            out << pair.first << "," << pair.second;
                            first = false;
                        }
                        else
                        {
                            out << ":" << pair.first << "," << pair.second;
                        }
                    }

                    SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString("ApplyTransmogResult:1:%s", out.str().c_str()));
                }
                else
                {
                    SendAddOnMessage(player, GetChatCommandPrefix(), "ApplyTransmogResult:0");
                }
                
                return true;
            }
        }

        return false;
    }

    void TransmogModule::UpdateItemAppearance(Player* player, Item* item) const
    {
        if (item->IsEquipped())
	    {
            player->SetVisibleItemSlot(item->GetSlot(), item);
	    }
    }

    uint32 TransmogModule::GetTransmogAppearance(const Item* item) const
    {	
	    if (item)
	    {
		    const ObjectGuid itemGUID = item->GetObjectGuid();
            const auto itr = dataMap.find(itemGUID);
            if (itr == dataMap.end()) return 0;
            const auto itr2 = entryMap.find(itr->second);
            if (itr2 == entryMap.end()) return 0;
            const auto itr3 = itr2->second.find(itemGUID);
            if (itr3 == itr2->second.end()) return 0;
            return itr3->second;
	    }

	    return 0;
    }

    bool TransmogModule::ApplyTransmog(Player* player, Item* item, uint32 transmogItemID, bool updateAppearance)
    {
        if (player && item)
        {
            if (IsValidTransmog(player, transmogItemID))
            {
                const ObjectGuid itemGUID = item->GetObjectGuid();
                const uint32 playerID = player->GetObjectGuid().GetCounter();

                entryMap[playerID][itemGUID] = transmogItemID;
                dataMap[itemGUID] = playerID;

                CharacterDatabase.PExecute("REPLACE INTO `custom_transmog_active` (`item_guid`, `transmog_entry`, `player`) VALUES (%u, %u, %u)", itemGUID.GetCounter(), transmogItemID, playerID);

                if (updateAppearance)
                {
                    UpdateItemAppearance(player, item);
                }

                return true;
            }
        }

        return false;
    }

    bool TransmogModule::RemoveTransmog(Player* player, Item* item, bool updateAppearance)
    {
        if (player && item)
        {
            const ObjectGuid itemGUID = item->GetObjectGuid();
            if (dataMap.find(itemGUID) != dataMap.end())
            {
                if (entryMap.find(dataMap[itemGUID]) != entryMap.end())
                {
                    entryMap[dataMap[itemGUID]].erase(itemGUID);
                }

                dataMap.erase(itemGUID);
            }

            CharacterDatabase.PExecute("DELETE FROM `custom_transmog_active` WHERE `item_guid` = %u", itemGUID.GetCounter());

            if (updateAppearance)
            {
                UpdateItemAppearance(player, item);
            }

            return true;
        }

        return false;
    }

    bool TransmogModule::IsItemTransmogrified(const Item* item) const
    {
        return GetTransmogAppearance(item) != 0;
    }

    std::vector<std::pair<Item*, uint32>> TransmogModule::GetTransmogrifiedItems(const Player* player, bool equipped) const
    {
        std::vector<std::pair<Item*, uint32>> transmogrifiedItems;
        if (player)
        {
            if (equipped)
            {
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        uint32 transmogItemEntry = GetTransmogAppearance(item);
                        if (transmogItemEntry != 0)
                        {
                            transmogrifiedItems.push_back(std::make_pair(item, transmogItemEntry));
                        }
                    }
                }
            }
            else
            {
                auto playerTransmogsIt = entryMap.find(player->GetObjectGuid());
                if (playerTransmogsIt != entryMap.end())
                {
                    const auto& playerTransmogs = playerTransmogsIt->second;
                    for (const auto it : playerTransmogs)
                    {
                        if (Item* item = player->GetItemByGuid(it.first))
                        {
                            transmogrifiedItems.push_back(std::make_pair(item, it.second));
                        }
                    }
                }
            }
        }

        return transmogrifiedItems;
    }

    std::vector<uint8> GetWeaponAvailableForClass(uint8 playerClass)
    {
        std::map<uint8, std::vector<uint8>> weaponPerClass =
        {
            { CLASS_WARRIOR, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_GUN, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_CROSSBOW } },
            { CLASS_PALADIN, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_SWORD2 } },
            { CLASS_HUNTER, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_GUN, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_CROSSBOW } },
            { CLASS_ROGUE, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_GUN, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_CROSSBOW } },
            { CLASS_PRIEST, { ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_WAND } },
#if EXPANSION == 2
            { CLASS_DEATH_KNIGHT, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_SWORD2 } },
#endif
            { CLASS_SHAMAN, { ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER } },
            { CLASS_MAGE, { ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_WAND } },
            { CLASS_WARLOCK, { ITEM_SUBCLASS_WEAPON_SWORD, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_DAGGER, ITEM_SUBCLASS_WEAPON_WAND } },
            { CLASS_DRUID, { ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER } },
        };

        return weaponPerClass[playerClass];
    }

    std::vector<uint8> GetArmorAvailableForClass(uint8 playerClass)
    {
        std::map<uint8, std::vector<uint8>> armorPerClass =
        {
            { CLASS_WARRIOR, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL, ITEM_SUBCLASS_ARMOR_PLATE, ITEM_SUBCLASS_ARMOR_SHIELD } },
            { CLASS_PALADIN, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL, ITEM_SUBCLASS_ARMOR_PLATE, ITEM_SUBCLASS_ARMOR_SHIELD } },
            { CLASS_HUNTER, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL } },
            { CLASS_ROGUE, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER } },
            { CLASS_PRIEST, { ITEM_SUBCLASS_ARMOR_CLOTH } },
#if EXPANSION == 2
            { CLASS_DEATH_KNIGHT, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL, ITEM_SUBCLASS_ARMOR_PLATE } },
#endif
            { CLASS_SHAMAN, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL } },
            { CLASS_MAGE, { ITEM_SUBCLASS_ARMOR_CLOTH } },
            { CLASS_WARLOCK, { ITEM_SUBCLASS_ARMOR_CLOTH } },
            { CLASS_DRUID, { ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER } },
        };

        return armorPerClass[playerClass];
    }

    bool TransmogModule::IsValidTransmog(const Player* player, const ItemPrototype* itemPrototype) const
    {
        if (player && itemPrototype)
        {
            if (itemPrototype->Class == ITEM_CLASS_WEAPON || itemPrototype->Class == ITEM_CLASS_ARMOR)
            {
                // Class/race requirement check
                if ((itemPrototype->AllowableClass & player->getClassMask()) == 0 ||
                    (itemPrototype->AllowableRace & player->getRaceMask()) == 0)
                {
                    return false;
                }

                // Valid for class check
                const uint8 playerClass = player->getClass();
                std::vector<uint8> itemTypeAvailable;
                if (itemPrototype->Class == ITEM_CLASS_WEAPON)
                {
                    itemTypeAvailable = GetWeaponAvailableForClass(playerClass);
                }
                else if (itemPrototype->Class == ITEM_CLASS_ARMOR)
                {
                    itemTypeAvailable = GetArmorAvailableForClass(playerClass);
                }

                for (uint8 subclassAvailable : itemTypeAvailable)
                {
                    if (itemPrototype->SubClass == subclassAvailable)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool TransmogModule::IsValidTransmog(const Player* player, uint32 itemEntry) const
    {
        return IsValidTransmog(player, sObjectMgr.GetItemPrototype(itemEntry));
    }

    void TransmogModule::LoadActiveTransmogs(Player* player)
    {
        const uint32 playerID = player->GetObjectGuid().GetCounter();
        entryMap.erase(playerID);

        auto result = CharacterDatabase.PQuery("SELECT `item_guid`, `transmog_entry` FROM `custom_transmog_active` WHERE `player` = %u", playerID);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                const ObjectGuid itemGUID = ObjectGuid(HIGHGUID_ITEM, (fields[0].GetUInt32()));
                const uint32 transmogEntry = fields[1].GetUInt32();
                if (sObjectMgr.GetItemPrototype(transmogEntry))
                {
                    dataMap[itemGUID] = playerID;
                    entryMap[playerID][itemGUID] = transmogEntry;
                }
                else
                {
                    sLog.outError("Item entry (Entry: %u, player ID: %u) does not exist, ignoring.", transmogEntry, playerID);
                    CharacterDatabase.PExecute("DELETE FROM `custom_transmog_active` WHERE `transmog_entry` = %u", transmogEntry);
                }
            } 
            while (result->NextRow());

            // Reload the item visuals
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    UpdateItemAppearance(player, item);
                }
            }
        }
    }

    void TransmogModule::SendActiveTransmogs(const Player* player)
    {
        const auto transmogItems = GetTransmogrifiedItems(player, true);
        if (!transmogItems.empty())
        {
            bool first = true;
            std::ostringstream out;
            for (const auto& pair : transmogItems)
            {
                const uint32 itemSlot = pair.first->GetSlot();
                const uint32 transmogEntry = pair.second;
                if (first)
                {
                    first = false;
                    out << helper::FormatString("%u,%u", itemSlot, transmogEntry);
                }
                else
                {
                    out << helper::FormatString(":%u,%u", itemSlot, transmogEntry);
                }
            }

            SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString("TransmogStatus:%u:%s", transmogItems.size(), out.str().c_str()));
        }
        else
        {
            SendAddOnMessage(player, GetChatCommandPrefix(), "TransmogStatus:0");
        }
    }

    void TransmogModule::LoadDiscoveredTransmogs(const Player* player)
    {
        if (player)
        {
            const uint32 playerID = player->GetObjectGuid().GetCounter();
            auto& discoveredTransmogs = playerDiscoveredTransmogs[playerID];
            discoveredTransmogs.clear();

            auto result = CharacterDatabase.PQuery("SELECT `item_entry` FROM `custom_transmog_discovered` WHERE `player` = %u", playerID);
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    const uint32 itemEntry = fields[0].GetUInt32();
                    if (IsValidTransmog(player, itemEntry))
                    {
                        AddDiscoveredTransmog(player, itemEntry, false, false);
                    }
                    else
                    {
                        sLog.outError("Item entry (Entry: %u, player ID: %u) does not exist, ignoring.", itemEntry, playerID);
                        CharacterDatabase.PExecute("DELETE FROM `custom_transmog_discovered` WHERE `item_entry` = %u", itemEntry);
                    }
                } 
                while (result->NextRow());
            }
            else
            {
                // Calculate all available transmog from the items in the inventory
                auto CheckTransmogItem = [&](Item* inventoryItem)
                {
                    const uint32 itemEntry = inventoryItem->GetEntry();
                    if (IsValidTransmog(player, itemEntry))
                    {
                        AddDiscoveredTransmog(player, itemEntry, false, true);
                    }
                };

                helper::ForEachEquippedItem(player, CheckTransmogItem);
                helper::ForEachInventoryItem(player, CheckTransmogItem);
                helper::ForEachBankItem(player, CheckTransmogItem);
            }
        }
    }

    void TransmogModule::AddDiscoveredTransmog(const Player* player, uint32 itemEntry, bool sendToClient, bool addToDB)
    {
        const uint32 playerID = player->GetObjectGuid().GetCounter();
        auto it = playerDiscoveredTransmogs.find(playerID);
        if (it != playerDiscoveredTransmogs.end())
        {
            auto& availableTransmogs = it->second;
            if (const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemEntry))
            {
                if (availableTransmogs.find(proto->DisplayInfoID) == availableTransmogs.end())
                {
                    TransmogItem transmogItem;
                    transmogItem.itemClass = proto->Class;
                    transmogItem.itemSubclass = proto->SubClass;
                    transmogItem.itemID = proto->ItemId;
                    transmogItem.displayID = proto->DisplayInfoID;

                    if (player->ViableEquipSlots(proto, &transmogItem.slots[0]))
                    {
                        if (addToDB)
                        {
                            CharacterDatabase.PExecute("INSERT INTO `custom_transmog_discovered` (`player`, `item_entry`) VALUES (%u, %u)", playerID, transmogItem.itemID);
                        }

                        availableTransmogs.insert(std::make_pair(transmogItem.displayID, transmogItem));

                        if (sendToClient)
                        {
                            // Send message to client addon when new item has been discovered
                            SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString("NewTransmog:%u", transmogItem.itemID));

                            // Refresh the client addon available transmogs
                             for (uint8 slot : transmogItem.slots)
                            {
                                if (slot != NULL_SLOT)
                                {
                                    SendDiscoveredTransmogs(player, slot, transmogItem.itemClass, transmogItem.itemSubclass);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void TransmogModule::SendDiscoveredTransmogs(const Player* player, int8 slot, int8 itemClass, int8 itemSubclass)
    {
        const auto& discoveredTransmogs = playerDiscoveredTransmogs[player->GetObjectGuid().GetCounter()];

        // Sort by item types
        //        slot            item class + item subclass      item id
        std::map <uint8, std::map<uint32, std::vector<uint32>>> discoveredTransmogsFormatted;
        for (const auto& pair : discoveredTransmogs)
        {
            const TransmogItem& transmogItem = pair.second;

            if (itemClass >= 0 && itemClass != transmogItem.itemClass)
                continue;

            if (itemSubclass >= 0 && itemSubclass != transmogItem.itemSubclass)
                continue;

            const uint32 transmogItemClass = transmogItem.itemClass + transmogItem.itemSubclass;
            for (uint8 transmogSlot : transmogItem.slots)
            {
                if (slot >= 0 && slot != transmogSlot)
                    continue;

                if (transmogSlot != NULL_SLOT)
                {
                    bool front = false;
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, transmogSlot))
                    {
                        front = item->GetEntry() == transmogItem.itemID;
                    }

                    if (front)
                    {
                        discoveredTransmogsFormatted[transmogSlot][transmogItemClass].insert(discoveredTransmogsFormatted[transmogSlot][transmogItemClass].begin(), transmogItem.itemID);
                    }
                    else
                    {
                        discoveredTransmogsFormatted[transmogSlot][transmogItemClass].push_back(transmogItem.itemID);
                    }
                }
            }
        }

        for (auto& itemSlotIt : discoveredTransmogsFormatted)
        {
            const uint8 itemSlot = itemSlotIt.first;
            for (auto& itemClassIt : itemSlotIt.second)
            {
                const uint8 itemClass = itemClassIt.first;
                const std::vector<uint32>& itemIDs = itemClassIt.second;
                const uint32 amount = itemIDs.size();

                SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString
                (
                    "AvailableTransmogs:%u:%u:%u:%s",
                    itemSlot,
                    itemClass,
                    amount,
                    "start"
                ));

                uint32 itemIDCounter = 0;
                constexpr uint32 itemIDLimit = 10;

                bool first = true;
                std::ostringstream out;
                for (uint32 itemID : itemIDs)
                {
                    if (first)
                    {
                        first = false;
                        out << itemID;
                    }
                    else
                    {
                        out << ":" << itemID;
                    }

                    itemIDCounter++;

                    if (itemIDCounter >= itemIDLimit)
                    {
                        SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString
                        (
                            "AvailableTransmogs:%u:%u:%u:%s",
                            itemSlot,
                            itemClass,
                            amount,
                            out.str().c_str()
                        ));

                        itemIDCounter = 0;
                        first = true;
                        out.str("");
                    }
                }

                if (!out.str().empty())
                {
                    SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString
                    (
                        "AvailableTransmogs:%u:%u:%u:%s",
                        itemSlot,
                        itemClass,
                        amount,
                        out.str().c_str()
                    ));
                }

                SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString
                (
                    "AvailableTransmogs:%u:%u:%u:%s",
                    itemSlot,
                    itemClass,
                    amount,
                    "end"
                ));
            }
        }
    }

    std::pair<uint32, uint32> TransmogModule::CalculateTransmogCost(uint32 itemEntry) const
    {
        std::pair<uint32, uint32> result = { 0, 0 };
        uint32& cost = result.first;
        uint32& tokenID = result.second;

        if (GetConfig()->tokenRequired)
        {
            tokenID = GetConfig()->tokenEntry;
            cost = GetConfig()->tokenAmount;
        }
        else
        {
            if (const ItemPrototype* proto = sObjectMgr.GetItemPrototype(itemEntry))
            {
                cost = proto->SellPrice ? proto->SellPrice : 100U;
                cost += GetConfig()->costFee;
                cost *= GetConfig()->costMultiplier;
            }
        }

        return result;
    }

    void TransmogModule::SendTransmogCost(const Player* player, const std::vector<std::pair<uint32, uint32>>& slots) const
    {
        if (player)
        {
            uint32 cost = 0;
            uint32 tokenID = 0;
            bool canPurchase = false;

            if (slots.size() > 0)
            {
                for (auto& pair : slots)
                {
                    const uint32 slot = pair.first;
                    const uint32 itemID = pair.second;
                    if (const Item* slotItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        std::pair<uint32, uint32> itemCost = CalculateTransmogCost(slotItem->GetEntry());
                        cost += itemCost.first;
                        tokenID = itemCost.second;
                    }
                }

                canPurchase = tokenID ? player->HasItemCount(tokenID, cost) : player->GetMoney() >= cost;
            }

            SendAddOnMessage(player, GetChatCommandPrefix(), helper::FormatString
            (
                "TransmogCost:%u:%u:%u",
                cost,
                tokenID,
                canPurchase ? 1 : 0
            ));
        }
    }
}