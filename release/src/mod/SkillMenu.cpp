#include "mod/SkillMenu.h"
#include "mod/AlphaHud.h"
#include "mod/ClassData.h"
#include "mod/Progress.h"
#include "cube/Controls.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/Game.h"
#include "cube/Interface.h"
#include "cube/ServerUpdateIds.h"
#include "cube/World.h"
#include "cube/widgets/ChatWidget.h"
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/widgets/InventoryWidget.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Texture.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace mod {

namespace {

const float MinimumPanelWidth = 300.0f;
const float MinimumPanelHeight = 560.0f; // the alpha's 460 and room for the hovered skill's text
const float ClassMasterRangeSquared = 16.0f;

// The specializations' names and icons (alpha/icons), by class and specialization.
const char *specializationIcon(int classType, int specialization)
{
    switch (classType) {
    case cube::ClassWarrior:
        return specialization ? "warrior-guardian" : "warrior-berserker";
    case cube::ClassRanger:
        return specialization ? "ranger-scout" : "ranger-sniper";
    case cube::ClassMage:
        return specialization ? "mage-water" : "mage-fire";
    case cube::ClassRogue:
        return specialization ? "rogue-ninja" : "rogue-assassin";
    default:
        return "";
    }
}

const wchar_t *specializationName(int classType, int specialization)
{
    switch (classType) {
    case cube::ClassWarrior:
        return specialization ? L"Guardian" : L"Berserker";
    case cube::ClassRanger:
        return specialization ? L"Scout" : L"Sniper";
    case cube::ClassMage:
        return specialization ? L"Water Mage" : L"Fire Mage";
    case cube::ClassRogue:
        return specialization ? L"Ninja" : L"Assassin";
    default:
        return L"";
    }
}

std::wstring widen(const std::string &text)
{
    return std::wstring(text.begin(), text.end());
}

// A value of a skill for the info text: "Cooldown: 15.3 s" and the like.
std::wstring describeValue(const std::string &name, float value)
{
    std::wstringstream text;
    text << std::fixed << std::setprecision(1);
    if (name == "cooldown")
        text << L"Cooldown: " << value / 1000.0f << L" s";
    else if (name == "duration")
        text << L"Duration: " << value / 1000.0f << L" s";
    else if (name == "mana")
        text << L"Mana: " << (int)std::lround(value * 100.0f);
    else
        text << widen(name) << L": " << std::setprecision(2) << value;
    return text.str();
}

void setFrame(plasma::Node *button, bool hovered)
{
    if (plasma::Node *frame = button ? button->findNode(L"frame") : nullptr)
        frame->m_display->m_fillColor =
            hovered ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f) : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
}

}

SkillMenu::SkillMenu(cube::Game *game, AlphaHud *hud, plasma::Node *parent, plasma::Node *gui)
    : cube::BaseWidget(game->m_engine, game->m_engine->createNode(nullptr, nullptr, nullptr, nullptr), L""),
      m_game(game),
      m_hud(hud),
      m_parent(parent)
{
    setFont(L"resource1.dat");
    plasma::Node *panelTemplate = gui->findNode(L"blackwidget");
    m_skillTemplate = gui->findNode(L"skillbutton");
    m_childSkillTemplate = gui->findNode(L"childskillbutton");
    plasma::Node *specializationTemplate = gui->findNode(L"specializationbutton");
    if (!panelTemplate || !m_skillTemplate || !m_childSkillTemplate || !specializationTemplate)
        return;
    m_panel = panelTemplate->instance(parent);
    m_panel->setTransformation(m_panel->m_transformation->clone());
    m_panel->setDisplay(m_panel->m_display->clone());
    m_panel->moveIntoWidget(panelTemplate->getName(), this, true);
    m_panel->setVisible(false);
    for (int i = 0; i < 2; i++) {
        m_specializationButtons[i] = specializationTemplate->clone(parent);
        m_specializationButtons[i]->setVisible(false);
    }
}

bool SkillMenu::isOpen() const
{
    return m_panel && m_panel->isVisible();
}

void SkillMenu::toggle()
{
    if (!m_panel)
        return;
    if (isOpen()) {
        close();
        return;
    }
    cube::Creature *player = m_game->m_world->m_localCreature;
    m_points = player->m_playerData->modProgress.points;
    m_specialization = player->m_entityData.specialization;
    m_panel->setVisible(true);
    m_game->playSound(cube::SoundMenuSelect);
}

void SkillMenu::close()
{
    if (!m_panel)
        return;
    m_panel->setVisible(false);
    for (plasma::Node *button : m_specializationButtons)
        button->setVisible(false);
    for (Entry &entry : m_entries)
        entry.button->setVisible(false);
}

// The buttons of the class tree shown (the class kit of the specialization shown) and of the general skills: the
// first of a chain from "skillbutton", the skills needing another from "childskillbutton".
void SkillMenu::rebuild()
{
    for (Entry &entry : m_entries)
        m_game->m_engine->destroyNode(entry.button);
    m_entries.clear();
    cube::Creature *player = m_game->m_world->m_localCreature;
    std::shared_ptr<const ClassData> data = classData();
    std::vector<TreeEntry> tree;
    if (const ClassKit *kit = data->kit(player->m_entityData.classType, m_specialization))
        tree = kit->tree;
    tree.insert(tree.end(), data->generalTree.begin(), data->generalTree.end());
    plasma::Node *parent = m_parent;
    for (const TreeEntry &item : tree) {
        Entry entry;
        entry.skill = item.skill;
        entry.requires = item.requires;
        entry.requiredPoints = item.requiredPoints;
        entry.button = (item.requires.empty() ? m_skillTemplate : m_childSkillTemplate)->clone(parent);
        m_entries.push_back(entry);
    }
    m_game->m_engine->root()->initializeWidgets();
    m_builtClass = player->m_entityData.classType;
    m_builtSpecialization = m_specialization;
}

int SkillMenu::pending(const std::string &skill) const
{
    std::map<std::string, int>::const_iterator it = m_points.find(skill);
    return it == m_points.end() ? 0 : it->second;
}

bool SkillMenu::changed() const
{
    cube::Creature *player = m_game->m_world->m_localCreature;
    const Progress &progress = player->m_playerData->modProgress;
    if (m_specialization != player->m_entityData.specialization)
        return true;
    for (const std::pair<const std::string, int> &entry : m_points) {
        if (entry.second != progress.pointsOf(entry.first))
            return true;
    }
    for (const std::pair<const std::string, int> &entry : progress.points) {
        if (entry.second != pending(entry.first))
            return true;
    }
    return false;
}

// Gold: refundCost per point given back and specializationCost for a new specialization, times the level.
int SkillMenu::cost() const
{
    cube::Creature *player = m_game->m_world->m_localCreature;
    const Progression &progression = classData()->progression;
    int level = player->m_playerData->modProgress.level;
    int refunded = 0;
    for (const std::pair<const std::string, int> &entry : player->m_playerData->modProgress.points)
        refunded += (std::max)(0, entry.second - pending(entry.first));
    int total = refunded * progression.refundCost * level;
    if (m_specialization != player->m_entityData.specialization && level > 1)
        total += progression.specializationCost * level;
    return total;
}

bool SkillMenu::trainerNearby() const
{
    cube::Creature *player = m_game->m_world->m_localCreature;
    for (cube::Creature *creature : m_game->m_world->m_creatures) {
        if (!creature || !(creature->m_entityData.appearance.appearanceFlags & cube::AppearanceFlagClassMaster) ||
            creature->m_entityData.classType != player->m_entityData.classType)
            continue;
        plasma::Vector<3, float> offset(player->m_entityData.position - creature->m_entityData.position);
        if (offset.squaredLength() < ClassMasterRangeSquared)
            return true;
    }
    return false;
}

bool SkillMenu::canLearn() const
{
    int price = cost();
    if (price <= 0)
        return true;
    cube::Creature *player = m_game->m_world->m_localCreature;
    if (price > player->m_inventory.m_gold)
        return false;
    return !classData()->progression.requireTrainer || trainerNearby();
}

bool SkillMenu::pointerOverLearn()
{
    plasma::Vector<2, float> pointer = getPointerOffset();
    return changed() && pointer[1] >= getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] >= 0.0f && pointer[0] < getContentWidth() * 0.5f;
}

bool SkillMenu::pointerOverCancel()
{
    plasma::Vector<2, float> pointer = getPointerOffset();
    return changed() && pointer[1] >= getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] >= getContentWidth() * 0.5f && pointer[0] < getContentWidth();
}

void SkillMenu::update()
{
    cube::Creature *player = m_game->m_world->m_localCreature;
    bool on = m_panel && alphaClasses() && player && player->m_playerData;
    bool keyDown = m_game->m_controlStates[cube::Controls::ButtonSkillMenu];
    cube::Interface::InGame &inGame = m_game->m_interface.m_inGame;
    bool typing = inGame.m_chatWidget->m_typeboxActive;
    if (on && keyDown && !m_keyWasDown && !typing) {
        toggle();
        // The crafting inventory opens in the same place.
        if (isOpen()) {
            inGame.m_craftingInventoryWidget->getNode()->setVisible(false);
            inGame.m_formulaDetailsWidget->getNode()->setVisible(false);
        }
    }
    m_keyWasDown = keyDown;
    // Walking closes it, as it closes the release's panels (Game::update); so does the crafting inventory.
    bool walking = !typing && (m_game->m_controlStates[cube::Controls::ButtonUp] ||
                               m_game->m_controlStates[cube::Controls::ButtonDown] ||
                               m_game->m_controlStates[cube::Controls::ButtonLeft] ||
                               m_game->m_controlStates[cube::Controls::ButtonRight] ||
                               m_game->m_gamepadWalkVector.squaredLength() > 0.1f * 0.1f);
    if (walking || inGame.m_craftingInventoryWidget->getNode()->isVisible())
        on = false;
    if (!on || !isOpen()) {
        if (isOpen())
            close();
        return;
    }
    if (m_builtClass != player->m_entityData.classType || m_builtSpecialization != m_specialization)
        rebuild();

    // The panel in the crafting inventory's place and size (Interface::updateResolution): left of the equipment
    // inventory, from its top to the bottom of the formula details; the widgets' view sizes are in screen pixels. The
    // specializations at the top, then the class's skills in a row of four and the general skills in rows of two, each
    // beside the one it needs (the alpha's layout), centred.
    float scale = m_game->m_interface.m_scale;
    float width = (std::max)(inGame.m_craftingInventoryWidget->viewWidth() / scale, MinimumPanelWidth);
    float x = ((float)(m_game->m_width / 2) - inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f) / scale -
              10.0f - width;
    float y = ((float)(m_game->m_height / 2) - inGame.m_equipmentInventoryWidget->getViewHeight()) / scale - 50.0f;
    float bottom = ((float)(m_game->m_height / 2) + inGame.m_formulaDetailsWidget->getViewHeight()) / scale - 40.0f;
    float height = (std::max)(bottom - y, MinimumPanelHeight);
    if (std::fabs(width - m_panelWidth) > 0.5f || std::fabs(height - m_panelHeight) > 0.5f || scale != m_panelScale) {
        m_panelWidth = width;
        m_panelHeight = height;
        m_panelScale = scale;
        m_panel->widget()->resizeTo(width * scale, height * scale, true);
    }
    m_panel->setPosition(0.0f, 0.0f, x, y);
    float left = x + (width - 4 * 55.0f) * 0.5f + 5.0f;
    float top = y + 70.0f;
    m_hovered.clear();
    for (int i = 0; i < 2; i++) {
        plasma::Node *button = m_specializationButtons[i];
        button->setVisible(true);
        button->setPosition(0.0f, 0.0f, x + width * 0.5f + (i - 1) * 70.0f + 5.0f, top);
        bool hovered = button->widget() && button->widget()->isPointerOver();
        setFrame(button, hovered);
        m_hud->setIcon(button, specializationIcon(player->m_entityData.classType, i), i == m_specialization);
        if (hovered)
            m_hovered = i ? "#specialization1" : "#specialization0";
    }
    std::shared_ptr<const ClassData> data = classData();
    int column = 0;
    float row = top + 80.0f;
    bool general = false;
    for (Entry &entry : m_entries) {
        // The general skills start a new row after the class's; a chain's first skill starts a row there.
        bool classEntry = &entry - &m_entries[0] < (int)m_entries.size() - (int)data->generalTree.size();
        if (!classEntry && !general) {
            general = true;
            column = 0;
            row += 60.0f;
        } else if (general && entry.requires.empty() && column > 0) {
            column = 0;
            row += 60.0f;
        } else if (classEntry && column == 4) {
            column = 0;
            row += 60.0f;
        }
        entry.button->setVisible(true);
        entry.button->setPosition(0.0f, 0.0f, left + column * 55.0f, row);
        column++;
        const Skill *skill = data->skill(entry.skill);
        bool locked = !entry.requires.empty() && pending(entry.requires) < entry.requiredPoints;
        entry.button->setText(std::to_wstring(pending(entry.skill)));
        m_hud->setIcon(entry.button, skill ? skill->icon : "", !locked);
        bool hovered = entry.button->widget() && entry.button->widget()->isPointerOver();
        setFrame(entry.button, hovered);
        if (hovered)
            m_hovered = entry.skill;
    }
}

bool SkillMenu::handleButtonDown(int button)
{
    if (!isOpen())
        return false;
    cube::Creature *player = m_game->m_world->m_localCreature;
    if (button == 0) {
        for (int i = 0; i < 2; i++) {
            if (m_specializationButtons[i]->widget() && m_specializationButtons[i]->widget()->isPointerOver()) {
                m_specialization = i;
                m_game->playSound(cube::SoundMenuSelect);
                return true;
            }
        }
        if (pointerOverLearn() && canLearn()) {
            Progress &progress = player->m_playerData->modProgress;
            player->m_inventory.m_gold -= (std::max)(0, cost());
            // A new specialization takes the points of the old one's skills in the same places of its tree.
            std::shared_ptr<const ClassData> data = classData();
            const ClassKit *from = data->kit(player->m_entityData.classType, player->m_entityData.specialization);
            const ClassKit *to = data->kit(player->m_entityData.classType, m_specialization);
            if (from && to && from != to) {
                for (size_t i = 0; i < from->tree.size() && i < to->tree.size(); i++) {
                    const std::string &old = from->tree[i].skill;
                    const std::string &now = to->tree[i].skill;
                    if (old != now && pending(now) == 0) {
                        m_points[now] = pending(old);
                        m_points.erase(old);
                    }
                }
            }
            progress.points.clear();
            for (const std::pair<const std::string, int> &entry : m_points) {
                if (entry.second > 0)
                    progress.points[entry.first] = entry.second;
            }
            m_points = progress.points;
            player->m_entityData.specialization = (unsigned char)m_specialization;
            m_game->playSound(cube::SoundSkillUp);
            return true;
        }
        if (pointerOverCancel()) {
            m_points = player->m_playerData->modProgress.points;
            m_specialization = player->m_entityData.specialization;
            m_game->playSound(cube::SoundMenuDropItem);
            return true;
        }
    }
    for (Entry &entry : m_entries) {
        if (!entry.button->widget() || !entry.button->widget()->isPointerOver())
            continue;
        std::shared_ptr<const ClassData> data = classData();
        const Skill *skill = data->skill(entry.skill);
        int level = pending(entry.skill);
        if (button == 0) {
            int spent = 0;
            for (const std::pair<const std::string, int> &points : m_points)
                spent += points.second;
            bool unlocked = entry.requires.empty() || pending(entry.requires) >= entry.requiredPoints;
            bool belowMaximum = !skill || skill->maxPoints <= 0 || level < skill->maxPoints;
            if (spent < totalSkillPoints(player) && unlocked && belowMaximum) {
                m_points[entry.skill] = level + 1;
                m_game->playSound(cube::SoundMenuSelect);
            }
        } else if (level > 0) {
            // A skill another needs keeps the points that one needs.
            int keep = 0;
            for (const Entry &other : m_entries) {
                if (other.requires == entry.skill && pending(other.skill) > 0)
                    keep = (std::max)(keep, other.requiredPoints);
            }
            if (level > keep) {
                m_points[entry.skill] = level - 1;
                m_game->playSound(cube::SoundMenuDropItem);
            }
        }
        return true;
    }
    // A click on the panel itself stays in the menu.
    return m_panel->widget() && isPointerOver();
}

// The texts, in the alpha's places: the title, the points spent of all there are, the hovered skill, the price of the
// changes with what prevents them, and Learn and Cancel.
std::vector<std::wstring> SkillMenu::wrap(const std::wstring &text, float width)
{
    std::vector<std::wstring> lines;
    std::wstringstream words(text);
    std::wstring word;
    std::wstring line;
    while (words >> word) {
        std::wstring longer = line.empty() ? word : line + L" " + word;
        plasma::Vector<2, float> minimum, maximum;
        measureText(longer, minimum, maximum);
        if (!line.empty() && maximum[0] - minimum[0] > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = longer;
        }
    }
    if (!line.empty())
        lines.push_back(line);
    return lines;
}

void SkillMenu::draw()
{
    if (!isOpen())
        return;
    cube::Creature *player = m_game->m_world->m_localCreature;
    plasma::Vector<4, float> white(1.0f, 1.0f, 1.0f, 1.0f);
    plasma::Vector<4, float> black(0.0f, 0.0f, 0.0f, 1.0f);
    setTextStrokeColor(black);
    setTextFlags(0);
    setTextColor(white);
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    renderText(L"Skills", 15.0f, 25.0f);
    int spent = 0;
    for (const std::pair<const std::string, int> &entry : m_points)
        spent += entry.second;
    setTextSize(10.0f);
    setTextStrokeRadius(2.0f);
    renderText(L"Points: " + std::to_wstring(spent) + L"/" + std::to_wstring(totalSkillPoints(player)), 15.0f, 45.0f);

    // The hovered skill: its name and points, what it does and its values at those points and the next.
    std::shared_ptr<const ClassData> data = classData();
    float y = 410.0f;
    if (m_hovered == "#specialization0" || m_hovered == "#specialization1") {
        int specialization = m_hovered.back() - '0';
        setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.3f, 1.0f));
        renderText(specializationName(player->m_entityData.classType, specialization), 15.0f, y);
    } else if (const Skill *skill = data->skill(m_hovered)) {
        int points = pending(m_hovered);
        setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.3f, 1.0f));
        renderText(widen(skill->name) + L" (" + std::to_wstring(points) + L")", 15.0f, y);
        setTextColor(white);
        y += 2.0f;
        for (const std::wstring &line : wrap(widen(skill->description), m_panelWidth - 30.0f)) {
            y += 14.0f;
            renderText(line, 15.0f, y);
        }
        for (const std::pair<const std::string, Value> &value : skill->values) {
            if (value.second.perBonus == 0.0f && value.second.base == 0.0f)
                continue;
            std::wstring line = describeValue(value.first, value.second.at(points));
            if (value.second.perBonus != 0.0f)
                line += L"  (next " + describeValue(value.first, value.second.at(points + 1)).substr(line.find(L':') + 2) +
                        L")";
            y += 14.0f;
            renderText(line, 15.0f, y);
        }
    }

    if (!changed())
        return;
    int price = cost();
    float bottom = getContentHeight();
    if (price > 0) {
        setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
        if (price > player->m_inventory.m_gold)
            renderText(L"Not enough money.", 15.0f, bottom - 65.0f);
        else if (classData()->progression.requireTrainer && !trainerNearby())
            renderText(L"Requires class trainer.", 15.0f, bottom - 65.0f);
        setTextColor(white);
        renderText(L"Cost: " + std::to_wstring(price) + L" gold", 15.0f, bottom - 45.0f);
    }
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    bool learnable = canLearn();
    setTextColor(!learnable ? plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f)
                            : (pointerOverLearn() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f) : white));
    renderText(L"Learn", getContentWidth() / 3.0f, bottom - 20.0f);
    setTextColor(pointerOverCancel() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f) : white);
    renderText(L"Cancel", getContentWidth() * 2.0f / 3.0f, bottom - 20.0f);
}

}
