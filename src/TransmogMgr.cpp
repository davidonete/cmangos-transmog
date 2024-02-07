#include "TransmogMgr.h"
#include "TransmogConfig.h"

#include "Entities/GossipDef.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"

void TransmogMgr::Init()
{
	sTransmogConfig.Initialize();

	if (sTransmogConfig.enabled)
	{
		// Delete corrupted transmog config
		CharacterDatabase.Execute("DELETE FROM custom_transmogrification WHERE NOT EXISTS (SELECT 1 FROM item_instance WHERE item_instance.guid = custom_transmogrification.GUID)");
	
		if (sTransmogConfig.presetsEnabled)
		{
			// Delete presets for non existing player characters
			CharacterDatabase.Execute("DELETE FROM `custom_transmogrification_sets` WHERE NOT EXISTS(SELECT 1 FROM characters WHERE characters.guid = custom_transmogrification_sets.Owner)");
		
            sWorld.ExecuteForAllSessions([this](WorldSession& worldSession)
            {
				LoadPlayerPresets(worldSession.GetPlayer());
            });
		}
	}
}

void TransmogMgr::OnPlayerLogout(Player* player)
{
	if (sTransmogConfig.enabled)
	{
        if (player)
        {
            const ObjectGuid playerID = player->GetObjectGuid();
            for (auto it = entryMap[playerID].begin(); it != entryMap[playerID].end(); ++it)
            {
                dataMap.erase(it->first);
            }

            entryMap.erase(playerID);

            if (sTransmogConfig.presetsEnabled)
            {
				UnloadPlayerPresets(player);
            }
        }
	}
}

const char* GetSlotName(uint8 slot)
{
    switch (slot)
    {
		case EQUIPMENT_SLOT_HEAD: return "Head";
		case EQUIPMENT_SLOT_SHOULDERS: return "Shoulders";
		case EQUIPMENT_SLOT_BODY: return "Shirt";
		case EQUIPMENT_SLOT_CHEST: return "Chest";
		case EQUIPMENT_SLOT_WAIST: return "Waist";
		case EQUIPMENT_SLOT_LEGS: return "Legs";
		case EQUIPMENT_SLOT_FEET: return "Feet";
		case EQUIPMENT_SLOT_WRISTS: return "Wrists";
		case EQUIPMENT_SLOT_HANDS: return "Hands";
		case EQUIPMENT_SLOT_BACK: return "Back";
		case EQUIPMENT_SLOT_MAINHAND: return "Main hand";
		case EQUIPMENT_SLOT_OFFHAND: return "Off hand";
		case EQUIPMENT_SLOT_RANGED: return "Ranged";
		case EQUIPMENT_SLOT_TABARD: return "Tabard";
		default: return nullptr;
    }
}

uint32 GetTransmogItemPrice(const Item* item)
{
    uint32 price = 0U;
    if (item)
    {
        const ItemPrototype* proto = item->GetProto();
        price = proto->SellPrice ? proto->SellPrice : 100U;
        price += sTransmogConfig.costFee;
        price *= sTransmogConfig.costMultiplier;
    }

    return price;
}

bool TransmogMgr::OnPlayerGossipHello(Player* player, Creature* creature)
{
    if (sTransmogConfig.enabled)
    {
        if (player && creature)
        {
			// Check if speaking with transmog npc
			if (creature->GetEntry() != TRANSMOG_NPC_ENTRY)
				return false;

            player->GetPlayerMenu()->ClearMenus();
			player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "How does transmogrification work?", EQUIPMENT_SLOT_END + 9, 0, "", 0);

            // Only show the menu option for items that you have equipped
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
            {
                // No point in checking for tabard, shirt, necklace, rings or trinkets
                if (slot == EQUIPMENT_SLOT_NECK || 
					slot == EQUIPMENT_SLOT_FINGER1 || 
					slot == EQUIPMENT_SLOT_FINGER2 || 
					slot == EQUIPMENT_SLOT_TRINKET1 || 
					slot == EQUIPMENT_SLOT_TRINKET2 || 
					slot == EQUIPMENT_SLOT_TABARD || 
					slot == EQUIPMENT_SLOT_BODY)
				{
                    continue;
				}

                // Only show the menu option for transmogrifiers that you have per slot
                if (Item* pEquippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    // Only show the menu option if there is a transmogrifying option available
                    bool hasOption = false;

					// Check in main inventory
                    if (!hasOption)
                    {
                        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
                        {
                            if (Item* pTransmogrifier = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
                                if (!CanTransmogItemWithItem(player, pEquippedItem->GetProto(), pTransmogrifier->GetProto()))
								{
                                    continue;
								}

                                hasOption = true;
                                break;
                            }
                        }
                    }

					// Check in the other bags
                    if (!hasOption)
                    {
                        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                        {
                            if (const auto pBag = dynamic_cast<Bag*>(player->GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
                            {
                                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                                {
                                    if (Item* pTransmogrifier = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
                                        if (!CanTransmogItemWithItem(player, pEquippedItem->GetProto(), pTransmogrifier->GetProto()))
										{
                                            continue;
										}

                                        hasOption = true;
                                        break;
                                    }
                                }
                            }

                            // If we find a suitable transmog in the first bag then there's no point in checking the rest
                            if (hasOption)
							{
                                break;
							}
                        }
                    }

					// Check in the bank's main inventory
                    if (!hasOption)
                    {
                        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
                        {
                            if (Item* pTransmogrifier = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
                                if (!CanTransmogItemWithItem(player, pEquippedItem->GetProto(), pTransmogrifier->GetProto()))
								{
                                    continue;
								}

                                hasOption = true;
                                break;
                            }
                        }
                    }

					// Check in the bank's other bags
                    if (!hasOption)
                    {
                        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
                        {
                            if (const auto pBag = dynamic_cast<Bag*>(player->GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
                            {
                                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                                {
                                    if (Item* pTransmogrifier = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
                                        if (!CanTransmogItemWithItem(player, pEquippedItem->GetProto(), pTransmogrifier->GetProto()))
										{
                                            continue;
										}

                                        hasOption = true;
                                        break;
                                    }
                                }
                            }

                            if (hasOption)
							{
                                break;
							}
                        }
                    }

                    if (hasOption)
                    {
                        std::string SlotName = GetSlotName(slot);
                        if (SlotName.length() > 0)
						{
                            player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TABARD, SlotName.c_str(), EQUIPMENT_SLOT_END, slot, "", 0);
						}
                    }
                }
            }

            // Remove all transmogrifiers
            player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE, "Remove all transmogrifications.", EQUIPMENT_SLOT_END + 2, 0, "", 0);
            player->GetPlayerMenu()->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
            return true;
		}
	}

	return false;
}

bool TransmogMgr::OnPlayerGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (sTransmogConfig.enabled)
    {
        if (player && creature)
        {
            // Check if speaking with transmog npc
            if (creature->GetEntry() != TRANSMOG_NPC_ENTRY)
                return false;

			const ObjectGuid playerId = player->GetObjectGuid();

            player->GetPlayerMenu()->ClearMenus();
            WorldSession* session = player->GetSession();
            switch (sender)
            {
				// Show items you can use
				case EQUIPMENT_SLOT_END:
				{
					ShowTransmogItems(player, creature, action);
					break;
				}

				// Main menu
				case EQUIPMENT_SLOT_END + 1: 
				{
					OnPlayerGossipHello(player, creature);
					break;
				}

				// Remove Transmogrifications
				case EQUIPMENT_SLOT_END + 2: 
				{
					bool removed = false;
					CharacterDatabase.BeginTransaction();
					for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
					{
						if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
						{
							if (!GetFakeEntry(newItem->GetObjectGuid()))
								continue;

							DeleteFakeEntry(player, slot, newItem);
							removed = true;
						}
					}

					if (removed)
					{
						session->SendAreaTriggerMessage("%s", LANG_ERR_UNTRANSMOG_OK);
						CharacterDatabase.CommitTransaction();
					}
					else
					{
						session->SendNotification(LANG_ERR_UNTRANSMOG_NO_TRANSMOGS);
					}

					OnPlayerGossipHello(player, creature);
					break;
				} 

				// Remove Transmogrification from single item
				case EQUIPMENT_SLOT_END + 3: 
				{
					if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
					{
						if (GetFakeEntry(newItem->GetObjectGuid()))
						{
							DeleteFakeEntry(player, action, newItem);
							session->SendAreaTriggerMessage("%s", LANG_ERR_UNTRANSMOG_OK);
						}
						else
						{
							session->SendNotification(LANG_ERR_UNTRANSMOG_NO_TRANSMOGS);
						}
					}

					OnPlayerGossipSelect(player, creature, EQUIPMENT_SLOT_END, action);
					break;
				}

				// Presets menu
				case EQUIPMENT_SLOT_END + 4: 
				{
					if (!sTransmogConfig.presetsEnabled)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "How do sets work?", EQUIPMENT_SLOT_END + 10, 0, "", 0);
					
					for (auto it = presetByName[playerId].begin(); it != presetByName[playerId].end(); ++it)
					{
						player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "zz?", EQUIPMENT_SLOT_END + 6, it->first, "", 0);
					}

					if (presetByName[playerId].size() < sTransmogConfig.maxPresets)
					{
						player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Save set", EQUIPMENT_SLOT_END + 8, 0, "", 0);
					}

					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 1, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
					break;
				} 

				// Use preset
				case EQUIPMENT_SLOT_END + 5: 
				{
					if (!sTransmogConfig.presetsEnabled)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					// action = presetID
					for (auto it = presetById[playerId][action].begin(); it != presetById[playerId][action].end(); ++it)
					{
						if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, it->first))
						{
							PresetTransmogItem(player, item, it->second, it->first);
						}
					}

					OnPlayerGossipSelect(player, creature, EQUIPMENT_SLOT_END + 6, action);
					break;
				}

				// View preset
				case EQUIPMENT_SLOT_END + 6: 
				{
					if (!sTransmogConfig.presetsEnabled)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					// action = presetID
					for (auto it = presetById[playerId][action].begin(); it != presetById[playerId][action].end(); ++it)
					{
						player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "zzz", sender, action, "", 0);
					}

					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Use this set", EQUIPMENT_SLOT_END + 5, action, "Using this set for transmogrify will bind transmogrified items to you and make them non-refundable and non-tradeable.\nDo you wish to continue?\n\n" + presetByName[playerId][action], false);
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Delete set", EQUIPMENT_SLOT_END + 7, action, "Are you sure you want to delete " + presetByName[playerId][action] + "?", false);
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 4, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
					break;
				} 

				// Delete preset
				case EQUIPMENT_SLOT_END + 7: 
				{
					if (!sTransmogConfig.presetsEnabled)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					// action = presetID
					CharacterDatabase.PExecute("DELETE FROM `custom_transmogrification_sets` WHERE Owner = %u AND PresetID = %u", playerId, action);
					presetById[playerId][action].clear();
					presetById[playerId].erase(action);
					presetByName[playerId].erase(action);

					OnPlayerGossipSelect(player, creature, EQUIPMENT_SLOT_END + 4, 0);
					break;
				} 

				// Save preset
				case EQUIPMENT_SLOT_END + 8: 
				{
					if (!sTransmogConfig.presetsEnabled || presetByName[playerId].size() >= sTransmogConfig.maxPresets)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					uint32 cost = 0;
					bool canSave = false;
					for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
					{
						if (!GetSlotName(slot))
							continue;

						if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
						{
							uint32 entry = GetFakeEntry(newItem->GetObjectGuid());
							if (!entry)
								continue;

							const ItemPrototype* temp = sObjectMgr.GetItemPrototype(entry);
							if (!temp)
								continue;

							if (!SuitableForTransmog(player, temp))
								continue;

							cost += GetTransmogItemPrice(newItem);
							canSave = true;
							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "zzzu", EQUIPMENT_SLOT_END + 8, 0, "", 0);
						}
					}
					if (canSave)
						player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Save set", 0, 0, "Insert set name", true);

					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Update menu", sender, action, "", 0);
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 4, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
					break;
				} 

				// Preset info
				case EQUIPMENT_SLOT_END + 10: 
				{
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 4, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_PRESET_INFO_NPC_TEXT, creature->GetObjectGuid());
					break;
				} 

				// Transmog info
				case EQUIPMENT_SLOT_END + 9: 
				{
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 1, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_INFO_NPC_TEXT, creature->GetObjectGuid());
					break;
				} 

				// TransmogItem
				default: 
				{
					if (!sender && !action)
					{
						OnPlayerGossipHello(player, creature);
						return true;
					}

					// sender = slot, action = display
					TransmogLanguage res = TransmogItem(player, ObjectGuid(HIGHGUID_ITEM, action), sender, false);
					if (res == LANG_ERR_TRANSMOG_OK)
					{
						session->SendAreaTriggerMessage("%s", LANG_ERR_TRANSMOG_OK);
					}
					else
					{
						session->SendNotification(res);
					}

					player->GetPlayerMenu()->CloseGossip();
					break;
				} 
            }

            return true;
		}
	}

	return false;
}

void TransmogMgr::LoadPlayerPresets(Player* player)
{
    if (player)
    {
        const ObjectGuid playerID = player->GetObjectGuid();
        auto result = CharacterDatabase.PQuery("SELECT `PresetID`, `SetName`, `SetData` FROM `custom_transmogrification_sets` WHERE Owner = %u", playerID.GetCounter());
        if (result)
        {
            do
            {
				Field* fields = result->Fetch();
                const uint8 presetID = fields[0].GetUInt8();
                const std::string presetName = fields[1].GetString();
                std::istringstream presetData (fields[2].GetString());

                while (presetData.good())
                {
                    uint32 slot;
                    uint32 entry;
                    presetData >> slot >> entry;

                    if (presetData.fail())
					{
                        break;
					}

                    if (slot >= EQUIPMENT_SLOT_END)
                    {
                        sLog.outError("Item entry (FakeEntry: %u, player: %s, slot: %u, presetId: %u) has invalid slot, ignoring.", entry, std::to_string(playerID).c_str(), slot, uint32(presetID));
                        continue;
                    }

                    if (sObjectMgr.GetItemPrototype(entry))
					{
                        presetById[playerID][presetID][slot] = entry;
					}
                }

                if (!presetById[playerID][presetID].empty())
                {
                    presetByName[playerID][presetID] = presetName;
                }
                else
                {
                    presetById[playerID].erase(presetID);
                    CharacterDatabase.PExecute("DELETE FROM `custom_transmogrification_sets` WHERE Owner = %u AND PresetID = %u", playerID.GetCounter(), presetID);
                }
            } 
			while (result->NextRow());
        }
	}
}

void TransmogMgr::UnloadPlayerPresets(Player* player)
{
	if (player)
	{
		const ObjectGuid playerID = player->GetObjectGuid();
        for (auto it = presetById[playerID].begin(); it != presetById[playerID].end(); ++it)
        {
            it->second.clear();
        }

        presetById[playerID].clear();
        presetByName[playerID].clear();
	}
}

bool IsRangedWeapon(uint32 Class, uint32 SubClass)
{
    return Class == ITEM_CLASS_WEAPON && 
			(SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
             SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
             SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW);
}

bool TransmogMgr::CanTransmogItemWithItem(Player* player, const ItemPrototype* target, const ItemPrototype* source) const
{
    if (!target || !source)
        return false;

	// Same item
    if (source->ItemId == target->ItemId)
        return false;

	// Same item model
    if (source->DisplayInfoID == target->DisplayInfoID)
        return false;

	// Different item type
    if (source->Class != target->Class)
        return false;

	// Invalid types
    if (source->InventoryType == INVTYPE_BAG ||
        source->InventoryType == INVTYPE_RELIC ||
        source->InventoryType == INVTYPE_FINGER ||
        source->InventoryType == INVTYPE_TRINKET ||
        source->InventoryType == INVTYPE_AMMO ||
        source->InventoryType == INVTYPE_QUIVER)
	{
        return false;
	}

	// Invalid types
    if (target->InventoryType == INVTYPE_BAG ||
        target->InventoryType == INVTYPE_RELIC ||
        target->InventoryType == INVTYPE_FINGER ||
        target->InventoryType == INVTYPE_TRINKET ||
        target->InventoryType == INVTYPE_AMMO ||
        target->InventoryType == INVTYPE_QUIVER)
	{
        return false;
	}

    if (!SuitableForTransmog(player, target) || !SuitableForTransmog(player, source))
        return false;

	// Ranged weapon check
    if (IsRangedWeapon(source->Class, source->SubClass) != IsRangedWeapon(target->Class, target->SubClass))
        return false;

    if (source->InventoryType != target->InventoryType)
    {
        if (source->Class == ITEM_CLASS_WEAPON && 
			!(IsRangedWeapon(target->Class, target->SubClass) 
			  || (target->InventoryType == INVTYPE_WEAPON || 
				  target->InventoryType == INVTYPE_2HWEAPON || 
				  target->InventoryType == INVTYPE_WEAPONMAINHAND || 
				  target->InventoryType == INVTYPE_WEAPONOFFHAND)
			  && (source->InventoryType == INVTYPE_WEAPON || 
				  source->InventoryType == INVTYPE_2HWEAPON || 
				  source->InventoryType == INVTYPE_WEAPONMAINHAND || 
				  source->InventoryType == INVTYPE_WEAPONOFFHAND)))
		{
            return false;
		}

        if (source->Class == ITEM_CLASS_ARMOR &&
            !((source->InventoryType == INVTYPE_CHEST || 
			   source->InventoryType == INVTYPE_ROBE) 
			  &&
               (target->InventoryType == INVTYPE_CHEST || 
			    target->InventoryType == INVTYPE_ROBE)))
		{
            return false;
		}
    }

    return true;
}

bool TransmogMgr::SuitableForTransmog(Player* player, const ItemPrototype* proto) const
{
    if (!player || !proto)
        return false;

	// Invalid types
    if (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON)
        return false;

    // Don't allow fishing poles
    if (proto->Class == ITEM_CLASS_WEAPON && proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        return false;

    // Check skill requirements
    if (proto->RequiredSkill != 0)
    {
        if (player->GetSkillValue(static_cast<uint16>(proto->RequiredSkill)) == 0)
            return false;

        if (player->GetSkillValue(static_cast<uint16>(proto->RequiredSkill)) < proto->RequiredSkillRank)
            return false;
    }

    // Check spell requirements
    if (proto->RequiredSpell != 0 && !player->HasSpell(proto->RequiredSpell))
        return false;

    // Check level requirements
    if (player->GetLevel() < proto->RequiredLevel)
        return false;

    return true;
}

std::string GetItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y)
{
    std::ostringstream ss;
    ss << "|TInterface";

    const ItemPrototype* temp = sObjectMgr.GetItemPrototype(entry);
    const ItemDisplayInfoEntry* dispInfo = nullptr;
    if (temp)
    {
        GameObjectDisplayInfoEntry const* info = sGameObjectDisplayInfoStore.LookupEntry(temp->DisplayInfoID);
        dispInfo = sItemStorage.LookupEntry<ItemDisplayInfoEntry>(temp->DisplayInfoID);
        if (dispInfo)
		{
            ss << "/ICONS/" << dispInfo->ID;
		}
    }

    if (!dispInfo)
	{
        ss << "/InventoryItems/WoWUnknownItem01";
	}

    ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
    return ss.str();
}

std::string GetItemLink(Item* item, WorldSession* session)
{
	int loc_idx = session->GetSessionDbLocaleIndex();
	const ItemPrototype* temp = item->GetProto();
	std::string name = temp->Name1;

	std::ostringstream oss;
	oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
		"|Hitem:" << temp->ItemId << ":" <<
		item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_0) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_1) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_2) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_3) << ":" <<
		item->GetItemRandomPropertyId() << ":" << item->GetItemSuffixFactor() << ":" <<
		item->GetOwner()->GetLevel() << "|h[" << name << "]|h|r";

	return oss.str();
}

std::string GetItemLink(uint32 entry, WorldSession* session)
{
	const ItemPrototype* temp = sObjectMgr.GetItemPrototype(entry);
	int loc_idx = session->GetSessionDbLocaleIndex();
	std::string name = temp->Name1;
	std::ostringstream oss;
	oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec << "|Hitem:" << entry << ":0:0:0:0:0:0:0:0:0|h[" << name << "]|h|r";
	return oss.str();
}

void TransmogMgr::ShowTransmogItems(Player* player, Creature* creature, uint8 slot)
{
    WorldSession* session = player->GetSession();
    Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (oldItem)
    {
        uint32 limit = 0;
        const uint32 price = GetTransmogItemPrice(oldItem);

        std::ostringstream ss;
        ss << std::endl;
        if (sTransmogConfig.tokenRequired)
		{
            ss << std::endl << std::endl << sTransmogConfig.tokenAmount << " x " << GetItemLink(sTransmogConfig.tokenEntry, session);
		}

        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (limit > MAX_OPTIONS)
                break;

            Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!newItem)
                continue;

            if (!CanTransmogItemWithItem(player, oldItem->GetProto(), newItem->GetProto()))
                continue;

            if (GetFakeEntry(oldItem->GetObjectGuid()) == newItem->GetEntry())
                continue;

            player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, ss.str(), price, false, "", 0);

			++limit;
        }

        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            const auto bag = dynamic_cast<Bag*>(player->GetItemByPos(INVENTORY_SLOT_BAG_0, i));
            if (!bag)
                continue;

            for (uint32 j = 0; j < bag->GetBagSize(); ++j)
            {
                if (limit > MAX_OPTIONS)
                    break;

                Item* newItem = player->GetItemByPos(i, j);
                if (!newItem)
                    continue;

                if (!CanTransmogItemWithItem(player, oldItem->GetProto(), newItem->GetProto()))
                    continue;

                if (GetFakeEntry(oldItem->GetObjectGuid()) == newItem->GetEntry())
                    continue;

                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, 
																	 GetItemIcon(newItem->GetEntry(), 30, 30, -18, 0) + GetItemLink(newItem, session), 
																	 slot, 
																	 newItem->GetObjectGuid().GetCounter(), 
																	 "Using this item for transmogrify will bind it to you and make it non-refundable and non-tradeable.\nDo you wish to continue?\n\n" + GetItemIcon(newItem->GetEntry(), 40, 40, -15, -10) + GetItemLink(newItem, session) + ss.str(),  
																	 false);
            
				++limit;
			}
        }
    }

	player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Remove transmogrification", EQUIPMENT_SLOT_END + 3, slot, "Remove transmogrification from the slot?", false);
    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Update menu", EQUIPMENT_SLOT_END, slot, "", 0);
    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, "Back...", EQUIPMENT_SLOT_END + 1, 0, "", 0);
    player->GetPlayerMenu()->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetObjectGuid());
}

void TransmogMgr::UpdateTransmogItem(Player* player, Item* item) const
{
    if (item->IsEquipped())
	{
        player->SetVisibleItemSlot(item->GetSlot(), item);
	}
}

void TransmogMgr::PresetTransmogItem(Player* player, Item* itemTransmogrified, uint32 fakeEntry, uint8 slot)
{
    if (!sTransmogConfig.presetsEnabled)
        return;

    if (!player || !itemTransmogrified)
        return;

    if (slot >= EQUIPMENT_SLOT_END)
        return;

    if (!CanTransmogItemWithItem(player, itemTransmogrified->GetProto(), sObjectMgr.GetItemPrototype(fakeEntry)))
        return;

    if (GetFakeEntry(itemTransmogrified->GetObjectGuid()))
	{
        DeleteFakeEntry(player, slot, itemTransmogrified);
	}

    SetFakeEntry(player, fakeEntry, itemTransmogrified);
    itemTransmogrified->SetOwnerGuid(player->GetObjectGuid());
}

TransmogLanguage TransmogMgr::TransmogItem(Player* player, ObjectGuid sourceItemGUID, uint8 targetItemSlot, bool noCost)
{
    if (targetItemSlot >= EQUIPMENT_SLOT_END)
    {
        sLog.outError("TransmogItem wrong slot: %u", targetItemSlot);
        return LANG_ERR_TRANSMOG_INVALID_SLOT;
    }

    Item* sourceItem = nullptr;
    if (sourceItemGUID)
    {
        sourceItem = player->GetItemByGuid(sourceItemGUID);
        if (!sourceItem)
        {
            return LANG_ERR_TRANSMOG_MISSING_SRC_ITEM;
        }
    }

    Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, targetItemSlot);
    if (!targetItem)
    {
        return LANG_ERR_TRANSMOG_MISSING_DEST_ITEM;
    }

    if (!sourceItem) // reset look newEntry
    {
        // Custom
        DeleteFakeEntry(player, targetItemSlot, targetItem);
    }
    else
    {
        if (!CanTransmogItemWithItem(player, targetItem->GetProto(), sourceItem->GetProto()))
            return LANG_ERR_TRANSMOG_INVALID_ITEMS;

        if (GetFakeEntry(targetItem->GetObjectGuid()) == sourceItem->GetEntry())
            return LANG_ERR_TRANSMOG_SAME_DISPLAYID;

        if (!noCost)
        {
            if (sTransmogConfig.tokenRequired)
            {
                if (player->HasItemCount(sTransmogConfig.tokenEntry, sTransmogConfig.tokenAmount))
				{
                    player->DestroyItemCount(sTransmogConfig.tokenEntry, sTransmogConfig.tokenAmount, true);
				}
                else
				{
                    return LANG_ERR_TRANSMOG_NOT_ENOUGH_TOKENS;
				}
            }

            const uint32 price = GetTransmogItemPrice(targetItem);
            if (price)
            {
                if (player->GetMoney() < price)
				{
                    return LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY;
				}

                player->ModifyMoney(-(int)price);
            }
        }

        SetFakeEntry(player, sourceItem->GetEntry(), targetItem);
        targetItem->SetOwnerGuid(player->GetObjectGuid());

        if (sourceItem->GetProto()->Bonding == BIND_WHEN_EQUIPPED || sourceItem->GetProto()->Bonding == BIND_WHEN_USE)
		{
            sourceItem->SetBinding(true);
		}

        sourceItem->SetOwnerGuid(player->GetObjectGuid());
    }

    return LANG_ERR_TRANSMOG_OK;
}

uint32 TransmogMgr::GetFakeEntry(const ObjectGuid& itemGUID) const
{
    const auto itr = dataMap.find(itemGUID);
    if (itr == dataMap.end()) return 0;
    const auto itr2 = entryMap.find(itr->second);
    if (itr2 == entryMap.end()) return 0;
    const auto itr3 = itr2->second.find(itemGUID);
    if (itr3 == itr2->second.end()) return 0;
    return itr3->second;
}

void TransmogMgr::SetFakeEntry(Player* player, uint32 newEntry, Item* itemTransmogrified)
{
    const ObjectGuid itemGUID = itemTransmogrified->GetObjectGuid();
    entryMap[player->GetObjectGuid()][itemGUID] = newEntry;
    dataMap[itemGUID] = player->GetObjectGuid();
    CharacterDatabase.PExecute("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", itemGUID.GetCounter(), newEntry, player->GetObjectGuid());
    UpdateTransmogItem(player, itemTransmogrified);
}

void TransmogMgr::DeleteFakeEntry(Player* player, uint8, Item* itemTransmogrified)
{
    DeleteFakeEntryFromDB(itemTransmogrified->GetObjectGuid());
    UpdateTransmogItem(player, itemTransmogrified);
}

void TransmogMgr::DeleteFakeEntryFromDB(const ObjectGuid& itemGUID)
{
    if (dataMap.find(itemGUID) != dataMap.end())
    {
        if (entryMap.find(dataMap[itemGUID]) != entryMap.end())
		{
            entryMap[dataMap[itemGUID]].erase(itemGUID);
		}

        dataMap.erase(itemGUID);
    }

    CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemGUID.GetCounter());
}

/*
#ifdef PRESETS

void TransmogMgr::PresetTransmogItem(Player* player, Item* itemTransmogrified, uint32 fakeEntry, uint8 slot)
{
	if (!EnableSets)
		return;
	if (!player || !itemTransmogrified)
		return;
	if (slot >= EQUIPMENT_SLOT_END)
		return;
	if (!CanTransmogItemItemWithItem(player, itemTransmogrified->GetProto(), sObjectMgr.GetItemPrototype(fakeEntry)))
		return;

	// [AZTH] Custom
	if (GetFakeEntry(itemTransmogrified->GetObjectGuid()))
		DeleteFakeEntry(player, slot, itemTransmogrified);

	SetFakeEntry(player, fakeEntry, itemTransmogrified); // newEntry
	itemTransmogrified->SetOwnerGuid(player->GetObjectGuid());
}

void TransmogMgr::LoadPlayerSets(ObjectGuid pGUID)
{
	for (presetData::iterator it = presetById[pGUID].begin(); it != presetById[pGUID].end(); ++it)
		it->second.clear();

	presetById[pGUID].clear();

	presetByName[pGUID].clear();

	auto result = CharacterDatabase.PQuery("SELECT `PresetID`, `SetName`, `SetData` FROM `custom_transmogrification_sets` WHERE Owner = %u", pGUID.GetCounter());
	if (result)
	{
		do
		{
			uint8 PresetID = (*result)[0].GetUInt8();
			std::string SetName = (*result)[1].GetString();
			std::istringstream SetData((*result)[2].GetString());
			while (SetData.good())
			{
				uint32 slot;
				uint32 entry;
				SetData >> slot >> entry;
				if (SetData.fail())
					break;
				if (slot >= EQUIPMENT_SLOT_END)
				{
					sLog.outError("Item entry (FakeEntry: %u, player: %s, slot: %u, presetId: %u) has invalid slot, ignoring.", entry, std::to_string(pGUID).c_str(), slot, uint32(PresetID));
					continue;
				}
				if (sObjectMgr.GetItemPrototype(entry))
					presetById[pGUID][PresetID][slot] = entry; // Transmogrification::Preset(presetName, fakeEntry);
															   //else
															   //sLog->outError(LOG_FILTER_SQL, "Item entry (FakeEntry: %u, playerGUID: %u, slot: %u, presetId: %u) does not exist, ignoring.", entry, GUID_LOPART(pGUID), uint32(slot), uint32(PresetID));
			}

			if (!presetById[pGUID][PresetID].empty())
			{
				presetByName[pGUID][PresetID] = SetName;
				// load all presets anyways
				//if (presetByName[pGUID].size() >= sTransmogConfig.maxPresets)
				//    break;
			}
			else // should be deleted on startup, so  this never runs (shouldnt..)
			{
				presetById[pGUID].erase(PresetID);
				CharacterDatabase.PExecute("DELETE FROM `custom_transmogrification_sets` WHERE Owner = %u AND PresetID = %u", pGUID.GetCounter(), PresetID);
			}
		} while (result->NextRow());
	}
}

bool TransmogMgr::GetEnableSets() const
{
	return EnableSets;
}
uint8 TransmogMgr::sTransmogConfig.maxPresets const
{
	return MaxSets;
}
float TransmogMgr::GetSetCostModifier() const
{
	return SetCostModifier;
}
int32 TransmogMgr::GetSetCopperCost() const
{
	return SetCopperCost;
}

void TransmogMgr::UnloadPlayerSets(ObjectGuid pGUID)
{
	for (presetData::iterator it = presetById[pGUID].begin(); it != presetById[pGUID].end(); ++it)
		it->second.clear();
	presetById[pGUID].clear();

	presetByName[pGUID].clear();
}
#endif

const char* TransmogMgr::GetSlotName(uint8 slot, WorldSession*) const
{
	switch (slot)
	{
	case EQUIPMENT_SLOT_HEAD: return  "Head";// session->GetAcoreString(LANG_SLOT_NAME_HEAD);
	case EQUIPMENT_SLOT_SHOULDERS: return  "Shoulders";// session->GetAcoreString(LANG_SLOT_NAME_SHOULDERS);
	case EQUIPMENT_SLOT_BODY: return  "Shirt";// session->GetAcoreString(LANG_SLOT_NAME_BODY);
	case EQUIPMENT_SLOT_CHEST: return  "Chest";// session->GetAcoreString(LANG_SLOT_NAME_CHEST);
	case EQUIPMENT_SLOT_WAIST: return  "Waist";// session->GetAcoreString(LANG_SLOT_NAME_WAIST);
	case EQUIPMENT_SLOT_LEGS: return  "Legs";// session->GetAcoreString(LANG_SLOT_NAME_LEGS);
	case EQUIPMENT_SLOT_FEET: return  "Feet";// session->GetAcoreString(LANG_SLOT_NAME_FEET);
	case EQUIPMENT_SLOT_WRISTS: return  "Wrists";// session->GetAcoreString(LANG_SLOT_NAME_WRISTS);
	case EQUIPMENT_SLOT_HANDS: return  "Hands";// session->GetAcoreString(LANG_SLOT_NAME_HANDS);
	case EQUIPMENT_SLOT_BACK: return  "Back";// session->GetAcoreString(LANG_SLOT_NAME_BACK);
	case EQUIPMENT_SLOT_MAINHAND: return  "Main hand";// session->GetAcoreString(LANG_SLOT_NAME_MAINHAND);
	case EQUIPMENT_SLOT_OFFHAND: return  "Off hand";// session->GetAcoreString(LANG_SLOT_NAME_OFFHAND);
	case EQUIPMENT_SLOT_RANGED: return  "Ranged";// session->GetAcoreString(LANG_SLOT_NAME_RANGED);
	case EQUIPMENT_SLOT_TABARD: return  "Tabard";// session->GetAcoreString(LANG_SLOT_NAME_TABARD);
	default: return nullptr;
	}
}

std::string TransmogMgr::GetItemIcon(uint32 entry, uint32 width, uint32 height, int x, int y) const
{
	std::ostringstream ss;
	ss << "|TInterface";
	const ItemPrototype* temp = sObjectMgr.GetItemPrototype(entry);
	const ItemDisplayInfoEntry* dispInfo = nullptr;
	if (temp)
	{
		GameObjectDisplayInfoEntry const* info = sGameObjectDisplayInfoStore.LookupEntry(temp->DisplayInfoID);
		dispInfo = sItemStorage.LookupEntry<ItemDisplayInfoEntry>(temp->DisplayInfoID);
		if (dispInfo)
			//ss << "/ICONS/" << dispInfo->inventoryIcon;
			ss << "/ICONS/" << dispInfo->ID;
	}
	if (!dispInfo)
		ss << "/InventoryItems/WoWUnknownItem01";
	ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
	return ss.str();
}

std::string TransmogMgr::GetSlotIcon(uint8 slot, uint32 width, uint32 height, int x, int y) const
{
	std::ostringstream ss;
	ss << "|TInterface/PaperDoll/";
	switch (slot)
	{
	case EQUIPMENT_SLOT_HEAD: ss << "UI-PaperDoll-Slot-Head"; break;
	case EQUIPMENT_SLOT_SHOULDERS: ss << "UI-PaperDoll-Slot-Shoulder"; break;
	case EQUIPMENT_SLOT_BODY: ss << "UI-PaperDoll-Slot-Shirt"; break;
	case EQUIPMENT_SLOT_CHEST: ss << "UI-PaperDoll-Slot-Chest"; break;
	case EQUIPMENT_SLOT_WAIST: ss << "UI-PaperDoll-Slot-Waist"; break;
	case EQUIPMENT_SLOT_LEGS: ss << "UI-PaperDoll-Slot-Legs"; break;
	case EQUIPMENT_SLOT_FEET: ss << "UI-PaperDoll-Slot-Feet"; break;
	case EQUIPMENT_SLOT_WRISTS: ss << "UI-PaperDoll-Slot-Wrists"; break;
	case EQUIPMENT_SLOT_HANDS: ss << "UI-PaperDoll-Slot-Hands"; break;
	case EQUIPMENT_SLOT_BACK: ss << "UI-PaperDoll-Slot-Chest"; break;
	case EQUIPMENT_SLOT_MAINHAND: ss << "UI-PaperDoll-Slot-MainHand"; break;
	case EQUIPMENT_SLOT_OFFHAND: ss << "UI-PaperDoll-Slot-SecondaryHand"; break;
	case EQUIPMENT_SLOT_RANGED: ss << "UI-PaperDoll-Slot-Ranged"; break;
	case EQUIPMENT_SLOT_TABARD: ss << "UI-PaperDoll-Slot-Tabard"; break;
	default: ss << "UI-Backpack-EmptySlot";
	}
	ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
	return ss.str();
}

std::string TransmogMgr::GetItemLink(Item* item, WorldSession* session) const
{
	int loc_idx = session->GetSessionDbLocaleIndex();
	const ItemPrototype* temp = item->GetProto();
	std::string name = temp->Name1;
	//if (ItemLocale const* il = sObjectMgr.GetItemLocale(temp->ItemId))
		//ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

	/*if (int32 itemRandPropId = item->GetItemRandomPropertyId())
	{
		std::array<char const*, 16> const* suffix = nullptr;
		if (itemRandPropId < 0)
		{
			if (const ItemRandomPropertiesEntry* itemRandEntry = sItemRandomPropertiesStore.LookupEntry(-itemRandPropId))
				suffix = &itemRandEntry->Name;
		}
		else
		{
			if (const ItemRandomPropertiesEntry* itemRandEntry = sItemRandomPropertiesStore.LookupEntry(itemRandPropId))
				suffix = &itemRandEntry->Name;
		}
		if (suffix)
		{
			std::string_view test((*suffix)[(name != temp->Name1) ? loc_idx : DEFAULT_LOCALE]);
			if (!test.empty())
			{
				name += ' ';
				name += test;
			}
		}
	}

	std::ostringstream oss;
	oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
		"|Hitem:" << temp->ItemId << ":" <<
		item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_0) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_1) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_2) << ":" <<
		item->GetEnchantmentId(PROP_ENCHANTMENT_SLOT_3) << ":" <<
		item->GetItemRandomPropertyId() << ":" << item->GetItemSuffixFactor() << ":" <<
		item->GetOwner()->GetLevel() << "|h[" << name << "]|h|r";

	return oss.str();
}

std::string TransmogMgr::GetItemLink(uint32 entry, WorldSession* session) const
{
	const ItemPrototype* temp = sObjectMgr.GetItemPrototype(entry);
	int loc_idx = session->GetSessionDbLocaleIndex();
	std::string name = temp->Name1;
	std::ostringstream oss;
	oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
		"|Hitem:" << entry << ":0:0:0:0:0:0:0:0:0|h[" << name << "]|h|r";

	return oss.str();
}

uint32 TransmogMgr::GetFakeEntry(const ObjectGuid itemGUID) const
{
	const auto itr = dataMap.find(itemGUID);
	if (itr == dataMap.end()) return 0;
	const auto itr2 = entryMap.find(itr->second);
	if (itr2 == entryMap.end()) return 0;
	const auto itr3 = itr2->second.find(itemGUID);
	if (itr3 == itr2->second.end()) return 0;
	return itr3->second;
}

void TransmogMgr::UpdateTransmogItem(Player* player, Item* item) const
{
	if (item->IsEquipped())
		player->SetVisibleItemSlot(item->GetSlot(), item);
}

void TransmogMgr::DeleteFakeEntry(Player* player, uint8, Item* itemTransmogrified)
{
	//if (!GetFakeEntry(item))
	//    return false;
	DeleteFakeEntryFromDB(itemTransmogrified->GetObjectGuid());
	UpdateTransmogItem(player, itemTransmogrified);
}

void TransmogMgr::SetFakeEntry(Player* player, uint32 newEntry, Item* itemTransmogrified)
{
	const ObjectGuid itemGUID = itemTransmogrified->GetObjectGuid();
	entryMap[player->GetObjectGuid()][itemGUID] = newEntry;
	dataMap[itemGUID] = player->GetObjectGuid();
	CharacterDatabase.PExecute("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", itemGUID.GetCounter(), newEntry, player->GetObjectGuid());
	UpdateTransmogItem(player, itemTransmogrified);
}

TransmogAcoreStrings TransmogMgr::TransmogItem(Player* player, ObjectGuid TransmogrifierGUID, uint8 slot, bool no_cost)
{
	if (slot >= EQUIPMENT_SLOT_END)
	{
		sLog.outError("TransmogItem wrong slot: %u", slot);
		return LANG_ERR_TRANSMOG_INVALID_SLOT;
	}

	Item* itemTransmogrifier = nullptr;
	// guid of the transmogrifier item, if it's not 0
	if (TransmogrifierGUID)
	{
		itemTransmogrifier = player->GetItemByGuid(TransmogrifierGUID);
		if (!itemTransmogrifier)
		{
			return LANG_ERR_TRANSMOG_MISSING_SRC_ITEM;
		}
	}

	// transmogrified item
	Item* itemTransmogrified = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
	if (!itemTransmogrified)
	{
		return LANG_ERR_TRANSMOG_MISSING_DEST_ITEM;
	}

	if (!itemTransmogrifier) // reset look newEntry
	{
		// Custom
		DeleteFakeEntry(player, slot, itemTransmogrified);
	}
	else
	{
		if (!CanTransmogItemItemWithItem(player, itemTransmogrified->GetProto(), itemTransmogrifier->GetProto()))
			return LANG_ERR_TRANSMOG_INVALID_ITEMS;

		if (sTransmogMgr->GetFakeEntry(itemTransmogrified->GetObjectGuid()) == itemTransmogrifier->GetEntry())
			return LANG_ERR_TRANSMOG_SAME_DISPLAYID;

		if (!no_cost)
		{
			if (RequireToken)
			{
				if (player->HasItemCount(TokenEntry, TokenAmount))
					player->DestroyItemCount(TokenEntry, TokenAmount, true);
				else
					return LANG_ERR_TRANSMOG_NOT_ENOUGH_TOKENS;
			}

			uint32 cost = GetSellPrice(itemTransmogrified->GetProto());
			cost *= ScaledCostModifier;
			cost += CopperCost;

			if (cost) // 0 cost if reverting look
			{
				if (player->GetMoney() < cost)
					return LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY;
				player->ModifyMoney(-(int)cost);
			}
		}

		// Custom
		SetFakeEntry(player, itemTransmogrifier->GetEntry(), itemTransmogrified); // newEntry
		itemTransmogrified->SetOwnerGuid(player->GetObjectGuid());

		if (itemTransmogrifier->GetProto()->Bonding == BIND_WHEN_EQUIPPED || itemTransmogrifier->GetProto()->Bonding == BIND_WHEN_USE)
			itemTransmogrifier->SetBinding(true);

		itemTransmogrifier->SetOwnerGuid(player->GetObjectGuid());
	}

	return LANG_ERR_TRANSMOG_OK;
}

bool TransmogMgr::CanTransmogItemItemWithItem(Player* player, ItemPrototype const* target, ItemPrototype const* source) const
{
	if (!target || !source)
		return false;

	if (source->ItemId == target->ItemId)
		return false;

	if (source->DisplayInfoID == target->DisplayInfoID)
		return false;

	if (source->Class != target->Class)
		return false;

	if (source->InventoryType == INVTYPE_BAG ||
		source->InventoryType == INVTYPE_RELIC ||
		source->InventoryType == INVTYPE_FINGER ||
		source->InventoryType == INVTYPE_TRINKET ||
		source->InventoryType == INVTYPE_AMMO ||
		source->InventoryType == INVTYPE_QUIVER)
		return false;

	if (target->InventoryType == INVTYPE_BAG ||
		target->InventoryType == INVTYPE_RELIC ||
		target->InventoryType == INVTYPE_FINGER ||
		target->InventoryType == INVTYPE_TRINKET ||
		target->InventoryType == INVTYPE_AMMO ||
		target->InventoryType == INVTYPE_QUIVER)
		return false;

	if (!SuitableForTransmogrification(player, target) || !SuitableForTransmogrification(player, source))
		return false;

	if (IsRangedWeapon(source->Class, source->SubClass) != IsRangedWeapon(target->Class, target->SubClass))
		return false;

	if (source->InventoryType != target->InventoryType)
	{
		if (source->Class == ITEM_CLASS_WEAPON && !(IsRangedWeapon(target->Class, target->SubClass) ||
			// [AZTH] Yehonal: fixed weapon check
			(target->InventoryType == INVTYPE_WEAPON || target->InventoryType == INVTYPE_2HWEAPON || target->InventoryType == INVTYPE_WEAPONMAINHAND || target->InventoryType == INVTYPE_WEAPONOFFHAND)
			&& (source->InventoryType == INVTYPE_WEAPON || source->InventoryType == INVTYPE_2HWEAPON || source->InventoryType == INVTYPE_WEAPONMAINHAND || source->InventoryType == INVTYPE_WEAPONOFFHAND)
			))
			return false;
		if (source->Class == ITEM_CLASS_ARMOR &&
			!((source->InventoryType == INVTYPE_CHEST || source->InventoryType == INVTYPE_ROBE) &&
				(target->InventoryType == INVTYPE_CHEST || target->InventoryType == INVTYPE_ROBE)))
			return false;
	}

	return true;
}

bool TransmogMgr::SuitableForTransmogrification(Player* player, ItemPrototype const* proto)
{
	if (!player || !proto)
		return false;

	if (proto->Class != ITEM_CLASS_ARMOR &&
		proto->Class != ITEM_CLASS_WEAPON)
		return false;

	// Don't allow fishing poles
	if (proto->Class == ITEM_CLASS_WEAPON && proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
		return false;

	// Check skill requirements
	if (proto->RequiredSkill != 0)
	{
		if (player->GetSkillValue(static_cast<uint16>(proto->RequiredSkill)) == 0)
			return false;

		if (player->GetSkillValue(static_cast<uint16>(proto->RequiredSkill)) < proto->RequiredSkillRank)
			return false;
	}

	// Check spell requirements
	if (proto->RequiredSpell != 0 && !player->HasSpell(proto->RequiredSpell))
		return false;

	// Check level requirements
	if (player->GetLevel() < proto->RequiredLevel)
		return false;

	return true;
}

uint32 TransmogMgr::GetSellPrice(ItemPrototype const* proto) const
{
	uint32 cost = proto->SellPrice ? proto->SellPrice : 100;
	return cost;
}

std::string TransmogMgr::FormatPrice(uint32 copper) const
{
	std::ostringstream out;
	if (!copper)
	{
		out << "0";
		return out.str();
	}

	uint32 gold = uint32(copper / 10000);
	copper -= (gold * 10000);
	uint32 silver = uint32(copper / 100);
	copper -= (silver * 100);

	bool space = false;
	if (gold > 0)
	{
		out << gold << "g";
		space = true;
	}

	if (silver > 0 && gold < 50)
	{
		if (space) out << " ";
		out << silver << "s";
		space = true;
	}

	if (copper > 0 && gold < 10)
	{
		if (space) out << " ";
		out << copper << "c";
	}

	return out.str();
}

bool TransmogMgr::IsRangedWeapon(uint32 Class, uint32 SubClass) const
{
	return Class == ITEM_CLASS_WEAPON && (
		SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
		SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
		SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW);
}

/*
bool TransmogMgr::IsAllowed(uint32 entry) const
{
	return Allowed.find(entry) != Allowed.end();
}

bool TransmogMgr::IsNotAllowed(uint32 entry) const
{
	return NotAllowed.find(entry) != NotAllowed.end();
}

bool TransmogMgr::IsAllowedQuality(uint32 quality) const
{
	switch (quality)
	{
	case ITEM_QUALITY_POOR: return AllowPoor;
	case ITEM_QUALITY_NORMAL: return AllowCommon;
	case ITEM_QUALITY_UNCOMMON: return AllowUncommon;
	case ITEM_QUALITY_RARE: return AllowRare;
	case ITEM_QUALITY_EPIC: return AllowEpic;
	case ITEM_QUALITY_LEGENDARY: return AllowLegendary;
	case ITEM_QUALITY_ARTIFACT: return AllowArtifact;
	default: return false;
	}
}

void TransmogMgr::LoadConfig(bool reload)
{
#ifdef PRESETS
	EnableSetInfo = true;
	SetNpcText = 601084;

	EnableSets = true;
	MaxSets = 10;
	SetCostModifier = 3.0f;
	SetCopperCost = 0;

	if (MaxSets > MAX_OPTIONS)
		MaxSets = MAX_OPTIONS;

	if (reload) // dont store presets for nothing
	{
		sWorld.ExecuteForAllSessions([this](WorldSession& worldSession)
			{
				if (Player* player = worldSession.GetPlayer())
				{
					// skipping session check
					UnloadPlayerSets(player->GetObjectGuid());
					if (GetEnableSets())
						LoadPlayerSets(player->GetObjectGuid());
				}
			});
	}
#endif

	EnableTransmogInfo = true;
	TransmogNpcText = 601083;
	TransmogNpcSelectLookText = 601085;
	TransmogNpcConfirmText = 601086;
	TransmogNpcAlreadyText = 601087;
	TransmogNpcAlreadyAltText = 601088;

	std::istringstream issAllowed("");
	std::istringstream issNotAllowed("");
	while (issAllowed.good())
	{
		uint32 entry;
		issAllowed >> entry;
		if (issAllowed.fail())
			break;
		Allowed.insert(entry);
	}
	while (issNotAllowed.good())
	{
		uint32 entry;
		issNotAllowed >> entry;
		if (issNotAllowed.fail())
			break;
		NotAllowed.insert(entry);
	}

	ScaledCostModifier = 1.0f;
	CopperCost = 0;

	RequireToken = false;
	TokenEntry = 49426;
	TokenAmount = 1;

	AllowPoor = true;
	AllowCommon = true;
	AllowUncommon = true;
	AllowRare = true;
	AllowEpic = true;
	AllowLegendary = true;
	AllowArtifact = true;

	AllowMixedArmorTypes = false;
	AllowMixedWeaponTypes = false;
	AllowFishingPoles = false;

	IgnoreReqRace = false;
	IgnoreReqClass = false;
	IgnoreReqSkill = false;
	IgnoreReqSpell = false;
	IgnoreReqLevel = false;
	IgnoreReqEvent = false;
	IgnoreReqStats = false;

	if (!sObjectMgr.GetItemPrototype(TokenEntry))
	{
		//sLog->outError(LOG_FILTER_SERVER_LOADING, "Transmogrification.TokenEntry (%u) does not exist. Using default.", TokenEntry);
		TokenEntry = 49426;
	}
}

void TransmogMgr::DeleteFakeEntryFromDB(const ObjectGuid itemLowGuid)
{
	const ObjectGuid itemGUID = itemLowGuid;

	if (dataMap.find(itemGUID) != dataMap.end())
	{
		if (entryMap.find(dataMap[itemGUID]) != entryMap.end())
			entryMap[dataMap[itemGUID]].erase(itemGUID);
		dataMap.erase(itemGUID);
	}

	CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemLowGuid.GetCounter());
}

void TransmogMgr::CleanUp(Player* player)
{
	auto result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", player->GetObjectGuid());
	if (result)
	{
		do
		{
			const auto itemGUID = ObjectGuid(HIGHGUID_ITEM, (*result)[0].GetUInt32());
			if (!player->GetItemByGuid(itemGUID))
				CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemGUID.GetCounter());
		} while (result->NextRow());
	}
}

void TransmogMgr::BuildTransmogMap(Player* player)
{
	const ObjectGuid playerGUID = player->GetObjectGuid();
	entryMap.erase(playerGUID);
	auto result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", player->GetObjectGuid());
	if (result)
	{
		do
		{
			ObjectGuid itemGUID = ObjectGuid(HIGHGUID_ITEM, (*result)[0].GetUInt32());
			const uint32 fakeEntry = (*result)[1].GetUInt32();
			if (sObjectMgr.GetItemPrototype(fakeEntry))
			{
				dataMap[itemGUID] = playerGUID;
				entryMap[playerGUID][itemGUID] = fakeEntry;
			}
		} while (result->NextRow());
	}
}

bool TransmogMgr::Refresh(Player* player, Item* pEquippedItem)
{
	//if (!player || !pEquippedItem)
	//	return false;

	//if (player->AI())
	//	return false;
	//
	bool ok = false;
	//const uint8 slot = pEquippedItem->GetSlot();
	//// We need the equipped item's GUID to compared with the ones saved in the DB for each player
	//const ObjectGuid EquippedItemGUID = pEquippedItem->GetObjectGuid();

	//// If we find an equipped item that has a Transmogrifier then we need to reapply it
	//if (const uint32 Transmogrifier = GetFakeEntry(EquippedItemGUID))
	//{
	//	// We need to search for the Transmogrifier in all the bags and bank
	//	if (Item* pItemTransmogrifier = player->GetItemByEntry(Transmogrifier))
	//	{
	//		const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//		if (player->GetUInt32Value(VisibleBase + 0) != pItemTransmogrifier->GetEntry())
	//		{
	//			player->SetVisibleItemSlot(slot, pItemTransmogrifier);
	//			ok = true;
	//		}
	//	}
	//	else
	//	{
	//		// If we couldn't find the Transmogrifier then we need to delete it from the DB
	//		DeleteFakeEntry(player, slot, pEquippedItem);
	//		const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//		if (player->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			player->SetVisibleItemSlot(slot, pEquippedItem);
	//	}
	//}

	return ok;
}

bool TransmogMgr::RevertAll(Player* player)
{
	if (!player)
		return false;

	bool ok = false;
	//if (!player->RevertTransmogSlots.empty())
	//{
	//	for (const auto slot : player->RevertTransmogSlots)
	//	{
	//		// If we find an equipped item then let's check if it's transmogrified
	//		if (Item* pEquippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//		{
	//			const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//			if (player->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			{
	//				player->SetVisibleItemSlot(slot, pEquippedItem);
	//				ok = true;
	//			}
	//		}
	//	}
	//	player->RevertTransmogSlots.clear();
	//}
	//else
	//{
	//	// Check all the equipped items to see if they have a Transmogrifier
	//	for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	//	{
	//		// If we find an equipped item then let's check if it's transmogrified
	//		if (Item* pEquippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//		{
	//			const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//			if (player->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			{
	//				player->SetVisibleItemSlot(slot, pEquippedItem);
	//				ok = true;
	//			}
	//		}
	//	}
	//}

	return ok;
}

bool TransmogMgr::ApplyAll(Player* player)
{
	//if (!player)
	//	return false;

	bool ok = false;
	//// Check all the equipped items to see if they have a Transmogrifier
	//for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	//{
	//	// If we find an equipped item then let's check if it's transmogrified
	//	if (Item* pEquippedItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//	{
	//		// We need the equipped item's GUID to compare with the ones saved in the DB for each player
	//		const ObjectGuid EquippedItemGUID = pEquippedItem->GetObjectGuid();

	//		// If we find an equipped item that has a Transmogrifier then we need to reapply it
	//		if (const uint32 Transmogrifier = GetFakeEntry(EquippedItemGUID))
	//		{
	//			// We need to search for the Transmogrifier in all the bags and bank
	//			if (Item* pItemTransmogrifier = player->GetItemByEntry(Transmogrifier))
	//			{
	//				const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//				if (player->GetUInt32Value(VisibleBase + 0) != pItemTransmogrifier->GetEntry())
	//				{
	//					player->SetVisibleItemSlot(slot, pItemTransmogrifier);
	//					ok = true;
	//				}
	//			}
	//			else
	//			{
	//				// If we couldn't find the Transmogrifier then we need to delete it from the DB
	//				DeleteFakeEntry(player, slot, pEquippedItem);
	//				const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//				if (player->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//					player->SetVisibleItemSlot(slot, pEquippedItem);
	//			}
	//		}
	//	}
	//}

	return ok;
}

void TransmogMgr::OnLogout(Player* player)
{
	if (!player)
		return;

	const ObjectGuid pGUID = player->GetObjectGuid();
	for (transmog2Data::const_iterator it = entryMap[pGUID].begin(); it != entryMap[pGUID].end(); ++it)
		dataMap.erase(it->first);
	entryMap.erase(pGUID);
}

bool TransmogMgr::GetEnableTransmogInfo() const
{
	return EnableTransmogInfo;
}
uint32 TransmogMgr::GetTransmogNpcText() const
{
	return TransmogNpcText;
}
uint32 TransmogMgr::GetNpcSelectLookText() const
{
	return TransmogNpcSelectLookText;
}
uint32 TransmogMgr::GetSetNpcConfirmText() const
{
	return TransmogNpcConfirmText;
}
uint32 TransmogMgr::GetSetNpcAlreadyText() const
{
	return TransmogNpcAlreadyText;
}
uint32 TransmogMgr::GetSetNpcAlreadyAltText() const
{
	return TransmogNpcAlreadyAltText;
}
bool TransmogMgr::GetEnableSetInfo() const
{
	return EnableSetInfo;
}
uint32 TransmogMgr::GetSetNpcText() const
{
	return SetNpcText;
}
float TransmogMgr::GetScaledCostModifier() const
{
	return ScaledCostModifier;
}
int32 TransmogMgr::GetCopperCost() const
{
	return CopperCost;
}
bool TransmogMgr::GetRequireToken() const
{
	return RequireToken;
}
uint32 TransmogMgr::GetTokenEntry() const
{
	return TokenEntry;
}
uint32 TransmogMgr::GetTokenAmount() const
{
	return TokenAmount;
}
bool TransmogMgr::GetAllowMixedArmorTypes() const
{
	return AllowMixedArmorTypes;
};
bool TransmogMgr::GetAllowMixedWeaponTypes() const
{
	return AllowMixedWeaponTypes;
};
*/