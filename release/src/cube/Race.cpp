#include "cube/World.h"
#include "cube/Race.h"
#include <cstdlib>
#include "cube/CreatureIds.h"
#include "cube/WorldIds.h"

namespace cube {

void appendRaceVariants(int race, std::vector<int> &variants)
{
    switch (race) {
    case RaceElfMale:
    case RaceElfFemale:
        variants.push_back(RaceElfMale);
        variants.push_back(RaceElfFemale);
        break;
    case RaceLizardmanMale:
    case RaceLizardmanFemale:
        variants.push_back(RaceLizardmanMale);
        variants.push_back(RaceLizardmanFemale);
        break;
    case RaceUndeadMale:
    case RaceUndeadFemale:
        variants.push_back(RaceUndeadMale);
        variants.push_back(RaceUndeadFemale);
        break;
    case RaceFrogmanMale:
    case RaceFrogmanFemale:
        variants.push_back(RaceFrogmanMale);
        variants.push_back(RaceFrogmanFemale);
        break;
    case RacePirateMale:
    case RacePirateFemale:
    case RaceEnhancedPirateMale:
        variants.push_back(RacePirateMale);
        variants.push_back(RacePirateFemale);
        variants.push_back(RaceEnhancedPirateMale);
        break;
    case RaceFamilyGoods:
        variants.push_back(RaceCrate);
        variants.push_back(RaceVase);
        variants.push_back(RaceTreasure);
        variants.push_back(RaceBarrel);
        variants.push_back(RaceSack);
        break;
    case RaceOrcMale:
    case RaceOrcFemale:
        variants.push_back(RaceOrcMale);
        variants.push_back(RaceOrcFemale);
        break;
    case RaceGoblinMale:
    case RaceGoblinFemale:
        variants.push_back(RaceGoblinMale);
        variants.push_back(RaceGoblinFemale);
        break;
    case RaceDwarfMale:
    case RaceDwarfFemale:
        variants.push_back(RaceDwarfMale);
        variants.push_back(RaceDwarfFemale);
        break;
    case RaceHumanMale:
    case RaceHumanFemale:
        variants.push_back(RaceHumanMale);
        variants.push_back(RaceHumanFemale);
        break;
    case RaceAlpaca:
    case RaceBrownAlpaca:
        variants.push_back(RaceAlpaca);
        variants.push_back(RaceBrownAlpaca);
        break;
    case RaceLionMale:
    case RaceLionFemale:
        variants.push_back(RaceLionMale);
        variants.push_back(RaceLionFemale);
        break;
    case RaceNomadMale:
    case RaceNomadFemale:
        variants.push_back(RaceNomadMale);
        variants.push_back(RaceNomadFemale);
        break;
    case RaceDarkCultistMale:
    case RaceDarkCultistFemale:
        variants.push_back(RaceDarkCultistMale);
        variants.push_back(RaceDarkCultistFemale);
        break;
    case RaceEmpireSoldierMale:
    case RaceEmpireSoldierFemale:
        variants.push_back(RaceEmpireSoldierMale);
        variants.push_back(RaceEmpireSoldierFemale);
        break;
    case RaceGreenSlime:
    case RacePinkSlime:
    case RaceYellowSlime:
    case RaceBlueSlime:
        variants.push_back(RaceBlueSlime);
        variants.push_back(RacePinkSlime);
        variants.push_back(RaceYellowSlime);
        variants.push_back(RaceGreenSlime);
        break;
    case RaceAnubis:
    case RaceHorus:
        variants.push_back(RaceAnubis);
        variants.push_back(RaceHorus);
        break;
    case RaceEnhancedOrcMale:
    case RaceEnhancedOrcFemale:
        variants.push_back(RaceEnhancedOrcMale);
        variants.push_back(RaceEnhancedOrcFemale);
        break;
    default:
        variants.push_back(race);
        break;
    }
}

int randomCaster()
{
    std::vector<int> families;
    families.push_back(RaceWizard);
    families.push_back(RaceSorcerer);
    families.push_back(RaceWitch);
    return randomRace(families[rand() % families.size()]);
}

int randomCreature()
{
    std::vector<int> families;
    families.push_back(RaceEnhancedOrcMale);
    return randomRace(families[rand() % families.size()]);
}

int randomDemonBoss()
{
    std::vector<int> families;
    families.push_back(RaceDarkCultistMale);
    return randomRace(families[rand() % families.size()]);
}

int randomDemonCreature()
{
    std::vector<int> families;
    families.push_back(RaceDemonMarauder);
    families.push_back(RaceImp);
    families.push_back(RaceDevourer);
    families.push_back(RaceCrawler);
    families.push_back(RaceFlyingEye);
    families.push_back(RaceDarkCultistMale);
    return randomRace(families[rand() % families.size()]);
}

int randomFighterRace()
{
    std::vector<int> families;
    families.push_back(RaceHumanMale);
    families.push_back(RaceDwarfMale);
    families.push_back(RaceElfMale);
    families.push_back(RaceFrogmanMale);
    families.push_back(RaceLizardmanMale);
    return randomRace(families[rand() % families.size()]);
}

int randomFolkRace(int biome)
{
    std::vector<int> families;
    families.push_back(RaceElfMale);
    families.push_back(RaceHumanMale);
    families.push_back(RaceGoblinMale);
    families.push_back(RaceLizardmanMale);
    families.push_back(RaceDwarfMale);
    families.push_back(RaceOrcMale);
    families.push_back(RaceFrogmanMale);
    families.push_back(RaceUndeadMale);
    if (biome == BiomeSnowlands || biome == BiomeSnowForest) {
        families.push_back(RacePolarGnoll);
        families.push_back(RaceSnowGnobold);
        families.push_back(RaceIceling);
        families.push_back(RacePolarGnoll);
        families.push_back(RaceSnowGnobold);
        families.push_back(RaceIceling);
        families.push_back(RacePolarGnoll);
        families.push_back(RaceSnowGnobold);
        families.push_back(RaceIceling);
    } else {
        families.push_back(RaceGnoll);
        families.push_back(RaceGnobold);
    }
    if (biome == BiomeDarkWoods || biome == BiomeDeadlands) {
        families.push_back(RaceDarkJester);
        families.push_back(RaceSkeletonKnight);
        families.push_back(RaceVampire);
    } else {
        families.push_back(RaceJester);
    }
    if (biome == BiomeDesert || biome == BiomeSavannah) {
        families.push_back(RaceNomadMale);
        families.push_back(RaceNomadMale);
        families.push_back(RaceNomadMale);
        families.push_back(RaceNomadMale);
        families.push_back(RaceNomadMale);
        families.push_back(RaceNomadMale);
    }
    families.push_back(RaceWitch);
    families.push_back(RaceWizard);
    families.push_back(RaceNecromancer);
    families.push_back(RaceInsectoid);
    families.push_back(RaceSpikeCreature);
    families.push_back(RaceMantis);
    families.push_back(RaceInsectoid);
    families.push_back(RaceSpectrino);
    families.push_back(RacePirateMale);
    return randomRace(families[rand() % families.size()]);
}

int randomPlantCreature()
{
    static const std::vector<int> families = {RaceRadishCreature, RacePlantCreature, RaceChiling,
                                              RaceOnionling,      RaceBloomling,     RaceHabanero};
    return randomRace(families[rand() % families.size()]);
}

int randomInsect()
{
    static const std::vector<int> families = {RaceBarkBeetle, RaceSnoutBeetle, RaceLemonBeetle};
    return randomRace(families[rand() % families.size()]);
}

int randomMobRace()
{
    std::vector<int> families;
    families.push_back(RaceElfMale);
    families.push_back(RaceHumanMale);
    families.push_back(RaceGoblinMale);
    families.push_back(RaceLizardmanMale);
    families.push_back(RaceDwarfMale);
    families.push_back(RaceOrcMale);
    families.push_back(RaceFrogmanMale);
    families.push_back(RaceUndeadMale);
    families.push_back(RaceGnoll);
    families.push_back(RaceWitch);
    families.push_back(RaceGnobold);
    families.push_back(RaceInsectoid);
    families.push_back(RaceSpikeCreature);
    families.push_back(RaceMantis);
    families.push_back(RaceInsectoid);
    families.push_back(RaceSpectrino);
    return randomRace(families[rand() % families.size()]);
}

int randomPeacefulRace()
{
    std::vector<int> families;
    families.push_back(RaceHumanMale);
    if (rand() % 4 == 0)
        families.push_back(RaceDwarfMale);
    return randomRace(families[rand() % families.size()]);
}

int randomRace(int race)
{
    std::vector<int> variants;
    appendRaceVariants(race, variants);
    if (!variants.empty())
        race = variants[rand() % variants.size()];
    return race;
}

int randomUndead()
{
    std::vector<int> families;
    families.push_back(RaceZombie);
    families.push_back(RaceSkeleton);
    families.push_back(RaceSkeletonKnight);
    families.push_back(RaceFrightener);
    families.push_back(RaceMonstrosity);
    families.push_back(RaceVampire);
    families.push_back(RaceNecromancer);
    families.push_back(RaceSkeletonDog);
    return randomRace(families[rand() % families.size()]);
}

int randomUndeadBoss()
{
    std::vector<int> families;
    families.push_back(RaceVampire);
    families.push_back(RaceNecromancer);
    families.push_back(RaceUndeadMale);
    families.push_back(RaceSkeletonKnight);
    families.push_back(RaceSkeleton);
    families.push_back(RaceMonstrosity);
    return randomRace(families[rand() % families.size()]);
}

}
