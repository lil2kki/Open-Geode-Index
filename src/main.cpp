#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

static const std::string myAPI = "open-geode.ir";

// Alt. Index
static bool altIndexEnabled = true;

// Unverified mods
static bool unverifiedEnabled = false;
static std::string latestModVersion = "xd";

// typeinfo_cast <3
class ModPopup : public FLAlertLayer {};
class FiltersPopup : public FLAlertLayer {};

$on_mod(DataSaved) {
    Mod::get()->setSavedValue<bool>("alt-index-enabled", altIndexEnabled);
    Mod::get()->setSavedValue<bool>("unverified-enabled", unverifiedEnabled);
}

static void applyUnverified(web::WebRequest& req, std::string& url) {
    if (unverifiedEnabled and string::contains(url, "api.geode-sdk.org/v1/mods")) {
        if (url == "https://api.geode-sdk.org/v1/mods") {
            req.param("status", "pending");
        }

        url = string::replace(url, "latest", string::replace(latestModVersion, "v", ""));

        if (string::contains(url, "/logo")) {
            auto parts = string::split(url, "/");
            if (parts.size() >= 2) {
                url = fmt::format("https://{}/mod.logo.php?id={}", myAPI, parts[parts.size() - 2]);
            }
        }
    }

    if (string::contains(url, "/mods/updates")) {
        url = string::replace(url, "api.geode-sdk.org/v1/mods/updates", myAPI + "/mod.updates.php");
    }
}

static void applyAltIndex(std::string& url) {
    if (string::contains(url, myAPI + "/mod.updates.php")) {
        url = "https://" + myAPI + "/v1/mods/updates";
    }

    auto repl = [&] { url = string::replace(url, "api.geode-sdk.org", myAPI); };

    if (altIndexEnabled) repl();
    if (string::contains(url, "/v1/mods/")) repl();
}

void openLinkInBrowserDT(ZStringView url) {
    geode::utils::web::openLinkInBrowser(string::replace(
        url.data(),
        "geode-sdk.org/mods/",
        (myAPI + "/ui/mod/").data()
    ).data());
}

$on_mod(Loaded) {
    // load state
    altIndexEnabled = Mod::get()->getSavedValue<bool>("alt-index-enabled");
    unverifiedEnabled = Mod::get()->getSavedValue<bool>("unverified-enabled");

    // intercept requests
    web::WebRequestInterceptEvent().listen(
        [](std::string_view id, web::WebRequest& req) {
            //log::debug("{}(id {}, req {})", __func__, id, req.getUrl());
            std::string url = req.getUrl().data();
            req.header("X-Upstream-Url", url);

            applyUnverified(req, url);
            applyAltIndex(url);

            req.url(url);
            return ListenerResult::Propagate;
        },
        Priority::Stub
    ).leak();

    // intercept geode mods links
    Mod::get()->hook(
        reinterpret_cast<void*>(geode::addresser::getNonVirtual(
            geode::modifier::Resolve<ZStringView>::func(&geode::utils::web::openLinkInBrowser)
        )),
        &openLinkInBrowserDT,
        "geode::utils::web::openLinkInBrowser",
        tulip::hook::TulipConvention::Stdcall
    ).unwrap();
}

class $modify(AltIndexUnverifiedPopupCatch, FLAlertLayer) {
    void setupForModPopup() {
        if (Ref ver = typeinfo_cast<CCLabelBMFont*>(
            this->querySelector("mod-stats-container > version > value-label")
        )) latestModVersion = ver->getString();
    }

    void setupForFiltersPopup() {
        // reload on apply
        addCleanupCallback(
            [sc = Ref(this->getParent())] {
                if (auto reloadBtn = typeinfo_cast<CCMenuItem*>(sc->querySelector(
                    "right-actions-menu > reload-button")
                )) reloadBtn->activate();
            }
        );

        // Alt. Index
        auto altToggle = CCMenuItemExt::createTogglerWithStandardSprites(
            0.6f, [](auto) { altIndexEnabled = !altIndexEnabled; }
        );
        altToggle->toggle(altIndexEnabled);
        altToggle->setPosition(20, 45);
        m_buttonMenu->addChild(altToggle);

        auto altLabel = CCLabelBMFont::create("Alt. Index", "bigFont.fnt");
        altLabel->setScale(0.325f);
        altLabel->setAnchorPoint({ 0.f, 0.5f });
        altLabel->setPosition(32, 45);
        m_buttonMenu->addChild(altLabel);

        // Unverified
        auto unvToggle = CCMenuItemExt::createTogglerWithStandardSprites(
            0.6f, [](auto) { unverifiedEnabled = !unverifiedEnabled; }
        );
        unvToggle->toggle(unverifiedEnabled);
        unvToggle->setPosition(20, 20);
        m_buttonMenu->addChild(unvToggle);

        auto unvLabel = CCLabelBMFont::create("Unverified", "bigFont.fnt");
        unvLabel->setScale(0.40f);
        unvLabel->setAnchorPoint({ 0.f, 0.5f });
        unvLabel->setPosition(32, 20);
        m_buttonMenu->addChild(unvLabel);
    }

    void show() {
        FLAlertLayer::show();
        if (!CCScene::get()->querySelector("ModsLayer")) return;
        if (typeinfo_cast<FiltersPopup*>(this)) setupForFiltersPopup();
        if (typeinfo_cast<ModPopup*>(this)) setupForModPopup();
    }
};