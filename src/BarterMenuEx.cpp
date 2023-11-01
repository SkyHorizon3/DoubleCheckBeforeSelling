#include "../include/BarterMenuEx.h"
#include "../include/MessageBox.h"

namespace CheckBeforeSelling
{
	void BarterMenuEx::Hook_PostCreate()
	{
		func(this);

		RE::GFxValue oldFunc, funct;
		bool success = this->GetRuntimeData().root.GetMember("onItemSelect", &oldFunc);
		if (!success)
		{
			g_Logger->critical("success is {}. Plugin won't work.", success);
		}

		auto impl_1 = RE::make_gptr<ItemSelectHandler>(oldFunc);
		this->uiMovie->CreateFunction(&funct, impl_1.get());
		this->GetRuntimeData().root.SetMember("onItemSelect", funct);  //Replace Vanilla Method

		auto impl_2 = RE::make_gptr<SellingConfirmHandler>(oldFunc);
		this->uiMovie->CreateFunction(&funct, impl_2.get());
		this->GetRuntimeData().root.SetMember("Maxsu_OnSellingConfirm", funct);

	}

	void BarterMenuEx::SellingConfirmHandler::Call(RE::GFxFunctionHandler::Params& a_params)
	{
		oldFunc_.Invoke("call", a_params.retVal, a_params.argsWithThisRef, a_params.argCount + 1);
	}

	void BarterMenuEx::ItemSelectHandler::Call(RE::GFxFunctionHandler::Params& a_params)
	{
		RE::UI* ui = RE::UI::GetSingleton();
		auto menu = ui->GetMenu<RE::BarterMenu>();

		if (!menu) {
			g_Logger->error("BarterMenu Not Found!");
			return;
		}


		if (menu->IsViewingVendorItems())
		{
			oldFunc_.Invoke("call", a_params.retVal, a_params.argsWithThisRef, a_params.argCount + 1);
			return;
		}


		auto selectedItem = menu->GetRuntimeData().itemList->GetSelectedItem();
		auto itemType = GetItemType(selectedItem);
		if (selectedItem && itemType != ItemType::kNone && selectedItem->data.GetEnabled()) {
			RE::GFxValue keyboardOrMouse;
			a_params.args[0].GetMember("keyboardOrMouse", &keyboardOrMouse);
			const double keyboardOrMouseVal = keyboardOrMouse.GetNumber();

			SkyrimScripting::ShowMessageBox(
				GetConfirmMessageText(itemType),
				{ "$Yes", "$No" },
				[menu, keyboardOrMouseVal](unsigned int a_result) {
					if (a_result == 0) {
						RE::GFxValue obj;
						menu->uiMovie->CreateObject(&obj);
						obj.SetMember("entry", menu->GetRuntimeData().itemList->GetSelectedItem()->obj);
						RE::GFxValue keyboardOrMouse;
						keyboardOrMouse.SetNumber(keyboardOrMouseVal);
						obj.SetMember("keyboardOrMouse", keyboardOrMouse);
						menu->GetRuntimeData().root.Invoke("Maxsu_OnSellingConfirm", nullptr, &obj, 1);
					}
					else {
						menu->GetRuntimeData().itemList->Update();
					}
				});
		}
		else {
			oldFunc_.Invoke("call", a_params.retVal, a_params.argsWithThisRef, a_params.argCount + 1);
		}
	}

	const std::string BarterMenuEx::GetConfirmMessageText(ItemType a_type)
	{
		auto GetTranslationKey = [](ItemType a_type) -> const std::string {
			switch (a_type) {
			case ItemType::kEquipped:
				return "$DoubleCheckBeforeSelling_EquippedItem";

			case ItemType::kFavorite:
				return "$DoubleCheckBeforeSelling_FavouritedItem";

			case ItemType::kUnique:
				return "$DoubleCheckBeforeSelling_UniqueItem";

			default:
				return "";
			}
			};

		std::string translationKey = GetTranslationKey(a_type);
		if (translationKey.empty()) {
			return "Error Item Type!";
		}

		std::string result;
		if (SKSE::Translation::Translate(translationKey, result)) {
			return result;
		}
		else {
			return "Invaild Translation String: " + translationKey;
		}
	}

	BarterMenuEx::ItemType BarterMenuEx::GetItemType(RE::ItemList::Item* a_item)
	{
		if (!a_item) {
			return ItemType::kNone;
		}

		if (a_item->data.GetEquipState() > 0 && EnableCheckForEquipped) {
			return ItemType::kEquipped;
		}

		if (a_item->data.GetFavorite() && EnableCheckForFavourited) {
			return ItemType::kFavorite;
		}

		auto HasExtraEnchanmtment = [](RE::InventoryEntryData* a_entry) -> bool {
			if (!a_entry || !a_entry->extraLists) {
				return false;
			}

			for (auto xList : *a_entry->extraLists) {
				if (xList && xList->GetByType<RE::ExtraEnchantment>()) {
					return true;
				}
			}

			return false;
			};

		auto entryObj = a_item->data.objDesc;
		if (EnableCheckForUnique && entryObj && HasExtraEnchanmtment(entryObj)) {
			return ItemType::kUnique;
		}

		return ItemType::kNone;
	}

}