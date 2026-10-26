#include "cube/WorldDimensions.h"
// Speech generates dialogs from dictionary text and world context.
#include "cube/Speech.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"
#include "cube/BytesIO.h"
#include "cube/Item.h"
#include "cube/RaceIds.h"
#include <cstdlib>
#define PUGIXML_WCHAR_MODE
#include <pugixml.hpp>
#include <sstream>

namespace cube {
// The random dialogs' choices (anonymous: this file's own): five vendor wares, eleven monsters,
// the dialogs random:1.. random:30; and the influence below which a scenery record does not name
// the landscape.
namespace {
enum {
    VendorItemChoices = 5
};
enum {
    RandomMonsterChoices = 11
};
enum {
    RandomDialogCount = 30
};
}

Speech::Speech()
{
    pugi::xml_document document;
    Database database;
    database.open("data4.db");
    BytesIO bytes;
    database.read("dict_en.xml", bytes);
    bytes.descramble();
    if (bytes.size() > 0) {
        if (document.load_buffer(bytes.m_data.empty() ? 0 : &bytes.m_data[0], bytes.size())) {
            pugi::xml_node root = document.first_child();
            for (pugi::xml_node_iterator element = root.begin(); element != root.end(); element++) {
                std::wstring name = element->name();
                std::wstring key = element->attribute(L"key").value();
                if (!key.empty()) {
                    if (name == L"name") {
                        Name entry;
                        for (pugi::xml_node_iterator child = element->begin(); child != element->end(); child++) {
                            if (child->name() && child->child_value()) {
                                entry.forms[child->name()] = child->child_value();
                            }
                        }
                        if (element->attribute(L"tags")) {
                            std::wstringstream tags(element->attribute(L"tags").value());
                            while (tags.good()) {
                                std::wstring tag;
                                tags >> tag;
                                if (!tag.empty())
                                    entry.tags.push_back(tag);
                            }
                        }
                        m_names[key] = entry;
                    }
                    if (name == L"sentence")
                        m_sentences[key] = element->child_value();
                    if (name == L"landscape") {
                        m_landscapeNormal[key] = element->child(L"normal").child_value();
                        m_landscapeIn[key] = element->child(L"in").child_value();
                        m_landscapeTo[key] = element->child(L"to").child_value();
                        m_landscapeThe[key] = element->child(L"the").child_value();
                    }
                    if (name == L"quest") {
                        Quest *quest = new Quest();
                        m_quests[key] = quest;
                        for (pugi::xml_node_iterator child = element->begin(); child != element->end(); child++) {
                            if (child->name() && child->child_value())
                                quest->texts[child->name()].parse(child->child_value());
                        }
                    }
                    if (name == L"speech") {
                        m_speech[key] = new QuestText();
                        m_speech[key]->parse(element->child_value());
                    }
                }
            }
        }
    }
    m_expressionKeys[TalkTest1] = L"talk:test:1";
    m_expressionKeys[TalkTest2] = L"talk:test:2";
    m_expressionKeys[PetFood] = L"talk:test:3";
    m_expressionKeys[Explored] = L"talk:test:4";
    m_expressionKeys[KillTest] = L"kill:test";
    m_expressionKeys[CreatureStrong] = L"creature:strong";
    m_expressionKeys[CreatureStupid] = L"creature:stupid";
    m_expressionKeys[CreatureClever] = L"creature:clever";
    m_expressionKeys[TakeCare1] = L"takecare:1";
    m_expressionKeys[TakeCare2] = L"takecare:2";
    m_expressionKeys[TakeCare3] = L"takecare:3";
    m_expressionKeys[TakeCare4] = L"takecare:4";
}

Speech::~Speech()
{
    for (std::map<std::wstring, Quest *>::iterator it = m_quests.begin(); it != m_quests.end(); ++it) {
        delete it->second;
    }
    for (std::map<std::wstring, QuestText *>::iterator it = m_speech.begin(); it != m_speech.end(); ++it) {
        delete it->second;
    }
}

// The styles' colours are RGBA.
bool Speech::generate(std::vector<Dialog> &dialogs, QuestText *text, QuestTextNode *node,
                      const std::set<std::wstring> &tags, const std::map<std::wstring, std::wstring> &variables)
{
    if (!node)
        return true;

    // The alpha searches the accumulated conditions but does not use the results.
    if (!node->allConditions.empty()) {
        for (std::set<std::wstring>::iterator it = node->allConditions.begin(); it != node->allConditions.end(); ++it) {
            tags.find(*it);
        }
    }
    bool conditional = false;
    if (!node->conditions.empty()) {
        bool met = false;
        for (std::set<std::wstring>::iterator it = node->conditions.begin(); it != node->conditions.end(); ++it) {
            if (tags.find(*it) != tags.end())
                met = true;
        }
        if (!met)
            return true;
        conditional = true;
    }

    if (!node->word.empty()) {
        if (node->word == L"%") {
            dialogs.push_back(Dialog());
        } else {
            plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
            if (node->parent) {
                for (std::set<std::wstring>::iterator it = node->parent->styles.begin();
                     it != node->parent->styles.end(); ++it) {
                    if (*it == L"$creature")
                        color = plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f);
                    if (*it == L"$name")
                        color = plasma::Vector<4, float>(0.6f, 0.3f, 1.0f, 1.0f);
                    if (*it == L"$item")
                        color = plasma::Vector<4, float>(0.3f, 0.3f, 1.0f, 1.0f);
                    if (*it == L"$object")
                        color = plasma::Vector<4, float>(1.0f, 0.5f, 0.3f, 1.0f);
                    if (*it == L"$zone")
                        color = plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f);
                    if (*it == L"$stress")
                        color = plasma::Vector<4, float>(0.6f, 0.3f, 1.0f, 1.0f);
                    if (*it == L"$number")
                        color = plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f);
                }
            }
            if (node->word[0] == L'@') {
                std::map<std::wstring, std::wstring>::const_iterator it = variables.find(node->word);
                if (it != variables.end())
                    dialogs.back().write(it->second, color);
            } else {
                dialogs.back().write(node->word, color);
            }
        }
    }
    for (std::list<QuestTextNode *>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
        if (!generate(dialogs, text, *it, tags, variables))
            break;
    }
    return !conditional;
}

void Speech::generateDialogs(const std::wstring &key, const Context *context, std::vector<Dialog> &dialogs)
{
    std::set<std::wstring> tags;
    std::map<std::wstring, std::wstring> variables;
    if (context) {
        tags = context->tags;
        variables = context->variables;
    }
    QuestText *text = m_speech[key];
    if (text) {
        if (dialogs.empty())
            dialogs.push_back(Dialog());
        generate(dialogs, text, text->m_root, tags, variables);
    }
}

void Speech::generateQuest(World *world, int expression, const plasma::Vector<3, int> &location,
                           std::vector<Dialog> &dialogs)
{
    srand(location[2]);
    Region::Record *scenery =
        world->getRecord(location[0] / ZonesPerSceneryRecord, location[1] / ZonesPerSceneryRecord);
    if (expression == Explored && scenery && scenery->kind != Region::Record::SceneryNone) {
        Context context;
        world->addScenery(L"scenery", scenery, context);
        generateDialogs(L"explored", &context, dialogs);
    } else if (expression == PetFood) {
        // The code divides by zero if the world's race table is empty.
        int race = world->m_petFoodRaces[rand() % world->m_petFoodRaces.size()];
        Context context;
        world->addCreature(L"creature", race, 0, 0, context);
        Item food;
        food.category = ItemCategoryPetFood;
        food.id = race;
        world->addItem(L"food", food, context);
        generateDialogs(L"petfood", &context, dialogs);
    } else {
        Context context;
        switch (rand() % VendorItemChoices) {
        case 0: {
            Item item;
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemFlask;
            world->addItem(L"itemvendoritem", item, context);
            break;
        }
        case 1: {
            Item item;
            item.category = ItemCategoryLamp;
            item.material = ItemMaterialIron;
            world->addItem(L"itemvendoritem", item, context);
            break;
        }
        case 2: {
            Item item;
            item.category = ItemCategorySpecial;
            item.id = SpecialItemHangGlider;
            world->addItem(L"itemvendoritem", item, context);
            break;
        }
        case 3: {
            Item item;
            item.category = ItemCategorySpecial;
            item.id = SpecialItemBoat;
            world->addItem(L"itemvendoritem", item, context);
            break;
        }
        case 4: {
            Item item;
            item.category = ItemCategoryConsumable;
            item.id = ConsumableBomb;
            world->addItem(L"itemvendoritem", item, context);
            break;
        }
        }
        int race = world->m_petFoodRaces[rand() % world->m_petFoodRaces.size()];
        world->addCreature(L"pet", race, 0, 0, context);
        int monster = RaceTroll;
        switch (rand() % RandomMonsterChoices) {
        case 0:
            monster = RaceTroll;
            break;
        case 1:
            monster = RaceDarkTroll;
            break;
        case 2:
            monster = RaceYeti;
            break;
        case 3:
            monster = RaceCyclops;
            break;
        case 4:
            monster = RaceSaurian;
            break;
        case 5:
            monster = RaceGolem;
            break;
        case 6:
            monster = RaceSnowGolem;
            break;
        case 7:
            monster = RaceEmberGolem;
            break;
        case 8:
            monster = RaceMammoth;
            break;
        case 9:
            monster = RaceHellDemon;
            break;
        case 10:
            monster = RaceLich;
            break;
        }
        world->addCreature(L"monster", monster, 0, 0, context);
        std::wstringstream key;
        key << L"random:" << (rand() % RandomDialogCount + 1);
        generateDialogs(key.str(), &context, dialogs);
    }
}

std::wstring Speech::landscapeName(World *world, int x, int y)
{
    Region::Record *scenery =
        world->getRecord(x / BlocksPerZone / ZonesPerSceneryRecord, y / BlocksPerZone / ZonesPerSceneryRecord);
    if (scenery && scenery->kind != Region::Record::SceneryNone && scenery->kind != Region::Record::SceneryPortal &&
        !(scenery->influence(Fixed(x), Fixed(y)) < 0.2f))
        return regionName(world, scenery);
    World::RegionRecord *region = world->nearestRegionRecord(x, y);
    if (!region)
        return L"";
    std::wstring text = m_landscapeNormal[L"Lands of"];
    if (region->height < 0)
        text = m_landscapeNormal[L"Ocean"];
    if (text.empty())
        return L"";
    size_t marker = text.find(L'@');
    if (marker == std::wstring::npos)
        return L"";
    text.replace(marker, 1, world->randomName(region->seed, -1));
    return text;
}

std::wstring Speech::regionName(World *world, Region::Record *region)
{
    std::wstring text = m_names[world->settlementKey(region->kind, region->style)].forms[L"singular"];
    if (text.empty())
        return L"";
    size_t marker = text.find_first_of(L'@');
    if (marker != std::wstring::npos)
        text.replace(marker, 1, world->randomName(region->seed, -1));
    return text;
}

void Dialog::write(std::wstring text, const plasma::Vector<4, float> &color)
{
    std::wstringstream words(text);
    // A failed extraction may append an empty final phrase.
    while (words.good()) {
        std::wstring word;
        words >> word;
        phrases.push_back(Phrase(word, color));
    }
}

std::wstring Speech::cellName(World *world, Region::Cell *cell, int x, int y)
{
    std::wstring key = world->cellKey(cell);
    if (key.empty())
        return L"";
    std::wstring text = m_names[key].forms[L"singular"];
    if (text.empty())
        return L"";
    size_t marker = text.find(L'@');
    if (marker != std::wstring::npos) {
        if (cell->seed && cell->kind)
            text.replace(marker, 1, world->randomName(cell->seed, -1));
        else
            text.replace(marker, 1, world->generateCoordinateName(x, y));
    }
    return text;
}

std::wstring Speech::cellName(World *world, int x, int y)
{
    Region::Cell *cell = world->getCell(x / BlocksPerZone, y / BlocksPerZone);
    if (!cell)
        return L"";
    return cellName(world, cell, x, y);
}

}
