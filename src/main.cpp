#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>

using namespace geode::prelude;

class $modify(DeathStampPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<PlayerObject*> markers;
        bool hasStampedThisAttempt = false;
    };

    void update(float dt) {
        PlayLayer::update(dt);

        // GD streams/culls level objects that fall far behind the camera to
        // save on rendering — since our markers aren't part of its tracked
        // object list, force them back visible every frame so they don't
        // silently get hidden as the player moves on.
        for (auto* marker : m_fields->markers) {
            if (marker && !marker->isVisible()) {
                marker->setVisible(true);
            }
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        // Many levels have a decorative hazard (or an invisible object RobTop
        // places at the start of every level) sitting right on top of the
        // spawn point, which fires destroyPlayer there on every respawn even
        // though it's not a real hazard. AttemptPlaybackMod hits the exact
        // same false positive and ignores anything within 45 units (1.5
        // blocks) of the start — same threshold here.
        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        bool nearSpawn = player && player->getPositionX() < 45.f;

        if (enabled && !nearSpawn && !m_fields->hasStampedThisAttempt) {
            m_fields->hasStampedThisAttempt = true;
            stampDeathMarker(player);
        }

        PlayLayer::destroyPlayer(player, obj);
    }

    void resetLevel() {
        m_fields->hasStampedThisAttempt = false;
        if (Mod::get()->getSettingValue<bool>("clear-on-new-attempt")) {
            clearMarkers();
        }
        PlayLayer::resetLevel();
    }

    void stampDeathMarker(PlayerObject* player) {
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");
        auto gm = GameManager::sharedState();

        // A real PlayerObject clone, not SimplePlayer — attempt-playback-style
        // mods use exactly this for in-level ghosts/markers (SimplePlayer only
        // ever shows up in their UI popups, never dropped into the live game
        // world), so this is the combination proven to actually render here.
        auto* ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, m_objectLayer, false);
        if (!ghost) {
            log::warn("Death Stamp: PlayerObject::create returned null");
            return;
        }

        // NOTE: deliberately NOT calling stopAllActions()/unscheduleUpdate()
        // here — AttemptPlaybackMod's own createNewGhost() doesn't either.
        // Freezing the node immediately after create() risks catching it mid
        // spawn-in transition, before it ever becomes visible. m_isDead is
        // what actually halts a player's own physics/simulation each frame,
        // which is the correct way to pin it in place.
        ghost->disablePlayerControls();
        ghost->m_isDead = true;
        ghost->setVisible(true);
        ghost->setColor(gm->colorForIdx(gm->getPlayerColor()));
        ghost->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        ghost->setOpacity(static_cast<GLubyte>(opacity));
        ghost->setRotation(player->getRotation());
        ghost->setID("death-stamp-marker"_spr);
        if (ghost->m_waveTrail) {
            ghost->m_waveTrail->setVisible(false);
        }

        // Add to the exact same parent the live player is in, so its
        // position needs no coordinate-space conversion at all.
        player->getParent()->addChild(ghost, 9999);
        ghost->setPosition(player->getPosition());

        m_fields->markers.push_back(ghost);
    }

    void clearMarkers() {
        for (auto marker : m_fields->markers) {
            marker->removeFromParent();
        }
        m_fields->markers.clear();
    }
};

// ESC 정지화면에서 Death Stamp를 바로 켜고 끌 수 있는 토글 버튼.
//
// This deliberately does NOT use CCMenuItemToggler. Its isToggled()/toggle()
// state kept behaving inconsistently across builds — sometimes needing two
// clicks, sometimes not updating at all, sometimes flipped on reopen — and
// none of the fixes chasing its internal semantics held up under testing.
// A plain CCMenuItemSpriteExtra with a manually swapped sprite sidesteps all
// of that: the persisted setting is the only state that exists, and the
// button image is just redrawn from it every time.
class $modify(DeathStampPauseLayer, PauseLayer) {
    struct Fields {
        CCMenuItemSpriteExtra* toggleButton = nullptr;
    };

    static ButtonSprite* createToggleSprite(bool enabled) {
        auto sprite = enabled
            ? ButtonSprite::create("Death Stamp: ON", "goldFont.fnt", "GJ_button_02.png", 0.8f)
            : ButtonSprite::create("Death Stamp: OFF", "goldFont.fnt", "GJ_button_06.png", 0.8f);
        sprite->setScale(0.6f);
        return sprite;
    }

    void customSetup() {
        PauseLayer::customSetup();

        bool enabled = Mod::get()->getSettingValue<bool>("enabled");

        auto button = CCMenuItemSpriteExtra::create(
            createToggleSprite(enabled),
            this, menu_selector(DeathStampPauseLayer::onToggleDeathStamp)
        );
        button->setID("death-stamp-toggle"_spr);
        m_fields->toggleButton = button;
        // Anchor the button's own bottom-left corner to the menu's origin —
        // without this its default center anchor bled half its width/height
        // past the menu's (0,0), pushing it off the left edge of the screen.
        button->setPosition(button->getContentSize().width / 2.f, button->getContentSize().height / 2.f);

        auto menu = CCMenu::create();
        menu->setID("death-stamp-toggle-menu"_spr);
        menu->addChild(button);
        menu->setContentSize(button->getContentSize());
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({40.f, 20.f});

        this->addChild(menu, 100);
    }

    void onToggleDeathStamp(CCObject*) {
        bool newValue = !Mod::get()->getSettingValue<bool>("enabled");
        Mod::get()->setSettingValue<bool>("enabled", newValue);
        m_fields->toggleButton->setSprite(createToggleSprite(newValue));
    }
};
