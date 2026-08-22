#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/Slider.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCControlColourPicker.h>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCScale9Sprite.h>
#include <algorithm>
#include <cmath>

constexpr float kDeathStampPi = 3.14159265358979323846f;

using namespace geode::prelude;

static ccColor3B readMarkerColor() {
    auto* mod = Mod::get();
    return ccc3(
        static_cast<GLubyte>(mod->getSettingValue<int64_t>("marker-color-r")),
        static_cast<GLubyte>(mod->getSettingValue<int64_t>("marker-color-g")),
        static_cast<GLubyte>(mod->getSettingValue<int64_t>("marker-color-b"))
    );
}

static void writeMarkerColor(ccColor3B color) {
    auto* mod = Mod::get();
    mod->setSettingValue<int64_t>("marker-color-r", color.r);
    mod->setSettingValue<int64_t>("marker-color-g", color.g);
    mod->setSettingValue<int64_t>("marker-color-b", color.b);
}

// GD's built-in bitmap fonts (bigFont.fnt/goldFont.fnt) only have Latin
// glyphs baked into their texture atlas — Korean text silently draws as
// nothing (zero-width) instead of erroring, which is why the popup's UI
// text has to stay English even though the mod's settings descriptions
// and about.md (rendered through Geode's own text widgets, not GD's) can
// be Korean.
static constexpr const char* kMarkerStyleValues[] = {"player", "x", "o"};
static constexpr const char* kMarkerStyleLabels[] = {"Icon", "X", "O"};
static constexpr int kMarkerStyleCount = 3;

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
        ccColor3B color = readMarkerColor();
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

public:
    // Called from the settings popup's Clear button via PlayLayer::get(), so
    // this needs to actually be reachable from outside the class.
    void clearMarkers() {
        for (auto marker : m_fields->markers) {
            if (marker) marker->removeFromParent();
        }
        m_fields->markers.clear();
    }
};

// Opens GD's own real color wheel (the same CCControlColourPicker used
// under the hood by things like the object color picker), rather than a
// fixed set of presets.
class MarkerColorPickerPopup : public geode::Popup, public cocos2d::extension::ColorPickerDelegate {
protected:
    cocos2d::extension::CCControlColourPicker* m_picker = nullptr;

    bool init(float width, float height) {
        if (!Popup::init(width, height)) return false;

        setID("death-stamp-color-picker-popup"_spr);
        setTitle("Marker Color");

        m_picker = cocos2d::extension::CCControlColourPicker::create();
        m_picker->setColorValue(readMarkerColor());
        m_picker->setDelegate(this);
        m_picker->setPosition(m_mainLayer->getContentSize() * 0.5f);
        m_mainLayer->addChild(m_picker);

        return true;
    }

    void colorValueChanged(ccColor3B color) override {
        writeMarkerColor(color);
    }

public:
    static MarkerColorPickerPopup* create() {
        auto ret = new MarkerColorPickerPopup();
        if (ret && ret->init(300.f, 300.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// Settings popup opened from the pause-menu button. Everything here reads
// straight from the persisted mod settings and writes straight back to them
// on every click/change — no separate widget-side state to keep in sync,
// since chasing that (CCMenuItemToggler's internal toggled state specifically)
// is what caused the pause-button bugs earlier in this mod's history.
// update() keeps refreshing continuously so the color swatch stays in sync
// while the color-picker sub-popup is open on top of this one.
class DeathStampSettingsPopup : public geode::Popup {
protected:
    CCMenuItemSpriteExtra* m_enabledCheckbox = nullptr;
    CCLabelBMFont* m_styleValueLabel = nullptr;
    CCScale9Sprite* m_colorSwatch = nullptr;
    Slider* m_opacitySlider = nullptr;
    CCLabelBMFont* m_opacityLabel = nullptr;
    geode::TextInput* m_maxInput = nullptr;
    CCMenu* m_menu = nullptr;
    int m_styleIndex = 0;

    bool init(float width, float height) {
        if (!Popup::init(width, height)) return false;

        setID("death-stamp-settings-popup"_spr);
        setTitle("Death Stamp Settings");

        auto style = Mod::get()->getSettingValue<std::string>("marker-style");
        for (int i = 0; i < kMarkerStyleCount; i++) {
            if (style == kMarkerStyleValues[i]) m_styleIndex = i;
        }

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
        m_enabledCheckbox->setPosition({labelX + 8.f, 155.f});
        m_menu->addChild(m_enabledCheckbox);

        auto* enabledLabel = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLabel->setAnchorPoint({0.f, 0.5f});
        enabledLabel->setPosition({labelX + 24.f, 155.f});
        enabledLabel->setScale(labelScale);
        root->addChild(enabledLabel);

        // Row 2: marker style — left/right cycle instead of one button per
        // option, so adding more shapes later doesn't need a wider popup.
        auto* styleLabel = CCLabelBMFont::create("Marker Shape", "bigFont.fnt");
        styleLabel->setAnchorPoint({0.f, 0.5f});
        styleLabel->setPosition({labelX, 115.f});
        styleLabel->setScale(labelScale);
        root->addChild(styleLabel);

        m_menu->addChild(makeArrow_(false, 115.f, menu_selector(DeathStampSettingsPopup::onStylePrev)));
        m_menu->addChild(makeArrow_(true, 115.f, menu_selector(DeathStampSettingsPopup::onStyleNext)));

        m_styleValueLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_styleValueLabel->setPosition({70.f, 115.f});
        m_styleValueLabel->setScale(0.5f);
        root->addChild(m_styleValueLabel);

        // Row 3: marker color — opens GD's real color wheel instead of
        // cycling two presets.
        auto* colorLabel = CCLabelBMFont::create("Marker Color (X/O only)", "bigFont.fnt");
        colorLabel->setAnchorPoint({0.f, 0.5f});
        colorLabel->setPosition({labelX, 75.f});
        colorLabel->setScale(labelScale);
        root->addChild(colorLabel);

        auto* swatchBg = CCScale9Sprite::create("square02b_001.png");
        swatchBg->setContentSize({34.f, 34.f});
        m_colorSwatch = swatchBg;
        auto* swatchBtn = CCMenuItemSpriteExtra::create(
            swatchBg, this, menu_selector(DeathStampSettingsPopup::onOpenColorPicker)
        );
        swatchBtn->setPosition({70.f, 75.f});
        m_menu->addChild(swatchBtn);

        // Row 4: opacity — the slider needs real clearance below its label,
        // or it visually bleeds into whatever row is above it.
        auto* opacityLabel = CCLabelBMFont::create("Opacity", "bigFont.fnt");
        opacityLabel->setAnchorPoint({0.f, 0.5f});
        opacityLabel->setPosition({labelX, 30.f});
        opacityLabel->setScale(labelScale);
        root->addChild(opacityLabel);

        m_opacitySlider = Slider::create(
            this, menu_selector(DeathStampSettingsPopup::onOpacitySlider), 0.6f
        );
        m_opacitySlider->setPosition({-25.f, -10.f});
        m_opacitySlider->setScale(0.6f);
        m_menu->addChild(m_opacitySlider);

        m_opacityLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_opacityLabel->setPosition({108.f, 30.f});
        m_opacityLabel->setScale(0.35f);
        root->addChild(m_opacityLabel);

        // Row 5: max markers
        auto* maxLabel = CCLabelBMFont::create("Max Markers (0 = all)", "bigFont.fnt");
        maxLabel->setAnchorPoint({0.f, 0.5f});
        maxLabel->setPosition({labelX, -70.f});
        maxLabel->setScale(labelScale);
        root->addChild(maxLabel);

        m_maxInput = geode::TextInput::create(60.f, "0");
        m_maxInput->setCommonFilter(geode::CommonFilter::Uint);
        m_maxInput->setMaxCharCount(3);
        m_maxInput->setPosition({105.f, -70.f});
        m_maxInput->setScale(0.85f);
        m_maxInput->setString(std::to_string(Mod::get()->getSettingValue<int64_t>("max-markers")));
        m_maxInput->setCallback([](std::string const& text) {
            int64_t value = text.empty() ? 0 : std::stoll(text);
            if (value < 0) value = 0;
            if (value > 999) value = 999;
            Mod::get()->setSettingValue<int64_t>("max-markers", value);
        });
        root->addChild(m_maxInput);

        // Row 6: clear all markers right now, on demand, instead of an
        // automatic "clear every retry" setting.
        auto* clearSprite = ButtonSprite::create("Clear All Markers", "bigFont.fnt", "GJ_button_06.png", 0.8f);
        clearSprite->setScale(0.7f);
        auto* clearBtn = CCMenuItemSpriteExtra::create(
            clearSprite, this, menu_selector(DeathStampSettingsPopup::onClearMarkers)
        );
        clearBtn->setPosition({0.f, -140.f});
        m_menu->addChild(clearBtn);

        refresh_();
        // update() only fires if the node is actually scheduled for it —
        // needed here so the color swatch keeps syncing while the color
        // picker sub-popup is open on top of this one.
        scheduleUpdate();
        return true;
    }

    void update(float) override {
        refresh_();
    }

    CCMenuItemSpriteExtra* makeArrow_(bool isRight, float y, SEL_MenuHandler sel) {
        auto* sprite = ButtonSprite::create(isRight ? ">" : "<", "bigFont.fnt", "GJ_button_04.png", 0.9f);
        sprite->setScale(0.5f);
        auto* item = CCMenuItemSpriteExtra::create(sprite, this, sel);
        item->setPosition({isRight ? 100.f : 40.f, y});
        return item;
    }

    void refresh_() {
        bool enabled = Mod::get()->getSettingValue<bool>("enabled");
        int opacity = Mod::get()->getSettingValue<int64_t>("marker-opacity");

        m_enabledCheckbox->setSprite(checkSprite_(enabled));
        m_styleValueLabel->setString(kMarkerStyleLabels[m_styleIndex]);
        m_colorSwatch->setColor(readMarkerColor());

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

    void onToggleEnabled(CCObject*) {
        bool newValue = !Mod::get()->getSettingValue<bool>("enabled");
        Mod::get()->setSettingValue<bool>("enabled", newValue);
        refresh_();
    }

    void onStylePrev(CCObject*) {
        m_styleIndex = (m_styleIndex - 1 + kMarkerStyleCount) % kMarkerStyleCount;
        Mod::get()->setSettingValue<std::string>("marker-style", kMarkerStyleValues[m_styleIndex]);
        refresh_();
    }

    void onStyleNext(CCObject*) {
        m_styleIndex = (m_styleIndex + 1) % kMarkerStyleCount;
        Mod::get()->setSettingValue<std::string>("marker-style", kMarkerStyleValues[m_styleIndex]);
        refresh_();
    }

    void onOpenColorPicker(CCObject*) {
        if (auto* popup = MarkerColorPickerPopup::create()) {
            popup->show();
        }
    }

    void onOpacitySlider(CCObject*) {
        int opacity = 20 + static_cast<int>(std::lround(
            std::clamp(m_opacitySlider->getValue(), 0.f, 1.f) * (255.f - 20.f)
        ));
        Mod::get()->setSettingValue<int64_t>("marker-opacity", opacity);
        m_opacityLabel->setString(fmt::format("{}/255", opacity).c_str());
    }

    void onClearMarkers(CCObject*) {
        if (auto* pl = PlayLayer::get()) {
            static_cast<DeathStampPlayLayer*>(pl)->clearMarkers();
        }
    }

public:
    static DeathStampSettingsPopup* create() {
        auto ret = new DeathStampSettingsPopup();
        if (ret && ret->init(340.f, 420.f)) {
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
