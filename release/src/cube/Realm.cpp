// Realm population, names and generated lore; see cube/Realm.h.
#include "cube/Realm.h"

#include "cube/Creature.h"
#include "cube/World.h"
#include "cube/CreatureIds.h"

namespace cube {

// Text choices use a switch on each drawn value, advancing seed for each draw. Multiple seed increments in one
// expression are unsequenced. Function-local tables hold strings or integers.

// The xorshift step every choice is drawn with (as in Item.cpp).
static inline unsigned int scramble(unsigned int value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

std::vector<std::string> Realm::artifactRumor(const Item &item, int unused1, int unused2, int unused3, int seed,
                                              bool found)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_item" << item.category << item.id << item.modifier << item.material << item.rarity;
    switch (scramble(seed++) % 3) {
    default:
        text << "there_is_a_legend_about_an_artifact";
        break;
    case 1:
        text << "im_looking_for_an_artifact";
        break;
    case 2:
        text << "there_is_a_mighty_artifact";
        break;
    }
    if (found) {
        switch (scramble(seed++) % 2) {
        default:
            text << "!question";
            break;
        case 1:
            text << "!alert";
            break;
        }
        text << "you_found_the_artifact";
        text << "new_page";
        switch (scramble(seed++) % 2) {
        default:
            text << "im_jealous";
            if (scramble(seed++) % 2) {
                text << "!angry";
            }
            break;
        case 1:
            text << "nice";
            if (scramble(seed++) % 2) {
                text << "!love";
            }
            break;
        }
    } else {
        text << "!question" << "i_wonder_where_it_could_be";
    }
    return text;
}

std::vector<std::string> Realm::artifactTale(const Item &item, int x, int y, int location, int seed)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_item" << item.category << item.id << item.modifier << item.material << item.rarity;
    switch (scramble(seed) % 3) {
    default:
        text << "there_is_a_legend_about_an_artifact";
        break;
    case 1:
        text << "the_item_was_a_mighty_artifact";
        break;
    case 2:
        text << "there_is_a_mighty_artifact";
        break;
    }
    text << "define_location" << x << y << location;
    if (location >= PlaceCatacombs) {
        text << "it_is_said_to_be_located_in_the_dungeon";
    } else {
        text << "it_is_said_to_be_located_in_the_location";
    }
    text << "mark_location" << x << y;
    return text;
}

int Realm::form()
{
    static std::vector<int> kinds = {FactionRealmKingdom,    FactionRealmKingdomOf, FactionRealmKingdoms,
                                     FactionRealmKingdomsOf, FactionRealmRealm,     FactionRealmRealmOf,
                                     FactionRealmRealms,     FactionRealmRealmsOf,  FactionRealmTribe,
                                     FactionRealmCult,       FactionRealmCultOf};
    return kinds[scramble(id + 8070736) % kinds.size()];
}

int Realm::noble()
{
    static std::vector<int> leaders = {RulerKing,         RulerQueen,         RulerCultistMale,   RulerCultistFemale,
                                       RulerEmperor,      RulerEmpress,       RulerLord,          RulerLady,
                                       RulerWizard,       RulerWitch,         RulerSorcerer,      RulerSorceress,
                                       RulerPrince,       RulerPrincess,      RulerChieftainMale, RulerChieftainFemale,
                                       RulerSeer,         RulerSeeress,       RulerElderMale,     RulerElderFemale,
                                       RulerShamanMale,   RulerShamanFemale,  RulerKnightMale,    RulerKnightFemale,
                                       RulerPreacherMale, RulerPreacherFemale};
    int index = scramble(id + 667755) % (leaders.size() / 2) * 2;
    if (isFemale(race())) {
        index++;
    }
    return leaders[index];
}

int Realm::killer(int seed)
{
    static std::vector<int> mobs = {
        RaceZombie,         RaceSkeleton,      RaceSkeletonKnight, RaceFrightener,    RaceMonstrosity,
        RaceNecromancer,    RaceSkeletonDog,   RaceSkeletonHorse,  RaceWraith,        RaceMonstrosity,
        RaceVampire,        RaceBlueSlime,     RaceWerewolf,       RaceDemonMarauder, RaceImp,
        RaceDevourer,       RaceCrawler,       RaceHellDemon,      RaceFlyingEye,     RaceDemonMarauder,
        RaceDemonMarauder,  RaceWitch,         RaceWizard,         RaceSorcerer,      RaceTroll,
        RaceDarkTroll,      RaceYeti,          RaceMammoth,        RaceCyclops,       RaceSaurian,
        RaceManaMech,       RaceLich,          RaceRuneGiant,      RaceGolem,         RaceEmberGolem,
        RaceDjinn,          RaceBarkBeetle,    RaceLemonBeetle,    RaceSnail,         RaceCaterpillar,
        RaceSnoutBeetle,    RacePlantCreature, RaceRadishCreature, RaceOnionling,     RaceDesertOnionling,
        RaceFamilyAssassin, RaceFamilyBandit};
    return mobs[scramble(seed) % mobs.size()];
}

int Realm::lurker(int seed)
{
    static std::vector<int> mobs = {RaceNecromancer,
                                    RaceVampire,
                                    RaceWitch,
                                    RaceSorcerer,
                                    RaceWizard,
                                    RaceHornet,
                                    RaceFamilyDemon,
                                    RaceFamilyBandit,
                                    RaceFamilyAssassin,
                                    RaceFamilyCultOfDoomAdherer,
                                    RaceFamilyUnholyPactFollower};
    return mobs[scramble(seed) % mobs.size()];
}

std::string goodTrait(int seed)
{
    static std::vector<std::string> traits = {"#kindness", "#friendliness", "#goodness", "#gentleness", "#honesty",
                                              "#pureness", "#loyalty",      "#fairness", "#generosity", "#happyness"};
    return traits[scramble(seed + 8371) % traits.size()];
}

std::string badTrait(int seed)
{
    static std::vector<std::string> traits = {"#brutality",      "#cruelty",        "#harshness",
                                              "#relentlessness", "#unmercifulness", "#ruthlessness",
                                              "#badmood",        "#badness",        "#viciousness"};
    return traits[scramble(seed + 5134) % traits.size()];
}

std::string Realm::birthplace(int seed)
{
    static std::vector<std::string> strings = {"#island",    "#village", "#ocean", "#mountains", "#snowlands",
                                               "#deadlands", "#farm",    "#city",  "#palace"};
    return strings[scramble(seed + 13134) % strings.size()];
}

std::string Realm::magic()
{
    static std::vector<std::string> strings = {"#rune_magic",    "#blood_magic",         "#gem_magic",
                                               "#dark_magic",    "#black_magic",         "#unholy_magic",
                                               "#holy_magic",    "#divine_magic",        "#life_magic",
                                               "#death_magic",   "#mana_transformation", "#matter_transformation",
                                               "#mana_infusion", "#particle_fusion",     "#advanced_alchemy"};
    return strings[scramble(id + 234234) % strings.size()];
}

std::string Realm::place()
{
    static std::vector<std::string> strings = {"#temple", "#pyramid",   "#palace",   "#grotto", "#cave",   "#citadel",
                                               "#castle", "#cathedral", "#fortress", "#tower",  "#volcano"};
    return strings[scramble(id + 234234) % strings.size()];
}

std::string Realm::origin()
{
    static std::vector<std::string> strings = {"#island",    "#mountains", "#ocean", "#snowlands",
                                               "#deadlands", "#forest",    "#desert"};
    return strings[scramble(id + 5454) % strings.size()];
}

std::string Realm::chiefName()
{
    static std::vector<std::string> first = {"Ka", "Cha", "Ca", "Cy", "Ko", "Go", "Ga", "Gro", "Te", "Tu",
                                             "Lu", "A",   "E",  "Y",  "I",  "Vy", "Wa", "No",  "Le", "Ti",
                                             "Ba", "So",  "Sa", "Si", "Xa", "Xy", "Xi", "Zu",  "Za"};
    static std::vector<std::string> second = {"mer", "ra", "gu", "ga", "mor", "nor", "lor", "gor", "nar", "far"};
    static std::vector<std::string> third = {"zan", "a", "o", "ia", "an", "e", "une", "unia", "nia"};
    return first[scramble(id + 98371) % first.size()] + second[scramble(id + 5004) % second.size()] +
           third[scramble(id + 20302) % third.size()];
}

std::string witness(int seed)
{
    static std::vector<std::string> strings = {"#ancient_inscription",
                                               "#old_inscription",
                                               "#magic_inscription",
                                               "#stone_tablet",
                                               "#inscribed_tablet",
                                               "#ancient_scroll",
                                               "#old_scroll",
                                               "#magic_scroll",
                                               "#ancient_book",
                                               "#magic_book",
                                               "#old_book",
                                               "#witch",
                                               "#wise_man",
                                               "#wise_woman",
                                               "#sorcerer",
                                               "#sorceress",
                                               "#wizard",
                                               "#ghost",
                                               "#spirit",
                                               "#djinn",
                                               "#demon"};
    return strings[scramble(seed + 77873) % strings.size()];
}

int Realm::count()
{
    return scramble(id + 99173) % 8 + 1;
}

int Realm::enemy(int seed)
{
    switch (scramble(id + 5500) % 16) {
    case 1:
        return RaceHumanMale + scramble(seed) % 2;
    case 2:
        return RaceGoblinMale + scramble(seed) % 2;
    case 3:
        return RaceLizardmanMale + scramble(seed) % 2;
    case 4:
        return RaceDwarfMale + scramble(seed) % 2;
    case 5:
        return RaceOrcMale + scramble(seed) % 2;
    case 6:
        return RaceFrogmanMale + scramble(seed) % 2;
    case 7:
        return RaceUndeadMale + scramble(seed) % 2;
    case 8:
        return RaceGnoll;
    case 9:
        return RaceGnobold;
    case 10:
        return RaceSpikeCreature;
    case 11:
        return RaceSeeker;
    case 12:
        return RaceWizard;
    case 13:
        return RaceNecromancer;
    case 14:
        return RaceDarkCultistMale + scramble(seed) % 2;
    case 15:
        return RaceSorcerer;
    }
    return RaceElfMale + scramble(seed) % 2;
}

bool Realm::hasPeople()
{
    return scramble(id + 776377) % 2 == 0;
}

bool Realm::hasTyrant()
{
    return scramble(id + 59083) % 2;
}

std::vector<std::string> Realm::landHistory(int seed, int x, int y)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_land" << x << y;
    text << "define_number" << members();
    text << "define_ruler" << noble();
    text << "define_mob" << scramble(id + 33092) << race();
    if (scramble(seed++) % 2) {
        text << "history_time" << "new_page";
    }
    if (hasPeople()) {
        switch (scramble(seed++) % 2) {
        default:
            switch (scramble(seed++) % 2) {
            default:
                text << "did_you_meet_any_residents";
                break;
            case 1:
                text << "there_are_still_some_residents_in_the_land";
                if (scramble(seed++) % 2) {
                    text << "new_page";
                    text << "did_you_meet_any";
                }
                break;
            }
            text << "new_page";
            switch (scramble(seed++) % 3) {
            default:
                text << "their_ruler_is_famous";
                if (scramble(seed++) % 2) {
                    text << "new_page";
                    switch (scramble(seed++) % 2) {
                    default:
                        if (hasTyrant()) {
                            text << badTrait(id + 7726);
                        } else {
                            text << goodTrait(id + 7726);
                        }
                        text << "he_is_known_for_his_trait";
                        if (scramble(seed++) % 2) {
                            if (hasTyrant()) {
                                text << "!sweat";
                            } else {
                                text << "!love";
                            }
                        }
                        break;
                    case 1:
                        text << magic();
                        text << "he_is_a_master_of_realm_magic";
                        break;
                    }
                }
                break;
            case 1:
                text << "there_are_legends_about_realm_artifacts";
                break;
            case 2:
                text << "you_can_find_realm_ruins_everywhere";
                break;
            }
            break;
        case 1:
            text << "have_you_heard_about_the_realm";
            text << "new_page";
            switch (scramble(seed++) % 4) {
            default:
                text << "there_are_still_some_residents_in_the_land";
                if (scramble(seed++) % 2) {
                    text << "new_page";
                    text << "did_you_meet_any";
                }
                break;
            case 1:
                text << "there_are_legends_about_realm_artifacts";
                break;
            case 2:
                text << "you_can_find_realm_ruins_everywhere";
                break;
            case 3:
                text << "define_model" << native(seed);
                switch (scramble(seed++) % 2) {
                default:
                    text << "its_a_nation_of_creatures";
                    break;
                case 1:
                    text << "the_residents_are_a_realm_of_creatures";
                    break;
                }
                break;
            }
            break;
        }
    } else {
        switch (scramble(seed++) % 2) {
        default:
            text << "many_years_ago_the_land_was_full_of_residents";
            text << "new_page";
            text << "their_ruler_was_famous";
            if (scramble(seed++) % 2) {
                text << "new_page";
                switch (scramble(seed++) % 2) {
                default:
                    if (hasTyrant()) {
                        text << badTrait(id + 7726);
                    } else {
                        text << goodTrait(id + 7726);
                    }
                    text << "he_was_known_for_his_trait";
                    if (scramble(seed++) % 2) {
                        if (hasTyrant()) {
                            text << "!sweat";
                        } else {
                            text << "!love";
                        }
                    }
                    break;
                case 1:
                    text << magic();
                    text << "he_was_a_master_of_realm_magic";
                    break;
                }
            }
            break;
        case 1:
            switch (scramble(seed++) % 2) {
            default:
                text << "there_is_a_perished_civilization";
                text << "new_page";
                text << "you_can_still_find_realm_ruins_everywhere";
                break;
            case 1:
                text << "have_you_heard_about_the_realm";
                text << "new_page";
                switch (scramble(seed++) % 2) {
                default:
                    text << "you_can_still_find_realm_ruins_everywhere";
                    break;
                case 1:
                    text << "there_are_legends_about_realm_artifacts";
                    break;
                }
                break;
            }
            break;
        }
    }
    return text;
}

int Realm::members()
{
    return scramble(id + 78601) % 9000 + 100;
}

int Realm::native(int seed)
{
    switch (scramble(id + 78716) % 11) {
    case 1:
        return RaceHumanMale + scramble(seed) % 2;
    case 2:
        return RaceGoblinMale + scramble(seed) % 2;
    case 3:
        return RaceLizardmanMale + scramble(seed) % 2;
    case 4:
        return RaceDwarfMale + scramble(seed) % 2;
    case 5:
        return RaceOrcMale + scramble(seed) % 2;
    case 6:
        return RaceFrogmanMale + scramble(seed) % 2;
    case 7:
        return RaceUndeadMale + scramble(seed) % 2;
    case 8:
        return RaceGnoll;
    case 9:
        return RaceGnobold;
    case 10:
        return RaceSpikeCreature;
    }
    return RaceElfMale + scramble(seed) % 2;
}

int Realm::race()
{
    switch (scramble(id + 33002) % 8) {
    case 1:
        return RaceHumanMale + scramble(id + 1212) % 2;
    case 2:
        return RaceGoblinMale + scramble(id + 1212) % 2;
    case 3:
        return RaceLizardmanMale + scramble(id + 1212) % 2;
    case 4:
        return RaceDwarfMale + scramble(id + 1212) % 2;
    case 5:
        return RaceOrcMale + scramble(id + 1212) % 2;
    case 6:
        return RaceFrogmanMale + scramble(id + 1212) % 2;
    case 7:
        return RaceUndeadMale + scramble(id + 1212) % 2;
    }
    return RaceElfMale + scramble(id + 1212) % 2;
}

std::vector<std::string> Realm::rulerLegend(int seed, int x, int y)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_ruler" << noble();
    switch (scramble(seed++) % 7) {
    default:
        if (hasPeople()) {
            switch (scramble(seed++) % 2) {
            default:
                text << "define_number" << members();
                text << "the_realm_exists_since_the_year";
                if (scramble(seed++) % 2) {
                    text << "new_page";
                    text << "define_mob" << scramble(id + 33092) << race();
                    text << "it_is_ruled_by";
                }
                break;
            case 1:
                text << "define_mob" << scramble(id + 33092) << race();
                text << "the_realm_is_ruled_by";
                break;
            }
            if (scramble(seed++) % 2) {
                text << magic();
                text << "he_is_a_master_of_realm_magic";
            }
        } else {
            switch (scramble(seed++) % 2) {
            default:
                text << "define_mob" << scramble(id + 33092) << race();
                if (scramble(seed++) % 2) {
                    text << "define_number" << members();
                    text << "in_the_year_there_was_a_realm";
                } else {
                    text << "once_there_was_a_realm";
                }
                text << "new_page";
                switch (scramble(seed++) % 2) {
                default:
                    text << "the_residents_had_a_ruler";
                    switch (scramble(seed++) % 3) {
                    default:
                        text << "define_mob" << scramble(id + 33092) << race();
                        text << "his_name_was";
                        break;
                    case 1:
                        if (hasTyrant()) {
                            text << badTrait(id + 7726);
                        } else {
                            text << goodTrait(id + 7726);
                        }
                        text << "he_was_known_for_his_trait";
                    case 2:
                        text << magic();
                        text << "he_was_a_master_of_realm_magic";
                        break;
                    }
                    break;
                case 1:
                    text << "the_realm_was_ruled_by";
                    break;
                }
                break;
            case 1:
                text << "define_mob" << scramble(id + 33092) << race();
                if (scramble(seed++) % 2) {
                    text << "define_number" << members();
                    text << "in_the_year_there_was_a_named_ruler";
                } else {
                    text << "once_there_was_a_a_named_ruler";
                }
                switch (scramble(seed++) % 3) {
                default:
                    text << "he_founded_the_realm";
                    break;
                case 1:
                    if (hasTyrant()) {
                        text << badTrait(id + 7726);
                    } else {
                        text << goodTrait(id + 7726);
                    }
                    text << "he_was_known_for_his_trait";
                    break;
                case 2:
                    text << magic();
                    text << "he_was_a_master_of_realm_magic";
                    break;
                }
                break;
            }
        }
        break;
    case 1:
        text << "define_mob" << scramble(id + 33092) << race();
        text << birthplace(id);
        text << "npc_lived_in_a_location" << "new_page";
        switch (scramble(id + 8971) % 3) {
        default:
            text << "define_faction" << count();
            text << "define_land" << scramble(id + 5322) % 14 << scramble(id + 98712);
            text << "during_the_faction_war_he_had_to_flee_to_the_land";
            break;
        case 1:
            text << "define_land" << scramble(id + 5322) % 14 << scramble(id + 98712);
            text << "later_he_moved_to_the_land";
            break;
        case 2:
            text << "define_land" << scramble(id + 5322) % 14 << scramble(id + 98712);
            text << "one_day_he_explored_the_land";
            break;
        }
        switch (scramble(id + 1233) % 5) {
        default:
            text << "define_faction" << count();
            text << "there_he_fought_against_the_faction";
            break;
        case 1:
            text << "there_he_explored_ancient_ruins_and_dungeons";
            break;
        case 2:
            text << magic();
            text << "there_he_learned_about_realm_magic";
            break;
        case 3:
            text << "after_some_years_he_founded_the_realm";
            break;
        case 4:
            text << "there_are_many_legends_about_his_adventures";
            break;
        }
        break;
    case 2:
        text << "define_mob" << scramble(id + 33092) << race();
        text << "define_land" << scramble(id + 5322) % 14 << scramble(id + 98712);
        text << witness(id);
        text << magic();
        switch (scramble(seed++) % 6) {
        default:
            text << "during_his_time_in_land_npc_discovered_wisdom";
            break;
        case 1:
            text << "npc_used_his_realm_magic_skills_to_explore_dangerous_dungeons";
            break;
        case 2:
            text << "define_faction" << count();
            text << "npc_used_his_realm_magic_skills_to_fight_the_faction";
            break;
        case 3:
            text << "using_realm_magic_npc_was_able_to_increase_his_power";
            break;
        case 4:
            text << "define_land" << x << y;
            text << "npc_used_his_realm_magic_skills_to_explore_the_land";
            break;
        case 5:
            text << "npc_spent_a_lot_of_time_exploring_the_land";
            break;
        }
        break;
    case 3:
        switch (scramble(seed++) % 2) {
        default:
            text << magic();
            if (hasPeople()) {
                text << "some_residents_use_realm_magic";
            } else {
                text << "some_residents_used_realm_magic";
            }
            break;
        case 1:
            if (hasPeople() || scramble(seed++) % 2) {
                text << "define_mob" << scramble(id + 33092) << race();
                text << "praise_the_ruler";
            } else {
                text << "define_mob" << scramble(id + 33092) << race();
                if (hasTyrant()) {
                    text << "the_residents_feared_their_ruler";
                    text << badTrait(id + 7726);
                } else {
                    text << "the_residents_loved_their_ruler";
                    text << goodTrait(id + 7726);
                }
                text << "he_was_known_for_his_trait";
            }
            break;
        }
        break;
    case 4:
        text << "define_name" << chiefName();
        text << "define_faction" << count();
        switch (scramble(seed++) % 4) {
        default:
            switch (id % 8) {
            default:
                text << "the_capital_of_the_realm_was_destroyed_during_a_war_against_the_faction";
                break;
            case 1:
                text << "the_capital_of_the_realm_sunk_in_the_ocean";
                break;
            case 2:
                text << "the_capital_of_the_realm_was_destroyed_by_a_meteor";
                break;
            case 3:
                text << "the_capital_of_the_realm_was_destroyed_by_an_earthquake";
                break;
            case 4:
                text << "the_capital_of_the_realm_was_destroyed_by_a_volcanic_eruption";
                break;
            case 5:
                text << "the_capital_of_the_realm_was_destroyed_by_a_titan";
                break;
            case 6:
                text << "the_capital_of_the_realm_was_destroyed_by_a_megasaurian";
                break;
            case 7:
                text << "the_capital_of_the_realm_was_destroyed_by_a_demon";
                break;
            }
            break;
        case 1:
            text << "the_capital_of_the_realm_was_called";
            break;
        case 2:
            text << "archeologists_are_looking_for_the_ruins_of_the_capital_of_the_realm";
            if (scramble(seed++)) {
                text << "new_page";
                text << "until_now_their_efforts_were_useless";
            }
            break;
        case 3:
            text << "define_number" << scramble(id + 50005) % 101 + members();
            text << "the_capital_of_the_realm_was_built_in_year";
            break;
        }
        break;
    case 5:
        text << "define_land" << x << y;
        text << "define_number" << members();
        switch (scramble(seed++) % 3) {
        default:
            if (hasPeople()) {
                text << "the_land_is_inhabited_by_residents_since_year";
            } else {
                text << "the_land_was_inhabited_by_residents";
            }
            break;
        case 1:
            text << "the_land_was_populated_by_residents_in_year";
            break;
        case 2:
            text << "define_faction" << count();
            text << "long_ago_there_was_a_war_in_the_land_between_the_realm_and_the_faction";
            break;
        }
        break;
    case 6:
        text << "define_mob" << scramble(id + 33092) << race();
        if (hasPeople()) {
            text << place() << origin();
            text << "the_ruler_lives_in";
        } else {
            text << place() << origin();
            text << "the_ruler_was_buried_in";
        }
        break;
    }
    return text;
}

std::vector<std::string> Realm::tombstone(int seed)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_mob" << scramble(seed++) << native(seed++);
    int age = scramble(seed++) % 500 + 18;
    switch (scramble(seed++) % 3) {
    default:
        text << "here_rests_hero_of_the_realm";
        break;
    case 1:
        text << "here_rests_resident";
        break;
    case 2: {
        int year = scramble(seed++) % 1000 + members();
        text << "define_number" << year << "define_number2" << year + age;
        text << "npc_life_span";
        break;
    }
    }
    text << "new_page";
    switch (scramble(seed++) % 11) {
    default:
        text << "define_faction" << count();
        text << "he_died_in_the_war_against_the_faction";
        break;
    case 1:
        text << "define_faction" << count();
        text << "he_was_killed_by_a_faction_member";
        break;
    case 2:
        text << "define_mob2" << scramble(seed++) << enemy(seed++);
        text << "he_was_killed_by_npc";
        break;
    case 3:
        text << "define_model2" << killer(seed++);
        text << "he_was_killed_by_a_monster";
        break;
    case 4:
        text << "define_model2" << lurker(seed++);
        text << "he_was_poisoned_by_a_creature";
        break;
    case 5:
        text << "he_lived_a_full_life";
        break;
    case 6:
        text << goodTrait(seed++);
        text << "he_was_known_for_his_trait";
        break;
    case 7:
        text << "define_number" << age;
        text << "he_lived_for_years";
        break;
    case 8:
        text << magic();
        text << "he_was_a_master_of_realm_magic";
        break;
    case 9:
        text << "there_are_many_legends_about_his_adventures";
        break;
    case 11:
        switch (scramble(seed++) % 4) {
        default:
            text << "he_was_a_warrior";
            break;
        case 1:
            text << "he_was_a_ranger";
            break;
        case 2:
            text << "he_was_a_rogue";
            break;
        case 3:
            text << "he_was_a_mage";
            break;
        }
        break;
    }
    return text;
}

std::vector<std::string> Realm::treasureHint(const Item &item, int unused1, int unused2, int unused3, int seed,
                                             bool found)
{
    std::vector<std::string> text;
    text << "define_realm" << form() << id;
    text << "define_item" << item.category << item.id << item.modifier << item.material << item.rarity;
    switch (scramble(seed++) % 2) {
    default:
        text << "there_is_a_legend_about_an_artifact";
        break;
    case 1:
        text << "there_is_a_mighty_artifact";
        break;
    }
    if (found) {
        switch (scramble(seed++) % 2) {
        default:
            text << "!question";
            break;
        case 1:
            text << "!alert";
            break;
        }
        text << "you_found_the_artifact";
        text << "new_page";
        text << "nice";
        if (scramble(seed++) % 2) {
            text << "!love";
        }
    } else {
        switch (scramble(seed++) % 2) {
        default:
            text << "are_you_looking_for_it";
            break;
        case 1:
            text << "!question" << "i_wonder_if_it_really_exists";
            break;
        }
    }
    return text;
}

}
