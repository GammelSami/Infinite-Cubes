// Realm provides a world's realm identity, residents and generated lore from its ID.
#pragma once
#include <string>
#include <vector>

#include "cube/Item.h"

namespace cube {

// Appends a number to a lore text, as a string. Texts are lists of dictionary keys and numbers ("define_realm", kind,
// id, ...) that the speech generator reads.
inline std::vector<std::string> &operator<<(std::vector<std::string> &text, const std::string &key)
{
    text.push_back(key);
    return text;
}

inline std::vector<std::string> &operator<<(std::vector<std::string> &text, int value)
{
    text.push_back(std::to_string(value));
    return text;
}
// An item as the script commands "define_item" expect it: the category, the id, the modifier, the material and the
// rarity.
inline std::vector<std::string> &operator<<(std::vector<std::string> &text, const Item &item)
{
    text << item.category << (int)item.id << (int)item.modifier << item.material << item.rarity;
    return text;
}

// Appends the two place coordinates (Missions.cpp).
std::vector<std::string> &operator<<(std::vector<std::string> &text, const plasma::Vector<2, int> &location);

// Generates a ruler-trait dictionary key, such as "#brutality" or "#kindness".
std::string badTrait(int seed);
std::string goodTrait(int seed);
// Who or what a legend comes from ("#ancient_inscription" ..., "#witch" ...). A free function, the last of the file.
std::string witness(int seed);

struct Realm {
    Realm(int id)
        : id(id)
    {}

    // The three ints are not used.
    std::vector<std::string> artifactRumor(const Item &item, int unused1, int unused2, int unused3, int seed,
                                           bool found);
    std::vector<std::string> artifactTale(const Item &item, int x, int y, int location, int seed);

    // Where a person of the realm was born ("#island", "#village" ...). The object is not used.
    std::string birthplace(int seed);
    // The name of the realm's founder, three syllables from fixed tables.
    std::string chiefName();
    int count();
    // The creature type of an enemy of the realm: one of two neighbouring types by the seed for most of the sixteen
    // choices.
    int enemy(int seed);
    // One of the kinds 12..22 (the "realm_..." expressions of the dictionary; Speech::realmName takes it).
    int form();
    // Whether the realm still has people: landHistory tells of the realm in the present tense when it does.
    bool hasPeople();
    // Whether the realm's ruler is a feared one: the lore gives the ruler a bad trait and "!sweat" instead of a good
    // one and "!love".
    bool hasTyrant();
    // The object is not used.
    int killer(int seed);
    std::vector<std::string> landHistory(int seed, int x, int y);
    int lurker(int seed);
    // The realm's magic ("#rune_magic" ...).
    std::string magic();
    // 100..9099.
    int members();
    // The creature type of a resident: the two types (male, female) of one of seven peoples, or one of three fixed
    // types.
    int native(int seed);
    // A type from a table of 13 pairs, the second of a pair when the realm's race is a female type.
    int noble();
    // The land the realm lies in ("#island", "#mountains" ...).
    std::string origin();
    // A place of the realm ("#temple", "#pyramid" ...).
    std::string place();
    int race();
    // The legend of the realm's ruler that a lore object tells: one of seven story lines, drawn with the seed; (x, y)
    // is the land the object stands in.
    std::vector<std::string> rulerLegend(int seed, int x, int y);
    // The inscription of a grave: who rests there, for some the years of birth and death, and how the person died or
    // what the person was known for.
    std::vector<std::string> tombstone(int seed);
    std::vector<std::string> treasureHint(const Item &item, int unused1, int unused2, int unused3, int seed,
                                          bool found);

    int id;
};

}
