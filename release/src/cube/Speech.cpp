// The constructor fills the type-to-key tables and reads dict_en.xml; the add functions and the script interpreters
// build the context of a text; generate and its callers turn dictionary entries into dialogs; the key functions map
// game ids to dictionary keys.
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include "pugixml.hpp"
#include "cube/Speech.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/Item.h"
#include "cube/SpeechText.h"
#include "cube/World.h"
#include "plasma/Vector.h"
#include "cube/CreatureIds.h"
#include "cube/ItemIds.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"
#include "cube/SpeechIds.h"

namespace cube {

static const int QuestionCommandBase = 10000;

Speech *Speech::s_instance;

static inline unsigned int scramble(unsigned int value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

// The entry of a name table chosen by the hash of seed.
static inline const std::wstring &pickName(const std::wstring *names, unsigned int count, int seed)
{
    const std::wstring *picked = names + scramble(seed) % count;
    return *picked;
}

// Vector helper.
static inline void normalize(plasma::Vector<2, float> &vector)
{
    float scale = 1.0f / sqrtf(vector[0] * vector[0] + vector[1] * vector[1]);
    vector[0] *= scale;
    vector[1] *= scale;
}

// The arguments of a text script (generateFromScript): each read takes the next strings, or leaves its target alone
// when too few are left.
class ScriptReader {
public:
    explicit ScriptReader(const std::vector<std::string> &script)
        : m_script(&script),
          m_position(0)
    {}

    bool isAtEnd() const { return m_position >= (int)m_script->size(); }

    // A number.
    template<class T> ScriptReader &operator>>(T &value)
    {
        if (m_position + 1 > (int)m_script->size())
            return *this;
        value = std::stoi((*m_script)[m_position++]);
        return *this;
    }

    ScriptReader &operator>>(std::string &value)
    {
        if (m_position + 1 > (int)m_script->size())
            return *this;
        value = (*m_script)[m_position++];
        return *this;
    }

    // An item as category, id, modifier, material, rarity.
    ScriptReader &operator>>(Item &item)
    {
        if (m_position + 5 > (int)m_script->size())
            return *this;
        item.category = std::stoi((*m_script)[m_position++]);
        item.id = std::stoi((*m_script)[m_position++]);
        item.modifier = std::stoi((*m_script)[m_position++]);
        item.material = std::stoi((*m_script)[m_position++]);
        item.rarity = std::stoi((*m_script)[m_position++]);
        return *this;
    }

    ScriptReader &operator>>(plasma::Vector<2, int> &value)
    {
        if (m_position + 2 > (int)m_script->size())
            return *this;
        value[0] = std::stoi((*m_script)[m_position++]);
        value[1] = std::stoi((*m_script)[m_position++]);
        return *this;
    }

private:
    const std::vector<std::string> *m_script;
    int m_position;
};

// The same for a numeric script (generateFromCommands).
class ScriptValueReader {
public:
    explicit ScriptValueReader(const std::vector<int> &script)
        : m_script(&script),
          m_position(0)
    {}

    bool isAtEnd() const { return m_position >= (int)m_script->size(); }

    ScriptValueReader &operator>>(int &value)
    {
        if (m_position + 1 > (int)m_script->size())
            return *this;
        value = (*m_script)[m_position++];
        return *this;
    }

    // A 64-bit value, high half first.
    ScriptValueReader &operator>>(long long &value)
    {
        if (m_position + 2 > (int)m_script->size())
            return *this;
        // Two unsequenced increments in one expression (undefined behaviour).
        value = ((long long)(*m_script)[m_position++] << 32) | (*m_script)[m_position++];
        return *this;
    }

    ScriptValueReader &operator>>(plasma::Vector<2, int> &value)
    {
        if (m_position + 2 > (int)m_script->size())
            return *this;
        value[0] = (*m_script)[m_position++];
        value[1] = (*m_script)[m_position++];
        return *this;
    }

    ScriptValueReader &operator>>(Dialog::ScriptRecord &value)
    {
        if (m_position + 8 > (int)m_script->size())
            return *this;
        value.byte0 = (*m_script)[m_position++];
        value.byte1 = (*m_script)[m_position++];
        value.ints0[0] = (*m_script)[m_position++];
        value.ints0[1] = (*m_script)[m_position++];
        // Two unsequenced increments in one expression (undefined behaviour).
        value.value64 = ((long long)(*m_script)[m_position++] << 32) | (*m_script)[m_position++];
        value.ints1[0] = (*m_script)[m_position++];
        value.ints1[1] = (*m_script)[m_position++];
        return *this;
    }

    // An item as category, id, modifier, material, rarity.
    ScriptValueReader &operator>>(Item &item)
    {
        if (m_position + 5 > (int)m_script->size())
            return *this;
        item.category = (*m_script)[m_position++];
        item.id = (*m_script)[m_position++];
        item.modifier = (*m_script)[m_position++];
        item.material = (*m_script)[m_position++];
        item.rarity = (*m_script)[m_position++];
        return *this;
    }

private:
    const std::vector<int> *m_script;
    int m_position;
};

// Fills the type-id-to-key tables and reads dict_en.xml (pugixml, wide character mode): each child of the document
// element has an "id" attribute and is kept by its element name, "sentence", "quest", "speech" or "name", or "macro".
Speech::Speech()
{
    s_instance = this;

    m_entityTypeIdMap[RaceElfMale] = L"ElfMale";
    m_entityTypeIdMap[RaceElfFemale] = L"ElfFemale";
    m_entityTypeIdMap[RaceHumanMale] = L"HumanMale";
    m_entityTypeIdMap[RaceHumanFemale] = L"HumanFemale";
    m_entityTypeIdMap[RaceGoblinMale] = L"GoblinMale";
    m_entityTypeIdMap[RaceGoblinFemale] = L"GoblinFemale";
    m_entityTypeIdMap[RaceBullterrier] = L"Bullterrier";
    m_entityTypeIdMap[RaceLizardmanMale] = L"LizardmanMale";
    m_entityTypeIdMap[RaceLizardmanFemale] = L"LizardmanFemale";
    m_entityTypeIdMap[RaceFrogmanMale] = L"FrogmanMale";
    m_entityTypeIdMap[RaceFrogmanFemale] = L"FrogmanFemale";
    m_entityTypeIdMap[RaceZombie] = L"Zombie";
    m_entityTypeIdMap[RaceBandit] = L"Bandit";
    m_entityTypeIdMap[RaceOldMan] = L"OldMan";
    m_entityTypeIdMap[RaceWizard] = L"Wizard";
    m_entityTypeIdMap[RaceSorcerer] = L"Sorcerer";
    m_entityTypeIdMap[RaceNecromancer] = L"Necromancer";
    m_entityTypeIdMap[RaceTroll] = L"Troll";
    m_entityTypeIdMap[RaceHellDemon] = L"HellDemon";
    m_entityTypeIdMap[RaceGolem] = L"Golem";
    m_entityTypeIdMap[RaceEmberGolem] = L"EmberGolem";
    m_entityTypeIdMap[RaceSnowGolem] = L"SnowGolem";
    m_entityTypeIdMap[RaceWerewolf] = L"Werewolf";
    m_entityTypeIdMap[RaceDemonMarauder] = L"DemonMarauder";
    m_entityTypeIdMap[RaceFlyingEye] = L"FlyingEye";
    m_entityTypeIdMap[RaceDarkTroll] = L"DarkTroll";
    m_entityTypeIdMap[RaceYeti] = L"Yeti";
    m_entityTypeIdMap[RaceOgre] = L"Ogre";
    m_entityTypeIdMap[RaceRockling] = L"Rockling";
    m_entityTypeIdMap[RaceVampire] = L"Vampire";
    m_entityTypeIdMap[RaceSpitter] = L"Spitter";
    m_entityTypeIdMap[RaceSpikeCreature] = L"SpikeCreature";
    m_entityTypeIdMap[RaceAnubis] = L"Anubis";
    m_entityTypeIdMap[RaceHorus] = L"Horus";
    m_entityTypeIdMap[RaceJester] = L"Jester";
    m_entityTypeIdMap[RaceDarkJester] = L"DarkJester";
    m_entityTypeIdMap[RaceSpectrino] = L"Spectrino";
    m_entityTypeIdMap[RaceDjinn] = L"Djinn";
    m_entityTypeIdMap[RaceMinotaur] = L"Minotaur";
    m_entityTypeIdMap[RaceImp] = L"Imp";
    m_entityTypeIdMap[RaceDragon] = L"Dragon";
    m_entityTypeIdMap[RaceLich] = L"Lich";
    m_entityTypeIdMap[RaceDwarfMale] = L"DwarfMale";
    m_entityTypeIdMap[RaceDwarfFemale] = L"DwarfFemale";
    m_entityTypeIdMap[RaceHornet] = L"Hornet";
    m_entityTypeIdMap[RaceInsectGuard] = L"InsectGuard";
    m_entityTypeIdMap[RaceMantis] = L"Mantis";
    m_entityTypeIdMap[RaceGinseng] = L"Ginseng";
    m_entityTypeIdMap[RaceFly] = L"Fly";
    m_entityTypeIdMap[RaceMidge] = L"Midge";
    m_entityTypeIdMap[RaceMosquito] = L"Mosquito";
    m_entityTypeIdMap[RaceSeagull] = L"Seagull";
    m_entityTypeIdMap[RaceRadishCreature] = L"RadishCreature";
    m_entityTypeIdMap[RaceRadishCreatureSprout] = L"RadishCreatureSprout";
    m_entityTypeIdMap[RacePlantCreatureSprout] = L"PlantCreatureSprout";
    m_entityTypeIdMap[RaceBabyElephant] = L"BabyElephant";
    m_entityTypeIdMap[RaceBabyMammoth] = L"BabyMammoth";
    m_entityTypeIdMap[RaceFlamingo] = L"Flamingo";
    m_entityTypeIdMap[RaceToucan] = L"Toucan";
    m_entityTypeIdMap[RacePlantCreature] = L"PlantCreature";
    m_entityTypeIdMap[RaceOnionling] = L"Onionling";
    m_entityTypeIdMap[RaceDesertOnionling] = L"DesertOnionling";
    m_entityTypeIdMap[RaceDevourer] = L"Devourer";
    m_entityTypeIdMap[RaceCrab] = L"Crab";
    m_entityTypeIdMap[RaceSeaCrab] = L"SeaCrab";
    m_entityTypeIdMap[RaceCrawler] = L"Crawler";
    m_entityTypeIdMap[RaceBarkBeetle] = L"BarkBeetle";
    m_entityTypeIdMap[RaceCaterpillar] = L"Caterpillar";
    m_entityTypeIdMap[RaceEarthCaterpillar] = L"EarthCaterpillar";
    m_entityTypeIdMap[RaceSnail] = L"Snail";
    m_entityTypeIdMap[RaceFireBeetle] = L"FireBeetle";
    m_entityTypeIdMap[RaceSnoutBeetle] = L"SnoutBeetle";
    m_entityTypeIdMap[RaceLemonBeetle] = L"LemonBeetle";
    m_entityTypeIdMap[RaceLemonFish] = L"LemonFish";
    m_entityTypeIdMap[RaceSapphireFish] = L"SapphireFish";
    m_entityTypeIdMap[RaceShark] = L"Shark";
    m_entityTypeIdMap[RaceLanternFish] = L"LanternFish";
    m_entityTypeIdMap[RaceMawFish] = L"MawFish";
    m_entityTypeIdMap[RacePiranha] = L"Piranha";
    m_entityTypeIdMap[RaceBlowfish] = L"Blowfish";
    m_entityTypeIdMap[RaceSeahorse] = L"Seahorse";
    m_entityTypeIdMap[RaceFrog] = L"Frog";
    m_entityTypeIdMap[RaceMole] = L"Mole";
    m_entityTypeIdMap[RaceBiter] = L"Biter";
    m_entityTypeIdMap[RaceSquirrel] = L"Squirrel";
    m_entityTypeIdMap[RaceRaccoon] = L"Raccoon";
    m_entityTypeIdMap[RaceDuckbill] = L"Duckbill";
    m_entityTypeIdMap[RaceCrocodile] = L"Crocodile";
    m_entityTypeIdMap[RaceOwl] = L"Owl";
    m_entityTypeIdMap[RacePenguin] = L"Penguin";
    m_entityTypeIdMap[RaceCrow] = L"Crow";
    m_entityTypeIdMap[RaceParrot] = L"Parrot";
    m_entityTypeIdMap[RaceSaurian] = L"Saurian";
    m_entityTypeIdMap[RaceRuneGiant] = L"RuneGiant";
    m_entityTypeIdMap[RaceCyclops] = L"Cyclops";
    m_entityTypeIdMap[RaceMammoth] = L"Mammoth";
    m_entityTypeIdMap[RaceWitch] = L"Witch";
    m_entityTypeIdMap[RaceGnobold] = L"Gnobold";
    m_entityTypeIdMap[RaceGnome] = L"Gnome";
    m_entityTypeIdMap[RaceSupplierGnome] = L"Gnome";
    m_entityTypeIdMap[RaceSnowGnobold] = L"SnowGnobold";
    m_entityTypeIdMap[RaceInsectoid] = L"Insectoid";
    m_entityTypeIdMap[RaceGnoll] = L"Gnoll";
    m_entityTypeIdMap[RacePolarGnoll] = L"PolarGnoll";
    m_entityTypeIdMap[RaceMermaid] = L"Mermaid";
    m_entityTypeIdMap[RaceMerman] = L"Merman";
    m_entityTypeIdMap[RaceBat] = L"Bat";
    m_entityTypeIdMap[RaceOrcMale] = L"OrcMale";
    m_entityTypeIdMap[RaceOrcFemale] = L"OrcFemale";
    m_entityTypeIdMap[RaceEnhancedOrcMale] = L"OrcMale";
    m_entityTypeIdMap[RaceEnhancedOrcFemale] = L"OrcFemale";
    m_entityTypeIdMap[RaceGreenSlime] = L"GreenSlime";
    m_entityTypeIdMap[RacePinkSlime] = L"PinkSlime";
    m_entityTypeIdMap[RaceYellowSlime] = L"YellowSlime";
    m_entityTypeIdMap[RaceBlueSlime] = L"BlueSlime";
    m_entityTypeIdMap[RaceFrightener] = L"Frightener";
    m_entityTypeIdMap[RaceSandHorror] = L"SandHorror";
    m_entityTypeIdMap[RaceBush] = L"Bush";
    m_entityTypeIdMap[RaceSnowBush] = L"SnowBush";
    m_entityTypeIdMap[RaceSnowBerryBush] = L"SnowBerryBush";
    m_entityTypeIdMap[RaceThornTree] = L"ThornTree";
    m_entityTypeIdMap[RaceCactus] = L"Cactus";
    m_entityTypeIdMap[RaceDummy] = L"Dummy";
    m_entityTypeIdMap[RaceAim] = L"Aim";
    m_entityTypeIdMap[RaceDemonPortal] = L"DemonPortal";
    m_entityTypeIdMap[RaceSlimeWell] = L"SlimeWell";
    m_entityTypeIdMap[RaceHornetsNest] = L"HornetsNest";
    m_entityTypeIdMap[RaceUnholyTomb] = L"UnholyTomb";
    m_entityTypeIdMap[RaceBonePile] = L"BonePile";
    m_entityTypeIdMap[RaceMolehill] = L"Molehill";
    m_entityTypeIdMap[RaceEarthPile] = L"EarthPile";
    m_entityTypeIdMap[RaceCottonPlant] = L"CottonPlant";
    m_entityTypeIdMap[RaceScrub] = L"Scrub";
    m_entityTypeIdMap[RaceCobwebScrub] = L"CobwebScrub";
    m_entityTypeIdMap[RaceFireScrub] = L"FireScrub";
    m_entityTypeIdMap[RaceBomb] = L"Bomb";
    m_entityTypeIdMap[RaceUndeadMale] = L"UndeadMale";
    m_entityTypeIdMap[RaceUndeadFemale] = L"UndeadFemale";
    m_entityTypeIdMap[RaceSkeleton] = L"Skeleton";
    m_entityTypeIdMap[RaceSkeletonKnight] = L"SkeletonKnight";
    m_entityTypeIdMap[RaceSkeletonHorse] = L"SkeletonHorse";
    m_entityTypeIdMap[RacePlainRunner] = L"PlainRunner";
    m_entityTypeIdMap[RaceLeafRunner] = L"LeafRunner";
    m_entityTypeIdMap[RaceSnowRunner] = L"SnowRunner";
    m_entityTypeIdMap[RaceDesertRunner] = L"DesertRunner";
    m_entityTypeIdMap[RacePeacock] = L"Peacock";
    m_entityTypeIdMap[RaceBunny] = L"Bunny";
    m_entityTypeIdMap[RacePorcupine] = L"Porcupine";
    m_entityTypeIdMap[RaceCollie] = L"Collie";
    m_entityTypeIdMap[RaceShepherdDog] = L"ShepherdDog";
    m_entityTypeIdMap[RaceAlpaca] = L"Alpaca";
    m_entityTypeIdMap[RaceBrownAlpaca] = L"BrownAlpaca";
    m_entityTypeIdMap[RaceSkullBull] = L"SkullBull";
    m_entityTypeIdMap[RaceEgg] = L"Egg";
    m_entityTypeIdMap[RaceTurtle] = L"Turtle";
    m_entityTypeIdMap[RaceLavaTurtle] = L"LavaTurtle";
    m_entityTypeIdMap[RaceBlueTurtle] = L"BlueTurtle";
    m_entityTypeIdMap[RaceTerrier] = L"Terrier";
    m_entityTypeIdMap[RaceScottishTerrier] = L"ScottishTerrier";
    m_entityTypeIdMap[RacePanther] = L"Panther";
    m_entityTypeIdMap[RaceSnowLeopard] = L"SnowLeopard";
    m_entityTypeIdMap[RaceCat] = L"Cat";
    m_entityTypeIdMap[RaceBrownCat] = L"BrownCat";
    m_entityTypeIdMap[RaceWhiteCat] = L"WhiteCat";
    m_entityTypeIdMap[RaceWolf] = L"Wolf";
    m_entityTypeIdMap[RaceHyena] = L"Hyena";
    m_entityTypeIdMap[RaceGiraffe] = L"Giraffe";
    m_entityTypeIdMap[RaceMakake] = L"Makake";
    m_entityTypeIdMap[RaceSkeletonDog] = L"SkeletonDog";
    m_entityTypeIdMap[RaceSavageDog] = L"SavageDog";
    m_entityTypeIdMap[RaceAlphaDog] = L"AlphaDog";
    m_entityTypeIdMap[RaceWarthog] = L"Warthog";
    m_entityTypeIdMap[RaceAntelope] = L"Antelope";
    m_entityTypeIdMap[RacePig] = L"Pig";
    m_entityTypeIdMap[RaceSheep] = L"Sheep";
    m_entityTypeIdMap[RaceHorse] = L"Horse";
    m_entityTypeIdMap[RaceGoldKnightHorse] = L"GoldKnightHorse";
    m_entityTypeIdMap[RaceCow] = L"Cow";
    m_entityTypeIdMap[RaceCamel] = L"Camel";
    m_entityTypeIdMap[RaceMonkey] = L"Monkey";
    m_entityTypeIdMap[RaceChicken] = L"Chicken";
    m_entityTypeIdMap[RaceMosquito] = L"Mosquito";
    m_entityTypeIdMap[RacePorcupine] = L"Porcupine";
    m_entityTypeIdMap[RaceVase] = L"Vase";
    m_entityTypeIdMap[RaceIronDeposit] = L"IronDeposit";
    m_entityTypeIdMap[RaceGoldDeposit] = L"GoldDeposit";
    m_entityTypeIdMap[RaceEmeraldDeposit] = L"EmeraldDeposit";
    m_entityTypeIdMap[RaceSapphireDeposit] = L"SapphireDeposit";
    m_entityTypeIdMap[RaceRubyDeposit] = L"RubyDeposit";
    m_entityTypeIdMap[RaceDiamondDeposit] = L"DiamondDeposit";
    m_entityTypeIdMap[RaceSilverDeposit] = L"SilverDeposit";
    m_entityTypeIdMap[RaceSandstoneDeposit] = L"SandstoneDeposit";
    m_entityTypeIdMap[RaceIceCrystalDeposit] = L"IceCrystalDeposit";
    m_entityTypeIdMap[RaceScarecrow] = L"Scarecrow";
    m_entityTypeIdMap[RaceNomadMale] = L"NomadMale";
    m_entityTypeIdMap[RaceNomadFemale] = L"NomadFemale";
    m_entityTypeIdMap[RaceDarkCultistMale] = L"DarkCultistMale";
    m_entityTypeIdMap[RaceDarkCultistFemale] = L"DarkCultistFemale";
    m_entityTypeIdMap[RaceBumblebee] = L"Bumblebee";
    m_entityTypeIdMap[RaceKoala] = L"Koala";
    m_entityTypeIdMap[RaceBeaver] = L"Beaver";
    m_entityTypeIdMap[RaceMoose] = L"Moose";
    m_entityTypeIdMap[RaceManaDeer] = L"ManaDeer";
    m_entityTypeIdMap[RaceChiling] = L"Chiling";
    m_entityTypeIdMap[RaceChilingSprout] = L"ChilingSprout";
    m_entityTypeIdMap[RaceHabanero] = L"Habanero";
    m_entityTypeIdMap[RaceBloomling] = L"Bloomling";
    m_entityTypeIdMap[RaceZebra] = L"Zebra";
    m_entityTypeIdMap[RaceElephant] = L"Elephant";
    m_entityTypeIdMap[RaceIceling] = L"Iceling";
    m_entityTypeIdMap[RaceRhino] = L"Rhino";
    m_entityTypeIdMap[RaceLionMale] = L"LionMale";
    m_entityTypeIdMap[RaceLionFemale] = L"LionFemale";
    m_entityTypeIdMap[RaceSnowLionMale] = L"SnowLionMale";
    m_entityTypeIdMap[RaceSnowLionFemale] = L"SnowLionFemale";
    m_entityTypeIdMap[RaceSilverLion] = L"SilverLion";
    m_entityTypeIdMap[RaceOstrich] = L"Ostrich";
    m_entityTypeIdMap[RaceWoodChest] = L"WoodChest";
    m_entityTypeIdMap[RaceSkullChest] = L"SkullChest";
    m_entityTypeIdMap[RaceObsidianChest] = L"ObsidianChest";
    m_entityTypeIdMap[RaceBoneChest] = L"BoneChest";
    m_entityTypeIdMap[RaceCampfire] = L"Campfire";
    m_entityTypeIdMap[RaceSleepingMat] = L"SleepingMat";
    m_entityTypeIdMap[RaceTent] = L"Tent";
    m_entityTypeIdMap[RaceBeachTowel] = L"BeachTowel";
    m_entityTypeIdMap[RaceSunshade] = L"Sunshade";
    m_entityTypeIdMap[RaceSign] = L"Sign";
    m_entityTypeIdMap[RaceSwimmingSign] = L"Sign";
    m_entityTypeIdMap[RaceCage1] = L"Cage1";
    m_entityTypeIdMap[RaceCage2] = L"Cage2";
    m_entityTypeIdMap[RaceCage3] = L"Cage3";
    m_entityTypeIdMap[RaceCage4] = L"Cage4";
    m_entityTypeIdMap[RaceCauldron] = L"Cauldron";
    m_entityTypeIdMap[RaceSeeker] = L"Seeker";
    m_entityTypeIdMap[RaceBrazier] = L"Brazier";
    m_entityTypeIdMap[RaceGhostBrazier] = L"GhostBrazier";
    m_entityTypeIdMap[RaceUnholyBonfire] = L"UnholyBonfire";
    m_entityTypeIdMap[RaceWagon] = L"Wagon";
    m_entityTypeIdMap[RaceWitchWagon] = L"WitchWagon";
    m_entityTypeIdMap[RaceRuneCube] = L"RuneCube";
    m_entityTypeIdMap[RacePirateMale] = L"PirateMale";
    m_entityTypeIdMap[RacePirateFemale] = L"PirateFemale";
    m_entityTypeIdMap[RaceEnhancedPirateMale] = L"PirateMale";
    m_entityTypeIdMap[RaceTreasure] = L"Treasure";
    m_entityTypeIdMap[RaceTreasureStatue] = L"TreasureStatue";
    m_entityTypeIdMap[RaceTreasureVase] = L"TreasureVase";
    m_entityTypeIdMap[RaceTreasureJewel] = L"TreasureJewel";
    m_entityTypeIdMap[RaceDemonIdol] = L"DemonIdol";
    m_entityTypeIdMap[RaceMagicCrystal] = L"MagicCrystal";
    m_entityTypeIdMap[RaceBloodaxeClanBanner] = L"BloodaxeClanBanner";
    m_entityTypeIdMap[RaceStoneTablet] = L"StoneTablet";
    m_entityTypeIdMap[RaceLever] = L"Lever";
    m_entityTypeIdMap[RaceLockedLever] = L"LockedLever";
    m_entityTypeIdMap[RaceEmpireSoldierMale] = L"EmpireSoldierMale";
    m_entityTypeIdMap[RaceEmpireSoldierFemale] = L"EmpireSoldierFemale";
    m_entityTypeIdMap[RaceTombstone] = L"Tombstone";
    m_entityTypeIdMap[RaceManaPump] = L"ManaPump";
    m_entityTypeIdMap[RaceManaInductor] = L"ManaInductor";
    m_entityTypeIdMap[RaceManaGenerator] = L"ManaGenerator";
    m_entityTypeIdMap[RaceSack] = L"Sack";
    m_entityTypeIdMap[RaceBarrel] = L"Barrel";
    m_entityTypeIdMap[RaceBarrelTnt] = L"Barrel";
    m_entityTypeIdMap[RaceCrate] = L"Crate";
    m_entityTypeIdMap[RaceGate] = L"Gate";
    m_entityTypeIdMap[RaceLockedGate] = L"Gate";
    m_entityTypeIdMap[RaceRuneGate] = L"Gate";
    m_entityTypeIdMap[RaceStoneGate] = L"Gate";
    m_entityTypeIdMap[RaceWall] = L"Wall";
    m_entityTypeIdMap[RaceSpiderweb] = L"Spiderweb";
    m_entityTypeIdMap[RaceMagicBarrier] = L"MagicBarrier";
    m_entityTypeIdMap[RaceEmpress] = L"Empress";
    m_entityTypeIdMap[RaceArchdruid] = L"Archdruid";
    m_entityTypeIdMap[RaceHalfDemon] = L"HalfDemon";
    m_entityTypeIdMap[RaceGhostPreacher] = L"GhostPreacher";
    m_entityTypeIdMap[RaceLegendaryPaladin] = L"LegendaryPaladin";
    m_entityTypeIdMap[RaceAngel] = L"Angel";
    m_entityTypeIdMap[RaceManaMech] = L"ManaMech";
    m_entityTypeIdMap[RaceTreant] = L"Treant";
    m_entityTypeIdMap[RaceFireRunestone] = L"FireRunestone";
    m_entityTypeIdMap[RaceWaterRunestone] = L"WaterRunestone";
    m_entityTypeIdMap[RaceIceRunestone] = L"IceRunestone";
    m_entityTypeIdMap[RaceWindRunestone] = L"WindRunestone";
    m_entityTypeIdMap[RaceRunePortal] = L"RunePortal";
    m_entityTypeIdMap[RaceMonstrosity] = L"Monstrosity";
    m_entityTypeIdMap[RaceSkullSlater] = L"SkullSlater";
    m_entityTypeIdMap[RaceTortureRack] = L"TortureRack";
    m_entityTypeIdMap[RaceLegIrons] = L"LegIrons";
    m_entityTypeIdMap[RacePole] = L"Pole";
    m_entityTypeIdMap[RaceCeilingChain] = L"CeilingChain";
    m_entityTypeIdMap[RaceWraith] = L"Wraith";
    m_entityTypeIdMap[RaceShrineOfTheLight] = L"ShrineOfTheLight";
    m_entityTypeIdMap[RaceFootprints] = L"Footprints";
    m_entityTypeIdMap[RaceHorseFootprints] = L"HorseFootprints";
    m_entityTypeIdMap[RaceMonsterFootprints] = L"MonsterFootprints";
    m_entityTypeIdMap[RaceSarcophagus] = L"Sarcophagus";
    m_entityTypeIdMap[RacePedestal] = L"Pedestal";
    m_entityTypeIdMap[RaceSocket] = L"Socket";
    m_entityTypeIdMap[RaceBirdStatue] = L"BirdStatue";
    m_entityTypeIdMap[RaceMourningStatue] = L"MourningStatue";
    m_entityTypeIdMap[RaceEagle] = L"Eagle";
    m_entityTypeIdMap[RaceCreatorMale] = L"CreatorMale";
    m_entityTypeIdMap[RaceCreatorFemale] = L"CreatorFemale";
    m_entityTypeIdMap[RaceFamilyGhost] = L"Ghost";
    m_entityTypeIdMap[RaceFamilyKnight] = L"Knight";
    m_entityTypeIdMap[RaceFamilyDruid] = L"Druid";
    m_entityTypeIdMap[RaceFamilyDemon] = L"Demon";
    m_entityTypeIdMap[RaceFamilySteelEmpireForce] = L"SteelEmpireForce";
    m_entityTypeIdMap[RaceFamilyCultOfDoomAdherer] = L"CultOfDoomAdherer";
    m_entityTypeIdMap[RaceFamilyUnholyPactFollower] = L"UnholyPactFollower";
    m_entityTypeIdMap[RaceFamilyBandit] = L"Bandit";
    m_entityTypeIdMap[RaceFamilyAssassin] = L"Assassin";
    m_entityTypeIdMap[RaceFamilyGoods] = L"Goods";

    m_itemIdentifierMap[std::make_pair(ItemCategoryFeet, (int)FeetBoots)] = L"Boots";
    m_itemIdentifierMap[std::make_pair(ItemCategoryHands, (int)HandsGloves)] = L"Gloves";
    m_itemIdentifierMap[std::make_pair(ItemCategoryChest, (int)ChestChestArmor)] = L"ChestArmor";
    m_itemIdentifierMap[std::make_pair(ItemCategoryShoulder, (int)ShoulderShoulderArmor)] = L"ShoulderArmor";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPet, (int)PetItemElfMale)] = L"Pet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryNeck, (int)NeckAmulet)] = L"Amulet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryRing, (int)RingRing)] = L"Ring";
    m_itemIdentifierMap[std::make_pair(ItemCategoryFormula, (int)FormulaFormula)] = L"Formula";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponSword)] = L"Sword";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponDagger)] = L"Dagger";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponFist)] = L"Fist";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponMace)] = L"Mace";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponAxe)] = L"Axe";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponBow)] = L"Bow";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponCrossbow)] = L"Crossbow";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponBoomerang)] = L"Boomerang";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponArrows)] = L"Arrows";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponArrow)] = L"Arrow";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponTorch)] = L"Torch";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponStaff)] = L"Staff";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponWand)] = L"Wand";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponBracelet)] = L"Bracelet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponShield)] = L"Shield";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponGreatsword)] = L"Greatsword";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponGreataxe)] = L"Greataxe";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponGreatmace)] = L"Greatmace";
    m_itemIdentifierMap[std::make_pair(ItemCategoryWeapon, (int)WeaponLongsword)] = L"Longsword";
    m_itemIdentifierMap[std::make_pair(ItemCategoryCoin, (int)CoinCoin)] = L"Coin";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPlatinumCoin, (int)PlatinumCoinPlatinumCoin)] = L"PlatinumCoin";
    m_itemIdentifierMap[std::make_pair(ItemCategoryCandle, (int)CandleItemCandle)] = L"Candle";
    m_itemIdentifierMap[std::make_pair(ItemCategoryCandle, (int)CandleItemUndead)] = L"Candle";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemHangGlider)] = L"HangGlider";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemBoat)] = L"Boat";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemReins)] = L"Reins";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemMagicCraftingBook)] = L"MagicCraftingBook";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemRareCraftingBook)] = L"RareCraftingBook";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemEpicCraftingBook)] = L"EpicCraftingBook";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemLegendaryCraftingBook)] =
        L"LegendaryCraftingBook";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemClimbingSpikes)] = L"ClimbingSpikes";
    m_itemIdentifierMap[std::make_pair(ItemCategoryLamp, (int)LampLamp)] = L"Lamp";
    m_itemIdentifierMap[std::make_pair(ItemCategoryManaCube, (int)ManaCubeManaCube)] = L"ManaCube";
    m_itemIdentifierMap[std::make_pair(ItemCategoryGoldBag, (int)GoldBagGoldBag)] = L"GoldBag";
    m_itemIdentifierMap[std::make_pair(ItemCategoryManaSpirit, (int)ManaSpiritManaSpirit)] = L"ManaSpirit";
    for (int i = 0; i < RaceSupplierGnome + 1; i++) {
        m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, i)] = L"Bait";
    }
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBlueSlime)] = L"BlueJelly";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceGreenSlime)] = L"GreenJelly";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePinkSlime)] = L"PinkJelly";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceYellowSlime)] = L"YellowJelly";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBunny)] = L"Carrot";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePig)] = L"PumpkinMash";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceWarthog)] = L"BananaMash";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCat)] = L"Candy";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceOwl)] = L"Lollipop";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePenguin)] = L"Softice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceMole)] = L"ChocolateDonut";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSheep)] = L"CottonCandy";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceHornet)] = L"Popcorn";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceChicken)] = L"CerealBar";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSquirrel)] = L"StrawberryCake";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceRaccoon)] = L"ChocolateCake";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBrownAlpaca)] = L"ChocolateCupcake";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceAlpaca)] = L"VanillaCupcake";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceMonkey)] = L"BananaSplit";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCaterpillar)] = L"MixedSalad";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceEarthCaterpillar)] = L"RadicchioSalad";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSnail)] = L"CabbageRolls";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceScottishTerrier)] = L"Croissant";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBarkBeetle)] = L"Bread";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSnoutBeetle)] = L"Lolly";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceLemonBeetle)] = L"LemonTart";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePeacock)] = L"ChocolateCookie";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCollie)] = L"BubbleGum";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCrow)] = L"LicoriceCandy";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceTurtle)] = L"CinnamonRole";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCrocodile)] = L"AppleRing";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceTerrier)] = L"Waffle";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSpitter)] = L"WaterIce";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCamel)] = L"DateCookie";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceHorse)] = L"CandiedApple";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceCrab)] = L"StrawberryCocktail";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePlainRunner)] = L"MilkChocolateBar";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceDesertRunner)] = L"CaramelChocolateBar";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceLeafRunner)] = L"MintChocolateBar";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSnowRunner)] = L"WhiteChocolateBar";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceDuckbill)] = L"SugarCandy";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePorcupine)] = L"BlackberryMarmelade";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceSeagull)] = L"SaltedCaramel";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceParrot)] = L"GingerTartlet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBat)] = L"MangoJuice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceFly)] = L"FruitBasket";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceMidge)] = L"MelonIceCream";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceMosquito)] = L"BloodOrangeJuice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBiter)] = L"Pancakes";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceFireBeetle)] = L"Curry";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBumblebee)] = L"BiscuitRole";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBeaver)] = L"Buckhorn";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceKoala)] = L"EucalyptusCandy";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceRadishCreatureSprout)] = L"MineralWater";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RacePlantCreatureSprout)] = L"SpringWater";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBabyElephant)] = L"Peanut";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceBabyMammoth)] = L"ChocolateIceCream";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPetFood, (int)RaceFlamingo)] = L"RaspberryJuice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableLifePotion)] = L"LifePotion";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableGreaterLifePotion)] =
        L"GreaterLifePotion";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableCactusPotion)] = L"CactusPotion";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableEssenceOfWisdom)] = L"EssenceOfWisdom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableGinsengSoup)] = L"GinsengSoup";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableSnowBerryMash)] = L"SnowBerryMash";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableMushroomSpit)] = L"MushroomSpit";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumablePineappleSlice)] = L"PineappleSlice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumablePumpkinMuffin)] = L"PumpkinMuffin";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableCookie)] = L"Cookie";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableBomb)] = L"Bomb";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumablePortalStone)] = L"PortalStone";
    m_itemIdentifierMap[std::make_pair(ItemCategoryConsumable, (int)ConsumableApple)] = L"Apple";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirElixirOfLife)] = L"ElixirOfLife";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirElixirOfPower)] = L"ElixirOfPower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirElixirOfToughness)] = L"ElixirOfToughness";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirElixirOfSanity)] = L"ElixirOfSanity";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirLemonade)] = L"Lemonade";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirHotChocolate)] = L"HotChocolate";
    m_itemIdentifierMap[std::make_pair(ItemCategoryElixir, (int)ElixirGreenSmoothie)] = L"GreenSmoothie";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemNugget)] = L"Nugget";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemEmerald)] = L"Emerald";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSapphire)] = L"Sapphire";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemRuby)] = L"Ruby";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemDiamond)] = L"Diamond";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemLog)] = L"Log";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemFeather)] = L"Feather";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemHorn)] = L"Horn";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemClaw)] = L"Claw";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemFiber)] = L"Fiber";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemHair)] = L"Hair";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemCrystal)] = L"Crystal";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemYarn)] = L"Yarn";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemCobweb)] = L"Cobweb";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemCube)] = L"Cube";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemCapsule)] = L"Capsule";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemOrb)] = L"Orb";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemFlask)] = L"Flask";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSpirit)] = L"Spirit";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemMushroom)] = L"Mushroom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemShimmerMushroom)] = L"ShimmerMushroom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemGinsengRoot)] = L"GinsengRoot";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemPumpkin)] = L"Pumpkin";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemPineapple)] = L"Pineapple";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemHeartflower)] = L"Heartflower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemPricklyPear)] = L"PricklyPear";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemFrozenHeartflower)] =
        L"FrozenHeartflower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemMindflower)] = L"Mindflower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSoulFlower)] = L"SoulFlower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemManaOrchid)] = L"ManaOrchid";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSpiritflower)] = L"Spiritflower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemRadishSlice)] = L"RadishSlice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemOnionSlice)] = L"OnionSlice";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemWaterFlask)] = L"WaterFlask";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSnowBerry)] = L"SnowBerry";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemDragonRoot)] = L"DragonRoot";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemDewdrop)] = L"Dewdrop";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemSugarCube)] = L"SugarCube";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemIceCube)] = L"IceCube";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemLemon)] = L"Lemon";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemBanana)] = L"Banana";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemKale)] = L"Kale";
    m_itemIdentifierMap[std::make_pair(ItemCategoryMaterial, (int)MaterialItemCocoaBean)] = L"CocoaBean";
    m_itemIdentifierMap[std::make_pair(ItemCategoryBeak, (int)BeakBeak)] = L"Beak";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemAmulet1)] = L"Amulet1";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemAmulet2)] = L"Amulet2";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemJewelCase)] = L"JewelCase";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemGoldKey)] = L"GoldKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBoneKey)] = L"BoneKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBronzeKey)] = L"BronzeKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSilverKey)] = L"SilverKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSkullKey)] = L"SkullKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemRustyKey)] = L"RustyKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemIronKey)] = L"IronKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemScratchedKey)] = L"ScratchedKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemAdornedKey)] = L"AdornedKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemCopperKey)] = L"CopperKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemObsidianKey)] = L"ObsidianKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSteelKey)] = L"SteelKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemTwistedKey)] = L"TwistedKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSmallKey)] = L"SmallKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemHeavyKey)] = L"HeavyKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemMassiveKey)] = L"MassiveKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemWoodKey)] = L"WoodKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemStoneKey)] = L"StoneKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemTinyKey)] = L"TinyKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemDelicateKey)] = L"DelicateKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemItemTreasure)] = L"ItemTreasure";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemItemTreasureStatue)] = L"ItemTreasureStatue";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemItemTreasureJewel)] = L"ItemTreasureJewel";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemItemTreasureVase)] = L"ItemTreasureVase";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemHerb)] = L"Herb";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemFlower)] = L"Flower";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemLetter)] = L"Letter";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemDragonDrool)] = L"DragonDrool";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemFrogSlime)] = L"FrogSlime";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemGargoyleTooth)] = L"GargoyleTooth";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSlimyEye)] = L"SlimyEye";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBroom)] = L"Broom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemAnchor)] = L"Anchor";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemWalkingStick)] = L"WalkingStick";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemTrollFingernail)] = L"TrollFingernail";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSpoon)] = L"Spoon";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSocks)] = L"Socks";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemPicnicBasket)] = L"PicnicBasket";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemPan)] = L"Pan";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemLunchBox)] = L"LunchBox";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemLuckyCharm)] = L"LuckyCharm";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBelt)] = L"Belt";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemRareCrystal)] = L"RareCrystal";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemFabricScrap)] = L"FabricScrap";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemButton)] = L"Button";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemFruit)] = L"Fruit";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemNut)] = L"Nut";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemRareFiber)] = L"RareFiber";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemRareBlossom)] = L"RareBlossom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemMagicBook)] = L"MagicBook";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSpellBook)] = L"SpellBook";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemMagicallyLockedBook)] = L"MagicallyLockedBook";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBrokenWeapon)] = L"BrokenWeapon";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemEnchantedTimber)] = L"EnchantedTimber";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemGhostRemains)] = L"GhostRemains";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemSalve)] = L"Salve";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemAntivenom)] = L"Antivenom";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemPainkiller)] = L"Painkiller";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemStaminaPotion)] = L"StaminaPotion";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemMedicine)] = L"Medicine";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemNote)] = L"Note";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemScroll)] = L"Scroll";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemMap)] = L"Map";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemTablet)] = L"Tablet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemBag)] = L"Bag";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemPouch)] = L"Pouch";
    m_itemIdentifierMap[std::make_pair(ItemCategoryQuest, (int)QuestItemEternalEmber)] = L"EternalEmber";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemFlute)] = L"Flute";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemHarp)] = L"Harp";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemTreasureSpirit)] = L"TreasureSpirit";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemFiddle)] = L"Fiddle";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemBell)] = L"Bell";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemLute)] = L"Lute";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemWhistle)] = L"Whistle";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemNightingale)] = L"Nightingale";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemVoidStone)] = L"VoidStone";
    m_itemIdentifierMap[std::make_pair(ItemCategorySpecial, (int)SpecialItemGateKey)] = L"GateKey";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactVase)] = L"ArtifactVase";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactAmulet)] = L"ArtifactAmulet";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactRing)] = L"ArtifactRing";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactStone)] = L"ArtifactStone";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactGem)] = L"ArtifactGem";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactCrown)] = L"ArtifactCrown";
    m_itemIdentifierMap[std::make_pair(ItemCategoryArtifact, (int)Artifact_ArtifactScepter)] = L"ArtifactScepter";
    m_itemIdentifierMap[std::make_pair(ItemCategoryLeftovers, (int)LeftoversLeftovers)] = L"Leftovers";
    m_itemIdentifierMap[std::make_pair(ItemCategoryPainting, (int)PaintingPainting)] = L"Painting";

    m_skillTypeIdMap[AbilitySmash] = L"SkillSmash";
    m_skillTypeIdMap[AbilityCyclone] = L"SkillCyclone";
    m_skillTypeIdMap[AbilityBulwark] = L"SkillBulwark";
    m_skillTypeIdMap[AbilityHeroicShout] = L"SkillHeroicShout";
    m_skillTypeIdMap[AbilityWarFrenzy] = L"SkillWarFrenzy";
    m_skillTypeIdMap[AbilityRangerKick] = L"SkillRangerKick";
    m_skillTypeIdMap[AbilityRetreat] = L"SkillRetreat";
    m_skillTypeIdMap[AbilityAim] = L"SkillAim";
    m_skillTypeIdMap[AbilitySwiftness] = L"SkillSwiftness";
    m_skillTypeIdMap[AbilityQuicksandTrap] = L"SkillQuicksandTrap";
    m_skillTypeIdMap[AbilityShadowClone] = L"SkillShadowClone";
    m_skillTypeIdMap[AbilityFireExplosion] = L"SkillFireExplosion";
    m_skillTypeIdMap[AbilityManaShield] = L"SkillManaShield";
    m_skillTypeIdMap[AbilityTeleport] = L"SkillTeleport";
    m_skillTypeIdMap[AbilityHealingStream] = L"SkillHealingStream";
    m_skillTypeIdMap[AbilityCenteredHealingStream] = L"SkillHealingStream";
    m_skillTypeIdMap[AbilityFloat] = L"SkillFloat";
    m_skillTypeIdMap[AbilityLeftDaggerPierce] = L"SkillAttack";
    m_skillTypeIdMap[AbilityRightDaggerPierce] = L"SkillAttack";
    m_skillTypeIdMap[AbilityLeftStab] = L"SkillAttack";
    m_skillTypeIdMap[AbilityRightStab] = L"SkillAttack";
    m_skillTypeIdMap[AbilityLeftSweep] = L"SkillAttack";
    m_skillTypeIdMap[AbilityRightSweep] = L"SkillAttack";
    m_skillTypeIdMap[AbilityTwoHandedSwing] = L"SkillAttack";
    m_skillTypeIdMap[AbilityTwoHandedThrust] = L"SkillAttack";
    m_skillTypeIdMap[AbilityTwoHandedSweep] = L"SkillAttack";
    m_skillTypeIdMap[AbilityShieldAttack] = L"SkillAttack";
    m_skillTypeIdMap[AbilityBlockJab] = L"SkillAttack";
    m_skillTypeIdMap[AbilityLeftSlash] = L"SkillAttack";
    m_skillTypeIdMap[AbilityRightSlash] = L"SkillAttack";
    m_skillTypeIdMap[AbilityShoot] = L"SkillAttack";
    m_skillTypeIdMap[AbilityThrowBoomerang] = L"SkillAttack";
    m_skillTypeIdMap[AbilityChargedBoomerang] = L"SkillChargedBoomerang";
    m_skillTypeIdMap[AbilitySalvo] = L"SkillSalvo";
    m_skillTypeIdMap[AbilityChargedShot] = L"SkillChargedShot";
    m_skillTypeIdMap[AbilityCrush] = L"SkillCrush";
    m_skillTypeIdMap[AbilitySwirl] = L"SkillSwirl";
    m_skillTypeIdMap[AbilityBlock] = L"SkillBlock";
    m_skillTypeIdMap[AbilityPerforate] = L"SkillPerforate";
    m_skillTypeIdMap[AbilityAmbush] = L"SkillAmbush";
    m_skillTypeIdMap[AbilityKick] = L"SkillKick";
    m_skillTypeIdMap[AbilityIntercept] = L"SkillIntercept";
    m_skillTypeIdMap[AbilityShuriken] = L"SkillShuriken";
    m_skillTypeIdMap[AbilityCamouflage] = L"SkillCamouflage";
    m_skillTypeIdMap[AbilitySneak] = L"SkillSneak";
    m_skillTypeIdMap[AbilityAssassinate] = L"SkillAssassinate";
    m_skillTypeIdMap[AbilityNinjutsu] = L"SkillNinjutsu";
    m_skillTypeIdMap[AbilityFlameRush] = L"SkillFlameRush";
    m_skillTypeIdMap[AbilityWaterBubbles] = L"SkillWaterBubbles";
    m_skillTypeIdMap[AbilityFireBreath] = L"SkillFireBreath";
    m_skillTypeIdMap[AbilityLavaPuddle] = L"SkillLavaPuddle";
    m_skillTypeIdMap[AbilityFireMissiles] = L"SkillFireMissiles";
    m_skillTypeIdMap[AbilityHeroicShout] = L"SkillHeroicShout";
    m_skillTypeIdMap[AbilityRockFist] = L"SkillRockFist";
    m_skillTypeIdMap[AbilitySprint] = L"SkillSprint";
    m_skillTypeIdMap[AbilityThrowPoisonVial] = L"SkillThrowPoisonVial";
    m_skillTypeIdMap[AbilityFireSwirl] = L"SkillFireSwirl";
    m_skillTypeIdMap[AbilityFireVortex] = L"SkillFireVortex";
    m_skillTypeIdMap[AbilityMonsterFirebolt] = L"SkillFirebolt";
    m_skillTypeIdMap[AbilityFirebolt] = L"SkillFirebolt";
    m_skillTypeIdMap[AbilityBraceletFirebolt] = L"SkillFirebolt";
    m_skillTypeIdMap[AbilityFireball] = L"SkillFireball";
    m_skillTypeIdMap[AbilityFireRay] = L"SkillFireRay";
    m_skillTypeIdMap[AbilityFireBeam] = L"SkillFireBeam";
    m_skillTypeIdMap[AbilityWaterSwirl] = L"SkillWaterSwirl";
    m_skillTypeIdMap[AbilityWaterVortex] = L"SkillWaterVortex";
    m_skillTypeIdMap[AbilityMonsterWaterbolt] = L"SkillWaterbolt";
    m_skillTypeIdMap[AbilityWaterbolt] = L"SkillWaterbolt";
    m_skillTypeIdMap[AbilityBraceletWaterbolt] = L"SkillWaterbolt";
    m_skillTypeIdMap[AbilityWaterball] = L"SkillWaterball";
    m_skillTypeIdMap[AbilityWaterRay] = L"SkillWaterRay";
    m_skillTypeIdMap[AbilityWaterBeam] = L"SkillWaterBeam";
    m_skillTypeIdMap[AbilityPortCity] = L"SkillPortCity";

    m_factionTypeIdMap[FactionOrderOfTheLight] = L"OrderOfTheLight";
    m_factionTypeIdMap[FactionObsidianKnights] = L"ObsidianKnights";
    m_factionTypeIdMap[FactionBloodaxeClan] = L"BloodaxeClan";
    m_factionTypeIdMap[FactionCultOfDoom] = L"CultOfDoom";
    m_factionTypeIdMap[FactionUnholyPact] = L"UnholyPact";
    m_factionTypeIdMap[FactionSteelEmpire] = L"SteelEmpire";
    m_factionTypeIdMap[FactionExplorersGuild] = L"ExplorersGuild";
    m_factionTypeIdMap[FactionDruidsOfMana] = L"DruidsOfMana";
    m_factionTypeIdMap[FactionMonsterHunters] = L"MonsterHunters";
    m_factionTypeIdMap[FactionGang] = L"Gang";
    m_factionTypeIdMap[FactionRestlessWarrior] = L"RestlessWarrior";
    m_factionTypeIdMap[FactionRealmKingdom] = L"RealmFactionKingdom";
    m_factionTypeIdMap[FactionRealmKingdomOf] = L"RealmFactionKingdomOf";
    m_factionTypeIdMap[FactionRealmKingdoms] = L"RealmFactionKingdoms";
    m_factionTypeIdMap[FactionRealmKingdomsOf] = L"RealmFactionKingdomsOf";
    m_factionTypeIdMap[FactionRealmRealm] = L"RealmFactionRealm";
    m_factionTypeIdMap[FactionRealmRealmOf] = L"RealmFactionRealmOf";
    m_factionTypeIdMap[FactionRealmRealms] = L"RealmFactionRealms";
    m_factionTypeIdMap[FactionRealmRealmsOf] = L"RealmFactionRealmsOf";
    m_factionTypeIdMap[FactionRealmTribe] = L"RealmFactionTribe";
    m_factionTypeIdMap[FactionRealmCult] = L"RealmFactionCult";
    m_factionTypeIdMap[FactionRealmCultOf] = L"RealmFactionCultOf";

    m_rulerTypeIdMap[RulerKing] = L"RulerKing";
    m_rulerTypeIdMap[RulerQueen] = L"RulerQueen";
    m_rulerTypeIdMap[RulerCultistMale] = L"RulerCultistMale";
    m_rulerTypeIdMap[RulerCultistFemale] = L"RulerCultistFemale";
    m_rulerTypeIdMap[RulerEmperor] = L"RulerEmperor";
    m_rulerTypeIdMap[RulerEmpress] = L"RulerEmpress";
    m_rulerTypeIdMap[RulerLord] = L"RulerLord";
    m_rulerTypeIdMap[RulerLady] = L"RulerLady";
    m_rulerTypeIdMap[RulerWizard] = L"RulerWizard";
    m_rulerTypeIdMap[RulerWitch] = L"RulerWitch";
    m_rulerTypeIdMap[RulerSorcerer] = L"RulerSorcerer";
    m_rulerTypeIdMap[RulerSorceress] = L"RulerSorceress";
    m_rulerTypeIdMap[RulerPrince] = L"RulerPrince";
    m_rulerTypeIdMap[RulerPrincess] = L"RulerPrincess";
    m_rulerTypeIdMap[RulerChieftainMale] = L"RulerChieftainMale";
    m_rulerTypeIdMap[RulerChieftainFemale] = L"RulerChieftainFemale";
    m_rulerTypeIdMap[RulerSeer] = L"RulerSeer";
    m_rulerTypeIdMap[RulerSeeress] = L"RulerSeeress";
    m_rulerTypeIdMap[RulerElderMale] = L"RulerElderMale";
    m_rulerTypeIdMap[RulerElderFemale] = L"RulerElderFemale";
    m_rulerTypeIdMap[RulerShamanMale] = L"RulerShamanMale";
    m_rulerTypeIdMap[RulerShamanFemale] = L"RulerShamanFemale";
    m_rulerTypeIdMap[RulerKnightMale] = L"RulerKnightMale";
    m_rulerTypeIdMap[RulerKnightFemale] = L"RulerKnightFemale";
    m_rulerTypeIdMap[RulerPreacherMale] = L"RulerPreacherMale";
    m_rulerTypeIdMap[RulerPreacherFemale] = L"RulerPreacherFemale";

    m_specializationTypeIdMap[std::make_pair(ClassWarrior, 0)] = L"SpecializationBerserker";
    m_specializationTypeIdMap[std::make_pair(ClassWarrior, 1)] = L"SpecializationGuardian";
    m_specializationTypeIdMap[std::make_pair(ClassRanger, 0)] = L"SpecializationSniper";
    m_specializationTypeIdMap[std::make_pair(ClassRanger, 1)] = L"SpecializationScout";
    m_specializationTypeIdMap[std::make_pair(ClassMage, 0)] = L"SpecializationFire";
    m_specializationTypeIdMap[std::make_pair(ClassMage, 1)] = L"SpecializationWater";
    m_specializationTypeIdMap[std::make_pair(ClassRogue, 0)] = L"SpecializationAssassin";
    m_specializationTypeIdMap[std::make_pair(ClassRogue, 1)] = L"SpecializationNinja";

    pugi::xml_document document;
    if (!document.load_file("dict_en.xml")) {
        return;
    }
    pugi::xml_node root = document.first_child();
    for (pugi::xml_node_iterator element = root.begin(); element != root.end(); element++) {
        std::wstring name = element->name();
        std::wstring id = element->attribute(L"id").value();
        if (!id.empty()) {
            if (name == L"sentence") {
                m_sentences[id] = element->child_value();
            }
            if (name == L"quest") {
                Quest *quest = new Quest();
                m_quests[id] = quest;
                for (pugi::xml_node_iterator child = element->begin(); child != element->end(); child++) {
                    if (child->name() && child->child_value()) {
                        quest->texts[child->name()].parse(child->child_value());
                    }
                }
            }
            if (name == L"speech" || name == L"name") {
                m_speechNodeMap[id] = new SpeechText();
                m_speechNodeMap[id]->parse(element->child_value());
                for (pugi::xml_node_iterator child = element->begin(); child != element->end(); child++) {
                    if (child->name() && child->child_value()) {
                        std::wstring childText = child->child_value();
                        if (!childText.empty()) {
                            SpeechText *alternative = new SpeechText();
                            alternative->parse(childText);
                            m_speechNodeMap[id]->m_alternatives.push_back(alternative);
                        }
                    }
                }
                if (element->attribute(L"tags")) {
                    std::wstringstream tags(element->attribute(L"tags").value());
                    while (tags.good()) {
                        std::wstring tag;
                        tags >> tag;
                        if (!tag.empty()) {
                            m_speechNodeMap[id]->m_tags.push_back(tag);
                        }
                    }
                }
            }
            if (name == L"macro") {
                m_macroNodeMap[id] = new SpeechText();
                m_macroNodeMap[id]->parse(element->child_value());
            }
        }
    }
}

// Deletes the quests and the texts of the speech and macro maps.
Speech::~Speech()
{
    for (std::map<std::wstring, Quest *>::iterator it = m_quests.begin(); it != m_quests.end(); it++) {
        delete it->second;
    }
    for (std::map<std::wstring, SpeechText *>::iterator it = m_speechNodeMap.begin(); it != m_speechNodeMap.end();
         it++) {
        delete it->second;
    }
    for (std::map<std::wstring, SpeechText *>::iterator it = m_macroNodeMap.begin(); it != m_macroNodeMap.end(); it++) {
        delete it->second;
    }
}

void Speech::addCreature(const std::wstring &name, int entityType, Context &context)
{
    addTags(name, creatureKey(entityType), context);
    context.variables[std::wstring(L"@") + name] = creatureKey(entityType);
}

void Speech::addCreature(const std::wstring &name, int entityType, long long seed, Context &context)
{
    addTags(name, creatureKey(entityType), context);
    context.variables[std::wstring(L"@") + name] = creatureKey(entityType);
    context.variables[std::wstring(L"@") + name + L"_name"] = randomName((int)seed, entityType);
}

void Speech::addDirection(const std::wstring &name, const plasma::Vector<2, int> &direction, Context &context)
{
    plasma::Vector<2, float> unit(direction);
    normalize(unit);
    // The compass direction with the largest dot product.
    plasma::Vector<2, float> east(1.0f, 0.0f);
    plasma::Vector<2, float> northEast(1.0f, 1.0f);
    plasma::Vector<2, float> north(0.0f, 1.0f);
    plasma::Vector<2, float> northWest(-1.0f, 1.0f);
    plasma::Vector<2, float> west(-1.0f, 0.0f);
    plasma::Vector<2, float> southWest(-1.0f, -1.0f);
    plasma::Vector<2, float> south(0.0f, -1.0f);
    plasma::Vector<2, float> southEast(1.0f, -1.0f);
    float best = 0.0f;
    std::wstring tag;
    float dot;
    dot = east.dot(unit);
    if (dot > best) {
        tag = L"#east";
        best = dot;
    }
    normalize(northEast);
    dot = northEast.dot(unit);
    if (dot > best) {
        tag = L"#northeast";
        best = dot;
    }
    dot = north.dot(unit);
    if (dot > best) {
        tag = L"#north";
        best = dot;
    }
    normalize(northWest);
    dot = northWest.dot(unit);
    if (dot > best) {
        tag = L"#northwest";
        best = dot;
    }
    dot = west.dot(unit);
    if (dot > best) {
        tag = L"#west";
        best = dot;
    }
    normalize(southWest);
    dot = southWest.dot(unit);
    if (dot > best) {
        tag = L"#southwest";
        best = dot;
    }
    dot = south.dot(unit);
    if (dot > best) {
        tag = L"#south";
        best = dot;
    }
    normalize(southEast);
    dot = southEast.dot(unit);
    if (dot > best) {
        tag = L"#southeast";
        best = dot;
    }
    context.tags.insert(tag);
}

void Speech::addEmpty(const std::wstring &name, int biome, long long seed, Context &context)
{
    std::wstring key = emptyText();
    addTags(name, key, context);
    context.variables[std::wstring(L"@") + name] = key;
    context.variables[std::wstring(L"!@") + name] = randomName((int)seed, RaceNone);
    context.colors[std::wstring(L"@") + name] = plasma::Vector<4, float>(1.0f, 0.5f, 0.25f, 1.0f);
}

void Speech::addFaction(const std::wstring &name, int faction, long long seed, int kind, Context &context)
{
    std::wstring key = factionKey(faction);
    addTags(name, key, context);
    context.variables[std::wstring(L"@") + name] = key;
    context.variables[std::wstring(L"!@") + name] = randomName((int)seed, kind);
}

void Speech::addItem(const std::wstring &name, const Item &item, Context &context)
{
    addTags(name, getItemKey(item.category, item.id), context);
    context.variables[std::wstring(L"@") + name] = getItemKey(item.category, item.id);
    // Item-name race noise: multiplier 7 selects among the eleven races ElfMale through DwarfFemale.
    context.variables[std::wstring(L"!@") + name] = randomName(item.modifier, 7 * item.modifier % 11);
    if (item.rarity != ItemRarityWhite) {
        context.variables[std::wstring(L"@") + name + L"rarity"] = L"+" + std::to_wstring(item.rarity);
        context.colors[std::wstring(L"@") + name + L"rarity"] = item.textColor(item.region);
    }
    context.colors[std::wstring(L"@") + name] = item.textColor(item.region);
    if (item.category != ItemCategorySpecial && item.material != ItemMaterialNone) {
        addTags(L"material", getMaterialKey(item.material), context);
        context.variables[std::wstring(L"@material") + name] = getMaterialKey(item.material);
        context.tags.insert(L"#material" + name);
    }
    // Weapons and armour (and category 26) get a quality word.
    if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing) ||
        item.category == ItemCategoryEyePatch) {
        SpeechText *text = speechText(getItemKey(item.category, item.id));
        if (text) {
            std::list<std::wstring>::const_iterator tag;
            for (tag = text->m_tags.begin(); tag != text->m_tags.end(); ++tag) {
                context.tags.insert(std::wstring(L"#material") + name + L"_" + *tag);
            }
            context.variables[std::wstring(L"@material") + name] = getMaterialKey(item.material);
            context.variables[std::wstring(L"@name")] = randomName(item.modifier, 7 * item.modifier % 11);
            for (tag = text->m_tags.begin(); tag != text->m_tags.end(); ++tag) {
                context.tags.insert(std::wstring(L"#complete") + name + L"_" + *tag);
            }
            context.tags.insert(L"#complete" + name);
            context.colors[std::wstring(L"@complete") + name] = item.textColor(item.region);
            switch (item.rarity) {
            case ItemRarityWhite:
                switch (item.modifier % 10) {
                case ItemAdjective0Shabby:
                    context.variables[std::wstring(L"@complete") + name] = L"item:shabby";
                    break;
                case ItemAdjective0Plain:
                    context.variables[std::wstring(L"@complete") + name] = L"item:plain";
                    break;
                case ItemAdjective0Battered:
                    context.variables[std::wstring(L"@complete") + name] = L"item:battered";
                    break;
                case ItemAdjective0Artless:
                    context.variables[std::wstring(L"@complete") + name] = L"item:artless";
                    break;
                case ItemAdjective0Unwieldy:
                    context.variables[std::wstring(L"@complete") + name] = L"item:unwieldy";
                    break;
                case ItemAdjective0Used:
                    context.variables[std::wstring(L"@complete") + name] = L"item:used";
                    break;
                case ItemAdjective0Dusty:
                    context.variables[std::wstring(L"@complete") + name] = L"item:dusty";
                    break;
                case ItemAdjective0Scratched:
                    context.variables[std::wstring(L"@complete") + name] = L"item:scratched";
                    break;
                case ItemAdjective0Worn:
                    context.variables[std::wstring(L"@complete") + name] = L"item:worn";
                    break;
                case ItemAdjective0Common:
                    context.variables[std::wstring(L"@complete") + name] = L"item:common";
                    break;
                }
                break;
            case ItemRarityGreen:
                switch (item.modifier % 10) {
                case ItemAdjective1Adjusted:
                    context.variables[std::wstring(L"@complete") + name] = L"item:adjusted";
                    break;
                case ItemAdjective1Balanced:
                    context.variables[std::wstring(L"@complete") + name] = L"item:balanced";
                    break;
                case ItemAdjective1Battletested:
                    context.variables[std::wstring(L"@complete") + name] = L"item:battletested";
                    break;
                case ItemAdjective1Good:
                    context.variables[std::wstring(L"@complete") + name] = L"item:good";
                    break;
                case ItemAdjective1Handmade:
                    context.variables[std::wstring(L"@complete") + name] = L"item:handmade";
                    break;
                case ItemAdjective1Fair:
                    context.variables[std::wstring(L"@complete") + name] = L"item:fair";
                    break;
                case ItemAdjective1Neat:
                    context.variables[std::wstring(L"@complete") + name] = L"item:neat";
                    break;
                case ItemAdjective1Clean:
                    context.variables[std::wstring(L"@complete") + name] = L"item:clean";
                    break;
                case ItemAdjective1Undamaged:
                    context.variables[std::wstring(L"@complete") + name] = L"item:undamaged";
                    break;
                case ItemAdjective1Flawless:
                    context.variables[std::wstring(L"@complete") + name] = L"item:flawless";
                    break;
                }
                break;
            case ItemRarityBlue:
                switch (item.modifier % 10) {
                case ItemAdjective2Exceptional:
                    context.variables[std::wstring(L"@complete") + name] = L"item:exceptional";
                    break;
                case ItemAdjective2Polished:
                    context.variables[std::wstring(L"@complete") + name] = L"item:polished";
                    break;
                case ItemAdjective2Extraordinary:
                    context.variables[std::wstring(L"@complete") + name] = L"item:extraordinary";
                    break;
                case ItemAdjective2Exquisite:
                    context.variables[std::wstring(L"@complete") + name] = L"item:exquisite";
                    break;
                case ItemAdjective2Superb:
                    context.variables[std::wstring(L"@complete") + name] = L"item:superb";
                    break;
                case ItemAdjective2Unique:
                    context.variables[std::wstring(L"@complete") + name] = L"item:unique";
                    break;
                case ItemAdjective2Handsome:
                    context.variables[std::wstring(L"@complete") + name] = L"item:handsome";
                    break;
                case ItemAdjective2Grand:
                    context.variables[std::wstring(L"@complete") + name] = L"item:grand";
                    break;
                case ItemAdjective2Magic:
                    context.variables[std::wstring(L"@complete") + name] = L"item:magic";
                    break;
                case ItemAdjective2Decorated:
                    context.variables[std::wstring(L"@complete") + name] = L"item:decorated";
                    break;
                }
                break;
            case ItemRarityPurple:
                switch (item.modifier % 10) {
                case ItemAdjective3Exceptional2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:exceptional2";
                    break;
                case ItemAdjective3Polished2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:polished2";
                    break;
                case ItemAdjective3Extraordinary2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:extraordinary2";
                    break;
                case ItemAdjective3Exquisite2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:exquisite2";
                    break;
                case ItemAdjective3Superb2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:superb2";
                    break;
                case ItemAdjective3Unique2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:unique2";
                    break;
                case ItemAdjective3Handsome2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:handsome2";
                    break;
                case ItemAdjective3Grand2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:grand2";
                    break;
                case ItemAdjective3Magic2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:magic2";
                    break;
                case ItemAdjective3Decorated2:
                    context.variables[std::wstring(L"@complete") + name] = L"item:decorated2";
                    break;
                }
                break;
            case ItemRarityYellow:
                switch (item.modifier % 10) {
                case ItemAdjective4Brilliant:
                    context.variables[std::wstring(L"@complete") + name] = L"item:brilliant";
                    break;
                case ItemAdjective4Shining:
                    context.variables[std::wstring(L"@complete") + name] = L"item:shining";
                    break;
                case ItemAdjective4Magnificent:
                    context.variables[std::wstring(L"@complete") + name] = L"item:magnificent";
                    break;
                case ItemAdjective4Sublime:
                    context.variables[std::wstring(L"@complete") + name] = L"item:sublime";
                    break;
                case ItemAdjective4Pompous:
                    context.variables[std::wstring(L"@complete") + name] = L"item:pompous";
                    break;
                case ItemAdjective4Glorious:
                    context.variables[std::wstring(L"@complete") + name] = L"item:glorious";
                    break;
                case ItemAdjective4Splendid:
                    context.variables[std::wstring(L"@complete") + name] = L"item:splendid";
                    break;
                case ItemAdjective4Famous:
                    context.variables[std::wstring(L"@complete") + name] = L"item:famous";
                    break;
                case ItemAdjective4Legendary:
                    context.variables[std::wstring(L"@complete") + name] = L"item:legendary";
                    break;
                case ItemAdjective4Fabulous:
                    context.variables[std::wstring(L"@complete") + name] = L"item:fabulous";
                    break;
                }
                break;
            }
            SpeechText *quality = speechText(context.variables[std::wstring(L"@complete") + name]);
            for (const std::wstring &qualityTag : quality->m_tags) {
                context.tags.insert(std::wstring(L"#complete") + name + L"_" + qualityTag);
                std::wcout << L"tag " << qualityTag << std::endl;
            }
        }
    }
}

void Speech::addLand(const std::wstring &name, int biome, long long seed, Context &context)
{
    std::wstring key = getLandName(biome);
    addTags(name, key, context);
    context.variables[std::wstring(L"@") + name] = key;
    context.variables[std::wstring(L"!@") + name] = randomName((int)seed, RaceNone);
    context.colors[std::wstring(L"@") + name] = getLandColor(biome);
}

void Speech::addLands(const std::wstring &name, long long seed, Context &context)
{
    std::wstring key;
    if (seed != 0) {
        key = L"Lands";
    } else {
        key = L"Ocean";
    }
    addTags(name, key, context);
    context.variables[std::wstring(L"@") + name] = key;
    context.variables[std::wstring(L"!@") + name] = randomName((int)seed, RaceNone);
}

void Speech::addLocation(const std::wstring &name, int x, int y, int type, Context &context)
{
    addTags(name, getLocationKey(type), context);
    context.variables[std::wstring(L"@") + name] = getLocationKey(type);
    context.variables[std::wstring(L"!@") + name] = randomName(metroNameSeed(x, y), metroNameVariant(x, y, type));
}

void Speech::addRealm(int kind, int realm, Context &context)
{
    std::wstring key = factionKey(kind);
    addTags(L"realm", key, context);
    context.variables[L"@realm"] = key;

    static std::wstring prefixes[40] = {L"Kro", L"Ari",  L"As",  L"Ge",   L"Kur", L"Ro",  L"Thano", L"La",
                                        L"Des", L"Var",  L"An",  L"Theo", L"Ga",  L"Lan", L"Dura",  L"Dama",
                                        L"Se",  L"Thal", L"Nar", L"San",  L"Tel", L"Tal", L"Ar",    L"Mar",
                                        L"Gro", L"Yu",   L"Ja",  L"Lan",  L"Cor", L"Co",  L"Thal",  L"Tul",
                                        L"Le",  L"Ri",   L"U",   L"Va",   L"Wyn", L"Wyr", L"Cyd",   L"Cyr"};
    static std::wstring suffixes[30] = {L"no",  L"mid",  L"mod", L"mor",  L"nor", L"ion",  L"kor", L"lon",
                                        L"rok", L"tar",  L"dan", L"gor",  L"dar", L"dara", L"lan", L"ka",
                                        L"gor", L"rior", L"ria", L"mor",  L"sen", L"dor",  L"ion", L"kin",
                                        L"len", L"ber",  L"lur", L"more", L"ris", L"nis"};
    context.variables[L"@prefix"] = pickName(prefixes, 40, realm) + pickName(suffixes, 30, realm), realm += 2;

    static std::wstring suffixTags[4] = {L"suffix:ia", L"suffix:a", L"suffix:t", L"suffix:ar"};
    context.variables[L"@suffix"] = pickName(suffixTags, 4, realm);
}

void Speech::addRuler(int ruler, Context &context)
{
    std::wstring key = rulerKey(ruler);
    SpeechText *text = speechText(key);
    if (text) {
        for (std::list<std::wstring>::const_iterator tag = text->m_tags.begin(); tag != text->m_tags.end(); ++tag) {
            context.tags.insert(L"#ruler_" + *tag);
        }
        context.variables[L"@ruler"] = key;
    }
}

void Speech::addSpecialization(const std::wstring &name, int classType, int specialization, Context &context)
{
    std::wstring key = specializationKey(classType, specialization);
    addTags(name, key, context);
    context.variables[std::wstring(L"@") + name] = key;
}

void Speech::addTags(const std::wstring &name, const std::wstring &key, Context &context)
{
    // Drop the tags of whatever the name stood for before.
    std::list<std::wstring> stale;
    std::wstring prefix = L"#" + name + L"_";
    for (const std::wstring &tag : context.tags) {
        if (tag.size() >= prefix.size()) {
            if (tag.substr(0, prefix.size()).compare(prefix) == 0) {
                stale.push_back(tag);
            }
        }
    }
    for (std::list<std::wstring>::iterator tag = stale.begin(); tag != stale.end(); ++tag) {
        context.tags.erase(*tag);
    }
    SpeechText *text = speechText(key);
    if (text) {
        for (std::list<std::wstring>::const_iterator tag = text->m_tags.begin(); tag != text->m_tags.end(); ++tag) {
            context.tags.insert(std::wstring(L"#") + name + L"_" + *tag);
        }
    }
}

std::wstring Speech::createItemName(const Item &item)
{
    Item named = item.displayedItem();
    if (named.category == ItemCategoryFormula) {
        // A formula is named after the item it makes.
        Item product = named;
        product.category = (unsigned char)product.formulaCategory;
        product.formulaCategory = ItemCategoryNone;
        return std::wstring(L"Formula: ") + createItemName(product);
    }
    if (named.category == ItemCategoryGlyph) {
        return std::wstring(L"Glyph of ") + getText(skillKey(named.id), L"default", L"");
    }
    if (named.category == ItemCategoryPet) {
        return std::wstring(L"Pet: ") + creatureKey(named.id);
    }
    if (named.category == ItemCategoryQuest || named.category == ItemCategoryArtifact) {
        // Lore items: the key's text with a generated name for L"@".
        std::map<std::wstring, std::wstring> macros;
        std::set<std::wstring> tags;
        macros[L"@"] = randomName(named.modifier, named.modifier * 7 % 11);
        return generateText(getItemKey(named.category, named.id), tags, macros, 0);
    }
    // Categories without a material word, and anything of material 0.
    bool plain = named.category == ItemCategoryConsumable || named.category == ItemCategoryLeftovers ||
                 named.category == ItemCategoryCandle; // Local temporary.
    if (plain || named.material == ItemMaterialNone) {
        return getText(getItemKey(named.category, named.id), L"default", L"");
    }

    // Counted and never read: the spirits per spirit material (128..131).
    int spiritCounts[4] = {0, 0, 0, 0};
    for (int i = 0; i < named.spiritCount; i++) {
        if (named.spirits[i].material >= ItemMaterialFireSpirit) {
            spiritCounts[named.spirits[i].material - ItemMaterialFireSpirit]++;
        }
    }
    std::wstringstream stream; // never written to
    // Weapons, armour and the other crafted categories get a quality word chosen by rarity and modifier ("item:shabby"
    // ... "item:fabulous").
    if (named.category == ItemCategoryWeapon || named.category == ItemCategoryChest ||
        named.category == ItemCategoryHands || named.category == ItemCategoryFeet ||
        named.category == ItemCategoryShoulder || named.category == ItemCategoryNeck ||
        named.category == ItemCategoryRing || named.category == ItemCategoryEyePatch) {
        SpeechText *text = speechText(getItemKey(named.category, named.id));
        if (text) {
            std::map<std::wstring, std::wstring> macros;
            std::set<std::wstring> tags;
            for (std::list<std::wstring>::iterator it = text->m_tags.begin(); it != text->m_tags.end(); ++it) {
                tags.insert(L"#" + *it);
            }
            macros[L"@name"] = randomName(named.modifier, named.modifier * 7 % 11);
            macros[L"@item"] = getItemKey(named.category, named.id);
            macros[L"@materialitem"] = getMaterialKey(named.material);
            switch (named.rarity) {
            case ItemRarityWhite:
                switch (named.modifier % 10) {
                case ItemAdjective0Shabby:
                    return generateText(L"item:shabby", tags, macros, 0);
                case ItemAdjective0Plain:
                    return generateText(L"item:plain", tags, macros, 0);
                case ItemAdjective0Battered:
                    return generateText(L"item:battered", tags, macros, 0);
                case ItemAdjective0Artless:
                    return generateText(L"item:artless", tags, macros, 0);
                case ItemAdjective0Unwieldy:
                    return generateText(L"item:unwieldy", tags, macros, 0);
                case ItemAdjective0Used:
                    return generateText(L"item:used", tags, macros, 0);
                case ItemAdjective0Dusty:
                    return generateText(L"item:dusty", tags, macros, 0);
                case ItemAdjective0Scratched:
                    return generateText(L"item:scratched", tags, macros, 0);
                case ItemAdjective0Worn:
                    return generateText(L"item:worn", tags, macros, 0);
                case ItemAdjective0Common:
                    return generateText(L"item:common", tags, macros, 0);
                }
                break;
            case ItemRarityGreen:
                switch (named.modifier % 10) {
                case ItemAdjective1Adjusted:
                    return generateText(L"item:adjusted", tags, macros, 0);
                case ItemAdjective1Balanced:
                    return generateText(L"item:balanced", tags, macros, 0);
                case ItemAdjective1Battletested:
                    return generateText(L"item:battletested", tags, macros, 0);
                case ItemAdjective1Good:
                    return generateText(L"item:good", tags, macros, 0);
                case ItemAdjective1Handmade:
                    return generateText(L"item:handmade", tags, macros, 0);
                case ItemAdjective1Fair:
                    return generateText(L"item:fair", tags, macros, 0);
                case ItemAdjective1Neat:
                    return generateText(L"item:neat", tags, macros, 0);
                case ItemAdjective1Clean:
                    return generateText(L"item:clean", tags, macros, 0);
                case ItemAdjective1Undamaged:
                    return generateText(L"item:undamaged", tags, macros, 0);
                case ItemAdjective1Flawless:
                    return generateText(L"item:flawless", tags, macros, 0);
                }
                break;
            case ItemRarityBlue:
                switch (named.modifier % 10) {
                case ItemAdjective2Exceptional:
                    return generateText(L"item:exceptional", tags, macros, 0);
                case ItemAdjective2Polished:
                    return generateText(L"item:polished", tags, macros, 0);
                case ItemAdjective2Extraordinary:
                    return generateText(L"item:extraordinary", tags, macros, 0);
                case ItemAdjective2Exquisite:
                    return generateText(L"item:exquisite", tags, macros, 0);
                case ItemAdjective2Superb:
                    return generateText(L"item:superb", tags, macros, 0);
                case ItemAdjective2Unique:
                    return generateText(L"item:unique", tags, macros, 0);
                case ItemAdjective2Handsome:
                    return generateText(L"item:handsome", tags, macros, 0);
                case ItemAdjective2Grand:
                    return generateText(L"item:grand", tags, macros, 0);
                case ItemAdjective2Magic:
                    return generateText(L"item:magic", tags, macros, 0);
                case ItemAdjective2Decorated:
                    return generateText(L"item:decorated", tags, macros, 0);
                }
                break;
            case ItemRarityPurple:
                switch (named.modifier % 10) {
                case ItemAdjective3Exceptional2:
                    return generateText(L"item:exceptional2", tags, macros, 0);
                case ItemAdjective3Polished2:
                    return generateText(L"item:polished2", tags, macros, 0);
                case ItemAdjective3Extraordinary2:
                    return generateText(L"item:extraordinary2", tags, macros, 0);
                case ItemAdjective3Exquisite2:
                    return generateText(L"item:exquisite2", tags, macros, 0);
                case ItemAdjective3Superb2:
                    return generateText(L"item:superb2", tags, macros, 0);
                case ItemAdjective3Unique2:
                    return generateText(L"item:unique2", tags, macros, 0);
                case ItemAdjective3Handsome2:
                    return generateText(L"item:handsome2", tags, macros, 0);
                case ItemAdjective3Grand2:
                    return generateText(L"item:grand2", tags, macros, 0);
                case ItemAdjective3Magic2:
                    return generateText(L"item:magic2", tags, macros, 0);
                case ItemAdjective3Decorated2:
                    return generateText(L"item:decorated2", tags, macros, 0);
                }
                break;
            case ItemRarityYellow:
                switch (named.modifier % 10) {
                case ItemAdjective4Brilliant:
                    return generateText(L"item:brilliant", tags, macros, 0);
                case ItemAdjective4Shining:
                    return generateText(L"item:shining", tags, macros, 0);
                case ItemAdjective4Magnificent:
                    return generateText(L"item:magnificent", tags, macros, 0);
                case ItemAdjective4Sublime:
                    return generateText(L"item:sublime", tags, macros, 0);
                case ItemAdjective4Pompous:
                    return generateText(L"item:pompous", tags, macros, 0);
                case ItemAdjective4Glorious:
                    return generateText(L"item:glorious", tags, macros, 0);
                case ItemAdjective4Splendid:
                    return generateText(L"item:splendid", tags, macros, 0);
                case ItemAdjective4Famous:
                    return generateText(L"item:famous", tags, macros, 0);
                case ItemAdjective4Legendary:
                    return generateText(L"item:legendary", tags, macros, 0);
                case ItemAdjective4Fabulous:
                    return generateText(L"item:fabulous", tags, macros, 0);
                }
                break;
            }
        }
    }
    if (named.category == ItemCategorySpecial) {
        // Named from the formula category instead of the modifier.
        std::map<std::wstring, std::wstring> macros;
        std::set<std::wstring> tags;
        macros[L"@name"] = randomName(named.formulaCategory, named.modifier * 7 % 11);
        return generateText(getItemKey(named.category, named.id), tags, macros, 0);
    }
    SpeechText *text = speechText(getItemKey(named.category, named.id));
    if (text) {
        std::map<std::wstring, std::wstring> macros;
        std::set<std::wstring> tags;
        for (std::list<std::wstring>::iterator it = text->m_tags.begin(); it != text->m_tags.end(); ++it) {
            tags.insert(L"#" + *it);
        }
        tags.insert(L"#item");
        macros[L"@name"] = randomName(named.modifier, named.modifier * 7 % 11);
        macros[L"@item"] = getItemKey(named.category, named.id);
        return generateText(getMaterialKey(named.material), tags, macros, 0);
    }
    return L"unknown";
}

std::wstring Speech::creatureKey(int type)
{
    std::map<int, std::wstring>::iterator it = m_entityTypeIdMap.find(type);
    if (it == m_entityTypeIdMap.end()) {
        return L"";
    }
    return it->second;
}

std::wstring Speech::emptyText()
{
    return L"";
}

std::wstring expressionKey(int expression)
{
    switch (expression) {
    case ExpressionHello:
        return L"hello";
    case ExpressionHowCanIHelpYou:
        return L"how_can_i_help_you";
    case ExpressionGoodbye:
        return L"goodbye";
    case ExpressionHelp:
        return L"help";
    case ExpressionIDontKnow:
        return L"i_dont_know";
    case ExpressionHelloListener:
        return L"hello_listener";
    case ExpressionMyNameIs:
        return L"my_name_is";
    case ExpressionHisNameIs:
        return L"his_name_is";
    case ExpressionAnItem:
        return L"an_item";
    case ExpressionTellMeAboutZone:
        return L"tell_me_about_zone";
    case ExpressionWhereCanIFindTheZone:
        return L"where_can_i_find_the_zone";
    case ExpressionWhereCanIFindThatZone:
        return L"where_can_i_find_that_zone";
    case ExpressionWhereCanIFindTheZones:
        return L"where_can_i_find_the_zones";
    case ExpressionWhereCanIFindTheFirstZone:
        return L"where_can_i_find_the_first_zone";
    case ExpressionWhereCanIFindTheSecondZone:
        return L"where_can_i_find_the_second_zone";
    case ExpressionWhereCanIFindTheThirdZone:
        return L"where_can_i_find_the_third_zone";
    case ExpressionWhereCanIFindTheFourthZone:
        return L"where_can_i_find_the_fourth_zone";
    case ExpressionYouWillPay:
        return L"you_will_pay";
    case ExpressionTakeThis:
        return L"take_this";
    case ExpressionGiveMeYourMoney:
        return L"give_me_your_money";
    case ExpressionGiveMeYourItem:
        return L"give_me_your_item";
    case ExpressionThereIsABoss:
        return L"there_is_a_boss";
    case ExpressionThereIsABossInZone:
        return L"there_is_a_boss_in_zone";
    case ExpressionThereIsACreature:
        return L"there_is_a_creature";
    case ExpressionThereIsACreatureInZone:
        return L"there_is_a_creature_in_zone";
    case ExpressionThereIsAHugeCreature:
        return L"there_is_a_huge_creature";
    case ExpressionThereIsAHugeCreatureInZone:
        return L"there_is_a_huge_creature_in_zone";
    case ExpressionThereAreCreaturesInZone:
        return L"there_are_creatures_in_zone";
    case ExpressionThereAreCreaturesEverywhere:
        return L"there_are_creatures_everywhere";
    case ExpressionYouDefeatedTheCreature:
        return L"you_defeated_the_creature";
    case ExpressionYouDefeatedTheCreatures:
        return L"you_defeated_the_creatures";
    case ExpressionYouDefeatedTheCreaturesInZone:
        return L"you_defeated_the_creatures_in_zone";
    case ExpressionYouDestroyedTheObjectsInZone:
        return L"you_destroyed_the_objects_in_zone";
    case ExpressionYouDefeatedBoss:
        return L"you_defeated_boss";
    case ExpressionYouMustDefeatHim:
        return L"you_must_defeat_him";
    case ExpressionYouMustDefeatThem:
        return L"you_must_defeat_them";
    case ExpressionDefeatTheCreatures:
        return L"defeat_the_creatures";
    case ExpressionDestroyTheObject:
        return L"destroy_the_object";
    case ExpressionYouDestroyedTheObject:
        return L"you_destroyed_the_object";
    case ExpressionYouMustDestroyThem:
        return L"you_must_destroy_them";
    case ExpressionThereMustBeAWayToRepelThem:
        return L"there_must_be_a_way_to_repel_them";
    case ExpressionYouRepelledTheCreatures:
        return L"you_repelled_the_creatures";
    case ExpressionYouRepelledTheCreaturesFromTheZone:
        return L"you_repelled_the_creatures_from_the_zone";
    case ExpressionYouDiscoveredAZone:
        return L"you_discovered_a_zone";
    case ExpressionYouDiscoveredTheZone:
        return L"you_discovered_the_zone";
    case ExpressionYouShouldExploreIt:
        return L"you_should_explore_it";
    case ExpressionYouShouldExploreThisZone:
        return L"you_should_explore_this_zone";
    case ExpressionTheZoneIsDirectionOfHere:
        return L"the_zone_is_direction_of_here";
    case ExpressionAZoneIsDirectionOfHere:
        return L"a_zone_is_direction_of_here";
    case ExpressionThereIsAZoneDirectionOfHere:
        return L"there_is_a_zone_direction_of_here";
    case ExpressionThereIsAZoneDirectionOfThisZone2:
        return L"there_is_a_zone_direction_of_this_zone2";
    case ExpressionZoneItIsDirectionOfThisZone2:
        return L"zone_it_is_direction_of_this_zone2";
    case ExpressionItsDistanceAway:
        return L"its_distance_away";
    case ExpressionItsNotFarAway:
        return L"its_not_far_away";
    case ExpressionItsFurtherAway:
        return L"its_further_away";
    case ExpressionItsFarAway:
        return L"its_far_away";
    case ExpressionThereIsAZoneInZone2:
        return L"there_is_a_zone_in_zone2";
    case ExpressionThereIsAZoneInThisZone2:
        return L"there_is_a_zone_in_this_zone2";
    case ExpressionThereAreTwoZonesInThisZone2:
        return L"there_are_two_zones_in_this_zone2";
    case ExpressionThereAreThreeZonesInThisZone2:
        return L"there_are_three_zones_in_this_zone2";
    case ExpressionThereAreFourZonesInThisZone2:
        return L"there_are_four_zones_in_this_zone2";
    case ExpressionThisZoneMightHaveToDoWithAllTheCreatures:
        return L"this_zone_might_have_to_do_with_all_the_creatures";
    case ExpressionTheseZonesMightHaveToDoWithAllTheCreatures:
        return L"these_zones_might_have_to_do_with_all_the_creatures";
    case ExpressionSoYouFoundTheZone:
        return L"so_you_found_the_zone";
    case ExpressionYouFoundAnItem:
        return L"you_found_an_item";
    case ExpressionYouReceivedAnItem:
        return L"you_received_an_item";
    case ExpressionYouReceivedTheItem:
        return L"you_received_the_item";
    case ExpressionYouLearnedHowToCraftTheItem:
        return L"you_learned_how_to_craft_the_item";
    case ExpressionYouOpenedTheObjectWithTheItem:
        return L"you_opened_the_object_with_the_item";
    case ExpressionGoodWork:
        return L"good_work";
    case ExpressionYouLearnedANewSkill:
        return L"you_learned_a_new_skill";
    case ExpressionYourSkillIncreasedToLevel:
        return L"your_skill_increased_to_level";
    case ExpressionYourLevelIncreasedTo:
        return L"your_level_increased_to";
    case ExpressionLetsSeeWhatYouGot:
        return L"lets_see_what_you_got";
    case ExpressionYouFoundOneItem:
        return L"you_found_one_item";
    case ExpressionYouFoundNItems:
        return L"you_found_n_items";
    case ExpressionYourRewardIsNGoldAndAnItem:
        return L"your_reward_is_n_gold_and_an_item";
    case ExpressionItSeemsLikeYouDontHaveAnyRewardsToClaim:
        return L"it_seems_like_you_dont_have_any_rewards_to_claim";
    case ExpressionHehehe:
        return L"hehehe";
    case ExpressionBringMeNItemsAndIWillGiveYouSomethingSpecial:
        return L"bring_me_n_items_and_i_will_give_you_something_special";
    case ExpressionYouBroughtEnoughItems:
        return L"you_brought_enough_items";
    case ExpressionLetMeFuseThem:
        return L"let_me_fuse_them";
    case ExpressionICreatedAnSomethingForYou:
        return L"i_created_an_something_for_you";
    case ExpressionYourSpecializationIsNow:
        return L"your_specialization_is_now";
    case ExpressionRealmKingdom:
        return L"realm_kingdom";
    case ExpressionRealmKingdomOf:
        return L"realm_kingdom_of";
    case ExpressionRealmKingdoms:
        return L"realm_kingdoms";
    case ExpressionRealmKingdomsOf:
        return L"realm_kingdoms_of";
    case ExpressionRealmRealm:
        return L"realm_realm";
    case ExpressionRealmRealmOf:
        return L"realm_realm_of";
    case ExpressionRealmRealms:
        return L"realm_realms";
    case ExpressionRealmRealmsOf:
        return L"realm_realms_of";
    case ExpressionRealmTribe:
        return L"realm_tribe";
    case ExpressionRealmCult:
        return L"realm_cult";
    case ExpressionRealmCultOf:
        return L"realm_cult_of";
    case ExpressionRulerKing:
        return L"ruler_king";
    case ExpressionRulerQueen:
        return L"ruler_queen";
    case ExpressionRulerCultistM:
        return L"ruler_cultist_m";
    case ExpressionRulerCultistF:
        return L"ruler_cultist_f";
    case ExpressionRulerEmperor:
        return L"ruler_emperor";
    case ExpressionRulerEmpress:
        return L"ruler_empress";
    case ExpressionRulerLord:
        return L"ruler_lord";
    case ExpressionRulerLady:
        return L"ruler_lady";
    case ExpressionRulerWizard:
        return L"ruler_wizard";
    case ExpressionRulerWitch:
        return L"ruler_witch";
    case ExpressionRulerSorcerer:
        return L"ruler_sorcerer";
    case ExpressionRulerSorceress:
        return L"ruler_sorceress";
    case ExpressionRulerPrince:
        return L"ruler_prince";
    case ExpressionRulerPrincess:
        return L"ruler_princess";
    case ExpressionRulerChieftainM:
        return L"ruler_chieftain_m";
    case ExpressionRulerChieftainF:
        return L"ruler_chieftain_f";
    case ExpressionRulerSeer:
        return L"ruler_seer";
    case ExpressionRulerSeeress:
        return L"ruler_seeress";
    case ExpressionRulerElderM:
        return L"ruler_elder_m";
    case ExpressionRulerElderF:
        return L"ruler_elder_f";
    case ExpressionRulerShamanM:
        return L"ruler_shaman_m";
    case ExpressionRulerShamanF:
        return L"ruler_shaman_f";
    case ExpressionRulerKnightM:
        return L"ruler_knight_m";
    case ExpressionRulerKnightF:
        return L"ruler_knight_f";
    case ExpressionRulerPreacherM:
        return L"ruler_preacher_m";
    case ExpressionRulerPreacherF:
        return L"ruler_preacher_f";
    case ExpressionQuestionClaimRewards:
        return L"question_claim_rewards";
    case ExpressionQuestionChangeSpecialization:
        return L"question_change_specialization";
    case ExpressionQuestionIWantToImproveMySkills:
        return L"question_i_want_to_improve_my_skills";
    case ExpressionTheRealmExistsSinceTheYear:
        return L"the_realm_exists_since_the_year";
    case ExpressionInTheYearThereWasARealm:
        return L"in_the_year_there_was_a_realm";
    case ExpressionOnceThereWasARealm:
        return L"once_there_was_a_realm";
    case ExpressionInTheYearThereWasANamedRuler:
        return L"in_the_year_there_was_a_named_ruler";
    case ExpressionOnceThereWasAANamedRuler:
        return L"once_there_was_a_a_named_ruler";
    case ExpressionHeFoundedTheRealm:
        return L"he_founded_the_realm";
    case ExpressionTheResidentsHadARuler:
        return L"the_residents_had_a_ruler";
    case ExpressionHisNameWas:
        return L"his_name_was";
    case ExpressionTheRealmWasRuledBy:
        return L"the_realm_was_ruled_by";
    case ExpressionTheRealmIsRuledBy:
        return L"the_realm_is_ruled_by";
    case ExpressionItIsRuledBy:
        return L"it_is_ruled_by";
    case ExpressionNpcLifeSpan:
        return L"npc_life_span";
    case ExpressionHereRestsResident:
        return L"here_rests_resident";
    case ExpressionHeWasKnownForHisGoodTrait:
        return L"he_was_known_for_his_good_trait";
    case ExpressionHeWasKnownForHisBadTrait:
        return L"he_was_known_for_his_bad_trait";
    case ExpressionHeLivedAFullLife:
        return L"he_lived_a_full_life";
    case ExpressionHeLivedForYears:
        return L"he_lived_for_years";
    case ExpressionHeDiedInTheWarAgainstTheFaction:
        return L"he_died_in_the_war_against_the_faction";
    case ExpressionHeWasKilledByAFactionMember:
        return L"he_was_killed_by_a_faction_member";
    case ExpressionHeWasKilledByNpc:
        return L"he_was_killed_by_npc";
    case ExpressionHeWasKilledByAMonster:
        return L"he_was_killed_by_a_monster";
    case ExpressionHeWasPoisonedByACreature:
        return L"he_was_poisoned_by_a_creature";
    default:
        return L"";
    }
}

std::wstring Speech::factionKey(int faction)
{
    std::map<int, std::wstring>::iterator it = m_factionTypeIdMap.find(faction);
    if (it == m_factionTypeIdMap.end()) {
        return L"";
    }
    return it->second;
}

std::wstring Speech::factionName(int kind, long long seed, int variant)
{
    if (kind >= FactionRealmKingdom) {
        return realmName(kind, (int)seed);
    }
    return getText(factionKey(kind), L"default", randomName((int)seed, variant));
}

std::wstring Speech::findPlaceName(int x, int y, int type, bool large)
{
    if (large) {
        x = floorDivide(x, 2) * 2;
        y = floorDivide(y, 2) * 2;
    }
    if (x == 0 && y == 0) {
        return L"???";
    }
    return getText(getLocationKey(type), L"named", randomName(metroNameSeed(x, y), metroNameVariant(x, y, type)));
}

bool Speech::generate(std::vector<Dialog> &dialogs, SpeechText *text, SpeechTextNode *node,
                      const std::set<std::wstring> &tags, const std::map<std::wstring, std::wstring> &variables,
                      const std::map<std::wstring, plasma::Vector<4, float>> &colors, int seed,
                      const plasma::Vector<4, float> &color, int shake, const std::wstring &prefix)
{
    if (!node)
        return true;

    // A node with conditions is built only when one of them is among the tags.
    bool conditional = false;
    if (node->conditions.size() != 0) {
        bool met = false;
        for (std::set<std::wstring>::iterator condition = node->conditions.begin(); condition != node->conditions.end();
             ++condition) {
            if (tags.find(*condition) != tags.end())
                met = true;
        }
        if (!met)
            return true;
        conditional = true;
    }

    // The node's style words.
    plasma::Vector<4, float> currentColor = color;
    for (std::set<std::wstring>::iterator style = node->styles.begin(); style != node->styles.end(); ++style) {
        if (*style == L"$creature")
            currentColor = plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f);
        if (*style == L"$name")
            currentColor = plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f);
        if (*style == L"$object")
            currentColor = plasma::Vector<4, float>(1.0f, 0.5f, 0.3f, 1.0f);
        if (*style == L"$zone")
            currentColor = plasma::Vector<4, float>(0.6f, 0.5f, 1.0f, 1.0f);
        if (*style == L"$stress")
            currentColor = plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f);
        if (*style == L"$realm")
            currentColor = plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f);
        if (*style == L"$special")
            currentColor = plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f);
        if (*style == L"$number")
            currentColor = plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f);
        if (*style == L"$green")
            currentColor = plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f);
        if (*style == L"$blue")
            currentColor = plasma::Vector<4, float>(0.3f, 0.45f, 1.0f, 1.0f);
        if (*style == L"$purple")
            currentColor = plasma::Vector<4, float>(0.55f, 0.2f, 1.0f, 1.0f);
        if (*style == L"$yellow")
            currentColor = plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f);
        if (*style == L"$shake")
            shake = 1;
    }

    if (!node->word.empty()) {
        if (node->word == L"%") {
            dialogs.push_back(Dialog());
        } else {
            // The words go to the last dialog, which is assumed to exist.
            if (!prefix.empty() && dialogs.back().phrases.size() == 0) {
                dialogs.back().write(prefix + L": ", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            }
            if (node->word[0] == L'@') {
                std::map<std::wstring, std::wstring>::const_iterator variable = variables.find(node->word);
                std::map<std::wstring, plasma::Vector<4, float>>::const_iterator variableColor =
                    colors.find(node->word);
                if (variableColor != colors.end())
                    currentColor = variableColor->second;
                if (node->speechTags.size() != 0) {
                    if (variable != variables.end()) {
                        // The variable names a speech text: build it with the node's arguments as its variables.
                        const std::wstring &name = variable->first;
                        std::map<std::wstring, SpeechText *>::iterator speech = m_speechNodeMap.find(variable->second);
                        if (speech != m_speechNodeMap.end() && speech->second) {
                            std::map<std::wstring, std::wstring> arguments(variables);
                            std::map<std::wstring, std::wstring>::const_iterator self = variables.find(L"!" + name);
                            if (self != variables.end())
                                arguments[L"@"] = self->second;
                            for (auto &argument : node->arguments) {
                                std::map<std::wstring, std::wstring>::const_iterator value =
                                    variables.find(argument.second);
                                if (value != variables.end())
                                    arguments[argument.first] = value->second;
                            }
                            std::set<std::wstring> speechTags(node->speechTags);
                            if (node->inheritTags)
                                speechTags.insert(tags.begin(), tags.end());
                            generate(dialogs, speech->second, speech->second->m_root, speechTags, arguments, colors,
                                     seed, currentColor, shake, L"");
                        }
                    }
                } else if (variable != variables.end()) {
                    dialogs.back().write(variable->second, currentColor, shake);
                }
            } else if (node->word[0] == L'*') {
                std::wstring name = node->word.substr(1, node->word.size() - 1);
                std::map<std::wstring, SpeechText *>::iterator macro = m_macroNodeMap.find(name);
                if (macro != m_macroNodeMap.end()) {
                    generate(dialogs, macro->second, macro->second->m_root, tags, variables, colors, seed, currentColor,
                             shake, L"");
                }
            } else if (node->word == L"%%") {
                dialogs.back().write(L"%", currentColor, shake);
            } else {
                dialogs.back().write(node->word, currentColor, shake);
            }
        }
    }

    // Type 3: the children are built with probability 1/2.
    int childSeed = seed;
    if (node->kind == 3) {
        bool built = (scramble(childSeed++) & 1) != 0;
        if (!built)
            return !conditional;
    }

    // Type 2: of the children without conditions one is picked; the others are built in order until one of them was
    // conditional and met.
    std::vector<SpeechTextNode *> alternatives;
    for (std::list<SpeechTextNode *>::iterator child = node->children.begin(); child != node->children.end(); ++child) {
        if (node->kind == 2 && (*child)->conditions.size() == 0) {
            alternatives.push_back(*child);
        } else if (!generate(dialogs, text, *child, tags, variables, colors, childSeed, currentColor, shake, prefix)) {
            return !conditional;
        }
    }
    if (!alternatives.empty()) {
        size_t count = alternatives.size();
        generate(dialogs, text, alternatives[scramble(childSeed) % count], tags, variables, colors, childSeed,
                 currentColor, shake, prefix);
    }
    return !conditional;
}

void Speech::generateDialogs(const std::wstring &key, Context &context, int seed, std::vector<Dialog> &dialogs,
                             const std::wstring &argument)
{
    if (!key.empty() && key[0] == L'!') {
        // A literal phrase (an emote such as L"!love"), in red.
        if (dialogs.empty() || dialogs.back().phrases.size() != 0)
            dialogs.push_back(Dialog());
        Dialog::Phrase phrase;
        phrase.text = key;
        phrase.color = plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f);
        // phrase.shake is copied uninitialised.
        dialogs.back().phrases.push_back(phrase);
    } else if (!key.empty() && key[0] == L'*') {
        // Literal text: one phrase per word (the '*' stays on the first).
        if (dialogs.empty() || dialogs.back().phrases.size() != 0)
            dialogs.push_back(Dialog());
        std::wstringstream stream(key);
        while (stream.good()) {
            Dialog::Phrase phrase;
            stream >> phrase.text;
            phrase.color = plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f);
            // phrase.shake is copied uninitialised.
            dialogs.back().phrases.push_back(phrase);
        }
    } else {
        // A speech text of the dictionary; when it has alternatives, seed picks one. A dialog of more than ten phrases
        // is not extended.
        std::map<std::wstring, SpeechText *>::iterator found = m_speechNodeMap.find(key);
        if (found != m_speechNodeMap.end()) {
            SpeechText *text = found->second;
            if (text) {
                if (!text->m_alternatives.empty()) {
                    text = text->m_alternatives[scramble(seed) % text->m_alternatives.size()];
                    seed++;
                }
                if (dialogs.empty() || dialogs.back().phrases.size() > 10)
                    dialogs.push_back(Dialog());
                generate(dialogs, text, text->m_root, context.tags, context.variables, context.colors, seed,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0, argument);
            }
        }
    }
}

void Speech::generateFromCommands(const std::vector<int> &commands, int seed, std::vector<Dialog> &dialogs,
                                  const std::wstring &argument)
{
    Context context;
    ScriptValueReader reader(commands);
    bool newPage = false;
    // A read past the end of the script leaves its target unset, here and in every case below.
    int command;
    while (!reader.isAtEnd()) {
        reader >> command;
        switch (command) {
        case SpeechCommand_NewPage:
            newPage = true;
            break;
        case SpeechCommand_DefineMob: {
            long long id;
            int type;
            reader >> id;
            reader >> type;
            addCreature(L"mob", type, id, context);
            break;
        }
        case SpeechCommand_DefineMob2: {
            long long id;
            int type;
            reader >> id;
            reader >> type;
            addCreature(L"mob2", type, id, context);
            break;
        }
        case SpeechCommand_DefineModel: {
            int type;
            reader >> type;
            addCreature(L"mob", type, context);
            break;
        }
        case SpeechCommand_DefineModel2: {
            int type;
            reader >> type;
            addCreature(L"mob2", type, context);
            break;
        }
        case SpeechCommand_DefineSpecialization: {
            int classType;
            int specialization;
            reader >> classType;
            reader >> specialization;
            addSpecialization(L"specialization", classType, specialization, context);
            break;
        }
        case SpeechCommand_DefineItem: {
            Item item;
            reader >> item;
            addItem(L"item", item, context);
            break;
        }
        case SpeechCommand_GiveItem: {
            Item item;
            reader >> item;
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().item = item;
            dialogs.back().givenCount = 1;
            break;
        }
        case SpeechCommand_GiveCoins: {
            int count;
            reader >> count;
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().item.category = ItemCategoryCoin;
            dialogs.back().item.material = ItemMaterialGold;
            dialogs.back().givenCount = count;
            break;
        }
        case SpeechCommand_TakeItem: {
            Item item;
            int count;
            reader >> item;
            reader >> count;
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().takenItem = item;
            dialogs.back().takenCount = count;
            break;
        }
        case SpeechCommand_AppendOpaqueRecord: {
            Dialog::ScriptRecord element;
            reader >> element;
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().scriptRecords.push_back(element);
            break;
        }
        case SpeechCommand_DefineAndMarkLocation: {
            int x;
            int y;
            int type;
            reader >> x;
            reader >> y;
            reader >> type;
            addLocation(L"zone", x, y, type, context);
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().markedLocations.push_back({{x, y}});
            break;
        }
        case SpeechCommand_DefineLands: {
            long long zone;
            reader >> zone;
            addLands(L"zone", zone, context);
            break;
        }
        case SpeechCommand_DefineLands2: {
            long long zone;
            reader >> zone;
            addLands(L"zone2", zone, context);
            break;
        }
        case SpeechCommand_DefineEmptyLand: {
            int biome;
            long long land;
            reader >> biome;
            reader >> land;
            addEmpty(L"zone", biome, land, context);
            break;
        }
        case SpeechCommand_DefineEmptyLand2: {
            int biome;
            long long land;
            reader >> biome;
            reader >> land;
            addEmpty(L"zone2", biome, land, context);
            break;
        }
        case SpeechCommand_DefineDirection: {
            plasma::Vector<2, int> direction;
            reader >> direction;
            addDirection(L"zone", direction, context);
            break;
        }
        case SpeechCommand_DefineDistance: {
            int distance;
            reader >> distance;
            std::wstringstream text;
            if (distance < 500)
                text << distance << L" m" << std::endl;
            else
                text << roundTenth(distance / 1000.0f) << L" km" << std::endl;
            context.variables[L"@distance"] = text.str();
            break;
        }
        case SpeechCommand_DefineSkill: {
            int skill;
            reader >> skill;
            context.variables[L"@skill"] = skillKey(skill);
            break;
        }
        case SpeechCommand_DefineLevel: {
            int level;
            reader >> level;
            context.variables[L"@level"] = std::to_wstring(level);
            break;
        }
        case SpeechCommand_DefineNumber: {
            int number;
            reader >> number;
            context.variables[L"@number"] = std::to_wstring(number);
            break;
        }
        case SpeechCommand_DefineNumber2: {
            int number;
            reader >> number;
            context.variables[L"@number2"] = std::to_wstring(number);
            break;
        }
        case SpeechCommand_DefineRealm: {
            int kind;
            int realm;
            reader >> kind;
            reader >> realm;
            addRealm(kind, realm, context);
            break;
        }
        case SpeechCommand_DefineFaction: {
            int faction;
            reader >> faction;
            addFaction(L"faction", faction, 0, 0, context);
            break;
        }
        case SpeechCommand_DefineRuler: {
            int ruler;
            reader >> ruler;
            addRuler(ruler, context);
            break;
        }
        // The emotes: each is one phrase in a dialog of its own. They read the last dialog without checking that there
        // is one.
        case SpeechCommand_Alert:
            if (dialogs.back().phrases.size() != 0)
                dialogs.push_back(Dialog());
            dialogs.back().write(L"!alert", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            newPage = true;
            break;
        case SpeechCommand_Question:
            if (dialogs.back().phrases.size() != 0)
                dialogs.push_back(Dialog());
            dialogs.back().write(L"!question", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            newPage = true;
            break;
        case SpeechCommand_Love:
            if (dialogs.back().phrases.size() != 0)
                dialogs.push_back(Dialog());
            dialogs.back().write(L"!love", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            newPage = true;
            break;
        case SpeechCommand_Angry:
            if (dialogs.back().phrases.size() != 0)
                dialogs.push_back(Dialog());
            dialogs.back().write(L"!angry", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            newPage = true;
            break;
        case SpeechCommand_Sweat:
            if (dialogs.back().phrases.size() != 0)
                dialogs.push_back(Dialog());
            dialogs.back().write(L"!sweat", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
            newPage = true;
            break;
        default:
            // A text: its id is the command number. Ids above QuestionCommandBase always open a dialog, which keeps
            // the id's low byte.
            if (newPage || command > QuestionCommandBase) {
                dialogs.push_back(Dialog());
                newPage = false;
                if (command > QuestionCommandBase)
                    dialogs.back().answer = (char)command;
            }
            generateDialogs(expressionKey(command), context, seed, dialogs, argument);
            break;
        }
    }
}

void Speech::generateFromScript(const std::vector<std::string> &script, int seed, std::vector<Dialog> &dialogs,
                                const std::wstring &argument)
{
    Context context;
    ScriptReader reader(script);
    bool newPage = false;
    while (!reader.isAtEnd()) {
        std::string command;
        reader >> command;
        if (command == "new_page") {
            newPage = true;
        } else if (command == "define_mob") {
            long long id;
            int type;
            reader >> id;
            reader >> type;
            addCreature(L"mob", type, id, context);
        } else if (command == "define_mob2") {
            long long id;
            int type;
            reader >> id;
            reader >> type;
            addCreature(L"mob2", type, id, context);
        } else if (command == "define_model") {
            int type;
            reader >> type;
            addCreature(L"mob", type, context);
        } else if (command == "define_model2") {
            int type;
            reader >> type;
            addCreature(L"mob2", type, context);
        } else if (command == "define_specialization") {
            int classType;
            int specialization;
            reader >> classType;
            reader >> specialization;
            addSpecialization(L"specialization", classType, specialization, context);
        } else if (command == "define_item") {
            Item item;
            reader >> item;
            addItem(L"item", item, context);
        } else if (command == "define_location") {
            int x;
            int y;
            int type;
            reader >> x;
            reader >> y;
            reader >> type;
            addLocation(L"zone", x, y, type, context);
        } else if (command == "mark_location") {
            int x;
            int y;
            reader >> x;
            reader >> y;
            if (dialogs.empty())
                dialogs.push_back(Dialog());
            dialogs.back().markedLocations.push_back({{x, y}});
        } else if (command == "define_land") {
            int biome;
            long long land;
            reader >> biome;
            reader >> land;
            addLand(L"land", biome, land, context);
        } else if (command == "define_land2") {
            int biome;
            long long land;
            reader >> biome;
            reader >> land;
            addLand(L"land2", biome, land, context);
        } else if (command == "define_direction") {
            plasma::Vector<2, int> direction;
            reader >> direction;
            addDirection(L"zone", direction, context);
        } else if (command == "define_distance") {
            int distance;
            reader >> distance;
            std::wstringstream text;
            if (distance < 500)
                text << distance << L" m" << std::endl;
            else
                text << roundTenth(distance / 1000.0f) << L" km" << std::endl;
            context.variables[L"@distance"] = text.str();
        } else if (command == "define_skill") {
            int skill;
            reader >> skill;
            context.variables[L"@skill"] = skillKey(skill);
        } else if (command == "define_level") {
            int level;
            reader >> level;
            context.variables[L"@level"] = std::to_wstring(level);
        } else if (command == "define_number") {
            int number;
            reader >> number;
            context.variables[L"@number"] = std::to_wstring(number);
        } else if (command == "define_number2") {
            int number;
            reader >> number;
            context.variables[L"@number2"] = std::to_wstring(number);
        } else if (command == "define_name") {
            std::string name;
            reader >> name;
            context.variables[L"@name"] = std::wstring(name.begin(), name.end());
        } else if (command == "define_realm") {
            int kind;
            int realm;
            reader >> kind;
            reader >> realm;
            addRealm(kind, realm, context);
        } else if (command == "define_faction") {
            int faction;
            reader >> faction;
            addFaction(L"faction", faction, 0, 0, context);
        } else if (command == "define_ruler") {
            int ruler;
            reader >> ruler;
            addRuler(ruler, context);
        } else {
            // A text key. "!..." is an emote phrase in a dialog of its own, "#..." adds a tag, a key ending in '?'
            // opens a dialog that keeps the key (a question).
            std::wstring key(command.begin(), command.end());
            if (!command.empty() && command[0] == '!') {
                if (dialogs.empty() || dialogs.back().phrases.size() != 0)
                    dialogs.push_back(Dialog());
                dialogs.back().write(key, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
                newPage = true;
            } else if (!command.empty() && command[0] == '#') {
                context.tags.insert(key);
            } else {
                bool question = !command.empty() && command.back() == '?';
                if (newPage || question) {
                    dialogs.push_back(Dialog());
                    newPage = false;
                    if (question)
                        dialogs.back().answer = command;
                }
                generateDialogs(key, context, seed, dialogs, argument);
            }
        }
    }
}

std::wstring Speech::generateText(const std::wstring &key, const std::set<std::wstring> &tags,
                                  const std::map<std::wstring, std::wstring> &macros, int seed)
{
    std::vector<Dialog> dialogs;
    Context context;
    context.tags = tags;
    context.variables = macros;
    generateDialogs(key, context, seed, dialogs, L"");

    // No blank before the next phrase: at the start and after "-", "=" and "'".
    bool joined = true;
    std::wstringstream stream;
    if (!dialogs.empty()) {
        for (std::list<Dialog::Phrase>::iterator it = dialogs.front().phrases.begin();
             it != dialogs.front().phrases.end(); ++it) {
            // "=" itself is not written; it joins its neighbours.
            if (it->text == L"=") {
                joined = true;
                continue;
            }
            bool punctuation = it->text == L"." || it->text == L"-" || it->text == L"=" || it->text == L"," ||
                               it->text == L";" || it->text == L"!" || it->text == L"?" || it->text == L":" ||
                               it->text == L"'";
            if (!joined && !punctuation) {
                stream << L" ";
            }
            joined = it->text == L"-" || it->text == L"=" || it->text == L"'";
            stream << it->text;
        }
    }
    return stream.str();
}

std::wstring Speech::getItemKey(int category, int id)
{
    std::map<std::pair<int, int>, std::wstring>::iterator it =
        m_itemIdentifierMap.find(std::pair<int, int>(category, id));
    if (it == m_itemIdentifierMap.end()) {
        return L"";
    }
    return it->second;
}

plasma::Vector<4, float> Speech::getLandColor(int biome)
{
    switch (biome) {
    case BiomeDesert:
        return plasma::Vector<4, float>(1.0f, 0.9f, 0.2f, 1.0f);
    case BiomeOcean:
        return plasma::Vector<4, float>(0.4f, 0.65f, 1.0f, 1.0f);
    case BiomePlains:
    case BiomeHills:
        return plasma::Vector<4, float>(0.5f, 1.0f, 0.25f, 1.0f);
    case BiomeSavannah:
        return plasma::Vector<4, float>(0.9f, 1.0f, 0.2f, 1.0f);
    case BiomeWetlands:
        return plasma::Vector<4, float>(0.3f, 0.7f, 1.0f, 1.0f);
    case BiomeDeadlands:
        return plasma::Vector<4, float>(0.5f, 0.1f, 1.0f, 1.0f);
    case BiomeSnowlands:
        return plasma::Vector<4, float>(0.7f, 0.9f, 1.0f, 1.0f);
    case BiomeFirelands:
        return plasma::Vector<4, float>(0.7f, 0.2f, 0.2f, 1.0f);
    case BiomeWoodlands:
        return plasma::Vector<4, float>(0.5f, 0.9f, 0.1f, 1.0f);
    case BiomeJungle:
        return plasma::Vector<4, float>(0.2f, 0.8f, 0.2f, 1.0f);
    case BiomeDarkWoods:
        return plasma::Vector<4, float>(0.4f, 0.3f, 0.9f, 1.0f);
    case BiomeSnowForest:
        return plasma::Vector<4, float>(0.6f, 0.7f, 1.0f, 1.0f);
    case BiomeMountains:
        return plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
    }
    return plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
}

std::wstring Speech::getLandName(int biome)
{
    switch (biome) {
    case BiomePlains:
        return L"Plains";
    case BiomeOcean:
        return L"Ocean";
    case BiomeHills:
        return L"Hills";
    case BiomeWoodlands:
        return L"Woodlands";
    case BiomeJungle:
        return L"Jungle";
    case BiomeWetlands:
        return L"Wetlands";
    case BiomeDesert:
        return L"Desert";
    case BiomeSnowlands:
        return L"Snowlands";
    case BiomeSavannah:
        return L"Savannah";
    case BiomeDeadlands:
        return L"Deadlands";
    case BiomeDarkWoods:
        return L"DarkWoods";
    case BiomeSnowForest:
        return L"SnowForest";
    case BiomeFirelands:
        return L"Firelands";
    case BiomeMountains:
        return L"Mountains";
    }
    return L"";
}

std::wstring Speech::getLocationKey(int type)
{
    switch (type) {
    case PlaceHouse:
        return L"House";
    case PlaceFarm:
        return L"Farm";
    case PlaceInn:
        return L"Inn";
    case PlaceCrypt:
        return L"Crypt";
    case PlaceVillage:
        return L"Village";
    case PlaceTower:
        return L"Tower";
    case PlaceCastle:
        return L"Castle";
    case PlaceGraveyard:
        return L"Graveyard";
    case PlaceMountain:
        return L"Mountain";
    case PlaceCave:
        return L"Cave";
    case PlaceLake:
        return L"Lake";
    case PlaceLavaLake:
        return L"LavaLake";
    case PlaceCrater:
        return L"Crater";
    case PlaceVolcano:
        return L"Volcano";
    case PlaceAncientRuins:
        return L"AncientRuins";
    case PlaceCircleOfPower:
        return L"CircleOfPower";
    case PlaceManaPump:
        return L"ManaPump";
    case PlaceCatacombs:
        return L"Catacombs";
    case PlaceSewers:
        return L"Sewers";
    case PlaceMines:
        return L"Mines";
    case PlaceVault:
        return L"Vault";
    case PlaceShrine:
        return L"Shrine";
    case PlaceGrotto:
        return L"Grotto";
    case PlaceOldHut:
        return L"OldHut";
    case PlaceTree:
        return L"Tree";
    case PlaceRock:
        return L"Rock";
    case PlaceIsland:
        return L"Island";
    case PlacePyramid:
        return L"Pyramid";
    case PlaceTemple:
        return L"Temple";
    case PlaceArena:
        return L"Arena";
    case PlaceForest:
        return L"Forest";
    case PlaceDarkForest:
        return L"DarkForest";
    case PlaceEnchantedForest:
        return L"EnchantedForest";
    case PlaceCanyon:
        return L"Canyon";
    case PlaceRocks:
        return L"Rocks";
    case PlacePlain:
        return L"Plain";
    case PlaceBeach:
        return L"Beach";
    case PlaceHill:
        return L"Hill";
    case PlaceGlade:
        return L"Glade";
    case PlaceGrove:
        return L"Grove";
    case PlaceCamp:
        return L"Camp";
    case PlaceMenhir:
        return L"Menhir";
    case PlaceBanditCamp:
        return L"BanditCamp";
    case PlaceSanctuary:
        return L"Sanctuary";
    case PlaceAltar:
        return L"Altar";
    }
    return L"";
}

std::wstring Speech::getMaterialKey(int material)
{
    // 43 and 128 are both fire, 23 and 130 both ice.
    switch (material) {
    case ItemMaterialIron:
        return L"MaterialIron";
    case ItemMaterialWood:
        return L"MaterialWood";
    case ItemMaterialGold:
        return L"MaterialGold";
    case ItemMaterialIce:
        return L"MaterialIce";
    case ItemMaterialLight:
        return L"MaterialLight";
    case ItemMaterialGlass:
        return L"MaterialGlass";
    case ItemMaterialSilk:
        return L"MaterialSilk";
    case ItemMaterialLinen:
        return L"MaterialLinen";
    case ItemMaterialCotton:
        return L"MaterialCotton";
    case ItemMaterialSilver:
        return L"MaterialSilver";
    case ItemMaterialPlatinum:
        return L"MaterialPlatinum";
    case ItemMaterialCopper:
        return L"MaterialCopper";
    case ItemMaterialSandstone:
        return L"MaterialSandstone";
    case ItemMaterialEmerald:
        return L"MaterialEmerald";
    case ItemMaterialSapphire:
        return L"MaterialSapphire";
    case ItemMaterialRuby:
        return L"MaterialRuby";
    case ItemMaterialDiamond:
        return L"MaterialDiamond";
    case ItemMaterialObsidian:
        return L"MaterialObsidian";
    case ItemMaterialSaurian:
        return L"MaterialSaurian";
    case ItemMaterialParrot:
        return L"MaterialParrot";
    case ItemMaterialMammoth:
        return L"MaterialMammoth";
    case ItemMaterialPlant:
        return L"MaterialPlant";
    case ItemMaterialBone:
        return L"MaterialBone";
    case ItemMaterialFireSpirit:
        return L"MaterialFire";
    case ItemMaterialUnholy:
        return L"MaterialUnholy";
    case ItemMaterialIceSpirit:
        return L"MaterialIce";
    case ItemMaterialWind:
        return L"MaterialWind";
    case ItemMaterialBronze:
        return L"MaterialBronze";
    case ItemMaterialCalico:
        return L"MaterialCalico";
    case ItemMaterialFelt:
        return L"MaterialFelt";
    case ItemMaterialFlannell:
        return L"MaterialFlannell";
    case ItemMaterialSteel:
        return L"MaterialSteel";
    case ItemMaterialMail:
        return L"MaterialMail";
    case ItemMaterialRivet:
        return L"MaterialRivet";
    case ItemMaterialFilament:
        return L"MaterialFilament";
    case ItemMaterialSpike:
        return L"MaterialSpike";
    case ItemMaterialFiber:
        return L"MaterialFiber";
    case ItemMaterialScale:
        return L"MaterialScale";
    case ItemMaterialWeb:
        return L"MaterialWeb";
    case ItemMaterialShadow:
        return L"MaterialShadow";
    case ItemMaterialDark:
        return L"MaterialDark";
    case ItemMaterialDemon:
        return L"MaterialDemon";
    case ItemMaterialFire:
        return L"MaterialFire";
    case ItemMaterialDruid:
        return L"MaterialDruid";
    case ItemMaterialClothing:
        return L"MaterialClothing";
    }
    return L"Material";
}

std::wstring Speech::getText(const std::wstring &key, const std::wstring &variant, const std::wstring &argument)
{
    std::set<std::wstring> tags;
    tags.insert(L"#" + variant);
    std::map<std::wstring, std::wstring> macros;
    macros[L"@"] = argument;
    return generateText(key, tags, macros, 0);
}

std::wstring Speech::realmName(int kind, int realm)
{
    Context context;
    addRealm(kind, realm, context);
    return generateText(L"realmname", context.tags, context.variables, 0);
}

std::wstring Speech::rulerKey(int ruler)
{
    std::map<int, std::wstring>::iterator it = m_rulerTypeIdMap.find(ruler);
    if (it == m_rulerTypeIdMap.end()) {
        return L"";
    }
    return it->second;
}

std::wstring Speech::skillKey(int skill)
{
    std::map<int, std::wstring>::iterator it = m_skillTypeIdMap.find(skill);
    if (it == m_skillTypeIdMap.end()) {
        return L"";
    }
    return it->second;
}

std::wstring Speech::specializationKey(int classType, int specialization)
{
    std::map<std::pair<int, int>, std::wstring>::iterator it =
        m_specializationTypeIdMap.find(std::pair<int, int>(classType, specialization));
    if (it == m_specializationTypeIdMap.end()) {
        return L"";
    }
    return it->second;
}

SpeechText *Speech::speechText(const std::wstring &key)
{
    std::map<std::wstring, SpeechText *>::iterator it = m_speechNodeMap.find(key);
    if (it == m_speechNodeMap.end()) {
        return nullptr;
    }
    return it->second;
}

std::wstring Speech::staticKey(int type)
{
    switch (type) {
    case BuildingShrineOfLife:
        return L"ShrineOfLife";
    case BuildingManaWell:
        return L"ManaWell";
    case BuildingShrineOfFinesse:
        return L"ShrineOfFinesse";
    }
    return L"";
}

void Dialog::write(std::wstring text, const plasma::Vector<4, float> &color, int shake)
{
    std::wstringstream stream(text);
    while (stream.good()) {
        std::wstring word;
        stream >> word;
        if (!phrases.empty() && phrases.back().text == L"=") {
            phrases.pop_back();
            if (!phrases.empty()) {
                phrases.back().text += word;
            }
        } else if (!phrases.empty() && !phrases.back().text.empty() &&
                   (phrases.back().text.back() == L'\'' || word == L"'")) {
            phrases.back().text += word;
        } else {
            phrases.push_back(Phrase(word, color, shake));
        }
    }
}

}
