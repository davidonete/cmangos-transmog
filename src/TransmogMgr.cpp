#include "TransmogMgr.h"
#include "TransmogConfig.h"

void TransmogMgr::Init()
{
	sTransmogConfig.Initialize();
}

/*
#ifdef PRESETS

void TransmogMgr::PresetTransmog(Player* player, Item* itemTransmogrified, uint32 fakeEntry, uint8 slot)
{
	if (!EnableSets)
		return;
	if (!player || !itemTransmogrified)
		return;
	if (slot >= EQUIPMENT_SLOT_END)
		return;
	if (!CanTransmogrifyItemWithItem(player, itemTransmogrified->GetProto(), sObjectMgr.GetItemPrototype(fakeEntry)))
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
				//if (presetByName[pGUID].size() >= GetMaxSets())
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
uint8 TransmogMgr::GetMaxSets() const
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

void TransmogMgr::UpdateItem(Player* player, Item* item) const
{
	if (item->IsEquipped())
		player->SetVisibleItemSlot(item->GetSlot(), item);
}

void TransmogMgr::DeleteFakeEntry(Player* player, uint8, Item* itemTransmogrified)
{
	//if (!GetFakeEntry(item))
	//    return false;
	DeleteFakeFromDB(itemTransmogrified->GetObjectGuid());
	UpdateItem(player, itemTransmogrified);
}

void TransmogMgr::SetFakeEntry(Player* player, uint32 newEntry, Item* itemTransmogrified)
{
	const ObjectGuid itemGUID = itemTransmogrified->GetObjectGuid();
	entryMap[player->GetObjectGuid()][itemGUID] = newEntry;
	dataMap[itemGUID] = player->GetObjectGuid();
	CharacterDatabase.PExecute("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", itemGUID.GetCounter(), newEntry, player->GetObjectGuid());
	UpdateItem(player, itemTransmogrified);
}

TransmogAcoreStrings TransmogMgr::Transmogrify(Player* player, ObjectGuid TransmogrifierGUID, uint8 slot, bool no_cost)
{
	if (slot >= EQUIPMENT_SLOT_END)
	{
		sLog.outError("Transmogrify wrong slot: %u", slot);
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
		if (!CanTransmogrifyItemWithItem(player, itemTransmogrified->GetProto(), itemTransmogrifier->GetProto()))
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

			uint32 cost = GetSpecialPrice(itemTransmogrified->GetProto());
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

bool TransmogMgr::CanTransmogrifyItemWithItem(Player* player, ItemPrototype const* target, ItemPrototype const* source) const
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

uint32 TransmogMgr::GetSpecialPrice(ItemPrototype const* proto) const
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

void TransmogMgr::DeleteFakeFromDB(const ObjectGuid itemLowGuid)
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

void TransmogMgr::CleanUp(Player* pPlayer)
{
	auto result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", pPlayer->GetObjectGuid());
	if (result)
	{
		do
		{
			const auto itemGUID = ObjectGuid(HIGHGUID_ITEM, (*result)[0].GetUInt32());
			if (!pPlayer->GetItemByGuid(itemGUID))
				CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE GUID = %u", itemGUID.GetCounter());
		} while (result->NextRow());
	}
}

void TransmogMgr::BuildTransmogMap(Player* pPlayer)
{
	const ObjectGuid playerGUID = pPlayer->GetObjectGuid();
	entryMap.erase(playerGUID);
	auto result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", pPlayer->GetObjectGuid());
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

bool TransmogMgr::Refresh(Player* pPlayer, Item* pEquippedItem)
{
	//if (!pPlayer || !pEquippedItem)
	//	return false;

	//if (pPlayer->AI())
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
	//	if (Item* pItemTransmogrifier = pPlayer->GetItemByEntry(Transmogrifier))
	//	{
	//		const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//		if (pPlayer->GetUInt32Value(VisibleBase + 0) != pItemTransmogrifier->GetEntry())
	//		{
	//			pPlayer->SetVisibleItemSlot(slot, pItemTransmogrifier);
	//			ok = true;
	//		}
	//	}
	//	else
	//	{
	//		// If we couldn't find the Transmogrifier then we need to delete it from the DB
	//		DeleteFakeEntry(pPlayer, slot, pEquippedItem);
	//		const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//		if (pPlayer->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			pPlayer->SetVisibleItemSlot(slot, pEquippedItem);
	//	}
	//}

	return ok;
}

bool TransmogMgr::RevertAll(Player* pPlayer)
{
	if (!pPlayer)
		return false;

	bool ok = false;
	//if (!pPlayer->RevertTransmogSlots.empty())
	//{
	//	for (const auto slot : pPlayer->RevertTransmogSlots)
	//	{
	//		// If we find an equipped item then let's check if it's transmogrified
	//		if (Item* pEquippedItem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//		{
	//			const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//			if (pPlayer->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			{
	//				pPlayer->SetVisibleItemSlot(slot, pEquippedItem);
	//				ok = true;
	//			}
	//		}
	//	}
	//	pPlayer->RevertTransmogSlots.clear();
	//}
	//else
	//{
	//	// Check all the equipped items to see if they have a Transmogrifier
	//	for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	//	{
	//		// If we find an equipped item then let's check if it's transmogrified
	//		if (Item* pEquippedItem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//		{
	//			const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//			if (pPlayer->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//			{
	//				pPlayer->SetVisibleItemSlot(slot, pEquippedItem);
	//				ok = true;
	//			}
	//		}
	//	}
	//}

	return ok;
}

bool TransmogMgr::ApplyAll(Player* pPlayer)
{
	//if (!pPlayer)
	//	return false;

	bool ok = false;
	//// Check all the equipped items to see if they have a Transmogrifier
	//for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	//{
	//	// If we find an equipped item then let's check if it's transmogrified
	//	if (Item* pEquippedItem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
	//	{
	//		// We need the equipped item's GUID to compare with the ones saved in the DB for each player
	//		const ObjectGuid EquippedItemGUID = pEquippedItem->GetObjectGuid();

	//		// If we find an equipped item that has a Transmogrifier then we need to reapply it
	//		if (const uint32 Transmogrifier = GetFakeEntry(EquippedItemGUID))
	//		{
	//			// We need to search for the Transmogrifier in all the bags and bank
	//			if (Item* pItemTransmogrifier = pPlayer->GetItemByEntry(Transmogrifier))
	//			{
	//				const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//				if (pPlayer->GetUInt32Value(VisibleBase + 0) != pItemTransmogrifier->GetEntry())
	//				{
	//					pPlayer->SetVisibleItemSlot(slot, pItemTransmogrifier);
	//					ok = true;
	//				}
	//			}
	//			else
	//			{
	//				// If we couldn't find the Transmogrifier then we need to delete it from the DB
	//				DeleteFakeEntry(pPlayer, slot, pEquippedItem);
	//				const uint16 VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + slot * MAX_VISIBLE_ITEM_OFFSET;
	//				if (pPlayer->GetUInt32Value(VisibleBase + 0) != pEquippedItem->GetEntry())
	//					pPlayer->SetVisibleItemSlot(slot, pEquippedItem);
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