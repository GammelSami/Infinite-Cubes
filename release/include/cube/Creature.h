#include <algorithm>
// Creature represents an entity's state and behavior. This file also defines its equipment, inventory, interactions,
// player discoveries, appearance, buffs and animation state.
#pragma once
#include <array>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "cube/CreatureIds.h"
#include "cube/Fixed.h"
#include "cube/IntTriple.h"
#include "cube/Item.h"
#include "cube/WorldMap.h"
#include "plasma/Vector.h"

namespace gfx {
class SpriteBatch;
}

namespace cube {

class SpriteManager;

class BytesIO;
class ServerUpdates;

struct Equipment {
    Item head;
    Item neck;
    Item chest;
    Item feet;
    Item hands;
    Item shoulder;
    Item weaponLeft;
    Item weaponRight;
    Item ringLeft;
    Item ringRight;
    Item pet;

    // Compares the pet first, then the others in member order.
    bool operator!=(const Equipment &other) const;
};

class Inventory {
public:
    Inventory()
        : m_cursorCount(0),
          m_gold(0),
          m_manaCubes(0)
    {}

    // Tests whether the item can be accepted; currency items may update currency totals.
    bool canAdd(const Item &item, int count);
    // Chooses the inventory tab for an item without consulting this inventory's contents.
    int itemTab(const Item &item)
    {
        switch (item.category) {
        case ItemCategoryWeapon:
        case ItemCategoryChest:
        case ItemCategoryHands:
        case ItemCategoryFeet:
        case ItemCategoryShoulder:
        case ItemCategoryNeck:
        case ItemCategoryRing:
        case ItemCategoryLeftovers:
        case ItemCategoryLamp:
        case ItemCategoryEyePatch:
            return InventoryTabEquipment;
        case ItemCategoryPet:
        case ItemCategoryPetFood:
            return InventoryTabPets;
        case ItemCategoryFormula:
        case ItemCategoryMaterial:
            return InventoryTabCrafting;
        case ItemCategoryQuest:
        case ItemCategorySpecial:
        case ItemCategoryGlyph:
            return InventoryTabSpecial;
        case ItemCategoryArtifact:
            return InventoryTabArtifacts;
        default:
            return InventoryTabConsumables;
        }
    }
    // How many of the item the tab it belongs in holds.
    int countItem(const Item &item)
    {
        int tab = itemTab(item);
        if (tab >= (int)m_inventoryTabs.size())
            return 0;
        int count = 0;
        for (ItemStack &stack : m_inventoryTabs[tab]) {
            if (item == stack.item)
                count += stack.quantity;
        }
        return count;
    }

    std::vector<std::vector<ItemStack>> m_inventoryTabs;
    int m_cursorCount;
    Item m_cursorItem;
    int m_gold;
    int m_manaCubes; // the add path adds category 14 items here

    void remove(const Item &item, int count)
    {
        if (count == 0 || item.category == ItemCategoryNone)
            return;
        if (item.category == ItemCategoryCoin && item.material == ItemMaterialGold) {
            m_gold -= count;
            if (m_gold < 0)
                m_gold = 0;
            return;
        }
        for (std::vector<ItemStack> &tab : m_inventoryTabs) {
            for (ItemStack &stack : tab) {
                if (stack.item == item && stack.quantity > 0) {
                    if (count < stack.quantity) {
                        stack.quantity -= count;
                        return;
                    }
                    count -= stack.quantity;
                    stack = ItemStack();
                    if (count == stack.quantity)
                        return;
                }
            }
        }
    }

    // Adds coins to m_gold, mana cubes to m_manaCubes and other items to the specified tab (-1 selects itemTab).
    // Reuses an equal stackable item, then an empty slot, then appends a slot. A count of -1 marks an unlimited stack
    // and is stored rather than added. The inventory keeps no reference to the supplied item.
    void add(const Item &item, int count, int tab = -1)
    {
        if (count == 0) {
            return;
        }
        if (tab == -1) {
            tab = itemTab(item);
        }
        if (item.category == ItemCategoryNone) {
            return;
        }
        if ((int)m_inventoryTabs.size() <= tab) {
            m_inventoryTabs.resize(tab + 1);
        }
        if (item.category == ItemCategoryCoin && item.material == ItemMaterialGold) {
            m_gold += count;
            return;
        }
        if (item.category == ItemCategoryManaCube) {
            m_manaCubes += count;
            return;
        }
        int emptySlot = -1;
        std::vector<ItemStack> *tabs = &m_inventoryTabs[0];
        int i = 0;
        ItemStack *stacks = &tabs[tab][0];
        for (; i < (int)m_inventoryTabs[tab].size(); i++) {
            if (stacks[i].quantity == 0 && emptySlot < 0) {
                emptySlot = i;
            }
            if (itemIsStackable(item) && stacks[i].item == item) {
                if (count == -1) {
                    tabs[tab][i].quantity = -1;
                } else {
                    tabs[tab][i].quantity += count;
                }
                return;
            }
        }
        if (emptySlot >= 0) {
            m_inventoryTabs[tab][emptySlot].item = item;
            m_inventoryTabs[tab][emptySlot].quantity = count;
            return;
        }
        ItemStack stack(item, count);
        m_inventoryTabs[tab].push_back(stack);
    }
};

struct Interaction {
    Interaction()
        : position(-1, -1, -1),
          creatureId(0),
          unusedB8(0),
          unusedC0(0),
          unusedC6(1)
    {}

    Item item;
    plasma::Vector<3, int> position;
    long long creatureId;
    long long unusedB8; // Initially 0.
    int unusedC0;       // Initially 0.
    unsigned char type;
    short unusedC6; // 1 when constructed; never read
};

// Character appearance choices shared by player data.
struct CharacterStyle {
    CharacterStyle()
        : race(0),
          gender(0),
          face(0),
          haircut(0),
          hairColor(255, 255, 255)
    {}

    int race;             // Race choice from CharacterStyleWidget (0..7).
    unsigned char gender; // Gender: 0 male, 1 female.
    int face;
    int haircut;
    plasma::Vector<3, unsigned char> hairColor; // RGB hair colour; initially white (255, 255, 255).
};

// Player-only data: appearance choices, discoveries and collected items.
struct PlayerData : CharacterStyle {
    std::list<Item> knownFormulas;
    // Uses the same key type as knownMissions.
    std::set<std::array<int, 2>> knownFlightPoints;
    std::set<std::array<int, 2>> discoveredPlaces;
    std::set<std::array<int, 2>> savedCellSet;
    std::set<std::array<int, 2>> discoveredAreas;
    std::set<std::array<int, 2>> unusedCellSet; // constructed, copied and destroyed only
    std::map<std::array<int, 2>, unsigned char> placeVisibility;
    std::set<IntTriple> collectedPlacedItems;
    std::set<IntTriple> savedTripleSet;
    std::set<std::array<int, 2>> discoveredMarkers;
    std::set<std::array<int, 2>> knownMissions;
    std::set<std::array<int, 2>> solvedMissions;
    // Its difference from solvedMissions is not known.
    std::set<std::array<int, 2>> finishedMissions;
    std::map<std::array<int, 2>, float> missionTimers;
    std::map<std::array<int, 2>, unsigned int> platinum;
    std::map<std::array<int, 2>, std::list<MapPlace>> treasures;
    std::list<plasma::Vector<2, int>> visitedRegions;
    std::map<unsigned, std::set<std::array<int, 2>>> loreByRealm;
    std::map<long long, std::list<Item>> purchasesByShop;
    Item usableSpecialItem;
    Item contextItem;         // the renderer calls the Item method on it
    int contextItemCount = 0; // Initially zero; copied with the player data.
    float deathMarkerTime = 0.0f;
    plasma::Vector<3, Fixed> deathPosition;
    std::string unusedString; // constructed, copied and destroyed only
    struct NumberedString {
        int seed; // not set by the constructor
        std::string name;
    } lastWorld;
    plasma::Vector<3, float> treasureSpiritOffset;
};

// EntityData::classType and ::specialization values: ClassType and Specialization in CreatureIds.h.

class Creature {
public:
    struct EntityData {
        // The look of a creature.
        struct Appearance {
            Appearance();

            // Tests an appearanceFlags bit.
            bool hasFlag(AppearanceFlag flag) const { return (appearanceFlags & flag) != 0; }
            // Compares members and vector elements, checking wingsPosition before tailPosition.
            bool operator!=(const Appearance &other) const;

            // Never set: zeroed by the constructor, copied, compared and sent with the appearance; no code stores
            // another value.
            plasma::Vector<2, unsigned char> unsetPair;
            plasma::Vector<3, unsigned char> hairColor;
            unsigned int
                appearanceFlags; // AppearanceFlag bits; AppearanceFlagTinted, Enhanced, Large and PromotedResident scale maxHP; Tinted | Enhanced and Large armor; Tinted | Large climbingSpeed
            plasma::Vector<3, float> size;
            short headModel;
            short hairModel;
            short handsModel;
            short feetModel;
            short chestModel;
            short tailModel;
            short shoulderModel;
            short wingsModel;
            float headScale;  // Initially 1.01.
            float chestScale; // Initially 1.0.
            float handsScale; // Initially 1.0.
            float feetScale;  // Initially 0.98.
            float shoulderModelScale;
            float weaponScale;        // Initially 0.95.
            float tailScale;          // Initially 0.8.
            float shoulderArmorScale; // Initially 1.0.
            float wingScale;          // Initially 1.0.
            float chestRotation;
            plasma::Vector<3, float> handsRotation;
            float feetRotation;
            float wingsRotation;
            float tailRotation;
            plasma::Vector<3, float> chestPosition; // (0, 0, -5)
            plasma::Vector<3, float> headPosition;  // (0, 0.5, 5)
            plasma::Vector<3, float> handsPosition; // (6, 0, 0)
            plasma::Vector<3, float> feetPosition;  // (3, 1, -10.5)
            plasma::Vector<3, float> tailPosition;
            plasma::Vector<3, float> wingsPosition;
        };

        // Sixteen independent entity toggles; the comments identify the uses known here.
        struct BinaryToggles {
            BinaryToggles()
                : lampOn(0),
                  active(0),
                  faceAim(0),
                  toggle3(0),
                  noTerrainCollision(0),
                  combatSideOverride(0),
                  fastMovement(0),
                  dodge(0),
                  missionComplete(0),
                  playerAi(0),
                  leanForwardPose(0),
                  suppressResidentPromotion(0),
                  wraithTargeting(0),
                  attachedToFurniture(0),
                  magicBarriersDisabled(0),
                  removePetrification(0)
            {}

            // Compares all sixteen toggles as one value.
            bool operator!=(const BinaryToggles &other) const
            {
                return *(const unsigned short *)this != *(const unsigned short *)&other;
            }

            unsigned short lampOn : 1;
            unsigned short active : 1; // Creature::isPassive needs it clear
            unsigned short faceAim : 1;
            unsigned short toggle3 : 1;
            unsigned short noTerrainCollision : 1;
            unsigned short combatSideOverride : 1;
            unsigned short fastMovement : 1;
            unsigned short dodge : 1;
            unsigned short missionComplete : 1;
            unsigned short playerAi : 1;
            unsigned short leanForwardPose : 1;
            unsigned short suppressResidentPromotion : 1;
            unsigned short wraithTargeting : 1; // Creature::isWraithTargeting returns it
            unsigned short attachedToFurniture : 1;
            unsigned short magicBarriersDisabled : 1;
            unsigned short removePetrification : 1;
        };

        EntityData();
        // Puts the motion, ability, timers, stats, equipment, appearance and name back to their initial values (not
        // the position, the rotation, the flags, the hostility type, the race or hp). Only Creature::reset calls it.
        void reset();
        void setToggles(unsigned short mask, bool value)
        {
            unsigned short &toggles = *reinterpret_cast<unsigned short *>(&binaryToggles);
            if (value) {
                toggles |= mask;
            } else {
                toggles &= ~mask;
            }
        }

        plasma::Vector<3, Fixed> position; // World position, also loaded from saved character data.
        plasma::Vector<3, float> rotation;
        plasma::Vector<3, float> velocity;
        plasma::Vector<3, float> acceleration;
        plasma::Vector<3, float> retreat;
        float headRotation;
        unsigned int flags;
        unsigned char hostilityType;
        int race;                     // Signed Race value.
        unsigned char currentAbility; // compared with 140 by critical
        float timeSinceAbility;
        int hitCombo;
        float timeSinceHit;
        Appearance appearance;
        BinaryToggles binaryToggles;
        float rollTime;
        float stunTime; // -3000.0 when constructed
        float actionLockTime;
        float slowedTime;
        float sprintTime;
        float revivalPenalty;
        int level; // 1 when constructed; read by the skill getters
        int xp;
        unsigned char classType;      // read by armor and maxHP
        unsigned char specialization; // maxHP
        plasma::Vector<2, int> region;
        float charge;
        plasma::Vector<3, float> finalHandsRotation;
        plasma::Vector<3, float> initialHandsRotation;
        plasma::Vector<3, float> attackRotation;
        float hp; // Initially 500.0; also loaded from saved character data.
        float blockPower;
        float mp;
        float stealth; // Scaled into the critical chance by critical().
        float petRefreshValue;
        float mediumSpeed;
        float lampDiameter;             // 10.0 when constructed
        unsigned char interactionState; // tested together with race and hostility type, Game::controlCharacter
        unsigned char
            buildingModel; // Zeroed by construction and reset; synchronised as entity bit 36. No other use is known.
        long long ownerId; // Creature ID looked up by Game::controlCharacter with World::creatureByID.
        long long focusId; // Creature ID.
        plasma::Vector<2, int> destination;
        plasma::Vector<3, Fixed> origin; // World position compared with position; not initialised by the constructor.
        plasma::Vector<2, int> portCity;
        Item actionItem;
        Equipment equipment;
        char name[16]; // Character name; the loader writes the terminating null.
        long long seed;
        unsigned char factionKind;
        int factionVariant;
        long long factionSeed;
    };

    // A timed effect on the creature.
    struct Buff {
        Buff()
            : type(0),
              strength(0),
              timeRemaining(0),
              sourceId(0)
        {}

        unsigned char type;
        float
            strength; // Buff strength: a multiplier for armor and maxHP, remaining shield absorption, damage or healing per tick, tameness, or the stack count displayed as "x N", depending on the buff type.
        float timeRemaining;
        long long sourceId;
    };

    class AnimationState {
    public:
        AnimationState(Creature *creature);
        // the names.
        void addSprites(gfx::SpriteBatch &batch, SpriteManager *spriteManager, float frameTime,
                        const plasma::Vector<4, float> &color, float alpha, const Fixed &originX, const Fixed &originY,
                        int hiddenParts, Creature *mount, float depthOffset, float scale);

        Creature *m_creature;
        float m_animationTime;
        int m_animation;
        int m_emoteTime;
        int m_unused14; // not initialised by the constructor; no access in the program
        // Rotations are in degrees about x, y and z, offsets in model units.
        Sprite *m_emoteSprite;
        plasma::Vector<3, float> m_leftHandOffset;
        plasma::Vector<3, float> m_rightHandOffset;
        plasma::Vector<3, float> m_leftHandRotation;
        plasma::Vector<3, float> m_rightHandRotation;
        long long m_unused50;
        long long m_unused58;
        long long m_unused60;
        plasma::Vector<3, float> m_bodyRotation;
        plasma::Vector<3, float> m_creatureRotation;
        plasma::Vector<3, float> m_bodyOffset;
        plasma::Vector<3, float> m_headOffset;
        plasma::Vector<3, float> m_headRotation;
        plasma::Vector<3, float> m_tailRotation;
        plasma::Vector<3, float> m_handsTurn;
        float m_bodyTurn;
        plasma::Vector<3, float> m_creatureOffset;
        float m_feetTurn;
        // Floating-point animation value read by addSprites.
        float m_glideLift;
        Sprite *m_headSprite;
        Sprite *m_hairSprite;
        Sprite *m_chestSprite;
        Sprite *m_shouldersSprite;
        Sprite *m_handsSprite;
        Sprite *m_wingsSprite;
        Sprite *m_tailSprite;
        Sprite *m_feetSprite;
        Sprite *m_rightWeaponSprite;
        Sprite *m_leftWeaponSprite;
        Sprite *m_shoulderArmorSprite;
        Sprite *m_neckSprite;
        plasma::Matrix<float> m_bodyMatrix;
        plasma::Matrix<float> m_handsMatrix;
        plasma::Matrix<float> m_chestMatrix;
        plasma::Matrix<float> m_headMatrix;
        plasma::Matrix<float> m_hairMatrix;
        plasma::Matrix<float> m_leftShoulderMatrix;
        plasma::Matrix<float> m_rightShoulderMatrix;
        plasma::Matrix<float> m_rightWeaponMatrix;
        plasma::Matrix<float> m_leftWeaponMatrix;
        plasma::Matrix<float> m_leftHandMatrix;
        plasma::Matrix<float> m_rightHandMatrix;
        plasma::Matrix<float> m_shoulderArmorMatrix;
        plasma::Matrix<float> m_neckMatrix;
        plasma::Matrix<float> m_leftWingMatrix;
        plasma::Matrix<float> m_rightWingMatrix;
        plasma::Matrix<float> m_leftFootMatrix;
        plasma::Matrix<float> m_rightFootMatrix;
        plasma::Matrix<float> m_tailMatrix;
        float
            m_scale; // 1.0 when constructed; a factor on the model's scale (addSprites scales the creature's matrix by it)
        // Animation motion is not otherwise documented here.
        plasma::Vector<3, float> m_trails[4][16];
        // Not read by addSprites.
        plasma::Vector<3, float> m_blendedFacing;
        unsigned char m_spritesInitialized;
        Fixed
            m_originX; // the origin of the last addSprites call (16.16 fixed point); the trails are kept relative to the drawn position from it
        Fixed m_originY;
        std::list<plasma::Vector<3, float>> m_points;
    };

    // Unused record type: neither m_unusedRecordMap nor m_unusedRecords is filled, so no record is read.
    struct UnusedMapRecord {
        unsigned char bytes[0x20];
    };
    struct UnusedVectorRecord {
        unsigned char bytes[0x20];
    };

    explicit Creature(const long long &id);
    virtual ~Creature();
    // The three phases of an ability (-1: the current one), in milliseconds, most of them divided by tempo(): the time
    // before it takes effect, the time of the effect and the time after it.
    int anticipationTime(int ability = AbilityUseCurrent);
    // While rolling, a class 4 creature of specialization 1 gains 0.25 mp and the buff 11 for 30000 ms, announced in
    // updates.
    void applyDodgeBonus(ServerUpdates &updates);
    float armor();
    // The time the current skill takes; compared with timeSinceAbility.
    int attackDuration();
    // Sums Item::GetAttackPower over the weapons (all equipped ones, or only those the current ability uses), scales
    // by stealth, the current ability, the appearance flags and the buffs 23 and 33.
    float attackPower(bool allWeapons);
    // The weapons that count for the current skill (currentAbility): a two-handed weapon of id 6 or 7 alone, otherwise
    // the left, the right or both by skill.
    std::list<Item *> attackWeapons();
    // 5 plus the weapons' damage (with a shield, id 13, in the left slot: four times the right weapon's damage, six
    // times for class 1 specialization 1), scaled by the appearance flags like maxHP.
    float blockPower();
    // Hostility types 0, 1, 3 and 6, and type 7 unless the race is 140..142.
    bool canBeDamaged();
    // Whether this creature and other may hurt each other (by hostility types, appearance flags and the binaryToggles
    // bit 5).
    bool canDoDamage(Creature *other);
    // Creatures other than the player (hostilityType not 0) can use every item; the player those of the class
    // (Item::fitsClass).
    bool canEquipItem(const Item &item);
    // Whether one more of the item fits in the inventory: counted items (categories 1, 11, 21 and 24) are limited to 1
    // (21, 24), 5 (category 1 ids 12 and 14), 10 (category 11 ids 39..41) or 50.
    bool canTakeItem(const Item &item);
    // Whether the skill can start now: not while it runs, not stunned (except skill 113), enough mp and stamina, no
    // cooldown left.
    bool canUseSkill(int skill, bool flag);
    // The movement skills: each maps a skill level (the artifact skill for the player, ten times the level for others)
    // onto a speed.
    float climbingSpeed();
    // The skill's cooldown time in milliseconds (0 without one; skills 50 and 108: 1800 instead of 16000 with a buff
    // of type 13).
    int cooldown(int skill);
    bool keepsCooldown(int skill);
    float critical(Creature *otherCreature, bool otherCreatureBasedOnResistance);
    // What an attacker's critical chance loses against this creature: armor / 2^4 * 0.05, a quarter of it with a buff
    // of type 14.
    float damageReduction();
    // The factor of the skill's damage (1.0 for most).
    float damageScale(int skill);
    // The sum of Item::demonDamageBonus over the filled slots of inventory tab 1 (0 with fewer than two tabs).
    float demonDamageBonus();
    // The movement skills: each maps a skill level (the artifact skill for the player, ten times the level for others)
    // onto a speed.
    float divingSkill();
    // Gives the creature the equipment of its race, class and flags (equip with the creature's own members).
    void equipByRace();
    // The weapon slots (right, then left) that hold a weapon. attackPower and spellPower sum over it.
    std::list<Item *> equippedWeapons();
    bool experiencesBuoyancy();
    // The first buff of that type in m_buffs, or null (HelpWidget: type 13 makes the stamina costs disappear).
    Buff *findBuff(int type);
    float hangGlidingSpeed();
    // False without character data.
    bool hasCompletedMission(int x, int y);
    bool hasFinishedMission(int x, int y);
    // Whether the inventory tabs hold at least count of the item, summed over all stacks that equal it.
    bool hasItem(const Item &item, int count);
    bool hasRandomChance(float chance);
    // Whether the inventory holds the special item (category 24) of that id from the creature's region; id 4 needs no
    // item.
    bool hasSpecialItem(int id);
    bool hasTwoHandedWeapon();
    int holdTime(int skill = AbilityUseCurrent);
    // std::max(graphicsScale, hitboxScale, physicsScale) / 2 + 5, or graphicsScale / 2 + 40 during skill 93.
    float interactionRange();
    // isCaster, the classes 149 and 150, or a weapon of id 6, 7, 8, 10 or 11 in either hand.
    bool isAttackingFromRange();
    // While m_entityData.actionLockTime is not positive: SkillBlock (9), SkillCyclone (95) before attackDuration,
    // skill 110, or SkillCrush (60) or SkillSwirl (64) as class 1 with specialization 1.
    bool isBlocking();
    bool isBoosted();
    // Hostility type 8 (the type of objects) and race 194..197, which Speech's entity names call Cage1..Cage4.
    bool isCage();
    bool isCaster();
    // Race 181..184: WoodChest, SkullChest, ObsidianChest, BoneChest.
    bool isChest();
    // Hostility type 8 and race 244..247 (all four named Gate).
    bool isGate();
    bool isGliding();
    bool isLarge();
    // Race 234 or 235: Lever, LockedLever.
    bool isLever();
    bool isMarked();
    bool isObstacle();
    bool isPassive();
    bool isWraithTargeting();
    float knockback(int skill);
    // False without character data.
    bool knowsFlightPoint(const plasma::Vector<2, int> &flightPoint);
    bool knowsFormula(const Item &item);
    // Whether the place at (x, y) is in the character data's knownMissions or solvedMissions.
    bool knowsMission(int x, int y);
    // The light radius from the lamp skill (see divingSkill).
    float lampDiameter();
    float magicDamageReduction();
    // The mana a skill costs (0.3 for the skills that buff 9 makes free).
    float manaCost(int skill, int unusedArgument);
    float manaGeneration();
    // The id stored in buff 18, else the non-zero key of m_aggro with the largest value (0 if none).
    long long maxAggroTarget();
    // The hit combo at which the combo bonuses of tempo are complete (by the right weapon's id).
    int maxCombo()
    {
        if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon) {
            switch (m_entityData.equipment.weaponRight.id) {
            case WeaponDagger:
            case WeaponFist:
            case WeaponStaff:
                return 50;
            case WeaponWand:
            case WeaponBracelet:
                return 20;
            case WeaponGreatsword:
            case WeaponGreataxe:
            case WeaponGreatmace:
                return 15;
            case WeaponBoomerang:
                return 80;
            case WeaponBow:
                return 60;
            default:
                return 30;
            }
        }
        if (m_entityData.appearance.appearanceFlags & AppearanceFlagBiteAttack) {
            return 15;
        }
        return 30;
    }
    // The maximum of EntityData::hp: by class, level, appearance flags, the equipment's Item::hp and the buffs 22 and
    // 32.
    float maxHP();
    // How long a hit or a fall stuns the creature, in milliseconds: 1500 for the player (hostilityType 0), 3000 for
    // others.
    int maxStunTime();
    // The skill the attack and the special attack buttons use now (by class, specialization and weapons; alternative:
    // the class skill button is held, Game::m_controlStates[2]).
    int nextAttackSkill(bool alternative);
    int nextSpecialAttackSkill(bool alternative);
    // Walks m_skillCooldowns and does nothing with the entries.
    void onHit();
    // Start the attack or the special attack (nextAttackSkill, nextSpecialAttackSkill) if the current skill has run
    // long enough and mana (and, for the attack, stamina) suffice: sets currentAbility and restarts timeSinceAbility.
    bool performAttack(bool alternative);
    bool performSpecialAttack(bool alternative);
    // The platinum coins of the character in the region it is in (0 without character data).
    int platinum();
    float regeneration();
    void removeItem(const Item &item, int count);
    // Puts the creature back into its initial state (entity data, buffs, inventory, interactions, cooldowns, timers);
    // the position, the hp and the artifact skills stay.
    void reset();
    // The magic counterpart of armor: sums Item::resistance.
    float resistance();
    float ridingSpeed();
    float sailingSpeed();
    // Adds the buff, or replaces one of the same type and sourceId (always when replace is set, else only for the
    // types 1..3, 6, 7 and 9..11).
    void setBuff(const Buff &buff, bool replace);
    // The position, moved forward by half the hitbox scale and up by 0.35 of the physics scale for creatures with
    // AppearanceFlagFlying.
    plasma::Vector<3, Fixed> sightPosition();
    // The level of a skill: 1000 for every skill in this release (callers test it for > 0 and pass it to the functions
    // of a level).
    int skillLevel(int skill);
    // The class skill by class and specialization (148/149, 143/142, 47/144, 109/141; 0 for other classes).
    int specialSkill();
    // 5 plus Item::weaponDamage over the weapons (all equipped ones, or only those the current ability uses), scaled
    // by the appearance flags and the buff 23.
    float spellPower(bool allWeapons);
    // The stamina a skill takes (0 with a buff of type 13).
    float staminaCost(int skill);
    void stopActions();
    // The time of the effect of an ability (-1: the current one), in milliseconds.
    int strikeTime(int ability = AbilityUseCurrent);
    float swimmingSpeed();
    float tempo();
    // The xp gained in all levels below the current one (10 * 2^((n - 1) / 2) for level n) plus the xp in the current
    // level.
    int totalXP();
    // Zeroes m_artifactSkills, sets the level to 1, then for every artifact in inventory tab 5 adds one level and one
    // point to the skill Item::artifactType selects.
    void updateLevelAndSkills();
    void greet(int &seed, std::vector<std::string> &text);

    long long m_id; // *id
    EntityData m_entityData;
    std::list<Buff> m_buffs; // walked by armor and maxHP
    float m_stepOffset;
    float m_hitFlash;
    // CharacterPreviewWidget::draw eases it to 0, or sets 0.2 while the pointer is over the preview.
    float m_walkAmplitude;
    float m_walkPhase;
    float m_stamina; // Initially 1.0; canUseSkill and performAttack reject skills whose stamina cost exceeds it.
    // Pet water level; no depletion or display use is known.
    float m_petWaterLevel; // 1.0 when constructed; saved with the character
    int m_unreadSavedInt;
    plasma::Vector<3, float> m_wallNormal;
    std::set<long long> m_struckCreatures; // Map keyed by signed creature IDs.
    std::set<long long> m_dodgingCreatures;
    long long m_petId;
    long long m_opponentId;
    float m_leapDuration; // Flight duration in milliseconds, derived from distance at 50 blocks per second.
    Inventory m_inventory;
    std::list<Interaction> m_interactions;
    // Only constructed and destroyed in the examined code.
    std::map<long long, std::vector<UnusedMapRecord>> m_unusedRecordMap;
    int m_strikeCombo;
    float m_rollTime; // Roll timer used by World::updateSurvival; no reader is known.
    plasma::Vector<3, Fixed> m_abilityTargetPosition;
    // Start values for interpolation over m_smoothingTime milliseconds.
    plasma::Vector<3, Fixed> m_previousDrawnPosition;
    plasma::Vector<3, Fixed> m_drawnPosition;
    plasma::Vector<3, float> m_previousDrawnRotation; // see m_previousDrawnPosition; not initialised by the constructor
    plasma::Vector<3, float> m_drawnRotation;         // Initially zero.
    plasma::Vector<3, float>
        m_previousDrawnAttackRotation;              // see m_previousDrawnPosition; not initialised by the constructor
    plasma::Vector<3, float> m_drawnAttackRotation; // Initially zero.
    float m_smoothingTime;
    std::array<int, 7> m_artifactSkills;
    std::map<int, float> m_skillCooldowns;
    std::map<long long, float> m_aggro;
    std::map<long long, float> m_hitCounts;
    float m_strikeStrength; // strikeTime uses it, in seconds, for ability 148
    unsigned char m_strikeLanded;
    float m_fallStartHeight;
    float m_glideTime; // World compares this timer with 3000.0f
    unsigned char m_criticalStrike;
    float m_airTime;
    float m_damageDealt;      // Total damage dealt, accumulated from hit packets by Game.
    unsigned int m_unusedBc4; // zeroed by the constructor; no other access in the program
    unsigned int
        m_resetOnHit; // Zeroed by reset and on the attacker of each hit packet in Game::processHitLights; never increased or read.
    unsigned int m_behaviours;
    // Meaning not known.
    std::vector<int> m_specialSkills; // Record elements whose meaning is unknown.
    int m_summonAppearanceMode;       // 1 when constructed
    std::vector<int> m_summonRaces;   // World chooses a random entry for a new creature, defaulting to 37.
    std::set<long long> m_groupMembers;
    long long m_unusedC18; // zeroed by the constructor and reset; no other access
    std::vector<UnusedVectorRecord> m_unusedRecords;
    AnimationState m_animationState;
    // Opaque record storage.
    PlayerData *m_playerData;
    float m_copiedToPet; // 200.0 when constructed; World::updateTamed copies the owner's to a new pet; nothing reads it
    unsigned char m_unused1534[0xc]; // Not initialised by the constructor; no access in the program.
    unsigned char m_unused1540;      // Initially zero; no other access.
    // Cached value synchronised from the pet; its meaning is unknown.
    float m_lastPetSyncValue; // World compares the cached petRefreshValue as float
    Item m_lastPetItem;
    std::list<plasma::Vector<3, int>> m_debugBlocks;
};

// Races 1, 3, 5, 8, 10, 12, 14, 16 (ElfFemale .. UndeadFemale in Speech's entity names), 84 and 228
// (DarkCultistFemale).
bool isFemale(unsigned int race);
// Returns an integer truth value.
int isHarmfulSkill(int skill);
// Tests for SkillHealingStream (134 or 135), returning an integer truth value.
int isHealingSkill(int skill);
bool isPet(int race);
int maxExperience(int level);

bool areEnemies(const Creature *a, const Creature *b);

// Sets an appearance (models, scales, offsets, flags, size) from the race. With a style the race is first replaced by
// the player race the style's race and gender choose; without one (style == 0) face, haircut and hair colour are
// random. The meaning of the last argument is not known: where set, some races get AppearanceFlagMarked.
void fillAppearance(int &race, Creature::EntityData::Appearance &appearance, const CharacterStyle *style,
                    bool allowMarked);

// The factor of the gold a killed creature drops, by its level (passed as a float): 1 + 0.1 per level above 1.
float prizeScale(float level);

// Values rising from x at level 1 towards 2x: x + x * (1 - 1 / ((level - 1) / 50 + 1)).
int rampDuration(int level);
float rampRange(int level);
float rampScale(int level);
float rampSpeed(int level);

plasma::Vector<3, float> riderOffset(int race);

int shelterDuration(int level);

float skillScale(int level);

// Parameters correspond to the Creature members passed by equipByRace.
void equip(int hostilityType, int race, int rarity, unsigned int flags, int factionKind, unsigned char &classType,
           unsigned char &specialization, Equipment &equipment, std::vector<int> &specialSkills,
           int &summonAppearanceMode, std::vector<int> &summonRaces, Inventory &inventory);

// A classification of the race (3: equip adds up to two of item category 1 id 1 to the inventory).
int speciesType(int race);

// Generates a two-syllable name from the seed and the race's male or female syllable lists. Unsupported races use the
// general lists.
std::wstring randomName(int seed, int race);

// 1.2 + 1.8 * (1 - 1 / ((level - 1) / 50 + 1)): rises from 1.2 towards 3. A free function (only an int argument).
float bonusFromLevel(int level);

// The writer stores a mask and the members of current that differ from previous, or all members when full is set. The
// reader applies that record to entityData. Both return the resulting byte-buffer size.
int decodeEntityData(BytesIO &bytes, Creature::EntityData &entityData);
int encodeEntityData(BytesIO &bytes, const Creature::EntityData &previous, const Creature::EntityData &current,
                     bool full);

// WorldMap::isMarked (declared in WorldMap.h, which this header includes before Creature is defined): whether the
// creature's id is among the map's marked creatures.
inline bool WorldMap::isMarked(Creature *creature)
{
    return m_creatures.find(creature->m_id) != m_creatures.end();
}

}
