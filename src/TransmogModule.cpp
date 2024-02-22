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
	}
}

void TransmogMgr::OnPlayerLogin(Player* player)
{
    if (sTransmogConfig.enabled)
    {
		if (player)
		{
			const ObjectGuid playerID = player->GetObjectGuid();

			// Load the transmog config for the player
			entryMap.erase(playerID);
            auto result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", playerID);
            if (result)
            {
                do
                {
					Field* fields = result->Fetch();
                    const ObjectGuid itemGUID = ObjectGuid(HIGHGUID_ITEM, (fields[0].GetUInt32()));
                    const uint32 fakeEntry = fields[1].GetUInt32();
                    if (sObjectMgr.GetItemPrototype(fakeEntry))
                    {
                        dataMap[itemGUID] = playerID;
                        entryMap[playerID][itemGUID] = fakeEntry;
                    }
                    else
                    {
                        sLog.outError("Item entry (Entry: %u, player ID: %u) does not exist, ignoring.", fakeEntry, playerID.GetCounter());
                        CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE FakeEntry = %u", fakeEntry);
                    }
                } 
				while (result->NextRow());

				// Reload the item visuals
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
					{
						UpdateTransmogItem(player, item);
					}
                }
			}
		}
	}
}

void TransmogMgr::OnPlayerLogout(Player* player)
{
	if (sTransmogConfig.enabled)
	{
        if (player)
        {
			// Unload transmog config
            const ObjectGuid playerID = player->GetObjectGuid();
            for (auto it = entryMap[playerID].begin(); it != entryMap[playerID].end(); ++it)
            {
                dataMap.erase(it->first);
            }

            entryMap.erase(playerID);
        }
	}
}

void TransmogMgr::OnPlayerCharacterDeletedFromDB(uint32 playerId)
{
    if (sTransmogConfig.enabled)
    {
		CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE Owner = %u", playerId);
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

uint32 TransmogMgr::GetTransmogPrice(const Item* item) const
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

std::string FormatPrice(uint32 copper)
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
                if (Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                {
                    // Only show the menu option if there is a transmogrifying option available
                    bool hasOption = false;

					// Check in main inventory
                    if (!hasOption)
                    {
                        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
                        {
                            if (Item* sourceItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
                                if (!CanApplyTransmog(player, targetItem, sourceItem))
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
                                    if (Item* sourceItem = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
                                        if (!CanApplyTransmog(player, targetItem, sourceItem))
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
                            if (Item* sourceItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
                                if (!CanApplyTransmog(player, targetItem, sourceItem))
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
                                    if (Item* sourceItem = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
                                        if (!CanApplyTransmog(player, targetItem, sourceItem))
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
                        std::string slotName = GetSlotName(slot);
                        if (slotName.length() > 0)
						{
                            player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TABARD, slotName.c_str(), EQUIPMENT_SLOT_END, slot, "", 0);
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

			const uint64 playerID = player->GetObjectGuid();
			WorldSession* playerSession = player->GetSession();

            player->GetPlayerMenu()->ClearMenus();
			switch (sender)
			{
                // Display the available transmogs inside the options
                case EQUIPMENT_SLOT_END:
                {
					std::map<uint32, Item*> possibleTransmogItems;
                    if (Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, (uint8)action))
                    {
                        const std::string itemName = targetItem->GetProto()->Name1;
						const std::string itemOption = playerSession->GetMangosString(LANG_MENU_YOUR_ITEM) + itemName;
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_VENDOR, itemOption.c_str(), sender, action, "", 0);

                        // Check in main inventory
                        for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
                        {
                            if (Item* sourceItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
								const uint32 sourceItemEntry = sourceItem->GetEntry();
                                if (CanApplyTransmog(player, targetItem, sourceItem) && GetFakeEntry(targetItem) != sourceItemEntry)
                                {
                                    if (possibleTransmogItems.find(sourceItemEntry) == possibleTransmogItems.end())
                                    {
                                        possibleTransmogItems[sourceItemEntry] = sourceItem;
                                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, sourceItem->GetProto()->Name1, action + 100, sourceItemEntry, "", 0);
                                    }
                                }
                            }
                        }

                        // Check in the other bags
                        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
                        {
                            if (const auto pBag = dynamic_cast<Bag*>(player->GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
                            {
                                for (uint32 j = 0; j < pBag->GetBagSize(); j++)
                                {
                                    if (Item* sourceItem = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
										const uint32 sourceItemEntry = sourceItem->GetEntry();
                                        if (CanApplyTransmog(player, targetItem, sourceItem) && GetFakeEntry(targetItem) != sourceItemEntry)
                                        {
                                            if (possibleTransmogItems.find(sourceItemEntry) == possibleTransmogItems.end())
                                            {
                                                possibleTransmogItems[sourceItemEntry] = sourceItem;
                                                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, sourceItem->GetProto()->Name1, action + 100, sourceItemEntry, "", 0);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Check in the bank's main inventory
                        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
                        {
                            if (Item* sourceItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                            {
								const uint32 sourceItemEntry = sourceItem->GetEntry();
                                if (CanApplyTransmog(player, targetItem, sourceItem) && GetFakeEntry(targetItem) != sourceItemEntry)
                                {
                                    if (possibleTransmogItems.find(sourceItemEntry) == possibleTransmogItems.end())
                                    {
                                        possibleTransmogItems[sourceItemEntry] = sourceItem;
                                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, sourceItem->GetProto()->Name1, action + 100, sourceItemEntry, "", 0);
                                    }
                                }
                            }
                        }

                        // Check in the bank's other bags
                        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
                        {
                            if (const auto pBag = dynamic_cast<Bag*>(player->GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
                            {
                                for (uint32 j = 0; j < pBag->GetBagSize(); j++)
                                {
                                    if (Item* sourceItem = pBag->GetItemByPos(static_cast<uint8>(j)))
                                    {
										const uint32 sourceItemEntry = sourceItem->GetEntry();
                                        if (CanApplyTransmog(player, targetItem, sourceItem) && GetFakeEntry(targetItem) != sourceItemEntry)
                                        {
                                            if (possibleTransmogItems.find(sourceItemEntry) == possibleTransmogItems.end())
                                            {
                                                possibleTransmogItems[sourceItemEntry] = sourceItem;
                                                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, sourceItem->GetProto()->Name1, action + 100, sourceItemEntry, "", 0);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Remove the transmog on the current item
						bool hasTransmog = false;
                        bool hasTransmogOptions = !possibleTransmogItems.empty();
                        if (const uint32 fakeEntry = GetFakeEntry(targetItem))
                        {
                            hasTransmog = true;
                            if (ItemPrototype const* pItem = sObjectMgr.GetItemPrototype(fakeEntry))
                            {
                                const std::string itemName = pItem->Name1;
                                const std::string illusion = playerSession->GetMangosString(LANG_MENU_REMOVE) + itemName;
                                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE, illusion.c_str(), EQUIPMENT_SLOT_END + 3, action, "", 0);
                            }
                        }
                        else
                        {
                            if (possibleTransmogItems.empty())
                            {
                                playerSession->SendAreaTriggerMessage(playerSession->GetMangosString(LANG_MENU_NO_SUITABLE_ITEMS));
                                OnPlayerGossipHello(player, creature);
                                return true;
                            }
                        }

                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TALK, playerSession->GetMangosString(LANG_MENU_BACK), EQUIPMENT_SLOT_END + 1, 0, "", 0);
                        if (hasTransmog && !hasTransmogOptions)
                        {
                            player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_ALREADY_NPC_TEXT, creature->GetObjectGuid());
                        }
                        else if (hasTransmog && hasTransmogOptions)
                        {
                            player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_ALREADY_ALT_NPC_TEXT, creature->GetObjectGuid());
                        }
                        else
                        {
                            player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_SELECT_LOOK_NPC_TEXT, creature->GetObjectGuid());
                        }
                    }
                    else
                    {
                        OnPlayerGossipHello(player, creature);
                    }

                    break;
                }

				// Go back to main menu
				case EQUIPMENT_SLOT_END + 1:
				{
					OnPlayerGossipHello(player, creature);
					break;
				}

				// Remove all transmogrifications
				case EQUIPMENT_SLOT_END + 2:
				{
					bool removed = false;
					for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
					{
						if (Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
						{
							if (!GetFakeEntry(targetItem))
								continue;

							DeleteFakeEntry(player, slot, targetItem);
							removed = true;
						}
					}

					if (removed)
					{
						playerSession->SendAreaTriggerMessage("%s", playerSession->GetMangosString(LANG_ERR_UNTRANSMOG_OK));
						creature->CastSpell(player, TRANSMOG_NPC_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
						player->CastSpell(player, TRANSMOG_SELF_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
					}
					else
					{
						playerSession->SendNotification(LANG_ERR_UNTRANSMOG_NO_TRANSMOGS);
					}

					OnPlayerGossipHello(player, creature);
					break;
				}

				// Remove transmogrification on a single equipped item
				case EQUIPMENT_SLOT_END + 3:
				{
					bool removed = false;
					if (Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, static_cast<uint8>(action)))
					{
						if (GetFakeEntry(targetItem))
						{
							DeleteFakeEntry(player, (uint8)action, targetItem);
							removed = true;
						}
					}

					const std::string slotName = GetSlotName((uint8)action);
					if (removed)
					{
						playerSession->SendAreaTriggerMessage("%s (%s)", playerSession->GetMangosString(LANG_ERR_UNTRANSMOG_SINGLE_OK), slotName.c_str());
						creature->CastSpell(player, TRANSMOG_NPC_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
						player->CastSpell(player, TRANSMOG_SELF_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
					}
					else
					{
						playerSession->SendAreaTriggerMessage("%s (%s)", playerSession->GetMangosString(LANG_ERR_UNTRANSMOG_SINGLE_NO_TRANSMOGS), slotName.c_str());
					}

					OnPlayerGossipHello(player, creature);
					break;
				}

				// Update all transmogrifications
				case EQUIPMENT_SLOT_END + 4:
				{
					//ApplyAll(player);
					playerSession->SendAreaTriggerMessage("Your appearance was refreshed");
					OnPlayerGossipHello(player, creature);
					break;
				}

				// Info about transmogrification
				case EQUIPMENT_SLOT_END + 9:
				{
					player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, playerSession->GetMangosString(LANG_MENU_BACK), EQUIPMENT_SLOT_END + 1, 0, "", 0);
					player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_INFO_NPC_TEXT, creature->GetObjectGuid());
					break;
				}

				default:
				{
					// sender = target equipment slot, action = source item entry
					Item* sourceItem = player->GetItemByEntry(action);
                    Item* targetItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, (uint8)(sender >= 100 ? sender - 100 : sender));
					if (sourceItem)
					{
						// Check cost
						if (sender >= 100)
						{
							// Display transaction
							if (!targetItem)
							{
								OnPlayerGossipHello(player, creature);
								return true;
							}

							std::string nameItemTransmogrified = targetItem->GetProto()->Name1;
							std::string before = playerSession->GetMangosString(LANG_MENU_BEFORE) + nameItemTransmogrified;
							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, before.c_str(), sender, action, "", 0);

							const uint32 price = GetTransmogPrice(targetItem);
							std::string nameItemTransmogrifier = sourceItem->GetProto()->Name1;
							std::string after = playerSession->GetMangosString(LANG_MENU_AFTER) + nameItemTransmogrifier;
							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, after.c_str(), sender, action, "", 0);

							std::string costStr = playerSession->GetMangosString(LANG_MENU_COST_IS) + FormatPrice(price);
							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, costStr.c_str(), sender, action, "", 0);

							// Only show confirmation button if player has enough money
							if (player->GetMoney() > price)
							{
								player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, playerSession->GetMangosString(LANG_MENU_CONFIRM), sender - 100, action, "", 0);
							}
							else
							{
								player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, playerSession->GetMangosString(LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY), sender, action, "", 0);
							}

							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TALK, playerSession->GetMangosString(LANG_MENU_BACK), EQUIPMENT_SLOT_END, targetItem->GetSlot(), "", 0);
							player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TALK, playerSession->GetMangosString(LANG_MENU_MAIN_MENU), EQUIPMENT_SLOT_END + 1, 0, "", 0);
							player->GetPlayerMenu()->SendGossipMenu(TRANSMOG_CONFIRM_NPC_TEXT, creature->GetObjectGuid());
						}
						else
						{
							// sender = equipment slot, action = item entry
							TransmogLanguage res = ApplyTransmog(player, sourceItem, targetItem);
							if (res == LANG_ERR_TRANSMOG_OK)
							{
								creature->CastSpell(player, TRANSMOG_NPC_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
								player->CastSpell(player, TRANSMOG_SELF_VISUAL_SPELL, TRIGGERED_OLD_TRIGGERED);
								playerSession->SendAreaTriggerMessage("%s (%s)", playerSession->GetMangosString(LANG_ERR_TRANSMOG_OK), GetSlotName((uint8)sender));
							}
							else
							{
								playerSession->SendNotification(res);
							}

							OnPlayerGossipHello(player, creature);
						}
					}

					break;
				}
			}

            return true;
        }
    }

    return false;
}

void TransmogMgr::OnPlayerSetVisibleItemSlot(Player* player, uint8 slot, Item* item)
{
	if (sTransmogConfig.enabled)
	{
		if (player && item)
		{
            if (uint32 entry = GetFakeEntry(item))
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

void TransmogMgr::OnPlayerMoveItemFromInventory(Player* player, Item* item)
{
    if (sTransmogConfig.enabled)
    {
        if (player && item)
        {
			DeleteFakeEntryFromDB(item);
		}
	}
}

bool IsRangedWeapon(uint32 Class, uint32 SubClass)
{
    return Class == ITEM_CLASS_WEAPON && 
		   (SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
            SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
            SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW);
}

bool TransmogMgr::CanApplyTransmog(Player* player, const Item* target, const Item* source) const
{
    if (!target || !source)
        return false;

	const ItemPrototype* targetProto = target->GetProto();
	const ItemPrototype* sourceProto = source->GetProto();

	// Same item
    if (sourceProto->ItemId == targetProto->ItemId)
        return false;

	// Same item model
    if (sourceProto->DisplayInfoID == targetProto->DisplayInfoID)
        return false;

	// Different item type
    if (sourceProto->Class != targetProto->Class)
        return false;

	// Invalid types
    if (sourceProto->InventoryType == INVTYPE_BAG ||
		sourceProto->InventoryType == INVTYPE_RELIC ||
		sourceProto->InventoryType == INVTYPE_FINGER ||
		sourceProto->InventoryType == INVTYPE_TRINKET ||
		sourceProto->InventoryType == INVTYPE_AMMO ||
		sourceProto->InventoryType == INVTYPE_QUIVER)
	{
        return false;
	}

	// Invalid types
    if (targetProto->InventoryType == INVTYPE_BAG ||
		targetProto->InventoryType == INVTYPE_RELIC ||
		targetProto->InventoryType == INVTYPE_FINGER ||
		targetProto->InventoryType == INVTYPE_TRINKET ||
		targetProto->InventoryType == INVTYPE_AMMO ||
		targetProto->InventoryType == INVTYPE_QUIVER)
	{
        return false;
	}

    if (!SuitableForTransmog(player, target) || !SuitableForTransmog(player, source))
        return false;

	// Ranged weapon check
    if (IsRangedWeapon(sourceProto->Class, sourceProto->SubClass) != IsRangedWeapon(targetProto->Class, targetProto->SubClass))
        return false;

    if (sourceProto->InventoryType != targetProto->InventoryType)
    {
        if (sourceProto->Class == ITEM_CLASS_WEAPON &&
			!(IsRangedWeapon(targetProto->Class, targetProto->SubClass)
			  || (targetProto->InventoryType == INVTYPE_WEAPON ||
				  targetProto->InventoryType == INVTYPE_2HWEAPON ||
				  targetProto->InventoryType == INVTYPE_WEAPONMAINHAND ||
				  targetProto->InventoryType == INVTYPE_WEAPONOFFHAND)
			  && (sourceProto->InventoryType == INVTYPE_WEAPON ||
				  sourceProto->InventoryType == INVTYPE_2HWEAPON ||
				  sourceProto->InventoryType == INVTYPE_WEAPONMAINHAND ||
				  sourceProto->InventoryType == INVTYPE_WEAPONOFFHAND)))
		{
            return false;
		}

        if (sourceProto->Class == ITEM_CLASS_ARMOR &&
            !((sourceProto->InventoryType == INVTYPE_CHEST ||
			   sourceProto->InventoryType == INVTYPE_ROBE)
			  &&
               (targetProto->InventoryType == INVTYPE_CHEST ||
				targetProto->InventoryType == INVTYPE_ROBE)))
		{
            return false;
		}
    }

    return true;
}

bool TransmogMgr::SuitableForTransmog(Player* player, const Item* item) const
{
    if (!player || !item)
        return false;

	const ItemPrototype* proto = item->GetProto();

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
        const uint32 price = GetTransmogPrice(oldItem);

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

            if (!CanApplyTransmog(player, oldItem, newItem))
                continue;

            if (GetFakeEntry(oldItem) == newItem->GetEntry())
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

                if (!CanApplyTransmog(player, oldItem, newItem))
                    continue;

                if (GetFakeEntry(oldItem) == newItem->GetEntry())
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

TransmogLanguage TransmogMgr::ApplyTransmog(Player* player, Item* sourceItem, Item* targetItem)
{
    if (!targetItem)
    {
        return LANG_ERR_TRANSMOG_MISSING_DEST_ITEM;
    }

	// reset look newEntry
    if (!sourceItem) 
    {
        // Custom
        DeleteFakeEntry(player, targetItem->GetSlot(), targetItem);
    }
    else
    {
        if (!CanApplyTransmog(player, targetItem, sourceItem))
            return LANG_ERR_TRANSMOG_INVALID_ITEMS;

        if (GetFakeEntry(targetItem) == sourceItem->GetEntry())
            return LANG_ERR_TRANSMOG_SAME_DISPLAYID;

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

        const uint32 price = GetTransmogPrice(targetItem);
        if (price)
        {
            if (player->GetMoney() < price)
			{
                return LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY;
			}

            player->ModifyMoney(-(int)price);
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

uint32 TransmogMgr::GetFakeEntry(Item* item) const
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

void TransmogMgr::SetFakeEntry(Player* player, uint32 newEntry, Item* item)
{
	if (item)
	{
		const ObjectGuid itemGUID = item->GetObjectGuid();
		entryMap[player->GetObjectGuid()][itemGUID] = newEntry;
		dataMap[itemGUID] = player->GetObjectGuid();
		CharacterDatabase.PExecute("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", itemGUID.GetCounter(), newEntry, player->GetObjectGuid());
		UpdateTransmogItem(player, item);
	}
}

void TransmogMgr::DeleteFakeEntry(Player* player, uint8, Item* item)
{
    DeleteFakeEntryFromDB(item);
    UpdateTransmogItem(player, item);
}

void TransmogMgr::DeleteFakeEntryFromDB(Item* item)
{
	if (item)
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

        CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemGUID.GetCounter());
	}
}