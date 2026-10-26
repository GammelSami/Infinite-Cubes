// See cube/PreviewWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <algorithm>
#include <list>
#include <sstream>
#include <string>
#include <vector>
#include "cube/widgets/PreviewWidget.h"
#include "cube/Realm.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/Speech.h"
#include "cube/World.h"
#include "cube/WorldMap.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "cube/CreatureIds.h"
#include "plasma/EditFilter.h"

namespace cube {

PreviewWidget::PreviewWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_zoneItem(nullptr),
      m_item(nullptr),
      m_count(1),
      m_unusedItemInfoFlag(false),
      m_skill(-1),
      m_skillLevel(0),
      m_specialization(-1),
      m_equippedItem(nullptr),
      m_flightPoint(0x7fffffff, 0x7fffffff),
      m_artifactRegion(0x7fffffff, 0x7fffffff),
      m_game(game)
{
    setFont(L"resource1.dat");
    setTextLineSpacing(4.0f);
}

void PreviewWidget::draw()
{
    // The rarity stars are shown only by the item details.
    m_node->findNode(L"star1")->display()->m_visibility.value() = 0;
    m_node->findNode(L"star2")->display()->m_visibility.value() = 0;
    m_node->findNode(L"star3")->display()->m_visibility.value() = 0;
    m_node->findNode(L"star4")->display()->m_visibility.value() = 0;

    if (m_flightPoint[0] != 0x7fffffff) {
        renderFlightPoint();
    } else if (m_artifactRegion[0] != 0x7fffffff) {
        renderArtifact();
    } else {
        plasma::Matrix<float> matrix;
        m_node->getMatrix(matrix);
        float scale = matrix.m_elements[0];
        if (!m_item) {
            if (m_skill >= 0) {
                renderSkill(m_skill, m_skillLevel, 14, 25, 1.0f, 275);
            } else if (m_specialization >= 0) {
                renderSpecialization(m_game->m_world->m_localCreature->m_entityData.classType, 14, 25, 1.0f, 275);
            }
        } else {
            // The item turns at the right of the box, its details at the left.
            Item item = m_item->transformed(m_game->m_world->m_localCreature);
            plasma::Vector<2, float> position = matrix * m_node->transformation()->m_pivot.value();
            m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
            m_game->m_renderer->drawItem(
                &item, viewWidth() * 0.75f + position[0], viewWidth() * 0.3f + position[1],
                plasma::Vector<3, float>(225.0f, 0.0f, (float)(m_engine->getCaretClock() % 10000) * 0.0001f * 360.0f),
                scale * 0.075f, -0.05f, true);
            m_engine->applyRenderState();
            plasma::Vector<2, float> size = m_game->writeItemInfo(this, &item, m_count, 14, 25, 1.0f, 260, true, true,
                                                                  false, m_zoneItem, m_equippedItem);
            resizeScaled(getContentWidth(), (std::max)(200.0f, size[1]), true);
            if (m_equippedItem) {
                std::wstring text(L"Currently equipped:");
                setTextSize(12.0f);
                setTextStrokeRadius(2.0f);
                setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                size[1] += 20.0f;
                renderText(text, 14.0f, size[1]);
                size[1] += 20.0f;
                size = m_game->writeItemInfo(this, m_equippedItem, 1, 14, (int)size[1], 0.75f, 260, true, false,
                                             m_unusedItemInfoFlag, nullptr, nullptr);
                resizeScaled(getContentWidth(), (std::max)(200.0f, size[1]), true);
            }
        }
    }
}

// The distance in metres, rounded to two significant digits below 1000 m, in kilometres with one decimal below 10 km,
// else whole kilometres.
static std::wstring formatDistance(int meters)
{
    if (meters < 10)
        return std::to_wstring(meters) + L" m";
    if (meters < 500)
        return std::to_wstring(10 * (meters / 10)) + L" m";
    if (meters < 1000)
        return std::to_wstring(100 * (meters / 100)) + L" m";
    int kilometers = meters / 1000;
    int tenths;
    if (meters < 10000 && (tenths = meters / 100 - 10 * kilometers) > 0)
        return std::to_wstring(kilometers) + L"." + std::to_wstring(tenths) + L" km";
    return std::to_wstring(kilometers) + L" km";
}

void PreviewWidget::renderFlightPoint()
{
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextFlags(1 << plasma::TEXT_WRAP);
    setTextWrapWidth(getContentWidth() - 20.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextSize(15.0f);
    Creature *player = m_game->m_world->m_localCreature;
    if (!(m_flightPoint != player->m_entityData.portCity)) {
    }
    setTextSize(12.0f);
    setTextColor(plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f));
    setTextWrapWidth(getContentWidth() - 30.0f);
    float y = renderText(L"Flight point", 14.0f, 25.0f)[1];
    y += 30.0f;
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));

    player = m_game->m_world->m_localCreature;
    double dy = (double)((Fixed(m_flightPoint[1]))-player->m_entityData.position[1]);
    double dx = (double)((Fixed(m_flightPoint[0]))-player->m_entityData.position[0]);
    y = renderText(L"Distance: " + formatDistance((int)(sqrt(dx * dx + dy * dy) * 0.5)), 14.0f, y)[1];
    y += 30.0f;

    if (m_game->m_world->m_localCreature->knowsFlightPoint(m_flightPoint) ||
        m_game->m_worldMap->m_selectedFlightTargetIsCreature) {
        Creature *creature = m_game->m_world->m_localCreature;
        plasma::Vector<3, int> position((int)creature->m_entityData.position[0],
                                        (int)creature->m_entityData.position[1],
                                        (int)creature->m_entityData.position[2]);
        int time = m_game->m_world->flightDuration(plasma::Vector<2, int>(position[0], position[1]), m_flightPoint);
        // While already flying there on an eagle, from the take-off point.
        creature = m_game->m_world->m_localCreature;
        if (creature->m_entityData.currentAbility == AbilityEagleFlight &&
            !(creature->m_entityData.destination != m_flightPoint)) {
            plasma::Vector<3, int> start((int)creature->m_entityData.origin[0], (int)creature->m_entityData.origin[1],
                                         (int)creature->m_entityData.origin[2]);
            time = (int)(m_game->m_world->flightDuration(plasma::Vector<2, int>(start[0], start[1]), m_flightPoint) -
                         creature->m_entityData.timeSinceAbility);
        }
        y = renderText(L"Flight time: " + std::to_wstring(time / 1000) + L" seconds", 14.0f, y)[1];
        y += 30.0f;
    } else {
        int cost = m_game->m_world->flightPrice(m_flightPoint);
        if (cost <= m_game->m_world->m_localCreature->m_inventory.m_gold) {
            setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f));
            y = renderText(L"Purchase for: " + std::to_wstring(cost) + L" gold", 14.0f, y)[1];
        } else {
            setTextColor(plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
            y = renderText(L"Cost: " + std::to_wstring(cost) + L" gold", 14.0f, y)[1];
        }
    }
    resizeScaled(getContentWidth(), y + 30.0f, true);
}

// The skill's description, laid out word by word: 12 units between words, 3 before punctuation, none after "-",
// wrapping at x + width.
void PreviewWidget::renderSkill(int skill, int level, int x, int y, float scale, int width)
{
    std::vector<Dialog> dialogs;
    Speech::Context context;
    std::wstring key;
    key = m_game->m_speech.skillKey(skill);
    m_game->m_speech.generateDialogs(std::wstring(L"description:") + key, context, 0, dialogs, L"");
    if (level > 0) {
        m_game->m_speech.generateDialogs(L"skill:level", context, 0, dialogs, L"");
        m_game->m_speech.generateDialogs(std::wstring(L"details:") + key, context, 0, dialogs, L"");
    }
    if (!dialogs.empty()) {
        setTextStrokeRadius(3.0f);
        float size = 12.0f;
        setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
        setTextFlags(0);
        for (std::vector<Dialog>::iterator dialog = dialogs.begin(); dialog != dialogs.end(); ++dialog) {
            setTextSize(size);
            int wordX = x;
            bool first = true;
            for (std::list<Dialog::Phrase>::iterator phrase = dialog->phrases.begin(); phrase != dialog->phrases.end();
                 ++phrase) {
                plasma::Matrix<float> matrix;
                m_engine->getModelViewMatrix(matrix);
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                measureText(phrase->text, minimum, maximum);
                int wordWidth = (int)(maximum[0] - minimum[0]);
                bool punctuation = phrase->text == L"." || phrase->text == L"-" || phrase->text == L"," ||
                                   phrase->text == L";" || phrase->text == L"!" || phrase->text == L"?" ||
                                   phrase->text == L":" || phrase->text == L"/" || phrase->text == L")";
                if (!first) {
                    if (punctuation)
                        wordX += 3;
                    else
                        wordX += 12;
                }
                first = phrase->text == L"-";
                if (!punctuation && wordWidth + wordX > x + width) {
                    wordX = x;
                    y = (int)(size + 5.0f + y);
                }
                setTextColor(phrase->color);
                renderText(phrase->text, (float)wordX, (float)y);
                wordX += wordWidth;
                if (phrase->text == L"/" || phrase->text == L"(")
                    wordX -= 9;
            }
            y += 18;
            size = 10.0f;
        }
    }
    resizeScaled(getContentWidth(), (float)y, true);
}

void PreviewWidget::renderSpecialization(int classType, int x, int y, float scale, int width)
{
    std::vector<Dialog> dialogs;
    Speech::Context context;
    switch (classType) {
    case ClassWarrior:
        switch (m_specialization) {
        case SpecializationBerserker:
            m_game->m_speech.generateDialogs(L"description:SpecializationBerserker", context, 0, dialogs, L"");
            break;
        case SpecializationGuardian:
            m_game->m_speech.generateDialogs(L"description:SpecializationGuardian", context, 0, dialogs, L"");
            break;
        }
        break;
    case ClassRanger:
        switch (m_specialization) {
        case SpecializationScout:
            m_game->m_speech.generateDialogs(L"description:SpecializationScout", context, 0, dialogs, L"");
            break;
        case SpecializationSniper:
            m_game->m_speech.generateDialogs(L"description:SpecializationSniper", context, 0, dialogs, L"");
            break;
        }
        break;
    case ClassMage:
        switch (m_specialization) {
        case SpecializationFire:
            m_game->m_speech.generateDialogs(L"description:SpecializationFire", context, 0, dialogs, L"");
            break;
        case SpecializationWater:
            m_game->m_speech.generateDialogs(L"description:SpecializationWater", context, 0, dialogs, L"");
            break;
        }
        break;
    case ClassRogue:
        switch (m_specialization) {
        case SpecializationAssassin:
            m_game->m_speech.generateDialogs(L"description:SpecializationAssassin", context, 0, dialogs, L"");
            break;
        case SpecializationNinja:
            m_game->m_speech.generateDialogs(L"description:SpecializationNinja", context, 0, dialogs, L"");
            break;
        }
        break;
    }
    std::wstringstream stream;
    if (!dialogs.empty()) {
        setTextStrokeRadius(3.0f);
        float size = 12.0f;
        setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
        setTextFlags(0);
        for (std::vector<Dialog>::iterator dialog = dialogs.begin(); dialog != dialogs.end(); ++dialog) {
            setTextSize(size);
            int wordX = x;
            bool first = true;
            for (std::list<Dialog::Phrase>::iterator phrase = dialog->phrases.begin(); phrase != dialog->phrases.end();
                 ++phrase) {
                plasma::Matrix<float> matrix;
                m_engine->getModelViewMatrix(matrix);
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                measureText(phrase->text, minimum, maximum);
                int wordWidth = (int)(maximum[0] - minimum[0]);
                bool punctuation = phrase->text == L"." || phrase->text == L"-" || phrase->text == L"," ||
                                   phrase->text == L";" || phrase->text == L"!" || phrase->text == L"?" ||
                                   phrase->text == L":" || phrase->text == L"/" || phrase->text == L")";
                if (!first) {
                    if (punctuation)
                        wordX += 3;
                    else
                        wordX += 12;
                }
                first = phrase->text == L"-";
                if (!punctuation && wordWidth + wordX > x + width) {
                    wordX = x;
                    y = (int)(size + 5.0f + y);
                }
                setTextColor(phrase->color);
                renderText(phrase->text, (float)wordX, (float)y);
                wordX += wordWidth;
                if (phrase->text == L"/" || phrase->text == L"(")
                    wordX -= 9;
            }
            y += 18;
            size = 10.0f;
        }
    }
    resizeScaled(getContentWidth(), (float)y, true);
}

// A pseudo-random number in 1..100000 from a seed.
static inline int randomFromSeed(int seed)
{
    seed = (seed << 13) ^ seed;
    return 100000 - ((seed * (seed * seed * 60493 + 19990303) - 771171059) & 0x7fffffff) / 21474;
}

void PreviewWidget::renderArtifact()
{
    if (!m_game->m_world->hasLore(m_artifactRegion[0], m_artifactRegion[1]))
        return;
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextFlags(1 << plasma::TEXT_WRAP);
    setTextWrapWidth(getContentWidth() - 20.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    int regionY = floorDivide(m_artifactRegion[1] * BlocksPerCell, BlocksPerPlace);
    int regionX = floorDivide(m_artifactRegion[0] * BlocksPerCell, BlocksPerPlace);
    World *world = m_game->m_world;
    int realm = world->mortalRealm(regionX, regionY, world->isLargePlace(regionX, regionY));
    renderText(m_game->m_speech.realmName(Realm(realm).form(), realm), 14.0f, 25.0f);

    int lore = 10 * (int)m_game->m_world->m_localCreature->m_playerData->loreByRealm[realm].size();
    if (lore < 100)
        setTextColor(plasma::Vector<4, float>(0.75f, 0.75f, 0.75f, 1.0f));
    else
        setTextColor(plasma::Vector<4, float>(0.6f, 1.0f, 0.25f, 1.0f));
    renderText(L"Lore " + std::to_wstring(lore) + L"%", 14.0f, 49.0f);

    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    std::vector<std::string> script = m_game->m_world->storyScript(m_artifactRegion[0], m_artifactRegion[1]);
    std::vector<Dialog> dialogs;
    // The text alternatives are seeded by the zone.
    m_game->m_speech.generateFromScript(script, randomFromSeed(m_artifactRegion[1] * 57 + m_artifactRegion[0]), dialogs,
                                        L"");
    plasma::Vector<2, float> size = renderTextLines(dialogs, 14.0f, 73.0f, getContentWidth() - 30.0f);
    size[1] += 30.0f;
    resizeScaled(getContentWidth(), size[1], true);
}

}
