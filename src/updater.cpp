#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

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
			auto installed_size = std::filesystem::file_size(getMod()->getPackagePath().string(), err_code_to_ignore);
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
								log::debug("{}", res.into(getMod()->getPackagePath().string()).err());
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