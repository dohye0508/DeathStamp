#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/Slider.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <algorithm>
#include <cmath>

constexpr float kDeathStampPi = 3.14159265358979323846f;

using namespace geode::prelude;

static ccColor3B colorForMarkerColorSetting(std::string const& name) {
    if (name == "green") return ccc3(60, 220, 100);
    return ccc3(235, 60, 60);
}

class $modify(DeathStampPlayLayer, PlayLayer) {
    struct Fields {
        std::vector<CCNode*> markers;
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
        auto style = Mod::get()->getSettingValue<std::string>("marker-style");

        CCNode* marker = nullptr;
        if (style == "x" || style == "o") {
            marker = createShapeMarker(player, style);
        } else {
            marker = createPlayerMarker(player);
        }
        if (!marker) return;

        m_fields->markers.push_back(marker);

        int64_t maxMarkers = Mod::get()->getSettingValue<int64_t>("max-markers");
        if (maxMarkers > 0) {
            while (static_cast<int64_t>(m_fields->markers.size()) > maxMarkers) {
                auto* oldest = m_fields->markers.front();
                if (oldest) oldest->removeFromParent();
                m_fields->markers.erase(m_fields->markers.begin());
            }
        }
    }

    // A real PlayerObject clone, not SimplePlayer — attempt-playback-style
    // mods use exactly this for in-level ghosts/markers (SimplePlayer only
    // ever shows up in their UI popups, never dropped into the live game
    // world), so this is the combination proven to actually render here.
    CCNode* createPlayerMarker(PlayerObject* player) {
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");
        auto gm = GameManager::sharedState();

        auto* ghost = PlayerObject::create(gm->getPlayerFrame(), gm->getPlayerShip(), this, m_objectLayer, false);
        if (!ghost) {
            log::warn("Death Stamp: PlayerObject::create returned null");
            return nullptr;
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
        ghost->setID("death-stamp-marker"_spr);
        if (ghost->m_waveTrail) {
            ghost->m_waveTrail->setVisible(false);
        }

        // Applied before rotation: toggleFlyMode/toggleRollMode/etc. reset
        // the player's rotation as part of switching vehicles, which was
        // wiping out setRotation() when that call came first.
        applyPlayerMode(ghost, player, gm);
        ghost->setRotation(player->getRotation());

        // Add to the exact same parent the live player is in, so its
        // position needs no coordinate-space conversion at all.
        player->getParent()->addChild(ghost, 9999);
        ghost->setPosition(player->getPosition());
        return ghost;
    }

    CCNode* createShapeMarker(PlayerObject* player, std::string const& style) {
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");
        auto colorName = Mod::get()->getSettingValue<std::string>("marker-color");
        ccColor3B color = colorForMarkerColorSetting(colorName);
        float alpha = static_cast<float>(opacity) / 255.f;
        ccColor4F c = ccc4f(color.r / 255.f, color.g / 255.f, color.b / 255.f, alpha);

        auto* draw = CCDrawNode::create();
        draw->setID("death-stamp-marker"_spr);

        if (style == "x") {
            constexpr float half = 9.f;
            constexpr float thickness = 2.5f;
            draw->drawSegment({-half, -half}, {half, half}, thickness, c);
            draw->drawSegment({-half, half}, {half, -half}, thickness, c);
        } else {
            // "o" — approximate a ring out of short segments; CCDrawNode has
            // no direct "draw circle outline" call.
            constexpr int segments = 20;
            constexpr float radius = 9.f;
            constexpr float thickness = 2.2f;
            for (int i = 0; i < segments; i++) {
                float a0 = static_cast<float>(i) / segments * 2.f * kDeathStampPi;
                float a1 = static_cast<float>(i + 1) / segments * 2.f * kDeathStampPi;
                CCPoint p0 = {radius * cosf(a0), radius * sinf(a0)};
                CCPoint p1 = {radius * cosf(a1), radius * sinf(a1)};
                draw->drawSegment(p0, p1, thickness, c);
            }
        }

        player->getParent()->addChild(draw, 9999);
        draw->setPosition(player->getPosition());
        return draw;
    }

    // PlayerObject::create() always gives you a cube — the vehicle a player
    // is in (ship/ball/UFO/wave/robot/spider/swing) is separate runtime state
    // (m_isShip etc.) that has to be copied over explicitly and paired with
    // the matching updatePlayer*Frame call, or it silently stays a cube.
    // Pattern taken from AttemptPlaybackMod's setPOFrameForIcon/forceMode.
    static void applyPlayerMode(PlayerObject* ghost, PlayerObject* live, GameManager* gm) {
        ghost->toggleFlyMode(false, true);
        ghost->toggleBirdMode(false, true);
        ghost->toggleRollMode(false, true);
        ghost->toggleDartMode(false, true);
        ghost->toggleRobotMode(false, true);
        ghost->toggleSpiderMode(false, true);
        ghost->toggleSwingMode(false, true);

        if (live->m_isShip) {
            ghost->toggleFlyMode(true, true);
            ghost->updatePlayerShipFrame(gm->getPlayerShip());
            ghost->updatePlayerFrame(gm->getPlayerFrame());
        } else if (live->m_isBird) {
            ghost->toggleBirdMode(true, true);
            ghost->updatePlayerBirdFrame(gm->getPlayerBird());
            ghost->updatePlayerFrame(gm->getPlayerFrame());
        } else if (live->m_isBall) {
            ghost->toggleRollMode(true, true);
            ghost->updatePlayerRollFrame(gm->getPlayerBall());
        } else if (live->m_isDart) {
            ghost->toggleDartMode(true, true);
            ghost->updatePlayerDartFrame(gm->getPlayerDart());
        } else if (live->m_isRobot) {
            ghost->toggleRobotMode(true, true);
            ghost->updatePlayerRobotFrame(gm->getPlayerRobot());
        } else if (live->m_isSpider) {
            ghost->toggleSpiderMode(true, true);
            ghost->updatePlayerSpiderFrame(gm->getPlayerSpider());
        } else if (live->m_isSwing) {
            ghost->toggleSwingMode(true, true);
            ghost->updatePlayerSwingFrame(gm->getPlayerSwing());
        } else {
            ghost->updatePlayerFrame(gm->getPlayerFrame());
        }

        if (ghost->m_isUpsideDown != live->m_isUpsideDown) {
            ghost->flipGravity(live->m_isUpsideDown, true);
        }
        if (ghost->m_vehicleSize != live->m_vehicleSize) {
            ghost->m_vehicleSize = live->m_vehicleSize;
            ghost->updatePlayerScale();
        }
        ghost->m_isGoingLeft = live->m_isGoingLeft;
    }

    void clearMarkers() {
        for (auto marker : m_fields->markers) {
            if (marker) marker->removeFromParent();
        }
        m_fields->markers.clear();
    }
};

// Settings popup opened from the pause-menu button. Everything here reads
// straight from the persisted mod settings and writes straight back to them
// on every click/change — no separate widget-side state to keep in sync,
// since chasing that (CCMenuItemToggler's internal toggled state specifically)
// is what caused the pause-button bugs earlier in this mod's history.
class DeathStampSettingsPopup : public geode::Popup {
protected:
    CCMenuItemSpriteExtra* m_enabledCheckbox = nullptr;
    CCMenuItemSpriteExtra* m_clearCheckbox = nullptr;
    CCMenuItemSpriteExtra* m_styleButtons[3] = {nullptr, nullptr, nullptr};
    CCMenuItemSpriteExtra* m_colorButtons[2] = {nullptr, nullptr};
    Slider* m_opacitySlider = nullptr;
    CCLabelBMFont* m_opacityLabel = nullptr;
    geode::TextInput* m_maxInput = nullptr;
    CCMenu* m_menu = nullptr;

    bool init(float width, float height) {
        if (!Popup::init(width, height)) return false;

        setID("death-stamp-settings-popup"_spr);
        setTitle("Death Stamp 설정");

        auto* root = CCNode::create();
        root->setPosition(m_mainLayer->getContentSize() * 0.5f);
        m_mainLayer->addChild(root);

        m_menu = CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        root->addChild(m_menu);

        constexpr float labelX = -140.f;
        constexpr float labelScale = 0.42f;

        // Row 1: enabled
        m_enabledCheckbox = CCMenuItemSpriteExtra::create(
            CCSprite::create(), this, menu_selector(DeathStampSettingsPopup::onToggleEnabled)
        );
        m_enabledCheckbox->setPosition({labelX + 8.f, 108.f});
        m_menu->addChild(m_enabledCheckbox);

        auto* enabledLabel = CCLabelBMFont::create("Death Stamp 켜기", "bigFont.fnt");
        enabledLabel->setAnchorPoint({0.f, 0.5f});
        enabledLabel->setPosition({labelX + 24.f, 108.f});
        enabledLabel->setScale(labelScale);
        root->addChild(enabledLabel);

        // Row 2: clear on new attempt
        m_clearCheckbox = CCMenuItemSpriteExtra::create(
            CCSprite::create(), this, menu_selector(DeathStampSettingsPopup::onToggleClear)
        );
        m_clearCheckbox->setPosition({labelX + 8.f, 78.f});
        m_menu->addChild(m_clearCheckbox);

        auto* clearLabel = CCLabelBMFont::create("재시도마다 표시 지우기", "bigFont.fnt");
        clearLabel->setAnchorPoint({0.f, 0.5f});
        clearLabel->setPosition({labelX + 24.f, 78.f});
        clearLabel->setScale(labelScale);
        root->addChild(clearLabel);

        // Row 3: marker style
        auto* styleLabel = CCLabelBMFont::create("마커 모양", "bigFont.fnt");
        styleLabel->setAnchorPoint({0.f, 0.5f});
        styleLabel->setPosition({labelX, 38.f});
        styleLabel->setScale(labelScale);
        root->addChild(styleLabel);

        m_styleNames[0] = "아이콘"; m_styleNames[1] = "X"; m_styleNames[2] = "O";
        m_styleValues[0] = "player"; m_styleValues[1] = "x"; m_styleValues[2] = "o";
        const float styleX[3] = {-15.f, 45.f, 100.f};
        for (int i = 0; i < 3; i++) {
            m_styleButtons[i] = CCMenuItemSpriteExtra::create(
                CCSprite::create(), this, menu_selector(DeathStampSettingsPopup::onPickStyle)
            );
            m_styleButtons[i]->setTag(i);
            m_styleButtons[i]->setPosition({styleX[i], 38.f});
            m_menu->addChild(m_styleButtons[i]);
        }

        // Row 4: marker color
        auto* colorLabel = CCLabelBMFont::create("마커 색 (X/O 전용)", "bigFont.fnt");
        colorLabel->setAnchorPoint({0.f, 0.5f});
        colorLabel->setPosition({labelX, 4.f});
        colorLabel->setScale(labelScale);
        root->addChild(colorLabel);

        m_colorNames[0] = "빨강"; m_colorNames[1] = "초록";
        m_colorValues[0] = "red"; m_colorValues[1] = "green";
        const float colorX[2] = {40.f, 95.f};
        for (int i = 0; i < 2; i++) {
            m_colorButtons[i] = CCMenuItemSpriteExtra::create(
                CCSprite::create(), this, menu_selector(DeathStampSettingsPopup::onPickColor)
            );
            m_colorButtons[i]->setTag(i);
            m_colorButtons[i]->setPosition({colorX[i], 4.f});
            m_menu->addChild(m_colorButtons[i]);
        }

        // Row 5: opacity
        auto* opacityLabel = CCLabelBMFont::create("표시 진하기", "bigFont.fnt");
        opacityLabel->setAnchorPoint({0.f, 0.5f});
        opacityLabel->setPosition({labelX, -32.f});
        opacityLabel->setScale(labelScale);
        root->addChild(opacityLabel);

        m_opacitySlider = Slider::create(
            this, menu_selector(DeathStampSettingsPopup::onOpacitySlider), 0.6f
        );
        m_opacitySlider->setPosition({-25.f, -56.f});
        m_opacitySlider->setScale(0.6f);
        m_menu->addChild(m_opacitySlider);

        m_opacityLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_opacityLabel->setPosition({108.f, -32.f});
        m_opacityLabel->setScale(0.35f);
        root->addChild(m_opacityLabel);

        // Row 6: max markers
        auto* maxLabel = CCLabelBMFont::create("최근 몇 개만 (0=전체)", "bigFont.fnt");
        maxLabel->setAnchorPoint({0.f, 0.5f});
        maxLabel->setPosition({labelX, -80.f});
        maxLabel->setScale(labelScale);
        root->addChild(maxLabel);

        m_maxInput = geode::TextInput::create(60.f, "0");
        m_maxInput->setCommonFilter(geode::CommonFilter::Uint);
        m_maxInput->setMaxCharCount(3);
        m_maxInput->setPosition({105.f, -80.f});
        m_maxInput->setScale(0.85f);
        m_maxInput->setString(std::to_string(Mod::get()->getSettingValue<int64_t>("max-markers")));
        m_maxInput->setCallback([](std::string const& text) {
            int64_t value = text.empty() ? 0 : std::stoll(text);
            if (value < 0) value = 0;
            if (value > 999) value = 999;
            Mod::get()->setSettingValue<int64_t>("max-markers", value);
        });
        root->addChild(m_maxInput);

        refresh_();
        return true;
    }

    void refresh_() {
        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        bool clear = Mod::get()->getSettingValue<bool>("clear-on-new-attempt");
        auto style = Mod::get()->getSettingValue<std::string>("marker-style");
        auto colorName = Mod::get()->getSettingValue<std::string>("marker-color");
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");

        m_enabledCheckbox->setSprite(checkSprite_(enabled));
        m_clearCheckbox->setSprite(checkSprite_(clear));

        for (int i = 0; i < 3; i++) {
            bool selected = style == m_styleValues[i];
            m_styleButtons[i]->setSprite(pillSprite_(m_styleNames[i], selected));
        }
        for (int i = 0; i < 2; i++) {
            bool selected = colorName == m_colorValues[i];
            m_colorButtons[i]->setSprite(pillSprite_(m_colorNames[i], selected));
        }

        m_opacitySlider->setValue(
            std::clamp(static_cast<float>(opacity - 20) / (255.f - 20.f), 0.f, 1.f)
        );
        m_opacityLabel->setString(fmt::format("{}/255", opacity).c_str());
    }

    static CCSprite* checkSprite_(bool checked) {
        return CCSprite::createWithSpriteFrameName(
            checked ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png"
        );
    }

    static ButtonSprite* pillSprite_(std::string const& text, bool selected) {
        auto* sprite = ButtonSprite::create(
            text.c_str(), "bigFont.fnt",
            selected ? "GJ_button_01.png" : "GJ_button_04.png", 0.8f
        );
        sprite->setScale(0.55f);
        return sprite;
    }

    void onToggleEnabled(CCObject*) {
        bool newValue = !Mod::get()->getSettingValue<bool>("enabled");
        Mod::get()->setSettingValue<bool>("enabled", newValue);
        refresh_();
    }

    void onToggleClear(CCObject*) {
        bool newValue = !Mod::get()->getSettingValue<bool>("clear-on-new-attempt");
        Mod::get()->setSettingValue<bool>("clear-on-new-attempt", newValue);
        refresh_();
    }

    void onPickStyle(CCObject* sender) {
        int idx = static_cast<CCNode*>(sender)->getTag();
        if (idx < 0 || idx > 2) return;
        Mod::get()->setSettingValue<std::string>("marker-style", m_styleValues[idx]);
        refresh_();
    }

    void onPickColor(CCObject* sender) {
        int idx = static_cast<CCNode*>(sender)->getTag();
        if (idx < 0 || idx > 1) return;
        Mod::get()->setSettingValue<std::string>("marker-color", m_colorValues[idx]);
        refresh_();
    }

    void onOpacitySlider(CCObject*) {
        int opacity = 20 + static_cast<int>(std::lround(
            std::clamp(m_opacitySlider->getValue(), 0.f, 1.f) * (255.f - 20.f)
        ));
        Mod::get()->setSettingValue<int64_t>("marker-opacity", opacity);
        m_opacityLabel->setString(fmt::format("{}/255", opacity).c_str());
    }

private:
    std::string m_styleNames[3];
    std::string m_styleValues[3];
    std::string m_colorNames[2];
    std::string m_colorValues[2];

public:
    static DeathStampSettingsPopup* create() {
        auto ret = new DeathStampSettingsPopup();
        if (ret && ret->init(340.f, 300.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ESC 정지화면에 Death Stamp 설정 팝업을 여는 버튼.
class $modify(DeathStampPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto sprite = ButtonSprite::create("Death Stamp", "goldFont.fnt", "GJ_button_02.png", 0.8f);
        sprite->setScale(0.6f);

        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(DeathStampPauseLayer::onOpenSettings)
        );
        button->setID("death-stamp-toggle"_spr);
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

    void onOpenSettings(CCObject*) {
        if (auto* popup = DeathStampSettingsPopup::create()) {
            popup->show();
        }
    }
};
