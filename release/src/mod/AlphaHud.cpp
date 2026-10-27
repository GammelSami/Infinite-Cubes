#include "mod/AlphaHud.h"
#include "mod/ClassData.h"
#include "mod/Progress.h"
#include "mod/SkillMenu.h"
#include "cube/Controls.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/Interface.h"
#include "cube/InterfaceIds.h"
#include "cube/ServerUpdateIds.h"
#include "cube/widgets/ChatWidget.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Node.h"
#include "plasma/PLXStreamReader.h"
#include "plasma/Texture.h"
#include "plasma/Transformation.h"
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <sstream>

namespace mod {

namespace {

// The ability bar's buttons, as the alpha numbered them: the mouse buttons, then the keys 1 to 4.
enum BarSlot { BarMouseLeft, BarMouseRight, BarKey1, BarKey2, BarKey3, BarKey4, BarSlotCount };

// The alpha's icon files of the game's abilities (GameController's constructor), by the release's numbering.
const std::pair<int, const char *> AbilityIcons[] = {
    {cube::AbilityTwoHandedSweep, "slam"},
    {cube::AbilityTwoHandedThrust, "slam-back"},
    {cube::AbilityTwoHandedSpin, "slam-left"},
    {cube::AbilityTwoHandedSlam, "slam-right"},
    {cube::AbilityLeftSlash, "attack-left"},
    {cube::AbilityRightSlash, "attack-right"},
    {cube::Ability_5, "stab-left"},
    {cube::Ability_4, "stab-right"},
    {cube::AbilityTwoHandedSwing, "jab"},
    {cube::AbilityCrush, "charge-slam"},
    {cube::AbilitySwirl, "swirl"},
    {cube::AbilityBlock, "block"},
    {cube::AbilityShieldAttack, "shield-attack"},
    {cube::AbilityBlockJab, "shield-slam"},
    {cube::AbilityLeftDaggerPierce, "pierce-left"},
    {cube::AbilityRightDaggerPierce, "pierce-right"},
    {cube::AbilityAmbush, "ambush"},
    {cube::AbilityLeftStab, "punch-left"},
    {cube::AbilityRightStab, "punch-right"},
    {cube::AbilityKick, "kick"},
    {cube::AbilityRangerKick, "ranger-kick"},
    {cube::AbilityLeftSweep, "slash-left"},
    {cube::AbilityRightSweep, "slash-right"},
    {cube::AbilityPerforate, "perforate"},
    {cube::AbilitySneak, "sneak"},
    {cube::AbilityAim, "aim"},
    {cube::AbilitySwiftness, "swiftness"},
    {cube::AbilityFirebolt, "firebolt"},
    {cube::AbilityBraceletFirebolt, "firebolt"},
    {cube::AbilityFireball, "fireball"},
    {cube::AbilityFireExplosion, "fire-explosion"},
    {cube::AbilityWaterbolt, "waterbolt"},
    {cube::AbilityBraceletWaterbolt, "waterbolt"},
    {cube::AbilityWaterball, "waterball"},
    {cube::AbilityWaterSalvo, "water-salvo"},
    {cube::AbilityHealingStream, "healing-stream"},
    {cube::AbilityFireSwirl, "fire-shock"},
    {cube::AbilityFireVortex, "fire-burst"},
    {cube::AbilityWaterSwirl, "water-shock"},
    {cube::AbilityWaterVortex, "water-burst"},
    {cube::AbilitySmash, "smash"},
    {cube::AbilityCyclone, "cyclone"},
    {cube::AbilityIntercept, "intercept"},
    {cube::AbilityTeleport, "teleport"},
    {cube::AbilityRetreat, "retreat2"},
    {cube::AbilityCrossedWeaponSweep, "slash"},
    {cube::AbilityShoot, "shoot"},
    {cube::AbilitySalvo, "salvo"},
    {cube::AbilityThrowBoomerang, "boomerang"},
    {cube::AbilityChargedBoomerang, "charge-boomerang"},
    {cube::AbilityChargedShot, "charge-shoot"},
    {cube::AbilityShuriken, "shuriken"},
    {cube::AbilityBulwark, "bulwark"},
    {cube::AbilityManaShield, "mana-shield"},
    {cube::AbilityCamouflage, "camouflage"},
    {cube::AbilityWarFrenzy, "warfrenzy"},
};

const wchar_t *className(int classType)
{
    switch (classType) {
    case cube::ClassWarrior:
        return L" Warrior";
    case cube::ClassRanger:
        return L" Ranger";
    case cube::ClassMage:
        return L" Mage";
    case cube::ClassRogue:
        return L" Rogue";
    default:
        return L"";
    }
}

plasma::SmoothMeshShape *meshShape(plasma::Node *node)
{
    return node ? dynamic_cast<plasma::SmoothMeshShape *>(node->m_shape) : nullptr;
}

}

AlphaHud::AlphaHud(cube::Interface *ui)
    : m_interface(ui)
{
    if (!std::filesystem::exists("alpha/gui.plx"))
        return;
    plasma::Engine *engine = ui->m_game->m_engine;
    std::vector<unsigned __int64> keys(1, plasma::keyFromPhrase("PlasmaXGraphics"));
    m_gui = engine->createNode(nullptr, nullptr, nullptr, nullptr);
    engine->readPlasmaFileIntoNode(L"alpha/gui.plx", m_gui, (1 << plasma::LOAD_HINTED_TEXT), nullptr, keys);
    plasma::Node *parent = ui->m_inGame.m_node;
    auto take = [&](const wchar_t *name) {
        plasma::Node *node = m_gui->findNode(name);
        if (node)
            node->setParent(parent);
        return node;
    };
    m_lifeBar = take(L"lifebar");
    m_experienceBar = take(L"experiencebar");
    m_manaCubeBar = take(L"manacubebar");
    m_hpBar = take(L"hpbar");
    m_mpBar = take(L"mpbar");
    plasma::Node *abilityButton = m_gui->findNode(L"abilitybutton");
    if (!m_lifeBar || !m_experienceBar || !m_hpBar || !m_mpBar || !abilityButton)
        return;
    m_lifeBarFill = m_lifeBar->findNode(L"bar");
    m_experienceBarFill = m_experienceBar->findNode(L"bar");
    m_hpBarFill = m_hpBar->findNode(L"bar");
    m_mpBarFill = m_mpBar->findNode(L"bar");
    m_chargeBar = m_mpBar->findNode(L"chargebar");
    for (int i = 0; i < BarSlotCount; i++) {
        plasma::Node *button = abilityButton->clone(parent);
        m_abilityButtons.push_back(button);
        m_cooldowns.push_back(button->findNode(L"cooldown"));
    }
    m_available = true;
    m_skillMenu = new SkillMenu(ui->m_game, this, parent, m_gui);
}

plasma::Texture *AlphaHud::icon(const std::string &name)
{
    std::map<std::string, plasma::Texture *>::iterator it = m_icons.find(name);
    if (it != m_icons.end())
        return it->second;
    plasma::TextureFormat format;
    format.minFilter = plasma::TEXTURE_FILTER_NEAREST;
    format.maxFilter = plasma::TEXTURE_FILTER_NEAREST;
    std::wstring path = L"alpha/icons/" + std::wstring(name.begin(), name.end()) + L".png";
    plasma::Texture *texture =
        name.empty() ? nullptr : m_interface->m_game->m_engine->loadImageTexture(path, format);
    m_icons[name] = texture;
    return texture;
}

std::string AlphaHud::abilityIcon(int ability)
{
    for (const std::pair<int, const char *> &entry : AbilityIcons) {
        if (entry.first == ability)
            return entry.second;
    }
    return "";
}

void AlphaHud::setIcon(plasma::Node *button, plasma::Texture *texture, bool usable)
{
    plasma::SmoothMeshShape *shape = meshShape(button);
    if (!shape)
        return;
    int id = texture ? texture->id() : (icon("none") ? icon("none")->id() : -1);
    float saturation = usable ? 1.0f : 0.0f;
    if (shape->m_texture.value() != id || shape->m_textureSaturation.value() != saturation) {
        shape->m_texture.value() = id;
        shape->m_textureSaturation.value() = saturation;
        shape->updateDrawingTextures();
    }
}

void AlphaHud::setFill(plasma::Node *fill, float fraction)
{
    if (!fill)
        return;
    fraction = (std::min)((std::max)(fraction, 0.0f), 1.0f);
    fill->m_transformation->m_deformation.value().identity();
    fill->m_transformation->m_deformation.value().scale(fraction, 1.0f);
    fill->m_transformation->attributesChanged(true);
}

bool AlphaHud::active() const
{
    return m_available && alphaClasses();
}

void AlphaHud::update()
{
    cube::Game *game = m_interface->m_game;
    cube::Creature *player = game->m_world->m_localCreature;
    bool on = m_available && alphaClasses() && player && player->m_playerData;
    cube::Interface::InGame &inGame = m_interface->m_inGame;
    for (plasma::Node *node : {m_lifeBar, m_experienceBar, m_manaCubeBar, m_hpBar, m_mpBar}) {
        if (node)
            node->setVisible(on);
    }
    for (plasma::Node *button : m_abilityButtons)
        button->setVisible(on);
    if (m_skillMenu)
        m_skillMenu->update();
    if (!on)
        return;
    // The release's bars and class skill give way.
    inGame.m_hpBar->setVisible(false);
    inGame.m_mpBar->setVisible(false);
    inGame.m_levelInfo->setVisible(false);
    inGame.m_levelText->setVisible(false);
    inGame.m_quickButtons[cube::QuickSlotSkill].button->setVisible(false);
    // The gold below its coin, clear of the life bar; the quick item (Q) closes the ability bar, as in the alpha.
    inGame.m_goldText->setPosition(0.0f, 0.0f, 40.0f, 100.0f);

    // Anchors in screen pixels, offsets in interface units (setPosition, as the release's layout does).
    float centre = (float)(game->m_width / 2);
    float bottom = (float)game->m_height;
    cube::Creature::EntityData &entity = player->m_entityData;

    // The life bar: "hp/max", the name, "LVL n Class" (the alpha's top left).
    std::wstringstream text;
    text << (int)entity.hp << L"/" << (int)player->maxHP();
    m_lifeBar->setText(L"text", text.str(), true);
    std::string name(entity.name, strnlen(entity.name, sizeof(entity.name)));
    m_lifeBar->setText(L"info", std::wstring(name.begin(), name.end()), true);
    text.str(L"");
    mod::Progress &progress = player->m_playerData->modProgress;
    text << L"LVL " << progress.level << className(entity.classType);
    m_lifeBar->setText(L"class", text.str(), true);
    m_lifeBar->setPosition(0.0f, 0.0f, 100.0f, 30.0f);
    setFill(m_lifeBarFill, entity.hp / player->maxHP());

    // The experience bar, and the mana cubes towards the next skill point.
    int next = maxExperience(progress.level);
    text.str(L"");
    text << progress.xp << L"/" << next;
    m_experienceBar->setText(L"text", text.str(), true);
    m_experienceBar->setPosition(0.0f, 0.0f, 100.0f, 75.0f);
    setFill(m_experienceBarFill, (float)progress.xp / (float)next);
    // A new level: the release's sound and a line in the chat, with the skill points it brings.
    while (progress.levelUps > 0) {
        progress.levelUps--;
        game->playSound(cube::SoundLevelUp);
        std::wstringstream message;
        message << L"You reached level " << progress.level << L"! Skill points: " << skillPoints(player) << L" (X)"
                << std::endl;
        inGame.m_chatWidget->printMessage(message.str(), plasma::Vector<3, unsigned char>(255, 220, 128));
    }
    if (m_manaCubeBar) {
        m_manaCubeBar->setPosition(0.0f, 0.0f, 100.0f, 130.0f);
        int perPoint = (std::max)(1, classData()->progression.manaCubesPerSkillPoint);
        int lit = player->m_inventory.m_manaCubes % perPoint;
        for (plasma::Node *cube : m_manaCubeBar->m_children) {
            cube->setVisible(lit > 0);
            lit--;
        }
    }

    // The hit point and mana bars left and right of the centre at the bottom.
    text.str(L"");
    text << (int)entity.hp << L"/" << (int)player->maxHP();
    m_hpBar->setText(L"text", text.str(), true);
    m_hpBar->setPosition(centre, bottom, -163.0f, -90.0f);
    setFill(m_hpBarFill, entity.hp / player->maxHP());
    text.str(L"");
    text << (int)(entity.mp * 100.0f + 0.5f) << L"/100";
    m_mpBar->setText(L"text", text.str(), true);
    m_mpBar->setPosition(centre, bottom, 8.0f, -90.0f);
    setFill(m_mpBarFill, entity.mp);
    setFill(m_chargeBar, entity.charge);

    // The ability bar: the mouse buttons' attacks, then the kit's skills (empty without points), 48 pixels apart.
    std::shared_ptr<const ClassData> data = classData();
    const ClassKit *kit = data->kit(entity.classType, entity.specialization);
    int abilities[BarSlotCount] = {player->nextAttackSkill(false), player->nextSpecialAttackSkill(false), 0, 0, 0, 0};
    std::string icons[BarSlotCount] = {abilityIcon(abilities[0]), abilityIcon(abilities[1])};
    for (int i = BarKey1; i < BarSlotCount && kit && i - BarKey1 < (int)kit->bar.size(); i++) {
        const Skill *skill = data->skill(kit->bar[i - BarKey1]);
        if (skill && player->m_playerData->modProgress.pointsOf(skill->id) > 0) {
            abilities[i] = skill->ability;
            icons[i] = skill->icon;
        }
    }
    static const int Controls[BarSlotCount] = {cube::Controls::ButtonAttack, cube::Controls::ButtonSpecialAttack,
                                               cube::Controls::ButtonSkill1, cube::Controls::ButtonSkill2,
                                               cube::Controls::ButtonSkill3, -1};
    inGame.m_quickButtons[cube::QuickSlotItem].button->setPosition(
        centre, bottom, (BarSlotCount * 2 - (BarSlotCount + 1)) * 24.0f + 6.0f, -50.0f);
    bool cursorShown = m_interface->isInMenu();
    for (int i = 0; i < BarSlotCount; i++) {
        plasma::Node *button = m_abilityButtons[i];
        button->setPosition(centre, bottom, (i * 2 - (BarSlotCount + 1)) * 24.0f + 6.0f, -50.0f);
        int control = Controls[i];
        button->setText(L"info", control < 0 ? L"4" : game->m_keyNames.getShortName(game->m_controls.buttons[control]),
                        true);
        int ability = abilities[i];
        setIcon(button, ability ? icon(icons[i]) : nullptr, ability && player->canUseSkill(ability, true));
        // The cooldown cover shrinks with the time left.
        if (plasma::Node *cooldown = m_cooldowns[i]) {
            float left = 0.0f;
            std::map<int, float>::iterator it = player->m_skillCooldowns.find(ability);
            int total = ability ? player->cooldown(ability) : 0;
            if (it != player->m_skillCooldowns.end() && total > 0)
                left = (std::max)(0.0f, it->second / (float)total);
            cooldown->setVisible(left > 0.0f);
            cooldown->m_transformation->m_deformation.value().identity();
            cooldown->m_transformation->m_deformation.value().scale(1.0f, (std::min)(left, 1.0f));
            cooldown->m_transformation->attributesChanged(true);
        }
        // The frame lights red while its input is held.
        if (plasma::Node *frame = button->findNode(L"frame")) {
            bool held = !cursorShown && control >= 0 && game->m_controlStates[control];
            frame->m_display->m_fillColor =
                held ? plasma::Vector<4, float>(1.0f, 0.0f, 0.2f, 1.0f) : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

}
