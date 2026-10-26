// Speech maps game identifiers to dictionary keys, loads dialog and name templates, and builds coloured dialogs from
// text and numeric scripts.
// The constructor fills the type-id-to-key tables (entity, skill, specialization, faction, ruler and item keys such as
// L"ElfMale", L"SkillSmash", L"SpecializationBerserker", L"OrderOfTheLight", L"RulerElderMale", L"Boots") and reads
// dict_en.xml, keeping its "sentence", "quest", "speech" / "name" and "macro" elements by their "id" attribute.
#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cube/SpeechText.h"
#include "plasma/Vector.h"
#include "cube/Dialog.h"

namespace cube {

struct Item;

// Returns the dictionary key for an expression ID, or an empty string; used by generateFromCommands.
std::wstring expressionKey(int expression);

class Speech {
public:
    struct Context {
        Context()
            : unused40(0),
              unused48(0)
        {}

        std::set<std::wstring> tags;
        std::map<std::wstring, std::wstring> variables;
        std::map<std::wstring, std::wstring> unusedMap; // constructed and destroyed only
        std::map<std::wstring, plasma::Vector<4, float>> colors;
        long long unused40; // 0; never read
        long long unused48; // 0; never read
        // Not set by the constructor.
        unsigned char unused50[0x10]; // no access
    };

    struct Quest {
        std::map<std::wstring, SpeechText> texts;
    };

    Speech();
    virtual ~Speech();

    // The add functions: each makes a script name stand for something. They put the tags of its dictionary entry into
    // context.tags (addTags) and set context.variables["@" + name] to its dictionary key; several also set a generated
    // proper name or a colour. "define_model": name stands for the entity type's key.
    void addCreature(const std::wstring &name, int entityType, Context &context);
    // "define_mob": the same, and "@<name>_name" is a name generated from seed for the entity type.
    void addCreature(const std::wstring &name, int entityType, long long seed, Context &context);
    // "define_direction": adds the compass tag ("#east", "#northeast"...) nearest to direction. name is not used.
    void addDirection(const std::wstring &name, const plasma::Vector<2, int> &direction, Context &context);
    // The third parameter is not used.
    void addEmpty(const std::wstring &name, int biome, long long seed, Context &context);
    // "define_faction": name stands for the faction's key; "!@<name>" is a name generated from (seed, kind).
    void addFaction(const std::wstring &name, int faction, long long seed, int kind, Context &context);
    // "define_item": the item's key, rarity ("@<name>rarity" = "+N"), colour, material and, for weapons and armour,
    // the "@complete<name>" quality word chosen by rarity and modifier.
    void addItem(const std::wstring &name, const Item &item, Context &context);
    // "define_land": name stands for the biome's key, coloured with the biome colour; "!@<name>" is a name generated
    // from seed.
    void addLand(const std::wstring &name, int biome, long long seed, Context &context);
    void addLands(const std::wstring &name, long long seed, Context &context);
    // "define_location": name stands for the place type's key; "!@<name>" is the place's generated name.
    void addLocation(const std::wstring &name, int x, int y, int type, Context &context);
    // "define_realm": "realm" stands for the key of kind; "@prefix" and "@suffix" are chosen by a hash of realm from
    // function-local tables.
    void addRealm(int kind, int realm, Context &context);
    // "define_ruler": adds "#ruler_<tag>" for the ruler's dictionary entry and sets "@ruler" to its key, when the
    // dictionary has the entry.
    void addRuler(int ruler, Context &context);
    // "define_specialization": name stands for the key of the class's specialization.
    void addSpecialization(const std::wstring &name, int classType, int specialization, Context &context);
    // Replaces the tags "#<name>_..." of context.tags by "#<name>_<tag>" for each tag of the dictionary entry key
    // (m_speechNodeMap), if any.
    void addTags(const std::wstring &name, const std::wstring &key, Context &context);
    // Display name of an item.
    std::wstring createItemName(const Item &item);
    // The dictionary key of an entity type (m_entityTypeIdMap), or an empty string.
    std::wstring creatureKey(int type);
    std::wstring emptyText();
    // The dictionary key of a faction (m_factionTypeIdMap), or an empty string.
    std::wstring factionKey(int faction);
    // The name of a faction or realm kind: below 12 the "default" text of the faction's key with a name generated from
    // the two seeds, otherwise realmName(kind, seed).
    std::wstring factionName(int kind, long long seed, int variant);
    // The generated name of the place of a 1024-block cell (rounded down to even coordinates for a large place) and
    // its type.
    std::wstring findPlaceName(int x, int y, int type, bool large);
    // Appends the words of one node of a parsed text, and of the nodes below it, to the last dialog (a "%" node starts
    // a new dialog). A node with conditions is skipped unless one of them is in tags; "$..." style words choose the
    // colour and "$shake" sets shake; "@name" nodes are replaced by the variable of that name (or, when the node has
    // arguments, by the speech text the variable names, built with its own variables and tags); "*name" nodes by the
    // macro of that name. Node type 3 is built with probability 1/2, and of the unconditional children of a type 2
    // node one is picked; seed drives both. prefix, when not empty, opens an empty dialog as "<prefix>: ". False when
    // the node had conditions and was built (the caller then skips its remaining siblings).
    bool generate(std::vector<Dialog> &dialogs, SpeechText *text, SpeechTextNode *node,
                  const std::set<std::wstring> &tags, const std::map<std::wstring, std::wstring> &variables,
                  const std::map<std::wstring, plasma::Vector<4, float>> &colors, int seed,
                  const plasma::Vector<4, float> &color, int shake, const std::wstring &prefix);
    // Appends the text for key to dialogs. A key starting with '!' becomes one literal phrase; '*' splits the literal
    // at whitespace; other keys are looked up in m_speechNodeMap and built with generate. seed chooses alternatives;
    // argument supplies generate's prefix.
    void generateDialogs(const std::wstring &key, Context &context, int seed, std::vector<Dialog> &dialogs,
                         const std::wstring &argument);
    // The same as generateFromScript for a script in its numeric form: command numbers 0..23 and 61..66 with int
    // arguments; any other number is the id of a text key (expressionKey).
    void generateFromCommands(const std::vector<int> &commands, int seed, std::vector<Dialog> &dialogs,
                              const std::wstring &argument);
    // Builds dialogs from a script of commands ("new_page", "define_mob", "define_item", "mark_location" ...), each
    // followed by its arguments as strings; any other string is a text key (generateDialogs), an emote ("!...") or a
    // tag ("#..."). seed chooses among text alternatives.
    void generateFromScript(const std::vector<std::string> &script, int seed, std::vector<Dialog> &dialogs,
                            const std::wstring &argument);
    // The text generated for key: builds its dialogs (generateDialogs) with a context holding tags and macros, and
    // joins the phrases of the first dialog with blanks (none around "-", "=" and "'", none before punctuation). seed
    // chooses among alternatives.
    std::wstring generateText(const std::wstring &key, const std::set<std::wstring> &tags,
                              const std::map<std::wstring, std::wstring> &macros, int seed);
    // The dictionary key of an item's category and id (m_itemIdentifierMap), or L"".
    std::wstring getItemKey(int category, int id);
    // The map text colour of a biome type.
    plasma::Vector<4, float> getLandColor(int biome);
    // The dictionary key of a biome type (L"Plains", L"Ocean" ...).
    std::wstring getLandName(int biome);
    // The dictionary key of a place type (L"House", L"Farm" ...).
    std::wstring getLocationKey(int type);
    // The dictionary key of an item material (L"MaterialIron" ...; L"Material" for any other).
    std::wstring getMaterialKey(int material);
    // The text of key in the given variant (the tag L"#" + variant; CharacterWidget passes L"default"), with argument
    // as the macro L"@". Callers throughout the game and the interface.
    std::wstring getText(const std::wstring &key, const std::wstring &variant, const std::wstring &argument);
    // The "realmname" text of a realm, of the given kind (Realm::form).
    std::wstring realmName(int kind, int realm);
    // The dictionary key of a ruler type (m_rulerTypeIdMap), or L"".
    std::wstring rulerKey(int ruler);
    // The dictionary key of a skill (m_skillTypeIdMap), or L"".
    std::wstring skillKey(int skill);
    // The key of a (class, specialization) pair (L"SpecializationBerserker" for (1, 0)), or L"".
    std::wstring specializationKey(int classType, int specialization);
    // The parsed "speech" or "name" element of key (m_speechNodeMap), or null.
    SpeechText *speechText(const std::wstring &key);
    // The dictionary key of three static entity types (53 L"ShrineOfLife", 58 L"ManaWell", 59 L"ShrineOfFinesse"),
    // else L"".
    std::wstring staticKey(int type);

    static Speech *s_instance;

protected:
    std::map<int, std::wstring> m_entityTypeIdMap; // entity type -> key (L"ElfMale" for 0, ...)
    std::map<int, std::wstring> m_skillTypeIdMap;  // skill -> key (L"SkillSmash"...)
    // (class, specialization) -> key (L"SpecializationBerserker" for (1, 0)).
    std::map<std::pair<int, int>, std::wstring> m_specializationTypeIdMap;
    std::map<int, std::wstring> m_factionTypeIdMap; // faction -> key (L"OrderOfTheLight"...)
    std::map<int, std::wstring> m_rulerTypeIdMap;   // ruler type -> key (L"RulerElderMale" for 18, ...)
    // Two ints (item category and id) -> key (L"Boots"...).
    std::map<std::pair<int, int>, std::wstring> m_itemIdentifierMap;
    std::map<std::wstring, std::wstring> m_unusedTextMap;
    std::map<std::wstring, std::wstring> m_sentences;
    std::map<std::wstring, Quest *> m_quests;
    std::map<std::wstring, SpeechText *> m_speechNodeMap;
    // dict_en.xml "macro" elements: id -> text; owned.
    std::map<std::wstring, SpeechText *> m_macroNodeMap;
    // Constructed and destroyed only; never filled or read.
    std::map<long long, std::wstring> m_unusedIdMap;
};

}
