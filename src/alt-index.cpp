#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

auto myAPI = std::string("open-geode.ir");

auto enabled = true;

$on_mod(DataSaved) {
    Mod::get()->setSavedValue<bool>("enabled", enabled);
}

void openLinkInBrowserDT(ZStringView url) {
	geode::utils::web::openLinkInBrowser(string::replace(
		url.data(),
		"geode-sdk.org/mods/",
		(myAPI + "/ui/mod/").data()
	).data());
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

			self->header("X-Upstream-Url", givenUrl);

			if (string::contains(givenUrl.data(), "open-geode.ir/mod.updates.php")) {
				givenUrl = "https://" + myAPI + "/v1/mods/updates";
			}

			auto repl = [&] { givenUrl = string::replace(givenUrl.data(), "api.geode-sdk.org", myAPI); };
			if (enabled) repl();
			if (string::contains(givenUrl.data(), "/v1/mods/")) repl();

			self->url(givenUrl);

            return ListenerResult::Propagate; 
        }, Priority::Last
    ).leak();
	//intercept geode mods links
	Mod::get()->hook(
		reinterpret_cast<void*>(geode::addresser::getNonVirtual(
			geode::modifier::Resolve<ZStringView>::func(&geode::utils::web::openLinkInBrowser)
		)),
		&openLinkInBrowserDT,
		"geode::utils::web::openLinkInBrowser",
		tulip::hook::TulipConvention::Stdcall
	).unwrap();
}

#include <Geode/modify/FLAlertLayer.hpp>
class ModPopup : public FLAlertLayer {};
class FiltersPopup : public FLAlertLayer {};
class $modify(AltIndexPopupCatch, FLAlertLayer) {
	void setupForModPopup() {
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
		auto toggle = CCMenuItemExt::createTogglerWithStandardSprites(
			0.6, [](auto) { enabled = !enabled; }
		);
		toggle->toggle(enabled);
		toggle->setPosition(20, 45);
		m_buttonMenu->addChild(toggle);
		//label
		auto Label = CCLabelBMFont::create("Alt. Index", "bigFont.fnt");
		Label->setScale(0.325f);
		Label->setAnchorPoint({ 0.f, 0.5f });
		Label->setPosition(32, 45);
		m_buttonMenu->addChild(Label);
    }
	void show() {
		FLAlertLayer::show();
        if (typeinfo_cast<FiltersPopup*>(this) and CCScene::get()->querySelector("ModsLayer")) setupForFiltersPopup();
		if (typeinfo_cast<ModPopup*>(this) and CCScene::get()->querySelector("ModsLayer")) setupForModPopup();
	}
};