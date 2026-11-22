#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

auto LATEST_MOD_VERSION = std::string("xd");

auto enabled = false;

$on_mod(DataSaved) {
	Mod::get()->setSavedValue<bool>("enabled", enabled);
}
$on_mod(Loaded) {
	//load state
	enabled = Mod::get()->getSavedValue<bool>("enabled");
	//intercept requests
	web::WebRequestInterceptEvent().listen(
		[](std::string_view id, web::WebRequest& req) {
			//log::debug("{}(id {}, req {})", __func__, id, req.getUrl());

			auto self = &req;
			std::string givenUrl = req.getUrl().data();

			if (enabled and string::contains(givenUrl.data(), "api.geode-sdk.org/v1/mods")) {

				if (givenUrl == "https://api.geode-sdk.org/v1/mods") {
					self->param("status", "pending");
				}

				givenUrl = string::replace(
					givenUrl.data(), "latest",
					string::replace(LATEST_MOD_VERSION, "v", "")
				);

				if (string::contains(givenUrl.data(), "/logo")) {
					auto spliturl = string::split(givenUrl.data(), "/");
					givenUrl = fmt::format(
						"https://open-geode.ir/mod.logo.php?id={}",
						spliturl[spliturl.size() - 2]
					);
				}

			}

			if (string::contains(givenUrl.data(), "/mods/updates")) givenUrl = string::replace(
				givenUrl.data(), "api.geode-sdk.org/v1/mods/updates",
				"open-geode.ir/mod.updates.php"
			);

			self->url(givenUrl);

			return ListenerResult::Propagate;
		}, Priority::Stub
	).leak();
}

void TOGGLE_MAIN() {
	// there was a lot of removed code here ~
	enabled = !enabled;
}

#include <Geode/modify/FLAlertLayer.hpp>
class ModPopup : public FLAlertLayer {};
class FiltersPopup : public FLAlertLayer {};
class $modify(UnverifiedModsPopupCatch, FLAlertLayer) {
	void setupForModPopup() {
		if (Ref ver = typeinfo_cast<CCLabelBMFont*>(
			this->querySelector("mod-stats-container > version > value-label")
		))  LATEST_MOD_VERSION = ver->getString();
	}
	void setupForFiltersPopup() {
		//reload on apply
		addCleanupCallback(
			[sc = Ref(this->getParent())] {
				if (auto reload_btn = typeinfo_cast<CCMenuItem*>(sc->querySelector(
					"right-actions-menu > reload-button")
				)) reload_btn->activate();
			}
		);
		//toggle
		auto toggle = CCMenuItemExt::createTogglerWithStandardSprites(0.6,
			[](auto) {
				TOGGLE_MAIN();
			}
		);
		toggle->toggle(enabled);
		toggle->setPosition(20, 20);
		m_buttonMenu->addChild(toggle);
		//label
		auto Label = CCLabelBMFont::create("Unverified", "bigFont.fnt");
		Label->setScale(0.40f);
		Label->setAnchorPoint({ 0.f, 0.5f });
		Label->setPosition(32, 20);
		m_buttonMenu->addChild(Label);
	}
	void show() {
		FLAlertLayer::show();
		if (typeinfo_cast<FiltersPopup*>(this) and CCScene::get()->querySelector("ModsLayer")) setupForFiltersPopup();
		if (typeinfo_cast<ModPopup*>(this) and CCScene::get()->querySelector("ModsLayer")) setupForModPopup();
	}
};