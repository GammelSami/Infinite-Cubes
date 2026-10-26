// Creature entity-data network encoding and decoding.
#include <list>

#include "cube/BytesIO.h"
#include "cube/Creature.h"
#include "cube/Zone.h"
#include "cube/ZoneIds.h"

namespace cube {

// A member that encodeEntityData has found changed: where its bytes are and how many.
typedef std::pair<const void *, int> EntityDataChunk;

// Records current as changed when it differs from previous or when every member is wanted, and sets the member's bit
// in mask. Always moves on to the next bit.
template<class T>
bool addField(std::list<EntityDataChunk> &chunks, unsigned long long &mask, int &bit, const T &previous,
              const T &current, bool full)
{
    bool changed = false;
    if (full || previous != current) {
        chunks.push_back(std::make_pair(&current, sizeof(T)));
        mask |= 1ULL << bit;
        changed = true;
    }
    bit++;
    return changed;
}

// Reads a member when its bit is set in mask and moves on to the next bit. Returns the member's size when the bit was
// set.
template<class T> int readField(BytesIO &bytes, const unsigned long long &mask, int &bit, T &value)
{
    int size = 0;
    if (mask & (1ULL << bit)) {
        bytes.read(&value, sizeof(T));
        size = sizeof(T);
    }
    bit++;
    return size;
}

bool Creature::EntityData::Appearance::operator!=(const Appearance &other) const
{
    return unsetPair != other.unsetPair || hairColor != other.hairColor || appearanceFlags != other.appearanceFlags ||
           size != other.size || headModel != other.headModel || hairModel != other.hairModel ||
           handsModel != other.handsModel || feetModel != other.feetModel || chestModel != other.chestModel ||
           tailModel != other.tailModel || shoulderModel != other.shoulderModel || wingsModel != other.wingsModel ||
           headScale != other.headScale || chestScale != other.chestScale || handsScale != other.handsScale ||
           feetScale != other.feetScale || shoulderModelScale != other.shoulderModelScale ||
           weaponScale != other.weaponScale || tailScale != other.tailScale ||
           shoulderArmorScale != other.shoulderArmorScale || wingScale != other.wingScale ||
           chestRotation != other.chestRotation || handsRotation != other.handsRotation ||
           feetRotation != other.feetRotation || wingsRotation != other.wingsRotation ||
           tailRotation != other.tailRotation || chestPosition != other.chestPosition ||
           headPosition != other.headPosition || handsPosition != other.handsPosition ||
           feetPosition != other.feetPosition || wingsPosition != other.wingsPosition ||
           tailPosition != other.tailPosition;
}

bool Equipment::operator!=(const Equipment &other) const
{
    return pet != other.pet || head != other.head || neck != other.neck || chest != other.chest || feet != other.feet ||
           hands != other.hands || shoulder != other.shoulder || weaponLeft != other.weaponLeft ||
           weaponRight != other.weaponRight || ringLeft != other.ringLeft || ringRight != other.ringRight;
}

// The name: compared as a string, sent as its 16 bytes.
bool addField(std::list<EntityDataChunk> &chunks, unsigned long long &mask, int &bit, const char *previous,
              const char *current, bool full)
{
    bool changed = false;
    if (full || strcmp(previous, current)) {
        chunks.push_back(std::make_pair(current, 16));
        mask |= 1ULL << bit;
        changed = true;
    }
    bit++;
    return changed;
}

int decodeEntityData(BytesIO &bytes, Creature::EntityData &entityData)
{
    unsigned long long mask = 0;
    bytes.read(mask);
    int bit = 0;
    readField(bytes, mask, bit, entityData.position);
    readField(bytes, mask, bit, entityData.rotation);
    readField(bytes, mask, bit, entityData.velocity);
    readField(bytes, mask, bit, entityData.acceleration);
    readField(bytes, mask, bit, entityData.retreat);
    readField(bytes, mask, bit, entityData.headRotation);
    readField(bytes, mask, bit, entityData.flags);
    readField(bytes, mask, bit, entityData.hostilityType);
    readField(bytes, mask, bit, entityData.race);
    readField(bytes, mask, bit, entityData.currentAbility);
    readField(bytes, mask, bit, entityData.timeSinceAbility);
    readField(bytes, mask, bit, entityData.hitCombo);
    readField(bytes, mask, bit, entityData.timeSinceHit);
    readField(bytes, mask, bit, entityData.appearance);
    readField(bytes, mask, bit, entityData.binaryToggles);
    readField(bytes, mask, bit, entityData.rollTime);
    readField(bytes, mask, bit, entityData.stunTime);
    readField(bytes, mask, bit, entityData.actionLockTime);
    readField(bytes, mask, bit, entityData.slowedTime);
    readField(bytes, mask, bit, entityData.sprintTime);
    readField(bytes, mask, bit, entityData.revivalPenalty);
    readField(bytes, mask, bit, entityData.level);
    readField(bytes, mask, bit, entityData.xp);
    readField(bytes, mask, bit, entityData.classType);
    readField(bytes, mask, bit, entityData.specialization);
    readField(bytes, mask, bit, entityData.charge);
    readField(bytes, mask, bit, entityData.finalHandsRotation);
    readField(bytes, mask, bit, entityData.initialHandsRotation);
    readField(bytes, mask, bit, entityData.attackRotation);
    readField(bytes, mask, bit, entityData.hp);
    readField(bytes, mask, bit, entityData.mp);
    readField(bytes, mask, bit, entityData.stealth);
    readField(bytes, mask, bit, entityData.petRefreshValue);
    readField(bytes, mask, bit, entityData.mediumSpeed);
    readField(bytes, mask, bit, entityData.lampDiameter);
    readField(bytes, mask, bit, entityData.interactionState);
    readField(bytes, mask, bit, entityData.buildingModel);
    readField(bytes, mask, bit, entityData.ownerId);
    readField(bytes, mask, bit, entityData.focusId);
    readField(bytes, mask, bit, entityData.destination);
    readField(bytes, mask, bit, entityData.region);
    readField(bytes, mask, bit, entityData.origin);
    readField(bytes, mask, bit, entityData.actionItem);
    readField(bytes, mask, bit, entityData.equipment);
    readField(bytes, mask, bit, entityData.name);
    readField(bytes, mask, bit, entityData.seed);
    readField(bytes, mask, bit, entityData.factionKind);
    readField(bytes, mask, bit, entityData.factionVariant);
    readField(bytes, mask, bit, entityData.factionSeed);
    return bytes.size();
}

int encodeEntityData(BytesIO &bytes, const Creature::EntityData &previous, const Creature::EntityData &current,
                     bool full)
{
    std::list<EntityDataChunk> chunks;
    unsigned long long mask = 0;
    int bit = 0;
    addField(chunks, mask, bit, previous.position, current.position, full);
    addField(chunks, mask, bit, previous.rotation, current.rotation, full);
    addField(chunks, mask, bit, previous.velocity, current.velocity, full);
    addField(chunks, mask, bit, previous.acceleration, current.acceleration, full);
    addField(chunks, mask, bit, previous.retreat, current.retreat, full);
    addField(chunks, mask, bit, previous.headRotation, current.headRotation, full);
    addField(chunks, mask, bit, previous.flags, current.flags, full);
    addField(chunks, mask, bit, previous.hostilityType, current.hostilityType, full);
    addField(chunks, mask, bit, previous.race, current.race, full);
    addField(chunks, mask, bit, previous.currentAbility, current.currentAbility, full);
    addField(chunks, mask, bit, previous.timeSinceAbility, current.timeSinceAbility, full);
    addField(chunks, mask, bit, previous.hitCombo, current.hitCombo, full);
    addField(chunks, mask, bit, previous.timeSinceHit, current.timeSinceHit, full);
    addField(chunks, mask, bit, previous.appearance, current.appearance, full);
    addField(chunks, mask, bit, previous.binaryToggles, current.binaryToggles, full);
    addField(chunks, mask, bit, previous.rollTime, current.rollTime, full);
    addField(chunks, mask, bit, previous.stunTime, current.stunTime, full);
    addField(chunks, mask, bit, previous.actionLockTime, current.actionLockTime, full);
    addField(chunks, mask, bit, previous.slowedTime, current.slowedTime, full);
    addField(chunks, mask, bit, previous.sprintTime, current.sprintTime, full);
    addField(chunks, mask, bit, previous.revivalPenalty, current.revivalPenalty, full);
    addField(chunks, mask, bit, previous.level, current.level, full);
    addField(chunks, mask, bit, previous.xp, current.xp, full);
    addField(chunks, mask, bit, previous.classType, current.classType, full);
    addField(chunks, mask, bit, previous.specialization, current.specialization, full);
    addField(chunks, mask, bit, previous.charge, current.charge, full);
    addField(chunks, mask, bit, previous.finalHandsRotation, current.finalHandsRotation, full);
    addField(chunks, mask, bit, previous.initialHandsRotation, current.initialHandsRotation, full);
    addField(chunks, mask, bit, previous.attackRotation, current.attackRotation, full);
    addField(chunks, mask, bit, previous.hp, current.hp, full);
    addField(chunks, mask, bit, previous.mp, current.mp, full);
    addField(chunks, mask, bit, previous.stealth, current.stealth, full);
    addField(chunks, mask, bit, previous.petRefreshValue, current.petRefreshValue, full);
    addField(chunks, mask, bit, previous.mediumSpeed, current.mediumSpeed, full);
    addField(chunks, mask, bit, previous.lampDiameter, current.lampDiameter, full);
    addField(chunks, mask, bit, previous.interactionState, current.interactionState, full);
    addField(chunks, mask, bit, previous.buildingModel, current.buildingModel, full);
    addField(chunks, mask, bit, previous.ownerId, current.ownerId, full);
    addField(chunks, mask, bit, previous.focusId, current.focusId, full);
    addField(chunks, mask, bit, previous.destination, current.destination, full);
    addField(chunks, mask, bit, previous.region, current.region, full);
    addField(chunks, mask, bit, previous.origin, current.origin, full);
    addField(chunks, mask, bit, previous.actionItem, current.actionItem, full);
    addField(chunks, mask, bit, previous.equipment, current.equipment, full);
    addField(chunks, mask, bit, previous.name, current.name, full);
    addField(chunks, mask, bit, previous.seed, current.seed, full);
    addField(chunks, mask, bit, previous.factionKind, current.factionKind, full);
    addField(chunks, mask, bit, previous.factionVariant, current.factionVariant, full);
    addField(chunks, mask, bit, previous.factionSeed, current.factionSeed, full);

    bytes.write(mask);
    for (const EntityDataChunk &chunk : chunks)
        bytes.write(chunk.first, chunk.second);
    return bytes.size();
}

// A size factor by static entity type (65..93; 1 for every other type).
float entityScale(int type)
{
    switch (type) {
    case StaticEntityCageBase:
    case StaticEntityCage2Base:
    case StaticEntityCage3Base:
    case StaticEntityCage4Base:
        return 1.1f;
    case StaticEntityCauldron:
        return 1.25f;
    case StaticEntityPole:
    case StaticEntityTent:
        return 1.3f;
    case StaticEntityLegIrons:
        return 3.0f;
    case StaticEntityStreetLightUndead:
        return 2.0f;
    case StaticEntityStreetLight01:
        return 1.8f;
    default:
        return 1.0f;
    }
}

// Static entity extent in blocks; unlisted types use one block.
plasma::Vector<3, float> entitySize(int type)
{
    switch (type) {
    case StaticEntityCageBase:
    case StaticEntityCage2Base:
    case StaticEntityCage3Base:
    case StaticEntityCage4Base:
        return plasma::Vector<3, float>(3.0f, 3.0f, 4.0f);
    case StaticEntityCampfire:
        return plasma::Vector<3, float>(2.0f, 2.0f, 0.5f);
    case StaticEntityBeachTowel:
    case StaticEntityWoodMat:
        return plasma::Vector<3, float>(2.0f, 2.0f, 0.25f);
    case StaticEntityBirdStatue:
    case StaticEntityBeachUmbrella:
        return plasma::Vector<3, float>(4.0f, 4.0f, 4.0f);
    case StaticEntityBarrel:
    case StaticEntityCrate:
    case StaticEntitySpinningwheel:
    case StaticEntityLoom:
        return plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
    case StaticEntityOpenCrate:
    case StaticEntitySawbench:
    case StaticEntityWorkbench:
    case StaticEntityCustomizationBench:
        return plasma::Vector<3, float>(2.0f, 2.0f, 1.0f);
    case StaticEntityCauldron:
    case StaticEntityTent:
    case StaticEntityBigPumpkin:
    case StaticEntityBrazier:
        return plasma::Vector<3, float>(3.0f, 3.0f, 2.0f);
    case StaticEntityWagon1:
    case StaticEntityWagon2:
    case StaticEntityWitchWagon:
        return plasma::Vector<3, float>(4.0f, 4.0f, 2.5f);
    case StaticEntityStool:
        return plasma::Vector<3, float>(1.0f, 1.0f, 0.5f);
    case StaticEntityPedestal:
    case StaticEntityStoneTablet1:
    case StaticEntityStoneTablet2:
    case StaticEntityStoneTablet3:
    case StaticEntityStoneTablet4:
    case StaticEntityStoneTablet5:
    case StaticEntityStoneTablet6:
        return plasma::Vector<3, float>(1.5f, 1.5f, 2.0f);
    case StaticEntityCastleWindow:
        return plasma::Vector<3, float>(2.0f, 1.0f, 4.0f);
    case StaticEntityGate:
    case StaticEntityLockedGate:
    case StaticEntityRuneGate:
    case StaticEntityStoneGate:
    case StaticEntityStoneWall:
    case StaticEntitySpiderwebWall:
    case StaticEntityMagicBarrier:
    case StaticEntityGrid:
    case StaticEntityGoldenGate:
        return plasma::Vector<3, float>(1.0f, 0.1f, 1.0f);
    case StaticEntityDoor:
    case StaticEntityGraveStone2:
    case StaticEntityGraveStone3:
        return plasma::Vector<3, float>(2.0f, 1.0f, 3.0f);
    case StaticEntityBigDoor:
    case StaticEntityBigIronDoor:
        return plasma::Vector<3, float>(4.0f, 1.0f, 4.0f);
    case StaticEntityStreetLight01:
        return plasma::Vector<3, float>(1.0f, 1.0f, 4.0f);
    case StaticEntityWindow:
    case StaticEntityGraveStone1:
        return plasma::Vector<3, float>(2.0f, 1.0f, 2.0f);
    case StaticEntityFlowerBox01:
    case StaticEntityFlowerBox02:
    case StaticEntityFlowerBox03:
    case StaticEntityFence01:
    case StaticEntityFence02:
    case StaticEntityFence03:
    case StaticEntityFence04:
    case StaticEntityFenceIron1:
    case StaticEntityFenceIron2:
    case StaticEntityFenceIron3:
        return plasma::Vector<3, float>(2.0f, 1.0f, 1.0f);
    case StaticEntityMourningStatue:
        return plasma::Vector<3, float>(2.0f, 2.0f, 3.0f);
    case StaticEntityFurnace:
        return plasma::Vector<3, float>(3.0f, 3.0f, 3.0f);
    case StaticEntityAnvil:
        return plasma::Vector<3, float>(1.5f, 1.5f, 1.0f);
    case StaticEntityChestBase02:
    case StaticEntityRewardChest:
        return plasma::Vector<3, float>(3.0f, 2.0f, 1.0f);
    case StaticEntitySarcophagus:
        return plasma::Vector<3, float>(2.0f, 3.0f, 3.0f);
    case StaticEntityGoddess2:
        return plasma::Vector<3, float>(2.0f, 2.0f, 4.0f);
    case StaticEntityCounter:
        return plasma::Vector<3, float>(4.0f, 1.0f, 1.0f);
    case StaticEntityBench:
    case StaticEntityBenchBlack:
        return plasma::Vector<3, float>(3.0f, 0.4f, 0.4f);
    case StaticEntityStreetLight02:
    case StaticEntityStreetLight:
    case StaticEntityStreetLightUndead:
        return plasma::Vector<3, float>(2.0f, 2.0f, 8.0f);
    case StaticEntityTortureRack:
        return plasma::Vector<3, float>(1.0f, 1.5f, 1.0f);
    case StaticEntityInscribedTablet:
        return plasma::Vector<3, float>(3.0f, 1.0f, 5.0f);
    case StaticEntityTable:
        return plasma::Vector<3, float>(3.0f, 3.0f, 1.0f);
    case StaticEntityBed:
        return plasma::Vector<3, float>(1.65f, 2.1f, 1.0f);
    case StaticEntityCupboard:
    case StaticEntityShelf1:
    case StaticEntityShelf2:
    case StaticEntityShelf3:
    case StaticEntityCastleShelf1:
    case StaticEntityCastleShelf2:
    case StaticEntityCastleShelf3:
    case StaticEntityStoneShelf1:
    case StaticEntityStoneShelf2:
    case StaticEntityStoneShelf3:
    case StaticEntitySandstoneShelf1:
    case StaticEntitySandstoneShelf2:
    case StaticEntitySandstoneShelf3:
        return plasma::Vector<3, float>(3.0f, 1.0f, 4.5f);
    case StaticEntitySack:
        return plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
    default:
        return plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
    }
}

plasma::Vector<3, float> StaticEntity::scaledSize() const
{
    return entitySize(type) * scale;
}

}
