#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(DeathStampPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<CCDrawNode*> markers;
        float stampCooldownRemaining = 0.f;
        CCLabelBMFont* statusLabel = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto label = CCLabelBMFont::create("DEATH STAMP: NOCLIP TEST", "bigFont.fnt");
        label->setScale(0.35f);
        label->setOpacity(180);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({6.f, CCDirector::sharedDirector()->getWinSize().height - 6.f});
        label->setID("death-stamp-status-label"_spr);
        label->setZOrder(1000);
        this->addChild(label);
        m_fields->statusLabel = label;

        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        if (m_fields->stampCooldownRemaining > 0.f) {
            m_fields->stampCooldownRemaining -= dt;
        }

        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        if (m_fields->statusLabel) {
            m_fields->statusLabel->setVisible(enabled);
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        if (!enabled) {
            PlayLayer::destroyPlayer(player, obj);
            return;
        }

        // destroyPlayer fires every frame the player overlaps a hazard, so
        // the cooldown timer is what turns repeated calls into a single
        // "rising edge" stamp instead of spamming markers.
        if (m_fields->stampCooldownRemaining <= 0.f) {
            stampDeathMarker(player->getPosition());
            m_fields->stampCooldownRemaining = Mod::get()->getSettingValue<double>("stamp-cooldown");
        }

        // Hazards in GD don't physically block movement, they only trigger
        // this callback — so simply not calling the original keeps the
        // player moving through instead of resetting the level.
        if (Mod::get()->getSettingValue<bool>("noclip")) {
            return;
        }

        PlayLayer::destroyPlayer(player, obj);
    }

    void resetLevel() {
        m_fields->stampCooldownRemaining = 0.f;
        if (Mod::get()->getSettingValue<bool>("clear-on-new-attempt")) {
            clearMarkers();
        }
        PlayLayer::resetLevel();
    }

    void stampDeathMarker(CCPoint pos) {
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");

        auto marker = CCDrawNode::create();
        marker->drawDot({0.f, 0.f}, 7.f, ccc4f(1.f, 0.15f, 0.15f, opacity / 255.f));
        marker->drawSegment({-9.f, -9.f}, {9.f, 9.f}, 2.f, ccc4f(1.f, 1.f, 1.f, opacity / 255.f));
        marker->drawSegment({-9.f, 9.f}, {9.f, -9.f}, 2.f, ccc4f(1.f, 1.f, 1.f, opacity / 255.f));
        marker->setPosition(pos);
        marker->setZOrder(1000);
        marker->setID("death-stamp-marker"_spr);

        m_objectLayer->addChild(marker);
        m_fields->markers.push_back(marker);
    }

    void clearMarkers() {
        for (auto marker : m_fields->markers) {
            marker->removeFromParent();
        }
        m_fields->markers.clear();
    }
};

// ESC 정지화면에서 Death Stamp를 바로 켜고 끌 수 있는 토글 버튼.
class $modify(DeathStampPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        bool enabled = Mod::get()->getSettingValue<bool>("enabled");

        auto onSprite = ButtonSprite::create("Death Stamp: ON", "goldFont.fnt", "GJ_button_02.png", 0.8f);
        auto offSprite = ButtonSprite::create("Death Stamp: OFF", "chatFont.fnt", "GJ_button_04.png", 0.8f);
        onSprite->setScale(0.6f);
        offSprite->setScale(0.6f);

        auto toggler = CCMenuItemToggler::create(
            onSprite, offSprite,
            this, menu_selector(DeathStampPauseLayer::onToggleDeathStamp)
        );
        toggler->toggle(enabled);
        toggler->setID("death-stamp-toggle"_spr);

        auto menu = CCMenu::create();
        menu->setID("death-stamp-toggle-menu"_spr);
        menu->addChild(toggler);
        menu->setContentSize(toggler->getContentSize());
        menu->setAnchorPoint({0.f, 0.f});

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({20.f, 20.f});
        this->addChild(menu, 100);
    }

    void onToggleDeathStamp(CCObject* sender) {
        // CCMenuItemToggler flips its own m_bToggled before invoking this
        // callback, so isToggled() already reflects the state to apply.
        auto toggler = static_cast<CCMenuItemToggler*>(sender);
        Mod::get()->setSettingValue<bool>("enabled", toggler->isToggled());
    }
};
