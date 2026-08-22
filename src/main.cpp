#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

auto myAPI = std::string("open-geode.7m.pl");

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

			if (string::contains(givenUrl.data(), "geode-comments.bccst.ru/mod.updates.php")) {
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
class $modify(PopupCatch, FLAlertLayer) {
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

//deps and dodeps
#include <Geode/modify/MenuLayer.hpp>
class $modify(MenuLayerExt, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;

        static int ok = 0;
        if (ok++) return ok;

		static auto id = std::string(getMod()->getID());
		static auto repo = getMod()->getMetadata().getLinks().getSourceURL().value_or("https://github.com/lil2kki/Open-Geode-Index");

		//size check
		auto webListener = new async::TaskHolder<web::WebResponse>;
		auto req = web::WebRequest();
		req.onProgress([_this = Ref(this), webListener](web::WebProgress const& prog) {
			//log::debug("{}", prog.downloadTotal());

			if (prog.downloadTotal() > 0) void(); else return;

			webListener->cancel();

			auto err_code_to_ignore = std::error_code();
			auto installed_size = std::filesystem::file_size(getMod()->getPackagePath(), err_code_to_ignore);
			auto actual_size = prog.downloadTotal();

			if (installed_size == actual_size) return;

            openInfoPopup(id);

			auto pop = geode::createQuickPopup(
                (getMod()->getName() + " Update!").c_str(),
				fmt::format(
					"Latest release size mismatch with installed one!"
					"\n" "Download latest release of mod?"
				),
				"Later.", "Yes", [](CCNode* pop, bool Yes) {
					if (!Yes) return;

					Ref state_win = Notification::create("Downloading... (///%)");
					state_win->setTime(1337.f);
					state_win->show();

					auto dlReq = web::WebRequest();

					dlReq.onProgress([state_win](web::WebProgress const& p) {
						state_win->setString(fmt::format("Downloading... ({}%)", (int)p.downloadProgress().value_or(000)));
						});

					auto listener = new async::TaskHolder<web::WebResponse>;
					listener->spawn(
						dlReq.get(repo + "/releases/latest/download/" + id + ".geode"),
						[state_win](web::WebResponse res) {
							std::string data = res.string().unwrapOr("no res");
							state_win->removeFromParent();
                            openModsList();
							if (res.code() < 399) {
								log::debug("{}", res.into(getMod()->getPackagePath()).err());
								auto geode = Loader::get()->getInstalledMod("geode.loader");//log-thread
								geode->setSettingValue("log-thread", !geode->getSettingValue<bool>("log-thread"));
								geode->setSettingValue("log-thread", !geode->getSettingValue<bool>("log-thread"));
							}
							else {
								auto asd = geode::createQuickPopup(
									"Request exception",
									data,
									"OK", nullptr, 420.f, nullptr, false
								);
								asd->show();
							};
						}
					);

				}, false
			);
            pop->m_scene = OverlayManager::get();
			pop->show();
			});
		webListener->spawn(req.get(repo + "/releases/latest/download/" + id + ".geode"), [](auto) {});

		return true;
	}
};