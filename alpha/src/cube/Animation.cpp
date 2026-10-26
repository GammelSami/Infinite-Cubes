// Creature animation updates and renders the parts of each creature model.
#include <math.h>
#include "cube/Animation.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/CubeShader.h"
#include "cube/DefaultBlocks.h"
#include "cube/ItemIds.h"
#include "cube/RaceIds.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

#include "cube/SpriteIds.h"

namespace cube {

namespace {
enum {
    BowDrawPoseDurationMultiplier = 2,
    CycloneWindupMs = 600,
    QuakePoseStrikeExtensionMs = 400,
    ParryWindupMs = 100,
    ParryFollowThroughStartMs = 500,
};
}

Creature::AnimationState::AnimationState(Creature *creature)
    : m_creature(creature),
      m_animationTime(0),
      m_animation(AnimationNone),
      m_leftHandOffset(0, 0, 0),
      m_rightHandOffset(0, 0, 0),
      m_leftHandRotation(0, 0, 0),
      m_rightHandRotation(0, 0, 0),
      unused03c(0),
      unused040(0),
      unused044(0),
      unused048(0),
      unused04c(0),
      unused050(0),
      m_bodyRotation(0, 0, 0),
      m_creatureRotation(0, 0, 0),
      m_bodyOffset(0, 0, 0),
      m_headOffset(0, 0, 0),
      m_headRotation(0, 0, 0),
      m_tailRotation(0, 0, 0),
      m_handsTurn(0, 0, 0),
      m_bodyTurn(0),
      m_creatureOffset(0, 0, 0),
      m_glideLift(0),
      m_headSprite(0),
      m_chestSprite(0),
      m_shouldersSprite(0),
      m_handsSprite(0),
      m_wingsSprite(0),
      m_tailSprite(0),
      m_feetSprite(0),
      m_rightWeaponSprite(0),
      m_shoulderArmorSprite(0),
      m_neckSprite(0),
      m_blendedFacing(0, 0, 0),
      m_hasSpriteUpdate(false),
      m_originX(0),
      m_originY(0)
{}

#ifndef CUBE_SERVER // Server builds do not render creatures.
// Poses and draws up to twelve model sprites using rotations in degrees, offsets in model units
// and blended poses. Equipment replaces body models; the walk cycle supplies the rest pose, each
// Animation supplies targets, and stun, climbing and lamp state add corrections. Model x points
// sideways, y forwards and z up; left parts have negative x and right limbs mirror the left
// models.
//
// milliseconds is the blend interval, zero for after-images. color multiplies the material color;
// OriginX and originY use 16.16 world coordinates. hiddenParts masks the head, hair, shoulder
// armor, chest, hands, weapons and feet. mount supplies rider offsets when riding. Curves use
// radians for sine and cosine phases and milliseconds for timing.
void Creature::AnimationState::addSprites(CubeShader *shader, const plasma::Matrix<float> &view,
                                          const plasma::Matrix<float> &projection, SpriteManager *spriteManager,
                                          int milliseconds, const plasma::Vector<4, float> &color, const Fixed &originX,
                                          const Fixed &originY, int hiddenParts, Creature *mount)
{
    m_hasSpriteUpdate = true;
    m_originX = originX;
    m_originY = originY;

    // --- The sprites --- Worn equipment replaces the body's own model of that part; the head, the
    // hair, the shoulders, the tail and the wings come from the appearance.
    m_headSprite = 0;
    m_hairSprite = 0;
    m_shouldersSprite = 0;
    m_handsSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.hands);
    m_feetSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.feet);
    m_shoulderArmorSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.shoulder);
    m_neckSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.neck);
    m_rightWeaponSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponRight);
    m_leftWeaponSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponLeft);
    m_chestSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.chest);
    m_tailSprite = 0;
    m_wingsSprite = 0;
    if (m_headSprite == 0) {
        m_headSprite = spriteManager->sprite(m_creature->m_entityData.appearance.headModel);
        m_hairSprite = spriteManager->sprite(m_creature->m_entityData.appearance.hairModel);
    }
    if (m_handsSprite == 0)
        m_handsSprite = spriteManager->sprite(m_creature->m_entityData.appearance.handsModel);
    if (m_feetSprite == 0)
        m_feetSprite = spriteManager->sprite(m_creature->m_entityData.appearance.feetModel);
    if (m_chestSprite == 0)
        m_chestSprite = spriteManager->sprite(m_creature->m_entityData.appearance.chestModel);
    if (m_tailSprite == 0)
        m_tailSprite = spriteManager->sprite(m_creature->m_entityData.appearance.tailModel);
    if (m_wingsSprite == 0)
        m_wingsSprite = spriteManager->sprite(m_creature->m_entityData.appearance.wingsModel);
    if (m_shouldersSprite == 0)
        m_shouldersSprite = spriteManager->sprite(m_creature->m_entityData.appearance.shoulderModel);

    // --- The rest pose, from the appearance --- The animation cases change these copies.
    float headScale = m_creature->m_entityData.appearance.headScale;
    float chestScale = m_creature->m_entityData.appearance.chestScale;
    float handsScale = m_creature->m_entityData.appearance.handsScale;
    float feetScale = m_creature->m_entityData.appearance.feetScale;
    float shouldersScale = m_creature->m_entityData.appearance.shoulderModelScale;
    float rightWeaponScale = m_creature->m_entityData.appearance.weaponScale;
    float leftWeaponScale = m_creature->m_entityData.appearance.weaponScale;
    float tailScale = m_creature->m_entityData.appearance.tailScale;
    float shoulderArmorScale = m_creature->m_entityData.appearance.shoulderArmorScale;
    plasma::Vector<3, float> chestPosition = m_creature->m_entityData.appearance.chestPosition;
    plasma::Vector<3, float> headPosition = m_creature->m_entityData.appearance.headPosition;
    // Extra turns of the weapon in each hand; no case sets them, so they stay zero (the in-hand weapon
    // matrices still apply them).
    plasma::Vector<3, float> rightWeaponTurn(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftWeaponTurn(0.0f, 0.0f, 0.0f);
    // The hands' own rotation applies only without an animation; the left hand's is mirrored.
    plasma::Vector<3, float> leftHandAngles(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> rightHandAngles(0.0f, 0.0f, 0.0f);
    if (m_animation == AnimationNone) {
        leftHandAngles = m_creature->m_entityData.appearance.handsRotation;
        rightHandAngles = m_creature->m_entityData.appearance.handsRotation;
        leftHandAngles[1] = -leftHandAngles[1];
        leftHandAngles[2] = -leftHandAngles[2];
    }

    // --- The walk cycle --- m_walkAmplitude is the cycle's weight and m_walkPhase its phase
    // (Creature.h). armSwing is the angle (radians) the arms swing by, damped for what the hands are
    // doing; swing keeps it before the weapon and item dampings.
    float cycle = sinf((float)(m_creature->m_walkPhase + 1.5707963267948966));
    float armSwing = (float)(m_creature->m_walkAmplitude * 2.199114820062152 * cycle);
    unsigned char ability = m_creature->m_entityData.currentAbility;
    if (ability == AbilitySneak)
        armSwing *= 0.5f;
    if (ability == AbilityRidePet || ability == AbilitySailBoat)
        armSwing *= 0.1f;
    // Leaning forward at speed: running while pushing and moving, or the intercept.
    bool leaningForward;
    if ((!(m_creature->m_entityData.binaryToggles.climbing &&
           (m_creature->m_entityData.flags & (1 << CreatureFlagAtWall))) &&
         m_creature->m_entityData.binaryToggles.fastMovement &&
         m_creature->m_entityData.acceleration.squaredLength() > 0.1f &&
         m_creature->m_entityData.velocity[0] * m_creature->m_entityData.velocity[0] +
                 m_creature->m_entityData.velocity[1] * m_creature->m_entityData.velocity[1] >
             0.1f) ||
        ability == AbilityIntercept) {
        armSwing *= 1.25f;
        leaningForward = true;
    } else {
        leaningForward = false;
    }
    // A flier in the air does not swing; a four-legged creature less.
    if ((m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagFlying)) &&
        !(m_creature->m_entityData.flags & (1 << CreatureFlagOnGround)))
        armSwing = 0.0f;
    if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped))
        armSwing *= 0.75f;
    float swing = armSwing;
    if (m_creature->m_entityData.equipment.weaponRight.id == WeaponLongsword && ability != AbilityNone)
        armSwing *= 0.25f;
    if ((m_animation == AnimationDrink || m_animation == AnimationEat) &&
        m_creature->m_entityData.timeSinceAbility < m_creature->m_entityData.actionItem.cooldown())
        armSwing *= 0.1f;
    if (m_animation == AnimationOfferPetFood)
        armSwing *= 0.1f;
    if (ability == AbilityShoot || ability == AbilityChargeFireball || ability == AbilitySwirl ||
        ability == AbilityBlock || ability == AbilitySupportRay || m_animation == AnimationTwoHandedSweep ||
        m_animation == AnimationTwoHandedThrust || m_animation == AnimationTwoHandedSpin ||
        m_animation == AnimationGuardianCrushStrike || m_animation == AnimationTwoHandedSwing)
        armSwing *= 0.5f;
    if (m_creature->m_entityData.binaryToggles.gliding)
        armSwing = 0.0f;

    // --- The target pose --- What the animation asks for; the state's pose members move towards these
    // after the switch.
    plasma::Vector<3, float> rightHandRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftHandRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftHandOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> rightHandOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> bodyRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> creatureRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> bodyOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> headRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> tailRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> creatureOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> headOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> handsTurn(0.0f, 0.0f, 0.0f);
    float bodyTurn = 0.0f;
    float glideLift = 0.0f;
    if (m_creature->isGliding())
        glideLift = m_creature->m_entityData.appearance.size[2] * 0.5f;
    // With faceAim the body turns from the facing direction (attackRotation) towards the acceleration:
    // the angle between them in the ground plane, in degrees, reversed when they point apart.
    if (m_creature->m_entityData.binaryToggles.faceAim) {
        float pushSquared = m_creature->m_entityData.acceleration[0] * m_creature->m_entityData.acceleration[0] +
                            m_creature->m_entityData.acceleration[1] * m_creature->m_entityData.acceleration[1];
        if (pushSquared > 5.0f) {
            float facingSquared =
                m_creature->m_entityData.attackRotation[0] * m_creature->m_entityData.attackRotation[0] +
                m_creature->m_entityData.attackRotation[1] * m_creature->m_entityData.attackRotation[1];
            if (facingSquared > 0.0f) {
                float pushScale = 1.0f / sqrtf(pushSquared);
                float pushX = m_creature->m_entityData.acceleration[0] * pushScale;
                float pushY = m_creature->m_entityData.acceleration[1] * pushScale;
                float facingScale = 1.0f / sqrtf(facingSquared);
                float facingX = m_creature->m_entityData.attackRotation[0] * facingScale;
                float facingY = m_creature->m_entityData.attackRotation[1] * facingScale;
                float sine = facingX * pushY - facingY * pushX;
                if (sine < -1.0f)
                    sine = -1.0f;
                else if (sine > 1.0f)
                    sine = 1.0f;
                bodyTurn = (float)(asinf(sine) * 57.29577951308232);
                if (facingY * pushY + facingX * pushX < -0.2f)
                    bodyTurn = -bodyTurn;
            }
        }
    }

    // --- The feet's part of the walk cycle --- The swing of each foot about the x axis in degrees
    // (the left forwards while the right is back), the feet's roll and how far each is lifted.
    float footRoll = (float)((swing * 0.0f * 80.0f) / 3.141592653589793);
    float leftFootSwing = (float)((swing * 180.0f) / 3.141592653589793);
    float rightFootSwing = -leftFootSwing;
    if (leftFootSwing > 0.0f)
        leftFootSwing *= 1.5f;
    if (rightFootSwing > 0.0f)
        rightFootSwing *= 1.5f;
    float leftFootLift = m_creature->m_walkAmplitude * 4.0f * cycle;
    float rightFootLift = (1.0f - cycle) * (m_creature->m_walkAmplitude * 4.0f);
    if ((m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagFlying)) &&
        !(m_creature->m_entityData.flags & (1 << CreatureFlagOnGround))) {
        leftFootLift = 0.0f;
        rightFootLift = 0.0f;
    }
    if (0.0f > leftFootSwing)
        leftFootSwing *= 1.5f;
    if (0.0f > rightFootSwing)
        rightFootSwing *= 1.5f;
    // Riding a pet (not as a hostility-5 creature): the legs spread.
    if (m_creature->m_petId != 0 && m_creature->m_entityData.hostilityType != HostilityPet) {
        footRoll = 10.0f;
        leftFootSwing = 20.0f;
        rightFootSwing = 20.0f;
    }
    // Sitting, eating, riding, sailing: the legs forward.
    if (m_animation == AnimationSit || m_animation == AnimationEat || m_animation == AnimationRide ||
        m_animation == AnimationSailBoat) {
        footRoll = 10.0f;
        leftFootSwing = 40.0f;
        rightFootSwing = 40.0f;
    }
    if (m_animation == AnimationSleep) {
        leftFootSwing = -10.0f;
        rightFootSwing = -10.0f;
    }

    // --- The hands' resting places --- A four-legged creature keeps its hands (front feet) where the
    // appearance puts them; the others swing them along the walk, raised a little with the cycle.
    plasma::Vector<3, float> leftHandPosition;
    plasma::Vector<3, float> rightHandPosition;
    if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped)) {
        leftHandPosition = plasma::Vector<3, float>(-m_creature->m_entityData.appearance.handsPosition[0],
                                                    m_creature->m_entityData.appearance.handsPosition[1],
                                                    m_creature->m_entityData.appearance.handsPosition[2]);
        rightHandPosition = m_creature->m_entityData.appearance.handsPosition;
    } else {
        float reach = m_creature->m_entityData.appearance.handsPosition[0] * m_creature->m_walkAmplitude +
                      m_creature->m_entityData.appearance.handsPosition[0];
        float across = -(cosf(armSwing) * reach);
        float along = -(sinf(armSwing) * reach);
        leftHandPosition = plasma::Vector<3, float>(across, along,
                                                    m_creature->m_entityData.appearance.handsPosition[2] - 4.1f +
                                                        m_creature->m_walkAmplitude * 6.0f);
        rightHandPosition = plasma::Vector<3, float>(-across, -along,
                                                     m_creature->m_entityData.appearance.handsPosition[2] - 4.1f +
                                                         m_creature->m_walkAmplitude * 6.0f);
        leftHandPosition[1] = m_creature->m_entityData.appearance.handsPosition[1] + leftHandPosition[1];
        rightHandPosition[1] = m_creature->m_entityData.appearance.handsPosition[1] + rightHandPosition[1];
    }
    leftHandPosition[1] = 2.1f - m_creature->m_walkAmplitude * 5.0f + leftHandPosition[1];
    rightHandPosition[1] = 2.1f - m_creature->m_walkAmplitude * 5.0f + rightHandPosition[1];
    // On a pet the hands hold the reins: closer together and forwards.
    if (m_creature->m_petId != 0) {
        leftHandPosition[1] += 2.0f;
        rightHandPosition[1] += 2.0f;
        leftHandPosition[0] *= 0.5f;
        rightHandPosition[0] *= 0.5f;
    }
    // The shoulders' places, halfway between each hand and the top of the body (drawn with the
    // shoulder model).
    plasma::Vector<3, float> leftShoulderPosition(
        (m_creature->m_entityData.appearance.handsPosition[0] * -0.3f + leftHandPosition[0]) * 0.5f,
        (leftHandPosition[1] - 8.0f) * 0.5f,
        (leftHandPosition[2] + (m_creature->m_entityData.appearance.handsPosition[2] + 12.0f)) * 0.5f);
    leftShoulderPosition[1] *= 0.5f;
    plasma::Vector<3, float> rightShoulderPosition(
        (m_creature->m_entityData.appearance.handsPosition[0] * 0.3f + rightHandPosition[0]) * 0.5f,
        (rightHandPosition[1] - 8.0f) * 0.5f,
        (rightHandPosition[2] + (m_creature->m_entityData.appearance.handsPosition[2] + 12.0f)) * 0.5f);
    rightShoulderPosition[1] *= 0.5;
    plasma::Vector<3, float> leftFootPosition(-m_creature->m_entityData.appearance.feetPosition[0],
                                              m_creature->m_entityData.appearance.feetPosition[1],
                                              m_creature->m_entityData.appearance.feetPosition[2]);
    plasma::Vector<3, float> rightFootPosition = m_creature->m_entityData.appearance.feetPosition;
    plasma::Vector<3, float> tailPosition = m_creature->m_entityData.appearance.tailPosition;
    // Arrows in the left hand are drawn on the back, not in the hand.
    Sprite *arrowsSprite = 0;
    if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        m_creature->m_entityData.equipment.weaponLeft.id == WeaponArrows) {
        arrowsSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponLeft);
        m_leftWeaponSprite = 0;
    }

    float feetTurn = 0.0f;          // added to both feet's swing (a kick)
    bool leftFootTurnedOut = false; // the left foot turned out 60 degrees (a stance)
    float blendRate =
        0.01f; // how fast the pose follows the target (per millisecond) Sneaking: crouched, the head held up, the
    // hands raised (more while attacking).
    if (m_creature->m_entityData.binaryToggles.sneaking) {
        creatureRotation = plasma::Vector<3, float>(-70.0f, 0.0f, 0.0f);
        creatureOffset[2] =
            (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        headRotation[0] = 70.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, 1.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, 1.0f);
        headOffset = plasma::Vector<3, float>(0.0f, -1.0f, 0.0f);
        if (m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow)
            rightHandOffset[1] = 11.0f;
        leftFootSwing = -20.0f;
        rightFootSwing = -20.0f;
        if (m_creature->m_entityData.currentAbility != AbilityNone &&
            m_creature->m_entityData.timeSinceAbility < m_creature->attackDuration()) {
            leftHandOffset += plasma::Vector<3, float>(0.0f, -5.0f, 6.0f);
            rightHandOffset += plasma::Vector<3, float>(0.0f, -5.0f, 6.0f);
        }
    }

    // The animation switch computes the target pose. Case comments describe the pose and the abilities that select it.
    switch (m_animation) {
    case AnimationSit: // sitting (AbilitySit; AnimationSit)
        creatureOffset[2] = -0.14f;
        bodyRotation[0] = 10.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        headOffset = plasma::Vector<3, float>(0.0f, 2.0f, -2.0f);
        break;
    case AnimationPresentItem: // ability 109: holding sprite 2566 up for five seconds (AnimationPresentItem)
        if (m_animationTime < 5000) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = 30.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
        }
        break;
    case AnimationRide: // ability 106: riding the mount (AnimationRide)
        if (mount) {
            creatureOffset[1] -= 0.25f;
            creatureOffset[2] =
                mount->m_entityData.appearance.size[2] * 0.75f + leftFootLift * 0.3f + creatureOffset[2];
            creatureOffset += riderOffset(mount->m_entityData.race);
            if (mount->m_entityData.race == RaceAlpaca || mount->m_entityData.race == RaceBrownAlpaca)
                creatureOffset[2] -= 0.75f;
            glideLift = -creatureOffset[2];
        }
        bodyRotation[0] = 20.0f;
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, -3.0f);
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, -3.0f);
        headOffset = plasma::Vector<3, float>(0.0f, 2.0f, -2.0f);
        leftHandRotation = plasma::Vector<3, float>(60.0f, 90.0f, 60.0f);
        rightHandRotation = plasma::Vector<3, float>(60.0f, -90.0f, -60.0f);
        break;
    case AnimationSailBoat: // ability 107: sailing (AnimationSailBoat)
        bodyRotation[0] = 20.0f;
        creatureOffset[2] += 1.25f;
        creatureRotation[2] = -50.0f;
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, -3.0f);
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, -3.0f);
        headOffset = plasma::Vector<3, float>(0.0f, 1.0f, 0.0f);
        headRotation = plasma::Vector<3, float>(0.0f, 0.0f, 30.0f);
        leftHandRotation = plasma::Vector<3, float>(60.0f, 90.0f, 60.0f);
        rightHandRotation = plasma::Vector<3, float>(60.0f, -90.0f, -60.0f);
        break;
    case AnimationSleep: // sleeping (AbilitySleep; AnimationSleep): lying on the back
        creatureRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
        headRotation[0] = 20.0f;
        creatureOffset[2] =
            (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f - 0.15f;
        headOffset = plasma::Vector<3, float>(0.0f, -1.0f, 0.0f);
        leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        break;
    case AnimationEat: // eating (AbilityEat; AnimationEat): seated, then as drinking
        creatureOffset[2] = -0.14f;
        bodyRotation[0] = 10.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        headOffset = plasma::Vector<3, float>(0.0f, 2.0f, -2.0f);
        // Fall through
    case AnimationDrink: // drinking (AbilityDrink; AnimationDrink): the item to the mouth while its cooldown runs
        if (m_creature->m_entityData.timeSinceAbility < m_creature->m_entityData.actionItem.cooldown()) {
            headRotation = plasma::Vector<3, float>(cosf(m_animationTime * 0.02f) * 5.0f + 45.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(1.0f, 7.0f, 8.0f);
            rightHandOffset = plasma::Vector<3, float>(-1.0f, 7.0f, 8.0f);
            leftHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
        }
        break;
    case AnimationOfferPetFood: // ability 82: holding out the pet food (AnimationOfferPetFood)
        bodyRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, 6.0f);
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, 6.0f);
        bodyOffset[1] = 3.0f;
        break;
    case AnimationSneak: // sneaking (AbilitySneak; AnimationSneak): crouched while moving
        if (m_creature->m_entityData.acceleration.squaredLength() > 0.0f) {
            creatureRotation[0] = 20.0f;
            bodyRotation[0] = -10.0f;
            headRotation[0] = -20.0f;
            leftHandRotation[0] = -20.0f;
            leftHandOffset[1] += 4.0f;
            rightHandRotation[0] = -20.0f;
            rightHandOffset[1] += 4.0f;
            feetTurn = -40.0f;
            leftFootPosition[2] += 3.0f;
            rightFootPosition[2] += 3.0f;
            rightFootPosition[1] += 4.0f;
            leftFootPosition[1] += 4.0f;
        }
        break;
    case AnimationDiveBite: // ability 51: a forward roll (AnimationDiveBite)
        if (m_animationTime < m_creature->anticipationTime())
            creatureRotation[0] = -30.0f;
        else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime())
            creatureRotation[0] = 120.0f;
        break;
    case AnimationHeadbutt: // AbilityHeadbutt (AnimationHeadbutt)
        if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2) {
            headRotation = plasma::Vector<3, float>(-30.0f, 10.0f, 90.0f);
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, 60.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, -2.0f, 0.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            headRotation = plasma::Vector<3, float>(30.0f, -10.0f, -30.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, -20.0f);
            headOffset = plasma::Vector<3, float>(3.0f, 0.0f, 2.0f);
        }
        break;
    case AnimationHornCharge: // ability 69 (AnimationHornCharge)
        if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2) {
            headRotation = plasma::Vector<3, float>(-70.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -3.0f, -1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            headRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(20.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -3.0f, 1.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
            rightHandRotation[0] = -20.0f;
            leftHandRotation[0] = -20.0f;
            tailRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
        }
        break;
    case AnimationStomp: // ability 74 (AnimationStomp)
        if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2) {
            headRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -6.0f, -2.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 30.0f;
            tailRotation = plasma::Vector<3, float>(-50.0f, 0.0f, 0.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            headRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
            leftHandRotation[0] = 0.0f;
            rightHandRotation[0] = 0.0f;
            blendRate = 0.02f;
        }
        break;
    case AnimationClawStrike: // ability 75 (AnimationClawStrike)
    case AnimationEatPetFood: // AbilityPetFood (AnimationEatPetFood)
        if (m_animationTime < m_creature->anticipationTime()) {
            headRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -1.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
            leftHandOffset[2] = 2.0f;
            rightHandOffset[2] = 2.0f;
            tailRotation = plasma::Vector<3, float>(30.0f, 0.0f, 10.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 3) {
            headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -4.0f, 2.0f);
            leftHandRotation[0] = 60.0f;
            rightHandRotation[0] = 70.0f;
            tailRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() * 2 / 3) {
            headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(40.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
            leftHandRotation[0] = 70.0f;
            rightHandRotation[0] = -30.0f;
            rightHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, -2.0f);
            tailRotation = plasma::Vector<3, float>(-30.0f, 0.0f, -10.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
            leftHandRotation[0] = -30.0f;
            rightHandRotation[0] = -40.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, -2.0f);
            tailRotation = plasma::Vector<3, float>(-30.0f, 0.0f, -10.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            headRotation = plasma::Vector<3, float>(10.0f, 0.0f, -10.0f);
            bodyRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, 1.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
            leftHandOffset[2] = 2.0f;
            rightHandOffset[2] = 2.0f;
            tailRotation = plasma::Vector<3, float>(30.0f, 0.0f, 10.0f);
        }
        break;
    case AnimationScream: // ability 73 (AnimationScream)
        if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2) {
            headRotation = plasma::Vector<3, float>(-10.0f, 10.0f, 30.0f);
            bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, 0.0f, -1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
            blendRate = 0.005f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            headRotation = plasma::Vector<3, float>(50.0f, 30.0f, 0.0f);
            if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime())
                headRotation[1] = -30.0f;
            bodyRotation = plasma::Vector<3, float>(30.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -1.0f, 2.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
            rightHandRotation[0] = -60.0f;
            blendRate = 0.005f;
            leftHandRotation[0] = -20.0f;
            tailRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
        }
        break;
    case AnimationDashRight: // ability 78: a dash to the right with a spin (AnimationDashRight)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 10.0f;
            bodyRotation[1] = -30.0f;
            leftHandRotation[1] = 30.0f;
            rightHandRotation[1] = 30.0f;
            headRotation[2] = -60.0f;
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            leftHandRotation[1] = -80.0f;
            rightHandRotation[1] = -80.0f;
            creatureRotation[1] =
                (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime() * 360.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[1] = 30.0f;
            leftHandRotation[1] = -30.0f;
            rightHandRotation[1] = -30.0f;
        }
        break;
    case AnimationDashLeft: // ability 77: a dash to the left (AnimationDashLeft)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -10.0f;
            bodyRotation[1] = 30.0f;
            leftHandRotation[1] = -30.0f;
            rightHandRotation[1] = -30.0f;
            headRotation[2] = 60.0f;
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            leftHandRotation[1] = 80.0f;
            rightHandRotation[1] = 80.0f;
            creatureRotation[1] =
                (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime() * -360.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[1] = -30.0f;
            leftHandRotation[1] = 30.0f;
            rightHandRotation[1] = 30.0f;
        }
        break;
    case AnimationDashForward: // ability 76: a forward somersault (AnimationDashForward)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[0] = -10.0f;
            headRotation[0] = 10.0f;
            tailRotation[0] = -60.0f;
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            leftHandRotation[0] = 80.0f;
            rightHandRotation[0] = 80.0f;
            headRotation[0] = -60.0f;
            creatureRotation[0] =
                (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime() * -360.0f;
            headOffset = plasma::Vector<3, float>(0.0f, -3.0f, 0.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = -10.0f;
            headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 0.0f);
        }
        break;
    case AnimationTailStrike: // ability 70: a tail sweep, turning round (AnimationTailStrike)
        if (m_animationTime < m_creature->anticipationTime()) {
            creatureRotation[2] = 90.0f;
            blendRate = 0.002f;
            headRotation = plasma::Vector<3, float>(0.0f, 10.0f, -60.0f);
            tailRotation = plasma::Vector<3, float>(0.0f, 0.0f, -70.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            creatureRotation[2] = 200.0f;
            headRotation = plasma::Vector<3, float>(0.0f, -20.0f, 60.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, 0.0f, -2.0f);
            tailRotation = plasma::Vector<3, float>(0.0f, 0.0f, 70.0f);
        }
        break;
    case AnimationShell: // abilities 71, 72: withdrawing into the shell (AnimationShell) The parts move into the chest over
        // the anticipation, vanish during the strike, and come out again over the hold.
        if (m_animationTime < m_creature->anticipationTime()) {
            float shellRetractionFraction = (float)m_animationTime / (float)m_creature->anticipationTime();
            headPosition[2] += (chestPosition[2] - 2.0f - headPosition[2]) * shellRetractionFraction;
            headPosition[0] += (chestPosition[0] - 0.0f - headPosition[0]) * shellRetractionFraction;
            headPosition[1] += (chestPosition[1] - 0.0f - headPosition[1]) * shellRetractionFraction;
            leftHandPosition[0] += (chestPosition[0] - leftHandPosition[0]) * shellRetractionFraction;
            leftHandPosition[2] += (chestPosition[2] - leftHandPosition[2]) * shellRetractionFraction;
            leftHandPosition[1] += (chestPosition[1] - leftHandPosition[1]) * shellRetractionFraction;
            rightHandPosition[0] += (chestPosition[0] - rightHandPosition[0]) * shellRetractionFraction;
            rightHandPosition[2] += (chestPosition[2] - rightHandPosition[2]) * shellRetractionFraction;
            rightHandPosition[1] += (chestPosition[1] - rightHandPosition[1]) * shellRetractionFraction;
            leftFootPosition[0] += (chestPosition[0] - leftFootPosition[0]) * shellRetractionFraction;
            leftFootPosition[2] += (chestPosition[2] - leftFootPosition[2]) * shellRetractionFraction;
            leftFootPosition[1] += (chestPosition[1] - leftFootPosition[1]) * shellRetractionFraction;
            rightFootPosition[1] += shellRetractionFraction * (chestPosition[1] - rightFootPosition[1]);
            rightFootPosition[2] += shellRetractionFraction * (chestPosition[2] - rightFootPosition[2]);
            rightFootPosition[0] += (chestPosition[0] - rightFootPosition[0]) * shellRetractionFraction;
            tailPosition[0] += shellRetractionFraction * (chestPosition[0] + 0.0f - tailPosition[0]);
            tailPosition[1] += shellRetractionFraction * (chestPosition[1] + 4.0f - tailPosition[1]);
            tailPosition[2] += shellRetractionFraction * (chestPosition[2] + 4.0f - tailPosition[2]);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            m_headSprite = 0;
            m_handsSprite = 0;
            m_feetSprite = 0;
            m_tailSprite = 0;
            bodyOffset = plasma::Vector<3, float>(0.0f, 0.0f, -3.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            float shellRetractionFraction =
                1.0f - (float)(m_animationTime - m_creature->anticipationTime() - m_creature->strikeTime()) /
                           (float)m_creature->holdTime();
            headPosition[0] += shellRetractionFraction * (chestPosition[0] - 0.0f - headPosition[0]);
            headPosition[2] += shellRetractionFraction * (chestPosition[2] - 2.0f - headPosition[2]);
            headPosition[1] += shellRetractionFraction * (chestPosition[1] - 0.0f - headPosition[1]);
            leftHandPosition[0] += shellRetractionFraction * (chestPosition[0] - leftHandPosition[0]);
            leftHandPosition[1] += shellRetractionFraction * (chestPosition[1] - leftHandPosition[1]);
            leftHandPosition[2] += shellRetractionFraction * (chestPosition[2] - leftHandPosition[2]);
            rightHandPosition[0] += shellRetractionFraction * (chestPosition[0] - rightHandPosition[0]);
            rightHandPosition[1] += shellRetractionFraction * (chestPosition[1] - rightHandPosition[1]);
            rightHandPosition[2] += shellRetractionFraction * (chestPosition[2] - rightHandPosition[2]);
            leftFootPosition[1] += (chestPosition[1] - leftFootPosition[1]) * shellRetractionFraction;
            leftFootPosition[2] += (chestPosition[2] - leftFootPosition[2]) * shellRetractionFraction;
            leftFootPosition[0] += (chestPosition[0] - leftFootPosition[0]) * shellRetractionFraction;
            rightFootPosition[1] += (chestPosition[1] - rightFootPosition[1]) * shellRetractionFraction;
            rightFootPosition[2] += (chestPosition[2] - rightFootPosition[2]) * shellRetractionFraction;
            rightFootPosition[0] += (chestPosition[0] - rightFootPosition[0]) * shellRetractionFraction;
            tailPosition[0] += (chestPosition[0] + 0.0f - tailPosition[0]) * shellRetractionFraction;
            tailPosition[1] += (chestPosition[1] + 4.0f - tailPosition[1]) * shellRetractionFraction;
            tailPosition[2] += (chestPosition[2] + 4.0f - tailPosition[2]) * shellRetractionFraction;
        }
        break;
    case AnimationTwoHandedSwing: // ability 67 (AnimationTwoHandedSwing): a swing that ends in a two-handed rest
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 80.0f);
            bodyRotation[2] = -80.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -90.0f);
            bodyRotation[2] = 90.0f;
            leftFootTurnedOut = true;
            rightHandPosition[0] += 4.0f;
            leftFootPosition[1] -= 4.0f;
            rightFootPosition[1] += 4.0f;
            blendRate = 0.05f;
        } else {
            m_animation = AnimationNone;
            rightHandPosition[1] += 4.0f;
            rightHandPosition[0] -= 8.0f;
            leftHandPosition = rightHandPosition;
            rightHandRotation[0] = -60.0f;
            rightHandRotation[2] = 20.0f;
            bodyRotation[2] = -20.0f;
            leftHandPosition[0] -= 2.0f;
            leftHandPosition[2] -= 1.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
        }
        // Both hands on the weapon.
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] += 2.0f;
        leftShoulderPosition[1] += 10.0f;
        leftShoulderPosition[2] -= 4.0f;
        leftHandRotation = rightHandRotation;
        break;
    case AnimationCrush: // ability 59 (AnimationCrush): a two-handed overhead blow
        if (m_animationTime < m_creature->anticipationTime() / 2) {
            float raise = (float)m_animationTime / (float)(m_creature->anticipationTime() / 2);
            if (raise > 1.0f)
                raise = 1.0f;
            creatureRotation[0] = raise * 5.0f;
            bodyRotation[1] = raise * 10.0f;
            bodyRotation[2] = raise * -20.0f;
            rightHandRotation = (1.0f - raise) * plasma::Vector<3, float>(0.0f, 120.0f, 0.0f) +
                                raise * plasma::Vector<3, float>(60.0f, 60.0f, 60.0f);
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.005f;
            leftHandRotation = rightHandRotation;
        } else {
            float crushPoseBlendRate;
            if (m_creature->m_entityData.classType == ClassWarrior &&
                m_creature->m_entityData.specialization == SpecializationGuardian) {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 7.0f, 4.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, -60.0f, 0.0f);
                crushPoseBlendRate = 0.01f;
            } else {
                float fall = (float)(m_animationTime - m_creature->anticipationTime() / 2) /
                             (float)(m_creature->anticipationTime() / 2) * 1.5f;
                if (fall > 1.0f)
                    fall = 1.0f;
                creatureRotation[0] = 0.0f;
                bodyRotation[0] = 20.0f;
                bodyRotation[1] = 0.0f;
                bodyRotation[2] = 0.0f;
                rightHandRotation = (1.0f - fall) * plasma::Vector<3, float>(90.0f, 0.0f, 90.0f) +
                                    fall * plasma::Vector<3, float>(30.0f, 0.0f, 0.0f);
                rightHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + rightHandPosition[2];
                rightHandOffset = plasma::Vector<3, float>(-6.0f, 0.0f, fall * 16.0f);
                crushPoseBlendRate = 0.005f;
            }
            blendRate = crushPoseBlendRate;
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    case AnimationTwoHandedSweep: // abilities 57, 60 (AnimationTwoHandedSweep): a two-handed sweep
        if (m_animationTime < m_creature->anticipationTime()) {
            float raise = (float)m_animationTime / (float)m_creature->anticipationTime() * 1.5f;
            if (raise > 1.0f)
                raise = 1.0f;
            creatureRotation[0] = raise * 10.0f + 5.0f;
            bodyRotation[0] = 10.0f;
            bodyRotation[1] = 10.0f;
            bodyRotation[2] = -60.0f;
            rightHandRotation = (1.0f - raise) * plasma::Vector<3, float>(90.0f, 0.0f, 90.0f) +
                                raise * plasma::Vector<3, float>(60.0f, 0.0f, 60.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, raise * 14.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.01f;
            leftHandRotation = rightHandRotation;
        } else if (m_animationTime < m_creature->attackDuration()) {
            float sweep = (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime();
            if (sweep > 1.0f)
                sweep = 1.0f;
            creatureRotation[0] = -10.0f;
            bodyRotation[1] = 10.0f;
            bodyRotation[2] = 60.0f;
            blendRate = 0.04f;
            rightHandRotation = plasma::Vector<3, float>(-70.0f, 0.0f, -60.0f);
            rightHandOffset =
                plasma::Vector<3, float>(0.0f, sinf((float)(sweep * 3.141592653589793 * 0.5)) * 2.0f, 0.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftHandPosition[1] -= 1.0f;
            leftHandRotation = rightHandRotation;
        } else {
            m_animation = AnimationNone;
            rightHandPosition[1] += 2.0f;
            rightHandPosition[2] += 3.0f;
            leftHandPosition = rightHandPosition;
            rightHandRotation[0] = 0.0f;
            rightHandRotation[1] = 20.0f;
            leftHandPosition[0] -= 3.0f;
            leftHandPosition[2] -= 3.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    case AnimationTwoHandedSpin: // abilities 61, 66 (AnimationTwoHandedSpin): a two-handed spin, the hands turned by the aim
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        if (m_animationTime < m_creature->anticipationTime() / 2) {
            float raise = (float)m_animationTime / (float)(m_creature->anticipationTime() / 2);
            if (raise > 1.0f)
                raise = 1.0f;
            bodyRotation[1] = raise * 10.0f;
            bodyRotation[2] = raise * -80.0f;
            rightHandRotation = (1.0f - raise) * plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f) +
                                raise * plasma::Vector<3, float>(-90.0f, 90.0f, -30.0f);
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.01f;
        } else if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, -90.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 110.0f, -20.0f);
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.01f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            creatureRotation[2] = 20.0f;
            bodyRotation[0] = 0.0f;
            bodyRotation[1] = 0.0f;
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 90.0f);
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.01f;
        } else {
            m_animation = AnimationNone;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftHandPosition[2] -= 1.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            rightHandRotation[1] = 90.0f;
            bodyRotation[2] = -80.0f;
            leftHandRotation = rightHandRotation;
        }
        {
            // One full turn over the strike.
            float turn = (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime();
            if (turn > 1.0f)
                turn = 1.0f;
            else if (0.0f > turn)
                turn = 0.0f;
            creatureRotation[2] = turn * 360.0f;
        }
        leftHandRotation = rightHandRotation;
        break;
    case AnimationGuardianCrushStrike: // ability 20, AbilityRangerKick (AnimationGuardianCrushStrike): a two-handed turn the other way
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        if (m_animationTime < m_creature->anticipationTime()) {
            creatureRotation[2] = 30.0f;
            bodyRotation[0] = 0.0f;
            bodyRotation[1] = 0.0f;
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 90.0f);
            // Here the right hand joins the left, the other way from the strike below.
            rightHandPosition = leftHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.005f;
            leftHandRotation = rightHandRotation;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[0] = 0.0f;
            bodyRotation[1] = 0.0f;
            bodyRotation[2] = 0.0f;
            m_creature->anticipationTime(); // called and discarded in the code
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 0.0f);
            if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime())
                creatureOffset[0] = 0.25f;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            float turn = (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime();
            if (turn > 1.0f)
                turn = 1.0f;
            creatureRotation[2] = turn * -360.0f;
            blendRate = 0.03f;
            leftHandRotation = rightHandRotation;
        } else {
            m_animation = AnimationNone;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftHandPosition[2] -= 1.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 0.0f);
            bodyRotation[2] = 0.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    case AnimationTwoHandedThrust: // ability 58 (AnimationTwoHandedThrust): a two-handed thrust
        if (m_animationTime < m_creature->anticipationTime()) {
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-5.0f, 8.0f, 0.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.005f;
            leftHandRotation = rightHandRotation;
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2) {
            creatureRotation = plasma::Vector<3, float>(10.0f, 0.0f, -10.0f);
            bodyRotation = plasma::Vector<3, float>(30.0f, 0.0f, -30.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandOffset = plasma::Vector<3, float>(-8.0f, -4.0f, 20.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.005f;
            leftHandRotation = rightHandRotation;
        } else if (m_animationTime < m_creature->attackDuration()) {
            creatureRotation[2] = -30.0f;
            bodyRotation = plasma::Vector<3, float>(0.0f, 20.0f, -60.0f);
            rightHandRotation = plasma::Vector<3, float>(70.0f, 0.0f, 90.0f);
            rightHandOffset = plasma::Vector<3, float>(5.0f, -2.0f, 0.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            blendRate = 0.02f;
            leftHandRotation = rightHandRotation;
        } else {
            m_animation = AnimationNone;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftHandPosition[2] -= 1.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            rightHandRotation[1] = 90.0f;
            bodyRotation[2] = -80.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    case AnimationSmash: // abilities 47, AbilitySmash (AnimationSmash): a leap and a blow from above
        if (500.0f > (float)m_animationTime) {
            float leap = (float)m_animationTime / 500.0f;
            if (leap > 1.0f)
                leap = 1.0f;
            if (m_creature->hasTwoHandedWeapon()) {
                rightHandRotation[0] = leap * 120.0f;
                rightHandOffset[1] = 0.0f;
                rightHandOffset[2] = 14.0f;
                rightHandOffset[0] = -rightHandPosition[0];
                leftHandOffset[1] = 0.0f;
                leftHandOffset[2] = 12.0f;
                leftHandOffset[0] = 2.0f - leftHandOffset[0];
            } else {
                rightHandOffset = plasma::Vector<3, float>(1.0f, 0.0f, 14.0f);
                leftHandOffset = plasma::Vector<3, float>(-1.0f, 0.0f, 14.0f);
                leftHandRotation[0] = -100.0f;
                rightHandRotation[0] = -100.0f;
            }
            float lean = leap * -40.0f;
            bodyRotation[0] = lean;
            feetTurn = leap * -50.0f;
            creatureRotation[0] = lean;
            // The intercept somersaults over the leap.
            if (m_creature->m_entityData.currentAbility == AbilityIntercept) {
                float somersault = (float)m_animationTime / 500.0f * 360.0f;
                if (somersault > 360.0f)
                    somersault = 360.0f;
                creatureRotation[0] = lean - somersault;
            }
            m_creature->m_walkAmplitude = 0.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            float land = (float)(m_animationTime - 500) / 500.0f * 2.0f;
            if (land > 1.0f)
                land = 1.0f;
            creatureRotation[0] = (1.0f - land) * -60.0f;
            bodyRotation[0] = -20.0f;
            rightHandRotation[0] = -70.0f;
            if (m_creature->hasTwoHandedWeapon()) {
                rightHandOffset[1] = 4.0f;
                rightHandOffset[2] = 0.0f;
                rightHandOffset[0] = -rightHandPosition[0];
                leftHandOffset[1] = 4.0f;
                leftHandOffset[2] = 0.0f;
                leftHandOffset[0] = 2.0f - leftHandOffset[0];
                blendRate = 0.1f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(1.0f, 3.0f, 8.0f);
                leftHandOffset = plasma::Vector<3, float>(-1.0f, 3.0f, 8.0f);
                leftHandRotation[0] = -150.0f;
                rightHandRotation[0] = -150.0f;
                blendRate = 0.1f;
            }
        } else if (m_creature->hasTwoHandedWeapon()) {
            rightHandPosition[1] += 4.0f;
            rightHandPosition[0] -= 8.0f;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 2.0f;
            leftHandPosition[2] -= 1.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
            rightHandRotation[0] = -60.0f;
            rightHandRotation[2] = 20.0f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(1.0f, 3.0f, 2.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, 3.0f, 2.0f);
            leftHandRotation[0] = -250.0f;
            rightHandRotation[0] = -250.0f;
        }
        // A shield in the left hand stays turned to the front.
        if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
            m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield)
            leftHandRotation = plasma::Vector<3, float>(-250.0f, -90.0f, 0.0f);
        break;
    case AnimationTwoHandedRest: // the two-handed rest (AnimationTwoHandedRest): the hands turned from the initial to the final aim
        if (m_animationTime < m_creature->attackDuration() / 2) {
            handsTurn = m_creature->m_entityData.initialHandsRotation;
            blendRate = 0.005f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            float restAimTransitionProgress =
                (float)(m_animationTime - m_creature->attackDuration() / 2) / (float)(m_creature->attackDuration() / 2);
            float turn = restAimTransitionProgress * restAimTransitionProgress * restAimTransitionProgress * 15.0f;
            if (turn > 1.0f)
                turn = 1.0f;
            handsTurn = (1.0f - turn) * m_creature->m_entityData.initialHandsRotation +
                        turn * m_creature->m_entityData.finalHandsRotation;
            blendRate = 0.03f;
        } else {
            handsTurn = m_creature->m_entityData.finalHandsRotation;
        }
        rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
        if (m_creature->hasTwoHandedWeapon()) {
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[1] += 10.0f;
            leftShoulderPosition[2] -= 4.0f;
        }
        break;
    case AnimationShoot: // abilities 24, 25, AbilityRetreat, 55 and the intercept with a bow (AnimationShoot) The bow in
        // the left hand, an arrow (sprite 840) in the right; a bow or crossbow in the right hand mirrors
        // the pose.
        if (m_creature->m_entityData.equipment.weaponRight.id != WeaponBow &&
            m_creature->m_entityData.equipment.weaponRight.id != WeaponCrossbow) {
            leftHandOffset[1] += 3.0f;
            leftHandOffset[0] -= 4.0f;
            leftHandOffset[2] += 2.0f;
            rightHandOffset[1] += 2.0f;
            rightHandOffset[0] -= 10.0f;
            rightHandOffset[2] += 1.0f;
            bodyRotation[2] = -60.0f;
            leftHandRotation[2] = 60.0f;
            leftHandRotation[1] = -90.0f;
            rightHandRotation[2] = 60.0f;
            if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon)
                m_rightWeaponSprite = spriteManager->sprite(SpriteArrow);
            // The recoil of the first 300 ms.
            if (m_animationTime < 300) {
                leftHandOffset[2] -= 1.0f;
                leftHandRotation[0] += 20.0f;
                leftHandRotation[1] -= 20.0f;
            }
        } else {
            rightHandOffset[1] += 3.0f;
            rightHandOffset[0] += 4.0f;
            rightHandOffset[2] += 2.0f;
            leftHandOffset[1] += 2.0f;
            leftHandOffset[0] += 10.0f;
            leftHandOffset[2] += 1.0f;
            bodyRotation[2] = 60.0f;
            rightHandRotation[2] = -60.0f;
            rightHandRotation[1] = -90.0f;
            leftHandRotation[2] = -60.0f;
            if (m_creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon)
                m_leftWeaponSprite = spriteManager->sprite(SpriteArrow);
            if (m_animationTime < 300) {
                rightHandOffset[2] -= 1.0f;
                rightHandRotation[0] += 20.0f;
                rightHandRotation[1] += 20.0f;
            }
        }
        break;
    case AnimationThrowBoomerang: // ability 26 (AnimationThrowBoomerang): the weapon thrown, with a spin while powered
        if (m_creature->m_attackPower > 0.0f) {
            if (m_animationTime < m_creature->anticipationTime()) {
                bodyRotation[2] = -90.0f;
                rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
                m_leftWeaponSprite = m_rightWeaponSprite;
            } else if (m_animationTime < m_creature->anticipationTime() + 300) {
                bodyRotation[2] = 10.0f;
                leftHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 1.0f);
                leftHandRotation = plasma::Vector<3, float>(0.0f, -10.0f, 0.0f);
                m_leftWeaponSprite = m_rightWeaponSprite;
                m_rightWeaponSprite = 0;
            } else if (m_animationTime < m_creature->attackDuration()) {
                m_rightWeaponSprite = 0;
                bodyRotation[2] = 90.0f;
                m_leftWeaponSprite = 0;
            }
            blendRate = 0.03f;
            if (m_animationTime > m_creature->anticipationTime() && m_animationTime < m_creature->attackDuration()) {
                float spin = (float)(m_animationTime - m_creature->anticipationTime()) * 360.0f / 200.0f;
                creatureRotation[2] = spin;
                if (spin > 700.0f)
                    creatureRotation[2] = 700.0f;
            }
        } else if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -90.0f;
            rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = 90.0f;
            m_rightWeaponSprite = 0;
        }
        break;
    case AnimationChargedBoomerang: // ability 27 (AnimationChargedBoomerang): charging a throw
        bodyRotation[2] = m_creature->m_entityData.charge * -90.0f;
        rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
        rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
        rightHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + rightHandPosition[2];
        leftHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + leftHandPosition[2];
        if (m_creature->m_entityData.charge > 0.0f)
            m_leftWeaponSprite = m_rightWeaponSprite;
        break;
    case AnimationThrowRock: // ability 108 (AnimationThrowRock): a rock (sprite 2101) lifted over the head and thrown
        if (m_creature->m_entityData.timeSinceAbility < m_creature->anticipationTime()) {
            rightHandOffset = plasma::Vector<3, float>(-3.0f, -2.0f, 10.0f);
            leftHandOffset = plasma::Vector<3, float>(3.0f, -2.0f, 10.0f);
            m_leftWeaponSprite = spriteManager->sprite(SpriteIronDeposit);
            leftWeaponScale *= 1.5f;
            bodyRotation[0] = 40.0f;
            creatureRotation[0] = 10.0f;
        } else if (m_creature->m_entityData.timeSinceAbility <
                   m_creature->strikeTime() + m_creature->anticipationTime()) {
            blendRate = 0.01f;
            rightHandOffset = plasma::Vector<3, float>(0.0f, 5.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(0.0f, 5.0f, 0.0f);
            bodyRotation[0] = -20.0f;
            creatureRotation[0] = -40.0f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, -1.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-2.0f, -1.0f, 0.0f);
        }
        break;
    case AnimationDrawBow: // abilities 22, 23 (AnimationDrawBow): drawing a bow; an arrow (sprite 840) nocked
        if (m_creature->m_entityData.equipment.weaponRight.id != WeaponBow &&
            m_creature->m_entityData.equipment.weaponRight.id != WeaponCrossbow) {
            // The bow in the left hand.
            if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow)
                leftHandRotation[1] = -90.0f;
            if (m_animationTime < m_creature->attackDuration() * BowDrawPoseDurationMultiplier) {
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponBow) {
                    leftHandOffset[1] += 5.0f;
                    leftHandOffset[0] -= 7.0f;
                } else {
                    leftHandOffset[1] += 2.0f;
                    leftHandOffset[0] -= 3.0f;
                }
                leftHandOffset[2] += 4.0f;
                rightHandOffset[1] += 3.0f;
                rightHandOffset[0] -= 10.0f;
                rightHandOffset[2] += 3.0f;
                bodyRotation[2] -= 70.0f;
                leftHandRotation[2] = 70.0f;
                rightHandRotation[2] = 70.0f;
                // The first half: the draw, the arrow not yet in hand.
                if (m_animationTime < m_creature->attackDuration() / 2) {
                    leftHandOffset[2] -= 2.0f;
                    leftHandRotation[0] = -5.0f;
                    rightHandOffset = plasma::Vector<3, float>(-5.0f, 1.0f, 0.0f);
                    m_rightWeaponSprite = 0;
                    bodyRotation[2] = -50.0f;
                    leftHandRotation[2] = 70.0f;
                    break;
                }
            } else {
                leftHandOffset[0] -= 1.0f;
                leftHandRotation[0] = -30.0f;
                rightHandRotation[0] = -30.0f;
                rightHandRotation[1] = 45.0f;
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow)
                    leftHandRotation[2] = 50.0f;
            }
            if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon)
                m_rightWeaponSprite = spriteManager->sprite(SpriteArrow);
        } else {
            // The bow in the right hand: mirrored.
            if (m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow)
                rightHandRotation[1] = -90.0f;
            if (m_animationTime < m_creature->attackDuration() * 2) {
                if (m_creature->m_entityData.equipment.weaponRight.id == WeaponBow) {
                    rightHandOffset[1] += 5.0f;
                    rightHandOffset[0] += 7.0f;
                } else {
                    rightHandOffset[1] += 2.0f;
                    rightHandOffset[0] += 3.0f;
                }
                rightHandOffset[2] += 4.0f;
                leftHandOffset[1] += 3.0f;
                leftHandOffset[0] += 10.0f;
                leftHandOffset[2] += 3.0f;
                bodyRotation[2] = 70.0f;
                rightHandRotation[2] = -70.0f;
                leftHandRotation[2] = -70.0f;
                if (m_animationTime < m_creature->attackDuration() / 2) {
                    rightHandOffset[2] -= 2.0f;
                    rightHandRotation[0] = -5.0f;
                    leftHandOffset = plasma::Vector<3, float>(5.0f, 1.0f, 0.0f);
                    m_leftWeaponSprite = 0;
                    bodyRotation[2] = 50.0f;
                    rightHandRotation[2] = -70.0f;
                    break;
                }
            } else {
                rightHandOffset[0] += 1.0f;
                rightHandRotation[0] = -30.0f;
                leftHandRotation[0] = -30.0f;
                leftHandRotation[1] = -45.0f;
            }
            if (m_creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon)
                m_leftWeaponSprite = spriteManager->sprite(SpriteArrow);
        }
        break;
    case AnimationSwirlStrike: // ability 11 (AnimationSwirlStrike): a spin with the weapon held out
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -80.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -90.0f);
            leftHandRotation = plasma::Vector<3, float>(20.0f, 90.0f, 90.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            bodyRotation[2] = -90.0f;
            creatureRotation[2] =
                (float)(m_animationTime - m_creature->anticipationTime()) * 360.0f / (float)m_creature->strikeTime();
            leftHandRotation = plasma::Vector<3, float>(0.0f, 100.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            blendRate = 0.2f;
        } else {
            bodyRotation[2] = 80.0f;
            creatureRotation[2] = 0.0f;
            leftHandRotation = plasma::Vector<3, float>(90.0f, 90.0f, 270.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 90.0f);
        }
        break;
    case AnimationCyclone: // AbilityCyclone (AnimationCyclone): spinning for five seconds
        if (m_animationTime < CycloneWindupMs) {
            bodyRotation[2] = -80.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -90.0f);
            leftHandRotation = plasma::Vector<3, float>(20.0f, 90.0f, 90.0f);
        } else if (m_animationTime < 5000) {
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = (float)(m_animationTime - CycloneWindupMs) * 360.0f / 300.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            blendRate = 0.2f;
        } else {
            bodyRotation[2] = 80.0f;
            creatureRotation[2] = 0.0f;
            leftHandRotation = plasma::Vector<3, float>(90.0f, 90.0f, 270.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 90.0f);
        }
        break;
    case AnimationQuake: // ability 91 (AnimationQuake): the hands beat the ground in turn
        if (m_animationTime <
            m_creature->anticipationTime() + (m_creature->strikeTime() + QuakePoseStrikeExtensionMs)) {
            if (m_animationTime <= m_creature->anticipationTime() ||
                (m_animationTime - m_creature->anticipationTime() + 100) / 500 % 2 == 1) {
                leftHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 12.0f);
                rightHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 12.0f);
                bodyRotation[0] = 20.0f;
                creatureRotation[0] = 10.0f;
            } else if ((m_animationTime - m_creature->anticipationTime() + 100) / 1000 % 2 == 1) {
                leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
                rightHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 12.0f);
                blendRate = 0.03f;
                bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, -20.0f);
                creatureRotation = plasma::Vector<3, float>(-5.0f, 0.0f, -10.0f);
            } else {
                leftHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 12.0f);
                rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
                blendRate = 0.03f;
                bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 20.0f);
                creatureRotation = plasma::Vector<3, float>(-5.0f, 0.0f, 10.0f);
            }
        }
        break;
    case AnimationBlockStrike: // ability 104 (AnimationBlockStrike): a turn with the weapon held across
    {
        int animationTime = m_animationTime;
        if (animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            blendRate = 0.03f;
            creatureRotation[2] =
                (float)animationTime * -360.0f / (float)(m_creature->anticipationTime() + m_creature->strikeTime());
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(4.0f, 3.0f, 0.0f);
            bodyRotation[2] =
                (float)m_animationTime * -60.0f / (float)(m_creature->anticipationTime() + m_creature->strikeTime());
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
        } else {
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-60.0f, -60.0f, -60.0f);
        }
    } break;
    case AnimationCrossedWeaponSweep: // ability 12 (AnimationCrossedWeaponSweep): both weapons crossed, then swept apart
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandRotation = plasma::Vector<3, float>(-80.0f, -90.0f, 60.0f);
            leftHandRotation = plasma::Vector<3, float>(-80.0f, 90.0f, -60.0f);
            rightHandOffset[1] += 3.0f;
            rightHandOffset[0] -= 5.0f;
            leftHandOffset[0] += 5.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, -30.0f);
            rightHandOffset[1] -= 5.0f;
            leftHandOffset[1] -= 5.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 110.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -110.0f);
            blendRate = 0.04f;
            creatureOffset[1] = -0.5f;
        } else {
            // Over: the weapons lowered; a single right-hand weapon is held with both hands.
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            leftHandRotation[0] = -90.0f;
            bodyRotation[2] = 20.0f;
            rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
            leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
            leftHandOffset[1] -= 4.0f;
            leftHandOffset[2] += 2.0f;
            if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                rightHandPosition[1] += 2.0f;
                rightHandPosition[2] += 3.0f;
                leftHandPosition = rightHandPosition;
                leftHandPosition[0] -= 3.0f;
                leftHandPosition[2] -= 3.0f;
                leftShoulderPosition[1] += 10.0f;
                leftShoulderPosition[2] -= 4.0f;
                rightHandRotation[0] = 0.0f;
                rightHandRotation[1] = 20.0f;
                leftHandRotation = rightHandRotation;
            }
        }
        break;
    case AnimationAmbush: // abilities 16, AbilityAmbush and the intercept with a left weapon (AnimationAmbush)
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 40.0f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -7.0f, 0.0f);
            bodyRotation[2] = -40.0f;
            creatureOffset[1] = -0.5f;
            blendRate = 0.005f;
            leftFootTurnedOut = true;
        } else if (m_animationTime < m_creature->attackDuration()) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -60.0f, -30.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 60.0f, -50.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 5.0f, 2.0f);
            leftHandOffset = plasma::Vector<3, float>(6.0f, 8.0f, 2.0f);
            bodyRotation[2] = 40.0f;
            blendRate = 0.05f;
            leftFootTurnedOut = true;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            // Races 96 and 87 do not lower their weapons.
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -60.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    case AnimationIntercept: // the intercept without a left weapon (AnimationIntercept)
        rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 80.0f);
        bodyRotation[2] = -80.0f;
        break;
    case AnimationMonsterCharge: // ability 64 (AnimationMonsterCharge): both hands forward, bobbing
        rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -40.0f);
        leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 40.0f);
        rightHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + rightHandPosition[2];
        leftHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + leftHandPosition[2];
        rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
        leftHandOffset = plasma::Vector<3, float>(-1.0f, -7.0f, 0.0f);
        bodyRotation[2] = -40.0f;
        blendRate = 0.005f;
        break;
    case AnimationSwirl: // ability 63 (AnimationSwirl): as 5, with a guard for specialization 1
        if (m_creature->m_entityData.specialization == SpecializationGuardian) {
            rightHandRotation = plasma::Vector<3, float>(-50.0f, -60.0f, 40.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -50.0f);
            rightHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + rightHandPosition[2];
            leftHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + leftHandPosition[2];
            rightHandOffset = plasma::Vector<3, float>(-3.0f, 7.0f, 3.0f);
            leftHandOffset = plasma::Vector<3, float>(-2.0f, 2.0f, 0.0f);
            bodyRotation[2] = -20.0f;
            blendRate = 0.005f;
        } else {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -40.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -40.0f);
            rightHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + rightHandPosition[2];
            leftHandPosition[2] = cosf(m_animationTime * 0.5f) * 0.5f + leftHandPosition[2];
            rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
            // Not pulled back as the right hand is: (-1, 0, 1).
            leftHandOffset = plasma::Vector<3, float>(-1.0f, 0.0f, 1.0f);
            bodyRotation[2] = -40.0f;
            blendRate = 0.005f;
        }
        break;
    case AnimationParry: // ability 98 (AnimationParry): the left weapon (or the right) swung across
        if (m_leftWeaponSprite) {
            if (m_animationTime < ParryWindupMs) {
                bodyRotation[2] = 80.0f;
                leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
                rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            } else if (m_animationTime < ParryFollowThroughStartMs) {
                bodyRotation[2] = -90.0f;
                leftHandRotation = plasma::Vector<3, float>(0.0f, 60.0f, 90.0f);
                leftHandOffset[2] += 5.0f;
                rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 45.0f);
            } else {
                bodyRotation[2] = -30.0f;
                leftHandRotation = plasma::Vector<3, float>(0.0f, 100.0f, 60.0f);
                rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            }
        } else {
            if (m_animationTime < ParryWindupMs) {
                bodyRotation[2] = -80.0f;
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            } else if (m_animationTime < ParryFollowThroughStartMs) {
                bodyRotation[2] = 90.0f;
                rightHandRotation = plasma::Vector<3, float>(0.0f, 60.0f, -90.0f);
                rightHandRotation[2] += 5.0f;
                leftHandOffset[0] += 2.0f;
                rightHandOffset[0] += 2.0f;
                leftHandOffset[1] += 3.0f;
                rightHandOffset[1] += 3.0f;
                leftHandOffset[2] += 3.0f;
                rightHandOffset[2] += 3.0f;
            } else {
                rightHandPosition[1] += 2.0f;
                rightHandPosition[2] += 3.0f;
                leftHandPosition = rightHandPosition;
                leftHandPosition[0] -= 3.0f;
                leftHandPosition[2] -= 3.0f;
                leftShoulderPosition[1] += 10.0f;
                leftShoulderPosition[2] -= 4.0f;
                rightHandRotation[0] = 0.0f;
                rightHandRotation[1] = 20.0f;
            }
            // Both hands on the weapon.
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] -= 3.0f;
            leftHandPosition[2] -= 3.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    case AnimationThrow: // AbilityShuriken (AnimationThrow): a swing to and fro
        if (m_creature->m_entityData.timeSinceAbility < m_creature->attackDuration()) {
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, sinf(m_animationTime * 0.03f) * 90.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            blendRate = 0.04f;
        }
        break;
    case AnimationFireballCharge: // ability 36 (AnimationFireballCharge): the hands held out, stirring
        bodyRotation[2] = -30.0f;
        rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
        leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        rightHandOffset[1] += 4.0f;
        leftHandOffset[1] += 4.0f;
        rightHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + rightHandOffset[0];
        rightHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + rightHandOffset[2];
        leftHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + leftHandOffset[0];
        leftHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + leftHandOffset[2];
        break;
    case AnimationFireball: // abilities 37, 43 (AnimationFireball)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            rightHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + rightHandOffset[0];
            rightHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + rightHandOffset[2];
            leftHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + leftHandOffset[0];
            leftHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + leftHandOffset[2];
        } else {
            bodyRotation[2] = 60.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        }
        break;
    case AnimationFirebolt: // abilities 38, 44, 45, 46, 94, 95 (AnimationFirebolt)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            rightHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + rightHandOffset[0];
            rightHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + rightHandOffset[2];
            leftHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + leftHandOffset[0];
            leftHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + leftHandOffset[2];
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -90.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 0.0f, 1.0f);
            blendRate = 0.03f;
        } else {
            rightHandRotation = plasma::Vector<3, float>(-60.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 0.0f, 1.0f);
        }
        break;
    case AnimationRightCast: // abilities 40, 42 (AnimationRightCast): cast to the right
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            rightHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + rightHandOffset[0];
            rightHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + rightHandOffset[2];
            leftHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + leftHandOffset[0];
            leftHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + leftHandOffset[2];
            bodyRotation[2] = -20.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            creatureOffset[0] = -0.5f;
            bodyRotation[2] = 60.0f;
            creatureRotation[2] = 30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            leftFootTurnedOut = true;
        }
        break;
    case AnimationLeftCast: // abilities 39, 41 (AnimationLeftCast): cast to the left
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            rightHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + rightHandOffset[0];
            rightHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + rightHandOffset[2];
            leftHandOffset[0] = sinf(m_animationTime * 0.04f) * 0.5f + leftHandOffset[0];
            leftHandOffset[2] = cosf(m_animationTime * 0.05f) * 0.5f + leftHandOffset[2];
            bodyRotation[2] = 20.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            creatureOffset[0] = 0.5f;
            bodyRotation[2] = -60.0f;
            creatureRotation[2] = -30.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 60.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-2.0f, 0.0f, 0.0f);
            leftFootTurnedOut = true;
        }
        break;
    case AnimationSupportRay: // AbilitySupportRay (AnimationSupportRay); not for AppearanceFlagHeadAimOrigin
        if (!(m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagHeadAimOrigin))) {
            bodyRotation[2] = 60.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        }
        break;
    case AnimationCastSpell: // abilities 30..33, 34, 49, 87..90, 92, 105 (AnimationCastSpell): the hands circle, then thrust
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = 20.0f;
            float angle = m_animationTime * 0.005f;
            leftHandOffset = plasma::Vector<3, float>(cosf(angle) * 3.0f - 1.0f, 5.0f, sinf(angle) * 5.0f + 5.0f);
            angle = (float)(m_animationTime * 0.005f + 3.141592653589793);
            rightHandOffset = plasma::Vector<3, float>(cosf(angle) * 3.0f + 1.0f, 5.0f, sinf(angle) * 5.0f + 5.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            bodyRotation[0] = -20.0f;
            bodyRotation[2] = 10.0f;
            leftHandOffset = plasma::Vector<3, float>(2.0f, 4.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-2.0f, 6.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, 0.0f);
        } else if (m_animationTime < m_creature->attackDuration()) {
            leftHandOffset = plasma::Vector<3, float>(-3.0f, -6.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, -6.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 130.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -130.0f);
            blendRate = 0.03f;
        }
        break;
    case AnimationHealingStream: // ability 35 (AnimationHealingStream): the hands circle
    {
        bodyRotation[0] = 20.0f;
        headRotation[0] = 20.0f;
        float angle = m_animationTime * 0.005f;
        leftHandOffset = plasma::Vector<3, float>(cosf(angle) * 3.0f - 1.0f, 5.0f, sinf(angle) * 5.0f + 5.0f);
        angle = (float)(m_animationTime * 0.005f + 3.141592653589793);
        rightHandOffset = plasma::Vector<3, float>(cosf(angle) * 3.0f + 1.0f, 5.0f, sinf(angle) * 5.0f + 5.0f);
    } break;
    case AnimationBlock: // ability 8 (AnimationBlock)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 80.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            // The intercept turns round meanwhile.
            if (m_creature->m_entityData.currentAbility == AbilityIntercept)
                creatureRotation[2] = (float)-m_animationTime / 300.0f * 360.0f;
        } else {
            bodyRotation[2] = -90.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 45.0f);
        }
        break;
    case AnimationBulwark: // AbilityBulwark, AbilityManaShield (AnimationBulwark)
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[0] = -20.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, -2.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -2.0f, 0.0f);
        } else {
            bodyRotation[0] = 0.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-2.0f, 5.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(2.0f, 6.0f, 0.0f);
        }
        break;
    case AnimationBlockJab: // ability 10 (AnimationBlockJab): a blocking stance, then a jab
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 80.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            // The intercept turns round meanwhile.
            if (m_creature->m_entityData.currentAbility == AbilityIntercept)
                creatureRotation[2] = (float)-m_animationTime / 300.0f * 360.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = -90.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 45.0f);
            leftHandPosition[0] = sinf((m_animationTime - 600) * 0.1f) + leftHandPosition[0];
            blendRate = 0.03f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield)
                                       ? plasma::Vector<3, float>(0.0f, 0.0f, 90.0f)
                                       : plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    case AnimationRightStab: // ability 6 (AnimationRightStab): a stab to the right
        if (m_animationTime > m_creature->anticipationTime() && m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = 45.0f;
            creatureRotation[2] = 45.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(8.0f, 0.0f, 4.0f);
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -30.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyOffset[1] = 3.0f;
            blendRate = 0.03f;
        } else {
            // Between the strikes: the weapons circle slowly.
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float angle = m_animationTime * 0.01f;
            rightHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            angle = (float)(m_animationTime * 0.01f + 1.5707963267948966);
            leftHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            bodyOffset[2] = sinf(m_animationTime * 0.01f) * 0.5f + bodyOffset[2];
            blendRate = 0.03f;
        }
        break;
    case AnimationLeftStab: // ability 7 (AnimationLeftStab): a stab to the left
        if (m_animationTime > m_creature->anticipationTime() && m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = -20.0f;
            creatureRotation[2] = -20.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 40.0f);
            leftHandOffset = plasma::Vector<3, float>(0.0f, 8.0f, 4.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 30.0f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyOffset[1] = 3.0f;
            blendRate = 0.03f;
        } else {
            // Between the strikes: the weapons circle slowly.
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float angle = m_animationTime * 0.01f;
            rightHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            angle = (float)(m_animationTime * 0.01f + 1.5707963267948966);
            leftHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            bodyOffset[2] = sinf(m_animationTime * 0.005f) * 0.5f + bodyOffset[2];
            blendRate = 0.03f;
        }
        break;
    case AnimationLeftSweep: // ability 13 (AnimationLeftSweep): a sweep from the left
        if (m_animationTime < m_creature->anticipationTime()) {
            float progress = (float)m_animationTime / (float)m_creature->anticipationTime();
            rightHandRotation = plasma::Vector<3, float>(-80.0f, 90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(-10.0f, 8.0f, 8.0f);
            bodyRotation[2] = -45.0f;
            creatureRotation[2] = progress * -20.0f;
            creatureOffset[0] = progress * 0.0f;
            if (m_creature->m_swingCount / 2 % 3 == 2 &&
                m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2)
                creatureRotation[2] += progress * 360.0f;
            blendRate = 0.04f;
        } else if (m_animationTime <
                   m_creature->anticipationTime() + m_creature->strikeTime() + m_creature->holdTime()) {
            float progress =
                (float)(m_animationTime - m_creature->anticipationTime()) * 1.2f / (float)m_creature->strikeTime();
            if (progress > 1.0f)
                progress = 1.0f;
            float remaining = 1.0f - progress;
            creatureOffset[0] = remaining * 0.0f - progress * 0.8f;
            rightHandRotation = remaining * plasma::Vector<3, float>(-90.0f, 90.0f, 48.0f) +
                                progress * plasma::Vector<3, float>(-110.0f, 60.0f, 0.0f);
            rightHandOffset = remaining * plasma::Vector<3, float>(-10.0f, 8.0f, 8.0f) +
                              progress * plasma::Vector<3, float>(-12.0f, 0.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyRotation[2] = progress * 90.0f - remaining * 45.0f;
            creatureRotation[2] = progress * 30.0f;
            blendRate = 0.05f;
            footRoll = progress * 10.0f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        // The left hand follows the right, a little behind.
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        // Empty in the code: only the two calls remain.
        if (m_creature->m_swingCount / 2 % 3 == 2 &&
            m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
        }
        break;
    case AnimationRightSweep: // ability 14 (AnimationRightSweep): a sweep from the right
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-8.0f, 6.0f, 6.0f);
            headRotation[0] = -10.0f;
            headRotation[2] = 30.0f;
            bodyRotation[2] = 45.0f;
            blendRate = 0.05f;
            creatureOffset[0] = -0.0f;
            creatureRotation[2] = 20.0f;
            float progress = (float)m_animationTime / (float)m_creature->anticipationTime();
            if (m_creature->m_swingCount / 2 % 3 == 2 &&
                m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime() / 2)
                creatureRotation[2] -= progress * 360.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            float progress =
                (float)(m_animationTime - m_creature->anticipationTime()) * 1.2f / (float)m_creature->strikeTime();
            if (progress > 1.0f)
                progress = 1.0f;
            float remaining = 1.0f - progress;
            creatureOffset[0] = progress * 0.8f + remaining * -0.0f;
            headRotation[2] = -30.0f;
            rightHandRotation = remaining * plasma::Vector<3, float>(0.0f, -90.0f, 20.0f) +
                                progress * plasma::Vector<3, float>(0.0f, -90.0f, -130.0f);
            rightHandOffset = remaining * plasma::Vector<3, float>(-8.0f, 6.0f, 6.0f) +
                              progress * plasma::Vector<3, float>(0.0f, 2.0f, 0.0f);
            bodyRotation[2] = remaining * 45.0f - progress * 45.0f;
            creatureRotation[2] = progress * -30.0f;
            creatureRotation[0] = progress * -20.0f;
            blendRate = 0.05f;
            footRoll = progress * 10.0f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        // The left hand follows the right, a little behind.
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        break;
    case AnimationOverheadChop: // ability 15 (AnimationOverheadChop): an overhead chop
        if (m_animationTime < m_creature->anticipationTime()) {
            float progress = (float)m_animationTime / (float)m_creature->anticipationTime();
            rightHandOffset = plasma::Vector<3, float>(2.0f, 2.0f, 14.0f);
            rightHandRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyRotation[2] = -45.0f;
            creatureRotation[2] = progress * -20.0f;
            creatureOffset[0] = progress * 0.5f;
            blendRate = 0.04f;
        } else if (m_animationTime <
                   m_creature->anticipationTime() + m_creature->strikeTime() + m_creature->holdTime()) {
            float progress =
                (float)(m_animationTime - m_creature->anticipationTime()) * 2.0f / (float)m_creature->strikeTime();
            if (progress > 1.0f)
                progress = 1.0f;
            float remaining = 1.0f - progress;
            creatureRotation[0] = remaining * 20.0f - progress * 10.0f;
            rightHandRotation = remaining * plasma::Vector<3, float>(90.0f, 0.0f, 45.0f) +
                                progress * plasma::Vector<3, float>(-90.0f, 0.0f, -45.0f);
            rightHandOffset = remaining * plasma::Vector<3, float>(2.0f, 2.0f, 14.0f) +
                              progress * plasma::Vector<3, float>(0.0f, 9.0f, 0.0f);
            bodyRotation[2] = progress * 45.0f - remaining * 45.0f;
            blendRate = 0.05f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        // The left hand follows the right, a little behind.
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        break;
    case AnimationPerforate: // ability 5 (AnimationPerforate): a lunge
        if (m_animationTime < m_creature->anticipationTime()) {
            rightHandOffset = plasma::Vector<3, float>(4.0f, 2.0f, 8.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -180.0f, 90.0f);
            bodyRotation[2] = -90.0f;
            creatureOffset[1] = -1.0f;
            blendRate = 0.02f;
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            rightHandOffset = plasma::Vector<3, float>(6.0f, 0.0f, 0.0f);
            creatureOffset[1] = 0.5f;
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -90.0f);
            leftFootTurnedOut = true;
            blendRate = 0.05f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        // The left hand follows the right, a little behind.
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        break;
    case AnimationLeftBackhand: // abilities 3, 9, 19, 62 (AnimationLeftBackhand): a backhand from the left
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = -40.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -30.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 20.0f);
            rightHandOffset[1] -= 6.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = 90.0f;
            // A shield is held upright.
            leftHandRotation = m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield
                                   ? plasma::Vector<3, float>(0.0f, 0.0f, 40.0f)
                                   : plasma::Vector<3, float>(-40.0f, 0.0f, -30.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, 0.0f, 4.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, -1.0f, 0.0f);
            leftFootTurnedOut = true;
            blendRate = 0.05f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield
                                       ? plasma::Vector<3, float>(0.0f, 0.0f, 90.0f)
                                       : plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    case AnimationRightBackhand: // abilities 4, 18 (AnimationRightBackhand): a backhand from the right
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 40.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 30.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -20.0f);
            leftHandOffset[1] -= 6.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = -90.0f;
            rightHandRotation = plasma::Vector<3, float>(-40.0f, 0.0f, 30.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 0.0f, 4.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 90.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, -1.0f, 0.0f);
            leftFootTurnedOut = true;
            blendRate = 0.05f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    case AnimationKick: // ability 20, AbilityRangerKick (AnimationKick): a spinning kick
        rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 2.0f, 4.0f);
        leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
        leftHandOffset = plasma::Vector<3, float>(0.0f, 2.0f, 4.0f);
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 50.0f;
            leftHandRotation[0] = -90.0f;
            leftHandRotation[2] = 90.0f;
            rightHandRotation[0] = -90.0f;
            leftHandOffset = plasma::Vector<3, float>(-4.0f, -4.0f, -2.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 1.0f);
        } else if (m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
            float kick =
                (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime() * 1.5f;
            if (kick > 1.0f)
                kick = 1.0f;
            else if (0.0f > kick)
                kick = 0.0f;
            creatureRotation[2] = kick * -270.0f;
            creatureRotation[1] = kick * -40.0f;
            bodyOffset[2] += 4.0f;
            footRoll = kick * 40.0f;
            leftHandRotation[0] = -90.0f;
            leftHandRotation[2] = 90.0f;
            rightHandRotation[0] = -90.0f;
            leftHandOffset = plasma::Vector<3, float>(-4.0f, -4.0f, -2.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 1.0f);
            headRotation[0] = -30.0f;
            headRotation[2] = -60.0f;
            blendRate = 0.03f;
        } else {
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float angle = m_animationTime * 0.01f;
            rightHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            angle = (float)(m_animationTime * 0.01f + 1.5707963267948966);
            leftHandOffset = plasma::Vector<3, float>(0.0f, sinf(angle) + 2.0f, cosf(angle) + 4.0f);
            bodyOffset[2] = sinf(m_animationTime * 0.005f) * 0.5f + bodyOffset[2];
        }
        break;
    case AnimationLeftSlash: // ability 2 (AnimationLeftSlash): a slash from the left, the hands turned by the aim Every third
        // double swing (m_swingCount counts the swings) also turns the creature.
        if (m_creature->m_swingCount / 2 % 3 == 2) {
            float turn = (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime();
            if (turn > 1.0f)
                turn = 1.0f;
            else if (0.0f > turn)
                turn = 0.0f;
            creatureRotation[2] = turn * -360.0f;
            blendRate = 0.03f;
        } else {
            creatureRotation[2] = 0.0f;
        }
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 40.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 30.0f);
            leftHandOffset[1] -= 4.0f;
            leftHandOffset[2] += 4.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -120.0f);
            rightHandOffset[1] -= 6.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = -90.0f;
            creatureOffset[0] = -0.1f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -30.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -120.0f);
            rightHandOffset[1] -= 6.0f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield)
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
        break;
    case AnimationRightSlash: // ability 1 (AnimationRightSlash): a slash from the right
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        // Every third double swing (m_swingCount counts the swings) also turns the creature.
        if (m_creature->m_swingCount / 2 % 3 == 1) {
            float turn = (float)(m_animationTime - m_creature->anticipationTime()) / (float)m_creature->strikeTime();
            if (turn > 1.0f)
                turn = 1.0f;
            else if (0.0f > turn)
                turn = 0.0f;
            creatureRotation[2] = turn * -360.0f;
            blendRate = 0.03f;
        } else {
            creatureRotation[2] = 0.0f;
        }
        if (m_animationTime < m_creature->anticipationTime()) {
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 90.0f);
            rightHandOffset[1] += 6.0f;
            rightHandOffset[2] += 4.0f;
        } else if (m_animationTime < m_creature->attackDuration()) {
            bodyRotation[2] = 20.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -60.0f, 90.0f);
            leftHandOffset[1] -= 4.0f;
            leftHandOffset[2] += 4.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -220.0f);
            rightHandOffset[1] -= 6.0f;
            creatureOffset[0] = 0.1f;
            leftFootPosition[1] -= 4.0f;
            leftFootTurnedOut = true;
            rightFootPosition[1] += 4.0f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (m_creature->m_entityData.race != RaceZombie && m_creature->m_entityData.race != RaceMole) {
                leftHandRotation[0] = -60.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] += 2.0f;
                if (m_rightWeaponSprite != 0 && m_leftWeaponSprite == 0) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] += 3.0f;
                    leftHandPosition = rightHandPosition;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield)
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
        break;
    case AnimationActionLocked: // stunned or locked (EntityData::actionLockTime above zero; AnimationActionLocked)
        bodyRotation[0] += 30.0f;
        leftHandOffset = plasma::Vector<3, float>(-2.0f, 0.0f, 0.0f);
        rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
        leftHandRotation = plasma::Vector<3, float>(-10.0f, -10.0f, 90.0f);
        rightHandRotation = plasma::Vector<3, float>(-10.0f, 10.0f, -90.0f);
        break;
    default:
        // Animations 12, 13, 14, 36 and 64 have no pose of their own.
        break;
    }

    // --- Corrections to the target ---
    if (m_animation == AnimationNone)
        m_animationTime = 0;
    leftShoulderPosition = 0.5f * (leftShoulderPosition + leftHandPosition);
    rightShoulderPosition = 0.5f * (rightShoulderPosition + rightHandPosition);
    // Stunned: the creature falls over, onto its side when the appearance flag is set, else forwards;
    // it is lifted by half the difference of its size across and its height.
    if (m_creature->m_entityData.stunTime > 0) {
        if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped)) {
            creatureRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[0] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        } else {
            creatureRotation = plasma::Vector<3, float>(90.0f, 0.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        }
    }
    // Climbing a wall: the hands raised.
    if (m_creature->m_entityData.binaryToggles.climbing &&
        (m_creature->m_entityData.flags & (1 << CreatureFlagAtWall))) {
        rightHandOffset[2] += 7.0f;
        leftHandOffset[2] += 7.0f;
        leftHandRotation[0] += 90.0f;
        rightHandRotation[0] += 90.0f;
    }
    // A torch in the left hand is held up.
    if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponTorch) {
        leftHandRotation[0] = 0.0f;
        leftHandOffset[1] += 4.0f;
        leftHandOffset[2] += 4.0f;
    }
    // Leaning forward at speed; less while moving the way the creature pushes.
    if (!(m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped)) &&
        m_creature->m_entityData.currentAbility != AbilityRidePet &&
        m_creature->m_entityData.currentAbility != AbilitySailBoat) {
        if (leaningForward) {
            creatureRotation[0] -= 25.0f;
            headRotation[0] += 10.0f;
        } else {
            // Reads the horizontal velocity components and copies them as a pair.
            plasma::Vector<2, float> horizontal(m_creature->m_entityData.velocity[0],
                                                m_creature->m_entityData.velocity[1]);
            if (horizontal.squaredLength() > 0.1f &&
                (m_creature->m_entityData.acceleration & m_creature->m_entityData.velocity) > 0.0f) {
                creatureRotation[0] -= 10.0f;
                headRotation[0] += 5.0f;
            }
        }
    }

    // --- The blend --- The share of the way from the current pose to the target that milliseconds at
    // blendRate cover; the angles turn the shorter way.
    float poseBlend = 0.0f;
    blend(poseBlend, 1.0f, milliseconds, blendRate);
    m_rightHandRotation += poseBlend * (rightHandRotation - m_rightHandRotation);
    m_leftHandRotation += poseBlend * (leftHandRotation - m_leftHandRotation);
    m_rightHandOffset += poseBlend * (rightHandOffset - m_rightHandOffset);
    m_leftHandOffset += poseBlend * (leftHandOffset - m_leftHandOffset);
    m_bodyRotation += poseBlend * (bodyRotation - m_bodyRotation);
    m_creatureOffset += poseBlend * (creatureOffset - m_creatureOffset);
    m_glideLift = (glideLift - m_glideLift) * poseBlend + m_glideLift;
    m_creatureRotation[0] = interpolateAngle(m_creatureRotation[0], creatureRotation[0], poseBlend);
    m_creatureRotation[1] = interpolateAngle(m_creatureRotation[1], creatureRotation[1], poseBlend);
    m_creatureRotation[2] = interpolateAngle(m_creatureRotation[2], creatureRotation[2], poseBlend);
    m_headRotation[0] = interpolateAngle(m_headRotation[0], headRotation[0], poseBlend);
    m_headRotation[1] = interpolateAngle(m_headRotation[1], headRotation[1], poseBlend);
    m_headRotation[2] = interpolateAngle(m_headRotation[2], headRotation[2], poseBlend);
    m_tailRotation[0] = interpolateAngle(m_tailRotation[0], tailRotation[0], poseBlend);
    m_tailRotation[1] = interpolateAngle(m_tailRotation[1], tailRotation[1], poseBlend);
    m_tailRotation[2] = interpolateAngle(m_tailRotation[2], tailRotation[2], poseBlend);
    m_bodyTurn = (bodyTurn - m_bodyTurn) * poseBlend + m_bodyTurn;
    m_handsTurn += poseBlend * (handsTurn - m_handsTurn);
    m_bodyOffset += poseBlend * (bodyOffset - m_bodyOffset);
    m_headOffset += poseBlend * (headOffset - m_headOffset);
    // The support ray's facing follows the aim (unit length) at its own rate; otherwise it is cleared.
    if (m_creature->m_entityData.currentAbility == AbilitySupportRay) {
        plasma::Vector<3, float> facing = m_creature->m_entityData.attackRotation;
        if (facing.squaredLength() > 0.0f)
            facing.normalize();
        float facingShare = 0.0f;
        blend(facingShare, 1.0f, milliseconds, 0.01f);
        m_blendedFacing += facingShare * (facing - m_blendedFacing);
    } else {
        m_blendedFacing = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    // The pose adds to the rest pose; the shoulders follow the hands a little.
    rightHandAngles += m_rightHandRotation;
    leftHandAngles += m_leftHandRotation;
    rightHandPosition += m_rightHandOffset;
    leftHandPosition += m_leftHandOffset;
    rightShoulderPosition += 0.4f * m_rightHandOffset;
    leftShoulderPosition += 0.4f * m_leftHandOffset;

    // --- Hidden parts ---
    if (hiddenParts & (1 << HiddenCreatureHead))
        m_headSprite = 0;
    if (hiddenParts & (1 << HiddenCreatureHair))
        m_hairSprite = 0;
    if (hiddenParts & (1 << HiddenCreatureChest))
        m_chestSprite = 0;
    if (hiddenParts & (1 << HiddenCreatureShoulders))
        m_shoulderArmorSprite = 0;
    if (hiddenParts & (1 << HiddenCreatureWeapons)) {
        m_rightWeaponSprite = 0;
        m_leftWeaponSprite = 0;
    }
    shader->setMaterialColor(color);

    // --- The creature's matrix --- At the drawn position relative to the origin, lowered by the step
    // still to climb (m_stepOffset) and raised by the creature offset.
    plasma::Matrix<float> creatureMatrix;
    creatureMatrix.identity();
    creatureMatrix.translate((float)(m_creature->m_drawnPosition[0] + originX),
                             (float)(m_creature->m_drawnPosition[1] + originY),
                             (float)(m_creature->m_drawnPosition[2] - m_creature->m_stepOffset + m_creatureOffset[2]));
    // Rolling: a full turn over the roll's 600 ms about the axis across the velocity (along it for
    // race 101), except during the dashes of a rogue of specialization 1. The hands come up while
    // rolling or gliding.
    if (m_creature->m_entityData.rollTime != 0 &&
        !(m_creature->m_entityData.classType == ClassRogue &&
          m_creature->m_entityData.specialization == SpecializationNinja &&
          m_creature->m_entityData.timeSinceAbility < m_creature->attackDuration() &&
          (m_creature->m_entityData.currentAbility == AbilityAmbush ||
           m_creature->m_entityData.currentAbility == AbilityPerforate ||
           m_creature->m_entityData.currentAbility == AbilityKick))) {
        float rollAngle = (1.0f - (float)m_creature->m_entityData.rollTime / 600.0f) * -360.0f;
        handsTurn = m_creature->m_entityData.velocity.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        if (m_creature->m_entityData.race == RaceDragon)
            handsTurn = m_creature->m_entityData.velocity;
        if (handsTurn.squaredLength() > 0.01f) {
            handsTurn.normalize();
            creatureMatrix.rotate(rollAngle, handsTurn);
        }
        leftHandPosition[2] +=
            m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
        rightHandPosition[2] +=
            m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
    }
    if (m_creature->m_entityData.binaryToggles.gliding) {
        leftHandPosition[2] +=
            m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
        rightHandPosition[2] +=
            m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
    }
    creatureMatrix.rotateZ(m_creature->m_drawnRotation[2]);
    creatureMatrix.translate(0.0f, 0.0f, m_glideLift);
    creatureMatrix.rotateY(m_creature->m_drawnRotation[1]);
    creatureMatrix.rotateX(m_creature->m_drawnRotation[0]);
    creatureMatrix.translate(m_creatureOffset[0], m_creatureOffset[1], 0.0f);
    creatureMatrix.rotateZ(m_creatureRotation[2]);
    creatureMatrix.rotateY(m_creatureRotation[1]);
    creatureMatrix.rotateX(m_creatureRotation[0]);
    creatureMatrix.translate(0.0f, 0.0f, -m_glideLift);
    // Model units: 11.2 to the creature's width.
    float modelScale = m_creature->m_entityData.appearance.size[0] / 11.2f;
    creatureMatrix.scale(modelScale, modelScale, modelScale);
    // The body bobs with the walk (more while sneaking).
    float strideSize = fabsf(swing);
    float bob = strideSize * 4.0f;
    if (m_creature->m_entityData.currentAbility == AbilitySneak)
        bob *= 4.0f;

    // --- The body ---
    m_bodyMatrix = creatureMatrix;
    m_bodyMatrix.translate(0.0f, 0.0f, bob);
    m_bodyMatrix.translate(m_bodyOffset[0], m_bodyOffset[1], m_bodyOffset[2]);
    m_bodyMatrix.rotateZ(m_bodyRotation[2]);
    m_bodyMatrix.rotateY(m_bodyRotation[1]);
    m_bodyMatrix.rotateX(m_creature->m_entityData.appearance.chestRotation + m_bodyRotation[0]);

    // The hang glider (the special item) above the back while gliding.
    if (m_creature->m_entityData.binaryToggles.gliding) {
        Sprite *glider = spriteManager->itemSprite(&m_creature->m_entityData.equipment.special);
        if (glider) {
            plasma::Matrix<float> gliderMatrix = m_bodyMatrix;
            gliderMatrix.translate(chestPosition +
                                   plasma::Vector<3, float>(0.0f, 0.0f,
                                                            m_creature->m_entityData.appearance.size[2] * 8.0f /
                                                                m_creature->m_entityData.appearance.size[0]));
            gliderMatrix.rotateX(-m_creatureRotation[0]);
            gliderMatrix.scale(chestScale * 2.0f, chestScale * 2.0f, chestScale * 2.0f);
            gliderMatrix.translate(glider->sizeX() * -0.5f, glider->sizeY() * -0.5f, glider->sizeZ() * -0.5f);
            shader->setMatrices(gliderMatrix, view, projection);
            glider->draw();
        }
    }
    // The boat (the special item) under the body while sailing.
    if (m_creature->m_entityData.currentAbility == AbilitySailBoat) {
        Sprite *boat = spriteManager->itemSprite(&m_creature->m_entityData.equipment.special);
        if (boat) {
            plasma::Matrix<float> boatMatrix = m_bodyMatrix;
            boatMatrix.translate(chestPosition +
                                 plasma::Vector<3, float>(0.0f, 0.0f,
                                                          m_creature->m_entityData.appearance.size[2] * -9.0f /
                                                              m_creature->m_entityData.appearance.size[0]));
            boatMatrix.rotateX(-m_bodyRotation[0]);
            boatMatrix.rotateZ(-m_creatureRotation[2]);
            boatMatrix.scale(chestScale * 2.0f, chestScale * 2.0f, chestScale * 2.0f);
            boatMatrix.translate(1.0f, 2.0f, 0.0f);
            boatMatrix.translate(boat->sizeX() * -0.5f, boat->sizeY() * -0.5f, 0.0f);
            shader->setMatrices(boatMatrix, view, projection);
            boat->draw();
        }
    }
    // Animation 72, for its first five seconds: sprite 2566 turning between the hands (last frame's
    // hands matrix).
    if (m_animation == AnimationPresentItem && m_animationTime < 5000) {
        Sprite *item = spriteManager->sprite(SpriteManacube);
        if (item) {
            plasma::Matrix<float> itemMatrix = m_handsMatrix;
            itemMatrix.translate(0.5f * (leftHandPosition + rightHandPosition) +
                                 plasma::Vector<3, float>(0.0f, 0.0f, 8.0f));
            itemMatrix.rotateZ(m_animationTime * 0.02f);
            itemMatrix.rotateY(45.0f);
            itemMatrix.rotateX(45.0f);
            itemMatrix.scale(1.0f, 1.0f, 1.0f);
            itemMatrix.translate(item->sizeX() * -0.5f, item->sizeY() * -0.5f, item->sizeZ() * -0.5f);
            shader->setMatrices(itemMatrix, view, projection);
            item->draw();
        }
    }

    // --- The chest --- Turned with half the body turn and half the arm swing.
    if (m_chestSprite) {
        m_chestMatrix = m_bodyMatrix;
        m_chestMatrix.translate(chestPosition);
        double swingDegrees = (armSwing * 0.5f * 180.0f) / 3.141592653589793;
        m_chestMatrix.rotateZ((float)(m_bodyTurn * 0.5f + swingDegrees));
        m_chestMatrix.scale(chestScale, chestScale, chestScale);
        m_chestMatrix.translate(m_chestSprite->sizeX() * -0.5f, m_chestSprite->sizeY() * -0.5f,
                                m_chestSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_chestMatrix, view, projection);
        shader->setShininess(m_creature->m_entityData.equipment.chest.shininess());
        m_chestSprite->draw();
        shader->setShininess(0.0f);
        // While recovering (revivalPenalty below zero): sprite 2550 on the chest, upside down.
        if (0.0f > m_creature->m_entityData.revivalPenalty) {
            Sprite *sign = spriteManager->sprite(SpritePlaster);
            if (sign) {
                plasma::Matrix<float> signMatrix = m_bodyMatrix;
                signMatrix.translate(rightHandAngles);
                signMatrix.rotateZ((float)(m_bodyTurn * 0.5f + swingDegrees));
                signMatrix.scale(chestScale, chestScale, chestScale);
                signMatrix.translate(0.0f, m_chestSprite->sizeY() * -0.5f, 0.0f);
                signMatrix.scale(0.5f, 0.5f, 0.5f);
                signMatrix.rotateY(45.0f);
                signMatrix.rotateZ(180.0f);
                signMatrix.translate(sign->sizeX() * -0.5f, 0.0f, sign->sizeZ() * -0.5f);
                shader->setMatrices(signMatrix, view, projection);
                sign->draw();
            }
        }
    }

    // --- The neck item --- Hangs at the chest's front and swings forwards with the arms.
    if (m_neckSprite) {
        m_neckMatrix = m_bodyMatrix;
        m_neckMatrix.translate(chestPosition + plasma::Vector<3, float>(0.0f, 3.0f, 6.05f));
        m_neckMatrix.rotateZ((float)((armSwing * 0.5f * 180.0f) / 3.141592653589793 + m_bodyTurn * 0.5f));
        m_neckMatrix.rotateX((float)(armSwing * 0.7 * armSwing * 180.0 / 3.141592653589793));
        m_neckMatrix.scale(chestScale, chestScale, chestScale);
        m_neckMatrix.translate(m_neckSprite->sizeX() * -0.5f, -1.5f, (float)-m_neckSprite->sizeZ());
        shader->setMatrices(m_neckMatrix, view, projection);
        shader->setShininess(m_creature->m_entityData.equipment.neck.shininess());
        m_neckSprite->draw();
        shader->setShininess(0.0f);
    }

    // --- The shoulder armour --- Rises with the walk cycle's weight.
    if (m_shoulderArmorSprite) {
        m_shoulderArmorMatrix = m_bodyMatrix;
        m_shoulderArmorMatrix.translate(0.0f, 0.0f, m_creature->m_walkAmplitude * 2.0f + 4.0f);
        m_shoulderArmorMatrix.translate(chestPosition);
        m_shoulderArmorMatrix.rotateZ((float)((armSwing * 0.5f * 180.0f) / 3.141592653589793));
        m_shoulderArmorMatrix.scale(shoulderArmorScale, shoulderArmorScale, shoulderArmorScale);
        m_shoulderArmorMatrix.scale(chestScale + 0.1f, chestScale + 0.1f, chestScale + 0.1f);
        m_shoulderArmorMatrix.translate(m_shoulderArmorSprite->sizeX() * -0.5f, m_shoulderArmorSprite->sizeY() * -0.5f,
                                        m_shoulderArmorSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_shoulderArmorMatrix, view, projection);
        shader->setShininess(m_creature->m_entityData.equipment.shoulder.shininess());
        m_shoulderArmorSprite->draw();
        shader->setShininess(0.0f);
    }

    // --- The tail --- Swings against the arms, then the pose's tail rotation.
    if (m_tailSprite) {
        m_tailMatrix = m_bodyMatrix;
        m_tailMatrix.translate(chestPosition);
        m_tailMatrix.translate(tailPosition);
        m_tailMatrix.rotateX(m_creature->m_entityData.appearance.tailRotation);
        m_tailMatrix.rotateZ((float)((armSwing * -180.0f) / 3.141592653589793));
        m_tailMatrix.rotateZ(m_tailRotation[2]);
        m_tailMatrix.rotateY(m_tailRotation[1]);
        m_tailMatrix.rotateX(m_tailRotation[0]);
        m_tailMatrix.scale(tailScale, tailScale, tailScale);
        m_tailMatrix.scale(chestScale, chestScale, chestScale);
        m_tailMatrix.translate(m_tailSprite->sizeX() * -0.5f, (float)-m_tailSprite->sizeY(),
                               m_tailSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_tailMatrix, view, projection);
        m_tailSprite->draw();
    }

    // --- The head --- On the body, lowered by half the bob, turned back by the body's own rotation
    // and then by the head's: the aim's pitch (EntityData: headRotation) less the drawn pitch, plus
    // the pose.
    if (m_headSprite) {
        m_headMatrix = m_bodyMatrix;
        m_headMatrix.translate(m_headOffset[0] + headPosition[0], m_headOffset[1] + headPosition[1],
                               m_headOffset[2] + headPosition[2] - bob * 0.5f);
        m_headMatrix.translate(0.0f, 0.0f, -3.0f);
        m_headMatrix.rotateX(-m_bodyRotation[0] - m_creature->m_entityData.appearance.chestRotation);
        m_headMatrix.rotateY(-m_bodyRotation[1]);
        m_headMatrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        m_headMatrix.rotateY(m_headRotation[1]);
        m_headMatrix.rotateX(m_creature->m_entityData.headRotation - m_creature->m_drawnRotation[0] +
                             m_headRotation[0]);
        m_headMatrix.translate(0.0f, 0.0f, 3.0f);
        m_headMatrix.scale(headScale, headScale, headScale);
        // Race 101's head sits further back on its neck.
        if (m_creature->m_entityData.race == RaceDragon)
            m_headMatrix.translate(m_headSprite->sizeX() * -0.5f, m_headSprite->sizeY() * -0.2f,
                                   m_headSprite->sizeZ() * -0.5f);
        else
            m_headMatrix.translate(m_headSprite->sizeX() * -0.5f, m_headSprite->sizeY() * -0.5f,
                                   m_headSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_headMatrix, view, projection);
        m_headSprite->draw();
    }

    // --- The hair --- Placed like the head, in the appearance's hair colour (unlit by the draw colour
    // with AppearanceFlagSuppressHairDebris).
    if (m_hairSprite) {
        m_hairMatrix = m_bodyMatrix;
        m_hairMatrix.translate(m_headOffset[0] + headPosition[0], m_headOffset[1] + headPosition[1],
                               m_headOffset[2] + headPosition[2] - bob * 0.5f);
        m_hairMatrix.translate(0.0f, 0.0f, -3.0f);
        m_hairMatrix.rotateX(-m_bodyRotation[0] - m_creature->m_entityData.appearance.chestRotation);
        m_hairMatrix.rotateY(-m_bodyRotation[1]);
        m_hairMatrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        m_hairMatrix.rotateY(m_headRotation[1]);
        m_hairMatrix.rotateX(m_creature->m_entityData.headRotation - m_creature->m_drawnRotation[0] +
                             m_headRotation[0]);
        m_hairMatrix.translate(0.0f, 0.0f, 3.0f);
        m_hairMatrix.scale(headScale, headScale, headScale);
        m_hairMatrix.translate(m_hairSprite->sizeX() * -0.5f, m_hairSprite->sizeY() * -0.5f,
                               m_hairSprite->sizeZ() * -0.5f);
        shader->setMaterialColor(
            (m_creature->m_entityData.appearance.appearanceFlags & ((1 << AppearanceFlagSuppressHairDebris)))
                ? plasma::Vector<4, float>(m_creature->m_entityData.appearance.hairColor[0] / 255.0f,
                                           m_creature->m_entityData.appearance.hairColor[1] / 255.0f,
                                           m_creature->m_entityData.appearance.hairColor[2] / 255.0f, 1.0f)
                : color * plasma::Vector<4, float>(m_creature->m_entityData.appearance.hairColor[0] / 255.0f,
                                                   m_creature->m_entityData.appearance.hairColor[1] / 255.0f,
                                                   m_creature->m_entityData.appearance.hairColor[2] / 255.0f, 1.0f));
        shader->setMatrices(m_hairMatrix, view, projection);
        m_hairSprite->draw();
        shader->setMaterialColor(color);
    }

    // --- The hands' matrix --- Bobs with the body; pitched with the aim (not while climbing a wall or
    // in animations 2, 3 and 37), further while sneaking; the hands' turn about a point four units
    // down, then the body's rotation.
    m_handsMatrix = creatureMatrix;
    m_handsMatrix.translate(0.0f, 0.0f, bob);
    m_handsMatrix.translate(m_bodyOffset[0], m_bodyOffset[1], m_bodyOffset[2]);
    if (!(m_creature->m_entityData.binaryToggles.climbing &&
          (m_creature->m_entityData.flags & (1 << CreatureFlagAtWall))) &&
        m_animation != AnimationTwoHandedSweep && m_animation != AnimationTwoHandedThrust &&
        m_animation != AnimationTwoHandedSwing)
        m_handsMatrix.rotateX(m_creature->m_entityData.headRotation * 2.0f);
    if (m_creature->m_entityData.binaryToggles.sneaking)
        m_handsMatrix.rotateX(60.0f);
    m_handsMatrix.translate(0.0f, 0.0f, -4.0f);
    m_handsMatrix.rotateY(m_handsTurn[1]);
    m_handsMatrix.translate(0.0f, 0.0f, 4.0f);
    m_handsMatrix.rotateZ(m_bodyRotation[2]);
    m_handsMatrix.rotateY(m_bodyRotation[1]);
    m_handsMatrix.rotateX(m_bodyRotation[0]);

    // --- The shoulders --- The appearance's shoulder model at each shoulder.
    if (m_shouldersSprite) {
        m_leftShoulderMatrix = m_handsMatrix;
        m_leftShoulderMatrix.translate(leftShoulderPosition[0], leftShoulderPosition[1], leftShoulderPosition[2]);
        m_leftShoulderMatrix.scale(shouldersScale, shouldersScale, shouldersScale);
        m_leftShoulderMatrix.translate(m_shouldersSprite->sizeX() * -0.5f, m_shouldersSprite->sizeY() * -0.5f,
                                       m_shouldersSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_leftShoulderMatrix, view, projection);
        m_shouldersSprite->draw();
        m_rightShoulderMatrix = m_handsMatrix;
        m_rightShoulderMatrix.translate(rightShoulderPosition[0], rightShoulderPosition[1], rightShoulderPosition[2]);
        m_rightShoulderMatrix.scale(shouldersScale, shouldersScale, shouldersScale);
        m_rightShoulderMatrix.translate(m_shouldersSprite->sizeX() * -0.5f, m_shouldersSprite->sizeY() * -0.5f,
                                        m_shouldersSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_rightShoulderMatrix, view, projection);
        m_shouldersSprite->draw();
    }

    // --- The item in use --- Drinking or eating: the action item between the hands while its cooldown
    // runs.
    if ((m_creature->m_entityData.currentAbility == AbilityDrink ||
         m_creature->m_entityData.currentAbility == AbilityEat) &&
        m_creature->m_entityData.timeSinceAbility < m_creature->m_entityData.actionItem.cooldown()) {
        Sprite *item = spriteManager->itemSprite(&m_creature->m_entityData.actionItem);
        if (item) {
            plasma::Matrix<float> itemMatrix = m_handsMatrix;
            itemMatrix.translate(0.5f * (leftHandPosition + rightHandPosition));
            itemMatrix.rotateX(leftHandAngles[0]);
            itemMatrix.scale(handsScale, handsScale, handsScale);
            itemMatrix.translate(item->sizeX() * -0.5f, item->sizeY() * -0.5f, item->sizeZ() * -0.5f);
            shader->setMatrices(itemMatrix, view, projection);
            item->draw();
        }
    }
    // Ability 82: the pet item (pet food) held out between the hands.
    if (m_creature->m_entityData.currentAbility == AbilityOfferPetFood) {
        Sprite *food = spriteManager->itemSprite(&m_creature->m_entityData.equipment.pet);
        if (food) {
            plasma::Matrix<float> foodMatrix = m_handsMatrix;
            foodMatrix.translate(0.5f * (leftHandPosition + rightHandPosition));
            foodMatrix.rotateX(leftHandAngles[0]);
            foodMatrix.scale(handsScale, handsScale, handsScale);
            foodMatrix.translate(food->sizeX() * -0.5f, food->sizeY() * -0.5f, 0.0f);
            shader->setMatrices(foodMatrix, view, projection);
            food->draw();
        }
    }
    // A bow or crossbow in either hand: the quiver (sprite 839) on the back, slanted to that side.
    if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        (m_creature->m_entityData.equipment.weaponLeft.id == WeaponBow ||
         m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow)) {
        Sprite *quiver = spriteManager->sprite(SpriteQuiver);
        if (quiver) {
            // The copy of the hands' matrix is overwritten at once (as in the code).
            plasma::Matrix<float> quiverMatrix = m_handsMatrix;
            quiverMatrix = m_bodyMatrix;
            quiverMatrix.rotateZ((float)((armSwing * 0.25f * 180.0f) / 3.141592653589793));
            quiverMatrix.translate(0.0f, -7.1f, 0.1f);
            quiverMatrix.rotate(45.0f, 0.0f, 1.0f, 0.0f);
            quiverMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            quiverMatrix.scale(rightWeaponScale + 0.01f, rightWeaponScale + 0.01f, rightWeaponScale + 0.01f);
            quiverMatrix.translate(quiver->sizeX() * -0.5f, quiver->sizeY() * -0.5f, quiver->sizeZ() * -0.5f);
            shader->setMatrices(quiverMatrix, view, projection);
            quiver->draw();
        }
    }
    if (m_creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
        (m_creature->m_entityData.equipment.weaponRight.id == WeaponBow ||
         m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow)) {
        Sprite *quiver = spriteManager->sprite(SpriteQuiver);
        if (quiver) {
            plasma::Matrix<float> quiverMatrix = m_handsMatrix;
            quiverMatrix = m_bodyMatrix;
            quiverMatrix.rotateZ((float)((armSwing * 0.25f * 180.0f) / 3.141592653589793));
            quiverMatrix.translate(0.0f, -7.1f, 0.1f);
            quiverMatrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
            quiverMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            quiverMatrix.scale(rightWeaponScale + 0.01f, rightWeaponScale + 0.01f, rightWeaponScale + 0.01f);
            quiverMatrix.translate(quiver->sizeX() * -0.5f, quiver->sizeY() * -0.5f, quiver->sizeZ() * -0.5f);
            shader->setMatrices(quiverMatrix, view, projection);
            quiver->draw();
        }
    }

    // --- The right weapon --- Outside combat (no ability, or one of the peaceful ones, or gliding) a
    // weapon other than fists and bracelets hangs on the back; otherwise it is held in the right hand,
    // offset per weapon kind. Its tip and base start trails 0 and 1.
    if (m_rightWeaponSprite) {
        m_rightWeaponMatrix = m_handsMatrix;
        // A charged weapon pulses brighter.
        if (m_creature->m_entityData.charge > 0.0f)
            shader->setMaterialColor(color * ((cosf(m_animationTime * 0.01f) + 1.0f) * 0.25f + 1.0f));
        unsigned char currentAbility = m_creature->m_entityData.currentAbility;
        if ((currentAbility == AbilityNone || currentAbility == AbilityRangerKick ||
             currentAbility == AbilityPresentItem || currentAbility == AbilityRidePet ||
             currentAbility == AbilitySailBoat || currentAbility == AbilityTeleportToCity ||
             currentAbility == AbilityQuake || currentAbility == AbilityShuriken || currentAbility == AbilityDrink ||
             currentAbility == AbilityOfferPetFood || currentAbility == Ability85 || currentAbility == AbilitySneak ||
             currentAbility == AbilitySit || currentAbility == AbilityEat || currentAbility == AbilitySleep ||
             m_creature->m_entityData.binaryToggles.gliding) &&
            m_creature->m_entityData.equipment.weaponRight.id != WeaponFist &&
            m_creature->m_entityData.equipment.weaponRight.id != WeaponBracelet) {
            m_rightWeaponMatrix = m_bodyMatrix;
            m_rightWeaponMatrix.rotateZ((float)((armSwing * 0.25f * 180.0f) / 3.141592653589793));
            m_rightWeaponMatrix.translate(0.0f, -7.1f, 0.1f);
            // A staff hangs the other way.
            if (m_creature->m_entityData.equipment.weaponRight.id == WeaponStaff)
                m_rightWeaponMatrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
            else
                m_rightWeaponMatrix.rotate(-135.0f, 0.0f, 1.0f, 0.0f);
            m_rightWeaponMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            m_rightWeaponMatrix.scale(rightWeaponScale, rightWeaponScale, rightWeaponScale);
            m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f, m_rightWeaponSprite->sizeY() * -0.5f,
                                          m_rightWeaponSprite->sizeZ() * -0.5f);
        } else {
            m_rightWeaponMatrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            m_rightWeaponMatrix.rotateZ(rightHandAngles[2]);
            m_rightWeaponMatrix.rotateY(rightHandAngles[1]);
            m_rightWeaponMatrix.rotateX(rightHandAngles[0]);
            m_rightWeaponMatrix.rotate(rightWeaponTurn[2], 0.0f, 0.0f, 1.0f);
            m_rightWeaponMatrix.rotate(rightWeaponTurn[1], 0.0f, 1.0f, 0.0f);
            m_rightWeaponMatrix.rotate(m_creature->m_entityData.headRotation * 0.5f + rightWeaponTurn[0], 1.0f, 0.0f,
                                       0.0f);
            m_rightWeaponMatrix.scale(rightWeaponScale, rightWeaponScale, rightWeaponScale);
            // Where the hand grips each kind of weapon.
            if (m_rightWeaponSprite == spriteManager->sprite(SpriteArrow)) {
                m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f - 1.0f, 0.5f, 1.0f);
            } else {
                unsigned char weapon = m_creature->m_entityData.equipment.weaponRight.id;
                if (weapon == WeaponStaff) {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f,
                                                  m_rightWeaponSprite->sizeY() * -0.5f, -10.0f);
                } else if (weapon == WeaponBracelet) {
                    m_rightWeaponMatrix.scale(0.6f, 0.6f, 0.6f);
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f,
                                                  (float)(1 - m_rightWeaponSprite->sizeY()),
                                                  m_rightWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponBow) {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f,
                                                  1.5f - m_rightWeaponSprite->sizeY(),
                                                  m_rightWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponCrossbow) {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f, 0.0f,
                                                  m_rightWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponBoomerang) {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f, -2.0f, -4.0f);
                } else if (weapon == WeaponFist) {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f,
                                                  m_rightWeaponSprite->sizeY() * -0.5f,
                                                  m_rightWeaponSprite->sizeZ() * -0.5f);
                } else {
                    m_rightWeaponMatrix.translate(m_rightWeaponSprite->sizeX() * -0.5f,
                                                  m_rightWeaponSprite->sizeY() * -0.5f, -4.0f);
                }
            }
        }
        shader->setMatrices(m_rightWeaponMatrix, view, projection);
        shader->setShininess(m_creature->m_entityData.equipment.weaponRight.shininess());
        m_rightWeaponSprite->draw();
        shader->setShininess(0.0f);
        if (m_creature->m_entityData.charge > 0.0f)
            shader->setMaterialColor(color);
        m_trails[1][0] = m_rightWeaponMatrix * plasma::Vector<3, float>(m_rightWeaponSprite->sizeX() * 0.5f,
                                                                        m_rightWeaponSprite->sizeY() * 0.5f, 0.0f);
        m_trails[0][0] = m_rightWeaponMatrix * plasma::Vector<3, float>(m_rightWeaponSprite->sizeX() * 0.5f,
                                                                        m_rightWeaponSprite->sizeY() * 0.5f,
                                                                        (float)m_rightWeaponSprite->sizeZ());
    }

    // --- The left weapon --- As the right one, on the other side of the back (a shield upright, bows
    // and crossbows steeper), but gliding alone does not put it there; torches stay in the hand.
    // Animation 71 holds it between the hands. Its tip and base start trails 2 and 3.
    if (m_leftWeaponSprite) {
        m_leftWeaponMatrix = m_handsMatrix;
        unsigned char currentAbility = m_creature->m_entityData.currentAbility;
        if ((currentAbility == AbilityNone || currentAbility == AbilityRangerKick ||
             currentAbility == AbilityPresentItem || currentAbility == AbilityRidePet ||
             currentAbility == AbilitySailBoat || currentAbility == AbilityTeleportToCity ||
             currentAbility == AbilityQuake || currentAbility == AbilityShuriken || currentAbility == AbilityDrink ||
             currentAbility == AbilityOfferPetFood || currentAbility == Ability85 || currentAbility == AbilitySneak ||
             currentAbility == AbilitySit || currentAbility == AbilityEat || currentAbility == AbilitySleep) &&
            m_creature->m_entityData.equipment.weaponLeft.id != WeaponTorch &&
            m_creature->m_entityData.equipment.weaponLeft.id != WeaponFist &&
            m_creature->m_entityData.equipment.weaponLeft.id != WeaponBracelet) {
            m_leftWeaponMatrix = m_bodyMatrix;
            m_leftWeaponMatrix.translate(0.5f, -0.65f, 0.5f);
            m_leftWeaponMatrix.rotateZ((float)((armSwing * 0.25f * 180.0f) / 3.141592653589793));
            unsigned char weapon = m_creature->m_entityData.equipment.weaponLeft.id;
            if (weapon == WeaponShield) {
                m_leftWeaponMatrix.translate(0.0f, -8.2f, 0.0f);
                m_leftWeaponMatrix.rotate(180.0f, 0.0f, 0.0f, 1.0f);
            } else if (weapon == WeaponBow || weapon == WeaponCrossbow) {
                m_leftWeaponMatrix.translate(0.0f, -6.2f, 0.0f);
                m_leftWeaponMatrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
                m_leftWeaponMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            } else {
                m_leftWeaponMatrix.translate(0.0f, -7.2f, 0.0f);
                m_leftWeaponMatrix.rotate(-225.0f, 0.0f, 1.0f, 0.0f);
                m_leftWeaponMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            }
            m_leftWeaponMatrix.scale(leftWeaponScale, leftWeaponScale, leftWeaponScale);
            m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f, m_leftWeaponSprite->sizeY() * -0.5f,
                                         m_leftWeaponSprite->sizeZ() * -0.5f);
            shader->setMatrices(m_leftWeaponMatrix, view, projection);
            shader->setShininess(m_creature->m_entityData.equipment.weaponLeft.shininess());
            m_leftWeaponSprite->draw();
            shader->setShininess(0.0f);
        } else {
            if (m_animation == AnimationThrowRock)
                m_leftWeaponMatrix.translate(0.5f * (leftHandPosition + rightHandPosition) +
                                             plasma::Vector<3, float>(0.0f, 0.0f, 8.0f));
            else
                m_leftWeaponMatrix.translate(leftHandPosition[0], leftHandPosition[1], leftHandPosition[2]);
            m_leftWeaponMatrix.rotateZ(leftHandAngles[2]);
            m_leftWeaponMatrix.rotateY(leftHandAngles[1]);
            m_leftWeaponMatrix.rotateX(leftHandAngles[0]);
            m_leftWeaponMatrix.rotate(leftWeaponTurn[2], 0.0f, 0.0f, 1.0f);
            m_leftWeaponMatrix.rotate(leftWeaponTurn[1], 0.0f, 1.0f, 0.0f);
            m_leftWeaponMatrix.rotate(m_creature->m_entityData.headRotation * 0.5f + leftWeaponTurn[0], 1.0f, 0.0f,
                                      0.0f);
            m_leftWeaponMatrix.scale(leftWeaponScale, leftWeaponScale, leftWeaponScale);
            if (m_leftWeaponSprite == spriteManager->sprite(SpriteArrow)) {
                m_leftWeaponMatrix.translate(1.0f - m_leftWeaponSprite->sizeX() * 0.5f, 0.5f, 1.0f);
            } else if (m_animation == AnimationThrowRock) {
                m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f, m_leftWeaponSprite->sizeY() * -0.5f,
                                             m_leftWeaponSprite->sizeZ() * -0.5f);
            } else {
                unsigned char weapon = m_creature->m_entityData.equipment.weaponLeft.id;
                if (weapon == WeaponBow) {
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f,
                                                 1.5f - m_leftWeaponSprite->sizeY(),
                                                 m_leftWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponCrossbow) {
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f, 0.0f,
                                                 m_leftWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponBracelet) {
                    m_leftWeaponMatrix.scale(0.6f, 0.6f, 0.6f);
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f,
                                                 (float)(1 - m_leftWeaponSprite->sizeY()),
                                                 m_leftWeaponSprite->sizeZ() * -0.5f);
                } else if (weapon == WeaponShield) {
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f, 3.0f,
                                                 m_leftWeaponSprite->sizeZ() * -0.5f);
                } else if (m_creature->m_entityData.equipment.weaponRight.id == WeaponBoomerang) {
                    // The right weapon's kind decides here.
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f, -2.0f, -4.0f);
                } else if (weapon == WeaponFist) {
                    m_leftWeaponMatrix.rotateY(180.0f);
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f,
                                                 m_leftWeaponSprite->sizeY() * -0.5f,
                                                 m_leftWeaponSprite->sizeZ() * -0.5f);
                } else {
                    m_leftWeaponMatrix.translate(m_leftWeaponSprite->sizeX() * -0.5f,
                                                 m_leftWeaponSprite->sizeY() * -0.5f, -4.0f);
                }
            }
            if (m_creature->m_entityData.charge > 0.0f)
                shader->setMaterialColor(color * ((cosf(m_animationTime * 0.01f) + 1.0f) * 0.25f + 1.0f));
            shader->setMatrices(m_leftWeaponMatrix, view, projection);
            shader->setShininess(m_creature->m_entityData.equipment.weaponLeft.shininess());
            m_leftWeaponSprite->draw();
            shader->setShininess(0.0f);
            if (m_creature->m_entityData.charge > 0.0f)
                shader->setMaterialColor(color);
        }
        m_trails[3][0] = m_leftWeaponMatrix * plasma::Vector<3, float>(m_leftWeaponSprite->sizeX() * 0.5f,
                                                                       m_leftWeaponSprite->sizeY() * 0.5f, 0.0f);
        m_trails[2][0] = m_leftWeaponMatrix * plasma::Vector<3, float>(m_leftWeaponSprite->sizeX() * 0.5f,
                                                                       m_leftWeaponSprite->sizeY() * 0.5f,
                                                                       (float)m_leftWeaponSprite->sizeZ());
    }

    // Arrows in the left hand: on the back, at the left shoulder.
    if (arrowsSprite) {
        plasma::Matrix<float> arrowsMatrix = m_handsMatrix;
        arrowsMatrix = m_bodyMatrix;
        arrowsMatrix.translate(0.65f, -0.5f, 0.3f);
        arrowsMatrix.rotateZ((float)((armSwing * 0.25f * 180.0f) / 3.141592653589793));
        arrowsMatrix.translate(0.0f, -6.2f, 0.0f);
        arrowsMatrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
        arrowsMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
        arrowsMatrix.scale(m_creature->m_entityData.appearance.weaponScale,
                           m_creature->m_entityData.appearance.weaponScale,
                           m_creature->m_entityData.appearance.weaponScale);
        arrowsMatrix.translate(arrowsSprite->sizeX() * -0.5f, arrowsSprite->sizeY() * -0.5f,
                               arrowsSprite->sizeZ() * -0.5f);
        shader->setMatrices(arrowsMatrix, view, projection);
        arrowsSprite->draw();
    }

    // --- The weapon trails --- The four trails' first points were just placed at the weapons, in
    // drawing space; this moves them by the creature's position against the origin, then the rest of
    // each trail follows: a circle for the spin of animation 69, a lag behind the first point while an
    // attack swings, else the first point repeated.
    plasma::Vector<3, float> shift((float)(-m_creature->m_drawnPosition[0] - m_originX),
                                   (float)(-m_creature->m_drawnPosition[1] - m_originY), 0.0f);
    m_trails[3][0] += shift;
    m_trails[2][0] += shift;
    m_trails[1][0] += shift;
    m_trails[0][0] += shift;
    if (m_animation == AnimationBlockStrike && m_animationTime > m_creature->anticipationTime() &&
        m_animationTime < m_creature->anticipationTime() + m_creature->strikeTime()) {
        // A circle around the creature that widens with the strike.
        plasma::Matrix<float> spinMatrix;
        spinMatrix.identity();
        spinMatrix.translate((float)(m_creature->m_drawnPosition[0] + originX),
                             (float)(m_creature->m_drawnPosition[1] + originY),
                             (float)(m_creature->m_drawnPosition[2] - m_creature->m_stepOffset + m_creatureOffset[2]));
        spinMatrix.rotateZ(m_creature->m_drawnRotation[2]);
        float outerTrailRadius = (float)(m_animationTime - m_creature->anticipationTime()) * 20.0f /
                                 (float)(m_creature->anticipationTime() + m_creature->strikeTime());
        float innerTrailRadius = outerTrailRadius * 0.5f;
        for (int i = 0; i < 16; i++) {
            float angle = (float)(i * 3.141592653589793 * 0.0625 - 1.5707963267948966);
            float cosine = cosf(angle);
            float sine = sinf(angle);
            m_trails[1][i] = spinMatrix * (innerTrailRadius * plasma::Vector<3, float>(sine, cosine, 0.0f)) + shift;
            m_trails[0][i] = spinMatrix * (outerTrailRadius * plasma::Vector<3, float>(sine, cosine, 0.0f)) + shift;
        }
    } else if ((m_animation == AnimationLeftSweep || m_animation == AnimationRightSweep ||
                m_animation == AnimationOverheadChop || m_animation == 14 || m_animation == 12 ||
                m_animation == AnimationBlockJab || m_animation == AnimationParry ||
                m_animation == AnimationSwirlStrike || m_animation == AnimationSmash ||
                m_animation == AnimationCrossedWeaponSweep || m_animation == AnimationTwoHandedSweep ||
                m_animation == AnimationTwoHandedThrust || m_animation == AnimationTwoHandedSpin ||
                m_animation == AnimationGuardianCrushStrike || m_animation == AnimationAmbush) &&
               m_animationTime > m_creature->anticipationTime() && m_animationTime < m_creature->attackDuration()) {
        // Both weapons swing: every point follows the one before it.
        float share = 0.0f;
        blend(share, 1.0f, milliseconds, 0.05f);
        for (int i = 1; i < 16; i++) {
            m_trails[1][i] += share * (m_trails[1][i - 1] - m_trails[1][i]);
            m_trails[0][i] += share * (m_trails[0][i - 1] - m_trails[0][i]);
            m_trails[3][i] += share * (m_trails[3][i - 1] - m_trails[3][i]);
            m_trails[2][i] += share * (m_trails[2][i - 1] - m_trails[2][i]);
        }
    } else if ((m_animation == AnimationLeftSlash || m_animation == AnimationRightBackhand ||
                m_animation == AnimationLeftStab) &&
               m_creature->m_entityData.timeSinceAbility > m_creature->anticipationTime()) {
        // The left weapon swings; the right one's trail collapses.
        float share = 0.0f;
        blend(share, 1.0f, milliseconds, 0.05f);
        m_trails[3][0] = m_trails[3][0];
        m_trails[2][0] += 0.0f * (m_trails[2][0] - m_trails[3][0]);
        for (int i = 1; i < 16; i++) {
            m_trails[1][i] = m_trails[1][0];
            m_trails[0][i] = m_trails[1][0];
            m_trails[3][i] += share * (m_trails[3][i - 1] - m_trails[3][i]);
            m_trails[2][i] += share * (m_trails[2][i - 1] - m_trails[2][i]);
        }
    } else if ((m_animation == AnimationRightSlash || m_animation == AnimationLeftBackhand ||
                m_animation == AnimationRightStab || m_animation == AnimationKick) &&
               m_creature->m_entityData.timeSinceAbility > m_creature->anticipationTime() &&
               m_creature->m_entityData.timeSinceAbility <
                   m_creature->holdTime() / 2 + m_creature->anticipationTime() + m_creature->strikeTime()) {
        // The right weapon swings (animation 50, the kick: from the foot).
        float share = 0.0f;
        blend(share, 1.0f, milliseconds, 0.075f);
        if (m_animation == AnimationKick) {
            m_trails[0][0] = creatureMatrix * plasma::Vector<3, float>(20.0f, 0.0f, -12.0f);
            m_trails[1][0] = creatureMatrix * plasma::Vector<3, float>(8.0f, 0.0f, -12.0f);
        } else {
            m_trails[1][0] = 0.5f * m_trails[1][0] + 0.5f * m_trails[0][0];
            m_trails[0][0] += 0.0f * (m_trails[0][0] - m_trails[1][0]);
        }
        for (int i = 1; i < 16; i++) {
            m_trails[1][i] += share * (m_trails[1][i - 1] - m_trails[1][i]);
            m_trails[0][i] += share * (m_trails[0][i - 1] - m_trails[0][i]);
            m_trails[3][i] = m_trails[3][0];
            m_trails[2][i] = m_trails[3][0];
        }
    } else {
        for (int i = 1; i < 16; i++) {
            m_trails[1][i] = m_trails[1][0];
            m_trails[0][i] = m_trails[0][0];
            m_trails[3][i] = m_trails[3][0];
            m_trails[2][i] = m_trails[2][0];
        }
    }
    shader->setMaterialColor(color);

    // --- The hands --- The left hand, with the lamp when it is lit; the right hand is the left sprite
    // mirrored (drawn with the opposite culling). A four-legged creature's hands swing like feet (the
    // other side's foot swing).
    if (m_handsSprite) {
        if (!(hiddenParts & (1 << HiddenCreatureLeftHand))) {
            m_leftHandMatrix = m_handsMatrix;
            if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped))
                m_leftHandMatrix.rotate(rightFootSwing, 1.0f, 0.0f, 0.0f);
            m_leftHandMatrix.translate(leftHandPosition[0], leftHandPosition[1], leftHandPosition[2]);
            m_leftHandMatrix.rotateZ(leftHandAngles[2]);
            m_leftHandMatrix.rotateY(leftHandAngles[1]);
            if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped))
                m_leftHandMatrix.rotate(rightFootSwing, 1.0f, 0.0f, 0.0f);
            m_leftHandMatrix.rotateX(leftHandAngles[0]);
            m_leftHandMatrix.scale(handsScale, handsScale, handsScale);
            m_leftHandMatrix.translate(m_handsSprite->sizeX() * -0.5f, m_handsSprite->sizeY() * -0.5f,
                                       m_handsSprite->sizeZ() * -0.5f);
            shader->setMatrices(m_leftHandMatrix, view, projection);
            shader->setShininess(m_creature->m_entityData.equipment.hands.shininess());
            m_handsSprite->draw();
            if (m_creature->m_entityData.binaryToggles.lampOn) {
                Sprite *lamp = spriteManager->itemSprite(&m_creature->m_entityData.equipment.lamp);
                if (lamp) {
                    plasma::Matrix<float> lampMatrix = m_handsMatrix;
                    lampMatrix.translate(leftHandPosition[0], leftHandPosition[1] + 2.0f, leftHandPosition[2] - 2.0f);
                    lampMatrix.scale(0.8f, 0.8f, 0.8f);
                    lampMatrix.translate(lamp->sizeX() * -0.5f, 1.0f - lamp->sizeY() * 0.5f,
                                         (float)(2 - lamp->sizeZ()));
                    shader->setMatrices(lampMatrix, view, projection);
                    shader->setShininess(0.0f);
                    shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    lamp->draw();
                    shader->setMaterialColor(color);
                }
            }
        }
        if (!(hiddenParts & (1 << HiddenCreatureRightHand))) {
            m_rightHandMatrix = m_handsMatrix;
            if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped))
                m_rightHandMatrix.rotate(leftFootSwing, 1.0f, 0.0f, 0.0f);
            m_rightHandMatrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            m_rightHandMatrix.rotateZ(rightHandAngles[2]);
            m_rightHandMatrix.rotateY(rightHandAngles[1]);
            if (m_creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagQuadruped))
                m_rightHandMatrix.rotate(leftFootSwing, 1.0f, 0.0f, 0.0f);
            m_rightHandMatrix.rotateX(rightHandAngles[0]);
            m_rightHandMatrix.scale(-handsScale, handsScale, handsScale);
            m_rightHandMatrix.translate(m_handsSprite->sizeX() * -0.5f, m_handsSprite->sizeY() * -0.5f,
                                        m_handsSprite->sizeZ() * -0.5f);
            shader->getDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            shader->setMatrices(m_rightHandMatrix, view, projection);
            m_handsSprite->draw();
            shader->getDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        }
        shader->setShininess(0.0f);
    }

    // --- The wings --- Flapping with the walk cycle, mirrored by the flap's sign.
    if (m_wingsSprite) {
        float flap = cosf(m_creature->m_walkPhase * 0.5f) * (m_creature->m_walkAmplitude * 90.0f) - 45.0f;
        m_leftWingMatrix = m_handsMatrix;
        m_leftWingMatrix.translate(-m_creature->m_entityData.appearance.wingsPosition[0],
                                   m_creature->m_entityData.appearance.wingsPosition[1],
                                   m_creature->m_entityData.appearance.wingsPosition[2]);
        m_leftWingMatrix.rotateX(m_creature->m_entityData.appearance.wingsRotation);
        m_leftWingMatrix.rotateY(flap);
        m_leftWingMatrix.rotateZ(-90.0f);
        m_leftWingMatrix.rotateX(90.0f);
        m_leftWingMatrix.scale(m_creature->m_entityData.appearance.wingScale,
                               m_creature->m_entityData.appearance.wingScale,
                               m_creature->m_entityData.appearance.wingScale);
        m_leftWingMatrix.translate(m_wingsSprite->sizeX() * -0.5f, 0.0f, m_wingsSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_leftWingMatrix, view, projection);
        m_wingsSprite->draw();
        m_rightWingMatrix = m_handsMatrix;
        m_rightWingMatrix.translate(m_creature->m_entityData.appearance.wingsPosition[0],
                                    m_creature->m_entityData.appearance.wingsPosition[1],
                                    m_creature->m_entityData.appearance.wingsPosition[2]);
        m_rightWingMatrix.rotateX(m_creature->m_entityData.appearance.wingsRotation);
        m_rightWingMatrix.rotateY(-flap);
        m_rightWingMatrix.rotateZ(-90.0f);
        m_rightWingMatrix.rotateX(90.0f);
        m_rightWingMatrix.scale(m_creature->m_entityData.appearance.wingScale,
                                m_creature->m_entityData.appearance.wingScale,
                                m_creature->m_entityData.appearance.wingScale);
        m_rightWingMatrix.translate(m_wingsSprite->sizeX() * -0.5f, 0.0f, m_wingsSprite->sizeZ() * -0.5f);
        shader->setMatrices(m_rightWingMatrix, view, projection);
        m_wingsSprite->draw();
    }

    // --- The feet --- On the creature's matrix (not the body's), turned with the body turn, swung and
    // rolled by the walk cycle and lifted by the stride.
    if (m_feetSprite) {
        if (!(hiddenParts & (1 << HiddenCreatureLeftFoot))) {
            m_leftFootMatrix = creatureMatrix;
            m_leftFootMatrix.rotateZ(m_bodyTurn);
            float leftSwing = feetTurn + leftFootSwing;
            m_leftFootMatrix.rotate(m_creature->m_entityData.appearance.feetRotation + leftSwing, 1.0f, 0.0f, 0.0f);
            m_leftFootMatrix.rotateY(footRoll);
            m_leftFootMatrix.translate(leftFootPosition[0], leftFootPosition[1],
                                       strideSize * 3.0f + leftFootPosition[2] + leftFootLift);
            if (m_creature->m_entityData.currentAbility == AbilitySneak)
                m_leftFootMatrix.rotateX(leftFootSwing * -1.5f);
            m_leftFootMatrix.scale(feetScale, feetScale, feetScale);
            if (leftFootTurnedOut)
                m_leftFootMatrix.rotateZ(60.0f);
            else
                m_leftFootMatrix.rotate(leftSwing, 1.0f, 0.0f, 0.0f);
            m_leftFootMatrix.translate(m_feetSprite->sizeX() * -0.5f, m_feetSprite->sizeY() * -0.5f,
                                       m_feetSprite->sizeZ() * -0.5f);
            shader->setMatrices(m_leftFootMatrix, view, projection);
            shader->setShininess(m_creature->m_entityData.equipment.feet.shininess());
            m_feetSprite->draw();
            shader->setShininess(0.0f);
        }
        if (!(hiddenParts & (1 << HiddenCreatureRightFoot))) {
            m_rightFootMatrix = creatureMatrix;
            m_rightFootMatrix.rotateZ(m_bodyTurn);
            float rightSwing = feetTurn + rightFootSwing;
            m_rightFootMatrix.rotate(rightSwing, 1.0f, 0.0f, 0.0f);
            m_rightFootMatrix.rotateY(-footRoll);
            m_rightFootMatrix.translate(rightFootPosition[0], rightFootPosition[1],
                                        strideSize * 3.0f + rightFootPosition[2] + rightFootLift);
            if (m_creature->m_entityData.currentAbility == AbilitySneak)
                m_rightFootMatrix.rotateX(rightFootSwing * -1.5f);
            m_rightFootMatrix.scale(feetScale, feetScale, feetScale);
            m_rightFootMatrix.rotate(m_creature->m_entityData.appearance.feetRotation + rightSwing, 1.0f, 0.0f, 0.0f);
            m_rightFootMatrix.translate(m_feetSprite->sizeX() * -0.5f, m_feetSprite->sizeY() * -0.5f,
                                        m_feetSprite->sizeZ() * -0.5f);
            shader->setMatrices(m_rightFootMatrix, view, projection);
            shader->setShininess(m_creature->m_entityData.equipment.feet.shininess());
            m_feetSprite->draw();
            shader->setShininess(0.0f);
        }
    }
}
#endif

}
