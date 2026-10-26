#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <math.h>
#include <string.h>

#include "cube/Noise.h"
#include "cube/MathConstants.h"
#include "cube/World.h"
#include "cube/Creature.h"
#include "cube/Fixed.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "gfx/SpriteBatch.h"
#include "cube/CreatureIds.h"
#include "cube/ItemIds.h"
#include "cube/SpriteIds.h"

namespace cube {

Creature::AnimationState::AnimationState(Creature *creature)
    : m_creature(creature),
      m_animationTime(0),
      m_animation(AnimationNone),
      m_emoteTime(0),
      m_emoteSprite(nullptr),
      m_leftHandOffset(0.0f, 0.0f, 0.0f),
      m_rightHandOffset(0.0f, 0.0f, 0.0f),
      m_leftHandRotation(0.0f, 0.0f, 0.0f),
      m_rightHandRotation(0.0f, 0.0f, 0.0f),
      m_unused50(0),
      m_unused58(0),
      m_unused60(0),
      m_bodyRotation(0.0f, 0.0f, 0.0f),
      m_creatureRotation(0.0f, 0.0f, 0.0f),
      m_bodyOffset(0.0f, 0.0f, 0.0f),
      m_headOffset(0.0f, 0.0f, 0.0f),
      m_headRotation(0.0f, 0.0f, 0.0f),
      m_tailRotation(0.0f, 0.0f, 0.0f),
      m_handsTurn(0.0f, 0.0f, 0.0f),
      m_bodyTurn(0.0f),
      m_creatureOffset(0.0f, 0.0f, 0.0f),
      m_feetTurn(0.0f),
      m_glideLift(0),
      m_headSprite(nullptr),
      m_hairSprite(nullptr),
      m_chestSprite(nullptr),
      m_shouldersSprite(nullptr),
      m_handsSprite(nullptr),
      m_wingsSprite(nullptr),
      m_tailSprite(nullptr),
      m_feetSprite(nullptr),
      m_rightWeaponSprite(nullptr),
      m_shoulderArmorSprite(nullptr),
      m_neckSprite(nullptr),
      m_scale(1.0f),
      m_blendedFacing(0.0f, 0.0f, 0.0f),
      m_spritesInitialized(0),
      m_originX(0),
      m_originY(0)
{
    memset(m_trails, 0, sizeof(m_trails));
}

float interpolateAngle(float angle, float target, float factor)
{
    float a = angle / 180.0f * Pi;
    double cosA = cosf(a);
    double sinA = sinf(a);
    float b = target / 180.0f * Pi;
    double cosB = cosf(b);
    double sinB = sinf(b);
    if (cosB * cosA + sinB * sinA < -0.9f)
        return target;
    float sine = sinB * cosA - cosB * sinA;
    if (sine > 1.0f)
        sine = 1.0f;
    if (sine < -1.0f)
        sine = -1.0f;
    float delta = asinf(sine) * (factor * 180.0 / Pi);
    return angle + delta;
}

// The Vector operators are global templates (plasma/Vector.h); name lookup from this namespace needs them named,
// because cube declares operators of its own.
using ::operator*;

void Creature::AnimationState::addSprites(gfx::SpriteBatch &batch, SpriteManager *spriteManager, float frameTime,
                                          const plasma::Vector<4, float> &color, float alpha, const Fixed &originX,
                                          const Fixed &originY, int hiddenParts, Creature *mount, float depthOffset,
                                          float scale)
{
    // --- The tower model (race 302) --- A stack of sprite 3635, one segment for every two units of attackRotation[2]
    // (the height the entity carries there), each two blocks above and a quarter turn from the one below, and a last,
    // partial segment scaled by the remainder. The creature's own model is drawn after it as usual.
    if (m_creature->m_entityData.race == RaceClimbingPlant) {
        float segments = m_creature->m_entityData.attackRotation[2] * 0.5f;
        int fullSegments = (int)segments;
        Sprite *segment = spriteManager->sprite(SpriteClimbingPlantStalk);
        Fixed base = m_creature->m_drawnPosition[2] - m_creature->m_entityData.appearance.size[2] * 0.5f;
        for (int i = 0; i < fullSegments; i++) {
            float scale = 3.0f / (float)segment->m_size[2];
            plasma::Matrix<float> matrix;
            matrix.identity();
            matrix.translate((m_creature->m_drawnPosition[0] + originX).toFloat(),
                             (m_creature->m_drawnPosition[1] + originY).toFloat(),
                             (base + Fixed((long long)i * 0x20000)).toFloat());
            matrix.rotateZ((float)(i * 90) + m_creature->m_drawnRotation[2]);
            matrix.scale(scale, scale, scale);
            matrix.translate((float)segment->m_size[0] * -0.5f, (float)segment->m_size[1] * -0.5f, 0.0f);
            batch.add(segment, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), alpha, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        float scale = 3.0f / (float)segment->m_size[2] * (segments - (float)fullSegments);
        plasma::Matrix<float> matrix;
        matrix.identity();
        matrix.translate((m_creature->m_drawnPosition[0] + originX).toFloat(),
                         (m_creature->m_drawnPosition[1] + originY).toFloat(),
                         (base + Fixed((long long)(fullSegments * 2 << FixedFractionBits))).toFloat());
        matrix.rotateZ((float)(fullSegments * 90) + m_creature->m_drawnRotation[2]);
        matrix.scale(scale, scale, scale);
        matrix.translate((float)segment->m_size[0] * -0.5f, (float)segment->m_size[1] * -0.5f, 0.0f);
        batch.add(segment, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), alpha, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    m_spritesInitialized = 1;
    m_originX = originX;
    m_originY = originY;

    // --- The sprites --- Equipment the creature can wear replaces the body's own model of that part; a hand shows no
    // weapon while the other hand holds a two-handed one. The head, the hair, the shoulders, the tail and the wings
    // always come from the appearance.
    m_headSprite = nullptr;
    m_hairSprite = nullptr;
    m_shouldersSprite = nullptr;
    // The item of the first equipment slot, drawn on the head with the hair's matrix (see "head and hair").
    Sprite *slot0Sprite = nullptr;
    if (m_creature->canEquipItem(m_creature->m_entityData.equipment.head))
        slot0Sprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.head);
    m_handsSprite = m_creature->canEquipItem(m_creature->m_entityData.equipment.hands)
                        ? spriteManager->itemSprite(&m_creature->m_entityData.equipment.hands)
                        : nullptr;
    m_feetSprite = m_creature->canEquipItem(m_creature->m_entityData.equipment.feet)
                       ? spriteManager->itemSprite(&m_creature->m_entityData.equipment.feet)
                       : nullptr;
    m_shoulderArmorSprite = m_creature->canEquipItem(m_creature->m_entityData.equipment.shoulder)
                                ? spriteManager->itemSprite(&m_creature->m_entityData.equipment.shoulder)
                                : nullptr;
    m_neckSprite = m_creature->canEquipItem(m_creature->m_entityData.equipment.neck)
                       ? spriteManager->itemSprite(&m_creature->m_entityData.equipment.neck)
                       : nullptr;
    if (m_creature->canEquipItem(m_creature->m_entityData.equipment.weaponRight) &&
        !m_creature->m_entityData.equipment.weaponLeft.isTwoHanded())
        m_rightWeaponSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponRight);
    else
        m_rightWeaponSprite = nullptr;
    if (m_creature->canEquipItem(m_creature->m_entityData.equipment.weaponLeft) &&
        !m_creature->m_entityData.equipment.weaponRight.isTwoHanded())
        m_leftWeaponSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponLeft);
    else
        m_leftWeaponSprite = nullptr;
    m_chestSprite = m_creature->canEquipItem(m_creature->m_entityData.equipment.chest)
                        ? spriteManager->itemSprite(&m_creature->m_entityData.equipment.chest)
                        : nullptr;
    m_tailSprite = nullptr;
    m_wingsSprite = nullptr;
    if (m_headSprite == nullptr) {
        m_headSprite = spriteManager->sprite(m_creature->m_entityData.appearance.headModel);
        m_hairSprite = spriteManager->sprite(m_creature->m_entityData.appearance.hairModel);
    }
    if (m_handsSprite == nullptr)
        m_handsSprite = spriteManager->sprite(m_creature->m_entityData.appearance.handsModel);
    if (m_feetSprite == nullptr)
        m_feetSprite = spriteManager->sprite(m_creature->m_entityData.appearance.feetModel);
    if (m_chestSprite == nullptr)
        m_chestSprite = spriteManager->sprite(m_creature->m_entityData.appearance.chestModel);
    if (m_tailSprite == nullptr)
        m_tailSprite = spriteManager->sprite(m_creature->m_entityData.appearance.tailModel);
    if (m_wingsSprite == nullptr)
        m_wingsSprite = spriteManager->sprite(m_creature->m_entityData.appearance.wingsModel);
    if (m_shouldersSprite == nullptr)
        m_shouldersSprite = spriteManager->sprite(m_creature->m_entityData.appearance.shoulderModel);

    // --- The rest pose, from the appearance --- Positions are in the model's space: x to the creature's side (the
    // left parts take the negative x and the right parts, drawn mirrored, the positive), y forwards, z up. The
    // animation cases change copies.
    float headScale = m_creature->m_entityData.appearance.headScale;
    float chestScale = m_creature->m_entityData.appearance.chestScale;
    float handsScale = m_creature->m_entityData.appearance.handsScale;
    float shouldersScale = m_creature->m_entityData.appearance.shoulderModelScale;
    float rightWeaponScale = m_creature->m_entityData.appearance.weaponScale;
    float leftWeaponScale = m_creature->m_entityData.appearance.weaponScale;
    float feetScale = m_creature->m_entityData.appearance.feetScale;
    float tailScale = m_creature->m_entityData.appearance.tailScale;
    float shoulderArmorScale = m_creature->m_entityData.appearance.shoulderArmorScale;
    plasma::Vector<3, float> chestPosition = m_creature->m_entityData.appearance.chestPosition;
    plasma::Vector<3, float> headPosition = m_creature->m_entityData.appearance.headPosition;
    // The hands' own rotation applies only to the idle animation; the left hand's is mirrored.
    plasma::Vector<3, float> rightHandAngles(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftHandAngles(0.0f, 0.0f, 0.0f);
    if (m_animation == AnimationNone) {
        rightHandAngles = m_creature->m_entityData.appearance.handsRotation;
        leftHandAngles = plasma::Vector<3, float>(m_creature->m_entityData.appearance.handsRotation[0],
                                                  -m_creature->m_entityData.appearance.handsRotation[1],
                                                  -m_creature->m_entityData.appearance.handsRotation[2]);
    }

    // --- The walk cycle --- m_walkAmplitude is the cycle's amplitude and m_walkPhase its phase (Creature.h). `swing`
    // is the angle, in radians, by which the legs swing; `armSwing` is the same angle damped for what the hands are
    // doing.
    const float &amplitude = m_creature->m_walkAmplitude;
    const unsigned char &ability = m_creature->m_entityData.currentAbility;
    float cycle = sinf((float)((double)m_creature->m_walkPhase + HalfPi));
    float armSwing = (float)((double)amplitude * 2.199114820062152 * (double)cycle);
    if (ability == AbilitySneak ||
        (ability == AbilityClimb && (m_creature->m_entityData.flags & CreatureFlagOnGround) != 0))
        armSwing *= 0.5f;
    if (ability >= AbilityRidePet && ability <= AbilitySailBoat && ability != AbilityHangGlide)
        armSwing *= 0.1f;
    // AbilityClimb with CreatureFlagAtWall: the hands move up and down in turn (below) and are raised (after the
    // switch).
    bool handsAlternate = ability == AbilityClimb && (m_creature->m_entityData.flags & CreatureFlagAtWall) != 0;
    // Leaning forward at speed: toggle 6 while accelerating and moving, or abilities 50 and 82.
    bool leaningForward;
    if (!handsAlternate && m_creature->m_entityData.binaryToggles.fastMovement &&
        m_creature->m_entityData.acceleration.squaredLength() > 0.1f &&
        m_creature->m_entityData.velocity[0] * m_creature->m_entityData.velocity[0] +
                m_creature->m_entityData.velocity[1] * m_creature->m_entityData.velocity[1] >
            0.1f)
        leaningForward = true;
    else
        leaningForward = ability == AbilityIntercept || ability == AbilityDash;
    if (leaningForward)
        armSwing *= 1.25f;
    if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) != 0 &&
        (m_creature->m_entityData.flags & CreatureFlagOnGround) == 0)
        armSwing = 0.0f;
    if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
        armSwing *= 0.6f;
    float swing = armSwing;
    if (m_creature->m_entityData.equipment.weaponRight.id == WeaponLongsword && ability != AbilityNone)
        armSwing *= 0.25f;
    if ((m_animation == AnimationDrink || m_animation == AnimationEat) &&
        (float)m_creature->m_entityData.actionItem.cooldown() > m_creature->m_entityData.timeSinceAbility)
        armSwing *= 0.1f;
    if (m_animation == AnimationOfferPetFood || m_animation == AnimationCarryCreature)
        armSwing *= 0.1f;
    if (((m_animation >= AnimationPlayFlute && m_animation <= AnimationPlayNightingale) ||
         m_animation == AnimationCallPet) &&
        5000.0f > m_animationTime)
        armSwing *= 0.1f;
    if (ability == AbilityBlock || ability == AbilityShoot || ability == AbilitySupportRay ||
        ability == AbilityFireballCharge || ability == AbilitySwirl)
        armSwing *= 0.5f;
    else if (m_animation == AnimationTwoHandedSweep || m_animation == AnimationTwoHandedThrust ||
             m_animation == AnimationTwoHandedSwing || m_animation == AnimationTwoHandedSpin ||
             m_animation == AnimationGuardianCrushStrike)
        armSwing *= 0.5f;
    if (ability == AbilityHangGlide)
        armSwing = 0.0f;

    // --- The target pose --- What the animation asks for; the state's pose members move towards these after the
    // switch.
    plasma::Vector<3, float> rightHandRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftHandRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> leftHandOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> rightHandOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> bodyRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> creatureRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> bodyOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> headRotation(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> creatureOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> headOffset(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> handsTurn(0.0f, 0.0f, 0.0f);
    float tailRotationX = 0.0f;
    float tailRotationZ = 0.0f;
    float bodyTurn = 0.0f;
    float feetTurn = 0.0f;
    float glideLift = 0.0f;
    if (m_creature->isGliding())
        glideLift = m_creature->m_entityData.appearance.size[2] * 0.5f;
    // With toggle 2 the body turns from the facing direction (attackRotation) towards the acceleration: the angle
    // between the two in the ground plane, in degrees, reversed when they point apart.
    if (m_creature->m_entityData.binaryToggles.faceAim) {
        float pushSquared = m_creature->m_entityData.acceleration[0] * m_creature->m_entityData.acceleration[0] +
                            m_creature->m_entityData.acceleration[1] * m_creature->m_entityData.acceleration[1];
        float facingSquared = m_creature->m_entityData.attackRotation[0] * m_creature->m_entityData.attackRotation[0] +
                              m_creature->m_entityData.attackRotation[1] * m_creature->m_entityData.attackRotation[1];
        if (pushSquared > 5.0f && facingSquared > 0.0f) {
            float pushScale = 1.0f / std::sqrt(pushSquared);
            float facingScale = 1.0f / std::sqrt(facingSquared);
            float pushX = m_creature->m_entityData.acceleration[0] * pushScale;
            float pushY = m_creature->m_entityData.acceleration[1] * pushScale;
            float facingX = m_creature->m_entityData.attackRotation[0] * facingScale;
            float facingY = m_creature->m_entityData.attackRotation[1] * facingScale;
            float sine = std::max(-1.0f, facingX * pushY - facingY * pushX);
            sine = std::min(1.0f, sine);
            bodyTurn = (float)((double)asinf(sine) * DegreesPerRadian);
            if (-0.2f > facingX * pushX + facingY * pushY)
                bodyTurn = -bodyTurn;
        }
    }

    // --- The feet's part of the walk cycle --- The swing of each foot about the x axis in degrees (the left foot
    // forwards while the right is back; the hands of a four-legged creature swing with the opposite foot), the feet's
    // roll about y and how far each foot is lifted.
    float footRoll = (float)((double)(swing * 0.0f * 80.0f) / Pi);
    float leftFootSwing = (float)((double)(swing * 180.0f) / Pi);
    float rightFootSwing = -leftFootSwing;
    if (leftFootSwing > 0.0f)
        leftFootSwing *= 1.5f;
    if (rightFootSwing > 0.0f)
        rightFootSwing *= 1.5f;
    float rightFootLift = (1.0f - cycle) * (amplitude * 4.0f);
    float leftFootLift = amplitude * 4.0f * cycle;
    if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) != 0 &&
        (m_creature->m_entityData.flags & CreatureFlagOnGround) == 0) {
        leftFootLift = 0.0f;
        rightFootLift = 0.0f;
    }
    if (0.0f > leftFootSwing)
        leftFootSwing *= 1.5f;
    if (0.0f > rightFootSwing)
        rightFootSwing *= 1.5f;
    if ((m_animation >= AnimationSit && m_animation <= AnimationSailBoat) || m_animation == AnimationEat ||
        ability == AbilityEagleFlight || ability == AbilityMenuView) {
        footRoll = 10.0f;
        leftFootSwing = 40.0f;
        rightFootSwing = 40.0f;
    }
    if (m_animation == AnimationAssassinate) {
        leftFootSwing = 30.0f;
        rightFootSwing = -10.0f;
    }
    if (m_animation == AnimationSleep) {
        leftFootSwing = -10.0f;
        rightFootSwing = -10.0f;
    }
    if (m_animation == AnimationTortureRackPose) {
        leftFootSwing = -40.0f;
        rightFootSwing = -40.0f;
        footRoll = 5.0f;
    }

    // --- The hands, the shoulders, the feet and the tail at rest --- A creature with AppearanceFlagQuadruped keeps
    // its hands where the appearance puts them. Otherwise the hands swing about the body's axis with the walk cycle,
    // hang lower and come up and in as the cycle grows.
    plasma::Vector<3, float> leftHandPosition;
    plasma::Vector<3, float> rightHandPosition;
    if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0) {
        leftHandPosition = plasma::Vector<3, float>(-m_creature->m_entityData.appearance.handsPosition[0],
                                                    m_creature->m_entityData.appearance.handsPosition[1],
                                                    m_creature->m_entityData.appearance.handsPosition[2]);
        rightHandPosition = m_creature->m_entityData.appearance.handsPosition;
    } else {
        float cosine = cosf(armSwing);
        float sine = sinf(armSwing);
        float reach = m_creature->m_entityData.appearance.handsPosition[0] * amplitude +
                      m_creature->m_entityData.appearance.handsPosition[0];
        float height = m_creature->m_entityData.appearance.handsPosition[2] - 4.1f + amplitude * 6.0f;
        leftHandPosition = plasma::Vector<3, float>(
            -(cosine * reach), -(sine * reach) + m_creature->m_entityData.appearance.handsPosition[1], height);
        rightHandPosition = plasma::Vector<3, float>(
            cosine * reach, sine * reach + m_creature->m_entityData.appearance.handsPosition[1], height);
        if (handsAlternate) {
            float sine2 = sinf((float)((double)amplitude * Pi * (double)cycle));
            leftHandPosition[2] = height + sine2 * 8.0f;
            rightHandPosition[2] = height - sine2 * 8.0f;
        }
    }
    leftHandPosition[1] += 2.1f - amplitude * 5.0f;
    rightHandPosition[1] += 2.1f - amplitude * 5.0f;
    // The shoulders lie between the hand and a point beside the neck (averaged again after the switch).
    plasma::Vector<3, float> leftShoulderPosition(
        (m_creature->m_entityData.appearance.handsPosition[0] * -0.3f + leftHandPosition[0]) * 0.5f,
        (leftHandPosition[1] - 8.0f) * 0.5f * 0.5f,
        (leftHandPosition[2] + (m_creature->m_entityData.appearance.handsPosition[2] + 12.0f)) * 0.5f);
    plasma::Vector<3, float> rightShoulderPosition(
        (rightHandPosition[0] + m_creature->m_entityData.appearance.handsPosition[0] * 0.3f) * 0.5f,
        (float)((double)((rightHandPosition[1] - 8.0f) * 0.5f) * 0.5),
        (rightHandPosition[2] + (m_creature->m_entityData.appearance.handsPosition[2] + 12.0f)) * 0.5f);
    plasma::Vector<3, float> leftFootPosition(-m_creature->m_entityData.appearance.feetPosition[0],
                                              m_creature->m_entityData.appearance.feetPosition[1],
                                              m_creature->m_entityData.appearance.feetPosition[2]);
    plasma::Vector<3, float> rightFootPosition = m_creature->m_entityData.appearance.feetPosition;
    plasma::Vector<3, float> tailPosition = m_creature->m_entityData.appearance.tailPosition;
    Sprite *quiverSprite = nullptr;
    if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        m_creature->m_entityData.equipment.weaponLeft.id == WeaponArrows) {
        quiverSprite = spriteManager->itemSprite(&m_creature->m_entityData.equipment.weaponLeft);
        m_leftWeaponSprite = nullptr;
    }
    // The rate at which the state's pose follows the target (blend's rate); some animations change it.
    float blendRate = 0.01f;
    // Set by ability 82: the left foot is turned sideways instead of swinging.
    bool leftFootSideways = false;

    // --- Toggle 10: the creature leans far forward with the hands in front ---
    if (m_creature->m_entityData.binaryToggles.leanForwardPose) {
        creatureRotation = plasma::Vector<3, float>(-70.0f, 0.0f, 0.0f);
        headOffset = plasma::Vector<3, float>(0.0f, -1.0f, 0.0f);
        headRotation[0] = 70.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, 1.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, 1.0f);
        creatureOffset[2] =
            (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        if (m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow)
            rightHandOffset[1] = 11.0f;
        leftFootSwing = -20.0f;
        rightFootSwing = -20.0f;
        if (ability != AbilityNone && (float)m_creature->attackDuration() > m_creature->m_entityData.timeSinceAbility) {
            leftHandOffset += plasma::Vector<3, float>(0.0f, -5.0f, 6.0f);
            rightHandOffset += plasma::Vector<3, float>(0.0f, -5.0f, 6.0f);
        }
    }

    // Each animation sets target poses, and sometimes rest positions, scales or sprites, from its elapsed time.
    // Ability animations use anticipationTime, strikeTime and attackDuration to separate wind-up, strike and recovery.
    switch (m_animation) {
    // Animation 62: poses the hands, the body, the head, the whole creature and the feet.
    case AnimationSit: {
        headOffset[0] = 0.0f;
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0) {
            bodyRotation[0] = 30.0f;
            leftHandOffset[0] = -1.0f;
            leftHandOffset[1] = -7.0f;
            headOffset[1] += 1.0f;
            leftFootPosition[2] += 2.0f;
            rightFootPosition[2] += 2.0f;
            leftHandOffset[2] = 0.0f;
            rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
            leftHandRotation[0] = -100.0f;
            rightHandRotation[0] = -100.0f;
            headOffset[2] = -4.0f;
        } else {
            headOffset[1] = 2.0f;
            headOffset[2] = -2.0f;
            bodyRotation[0] = 10.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
            creatureOffset[2] = -(((m_creature->m_entityData.appearance.chestPosition[2] + 5.0f) / 140.0f + 0.075f) *
                                  m_creature->m_entityData.appearance.size[2]);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        }
        break;
    }
    // Animation 78: poses the hands, the body and the head by fixed times.
    case AnimationPresentItem: {
        if (200.0f > m_animationTime) {
            bodyRotation[0] = -20.0f;
            headRotation[0] = -10.0f;
            leftHandOffset = plasma::Vector<3, float>(4.0f, 5.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-4.0f, 5.0f, 0.0f);
        } else if (5000.0f > m_animationTime) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = 30.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
        }
        break;
    }
    // Animation 79: poses the hands, the body, the head, the whole creature and the feet by fixed times; with a motion
    // that cycles with the animation time.
    case Animation_79: {
        if (!(200.0f > m_animationTime)) {
            if (5000.0f > m_animationTime) {
                bodyRotation[0] = 20.0f;
                headRotation[0] = 30.0f;
                leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
                rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 16.0f);
                float cosineWave = cosf(m_animationTime * 0.003f);
                creatureOffset[2] = (float)((double)cosineWave * 0.2 + 0.800000011920929);
                feetTurn = -30.0f;
                rightFootPosition[1] += 4.0f;
                leftFootPosition[1] += 4.0f;
            }
        } else {
            bodyRotation[0] = -20.0f;
            headRotation[0] = -10.0f;
            leftHandOffset = plasma::Vector<3, float>(4.0f, 5.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-4.0f, 5.0f, 0.0f);
        }
        break;
    }
    // Animation 27: poses the whole creature and the feet; with a motion that cycles with the animation time.
    case AnimationFloat: {
        float cosineWave = cosf(m_animationTime * 0.003f);
        creatureOffset[2] = (float)((double)cosineWave * 0.2 + 0.800000011920929);
        feetTurn = -30.0f;
        rightFootPosition[1] += 4.0f;
        leftFootPosition[1] += 4.0f;
        break;
    }
    // Animation 99: poses the hands, the body and the head by fixed times; with a motion that cycles with the
    // animation time.
    case AnimationFeedClimbingPlant: {
        if (2000.0f > m_animationTime) {
            bodyRotation[0] = -40.0f;
            headRotation[0] = -30.0f;
            bodyOffset[1] += 2.0f;
            bodyOffset[2] -= 3.0f;
            float cosine = cosf(m_animationTime * 0.01f);
            bodyRotation[2] = cosine * 5.0f;
            float cosine2 = cosf(m_animationTime * 0.02f);
            leftHandOffset = plasma::Vector<3, float>(4.0f, 8.0f, cosine2 * 2.0f + 6.0f);
            float cosine3 = cosf((float)((double)(m_animationTime * 0.02f) + Pi));
            rightHandOffset = plasma::Vector<3, float>(-4.0f, 8.0f, cosine3 * 2.0f + 6.0f);
        }
        break;
    }
    // Animation 81: poses the hands and the body.
    case AnimationCarryCreature: {
        bodyRotation[0] = 20.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 12.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 12.0f);
        break;
    }
    // Animation 82: poses the hands and the body by fixed times.
    case AnimationThrowCreature: {
        if (1000.0f > m_animationTime) {
            bodyRotation[0] = -30.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 8.0f, 3.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 8.0f, 3.0f);
        }
        break;
    }
    // Animation 63: poses the hands, the body, the head, the whole creature and the glide lift; seats the creature on
    // its mount.
    case AnimationRide: {
        if (mount != nullptr) {
            creatureOffset[1] -= 0.25f;
            creatureOffset[2] += (mount->m_entityData.appearance.size[2] * 0.75f + leftFootLift * 0.3f);
            plasma::Vector<3, float> vector = riderOffset(mount->m_entityData.race);
            float offsetY = creatureOffset[1];
            creatureOffset[0] += vector[0];
            float offsetZ = creatureOffset[2];
            creatureOffset[1] = offsetY + vector[1];
            creatureOffset[2] = offsetZ + vector[2];
            float liftedZ;
            if (mount->m_entityData.race == RaceAlpaca || mount->m_entityData.race == RaceBrownAlpaca) {
                creatureOffset[2] = offsetZ + vector[2] - 0.75f;
                liftedZ = offsetZ + vector[2] - 0.75f;
            } else {
                liftedZ = offsetZ + vector[2];
            }
            glideLift = -liftedZ;
        }
        headOffset[1] = 2.0f;
        headOffset[2] = -2.0f;
        rightHandRotation[2] = -60.0f;
        headOffset[0] = 0.0f;
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, -3.0f);
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, -3.0f);
        bodyRotation[0] = 20.0f;
        leftHandRotation = plasma::Vector<3, float>(60.0f, 90.0f, 60.0f);
        rightHandRotation[0] = 60.0f;
        rightHandRotation[1] = -90.0f;
        break;
    }
    // Animation 64: poses the hands, the body, the head and the whole creature.
    case AnimationSailBoat: {
        float seatZ;
        if ((m_creature->m_entityData.flags & CreatureFlagOnGround) != 0) {
            seatZ = creatureOffset[2] + 0.85f;
        } else {
            seatZ = creatureOffset[2] + 1.25f;
        }
        creatureRotation[2] = -50.0f;
        creatureOffset[2] = seatZ;
        headOffset[1] = 1.0f;
        headOffset[2] = 0.0f;
        headRotation = plasma::Vector<3, float>(0.0f, 0.0f, 30.0f);
        rightHandRotation[2] = -60.0f;
        headOffset[0] = 0.0f;
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, -3.0f);
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, -3.0f);
        bodyRotation[0] = 20.0f;
        leftHandRotation = plasma::Vector<3, float>(60.0f, 90.0f, 60.0f);
        rightHandRotation[0] = 60.0f;
        rightHandRotation[1] = -90.0f;
        break;
    }
    // Animation 56: poses the hands, the head and the whole creature.
    case AnimationSleep: {
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0) {
            creatureRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[0] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        } else {
            creatureRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -1.0f, 0.0f);
            headRotation[0] = 20.0f;
            leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f -
                0.15f;
        }
        break;
    }
    // Animation 57: poses the whole creature.
    case AnimationLieDown: {
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) == 0) {
            creatureRotation = plasma::Vector<3, float>(90.0f, 0.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f -
                0.1f;
        } else {
            creatureRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[0] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        }
        break;
    }
    // Animation 58: poses the hands, the whole creature and the feet.
    case AnimationTortureRackPose: {
        creatureRotation = plasma::Vector<3, float>(90.0f, 0.0f, 0.0f);
        leftFootPosition[1] += 4.0f;
        rightFootPosition[1] += 4.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, -4.0f, 11.0f);
        rightHandOffset[0] = 0.0f;
        rightHandOffset[1] = -4.0f;
        creatureOffset[2] =
            (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        rightHandOffset[2] = 11.0f;
        break;
    }
    // Animation 53: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationEat: {
        creatureOffset[2] = -0.14f;
        bodyRotation[0] = 10.0f;
        leftHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 0.0f);
        headOffset = plasma::Vector<3, float>(0.0f, 2.0f, -2.0f);
        if ((float)m_creature->m_entityData.actionItem.cooldown() > m_creature->m_entityData.timeSinceAbility) {
            float cosineWave = cosf(m_animationTime * 0.02f);
            headRotation[1] = 0.0f;
            headRotation[2] = 0.0f;
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            leftHandOffset[0] = 1.0f;
            leftHandOffset[1] = 7.0f;
            headRotation[0] = cosineWave * 5.0f + 45.0f;
            leftHandOffset[2] = 8.0f;
            rightHandOffset = plasma::Vector<3, float>(-1.0f, 7.0f, 8.0f);
            leftHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
        }
        break;
    }
    // Animation 52: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationDrink: {
        if ((float)m_creature->m_entityData.actionItem.cooldown() > m_creature->m_entityData.timeSinceAbility) {
            float cosineWave = cosf(m_animationTime * 0.02f);
            headRotation[1] = 0.0f;
            headRotation[2] = 0.0f;
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(10.0f, 0.0f, 0.0f);
            leftHandOffset[0] = 1.0f;
            leftHandOffset[1] = 7.0f;
            headRotation[0] = cosineWave * 5.0f + 45.0f;
            leftHandOffset[2] = 8.0f;
            rightHandOffset = plasma::Vector<3, float>(-1.0f, 7.0f, 8.0f);
            leftHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(100.0f, 0.0f, 0.0f);
        }
        break;
    }
    // Animation 54: poses the hands and the body.
    case AnimationOfferPetFood: {
        bodyRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
        leftHandOffset = plasma::Vector<3, float>(4.0f, 6.0f, 6.0f);
        rightHandOffset = plasma::Vector<3, float>(-4.0f, 6.0f, 6.0f);
        bodyOffset[1] = 3.0f;
        break;
    }
    // Animation 55: poses the hands, the body, the head, the whole creature and the feet.
    case AnimationSneak: {
        if (m_creature->m_entityData.acceleration[0] * m_creature->m_entityData.acceleration[0] +
                m_creature->m_entityData.acceleration[1] * m_creature->m_entityData.acceleration[1] +
                m_creature->m_entityData.acceleration[2] * m_creature->m_entityData.acceleration[2] >
            0.0f) {
            creatureRotation[0] = 20.0f;
            bodyRotation[0] = -10.0f;
            leftHandOffset[1] += 4.0f;
            rightHandOffset[1] += 4.0f;
            headRotation[0] = -20.0f;
            leftHandRotation[0] = -20.0f;
            rightHandRotation[0] = -20.0f;
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) == 0) {
                feetTurn = -40.0f;
                float rightFootHeight = rightFootPosition[2] + 3.0f;
                rightFootPosition[1] += 4.0f;
                leftFootPosition[2] += 3.0f;
                rightFootPosition[2] = rightFootHeight;
                leftFootPosition[1] += 4.0f;
            }
        }
        break;
    }
    // Animation 59: poses the hands, the whole creature and the feet.
    case AnimationPolePose: {
        feetTurn = -30.0f;
        leftHandPosition = plasma::Vector<3, float>(-1.0f, -3.0f, -3.0f);
        rightHandPosition = plasma::Vector<3, float>(1.0f, -3.0f, -3.0f);
        creatureOffset[2] = 0.3f;
        rightFootPosition[1] += 4.0f;
        leftFootPosition[1] += 4.0f;
        break;
    }
    // Animation 60: poses the hands, the body, the whole creature and the feet; with a motion that cycles with the
    // animation time.
    case AnimationCeilingChainPose: {
        feetTurn = -30.0f;
        creatureRotation[0] = 178.0f;
        leftHandPosition = plasma::Vector<3, float>(-4.0f, 0.0f, 12.0f);
        rightHandPosition = plasma::Vector<3, float>(4.0f, 0.0f, 12.0f);
        float cosine = cosf(m_animationTime * 0.005f);
        rightFootPosition[1] += 4.0f;
        leftFootPosition[1] += 4.0f;
        bodyRotation[1] = cosine * 10.0f;
        break;
    }
    // Animation 76: poses the whole creature by the ability's anticipation and strike times.
    case AnimationDiveBite: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            creatureRotation[0] = -30.0f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                creatureRotation[0] = 120.0f;
            }
        }
        break;
    }
    // Animation 39: poses the body and the head by the ability's anticipation and strike times.
    case AnimationHeadbutt: {
        int strike = m_creature->strikeTime();
        int anticipation = m_creature->anticipationTime();
        if ((float)(anticipation + strike / 2) > m_animationTime) {
            headRotation = plasma::Vector<3, float>(-30.0f, 10.0f, 90.0f);
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, 60.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, -2.0f, 0.0f);
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            headRotation = plasma::Vector<3, float>(30.0f, -10.0f, -30.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, -20.0f);
            headOffset = plasma::Vector<3, float>(3.0f, 0.0f, 2.0f);
        }
        break;
    }
    case Animation_12:
    case Animation_13:
    case Animation_14:
    case Animation_37:
    case Animation_70:
    case Animation_83: {
        break;
    }
    // Animation 40: poses the hands, the body, the head and the tail by the ability's anticipation and strike times.
    case AnimationHornCharge: {
        int strike = m_creature->strikeTime();
        int anticipation = m_creature->anticipationTime();
        if ((float)(anticipation + strike / 2) > m_animationTime) {
            headRotation = plasma::Vector<3, float>(-70.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, -3.0f, -1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            headRotation[0] = 50.0f;
            tailRotationX = -10.0f;
            tailRotationZ = 0.0f;
            headRotation[1] = 0.0f;
            headRotation[2] = 0.0f;
            bodyRotation = plasma::Vector<3, float>(20.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -3.0f, 1.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
            leftHandRotation[0] = -20.0f;
            rightHandRotation[0] = -20.0f;
        }
        break;
    }
    // Animation 44: poses the hands, the body, the head and the tail by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationStomp: {
        int strike = m_creature->strikeTime();
        int anticipation = m_creature->anticipationTime();
        if ((float)(anticipation + strike / 2) > m_animationTime) {
            bodyRotation[0] = 50.0f;
            headOffset[1] = -6.0f;
            tailRotationX = -50.0f;
            tailRotationZ = 0.0f;
            headRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
            bodyRotation[1] = 0.0f;
            bodyRotation[2] = 0.0f;
            bodyOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
            headOffset[0] = 0.0f;
            headOffset[2] = -2.0f;
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 30.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
            leftHandRotation[0] = 0.0f;
            rightHandRotation[0] = 0.0f;
            blendRate = 0.02f;
            headRotation[1] = 0.0f;
            headRotation[2] = 0.0f;
            headRotation[0] = -10.0f;
        }
        break;
    }
    case AnimationClawStrike:
    case AnimationEatPetFood: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            headRotation[2] = 0.0f;
            bodyOffset[1] = -1.0f;
            bodyOffset[2] = 0.0f;
            tailRotationX = 30.0f;
            tailRotationZ = 10.0f;
            bodyOffset[0] = 0.0f;
            bodyRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
            headRotation[0] = 10.0f;
            headRotation[1] = 0.0f;
            headOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
            leftHandOffset[2] = 2.0f;
            rightHandOffset[2] = 2.0f;
        } else {
            int strike = m_creature->strikeTime();
            int anticipation = m_creature->anticipationTime();
            if ((float)(anticipation + strike / 3) > m_animationTime) {
                bodyRotation[0] = 50.0f;
                tailRotationX = -30.0f;
                tailRotationZ = 0.0f;
                headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
                bodyRotation[1] = 0.0f;
                bodyRotation[2] = 0.0f;
                bodyOffset = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
                headOffset = plasma::Vector<3, float>(0.0f, -4.0f, 2.0f);
                leftHandRotation[0] = 60.0f;
                rightHandRotation[0] = 70.0f;
            } else {
                int strike2 = m_creature->strikeTime();
                int anticipation2 = m_creature->anticipationTime();
                if ((float)(anticipation2 + (strike2 + strike2) / 3) > m_animationTime) {
                    bodyRotation[0] = 40.0f;
                    bodyRotation[1] = 0.0f;
                    leftHandRotation[0] = 70.0f;
                    rightHandRotation[0] = -30.0f;
                    rightHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, -2.0f);
                    tailRotationX = -30.0f;
                    tailRotationZ = -10.0f;
                    headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
                    bodyOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
                    bodyRotation[2] = 0.0f;
                    headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
                } else {
                    int anticipation3 = m_creature->anticipationTime();
                    int strike3 = m_creature->strikeTime();
                    if ((float)(anticipation3 + strike3) > m_animationTime) {
                        bodyRotation[0] = 10.0f;
                        bodyRotation[1] = 0.0f;
                        leftHandRotation[0] = -30.0f;
                        rightHandRotation[0] = -40.0f;
                        leftHandOffset = plasma::Vector<3, float>(0.0f, 3.0f, -2.0f);
                        tailRotationX = -30.0f;
                        tailRotationZ = -10.0f;
                        headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 2.0f);
                        bodyOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
                        bodyRotation[2] = 0.0f;
                        headRotation = plasma::Vector<3, float>(-30.0f, 0.0f, 0.0f);
                    } else if ((float)m_creature->attackDuration() > m_animationTime) {
                        headRotation[2] = -10.0f;
                        bodyOffset[1] = 1.0f;
                        bodyOffset[2] = 0.0f;
                        tailRotationX = 30.0f;
                        tailRotationZ = 10.0f;
                        bodyOffset[0] = 0.0f;
                        bodyRotation = plasma::Vector<3, float>(-20.0f, 0.0f, 0.0f);
                        headRotation[0] = 10.0f;
                        headRotation[1] = 0.0f;
                        headOffset = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
                        leftHandRotation[0] = 20.0f;
                        rightHandRotation[0] = 20.0f;
                        leftHandOffset[2] = 2.0f;
                        rightHandOffset[2] = 2.0f;
                    }
                }
            }
        }
        break;
    }
    // Animation 43: poses the hands, the body, the head and the tail by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationScream: {
        int strike = m_creature->strikeTime();
        int anticipation = m_creature->anticipationTime();
        if ((float)(anticipation + strike / 2) > m_animationTime) {
            headRotation[1] = 10.0f;
            headRotation[2] = 30.0f;
            bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, 0.0f, -1.0f);
            leftHandRotation[0] = 20.0f;
            rightHandRotation[0] = 20.0f;
            blendRate = 0.005f;
            headRotation[0] = -10.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            headRotation = plasma::Vector<3, float>(50.0f, 30.0f, 0.0f);
            int anticipation2 = m_creature->anticipationTime();
            int strike2 = m_creature->strikeTime();
            if ((float)(anticipation2 + strike2) > m_animationTime) {
                headRotation[1] = -30.0f;
            }
            rightHandRotation[0] = -60.0f;
            tailRotationX = -20.0f;
            tailRotationZ = 0.0f;
            bodyRotation = plasma::Vector<3, float>(30.0f, 0.0f, 0.0f);
            bodyOffset = plasma::Vector<3, float>(0.0f, -1.0f, 2.0f);
            headOffset = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
            leftHandRotation[0] = -20.0f;
            blendRate = 0.005f;
        }
        break;
    }
    // Animation 89: poses the hands, the body and the head by the ability's anticipation and strike times; sets the
    // blend rate.
    case AnimationHeroicShout: {
        int strike = m_creature->strikeTime();
        int anticipation = m_creature->anticipationTime();
        if ((float)(anticipation + strike / 2) > m_animationTime) {
            bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, 2.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 2.0f, 0.0f);
            blendRate = 0.005f;
            headRotation[1] = 0.0f;
            headRotation[2] = 0.0f;
            headRotation[0] = -10.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            headRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(30.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-4.0f, 3.0f, 5.0f);
            rightHandOffset = plasma::Vector<3, float>(4.0f, 3.0f, 5.0f);
            leftHandRotation[1] = -10.0f;
            rightHandRotation[1] = 10.0f;
            blendRate = 0.005f;
        }
        break;
    }
    // Animation 48: poses the hands, the body, the head and the whole creature by the ability's anticipation and
    // strike times.
    case AnimationDashRight: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            headRotation[2] = -60.0f;
            bodyRotation[2] = 10.0f;
            bodyRotation[1] = -30.0f;
            leftHandRotation[1] = 30.0f;
            rightHandRotation[1] = 30.0f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                leftHandRotation[1] = -80.0f;
                rightHandRotation[1] = -80.0f;
                creatureRotation[1] = (m_animationTime - (float)anticipation2) / (float)strike2 * 360.0f;
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                bodyRotation[1] = 30.0f;
                leftHandRotation[1] = -30.0f;
                rightHandRotation[1] = -30.0f;
            }
        }
        break;
    }
    // Animation 47: poses the hands, the body, the head and the whole creature by the ability's anticipation and
    // strike times.
    case AnimationDashLeft: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -10.0f;
            bodyRotation[1] = 30.0f;
            leftHandRotation[1] = -30.0f;
            rightHandRotation[1] = -30.0f;
            headRotation[2] = 60.0f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                leftHandRotation[1] = 80.0f;
                rightHandRotation[1] = 80.0f;
                creatureRotation[1] = (m_animationTime - (float)anticipation2) / (float)strike2 * -360.0f;
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                bodyRotation[1] = -30.0f;
                leftHandRotation[1] = 30.0f;
                rightHandRotation[1] = 30.0f;
            }
        }
        break;
    }
    // Animation 46: poses the hands, the body, the head, the whole creature and the tail by the ability's anticipation
    // and strike times.
    case AnimationDashForward: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            tailRotationX = -60.0f;
            bodyRotation[0] = -10.0f;
            headRotation[0] = 10.0f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                leftHandRotation[0] = 80.0f;
                rightHandRotation[0] = 80.0f;
                headOffset[0] = 0.0f;
                headOffset[1] = -3.0f;
                headRotation[0] = -60.0f;
                creatureRotation[0] = (m_animationTime - (float)anticipation2) / (float)strike2 * -360.0f;
                headOffset[2] = 0.0f;
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                bodyRotation[0] = 20.0f;
                headRotation[0] = -10.0f;
                headOffset = plasma::Vector<3, float>(0.0f, -2.0f, 0.0f);
            }
        }
        break;
    }
    // Animation 41: poses the head, the whole creature and the tail by the ability's anticipation time; sets the blend
    // rate.
    case AnimationTailStrike: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            headRotation[2] = -60.0f;
            tailRotationZ = -70.0f;
            tailRotationX = 0.0f;
            creatureRotation[2] = 90.0f;
            headRotation[0] = 0.0f;
            headRotation[1] = 10.0f;
            blendRate = 0.002f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            creatureRotation[2] = 200.0f;
            tailRotationZ = 70.0f;
            tailRotationX = 0.0f;
            headRotation = plasma::Vector<3, float>(0.0f, -20.0f, 60.0f);
            headOffset = plasma::Vector<3, float>(-3.0f, 0.0f, -2.0f);
        }
        break;
    }
    // Animation 42: the head, the hands, the feet and the tail are drawn into the chest during the anticipation, are
    // not drawn at all during the strike (the body sinks by three units) and come back out over the hold time.
    case AnimationShell: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            float drawnIn = m_animationTime / (float)m_creature->anticipationTime();
            plasma::Vector<3, float> toChest(chestPosition[0] - 0.0f - headPosition[0],
                                             chestPosition[1] - 0.0f - headPosition[1],
                                             chestPosition[2] - 2.0f - headPosition[2]);
            headPosition += drawnIn * toChest;
            leftHandPosition += drawnIn * (chestPosition - leftHandPosition);
            rightHandPosition += drawnIn * (chestPosition - rightHandPosition);
            leftFootPosition += drawnIn * (chestPosition - leftFootPosition);
            rightFootPosition += drawnIn * (chestPosition - rightFootPosition);
            tailPosition += drawnIn * (chestPosition + plasma::Vector<3, float>(0.0f, 4.0f, 4.0f) - tailPosition);
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                m_headSprite = nullptr;
                m_handsSprite = nullptr;
                m_feetSprite = nullptr;
                m_tailSprite = nullptr;
                bodyOffset = plasma::Vector<3, float>(0.0f, 0.0f, -3.0f);
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                int hold = m_creature->holdTime();
                float drawnIn = 1.0f - (m_animationTime - (float)anticipation2 - (float)strike2) / (float)hold;
                headPosition += drawnIn * (chestPosition - plasma::Vector<3, float>(0.0f, 0.0f, 2.0f) - headPosition);
                leftHandPosition += drawnIn * (chestPosition - leftHandPosition);
                rightHandPosition += drawnIn * (chestPosition - rightHandPosition);
                leftFootPosition += drawnIn * (chestPosition - leftFootPosition);
                rightFootPosition += drawnIn * (chestPosition - rightFootPosition);
                tailPosition += drawnIn * (chestPosition + plasma::Vector<3, float>(0.0f, 4.0f, 4.0f) - tailPosition);
            }
        }
        break;
    }
    // Animation 38: poses the hands, the shoulders, the body and the feet by the ability's anticipation time; returns
    // to animation 0 when the ability is over; sets the blend rate.
    case AnimationTwoHandedSwing: {
        float shoulderZ;
        float shoulderY;
        float targetShoulderZ;
        float targetShoulderY;
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 80.0f);
            bodyRotation[2] = -80.0f;
            shoulderZ = leftShoulderPosition[2];
            shoulderY = leftShoulderPosition[1];
            targetShoulderZ = shoulderZ;
            targetShoulderY = shoulderY;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -90.0f);
            bodyRotation[2] = 90.0f;
            leftFootPosition[1] -= 4.0f;
            rightHandPosition[0] += 4.0f;
            leftFootSideways = 1;
            rightFootPosition[1] += 4.0f;
            targetShoulderZ = leftShoulderPosition[2];
            targetShoulderY = leftShoulderPosition[1];
            blendRate = 0.05f;
        } else {
            m_animation = AnimationNone;
            rightHandPosition[0] -= 8.0f;
            rightHandPosition[1] += 4.0f;
            leftHandPosition = rightHandPosition;
            float handZ = leftHandPosition[2] - 1.0f;
            leftHandPosition[0] -= 2.0f;
            rightHandRotation[0] = -60.0f;
            leftHandPosition[2] = handZ;
            rightHandRotation[2] = 20.0f;
            bodyRotation[2] = -20.0f;
            shoulderZ = leftShoulderPosition[2] - 4.0f;
            shoulderY = leftShoulderPosition[1] + 10.0f;
            targetShoulderZ = shoulderZ;
            targetShoulderY = shoulderY;
        }
        leftHandPosition = rightHandPosition;
        leftShoulderPosition[1] = targetShoulderY + 10.0f;
        leftShoulderPosition[2] = targetShoulderZ - 4.0f;
        leftHandPosition[0] += 2.0f;
        leftHandRotation = rightHandRotation;
        break;
    }
    // Animation 4: poses the hands, the shoulders, the body and the whole creature by the ability's anticipation time;
    // with a motion that cycles with the animation time; sets the blend rate.
    case AnimationCrush: {
        if ((float)(m_creature->anticipationTime() / 2) > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            float fraction = std::min(1.0f, m_animationTime / (float)(anticipation / 2));
            creatureRotation[0] = fraction * 5.0f;
            bodyRotation[1] = fraction * 10.0f;
            bodyRotation[2] = fraction * -20.0f;
            rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(0.0f, 120.0f, 0.0f)) +
                                (fraction * plasma::Vector<3, float>(60.0f, 60.0f, 60.0f));
            leftHandPosition = rightHandPosition;
            blendRate = 0.005f;
        } else {
            if (!(m_creature->m_entityData.classType != ClassWarrior ||
                  m_creature->m_entityData.specialization != SpecializationGuardian)) {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 7.0f, 4.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, -60.0f, 0.0f);
                blendRate = 0.01f;
            } else {
                int anticipation = m_creature->anticipationTime();
                int anticipation2 = m_creature->anticipationTime();
                creatureRotation[0] = 0.0f;
                bodyRotation = plasma::Vector<3, float>(20.0f, 0.0f, 0.0f);
                float fraction2 =
                    std::min(1.0f, (m_animationTime - (float)(anticipation / 2)) / (float)(anticipation2 / 2) * 1.5f);
                rightHandRotation = ((1.0f - fraction2) * plasma::Vector<3, float>(90.0f, 0.0f, 90.0f)) +
                                    (fraction2 * plasma::Vector<3, float>(30.0f, 0.0f, 0.0f));
                float cosine = cosf(m_animationTime * 0.5f);
                rightHandPosition[2] += cosine * 0.5f;
                rightHandOffset = plasma::Vector<3, float>(-6.0f, 0.0f, fraction2 * 16.0f);
                blendRate = 0.005f;
            }
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
        }
        leftHandPosition[0] += 2.0f;
        leftShoulderPosition[2] -= 4.0f;
        leftShoulderPosition[1] += 10.0f;
        leftHandRotation = rightHandRotation;
        break;
    }
    // Animation 2: poses the hands, the shoulders, the body and the whole creature by the ability's anticipation and
    // strike times; with a motion that cycles with the animation time; returns to animation 0 when the ability is
    // over; sets the blend rate.
    case AnimationTwoHandedSweep: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            bodyRotation[0] = 10.0f;
            bodyRotation[1] = 10.0f;
            float fraction = std::min(1.0f, m_animationTime / (float)anticipation * 1.5f);
            creatureRotation[0] = fraction * 10.0f + 5.0f;
            bodyRotation[2] = -60.0f;
            rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(90.0f, 0.0f, 90.0f)) +
                                (fraction * plasma::Vector<3, float>(60.0f, 0.0f, 60.0f));
            rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, fraction * 14.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftShoulderPosition[1] += 10.0f;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftHandRotation = rightHandRotation;
            blendRate = 0.01f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            creatureRotation[0] = -10.0f;
            bodyRotation[1] = 10.0f;
            bodyRotation[2] = 60.0f;
            rightHandRotation = plasma::Vector<3, float>(-70.0f, 0.0f, -60.0f);
            float sine = sinf(
                (float)((double)std::min(1.0f, (m_animationTime - (float)anticipation) / (float)strike) * Pi * 0.5));
            rightHandOffset = plasma::Vector<3, float>(0.0f, sine * 2.0f, 0.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[1] -= 1.0f;
            leftHandPosition[0] += 2.0f;
            leftHandRotation = rightHandRotation;
            blendRate = 0.04f;
        } else {
            m_animation = AnimationNone;
            rightHandPosition[2] += 3.0f;
            rightHandPosition[1] += 2.0f;
            leftHandPosition = rightHandPosition;
            rightHandRotation[0] = 0.0f;
            rightHandRotation[1] = 20.0f;
            leftHandPosition[2] -= 3.0f;
            leftHandPosition[0] -= 3.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftShoulderPosition[1] += 10.0f;
            leftHandRotation = rightHandRotation;
            leftHandRotation = rightHandRotation;
        }
        break;
    }
    // Animation 49: poses the hands, the body, the head, the whole creature and the feet by the ability's
    // anticipation, strike and hold times; sets the blend rate.
    case AnimationTwoHandedSpin: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            rightHandRotation = plasma::Vector<3, float>(-40.0f, 60.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(-5.0f, 8.0f, 4.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 30.0f, -55.0f);
            creatureRotation[2] = m_animationTime / (float)anticipation * -20.0f;
            creatureOffset[0] = m_animationTime / (float)anticipation * 0.0f;
            if (m_creature->m_strikeCombo / 2 % 3 == 2) {
                int strikeLength = m_creature->strikeTime();
                int anticipationLength = m_creature->anticipationTime();
                if ((float)(anticipationLength + strikeLength / 2) > m_animationTime) {
                    creatureRotation[2] += m_animationTime / (float)anticipation * 360.0f;
                }
            }
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            int hold = m_creature->holdTime();
            if ((float)(hold + (anticipation + strike)) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                float fraction = std::min(1.0f, (m_animationTime - (float)anticipation2) * 1.2f / (float)strike2);
                footRoll = fraction;
                creatureOffset[0] = (1.0f - fraction) * 0.0f - fraction * 0.8f;
                rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(-60.0f, 60.0f, 60.0f)) +
                                    (fraction * plasma::Vector<3, float>(-60.0f, 60.0f, 60.0f));
                rightHandOffset = ((1.0f - fraction) * plasma::Vector<3, float>(-10.0f, 8.0f, 4.0f)) +
                                  (fraction * plasma::Vector<3, float>(-12.0f, 4.0f, 2.0f));
                bodyRotation[0] = -20.0f;
                headRotation[0] = -10.0f;
                bodyRotation[2] = fraction * 50.0f - (1.0f - fraction) * 45.0f;
                creatureRotation[2] = fraction * 30.0f;
                footRoll = footRoll * 10.0f;
                blendRate = 0.05f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                bodyRotation[2] = -30.0f;
                blendRate = 0.03f;
            }
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        float handY = leftHandPosition[1] - 0.2f;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        leftHandPosition[1] = handY;
        break;
    }
    // Animation 50: poses the hands, the body, the head, the whole creature and the feet by the ability's anticipation
    // and strike times; sets the blend rate.
    case AnimationGuardianCrushStrike: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(30.0f, -60.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-14.0f, 5.0f, 5.0f);
            creatureOffset[0] = -0.0f;
            headRotation[0] = -10.0f;
            headRotation[2] = 30.0f;
            bodyRotation[2] = 45.0f;
            creatureRotation[2] = 20.0f;
            int anticipation = m_creature->anticipationTime();
            if (m_creature->m_strikeCombo / 2 % 3 == 2) {
                int strikeLength = m_creature->strikeTime();
                int anticipationLength = m_creature->anticipationTime();
                if ((float)(anticipationLength + strikeLength / 2) > m_animationTime) {
                    creatureRotation[2] = creatureRotation[2] - m_animationTime / (float)anticipation * 360.0f;
                }
            }
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            headRotation[2] = -30.0f;
            float fraction = std::min(1.0f, (m_animationTime - (float)anticipation) * 1.2f / (float)strike);
            creatureOffset[0] = (1.0f - fraction) * -0.0f + fraction * 0.8f;
            rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(0.0f, -90.0f, 20.0f)) +
                                (fraction * plasma::Vector<3, float>(0.0f, -90.0f, -130.0f));
            plasma::Vector<3, float> vector = fraction * plasma::Vector<3, float>(0.0f, 2.0f, 0.0f);
            float remaining = 1.0f - fraction;
            float progress = fraction;
            rightHandOffset = ((1.0f - fraction) * plasma::Vector<3, float>(-8.0f, 6.0f, 6.0f)) + vector;
            creatureRotation[0] = progress * -20.0f;
            creatureRotation[2] = progress * -30.0f;
            bodyRotation[2] = remaining * 45.0f - progress * 45.0f;
            footRoll = progress * 10.0f;
            blendRate = 0.05f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            plasma::Vector<3, float> vector;
            vector = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            rightHandRotation = vector;
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] = leftHandPosition[1] - 0.2f;
        leftHandPosition[2] = leftHandPosition[2] - 1.0f;
        break;
    }
    // Animation 3: poses the hands, the shoulders, the body and the whole creature by the ability's anticipation and
    // strike times; returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationTwoHandedThrust: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-5.0f, 8.0f, 0.0f);
            leftHandOffset = rightHandOffset;
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftShoulderPosition[1] += 10.0f;
            blendRate = 0.005f;
        } else {
            int strike = m_creature->strikeTime();
            int anticipation = m_creature->anticipationTime();
            if ((float)(anticipation + strike / 2) > m_animationTime) {
                creatureRotation = plasma::Vector<3, float>(10.0f, 0.0f, -10.0f);
                bodyRotation = plasma::Vector<3, float>(30.0f, 0.0f, -30.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
                rightHandOffset = plasma::Vector<3, float>(-8.0f, -4.0f, 20.0f);
                leftHandOffset = rightHandOffset;
                leftHandPosition = rightHandPosition;
                leftShoulderPosition[1] += 10.0f;
                leftHandPosition[0] += 2.0f;
                leftShoulderPosition[2] -= 4.0f;
                blendRate = 0.005f;
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                creatureRotation[2] = -30.0f;
                bodyRotation = plasma::Vector<3, float>(0.0f, 20.0f, -60.0f);
                rightHandRotation = plasma::Vector<3, float>(70.0f, 0.0f, 90.0f);
                rightHandOffset = plasma::Vector<3, float>(5.0f, -2.0f, 0.0f);
                leftHandOffset = rightHandOffset;
                leftHandPosition = rightHandPosition;
                leftHandPosition[0] += 2.0f;
                leftShoulderPosition[2] -= 4.0f;
                leftShoulderPosition[1] += 10.0f;
                blendRate = 0.02f;
            } else {
                m_animation = AnimationNone;
                leftHandPosition = rightHandPosition;
                rightHandRotation[1] = 90.0f;
                leftHandPosition[2] -= 1.0f;
                leftHandPosition[0] -= 2.0f;
                float shoulderZ = leftShoulderPosition[2] - 4.0f;
                leftShoulderPosition[1] += 10.0f;
                bodyRotation[2] = -80.0f;
                leftShoulderPosition[2] = shoulderZ;
                leftHandRotation = rightHandRotation;
            }
        }
        leftHandRotation = rightHandRotation;
        break;
    }
    // Animation 1: poses the hands, the shoulders, the body, the whole creature and the feet while the ability lasts;
    // sets the blend rate.
    case AnimationSmash: {
        if (500.0f > m_animationTime) {
            feetTurn = std::min(1.0f, m_animationTime / 500.0f);
            bool twoHanded = m_creature->hasTwoHandedWeapon();
            if (twoHanded) {
                rightHandOffset[1] = 0.0f;
                rightHandOffset[0] = -rightHandPosition[0];
                rightHandRotation[0] = feetTurn * 120.0f;
                rightHandOffset[2] = 14.0f;
                float handOffsetX = 2.0f - leftHandOffset[0];
                leftHandOffset = plasma::Vector<3, float>(handOffsetX, 0.0f, 12.0f);
            } else {
                rightHandOffset = plasma::Vector<3, float>(1.0f, 0.0f, 14.0f);
                leftHandOffset = plasma::Vector<3, float>(-1.0f, 0.0f, 14.0f);
                leftHandRotation[0] = -100.0f;
                rightHandRotation[0] = -100.0f;
            }
            float tilt = feetTurn * -40.0f;
            feetTurn = feetTurn * -50.0f;
            bodyRotation[0] = tilt;
            creatureRotation[0] = tilt;
            if (m_creature->m_entityData.currentAbility == AbilityIntercept) {
                creatureRotation[0] = tilt - std::min(360.0f, m_animationTime / 500.0f * 360.0f);
            }
            m_creature->m_walkAmplitude = 0;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[0] = -20.0f;
            rightHandRotation[0] = -70.0f;
            creatureRotation[0] =
                (1.0f - std::min(1.0f, (m_animationTime - 500.0f) / 500.0f + (m_animationTime - 500.0f) / 500.0f)) *
                -60.0f;
            bool twoHanded = m_creature->hasTwoHandedWeapon();
            if (twoHanded) {
                rightHandOffset = plasma::Vector<3, float>(-rightHandPosition[0], 4.0f, 0.0f);
                float handOffsetX = 2.0f - leftHandOffset[0];
                leftHandOffset = plasma::Vector<3, float>(handOffsetX, 4.0f, 0.0f);
            } else {
                rightHandOffset = plasma::Vector<3, float>(1.0f, 3.0f, 8.0f);
                leftHandOffset = plasma::Vector<3, float>(-1.0f, 3.0f, 8.0f);
                leftHandRotation[0] = -150.0f;
                rightHandRotation[0] = -150.0f;
            }
            blendRate = 0.1f;
        } else if (m_creature->hasTwoHandedWeapon()) {
            rightHandPosition[1] += 4.0f;
            rightHandPosition[0] -= 8.0f;
            leftHandPosition = rightHandPosition;
            leftHandPosition[2] -= 1.0f;
            leftHandPosition[0] -= 2.0f;
            rightHandRotation[2] = 20.0f;
            float shoulderZ = leftShoulderPosition[2] - 4.0f;
            leftShoulderPosition[1] += 10.0f;
            rightHandRotation[0] = -60.0f;
            leftShoulderPosition[2] = shoulderZ;
        } else {
            rightHandOffset = plasma::Vector<3, float>(1.0f, 3.0f, 2.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, 3.0f, 2.0f);
            leftHandRotation[0] = -250.0f;
            rightHandRotation[0] = -250.0f;
        }
        if (!(m_creature->m_entityData.equipment.weaponLeft.category != ItemCategoryWeapon ||
              m_creature->m_entityData.equipment.weaponLeft.id != WeaponShield)) {
            leftHandRotation = plasma::Vector<3, float>(-250.0f, -90.0f, 0.0f);
        }
        break;
    }
    // Animation 7: poses the hands, the shoulders and the hands' turn while the ability lasts; sets the blend rate.
    case AnimationTwoHandedRest: {
        if ((float)(m_creature->attackDuration() / 2) > m_animationTime) {
            handsTurn = m_creature->m_entityData.initialHandsRotation;
            blendRate = 0.005f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            int duration = m_creature->attackDuration();
            int duration2 = m_creature->attackDuration();
            float fraction =
                std::min(1.0f, (m_animationTime - (float)(duration / 2)) / (float)(duration2 / 2) *
                                   ((m_animationTime - (float)(duration / 2)) / (float)(duration2 / 2)) *
                                   ((m_animationTime - (float)(duration / 2)) / (float)(duration2 / 2)) * 15.0f);
            float blend = fraction;
            handsTurn = ((1.0f - blend) * m_creature->m_entityData.initialHandsRotation) +
                        (fraction * m_creature->m_entityData.finalHandsRotation);
            blendRate = 0.03f;
        } else {
            handsTurn = m_creature->m_entityData.finalHandsRotation;
        }
        rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
        bool twoHanded = m_creature->hasTwoHandedWeapon();
        if (twoHanded) {
            leftHandPosition = rightHandPosition;
            leftHandPosition[0] += 2.0f;
            leftShoulderPosition[2] -= 4.0f;
            leftShoulderPosition[1] += 10.0f;
        }
        break;
    }
    // Animation 22: poses the hands and the body by fixed times.
    case AnimationShoot: {
        if (m_creature->m_entityData.equipment.weaponRight.id != WeaponBow &&
            m_creature->m_entityData.equipment.weaponRight.id != WeaponCrossbow) {
            leftHandRotation[1] = -90.0f;
            leftHandOffset[1] += 3.0f;
            float leftHandHeight = leftHandOffset[2] + 2.0f;
            leftHandOffset[0] -= 4.0f;
            rightHandOffset[1] += 2.0f;
            rightHandOffset[0] -= 10.0f;
            leftHandRotation[2] = 60.0f;
            rightHandRotation[2] = 60.0f;
            leftHandOffset[2] = leftHandHeight;
            rightHandOffset[2] += 1.0f;
            bodyRotation[2] = -60.0f;
            float leftHandAngleY;
            if (!(m_creature->m_entityData.equipment.weaponLeft.category != ItemCategoryWeapon ||
                  (unsigned)(m_creature->m_entityData.equipment.weaponLeft.id - WeaponHandAttacks) <= 1)) {
                Sprite *sprite = spriteManager->sprite(SpriteArrow);
                m_rightWeaponSprite = (Sprite *)(sprite);
                leftHandAngleY = leftHandRotation[1];
            } else {
                leftHandAngleY = -90.0f;
            }
            if (300.0f > m_animationTime) {
                leftHandOffset[2] -= 1.0f;
                leftHandRotation[0] += 20.0f;
                leftHandRotation[1] = leftHandAngleY - 20.0f;
            }
        } else {
            rightHandRotation[1] = -90.0f;
            rightHandOffset[1] += 3.0f;
            bodyRotation[2] = 60.0f;
            float leftHandY = leftHandOffset[1];
            leftHandOffset[0] += 10.0f;
            float rightHandHeight = rightHandOffset[2];
            rightHandOffset[0] += 4.0f;
            rightHandRotation[2] = -60.0f;
            leftHandRotation[2] = -60.0f;
            rightHandOffset[2] = rightHandHeight + 2.0f;
            float leftHandHeight = leftHandOffset[2];
            leftHandOffset[1] = leftHandY + 2.0f;
            leftHandOffset[2] = leftHandHeight + 1.0f;
            float rightHandAngleY;
            if (!(m_creature->m_entityData.equipment.weaponRight.category != ItemCategoryWeapon ||
                  (unsigned)(m_creature->m_entityData.equipment.weaponRight.id - WeaponHandAttacks) <= 1)) {
                Sprite *sprite = spriteManager->sprite(SpriteArrow);
                m_leftWeaponSprite = (Sprite *)(sprite);
                rightHandAngleY = rightHandRotation[1];
            } else {
                rightHandAngleY = -90.0f;
            }
            if (300.0f > m_animationTime) {
                rightHandOffset[2] -= 1.0f;
                rightHandRotation[0] += 20.0f;
                rightHandRotation[1] = rightHandAngleY + 20.0f;
            }
        }
        break;
    }
    // Animation 72: poses the hands, the body and the whole creature by the ability's anticipation time; sets the
    // blend rate.
    case AnimationThrowBoomerang: {
        if (m_creature->m_strikeStrength > 0.0f) {
            if ((float)m_creature->anticipationTime() > m_animationTime) {
                bodyRotation[2] = -90.0f;
                rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
                m_leftWeaponSprite = m_rightWeaponSprite;
            } else if ((float)(m_creature->anticipationTime() + 300) > m_animationTime) {
                bodyRotation[2] = 10.0f;
                leftHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 1.0f);
                leftHandRotation = plasma::Vector<3, float>(0.0f, -10.0f, 0.0f);
                m_leftWeaponSprite = m_rightWeaponSprite;
                m_rightWeaponSprite = nullptr;
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                bodyRotation[2] = 90.0f;
                m_rightWeaponSprite = nullptr;
                m_leftWeaponSprite = nullptr;
            }
            if (!(!(m_animationTime > (float)m_creature->anticipationTime()) ||
                  !((float)m_creature->attackDuration() > m_animationTime))) {
                int anticipation = m_creature->anticipationTime();
                creatureRotation[2] = (m_animationTime - (float)anticipation) * 360.0f / 200.0f;
                if ((m_animationTime - (float)anticipation) * 360.0f / 200.0f > 700.0f) {
                    creatureRotation[2] = 700.0f;
                    blendRate = 0.03f;
                } else {
                    blendRate = 0.03f;
                }
            } else {
                blendRate = 0.03f;
            }
        } else if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -90.0f;
            rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = 90.0f;
            m_rightWeaponSprite = nullptr;
        }
        break;
    }
    // Animation 73: poses the hands and the body; with a motion that cycles with the animation time.
    case AnimationChargedBoomerang: {
        bodyRotation[2] = m_creature->m_entityData.charge * -90.0f;
        rightHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 1.0f);
        rightHandRotation = plasma::Vector<3, float>(0.0f, 10.0f, 0.0f);
        float cosine = cosf(m_animationTime * 0.5f);
        rightHandPosition[2] += cosine * 0.5f;
        float cosine2 = cosf(m_animationTime * 0.5f);
        leftHandPosition[2] += cosine2 * 0.5f;
        if (m_creature->m_entityData.charge > 0.0f) {
            m_leftWeaponSprite = m_rightWeaponSprite;
        }
        break;
    }
    // Animation 77: poses the hands, the body, the whole creature and the part scales by the ability's anticipation
    // and strike times; sets the blend rate.
    case AnimationThrowRock: {
        if ((float)m_creature->anticipationTime() > m_creature->m_entityData.timeSinceAbility) {
            rightHandOffset = plasma::Vector<3, float>(-3.0f, -2.0f, 10.0f);
            leftHandOffset = plasma::Vector<3, float>(3.0f, -2.0f, 10.0f);
            Sprite *sprite = spriteManager->sprite(SpriteIronDeposit);
            m_leftWeaponSprite = (Sprite *)(sprite);
            bodyRotation[0] = 40.0f;
            leftWeaponScale = leftWeaponScale * 1.5f;
            creatureRotation[0] = 10.0f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_creature->m_entityData.timeSinceAbility) {
                rightHandOffset = plasma::Vector<3, float>(0.0f, 5.0f, 0.0f);
                leftHandOffset = plasma::Vector<3, float>(0.0f, 5.0f, 0.0f);
                bodyRotation[0] = -20.0f;
                creatureRotation[0] = -40.0f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(2.0f, -1.0f, 0.0f);
                leftHandOffset = plasma::Vector<3, float>(-2.0f, -1.0f, 0.0f);
            }
        }
        blendRate = 0.01f;
        break;
    }
    // Animation 21: poses the hands and the body while the ability lasts.
    case AnimationDrawBow: {
        if ((unsigned)(m_creature->m_entityData.equipment.weaponRight.id + -6) > 1) {
            if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow) {
                leftHandRotation[1] = -90.0f;
            }
            int duration = m_creature->attackDuration();
            if ((float)(duration + duration) > m_animationTime) {
                float leftHandY;
                float leftHandX;
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponBow) {
                    leftHandY = leftHandOffset[1] + 5.0f;
                    leftHandX = leftHandOffset[0] - 7.0f;
                } else {
                    leftHandY = leftHandOffset[1] + 2.0f;
                    leftHandX = leftHandOffset[0] - 3.0f;
                }
                leftHandOffset[1] = leftHandY;
                float leftHandHeight = leftHandOffset[2];
                leftHandOffset[0] = leftHandX;
                leftHandRotation[2] = 70.0f;
                rightHandRotation[2] = 70.0f;
                leftHandOffset[2] = leftHandHeight + 4.0f;
                float rightHandX = rightHandOffset[0] - 10.0f;
                rightHandOffset[1] += 3.0f;
                rightHandOffset[2] += 3.0f;
                rightHandOffset[0] = rightHandX;
                bodyRotation[2] -= 70.0f;
                int duration2 = m_creature->attackDuration();
                if ((float)(duration2 / 2) > m_animationTime) {
                    leftHandRotation[0] = -5.0f;
                    leftHandOffset[2] -= 2.0f;
                    rightHandOffset = plasma::Vector<3, float>(-5.0f, 1.0f, 0.0f);
                    bodyRotation[2] = -50.0f;
                    leftHandRotation[2] = 70.0f;
                    m_rightWeaponSprite = nullptr;
                } else if (!(m_creature->m_entityData.equipment.weaponLeft.category != ItemCategoryWeapon ||
                             (unsigned)(m_creature->m_entityData.equipment.weaponLeft.id - WeaponHandAttacks) <= 1)) {
                    Sprite *sprite = spriteManager->sprite(SpriteArrow);
                    m_rightWeaponSprite = (Sprite *)(sprite);
                }
            } else {
                leftHandRotation[0] = -30.0f;
                rightHandRotation[0] = -30.0f;
                rightHandRotation[1] = 45.0f;
                leftHandOffset[0] -= 1.0f;
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow) {
                    leftHandRotation[2] = 50.0f;
                }
                if (!(m_creature->m_entityData.equipment.weaponLeft.category != ItemCategoryWeapon ||
                      (unsigned)(m_creature->m_entityData.equipment.weaponLeft.id - WeaponHandAttacks) <= 1)) {
                    Sprite *sprite = spriteManager->sprite(SpriteArrow);
                    m_rightWeaponSprite = (Sprite *)(sprite);
                }
            }
        } else {
            if (m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow) {
                rightHandRotation[1] = -90.0f;
            }
            int duration = m_creature->attackDuration();
            if ((float)(duration + duration) > m_animationTime) {
                float rightHandY;
                float rightHandX;
                if (m_creature->m_entityData.equipment.weaponRight.id == WeaponBow) {
                    rightHandY = rightHandOffset[1] + 5.0f;
                    rightHandX = rightHandOffset[0] + 7.0f;
                } else {
                    rightHandY = rightHandOffset[1] + 2.0f;
                    rightHandX = rightHandOffset[0] + 3.0f;
                }
                rightHandOffset[0] = rightHandX;
                rightHandOffset[1] = rightHandY;
                bodyRotation[2] = 70.0f;
                rightHandRotation[2] = -70.0f;
                leftHandRotation[2] = -70.0f;
                leftHandOffset[1] += 3.0f;
                rightHandOffset[2] += 4.0f;
                leftHandOffset[0] += 10.0f;
                leftHandOffset[2] += 3.0f;
                int duration2 = m_creature->attackDuration();
                if ((float)(duration2 / 2) > m_animationTime) {
                    rightHandRotation[0] = -5.0f;
                    rightHandOffset[2] -= 2.0f;
                    leftHandOffset = plasma::Vector<3, float>(5.0f, 1.0f, 0.0f);
                    bodyRotation[2] = 50.0f;
                    rightHandRotation[2] = -70.0f;
                    m_leftWeaponSprite = nullptr;
                } else if (!(m_creature->m_entityData.equipment.weaponRight.category != ItemCategoryWeapon ||
                             (unsigned)(m_creature->m_entityData.equipment.weaponRight.id - WeaponHandAttacks) <= 1)) {
                    Sprite *sprite = spriteManager->sprite(SpriteArrow);
                    m_leftWeaponSprite = (Sprite *)(sprite);
                }
            } else {
                rightHandRotation[0] = -30.0f;
                leftHandRotation[0] = -30.0f;
                leftHandRotation[1] = -45.0f;
                rightHandOffset[0] += 1.0f;
                if (!(m_creature->m_entityData.equipment.weaponRight.category != ItemCategoryWeapon ||
                      (unsigned)(m_creature->m_entityData.equipment.weaponRight.id - WeaponHandAttacks) <= 1)) {
                    Sprite *sprite = spriteManager->sprite(SpriteArrow);
                    m_leftWeaponSprite = (Sprite *)(sprite);
                }
            }
        }
        break;
    }
    // Animation 29: poses the hands, the body and the whole creature by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationSwirlStrike: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -80.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -90.0f);
            leftHandRotation = plasma::Vector<3, float>(20.0f, 90.0f, 90.0f);
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                bodyRotation[2] = -90.0f;
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                creatureRotation[2] = (m_animationTime - (float)anticipation2) * 360.0f / (float)strike2;
                leftHandRotation = plasma::Vector<3, float>(0.0f, 100.0f, 90.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
                blendRate = 0.2f;
            } else {
                bodyRotation[2] = 80.0f;
                creatureRotation[2] = 0.0f;
                leftHandRotation = plasma::Vector<3, float>(90.0f, 90.0f, 270.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 90.0f);
            }
        }
        break;
    }
    // Animation 86: poses the hands, the body and the whole creature by the ability's anticipation and strike times;
    // with a motion that cycles with the animation time; sets the blend rate.
    case AnimationAssassinate: {
        if (m_animationTime > (float)m_creature->anticipationTime()) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            float sine = sinf((float)((double)((m_animationTime - (float)anticipation) / (float)strike * 32.0f) * Pi));
            bodyRotation[2] = -50.0f - sine * 40.0f;
            rightHandOffset[0] = sine * 8.0f - 3.0f;
            rightHandOffset[1] = 7.0f - sine * 5.0f;
            leftHandOffset[0] = -5.0f - sine * 8.0f;
            leftHandOffset[1] = 5.0f - sine * 5.0f;
            creatureRotation[2] = -20.0f - sine * 10.0f;
            if (m_creature->m_entityData.equipment.weaponRight.id != WeaponFist) {
                rightHandRotation[0] = -90.0f;
                leftHandRotation[0] = -90.0f;
            }
            rightHandRotation[2] = -(-50.0f - sine * 40.0f);
            leftHandRotation[2] = -(-50.0f - sine * 40.0f);
            creatureRotation[0] = -50.0f;
            blendRate = 0.02f;
        }
        break;
    }
    // Animation 65: poses the hands, the body and the whole creature while the ability lasts.
    case AnimationCyclone: {
        if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = 0.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
        } else {
            bodyRotation[2] = 80.0f;
            creatureRotation[2] = 0.0f;
            leftHandRotation = plasma::Vector<3, float>(90.0f, 90.0f, 270.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 90.0f);
        }
        break;
    }
    // Animation 69: poses the hands, the body and the whole creature by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationQuake: {
        int anticipation = m_creature->anticipationTime();
        int strike = m_creature->strikeTime();
        if ((float)(strike + 400 + anticipation) > m_animationTime) {
            if (!((float)m_creature->anticipationTime() >= m_animationTime)) {
                int anticipation2 = m_creature->anticipationTime();
                int number = ((int)m_animationTime - anticipation2 + 100) / 500 % 2;
                if (number != 1) {
                    if (!(((signed char)(((int)m_animationTime - m_creature->anticipationTime() + 100) / 1000) & 1) ==
                          0)) {
                        leftHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 12.0f);
                        rightHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
                        bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, 20.0f);
                        creatureRotation = plasma::Vector<3, float>(-5.0f, 0.0f, 10.0f);
                        blendRate = 0.03f;
                    } else {
                        leftHandOffset = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
                        rightHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 12.0f);
                        bodyRotation = plasma::Vector<3, float>(-10.0f, 0.0f, -20.0f);
                        creatureRotation = plasma::Vector<3, float>(-5.0f, 0.0f, -10.0f);
                        blendRate = 0.03f;
                    }
                } else {
                    leftHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 12.0f);
                    rightHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 12.0f);
                    bodyRotation[0] = 20.0f;
                    creatureRotation[0] = 10.0f;
                }
            } else {
                leftHandOffset = plasma::Vector<3, float>(2.0f, -2.0f, 12.0f);
                rightHandOffset = plasma::Vector<3, float>(-2.0f, -2.0f, 12.0f);
                bodyRotation[0] = 20.0f;
                creatureRotation[0] = 10.0f;
            }
        }
        break;
    }
    // Animation 75: poses the hands, the body and the whole creature by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationBlockStrike: {
        int anticipation = m_creature->anticipationTime();
        int strike = m_creature->strikeTime();
        if ((float)(anticipation + strike) > m_animationTime) {
            int anticipation2 = m_creature->anticipationTime();
            int strike2 = m_creature->strikeTime();
            creatureRotation[2] = m_animationTime * -360.0f / (float)(anticipation2 + strike2);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(4.0f, 3.0f, 0.0f);
            int anticipation3 = m_creature->anticipationTime();
            int strike3 = m_creature->strikeTime();
            bodyRotation[2] = m_animationTime * -60.0f / (float)(anticipation3 + strike3);
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            blendRate = 0.03f;
        } else {
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-60.0f, -60.0f, -60.0f);
        }
        break;
    }
    // Animation 30: poses the hands, the shoulders, the body, the whole creature and the hands' turn by the ability's
    // anticipation time; returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationCrossedWeaponSweep: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(-80.0f, -90.0f, 60.0f);
            leftHandRotation = plasma::Vector<3, float>(-80.0f, 90.0f, -60.0f);
            rightHandOffset[0] -= 5.0f;
            rightHandOffset[1] += 3.0f;
            leftHandOffset[0] += 5.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation = plasma::Vector<3, float>(0.0f, 0.0f, -30.0f);
            leftHandOffset[1] -= 5.0f;
            rightHandOffset[1] -= 5.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 110.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -110.0f);
            creatureOffset[1] = -0.5f;
            blendRate = 0.04f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            leftHandRotation[0] = -90.0f;
            bodyRotation[2] = 20.0f;
            rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
            leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
            leftHandOffset[1] -= 4.0f;
            leftHandOffset[2] = leftHandOffset[2] + 2.0f;
            if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                rightHandPosition[1] += 2.0f;
                rightHandPosition[2] = rightHandPosition[2] + 3.0f;
                leftHandPosition = rightHandPosition;
                rightHandRotation[0] = 0.0f;
                rightHandRotation[1] = 20.0f;
                leftHandPosition[0] -= 3.0f;
                leftHandPosition[2] = leftHandPosition[2] - 3.0f;
                leftShoulderPosition[1] += 10.0f;
                leftShoulderPosition[2] = leftShoulderPosition[2] - 4.0f;
                leftHandRotation = rightHandRotation;
            }
        }
        break;
    }
    // Animation 36: poses the hands, the shoulders, the body, the whole creature and the hands' turn by the ability's
    // anticipation time; returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationAmbush: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 40.0f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -7.0f, 0.0f);
            bodyRotation[2] = -40.0f;
            creatureOffset[1] = -0.5f;
            leftFootSideways = 1;
            blendRate = 0.005f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -60.0f, -30.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 60.0f, -50.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 5.0f, 2.0f);
            leftHandOffset = plasma::Vector<3, float>(6.0f, 8.0f, 2.0f);
            bodyRotation[2] = 40.0f;
            leftFootSideways = 1;
            blendRate = 0.05f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -60.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] = leftHandOffset[2] + 2.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] = rightHandPosition[2] + 3.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] = leftHandPosition[2] - 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] = leftShoulderPosition[2] - 4.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    }
    // Animation 35: poses the right hand and the body.
    case AnimationIntercept: {
        rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, 80.0f);
        bodyRotation[2] = -80.0f;
        break;
    }
    // Animation 5: poses the hands and the body; with a motion that cycles with the animation time; sets the blend
    // rate.
    case AnimationMonsterCharge: {
        rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -40.0f);
        leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 40.0f);
        float cosine = cosf(m_animationTime * 0.5f);
        rightHandPosition[2] += cosine * 0.5f;
        float cosine2 = cosf(m_animationTime * 0.5f);
        leftHandPosition[2] += cosine2 * 0.5f;
        rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
        leftHandOffset = plasma::Vector<3, float>(-1.0f, -7.0f, 0.0f);
        bodyRotation[2] = -40.0f;
        blendRate = 0.005f;
        break;
    }
    // Animation 6: poses the hands and the body; with a motion that cycles with the animation time; sets the blend
    // rate.
    case AnimationSwirl: {
        if (m_creature->hasTwoHandedWeapon()) {
            rightHandRotation = plasma::Vector<3, float>(-40.0f, 60.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(-5.0f, 8.0f, 4.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 30.0f, -55.0f);
            leftHandOffset = rightHandOffset;
            leftHandRotation = rightHandRotation;
            leftHandPosition = rightHandPosition;
            float leftHandY = leftHandPosition[1] - 0.2f;
            leftHandPosition[0] -= 0.2f;
            leftHandPosition[2] -= 1.0f;
            leftHandPosition[1] = leftHandY;
        } else if (m_creature->m_entityData.specialization == cube::SpecializationGuardian) {
            rightHandRotation = plasma::Vector<3, float>(-50.0f, -60.0f, 40.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -50.0f);
            float cosine = cosf(m_animationTime * 0.5f);
            rightHandPosition[2] += cosine * 0.5f;
            float cosine2 = cosf(m_animationTime * 0.5f);
            leftHandPosition[2] += cosine2 * 0.5f;
            rightHandOffset = plasma::Vector<3, float>(-3.0f, 7.0f, 3.0f);
            leftHandOffset = plasma::Vector<3, float>(-2.0f, 2.0f, 0.0f);
            bodyRotation[2] = -20.0f;
            blendRate = 0.005f;
        } else {
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -40.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, -40.0f);
            float cosine = cosf(m_animationTime * 0.5f);
            rightHandPosition[2] += cosine * 0.5f;
            float cosine2 = cosf(m_animationTime * 0.5f);
            leftHandPosition[2] += cosine2 * 0.5f;
            rightHandOffset = plasma::Vector<3, float>(1.0f, -7.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, 0.0f, 1.0f);
            bodyRotation[2] = -40.0f;
            blendRate = 0.005f;
        }
        break;
    }
    // Animation 8: poses the hands, the shoulders and the body by fixed times.
    case AnimationParry: {
        if (m_leftWeaponSprite != nullptr) {
            if (100.0f > m_animationTime) {
                bodyRotation[2] = 80.0f;
                leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
                rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            } else if (500.0f > m_animationTime) {
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
            if (100.0f > m_animationTime) {
                bodyRotation[2] = -80.0f;
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            } else if (500.0f > m_animationTime) {
                bodyRotation[2] = 90.0f;
                rightHandRotation = plasma::Vector<3, float>(0.0f, 60.0f, -90.0f);
                leftHandOffset[0] += 2.0f;
                rightHandOffset[1] += 3.0f;
                rightHandRotation[2] += 5.0f;
                rightHandOffset[2] += 3.0f;
                rightHandOffset[0] += 2.0f;
                leftHandOffset[1] += 3.0f;
                leftHandOffset[2] += 3.0f;
            } else {
                rightHandPosition[2] += 3.0f;
                rightHandPosition[1] += 2.0f;
                leftHandPosition = rightHandPosition;
                rightHandRotation[0] = 0.0f;
                leftHandPosition[2] -= 3.0f;
                leftHandPosition[0] -= 3.0f;
                rightHandRotation[1] = 20.0f;
                leftShoulderPosition[2] -= 4.0f;
                leftShoulderPosition[1] += 10.0f;
            }
            leftHandPosition = rightHandPosition;
            leftHandPosition[2] -= 3.0f;
            leftHandPosition[0] -= 3.0f;
            leftHandRotation = rightHandRotation;
        }
        break;
    }
    // Animation 71: poses the hands, the body, the head and the whole creature by the ability's anticipation time.
    case AnimationThrow: {
        if ((float)m_creature->anticipationTime() > m_creature->m_entityData.timeSinceAbility) {
            bodyRotation[2] = 60.0f;
            creatureRotation[2] = 60.0f;
            creatureRotation[0] = -25.0f;
            rightHandOffset = plasma::Vector<3, float>(-6.0f, -3.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
        } else if ((float)m_creature->attackDuration() > m_creature->m_entityData.timeSinceAbility) {
            bodyRotation[2] = -30.0f;
            creatureRotation[2] = -30.0f;
            creatureRotation[0] = 10.0f;
            rightHandOffset = plasma::Vector<3, float>(3.0f, -4.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(0.0f, 2.0f, -1.0f);
        }
        break;
    }
    // Animation 28: poses the hands and the body; with a motion that cycles with the animation time.
    case AnimationFireballCharge: {
        bodyRotation[2] = -30.0f;
        rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
        leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        rightHandOffset[1] += 4.0f;
        leftHandOffset[1] += 4.0f;
        float sine = sinf(m_animationTime * 0.04f);
        rightHandOffset[0] += sine * 0.5f;
        float cosine = cosf(m_animationTime * 0.05f);
        rightHandOffset[2] += cosine * 0.5f;
        float sine2 = sinf(m_animationTime * 0.04f);
        leftHandOffset[0] += sine2 * 0.5f;
        float cosineWave = cosf(m_animationTime * 0.05f);
        leftHandOffset[2] += cosineWave * 0.5f;
        break;
    }
    // Animation 23: poses the hands and the body by the ability's anticipation time; with a motion that cycles with
    // the animation time.
    case AnimationFireball: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            float sineWave = sinf(m_animationTime * 0.04f);
            rightHandOffset[0] += sineWave * 0.5f;
            float cosineWave2 = cosf(m_animationTime * 0.05f);
            rightHandOffset[2] += cosineWave2 * 0.5f;
            float sineWave2 = sinf(m_animationTime * 0.04f);
            leftHandOffset[0] += sineWave2 * 0.5f;
            float cosineWave = cosf(m_animationTime * 0.05f);
            leftHandOffset[2] += cosineWave * 0.5f;
        } else {
            bodyRotation[2] = 60.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        }
        break;
    }
    // Animation 26: poses the hands and the body by the ability's anticipation time; with a motion that cycles with
    // the animation time; sets the blend rate.
    case AnimationFirebolt: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            float sineWave = sinf(m_animationTime * 0.04f);
            rightHandOffset[0] += sineWave * 0.5f;
            float cosineWave2 = cosf(m_animationTime * 0.05f);
            rightHandOffset[2] += cosineWave2 * 0.5f;
            float sineWave2 = sinf(m_animationTime * 0.04f);
            leftHandOffset[0] += sineWave2 * 0.5f;
            float cosineWave = cosf(m_animationTime * 0.05f);
            leftHandOffset[2] += cosineWave * 0.5f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
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
    }
    // Animation 85: poses the hands and the body while the ability lasts.
    case AnimationFireBreath: {
        if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[0] = -30.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 60.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, -3.0f, 1.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, -3.0f, 1.0f);
        } else {
            rightHandRotation = plasma::Vector<3, float>(-60.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 0.0f, 1.0f);
        }
        break;
    }
    // Animation 25: poses the hands, the body and the whole creature by the ability's anticipation time; with a motion
    // that cycles with the animation time.
    case AnimationRightCast: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            float sine = sinf(m_animationTime * 0.04f);
            rightHandOffset[0] += sine * 0.5f;
            float cosine = cosf(m_animationTime * 0.05f);
            rightHandOffset[2] += cosine * 0.5f;
            float sine2 = sinf(m_animationTime * 0.04f);
            leftHandOffset[0] += sine2 * 0.5f;
            float cosine2 = cosf(m_animationTime * 0.05f);
            bodyRotation[2] = -20.0f;
            leftHandOffset[2] += cosine2 * 0.5f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            creatureOffset[0] = -0.5f;
            bodyRotation[2] = 60.0f;
            creatureRotation[2] = 30.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            leftFootSideways = 1;
        }
        break;
    }
    // Animation 24: poses the hands, the body and the whole creature by the ability's anticipation time; with a motion
    // that cycles with the animation time.
    case AnimationLeftCast: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandOffset[1] += 4.0f;
            leftHandOffset[1] += 4.0f;
            float sine = sinf(m_animationTime * 0.04f);
            rightHandOffset[0] += sine * 0.5f;
            float cosine = cosf(m_animationTime * 0.05f);
            rightHandOffset[2] += cosine * 0.5f;
            float sine2 = sinf(m_animationTime * 0.04f);
            leftHandOffset[0] += sine2 * 0.5f;
            float cosine2 = cosf(m_animationTime * 0.05f);
            bodyRotation[2] = 20.0f;
            leftHandOffset[2] += cosine2 * 0.5f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = -60.0f;
            creatureOffset[0] = 0.5f;
            creatureRotation[2] = -30.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 60.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-2.0f, 0.0f, 0.0f);
            leftFootSideways = 1;
        }
        break;
    }
    // Animation 11: poses the hands and the body.
    case AnimationSupportRay: {
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagHeadAimOrigin) == 0) {
            bodyRotation[2] = 60.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, -60.0f);
            leftHandRotation = plasma::Vector<3, float>(-45.0f, -45.0f, 0.0f);
        }
        break;
    }
    // Animation 66: poses the hands, the body and the head by the ability's anticipation and strike times; with a
    // motion that cycles with the animation time; sets the blend rate.
    case AnimationCastSpell: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = 20.0f;
            float sine = sinf(m_animationTime * 0.005f);
            float cosine = cosf(m_animationTime * 0.005f);
            leftHandOffset = plasma::Vector<3, float>(cosine * 3.0f - 1.0f, 5.0f, sine * 5.0f + 5.0f);
            float sine2 = sinf((float)((double)(m_animationTime * 0.005f) + Pi));
            float cosine2 = cosf((float)((double)(m_animationTime * 0.005f) + Pi));
            rightHandOffset = plasma::Vector<3, float>(cosine2 * 3.0f + 1.0f, 5.0f, (sine2 * 5.0f + 5.0f));
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                bodyRotation[0] = -20.0f;
                bodyRotation[2] = 10.0f;
                leftHandOffset = plasma::Vector<3, float>(2.0f, 4.0f, 0.0f);
                rightHandOffset = plasma::Vector<3, float>(-2.0f, 6.0f, 0.0f);
                leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, 0.0f);
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                leftHandOffset = plasma::Vector<3, float>(-3.0f, -6.0f, 0.0f);
                rightHandOffset = plasma::Vector<3, float>(3.0f, -6.0f, 0.0f);
                leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 130.0f);
                rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -130.0f);
                blendRate = 0.03f;
            }
        }
        break;
    }
    // Animation 67: poses the hands, the body, the head, the whole creature and the feet by the ability's anticipation
    // and strike times; with a motion that cycles with the animation time; sets the blend rate.
    case AnimationHealingStream: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[0] = 20.0f;
            headRotation[0] = 20.0f;
            float sine = sinf(m_animationTime * 0.005f);
            float cosine = cosf(m_animationTime * 0.005f);
            leftHandOffset = plasma::Vector<3, float>(cosine * 3.0f - 1.0f, 5.0f, sine * 5.0f + 5.0f);
            float sine2 = sinf((float)((double)(m_animationTime * 0.005f) + Pi));
            float cosine2 = cosf((float)((double)(m_animationTime * 0.005f) + Pi));
            rightHandOffset = plasma::Vector<3, float>(cosine2 * 3.0f + 1.0f, 5.0f, sine2 * 5.0f + 5.0f);
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation + strike) > m_animationTime) {
                bodyRotation[0] = -20.0f;
                bodyRotation[2] = 10.0f;
                leftHandOffset = plasma::Vector<3, float>(2.0f, 4.0f, 0.0f);
                rightHandOffset = plasma::Vector<3, float>(-2.0f, 6.0f, 0.0f);
                leftHandRotation = plasma::Vector<3, float>(0.0f, 45.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, -45.0f, 0.0f);
            } else if ((float)m_creature->attackDuration() > m_animationTime) {
                leftHandOffset = plasma::Vector<3, float>(-3.0f, -6.0f, 0.0f);
                rightHandOffset = plasma::Vector<3, float>(3.0f, -6.0f, 0.0f);
                leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 130.0f);
                rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -130.0f);
                blendRate = 0.03f;
                blendRate = 0.03f;
            }
        }
        if ((float)m_creature->attackDuration() > m_animationTime) {
            float cosine = cosf(m_animationTime * 0.003f);
            creatureOffset[2] = (float)((double)cosine * 0.2 + 0.800000011920929);
            feetTurn = -30.0f;
            leftFootPosition[1] += 4.0f;
            rightFootPosition[1] += 4.0f;
        }
        break;
    }
    // Animation 68: poses the hands, the body and the head; with a motion that cycles with the animation time.
    case Animation_68: {
        bodyRotation[0] = 20.0f;
        headRotation[0] = 20.0f;
        float sine = sinf(m_animationTime * 0.005f);
        float cosine = cosf(m_animationTime * 0.005f);
        leftHandOffset = plasma::Vector<3, float>(cosine * 3.0f - 1.0f, 5.0f, sine * 5.0f + 5.0f);
        float sine2 = sinf((float)((double)(m_animationTime * 0.005f) + Pi));
        float cosine2 = cosf((float)((double)(m_animationTime * 0.005f) + Pi));
        rightHandOffset = plasma::Vector<3, float>(cosine2 * 3.0f + 1.0f, 5.0f, (sine2 * 5.0f + 5.0f));
        break;
    }
    // Animation 9: poses the hands, the body and the whole creature by the ability's anticipation time.
    case AnimationBlock: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = 80.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            if (m_creature->m_entityData.currentAbility == AbilityIntercept) {
                creatureRotation[2] = m_animationTime / -300.0f * 360.0f;
            }
        } else {
            bodyRotation[2] = -90.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 45.0f);
        }
        break;
    }
    // Animation 74: poses the hands and the body by the ability's anticipation time.
    case AnimationBulwark: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
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
    }
    // Animation 10: poses the hands, the shoulders, the body, the whole creature and the hands' turn by the ability's
    // anticipation time; with a motion that cycles with the animation time; returns to animation 0 when the ability is
    // over; sets the blend rate.
    case AnimationBlockJab: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = 80.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-45.0f, 45.0f, 0.0f);
            if (m_creature->m_entityData.currentAbility == AbilityIntercept) {
                creatureRotation[2] = m_animationTime / -300.0f * 360.0f;
            }
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = -90.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 45.0f);
            float sine = sinf((m_animationTime - 600.0f) * 0.1f);
            leftHandPosition[0] += sine;
            blendRate = 0.03f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield) {
                    leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
                } else {
                    leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                }
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] = leftHandOffset[2] + 2.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] = rightHandPosition[2] + 3.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] = leftHandPosition[2] - 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] = leftShoulderPosition[2] - 4.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    }
    // Animation 20: poses the hands, the body, the head and the whole creature by the ability's anticipation time;
    // with a motion that cycles with the animation time; sets the blend rate.
    case AnimationRightStab: {
        if (!(!(m_animationTime > (float)m_creature->anticipationTime()) ||
              !((float)m_creature->attackDuration() > m_animationTime))) {
            bodyRotation[2] = 45.0f;
            creatureRotation[2] = 45.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(8.0f, 0.0f, 4.0f);
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -30.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyOffset[1] = 3.0f;
        } else {
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float cosine = cosf(m_animationTime * 0.01f);
            float sine = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            float cosine2 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine2 + 4.0f);
            float sineWave = sinf(m_animationTime * 0.01f);
            bodyOffset[2] += sineWave * 0.5f;
        }
        blendRate = 0.03f;
        break;
    }
    // Animation 19: poses the hands, the body, the head and the whole creature by the ability's anticipation time;
    // with a motion that cycles with the animation time; sets the blend rate.
    case AnimationLeftStab: {
        if (!(!(m_animationTime > (float)m_creature->anticipationTime()) ||
              !((float)m_creature->attackDuration() > m_animationTime))) {
            bodyRotation[2] = -20.0f;
            creatureRotation[2] = -20.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 40.0f);
            leftHandOffset = plasma::Vector<3, float>(0.0f, 8.0f, 4.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, 90.0f, 30.0f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyOffset[1] = 3.0f;
        } else {
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float cosine = cosf(m_animationTime * 0.01f);
            float sine = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            float cosine2 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine2 + 4.0f);
            float sine3 = sinf(m_animationTime * 0.005f);
            bodyOffset[2] += sine3 * 0.5f;
        }
        blendRate = 0.03f;
        break;
    }
    // Animation 88: poses the hands, the body and the whole creature; with a motion that cycles with the animation
    // time.
    case AnimationRockFistCharge: {
        bodyRotation[2] = -60.0f;
        creatureRotation[2] = 0.0f;
        rightHandRotation = plasma::Vector<3, float>(60.0f, 40.0f, 20.0f);
        float cosine = cosf(m_animationTime * 0.01f);
        float sine = sinf(m_animationTime * 0.01f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine);
        float cosine2 = cosf(m_animationTime * 0.5f);
        rightHandPosition[2] += cosine2 * 0.5f;
        leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
        float cosine3 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
        float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
        leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine3 + 4.0f);
        float sine3 = sinf(m_animationTime * 0.01f);
        bodyOffset[2] += sine3 * 0.5f;
        break;
    }
    // Animation 87: poses the hands, the body, the head and the whole creature by the ability's anticipation time;
    // with a motion that cycles with the animation time; sets the blend rate.
    case AnimationRockFist: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -60.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, 40.0f, 20.0f);
            float cosine = cosf(m_animationTime * 0.01f);
            float sine = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine);
            float cosine2 = cosf(m_animationTime * 0.5f);
            rightHandPosition[2] += cosine2 * 0.5f;
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            float cosine3 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine3 + 4.0f);
            float sineWave = sinf(m_animationTime * 0.01f);
            bodyOffset[2] += sineWave * 0.5f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = 45.0f;
            creatureRotation[2] = 45.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -60.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(8.0f, 0.0f, 4.0f);
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -30.0f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, -4.0f, -1.0f);
            headRotation[0] = -10.0f;
            bodyOffset[1] = 3.0f;
            leftFootSideways = 1;
        } else {
            bodyRotation[2] = 0.0f;
            creatureRotation[2] = 0.0f;
            rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
            float cosine = cosf(m_animationTime * 0.01f);
            float sine = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine + 4.0f);
            leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
            float cosine2 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
            leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine2 + 4.0f);
        }
        blendRate = 0.03f;
        break;
    }
    // Animation 31: poses the hands, the body, the head, the whole creature and the feet by the ability's
    // anticipation, strike and hold times; sets the blend rate.
    case AnimationLeftSweep: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            rightHandRotation = plasma::Vector<3, float>(-80.0f, 90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(-10.0f, 8.0f, 8.0f);
            bodyRotation[2] = -45.0f;
            creatureRotation[2] = m_animationTime / (float)anticipation * -20.0f;
            creatureOffset[0] = m_animationTime / (float)anticipation * 0.0f;
            if (m_creature->m_strikeCombo / 2 % 3 == 2) {
                int strikeLength = m_creature->strikeTime();
                int anticipationLength = m_creature->anticipationTime();
                if ((float)(anticipationLength + strikeLength / 2) > m_animationTime) {
                    creatureRotation[2] += m_animationTime / (float)anticipation * 360.0f;
                }
            }
            blendRate = 0.04f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            int hold = m_creature->holdTime();
            if ((float)(hold + (anticipation + strike)) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                float fraction = std::min(1.0f, (m_animationTime - (float)anticipation2) * 1.2f / (float)strike2);
                creatureOffset[0] = (1.0f - fraction) * 0.0f - fraction * 0.8f;
                rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(-90.0f, 90.0f, 48.0f)) +
                                    (fraction * plasma::Vector<3, float>(-110.0f, 60.0f, 0.0f));
                rightHandOffset = ((1.0f - fraction) * plasma::Vector<3, float>(-10.0f, 8.0f, 8.0f)) +
                                  (fraction * plasma::Vector<3, float>(-12.0f, 0.0f, -1.0f));
                headRotation[0] = -10.0f;
                bodyRotation[2] = fraction * 90.0f - (1.0f - fraction) * 45.0f;
                creatureRotation[2] = fraction * 30.0f;
                footRoll = fraction * 10.0f;
                blendRate = 0.05f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                bodyRotation[2] = -30.0f;
                blendRate = 0.03f;
            }
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        float leftHandY = leftHandPosition[1] - 0.2f;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        leftHandPosition[1] = leftHandY;
        if (m_creature->m_strikeCombo / 2 % 3 == 2) {
            m_creature->anticipationTime();
            m_creature->strikeTime();
        }
        break;
    }
    // Animation 32: poses the hands, the body, the head, the whole creature and the feet by the ability's anticipation
    // and strike times; sets the blend rate.
    case AnimationRightSweep: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-8.0f, 6.0f, 6.0f);
            creatureOffset[0] = -0.0f;
            headRotation[0] = -10.0f;
            headRotation[2] = 30.0f;
            bodyRotation[2] = 45.0f;
            creatureRotation[2] = 20.0f;
            int anticipation = m_creature->anticipationTime();
            if (m_creature->m_strikeCombo / 2 % 3 == 2) {
                int strikeLength = m_creature->strikeTime();
                int anticipationLength = m_creature->anticipationTime();
                if ((float)(anticipationLength + strikeLength / 2) > m_animationTime) {
                    creatureRotation[2] = creatureRotation[2] - m_animationTime / (float)anticipation * 360.0f;
                }
            }
            blendRate = 0.05f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            headRotation[2] = -30.0f;
            float fraction = std::min(1.0f, (m_animationTime - (float)anticipation) * 1.2f / (float)strike);
            creatureOffset[0] = fraction * 0.8f + (1.0f - fraction) * -0.0f;
            rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(0.0f, -90.0f, 20.0f)) +
                                (fraction * plasma::Vector<3, float>(0.0f, -90.0f, -130.0f));
            plasma::Vector<3, float> vector = fraction * plasma::Vector<3, float>(0.0f, 2.0f, 0.0f);
            float remaining = 1.0f - fraction;
            float progress = fraction;
            rightHandOffset = ((1.0f - fraction) * plasma::Vector<3, float>(-8.0f, 6.0f, 6.0f)) + vector;
            creatureRotation[0] = progress * -20.0f;
            creatureRotation[2] = progress * -30.0f;
            bodyRotation[2] = remaining * 45.0f - progress * 45.0f;
            footRoll = progress * 10.0f;
            blendRate = 0.05f;
        } else {
            rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
            plasma::Vector<3, float> vector;
            vector = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            rightHandRotation = vector;
            bodyRotation[2] = -30.0f;
            blendRate = 0.03f;
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] = leftHandPosition[1] - 0.2f;
        leftHandPosition[2] = leftHandPosition[2] - 1.0f;
        break;
    }
    // Animation 33: poses the hands, the body and the whole creature by the ability's anticipation, strike and hold
    // times; sets the blend rate.
    case AnimationOverheadChop: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            int anticipation = m_creature->anticipationTime();
            rightHandOffset = plasma::Vector<3, float>(2.0f, 2.0f, 14.0f);
            rightHandRotation = plasma::Vector<3, float>(50.0f, 0.0f, 0.0f);
            bodyRotation[2] = -45.0f;
            creatureRotation[2] = m_animationTime / (float)anticipation * -20.0f;
            creatureOffset[0] = m_animationTime / (float)anticipation * 0.5f;
            blendRate = 0.04f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            int hold = m_creature->holdTime();
            if ((float)(hold + (anticipation + strike)) > m_animationTime) {
                int anticipation2 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                float fraction =
                    std::min(1.0f, (m_animationTime - (float)anticipation2 + (m_animationTime - (float)anticipation2)) /
                                       (float)strike2);
                creatureRotation[0] = (1.0f - fraction) * 20.0f - fraction * 10.0f;
                rightHandRotation = ((1.0f - fraction) * plasma::Vector<3, float>(90.0f, 0.0f, 45.0f)) +
                                    (fraction * plasma::Vector<3, float>(-90.0f, 0.0f, -45.0f));
                rightHandOffset = ((1.0f - fraction) * plasma::Vector<3, float>(2.0f, 2.0f, 14.0f)) +
                                  (fraction * plasma::Vector<3, float>(0.0f, 9.0f, 0.0f));
                bodyRotation[2] = fraction * 45.0f - (1.0f - fraction) * 45.0f;
                blendRate = 0.05f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                bodyRotation[2] = -30.0f;
                blendRate = 0.03f;
            }
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[1] = leftHandPosition[1] - 0.2f;
        leftHandPosition[2] = leftHandPosition[2] - 1.0f;
        break;
    }
    // Animation 34: poses the hands, the body and the whole creature by the ability's anticipation and strike times;
    // sets the blend rate.
    case AnimationPerforate: {
        float rate;
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            rightHandOffset = plasma::Vector<3, float>(4.0f, 2.0f, 8.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -180.0f, 90.0f);
            bodyRotation[2] = -90.0f;
            creatureOffset[1] = -1.0f;
            rate = 0.02f;
        } else {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            float phaseRate;
            if ((float)(anticipation + strike) > m_animationTime) {
                rightHandOffset = plasma::Vector<3, float>(6.0f, 0.0f, 0.0f);
                creatureOffset[1] = 0.5f;
                bodyRotation[2] = 90.0f;
                rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -90.0f);
                leftFootSideways = 1;
                phaseRate = 0.05f;
            } else {
                rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
                rightHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                bodyRotation[2] = -30.0f;
                phaseRate = 0.03f;
            }
            rate = phaseRate;
        }
        leftHandOffset = rightHandOffset;
        leftHandRotation = rightHandRotation;
        leftHandPosition = rightHandPosition;
        float leftHandY = leftHandPosition[1] - 0.2f;
        leftHandPosition[0] -= 0.2f;
        leftHandPosition[2] -= 1.0f;
        leftHandPosition[1] = leftHandY;
        blendRate = rate;
        break;
    }
    // Animation 18: poses the hands, the shoulders, the body and the hands' turn by the ability's anticipation time;
    // returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationLeftBackhand: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = -40.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -30.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 20.0f);
            rightHandOffset[1] -= 6.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = 90.0f;
            float leftHandAngleX;
            float leftHandAngleZ;
            if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield) {
                leftHandAngleX = 0.0f;
                leftHandAngleZ = 40.0f;
            } else {
                leftHandAngleX = -40.0f;
                leftHandAngleZ = -30.0f;
            }
            leftHandRotation = plasma::Vector<3, float>(leftHandAngleX, 0.0f, leftHandAngleZ);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, 0.0f, 4.0f);
            rightHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -90.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, -1.0f, 0.0f);
            leftFootSideways = 1;
            blendRate = 0.05f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield) {
                    leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
                } else {
                    leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                }
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] = leftHandOffset[2] + 2.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] = rightHandPosition[2] + 3.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] = leftHandPosition[2] - 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] = leftShoulderPosition[2] - 4.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    }
    // Animation 17: poses the hands, the shoulders, the body and the hands' turn by the ability's anticipation time;
    // returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationRightBackhand: {
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = 40.0f;
            rightHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 30.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, -20.0f);
            leftHandOffset[1] -= 6.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = -90.0f;
            rightHandRotation = plasma::Vector<3, float>(-40.0f, 0.0f, 30.0f);
            rightHandOffset = plasma::Vector<3, float>(3.0f, 0.0f, 4.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 90.0f, 90.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, -1.0f, 0.0f);
            leftFootSideways = 1;
            blendRate = 0.05f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[1] -= 4.0f;
                leftHandOffset[2] = leftHandOffset[2] + 2.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[1] += 2.0f;
                    rightHandPosition[2] = rightHandPosition[2] + 3.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftHandPosition[2] = leftHandPosition[2] - 3.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftShoulderPosition[2] = leftShoulderPosition[2] - 4.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        break;
    }
    // Animation 51: poses the hands, the body, the head, the whole creature and the feet by the ability's anticipation
    // and strike times; with a motion that cycles with the animation time; sets the blend rate.
    case AnimationKick: {
        rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
        rightHandOffset = plasma::Vector<3, float>(0.0f, 2.0f, 4.0f);
        leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
        leftHandOffset = plasma::Vector<3, float>(0.0f, 2.0f, 4.0f);
        int anticipation = m_creature->anticipationTime();
        if ((float)anticipation > m_animationTime) {
            bodyRotation[2] = 50.0f;
            leftHandRotation[0] = -90.0f;
            leftHandRotation[2] = 90.0f;
            rightHandRotation[0] = -90.0f;
            leftHandOffset = plasma::Vector<3, float>(-4.0f, -4.0f, -2.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 1.0f);
        } else {
            int anticipation2 = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            if ((float)(anticipation2 + strike) > m_animationTime) {
                int anticipation3 = m_creature->anticipationTime();
                int strike2 = m_creature->strikeTime();
                leftHandRotation[0] = -90.0f;
                leftHandRotation[2] = 90.0f;
                rightHandRotation[0] = -90.0f;
                float fraction = std::min(1.0f, (m_animationTime - (float)anticipation3) / (float)strike2 * 1.5f);
                fraction = std::max(0.0f, fraction);
                creatureRotation[2] = fraction * -270.0f;
                creatureRotation[1] = fraction * -40.0f;
                footRoll = fraction * 40.0f;
                bodyOffset[2] += 4.0f;
                leftHandOffset = plasma::Vector<3, float>(-4.0f, -4.0f, -2.0f);
                rightHandOffset = plasma::Vector<3, float>(0.0f, 4.0f, 1.0f);
                headRotation[2] = -60.0f;
                headRotation[0] = -30.0f;
                blendRate = 0.03f;
            } else {
                bodyRotation[2] = 0.0f;
                creatureRotation[2] = 0.0f;
                rightHandRotation = plasma::Vector<3, float>(60.0f, -20.0f, 20.0f);
                float cosine = cosf(m_animationTime * 0.01f);
                float sine = sinf(m_animationTime * 0.01f);
                rightHandOffset = plasma::Vector<3, float>(0.0f, sine + 2.0f, cosine + 4.0f);
                leftHandRotation = plasma::Vector<3, float>(60.0f, 20.0f, -20.0f);
                float cosine2 = cosf((float)((double)(m_animationTime * 0.01f) + HalfPi));
                float sine2 = sinf((float)((double)(m_animationTime * 0.01f) + HalfPi));
                leftHandOffset = plasma::Vector<3, float>(0.0f, sine2 + 2.0f, cosine2 + 4.0f);
                float sine3 = sinf(m_animationTime * 0.005f);
                bodyOffset[2] += sine3 * 0.5f;
            }
        }
        break;
    }
    // Animation 15: poses the hands, the shoulders, the body, the whole creature and the hands' turn by the ability's
    // anticipation and strike times; returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationLeftSlash: {
        if (m_creature->m_strikeCombo / 2 % 3 == 2) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            float spin = std::min(1.0f, (m_animationTime - (float)anticipation) / (float)strike);
            creatureRotation[2] = std::max(0.0f, spin) * -360.0f;
            blendRate = 0.03f;
        } else {
            creatureRotation[2] = 0.0f;
        }
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        int anticipation = m_creature->anticipationTime();
        if ((float)anticipation > m_animationTime) {
            bodyRotation[2] = 40.0f;
            leftHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 30.0f);
            leftHandOffset[2] += 4.0f;
            leftHandOffset[1] -= 4.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -120.0f);
            rightHandOffset[1] -= 6.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            creatureOffset[0] = -0.1f;
            bodyRotation[2] = -90.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -90.0f, -30.0f);
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -120.0f);
            rightHandOffset[1] -= 6.0f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -90.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[2] += 2.0f;
                leftHandOffset[1] -= 4.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[2] += 3.0f;
                    rightHandPosition[1] += 2.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield) {
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
        }
        break;
    }
    // Animation 16: poses the hands, the shoulders, the body, the whole creature, the feet and the hands' turn by the
    // ability's anticipation and strike times; returns to animation 0 when the ability is over; sets the blend rate.
    case AnimationRightSlash: {
        handsTurn = m_creature->m_entityData.finalHandsRotation;
        if (m_creature->m_strikeCombo / 2 % 3 == 1) {
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            float spin = std::min(1.0f, (m_animationTime - (float)anticipation) / (float)strike);
            creatureRotation[2] = std::max(0.0f, spin) * -360.0f;
            blendRate = 0.03f;
        } else {
            creatureRotation[2] = 0.0f;
        }
        if ((float)m_creature->anticipationTime() > m_animationTime) {
            bodyRotation[2] = 90.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
            leftHandRotation = plasma::Vector<3, float>(-90.0f, 0.0f, 90.0f);
            rightHandOffset[2] += 4.0f;
            rightHandOffset[1] += 6.0f;
        } else if ((float)m_creature->attackDuration() > m_animationTime) {
            bodyRotation[2] = 20.0f;
            leftHandRotation = plasma::Vector<3, float>(-90.0f, -60.0f, 90.0f);
            leftHandOffset[2] += 4.0f;
            leftHandOffset[1] -= 4.0f;
            rightHandRotation = plasma::Vector<3, float>(0.0f, -90.0f, -220.0f);
            creatureOffset[0] = 0.1f;
            rightHandOffset[1] -= 6.0f;
            leftFootPosition[1] -= 4.0f;
            leftFootSideways = 1;
            rightFootPosition[1] += 4.0f;
        } else {
            handsTurn = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_animation = AnimationNone;
            if (!(m_creature->m_entityData.race == RaceZombie || m_creature->m_entityData.race == RaceMole)) {
                leftHandRotation[0] = -60.0f;
                bodyRotation[2] = 20.0f;
                rightHandRotation = plasma::Vector<3, float>(-120.0f, -90.0f, 30.0f);
                leftHandRotation = plasma::Vector<3, float>(-80.0f, 40.0f, 60.0f);
                leftHandOffset[2] += 2.0f;
                leftHandOffset[1] -= 4.0f;
                if (!(m_rightWeaponSprite == nullptr || m_leftWeaponSprite != nullptr)) {
                    rightHandPosition[2] += 3.0f;
                    rightHandPosition[1] += 2.0f;
                    leftHandPosition = rightHandPosition;
                    rightHandRotation[0] = 0.0f;
                    rightHandRotation[1] = 20.0f;
                    leftHandPosition[2] -= 3.0f;
                    leftHandPosition[0] -= 3.0f;
                    leftShoulderPosition[2] -= 4.0f;
                    leftShoulderPosition[1] += 10.0f;
                    leftHandRotation = rightHandRotation;
                }
            }
        }
        if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponShield) {
            leftHandRotation = plasma::Vector<3, float>(0.0f, 0.0f, 90.0f);
        }
        break;
    }
    // Animation 61: poses the hands, the body and the whole creature.
    case AnimationActionLocked: {
        if (m_creature->m_entityData.currentAbility != AbilityRidePet) {
            creatureRotation[0] += 20.0f;
            bodyRotation[0] += 30.0f;
        }
        leftHandOffset = plasma::Vector<3, float>(-2.0f, 0.0f, 0.0f);
        rightHandOffset = plasma::Vector<3, float>(2.0f, 0.0f, 0.0f);
        leftHandRotation = plasma::Vector<3, float>(-10.0f, -10.0f, 90.0f);
        plasma::Vector<3, float> vector = plasma::Vector<3, float>(-10.0f, 10.0f, -90.0f);
        rightHandRotation = vector;
        break;
    }
    // Animation 84: poses the whole creature.
    case AnimationRestHeal: {
        creatureOffset[2] = m_creature->m_entityData.appearance.size[2] * -0.65f;
        break;
    }
    // Animation 90: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationPlayFlute: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.003f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 10.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.002f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 5.0f, sine * 5.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(12.0f, 5.0f, 5.0f);
            rightHandOffset = plasma::Vector<3, float>(7.0f, 5.0f, 5.0f);
        }
        break;
    }
    // Animation 93: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationPlayWhistle: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.003f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 10.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.003f);
            float sine2 = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.002f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 5.0f, sine2 * 5.0f, sine * 15.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(4.0f, 9.0f, 3.5f);
            rightHandOffset = plasma::Vector<3, float>(-4.0f, 13.0f, 3.5f);
        }
        break;
    }
    // Animation 98: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationCallPet: {
        if (1000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.003f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 10.0f, 0.0f);
            bodyRotation = plasma::Vector<3, float>(10.0f, 0.0f, 20.0f);
            rightHandRotation[2] = -20.0f;
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-3.0f, -2.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(0.0f, 8.0f, 2.5f);
        }
        break;
    }
    // Animation 91: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationPlayHarp: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.002f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 5.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.0015f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 2.0f, sine * 10.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-5.0f, 7.0f, 4.0f);
            float sine2 = sinf(m_animationTime * 0.0075f);
            float cosine3 = cosf(m_animationTime * 0.0075f);
            rightHandOffset = plasma::Vector<3, float>(cosine3 * 2.0f - 7.0f, sine2 * 2.0f + 10.0f, 3.0f);
        }
        break;
    }
    // Animation 94: poses the right hand, the body, the head and the whole creature; with a motion that cycles with
    // the animation time.
    case AnimationPlayBell: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.002f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 5.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.0015f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 2.0f, sine * 10.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            float sine2 = sinf(m_animationTime * 0.0075f);
            float cosine3 = cosf(m_animationTime * 0.0075f);
            rightHandOffset = plasma::Vector<3, float>(cosine3 * 2.0f - 5.0f, sine2 * 2.0f + 10.0f, 5.0f);
            float sine3 = sinf(m_animationTime * 0.01f);
            rightHandRotation[1] = sine3 * 40.0f;
        }
        break;
    }
    // Animation 92: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationPlayFiddle: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.002f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 5.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.0015f);
            float sine2 = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.0015f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 2.0f, sine2 * 3.0f, sine * 15.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(7.0f, 21.0f, 3.0f);
            float cosine3 = cosf(m_animationTime * 0.0075f);
            rightHandOffset = plasma::Vector<3, float>(cosine3 * 0.5f + 5.0f, 11.0f, 4.0f);
        }
        break;
    }
    // Animation 95: poses the hands, the body, the head and the whole creature; with a motion that cycles with the
    // animation time.
    case AnimationPlayLute: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.002f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 5.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.0015f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 2.0f, sine * 10.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            leftHandOffset = plasma::Vector<3, float>(-10.0f, 6.0f, 1.0f);
            float sine2 = sinf(m_animationTime * 0.0075f);
            float cosine3 = cosf(m_animationTime * 0.0075f);
            rightHandOffset = plasma::Vector<3, float>(cosine3 - 10.0f, 9.0f, sine2 * 2.0f + 1.0f);
        }
        break;
    }
    // Animation 96: poses the right hand, the body, the head and the whole creature; with a motion that cycles with
    // the animation time.
    case AnimationPlayNightingale: {
        if (5000.0f > m_creature->m_entityData.timeSinceAbility) {
            float cosine = cosf(m_animationTime * 0.002f);
            headRotation = plasma::Vector<3, float>(0.0f, cosine * 5.0f, 0.0f);
            float sine = sinf(m_animationTime * 0.002f);
            float cosine2 = cosf(m_animationTime * 0.0015f);
            bodyRotation = plasma::Vector<3, float>(cosine2 * 2.0f, sine * 10.0f, 0.0f);
            creatureRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-5.0f, 10.0f, 6.0f);
        }
        break;
    }
    // Animation 97: poses the hands, the body and the head by fixed times; with a motion that cycles with the
    // animation time.
    case AnimationFlightToPoint: {
        if (m_animationTime > 2000.0f) {
            float cosine = cosf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(1.0f, 0.0f, cosine * 5.0f);
            float sine = sinf(m_animationTime * 0.01f);
            leftHandOffset = plasma::Vector<3, float>(-1.0f, 0.0f, sine * 5.0f);
            float sine2 = sinf(m_animationTime * 0.003f);
            float cosine2 = cosf(m_animationTime * 0.003f);
            headRotation = plasma::Vector<3, float>(-30.0f, cosine2 * 5.0f, sine2 * 10.0f);
            float sine3 = sinf(m_animationTime * 0.002f);
            bodyRotation = plasma::Vector<3, float>(-15.0f, sine3 * 10.0f, 0.0f);
        } else {
            float sine = sinf(m_animationTime * 0.005f);
            float cosine = cosf(m_animationTime * 0.003f);
            headRotation = plasma::Vector<3, float>(40.0f, cosine * 5.0f, sine * 10.0f);
            float sine2 = sinf(m_animationTime * 0.005f);
            bodyRotation = plasma::Vector<3, float>(10.0f, sine2 * 10.0f, 0.0f);
            rightHandOffset = plasma::Vector<3, float>(-4.0f, 2.0f, 14.0f);
        }
        break;
    }
    // Animation 100: poses the right hand and the body while the ability lasts; with a motion that cycles with the
    // animation time.
    case AnimationWave: {
        if ((float)m_creature->attackDuration() > m_animationTime) {
            float sine = sinf(m_animationTime * 0.0075f);
            bodyRotation = plasma::Vector<3, float>(10.0f, sine * 10.0f, 0.0f);
            float sine2 = sinf((float)((double)(bodyRotation[1] * 2.0f) * Pi / 180.0));
            rightHandOffset = plasma::Vector<3, float>(sine2 * 8.0f, 2.0f, 15.0f);
        }
        break;
    }
    // Animation 101: poses the right hand and the body while the ability lasts; with a motion that cycles with the
    // animation time.
    case AnimationPetAnimal: {
        if ((float)m_creature->attackDuration() > m_animationTime) {
            float sine = sinf(m_animationTime * 0.005f);
            float cosine = cosf(m_animationTime * 0.003f);
            bodyRotation = plasma::Vector<3, float>(cosine * 5.0f - 25.0f, sine * 6.0f, 0.0f);
            bodyOffset[1] = 5.0f;
            float sine2 = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(sine2 * 4.0f, 6.0f, 10.0f);
            rightHandRotation[0] = 20.0f;
        }
        break;
    }
    // Animation 102: poses the hands, the body, the whole creature and the feet while the ability lasts; with a motion
    // that cycles with the animation time.
    case AnimationDance: {
        if ((float)m_creature->attackDuration() > m_animationTime) {
            float sine = sinf(m_animationTime * 0.005f);
            float sine2 = sinf(m_animationTime * 0.002f);
            float cosine = cosf(m_animationTime * 0.003f);
            bodyRotation = plasma::Vector<3, float>(cosine * 10.0f, sine * 30.0f, sine2 * 10.0f);
            float sine3 = sinf((float)((double)(sine * 30.0f) * Pi / 180.0));
            bodyOffset[0] = sine3 * 5.0f;
            float sine4 = sinf(m_animationTime * 0.01f);
            rightHandOffset = plasma::Vector<3, float>(sine4 * 4.0f + 2.0f, 6.0f, 5.0f);
            float sine5 = sinf(m_animationTime * 0.01f);
            leftHandOffset = plasma::Vector<3, float>(sine5 * 4.0f - 2.0f, 6.0f, 5.0f);
            float cosine2 = cosf(m_animationTime * 0.0045f);
            feetTurn = cosine2 * 4.0f - 4.0f;
            float cosine3 = cosf((float)((double)(cosine2 * 4.0f - 4.0f) * Pi / 180.0));
            creatureOffset[2] = 1.0f - cosine3;
        }
        break;
    }
    default:
        break;
    }

    // --- Corrections to the target pose ---
    if (ability == AbilityDash) {
        rightFootSwing = 30.0f;
        leftFootSideways = true;
    }
    if (m_animation == AnimationNone)
        m_animationTime = 0;
    leftShoulderPosition = 0.5f * (leftShoulderPosition + leftHandPosition);
    rightShoulderPosition = 0.5f * (rightShoulderPosition + rightHandPosition);
    // Stunned: the creature lies down, a creature with AppearanceFlagQuadruped on its side.
    if (m_creature->m_entityData.stunTime > 0.0f) {
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0) {
            creatureRotation = plasma::Vector<3, float>(0.0f, 90.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[0] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        } else {
            creatureRotation = plasma::Vector<3, float>(90.0f, 0.0f, 0.0f);
            creatureOffset[2] =
                (m_creature->m_entityData.appearance.size[1] - m_creature->m_entityData.appearance.size[2]) * 0.5f;
        }
    }
    // AbilityClimb with CreatureFlagAtWall: the hands are raised.
    if (ability == AbilityClimb && (m_creature->m_entityData.flags & CreatureFlagAtWall) != 0) {
        rightHandOffset[2] += 7.0f;
        leftHandOffset[2] += 7.0f;
        leftHandRotation[0] += 90.0f;
        rightHandRotation[0] += 90.0f;
    }
    // A left-hand weapon of id 20 is held up in front.
    if (m_creature->m_entityData.equipment.weaponLeft.id == WeaponTorch) {
        leftHandRotation[0] = 0.0f;
        leftHandOffset[1] += 4.0f;
        leftHandOffset[2] += 4.0f;
    }
    // The lean of a creature on two legs: far forward at speed, a little when it moves the way it accelerates; not
    // during abilities 118, 119, 121, 128, 156 and 157.
    if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) == 0 &&
        !(ability == AbilityRidePet || ability == AbilityRideNpcMount || ability == AbilitySailBoat ||
          ability == AbilityCarryCreature || ability == AbilityEagleFlight || ability == AbilityMenuView)) {
        if (leaningForward) {
            creatureRotation[0] -= 25.0f;
            headRotation[0] += 10.0f;
            if (ability == AbilitySprint || ability == AbilityDash || ability == AbilityFlameRush) {
                creatureRotation[0] -= 20.0f;
                leftHandOffset[1] -= 5.0f;
                rightHandOffset[1] -= 5.0f;
            }
            if (ability == AbilityDash)
                bodyRotation[2] = 30.0f;
        } else {
            plasma::Vector<2, float> groundSpeed(m_creature->m_entityData.velocity[0],
                                                 m_creature->m_entityData.velocity[1]);
            if (groundSpeed.squaredLength() > 0.1f &&
                (m_creature->m_entityData.acceleration & m_creature->m_entityData.velocity) > 0.0f) {
                creatureRotation[0] -= 10.0f;
                headRotation[0] += 5.0f;
            }
        }
    }
    // Objects that are creatures, posed by their state (EntityData::interactionState): a gate is raised, the lid of a
    // chest (its head sprite) opens, a lever tips over, a cage's door (its head sprite) swings open.
    if (m_creature->isGate()) {
        if (m_creature->m_entityData.interactionState != 0)
            creatureOffset[2] = 10.0f;
        else
            creatureOffset[0] = 0.0f;
    }
    if (m_creature->isChest()) {
        headRotation[0] = 45.0f;
        if (m_creature->m_entityData.interactionState != 2)
            headRotation[0] = 0.0f;
    }
    if (m_creature->isLever()) {
        headRotation[0] = 45.0f;
        if (m_creature->m_entityData.interactionState == 0)
            headRotation[0] = -45.0f;
    }
    if (m_creature->isCage())
        headRotation =
            plasma::Vector<3, float>(0.0f, 0.0f, m_creature->m_entityData.interactionState != 0 ? 120.0f : 0.0f);
    // Buff 14 sinks the creature by a quarter of its height.
    if (m_creature->findBuff(BuffDefenseWeakened) != nullptr)
        creatureOffset[2] -= m_creature->m_entityData.appearance.size[2] * 0.25f;
    // AbilityClimb with CreatureFlagOnGround and without CreatureFlagAtWall: bent forward, the hands low in front.
    if (ability == AbilityClimb && (m_creature->m_entityData.flags & CreatureFlagOnGround) &&
        !(m_creature->m_entityData.flags & CreatureFlagAtWall)) {
        bodyRotation[0] = -30.0f;
        leftHandOffset[1] = 2.0f;
        rightHandOffset[1] = 2.0f;
        bodyOffset[1] += 2.0f;
        bodyOffset[2] -= 3.0f;
        leftHandOffset[0] += 1.0f;
        rightHandOffset[0] -= 1.0f;
    }

    // --- The state's pose follows the target --- poseBlend is the share of the remaining way covered in this frame
    // (blend, Noise.h, from 0 towards 1 at blendRate over frameTime); angles of the whole creature, the head and the
    // tail take the short way.
    float poseBlend = 0.0f;
    blend(poseBlend, 1.0f, frameTime, blendRate);
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
    m_tailRotation[0] = interpolateAngle(m_tailRotation[0], tailRotationX, poseBlend);
    m_tailRotation[1] = interpolateAngle(m_tailRotation[1], 0.0f, poseBlend);
    m_tailRotation[2] = interpolateAngle(m_tailRotation[2], tailRotationZ, poseBlend);
    m_bodyTurn = (bodyTurn - m_bodyTurn) * poseBlend + m_bodyTurn;
    m_handsTurn += poseBlend * (handsTurn - m_handsTurn);
    m_feetTurn = (feetTurn - m_feetTurn) * poseBlend + m_feetTurn;
    m_bodyOffset += poseBlend * (bodyOffset - m_bodyOffset);
    m_headOffset += poseBlend * (headOffset - m_headOffset);
    // m_blendedFacing follows the unit facing direction during ability 29 and is cleared otherwise.
    if (ability == AbilitySupportRay) {
        plasma::Vector<3, float> facing = m_creature->m_entityData.attackRotation;
        if (facing.squaredLength() > 0.0f)
            facing.normalize();
        float facingBlend = 0.0f;
        blend(facingBlend, 1.0f, frameTime, 0.01f);
        m_blendedFacing += facingBlend * (facing - m_blendedFacing);
    } else {
        m_blendedFacing = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    // From here on the hands' positions and angles include the animated pose.
    rightHandAngles += m_rightHandRotation;
    leftHandAngles += m_leftHandRotation;
    rightHandPosition += m_rightHandOffset;
    leftHandPosition += m_leftHandOffset;
    rightShoulderPosition += 0.4f * m_rightHandOffset;
    leftShoulderPosition += 0.4f * m_leftHandOffset;

    // --- Parts the caller hides ---
    if ((hiddenParts & HiddenPartHead) != 0)
        m_headSprite = nullptr;
    if ((hiddenParts & HiddenPartHair) != 0)
        m_hairSprite = nullptr;
    if (m_creature->m_entityData.race == RaceRunePortal && m_creature->m_entityData.interactionState == 0)
        m_hairSprite = nullptr;
    if ((hiddenParts & HiddenPartChest) != 0)
        m_chestSprite = nullptr;
    if ((hiddenParts & HiddenPartShoulderArmor) != 0)
        m_shoulderArmorSprite = nullptr;
    if ((hiddenParts & HiddenPartWeapons) != 0) {
        m_rightWeaponSprite = nullptr;
        m_leftWeaponSprite = nullptr;
    }

    // --- The points around the creature (m_points) --- Animation 97 starts six points ten blocks around and twenty
    // above the creature and draws them in to spots within its box; in any other animation the points left over drift
    // outwards and are all dropped once one is more than twenty blocks away. Each is a small sprite that alternates
    // between 3631 and 3632 every 200 ms.
    if (m_animation == AnimationFlightToPoint || m_points.size() != 0) {
        if (m_points.size() == 0) {
            for (int i = 0; i < 6; i++) {
                int y = rand();
                int x = rand();
                m_points.push_back(plasma::Vector<3, float>((float)x * 20.0f / (float)RAND_MAX - 10.0f,
                                                            (float)y * 20.0f / (float)RAND_MAX - 10.0f, 20.0f));
            }
        }
        int index = 1;
        for (std::list<plasma::Vector<3, float>>::iterator point = m_points.begin(); point != m_points.end();
             ++point, index++) {
            if (m_animation == AnimationFlightToPoint) {
                unsigned int depth = hash(index + 982) % 100;
                unsigned int across = hash(index) % 100;
                blend(*point,
                      plasma::Vector<3, float>(
                          ((float)across / 100.0f - 0.5f) * m_creature->m_entityData.appearance.size[0],
                          ((float)depth / 100.0f - 0.5f) * m_creature->m_entityData.appearance.size[1],
                          ((float)depth * 0.25f / 100.0f + 0.75f) * m_creature->m_entityData.appearance.size[1]),
                      frameTime, 0.0025f);
            } else {
                *point += point->normalized() * 0.05f * frameTime;
                if (point->squaredLength() > 400.0f) {
                    m_points.clear();
                    break;
                }
            }
            Sprite *sprite = spriteManager->sprite(
                (((unsigned)((int)m_animationTime + hash(index)) / 200) & 1) != 0 ? SpriteTinyBird1 : SpriteTinyBird2);
            plasma::Matrix<float> matrix;
            matrix.identity();
            Fixed z = m_creature->m_drawnPosition[2] + (*point)[2] - m_creature->m_stepOffset + m_creatureOffset[2];
            Fixed y = m_creature->m_drawnPosition[1] + (*point)[1] + originY;
            Fixed x = m_creature->m_drawnPosition[0] + (*point)[0] + originX;
            matrix.translate(x.toFloat(), y.toFloat(), z.toFloat());
            matrix.rotateZ((float)(hash(index + 9831) % 1000) / 1000.0f * 360.0f);
            matrix.scale(0.05f, 0.05f, 0.05f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // --- The creature's matrix --- The drawn position (m_drawnPosition relative to the origin, lowered by
    // m_stepOffset and raised by the pose's z offset), a roll about the direction of travel, the drawn rotation
    // (m_drawnRotation), the pose's offset and rotation about the point m_glideLift above the origin, then the scale.
    plasma::Matrix<float> creatureMatrix;
    creatureMatrix.identity();
    creatureMatrix.translate(
        (m_creature->m_drawnPosition[0] + originX).toFloat(), (m_creature->m_drawnPosition[1] + originY).toFloat(),
        (m_creature->m_drawnPosition[2] - m_creature->m_stepOffset + m_creatureOffset[2]).toFloat());
    // Rolling: one turn over the roll's 600 ms (abilities 108 and 160: over the ability's duration) about the
    // horizontal axis across the velocity. Not for class 4, specialization 1 during abilities 6, 18, 21.
    if (m_creature->m_entityData.rollTime != 0.0f &&
        !(m_creature->m_entityData.classType == ClassRogue &&
          m_creature->m_entityData.specialization == SpecializationNinja &&
          (float)m_creature->attackDuration() > m_creature->m_entityData.timeSinceAbility &&
          (ability == AbilityPerforate || ability == AbilityAmbush || ability == AbilityKick))) {
        float angle;
        if (ability == AbilityShuriken || ability == AbilityThrowPoisonVial)
            angle = m_creature->m_entityData.timeSinceAbility / (float)m_creature->attackDuration() * -360.0f;
        else
            angle = (1.0f - m_creature->m_entityData.rollTime / 600.0f) * -360.0f;
        plasma::Vector<3, float> axis =
            m_creature->m_entityData.velocity.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        if (0.0001f > axis.squaredLength())
            axis = m_creature->m_entityData.attackRotation.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        if (m_creature->m_entityData.race == RaceDragon)
            axis = m_creature->m_entityData.velocity;
        if (axis.squaredLength() > 0.01f) {
            axis.normalize();
            creatureMatrix.rotate(angle, axis);
        }
        if (ability != AbilityShuriken) {
            leftHandPosition[2] +=
                m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
            rightHandPosition[2] +=
                m_creature->m_entityData.appearance.size[2] * 5.0f / m_creature->m_entityData.appearance.size[0];
        }
    }
    if (ability == AbilityHangGlide) {
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
    // The model is 11.2 units wide per block of the creature's larger horizontal extent; race 285 also grows with its
    // mp.
    float modelScale =
        (std::max)(m_creature->m_entityData.appearance.size[0], m_creature->m_entityData.appearance.size[1]) / 11.2f;
    if (m_creature->m_entityData.race == RaceWaterBubble)
        modelScale = modelScale * m_creature->m_entityData.mp;
    creatureMatrix.scale(modelScale * m_scale, modelScale * m_scale, modelScale * m_scale);
    // Some races pulse once a second by the system clock: flatter and wider, then taller and thinner.
    if (m_creature->m_entityData.race == RaceWaterBubble ||
        (m_creature->m_entityData.race >= RaceGreenSlime && m_creature->m_entityData.race <= RaceBlueSlime)) {
        float phase = (float)(timeGetTime() % 1000) / 1000.0f;
        float pulse = cosf((float)((double)(phase + phase) * Pi));
        creatureMatrix.scale(1.0f - pulse * 0.1f, 1.0f - pulse * 0.1f, pulse * 0.1f + 1.0f);
    }
    if (m_creature->m_entityData.race == RaceCaterpillar || m_creature->m_entityData.race == RaceEarthCaterpillar) {
        float phase = (float)(timeGetTime() % 1000) / 1000.0f;
        float pulse = cosf((float)((double)(phase + phase) * Pi));
        creatureMatrix.scale(1.0f - pulse * 0.05f, 1.0f - pulse * 0.05f, pulse * 0.05f + 1.0f);
    }
    if (m_creature->m_entityData.race >= RaceManaPump && m_creature->m_entityData.race <= RaceManaGenerator) {
        float phase = (float)(timeGetTime() % 1000) / 1000.0f;
        float pulse = cosf((float)((double)(phase + phase) * Pi));
        creatureMatrix.scale(1.0f - pulse * 0.03f, 1.0f - pulse * 0.03f, pulse * 0.03f + 1.0f);
    }

    // --- The body's matrix --- The body bobs up with the size of the stride (four times as much during ability 80),
    // is moved and turned by the pose and scaled by the caller's factor. The chest, the neck, the shoulder armour, the
    // tail, the head and the hair hang from it.
    float strideSize = abs(swing);
    float bob = strideSize * 4.0f;
    if (ability == AbilitySneak)
        bob = bob * 4.0f;
    m_bodyMatrix = creatureMatrix;
    m_bodyMatrix.translate(0.0f, 0.0f, bob);
    m_bodyMatrix.translate(m_bodyOffset[0], m_bodyOffset[1], m_bodyOffset[2]);
    m_bodyMatrix.scale(scale, scale, scale);
    m_bodyMatrix.rotateZ(m_bodyRotation[2]);
    m_bodyMatrix.rotateY(m_bodyRotation[1]);
    m_bodyMatrix.rotateX(m_creature->m_entityData.appearance.chestRotation + m_bodyRotation[0]);
    // The batch's glow: -1 (drawn in grey, its luminance) for a creature with AppearanceFlagPetrified, 0 otherwise.
    float glow = (m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) != 0 ? -1.0f : 0.0f;
    // With AppearanceFlagFullShininess every equipment sprite is drawn with full shininess instead of the item's own
    // (Item::shininess).
    bool fullShine = (m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFullShininess) != 0;

    // Game::useItem leaves the current action's item in EntityData::actionItem. It is drawn above the creature while
    // gliding (120), below it during ability 121, or around the hands during consumable and pet-feeding animations.
    // This uses the hands' matrix from the previous call; the current matrix is built later.
    const Item &actionItem = m_creature->m_entityData.actionItem;
    if (ability == AbilityHangGlide) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_bodyMatrix;
            matrix.translate(chestPosition +
                             plasma::Vector<3, float>(0.0f, 0.0f,
                                                      m_creature->m_entityData.appearance.size[2] * 8.0f /
                                                          m_creature->m_entityData.appearance.size[0]));
            matrix.rotateX(-m_creatureRotation[0]);
            matrix.scale(chestScale + chestScale, chestScale + chestScale, chestScale + chestScale);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, 0.0f, depthOffset);
        }
    }
    if (ability == AbilitySailBoat) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_bodyMatrix;
            matrix.translate(chestPosition +
                             plasma::Vector<3, float>(0.0f, 0.0f,
                                                      m_creature->m_entityData.appearance.size[2] * -9.0f /
                                                          m_creature->m_entityData.appearance.size[0]));
            matrix.rotateX(-m_bodyRotation[0]);
            matrix.rotateZ(-m_creatureRotation[2]);
            matrix.scale(chestScale + chestScale, chestScale + chestScale, chestScale + chestScale);
            matrix.translate(1.0f, 2.0f, 0.0f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, 0.0f, depthOffset);
        }
    }
    // Abilities 156 (the flight to a flight point, Game.cpp and MapOverlayWidget.cpp) and 157: sprite 3637 under the
    // creature.
    if (ability == AbilityEagleFlight || ability == AbilityMenuView) {
        Sprite *sprite = spriteManager->sprite(SpriteEagle);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_bodyMatrix;
            matrix.translate(chestPosition +
                             plasma::Vector<3, float>(0.0f, 0.0f,
                                                      m_creature->m_entityData.appearance.size[2] * -10.5f /
                                                          m_creature->m_entityData.appearance.size[0]));
            matrix.rotateX(-m_bodyRotation[0]);
            matrix.rotateZ(-m_creatureRotation[2]);
            matrix.scale(chestScale + chestScale, chestScale + chestScale, chestScale + chestScale);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, 0.0f, depthOffset);
        }
    }
    // Animations 78, 79 (their first five seconds) and 99 (its first 300 ms): the item turns above the hands.
    if ((m_animation == AnimationPresentItem && 5000.0f > m_animationTime) ||
        (m_animation == Animation_79 && 5000.0f > m_animationTime) ||
        (m_animation == AnimationFeedClimbingPlant && 300.0f > m_animationTime)) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            float itemScale = actionItem.scale();
            matrix.translate(0.5f * (leftHandPosition + rightHandPosition) +
                             plasma::Vector<3, float>(0.0f, 0.0f, 8.0f));
            matrix.rotateZ(m_animationTime * 0.02f);
            matrix.rotateY(45.0f);
            matrix.rotateX(45.0f);
            matrix.scale(itemScale * 12.0f, itemScale * 12.0f, itemScale * 12.0f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    // Animations 90..96 (their first five seconds) and 98 (its first second): the item in or between the hands.
    if (m_animation == AnimationPlayFlute && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(0.5f * (leftHandPosition + rightHandPosition) +
                             plasma::Vector<3, float>(0.0f, 0.0f, -1.0f));
            matrix.scale(0.8f, 0.8f, 0.8f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayHarp && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(leftHandPosition + plasma::Vector<3, float>(7.5f, 0.0f, 0.0f));
            matrix.scale(0.75f, 0.75f, 0.75f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayBell && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
                matrix.rotate(leftFootSwing, 1.0f, 0.0f, 0.0f);
            matrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            matrix.rotateZ(rightHandAngles[2]);
            matrix.rotateY(rightHandAngles[1]);
            matrix.rotateX(rightHandAngles[0]);
            matrix.scale(0.75f, 0.75f, 0.75f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)(-sprite->m_size[2]));
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayLute && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(leftHandPosition + plasma::Vector<3, float>(7.5f, 0.0f, 0.0f));
            matrix.scale(0.85f, 0.85f, 0.85f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayWhistle && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(0.5f * (leftHandPosition + rightHandPosition));
            matrix.scale(0.75f, 0.75f, 0.75f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationCallPet && 1000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(rightHandPosition);
            matrix.translate(-2.3f, 0.3f, 0.13f);
            matrix.rotateZ(rightHandRotation[2]);
            matrix.scale(0.75f, 0.75f, 0.75f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayFiddle && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(leftHandPosition + plasma::Vector<3, float>(0.0f, -7.0f, 0.0f));
            matrix.scale(0.85f, 0.85f, 0.85f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }
    if (m_animation == AnimationPlayNightingale && 5000.0f > m_animationTime) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            matrix.rotateZ(rightHandAngles[2]);
            matrix.rotateY(rightHandAngles[1]);
            matrix.rotateX(rightHandAngles[0]);
            matrix.scale(0.5f, 0.5f, 0.5f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)(-sprite->m_size[2]));
            batch.add(sprite, matrix, color, alpha, glow, actionItem.shininess(), 0.0f, depthOffset);
        }
    }

    // --- The chest --- At the chest's position, turned half as far as the arms swing and half of the body's turn.
    if (m_chestSprite != nullptr) {
        m_chestMatrix = m_bodyMatrix;
        m_chestMatrix.translate(chestPosition);
        m_chestMatrix.rotateZ((float)((double)(armSwing * 0.5f * 180.0f) / Pi + (double)(m_bodyTurn * 0.5f)));
        m_chestMatrix.scale(chestScale, chestScale, chestScale);
        if (m_creature->m_entityData.race == RaceSnail) {
            float pulse = cosf(m_creature->m_entityData.timeSinceAbility * 0.01f);
            m_chestMatrix.scale(1.0f - pulse * 0.05f, 1.0f - pulse * 0.05f, pulse * 0.05f + 1.0f);
        }
        m_chestMatrix.translate((float)m_chestSprite->m_size[0] * -0.5f, (float)m_chestSprite->m_size[1] * -0.5f,
                                (float)m_chestSprite->m_size[2] * -0.5f);
        float shine = fullShine ? 1.0f : m_creature->m_entityData.equipment.chest.shininess();
        batch.add(m_chestSprite, m_chestMatrix, color, alpha, glow, shine, m_creature->m_hitFlash, depthOffset);
    }

    // --- The neck item --- In front of and above the chest; it turns with the chest and tips forward as the arms
    // swing.
    if (m_neckSprite != nullptr) {
        m_neckMatrix = m_bodyMatrix;
        m_neckMatrix.translate(chestPosition + plasma::Vector<3, float>(0.0f, 3.0f, 6.05f));
        m_neckMatrix.rotateZ((float)((double)(armSwing * 0.5f * 180.0f) / Pi + (double)(m_bodyTurn * 0.5f)));
        m_neckMatrix.rotateX((float)((double)armSwing * 0.7 * (double)armSwing * 180.0 / Pi));
        m_neckMatrix.scale(chestScale, chestScale, chestScale);
        m_neckMatrix.translate((float)m_neckSprite->m_size[0] * -0.5f, -1.5f, (float)(-m_neckSprite->m_size[2]));
        float shine = fullShine ? 1.0f : m_creature->m_entityData.equipment.neck.shininess();
        batch.add(m_neckSprite, m_neckMatrix, color, alpha, glow, shine, m_creature->m_hitFlash, depthOffset);
    }

    // --- The shoulder armour --- One piece over the chest, raised with the walk cycle's amplitude, a little larger
    // than the chest.
    if (m_shoulderArmorSprite != nullptr) {
        m_shoulderArmorMatrix = m_bodyMatrix;
        m_shoulderArmorMatrix.translate(0.0f, 0.0f, amplitude + amplitude + 4.0f);
        m_shoulderArmorMatrix.translate(chestPosition);
        m_shoulderArmorMatrix.rotateZ((float)((double)(armSwing * 0.5f * 180.0f) / Pi));
        m_shoulderArmorMatrix.scale(shoulderArmorScale, shoulderArmorScale, shoulderArmorScale);
        m_shoulderArmorMatrix.scale(chestScale + 0.1f, chestScale + 0.1f, chestScale + 0.1f);
        m_shoulderArmorMatrix.translate((float)m_shoulderArmorSprite->m_size[0] * -0.5f,
                                        (float)m_shoulderArmorSprite->m_size[1] * -0.5f,
                                        (float)m_shoulderArmorSprite->m_size[2] * -0.5f);
        float shine = fullShine ? 1.0f : m_creature->m_entityData.equipment.shoulder.shininess();
        batch.add(m_shoulderArmorSprite, m_shoulderArmorMatrix, color, alpha, glow, shine, m_creature->m_hitFlash,
                  depthOffset);
    }

    // --- The tail --- At Appearance::tailPosition from the chest, tilted by Appearance::tailRotation, swinging
    // against the arms and turned by the pose; the sprite hangs from its front edge.
    if (m_tailSprite != nullptr) {
        m_tailMatrix = m_bodyMatrix;
        m_tailMatrix.translate(chestPosition);
        m_tailMatrix.translate(tailPosition);
        m_tailMatrix.rotateX(m_creature->m_entityData.appearance.tailRotation);
        m_tailMatrix.rotateZ((float)((double)(armSwing * -180.0f) / Pi));
        m_tailMatrix.rotateZ(m_tailRotation[2]);
        m_tailMatrix.rotateY(m_tailRotation[1]);
        m_tailMatrix.rotateX(m_tailRotation[0]);
        m_tailMatrix.scale(tailScale, tailScale, tailScale);
        m_tailMatrix.scale(chestScale, chestScale, chestScale);
        m_tailMatrix.translate((float)m_tailSprite->m_size[0] * -0.5f, (float)(-m_tailSprite->m_size[1]),
                               (float)m_tailSprite->m_size[2] * -0.5f);
        batch.add(m_tailSprite, m_tailMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
    }

    // --- The head --- At the head's position plus the pose's offset, lowered by half the bob. The body's rotation is
    // undone and the head's own applied about a point three units below the head's centre; the entity's head rotation
    // (looking up and down) is relative to the drawn pitch. The lid of a chest and the door of a cage are head sprites
    // that turn about an edge instead of their centre.
    plasma::Vector<3, float> headCentre(headPosition[0] + m_headOffset[0], headPosition[1] + m_headOffset[1],
                                        headPosition[2] + m_headOffset[2] - bob * 0.5f);
    if (m_headSprite != nullptr) {
        m_headMatrix = m_bodyMatrix;
        m_headMatrix.translate(headCentre[0], headCentre[1], headCentre[2]);
        m_headMatrix.translate(0.0f, 0.0f, -3.0f);
        if (m_creature->isChest())
            m_headMatrix.translate(0.0f, (float)m_headSprite->m_size[1] * -0.25f, 0.0f);
        else if (m_creature->isCage())
            m_headMatrix.translate(
                (float)m_headSprite->m_size[0] * -0.5f * m_creature->m_entityData.appearance.headScale,
                (float)m_headSprite->m_size[1] * 0.5f * m_creature->m_entityData.appearance.headScale, 0.0f);
        m_headMatrix.rotateX(-m_bodyRotation[0] - m_creature->m_entityData.appearance.chestRotation);
        m_headMatrix.rotateY(-m_bodyRotation[1]);
        m_headMatrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        m_headMatrix.rotateY(m_headRotation[1]);
        m_headMatrix.rotateX(m_creature->m_entityData.headRotation - m_creature->m_drawnRotation[0] +
                             m_headRotation[0]);
        m_headMatrix.translate(0.0f, 0.0f, 3.0f);
        m_headMatrix.scale(headScale, headScale, headScale);
        float backX;
        float backY;
        if (m_creature->isChest()) {
            backX = (float)m_headSprite->m_size[0] * -0.5f;
            backY = 0.0f;
        } else if (m_creature->m_entityData.race == RaceDragon) {
            backX = (float)m_headSprite->m_size[0] * -0.5f;
            backY = (float)m_headSprite->m_size[1] * -0.2f;
        } else if (m_creature->isCage()) {
            backX = 0.0f;
            backY = (float)(-m_headSprite->m_size[1]);
        } else {
            backX = (float)m_headSprite->m_size[0] * -0.5f;
            backY = (float)m_headSprite->m_size[1] * -0.5f;
        }
        m_headMatrix.translate(backX, backY, (float)m_headSprite->m_size[2] * -0.5f);
        batch.add(m_headSprite, m_headMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
    }
    // Ability 138: sprite 3563, four times its size, where the head is; it turns with the head but does not nod.
    if (ability == AbilitySlowFall) {
        Sprite *sprite = spriteManager->sprite(SpriteParachute);
        plasma::Matrix<float> matrix = m_bodyMatrix;
        matrix.translate(headCentre[0], headCentre[1], headCentre[2]);
        matrix.translate(0.0f, 0.0f, -3.0f);
        matrix.rotateY(-m_bodyRotation[1]);
        matrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        matrix.rotateY(m_headRotation[1]);
        matrix.translate(0.0f, 0.0f, 3.0f);
        matrix.scale(4.0f, 4.0f, 4.0f);
        matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
        batch.add(sprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
    }

    // --- The hair --- The head's transform, a hair's breadth larger, tinted with the appearance's hair colour. With
    // appearanceFlags bit 22 only the green component scales the colour; bit 10 sets the glow to 1.
    if (m_hairSprite != nullptr) {
        m_hairMatrix = m_bodyMatrix;
        m_hairMatrix.translate(headCentre[0], headCentre[1], headCentre[2]);
        m_hairMatrix.translate(0.0f, 0.0f, -3.0f);
        m_hairMatrix.rotateX(-m_bodyRotation[0] - m_creature->m_entityData.appearance.chestRotation);
        m_hairMatrix.rotateY(-m_bodyRotation[1]);
        m_hairMatrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        m_hairMatrix.rotateY(m_headRotation[1]);
        m_hairMatrix.rotateX(m_creature->m_entityData.headRotation - m_creature->m_drawnRotation[0] +
                             m_headRotation[0]);
        m_hairMatrix.translate(0.0f, 0.0f, 3.0f);
        m_hairMatrix.scale(headScale + 0.001f, headScale + 0.001f, headScale + 0.001f);
        m_hairMatrix.translate((float)m_hairSprite->m_size[0] * -0.5f, (float)m_hairSprite->m_size[1] * -0.5f,
                               (float)m_hairSprite->m_size[2] * -0.5f);
        plasma::Vector<4, float> tint =
            color * plasma::Vector<4, float>((float)m_creature->m_entityData.appearance.hairColor[0] / 255.0f,
                                             (float)m_creature->m_entityData.appearance.hairColor[1] / 255.0f,
                                             (float)m_creature->m_entityData.appearance.hairColor[2] / 255.0f, 1.0f);
        float hairGlow =
            (m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSuppressHairDebris) != 0 ? 1.0f : 0.0f;
        if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) != 0) {
            tint = color * (float)m_creature->m_entityData.appearance.hairColor[1] / 255.0f;
            hairGlow = -1.0f;
        }
        batch.add(m_hairSprite, m_hairMatrix, tint, alpha, hairGlow, 0.0f, m_creature->m_hitFlash,
                  depthOffset + 0.0001f);
    }

    // Draws the first equipment-slot item on the head, using the head transform and hair height. Does not check that
    // the hair sprite exists before reading its size.
    if (slot0Sprite != nullptr) {
        plasma::Matrix<float> matrix = m_bodyMatrix;
        matrix.translate(headCentre[0], headCentre[1], headCentre[2]);
        matrix.translate(0.0f, 0.0f, -3.0f);
        matrix.rotateX(-m_bodyRotation[0] - m_creature->m_entityData.appearance.chestRotation);
        matrix.rotateY(-m_bodyRotation[1]);
        matrix.rotateZ(m_headRotation[2] - m_bodyRotation[2]);
        matrix.rotateY(m_headRotation[1]);
        matrix.rotateX(m_creature->m_entityData.headRotation - m_creature->m_drawnRotation[0] + m_headRotation[0]);
        matrix.translate(0.0f, 0.0f, 3.0f);
        matrix.scale(headScale, headScale, headScale);
        matrix.translate((float)slot0Sprite->m_size[0] * -0.5f, (float)slot0Sprite->m_size[1] * -0.5f,
                         (float)m_hairSprite->m_size[2] * -0.5f);
        batch.add(slot0Sprite, matrix, color, alpha, glow, m_creature->m_entityData.equipment.head.shininess(),
                  m_creature->m_hitFlash, depthOffset);
    }

    // --- The hands' matrix --- The frame of the hands, the shoulders and the weapons: the creature's matrix raised by
    // the bob and moved by the pose, tipped by twice the entity's head rotation (not during AbilityClimb with
    // CreatureFlagAtWall nor in animations 2, 3 and 38) and by 60 degrees with toggle 10, turned about y four units
    // below by the pose's hands turn, then the body's rotation.
    m_handsMatrix = creatureMatrix;
    m_handsMatrix.translate(0.0f, 0.0f, bob);
    m_handsMatrix.translate(m_bodyOffset[0], m_bodyOffset[1], m_bodyOffset[2]);
    if ((ability != AbilityClimb || (m_creature->m_entityData.flags & CreatureFlagAtWall) == 0) &&
        m_animation != AnimationTwoHandedSweep && m_animation != AnimationTwoHandedThrust &&
        m_animation != AnimationTwoHandedSwing)
        m_handsMatrix.rotateX(m_creature->m_entityData.headRotation + m_creature->m_entityData.headRotation);
    if (m_creature->m_entityData.binaryToggles.leanForwardPose)
        m_handsMatrix.rotateX(60.0f);
    m_handsMatrix.translate(0.0f, 0.0f, -4.0f);
    m_handsMatrix.rotateY(m_handsTurn[1]);
    m_handsMatrix.translate(0.0f, 0.0f, 4.0f);
    m_handsMatrix.rotateZ(m_bodyRotation[2]);
    m_handsMatrix.rotateY(m_bodyRotation[1]);
    m_handsMatrix.rotateX(m_bodyRotation[0]);

    // --- The shoulders --- The appearance's shoulder model twice, between each hand and the neck.
    if (m_shouldersSprite != nullptr) {
        m_leftShoulderMatrix = m_handsMatrix;
        m_leftShoulderMatrix.translate(leftShoulderPosition[0], leftShoulderPosition[1], leftShoulderPosition[2]);
        m_leftShoulderMatrix.scale(shouldersScale, shouldersScale, shouldersScale);
        m_leftShoulderMatrix.translate((float)m_shouldersSprite->m_size[0] * -0.5f,
                                       (float)m_shouldersSprite->m_size[1] * -0.5f,
                                       (float)m_shouldersSprite->m_size[2] * -0.5f);
        batch.add(m_shouldersSprite, m_leftShoulderMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash,
                  depthOffset);
        m_rightShoulderMatrix = m_handsMatrix;
        m_rightShoulderMatrix.translate(rightShoulderPosition[0], rightShoulderPosition[1], rightShoulderPosition[2]);
        m_rightShoulderMatrix.scale(shouldersScale, shouldersScale, shouldersScale);
        m_rightShoulderMatrix.translate((float)m_shouldersSprite->m_size[0] * -0.5f,
                                        (float)m_shouldersSprite->m_size[1] * -0.5f,
                                        (float)m_shouldersSprite->m_size[2] * -0.5f);
        batch.add(m_shouldersSprite, m_rightShoulderMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash,
                  depthOffset);
    }

    // --- The action's item between the hands (abilities 84 and 85 while the item's cooldown lasts, and 86) ---
    if ((ability == AbilityDrink || ability == AbilityEat) &&
        (float)actionItem.cooldown() > m_creature->m_entityData.timeSinceAbility) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(0.5f * (leftHandPosition + rightHandPosition));
            matrix.rotateX(leftHandAngles[0]);
            float itemScale = actionItem.scale();
            matrix.scale(itemScale * (handsScale * 12.0f), itemScale * (handsScale * 12.0f),
                         itemScale * (handsScale * 12.0f));
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        }
    }
    if (ability == AbilityOfferPetFood) {
        Sprite *sprite = spriteManager->itemSprite(&actionItem);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix.translate(0.5f * (leftHandPosition + rightHandPosition));
            matrix.rotateX(leftHandAngles[0]);
            matrix.scale(handsScale, handsScale, handsScale);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        }
    }

    // Copies the hands' matrix and then replaces it by the body's.
    if (m_creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        (m_creature->m_entityData.equipment.weaponLeft.id == WeaponBow ||
         m_creature->m_entityData.equipment.weaponLeft.id == WeaponCrossbow)) {
        Sprite *sprite = spriteManager->sprite(SpriteQuiver);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix = m_bodyMatrix;
            matrix.rotateZ((float)((double)(armSwing * 0.25f * 180.0f) / Pi));
            matrix.translate(0.0f, -7.1f, 0.1f);
            matrix.rotate(45.0f, 0.0f, 1.0f, 0.0f);
            matrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            matrix.scale(rightWeaponScale + 0.01f, rightWeaponScale + 0.01f, rightWeaponScale + 0.01f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        }
    }
    if (m_creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
        (m_creature->m_entityData.equipment.weaponRight.id == WeaponBow ||
         m_creature->m_entityData.equipment.weaponRight.id == WeaponCrossbow)) {
        Sprite *sprite = spriteManager->sprite(SpriteQuiver);
        if (sprite != nullptr) {
            plasma::Matrix<float> matrix = m_handsMatrix;
            matrix = m_bodyMatrix;
            matrix.rotateZ((float)((double)(armSwing * 0.25f * 180.0f) / Pi));
            matrix.translate(0.0f, -7.1f, 0.1f);
            matrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
            matrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            matrix.scale(rightWeaponScale + 0.01f, rightWeaponScale + 0.01f, rightWeaponScale + 0.01f);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            batch.add(sprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        }
    }
    // --- The weapons --- During these abilities a creature does not hold its weapons: they are drawn on its back
    // (weapons of ids 4, 12 and 21, and 20 in the left hand, stay in the hand).
    const bool weaponsStowed =
        ability <= AbilityWatchPlayer || ability == AbilityRangerKick || ability == AbilityRetreat ||
        (ability >= AbilitySneak && ability <= AbilityTortureRackPose) || ability == AbilityCeilingChainPose ||
        ability == Ability_94 || ability == AbilityQuake || ability == AbilityShuriken ||
        (ability >= AbilityPortCity && ability <= AbilityClimb) || ability == AbilityPresentItem ||
        ability == Ability_125 || ability == AbilityCarryCreature || ability == AbilityThrowCreature ||
        ability == AbilityUseSpecialItem || ability == AbilitySlowFall ||
        (ability >= AbilityFlameRush && ability <= AbilityRockFist) || ability == AbilityFlightToPoint ||
        (ability >= AbilityFeedClimbingPlant && ability <= AbilityMenuView) ||
        (ability >= AbilityThrowPoisonVial && ability <= AbilityDance);

    // --- The right weapon --- In the right hand, or on the back while it is stowed. The item's spirits are added at
    // their places on the sprite (sprite 2630), and the ends of the blade start the first pair of trails (below).
    if (m_rightWeaponSprite != nullptr) {
        const unsigned int id = m_creature->m_entityData.equipment.weaponRight.id;
        float backX = (float)m_rightWeaponSprite->m_size[0] * -0.5f;
        float backY;
        float backZ;
        m_rightWeaponMatrix = m_handsMatrix;
        if (!weaponsStowed || id == WeaponFist || id == WeaponBracelet || id == WeaponMagnifier) {
            m_rightWeaponMatrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            m_rightWeaponMatrix.rotateZ(rightHandAngles[2]);
            m_rightWeaponMatrix.rotateY(rightHandAngles[1]);
            m_rightWeaponMatrix.rotateX(rightHandAngles[0]);
            m_rightWeaponMatrix.rotate(0.0f, 0.0f, 0.0f, 1.0f);
            m_rightWeaponMatrix.rotate(0.0f, 0.0f, 1.0f, 0.0f);
            m_rightWeaponMatrix.rotate(m_creature->m_entityData.headRotation * 0.5f + 0.0f, 1.0f, 0.0f, 0.0f);
            m_rightWeaponMatrix.scale(rightWeaponScale, rightWeaponScale, rightWeaponScale);
            if (id == WeaponMagnifier) {
                m_rightWeaponMatrix.rotateX(-45.0f);
                m_rightWeaponMatrix.translate(0.0f, 0.0f, 5.0f);
            }
            // Where the hand grips the sprite, by the kind of weapon.
            if (m_rightWeaponSprite == spriteManager->sprite(SpriteArrow)) {
                backX = (float)m_rightWeaponSprite->m_size[0] * -0.5f - 1.0f;
                backY = 0.5f;
                backZ = 1.0f;
            } else if (id == WeaponStaff) {
                backY = (float)m_rightWeaponSprite->m_size[1] * -0.5f;
                backZ = -10.0f;
            } else if (id == WeaponBracelet) {
                m_rightWeaponMatrix.scale(0.6f, 0.6f, 0.6f);
                backY = (float)(1 - m_rightWeaponSprite->m_size[1]);
                backZ = (float)m_rightWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponBow) {
                backY = 1.5f - (float)m_rightWeaponSprite->m_size[1];
                backZ = (float)m_rightWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponCrossbow) {
                backY = 0.0f;
                backZ = (float)m_rightWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponBoomerang) {
                backY = -2.0f;
                backZ = -4.0f;
            } else {
                backY = (float)m_rightWeaponSprite->m_size[1] * -0.5f;
                backZ = id != 4 ? -4.0f : (float)m_rightWeaponSprite->m_size[2] * -0.5f;
            }
        } else {
            m_rightWeaponMatrix = m_bodyMatrix;
            m_rightWeaponMatrix.rotateZ((float)((double)(armSwing * 0.25f * 180.0f) / Pi));
            m_rightWeaponMatrix.translate(0.0f, -7.1f, 0.1f);
            m_rightWeaponMatrix.rotate(id == WeaponStaff ? -45.0f : -135.0f, 0.0f, 1.0f, 0.0f);
            m_rightWeaponMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            m_rightWeaponMatrix.scale(rightWeaponScale, rightWeaponScale, rightWeaponScale);
            backY = (float)m_rightWeaponSprite->m_size[1] * -0.5f;
            backZ = (float)m_rightWeaponSprite->m_size[2] * -0.5f;
        }
        m_rightWeaponMatrix.translate(backX, backY, backZ);
        float shine = fullShine ? 1.0f : m_creature->m_entityData.equipment.weaponRight.shininess();
        batch.add(m_rightWeaponSprite, m_rightWeaponMatrix, color, alpha, glow, shine, m_creature->m_hitFlash,
                  depthOffset);
        batch.addSpirits(&m_creature->m_entityData.equipment.weaponRight, spriteManager->sprite(SpriteCube),
                         m_rightWeaponMatrix, color, 0.0f, alpha, glow, shine, m_creature->m_hitFlash, depthOffset);
        m_trails[1][0] =
            m_rightWeaponMatrix * plasma::Vector<3, float>((float)m_rightWeaponSprite->m_size[0] * 0.5f,
                                                           (float)m_rightWeaponSprite->m_size[1] * 0.5f, 0.0f);
        m_trails[0][0] = m_rightWeaponMatrix * plasma::Vector<3, float>((float)m_rightWeaponSprite->m_size[0] * 0.5f,
                                                                        (float)m_rightWeaponSprite->m_size[1] * 0.5f,
                                                                        (float)m_rightWeaponSprite->m_size[2]);
    }

    // --- The left weapon --- As the right one, in the left hand or on the back (id 13 lies flat there); in animation
    // 77 it floats above the hands. While the entity charges, its white value (the hit flash) pulses.
    if (m_leftWeaponSprite != nullptr) {
        const unsigned int id = m_creature->m_entityData.equipment.weaponLeft.id;
        float flash = m_creature->m_hitFlash;
        if (m_creature->m_entityData.charge > 0.0f)
            flash = m_creature->m_hitFlash + (cosf(m_animationTime * 0.01f) + 1.0f) * 0.25f;
        float backX;
        float backY;
        float backZ;
        m_leftWeaponMatrix = m_handsMatrix;
        if (!weaponsStowed || id == WeaponFist || id == WeaponBracelet || id == WeaponTorch || id == WeaponMagnifier) {
            if (m_animation == AnimationThrowRock)
                m_leftWeaponMatrix.translate(0.5f * (leftHandPosition + rightHandPosition) +
                                             plasma::Vector<3, float>(0.0f, 0.0f, 8.0f));
            else
                m_leftWeaponMatrix.translate(leftHandPosition[0] + 0.01f, leftHandPosition[1] + 0.01f,
                                             leftHandPosition[2] + 0.01f);
            m_leftWeaponMatrix.rotateZ(leftHandAngles[2]);
            m_leftWeaponMatrix.rotateY(leftHandAngles[1]);
            m_leftWeaponMatrix.rotateX(leftHandAngles[0]);
            m_leftWeaponMatrix.rotate(0.0f, 0.0f, 0.0f, 1.0f);
            m_leftWeaponMatrix.rotate(0.0f, 0.0f, 1.0f, 0.0f);
            m_leftWeaponMatrix.rotate(m_creature->m_entityData.headRotation * 0.5f + 0.0f, 1.0f, 0.0f, 0.0f);
            m_leftWeaponMatrix.scale(leftWeaponScale, leftWeaponScale, leftWeaponScale);
            backX = (float)m_leftWeaponSprite->m_size[0] * -0.5f;
            // Where the hand grips the sprite. The id 8 test reads the right weapon's id.
            if (m_leftWeaponSprite == spriteManager->sprite(SpriteArrow)) {
                backX = 1.0f - (float)m_leftWeaponSprite->m_size[0] * 0.5f;
                backY = 0.5f;
                backZ = 1.0f;
            } else if (m_animation == AnimationThrowRock) {
                backY = (float)m_leftWeaponSprite->m_size[1] * -0.5f;
                backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponBow) {
                backY = 1.5f - (float)m_leftWeaponSprite->m_size[1];
                backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponCrossbow) {
                backY = 0.0f;
                backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponBracelet) {
                m_leftWeaponMatrix.scale(0.6f, 0.6f, 0.6f);
                backY = (float)(1 - m_leftWeaponSprite->m_size[1]);
                backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
            } else if (id == WeaponShield) {
                backY = 3.0f;
                backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
            } else if (m_creature->m_entityData.equipment.weaponRight.id == WeaponBoomerang) {
                backY = -2.0f;
                backZ = -4.0f;
            } else {
                if (id == WeaponFist) {
                    m_leftWeaponMatrix.rotateY(180.0f);
                    backZ = (float)m_leftWeaponSprite->m_size[2] * -0.5f;
                } else {
                    backZ = -4.0f;
                }
                backY = (float)m_leftWeaponSprite->m_size[1] * -0.5f;
            }
        } else {
            m_leftWeaponMatrix = m_bodyMatrix;
            m_leftWeaponMatrix.translate(0.5f, -0.65f, 0.5f);
            m_leftWeaponMatrix.rotateZ((float)((double)(armSwing * 0.25f * 180.0f) / Pi));
            if (id == WeaponShield) {
                m_leftWeaponMatrix.translate(0.0f, -8.2f, 0.0f);
                m_leftWeaponMatrix.rotate(180.0f, 0.0f, 0.0f, 1.0f);
            } else {
                if (id != 6 && id != 7) {
                    m_leftWeaponMatrix.translate(0.0f, -7.2f, 0.0f);
                    m_leftWeaponMatrix.rotate(-225.0f, 0.0f, 1.0f, 0.0f);
                } else {
                    m_leftWeaponMatrix.translate(0.0f, -6.2f, 0.0f);
                    m_leftWeaponMatrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
                }
                m_leftWeaponMatrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
            }
            m_leftWeaponMatrix.scale(leftWeaponScale, leftWeaponScale, leftWeaponScale);
            backX = 0.001f - (float)m_leftWeaponSprite->m_size[0] * 0.5f;
            backY = 0.001f - (float)m_leftWeaponSprite->m_size[1] * 0.5f;
            backZ = 0.001f - (float)m_leftWeaponSprite->m_size[2] * 0.5f;
        }
        m_leftWeaponMatrix.translate(backX, backY, backZ);
        float shine = fullShine ? 1.0f : m_creature->m_entityData.equipment.weaponLeft.shininess();
        batch.add(m_leftWeaponSprite, m_leftWeaponMatrix, color, alpha, glow, shine, flash, depthOffset);
        batch.addSpirits(&m_creature->m_entityData.equipment.weaponLeft, spriteManager->sprite(SpriteCube),
                         m_leftWeaponMatrix, color, 0.0f, alpha, glow, shine, flash, depthOffset);
        m_trails[3][0] =
            m_leftWeaponMatrix * plasma::Vector<3, float>((float)m_leftWeaponSprite->m_size[0] * 0.5f,
                                                          (float)m_leftWeaponSprite->m_size[1] * 0.5f, 0.0f);
        m_trails[2][0] = m_leftWeaponMatrix * plasma::Vector<3, float>((float)m_leftWeaponSprite->m_size[0] * 0.5f,
                                                                       (float)m_leftWeaponSprite->m_size[1] * 0.5f,
                                                                       (float)m_leftWeaponSprite->m_size[2]);
    }

    // --- The quiver item (a left-hand weapon of id 14) on the back ---
    if (quiverSprite != nullptr) {
        plasma::Matrix<float> matrix = m_handsMatrix;
        matrix = m_bodyMatrix;
        matrix.translate(0.65f, -0.5f, 0.3f);
        matrix.rotateZ((float)((double)(armSwing * 0.25f * 180.0f) / Pi));
        matrix.translate(0.0f, -6.2f, 0.0f);
        matrix.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
        matrix.rotate(90.0f, 0.0f, 0.0f, 1.0f);
        matrix.scale(m_creature->m_entityData.appearance.weaponScale, m_creature->m_entityData.appearance.weaponScale,
                     m_creature->m_entityData.appearance.weaponScale);
        matrix.translate((float)quiverSprite->m_size[0] * -0.5f, (float)quiverSprite->m_size[1] * -0.5f,
                         (float)quiverSprite->m_size[2] * -0.5f);
        batch.add(quiverSprite, matrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
    }

    // --- The weapon trails --- Four rows of sixteen points, kept relative to the creature's drawn position: the tip
    // and the base of the right weapon and of the left weapon, newest first. Point 0 of each row is where the weapon's
    // end is now (set with the weapons above); while an attack strikes, each older point follows the one before it,
    // which leaves a ribbon behind the blade (D3D11Renderer::drawTrails); otherwise the rows collapse onto point 0.
    plasma::Vector<3, float> *const rightTip = m_trails[0];
    plasma::Vector<3, float> *const rightBase = m_trails[1];
    plasma::Vector<3, float> *const leftTip = m_trails[2];
    plasma::Vector<3, float> *const leftBase = m_trails[3];
    // The sums are raw additions in place here, not calls of Fixed's operator+ as elsewhere in this function.
    plasma::Vector<3, float> toCreature((float)Fixed(-(m_creature->m_drawnPosition[0].m_raw + m_originX.m_raw)),
                                        (float)Fixed(-(m_creature->m_drawnPosition[1].m_raw + m_originY.m_raw)), 0.0f);
    leftBase[0] += toCreature;
    leftTip[0] += toCreature;
    rightBase[0] += toCreature;
    rightTip[0] += toCreature;
    bool ring = false;
    if (m_animation == AnimationBlockStrike && m_animationTime > (float)m_creature->anticipationTime()) {
        int anticipation = m_creature->anticipationTime();
        int strike = m_creature->strikeTime();
        ring = (float)(anticipation + strike) > m_animationTime;
    }
    if (ring) {
        // Animation 75 while it strikes: the right weapon's rows are a half circle in front of the creature that grows
        // to twenty units (the base row at half the radius).
        plasma::Matrix<float> matrix;
        matrix.identity();
        matrix.translate((m_creature->m_drawnPosition[0] + originX).toFloat(),
                         (m_creature->m_drawnPosition[1] + originY).toFloat(),
                         (m_creature->m_drawnPosition[2] - m_creature->m_stepOffset + m_creatureOffset[2]).toFloat());
        matrix.rotateZ(m_creature->m_drawnRotation[2]);
        int anticipation = m_creature->anticipationTime();
        float radius = (m_animationTime - (float)anticipation) * 20.0f /
                       (float)(m_creature->anticipationTime() + m_creature->strikeTime());
        for (int i = 0; i < 16; i++) {
            float cosine = cosf((float)((double)i * Pi * 0.0625 - HalfPi));
            float sine = sinf((float)((double)i * Pi * 0.0625 - HalfPi));
            rightBase[i] = matrix * (radius * 0.5f * plasma::Vector<3, float>(sine, cosine, 0.0f)) + toCreature;
            rightTip[i] = matrix * (radius * plasma::Vector<3, float>(sine, cosine, 0.0f)) + toCreature;
        }
    } else if ((m_animation == AnimationSmash || m_animation == AnimationTwoHandedSweep ||
                m_animation == AnimationTwoHandedThrust || m_animation == AnimationParry ||
                m_animation == AnimationBlockJab || m_animation == Animation_12 || m_animation == Animation_14 ||
                (m_animation >= AnimationSwirlStrike && m_animation <= AnimationOverheadChop) ||
                m_animation == AnimationAmbush || m_animation == AnimationTwoHandedSpin ||
                m_animation == AnimationGuardianCrushStrike || m_animation == AnimationCyclone) &&
               m_animationTime > (float)m_creature->anticipationTime() &&
               (float)m_creature->attackDuration() > m_animationTime) {
        // Attacks with both weapons, after the anticipation: all four rows follow.
        float follow = 0.0f;
        blend(follow, 1.0f, frameTime, 0.05f);
        for (int i = 0; i < 15; i++) {
            rightBase[i + 1] += follow * (rightBase[i] - rightBase[i + 1]);
            rightTip[i + 1] += follow * (rightTip[i] - rightTip[i + 1]);
            leftBase[i + 1] += follow * (leftBase[i] - leftBase[i + 1]);
            leftTip[i + 1] += follow * (leftTip[i] - leftTip[i + 1]);
        }
    } else if ((m_animation == AnimationLeftSlash || m_animation == AnimationRightBackhand ||
                m_animation == AnimationLeftStab) &&
               m_creature->m_entityData.timeSinceAbility > (float)m_creature->anticipationTime()) {
        // Attacks with the left weapon: its rows follow, the right weapon's collapse onto its base. The first two
        // statements have no effect (a copy onto itself and a step of 0).
        float follow = 0.0f;
        blend(follow, 1.0f, frameTime, 0.05f);
        leftBase[0] = leftBase[0];
        leftTip[0] += 0.0f * (leftTip[0] - leftBase[0]);
        for (int i = 0; i < 15; i++) {
            rightBase[i + 1] = rightBase[0];
            rightTip[i + 1] = rightBase[0];
            leftBase[i + 1] += follow * (leftBase[i] - leftBase[i + 1]);
            leftTip[i + 1] += follow * (leftTip[i] - leftTip[i + 1]);
        }
    } else {
        // During right-weapon attacks and the first half of recovery, its trail rows follow the blade; the left rows
        // collapse onto their base. The animation 51 test cannot succeed in this branch.
        bool rightTrail = false;
        if ((m_animation == AnimationRightSlash || m_animation == AnimationLeftBackhand ||
             m_animation == AnimationRightStab || m_animation == AnimationRockFist) &&
            m_creature->m_entityData.timeSinceAbility > (float)m_creature->anticipationTime()) {
            int hold = m_creature->holdTime();
            int anticipation = m_creature->anticipationTime();
            int strike = m_creature->strikeTime();
            rightTrail = (float)(strike + (hold / 2 + anticipation)) > m_creature->m_entityData.timeSinceAbility;
        }
        if (rightTrail) {
            float follow = 0.0f;
            blend(follow, 1.0f, frameTime, 0.075f);
            if (m_animation == AnimationKick) {
                rightTip[0] = creatureMatrix * plasma::Vector<3, float>(20.0f, 0.0f, -12.0f);
                rightBase[0] = creatureMatrix * plasma::Vector<3, float>(8.0f, 0.0f, -12.0f);
            } else {
                rightBase[0] = 0.5f * rightBase[0] + 0.5f * rightTip[0];
                rightTip[0] += 0.0f * (rightTip[0] - rightBase[0]);
            }
            for (int i = 0; i < 15; i++) {
                rightBase[i + 1] += follow * (rightBase[i] - rightBase[i + 1]);
                rightTip[i + 1] += follow * (rightTip[i] - rightTip[i + 1]);
                leftBase[i + 1] = leftBase[0];
                leftTip[i + 1] = leftBase[0];
            }
        } else {
            // No attack: every row collapses onto its newest point.
            for (int i = 1; i < 16; i++) {
                rightBase[i] = rightBase[0];
                rightTip[i] = rightTip[0];
                leftBase[i] = leftBase[0];
                leftTip[i] = leftTip[0];
            }
        }
    }
    // A hand without a weapon leaves no trail.
    if (m_rightWeaponSprite == nullptr) {
        for (int i = 1; i < 16; i++) {
            rightBase[i] = rightBase[0];
            rightTip[i] = rightTip[0];
        }
    }
    if (m_leftWeaponSprite == nullptr) {
        for (int i = 1; i < 16; i++) {
            leftBase[i] = leftBase[0];
            leftTip[i] = leftTip[0];
        }
    }

    // --- The hands --- Each hand in the hands' frame at its position with its angles; the front feet of a four-legged
    // creature (AppearanceFlagQuadruped) also swing with the opposite hind foot, half before and half after the
    // position. The right hand is the left hand's sprite mirrored in x.
    if (m_handsSprite != nullptr) {
        if ((hiddenParts & HiddenPartLeftHand) == 0) {
            m_leftHandMatrix = m_handsMatrix;
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
                m_leftHandMatrix.rotate(rightFootSwing, 1.0f, 0.0f, 0.0f);
            m_leftHandMatrix.translate(leftHandPosition[0], leftHandPosition[1], leftHandPosition[2]);
            m_leftHandMatrix.rotateZ(leftHandAngles[2]);
            m_leftHandMatrix.rotateY(leftHandAngles[1]);
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
                m_leftHandMatrix.rotate(rightFootSwing, 1.0f, 0.0f, 0.0f);
            m_leftHandMatrix.rotateX(leftHandAngles[0]);
            m_leftHandMatrix.scale(handsScale, handsScale, handsScale);
            m_leftHandMatrix.translate(0.0017f - (float)m_handsSprite->m_size[0] * 0.5f,
                                       0.0017f - (float)m_handsSprite->m_size[1] * 0.5f,
                                       0.0017f - (float)m_handsSprite->m_size[2] * 0.5f);
            batch.add(m_handsSprite, m_leftHandMatrix, color, alpha, glow,
                      m_creature->m_entityData.equipment.hands.shininess(), m_creature->m_hitFlash, depthOffset);
            // Toggle 0: sprite 3338 in front of and below the left hand, in white with glow 1.
            if (m_creature->m_entityData.binaryToggles.lampOn) {
                Sprite *sprite = spriteManager->sprite(SpriteLantern02);
                if (sprite != nullptr) {
                    plasma::Matrix<float> matrix = m_handsMatrix;
                    matrix.translate(leftHandPosition[0], leftHandPosition[1] + 2.0f, leftHandPosition[2] - 2.0f);
                    matrix.scale(0.8f, 0.8f, 0.8f);
                    matrix.translate((float)sprite->m_size[0] * -0.5f, 1.0f - (float)sprite->m_size[1] * 0.5f,
                                     (float)(2 - sprite->m_size[2]));
                    batch.add(sprite, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), alpha, 1.0f, 0.0f,
                              m_creature->m_hitFlash, depthOffset);
                }
            }
        }
        if ((hiddenParts & HiddenPartRightHand) == 0) {
            // During ability 147, and ability 148 while it lasts, the right hand is sprite 3617 at three quarters of
            // the hands' scale.
            Sprite *sprite = m_handsSprite;
            float rightHandScale = handsScale;
            if (ability == AbilityRockFistCharge ||
                (ability == AbilityRockFist &&
                 (float)m_creature->attackDuration() > m_creature->m_entityData.timeSinceAbility)) {
                sprite = spriteManager->sprite(SpriteRockFist);
                rightHandScale = handsScale * 0.75f;
            }
            m_rightHandMatrix = m_handsMatrix;
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
                m_rightHandMatrix.rotate(leftFootSwing, 1.0f, 0.0f, 0.0f);
            m_rightHandMatrix.translate(rightHandPosition[0], rightHandPosition[1], rightHandPosition[2]);
            m_rightHandMatrix.rotateZ(rightHandAngles[2]);
            m_rightHandMatrix.rotateY(rightHandAngles[1]);
            if ((m_creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped) != 0)
                m_rightHandMatrix.rotate(leftFootSwing, 1.0f, 0.0f, 0.0f);
            m_rightHandMatrix.rotateX(rightHandAngles[0]);
            m_rightHandMatrix.scale(-rightHandScale, rightHandScale, rightHandScale);
            m_rightHandMatrix.translate(0.0023f - (float)sprite->m_size[0] * 0.5f,
                                        0.0023f - (float)sprite->m_size[1] * 0.5f,
                                        0.0023f - (float)sprite->m_size[2] * 0.5f);
            batch.addMirrored(sprite, m_rightHandMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        }
    }

    // --- The wings --- One sprite on each side of the hands' frame, hinged at its edge; the wings beat with half the
    // walk cycle's phase, as far as the cycle's amplitude.
    if (m_wingsSprite != nullptr) {
        float beat = cosf(m_creature->m_walkPhase * 0.5f) * (amplitude * 90.0f) - 45.0f;
        m_leftWingMatrix = m_handsMatrix;
        m_leftWingMatrix.translate(-m_creature->m_entityData.appearance.wingsPosition[0],
                                   m_creature->m_entityData.appearance.wingsPosition[1],
                                   m_creature->m_entityData.appearance.wingsPosition[2]);
        m_leftWingMatrix.rotateX(m_creature->m_entityData.appearance.wingsRotation);
        m_leftWingMatrix.rotateY(beat);
        m_leftWingMatrix.rotateZ(-90.0f);
        m_leftWingMatrix.rotateX(90.0f);
        m_leftWingMatrix.scale(m_creature->m_entityData.appearance.wingScale,
                               m_creature->m_entityData.appearance.wingScale,
                               m_creature->m_entityData.appearance.wingScale);
        m_leftWingMatrix.translate((float)m_wingsSprite->m_size[0] * -0.5f, 0.0f,
                                   (float)m_wingsSprite->m_size[2] * -0.5f);
        batch.add(m_wingsSprite, m_leftWingMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
        m_rightWingMatrix = m_handsMatrix;
        m_rightWingMatrix.translate(m_creature->m_entityData.appearance.wingsPosition[0],
                                    m_creature->m_entityData.appearance.wingsPosition[1],
                                    m_creature->m_entityData.appearance.wingsPosition[2]);
        m_rightWingMatrix.rotateX(m_creature->m_entityData.appearance.wingsRotation);
        m_rightWingMatrix.rotateY(-beat);
        m_rightWingMatrix.rotateZ(-90.0f);
        m_rightWingMatrix.rotateX(90.0f);
        m_rightWingMatrix.scale(m_creature->m_entityData.appearance.wingScale,
                                m_creature->m_entityData.appearance.wingScale,
                                m_creature->m_entityData.appearance.wingScale);
        m_rightWingMatrix.translate((float)m_wingsSprite->m_size[0] * -0.5f, 0.0f,
                                    (float)m_wingsSprite->m_size[2] * -0.5f);
        batch.add(m_wingsSprite, m_rightWingMatrix, color, alpha, glow, 0.0f, m_creature->m_hitFlash, depthOffset);
    }

    // --- The feet --- In the creature's frame (they do not follow the body's pose), turned with the body's turn. Each
    // foot swings about x by its share of the walk cycle plus the pose's feet turn, rolls about y, and is lifted by
    // the stride's size and its own lift. The right foot is the left foot's sprite mirrored in x. The appearance's
    // feet rotation is applied before the position for the left foot and after it for the right one.
    if (m_feetSprite != nullptr) {
        if ((hiddenParts & HiddenPartLeftFoot) == 0) {
            m_leftFootMatrix = creatureMatrix;
            m_leftFootMatrix.rotateZ(m_bodyTurn);
            m_leftFootMatrix.rotate(leftFootSwing + m_feetTurn + m_creature->m_entityData.appearance.feetRotation, 1.0f,
                                    0.0f, 0.0f);
            m_leftFootMatrix.rotateY(footRoll);
            m_leftFootMatrix.translate(leftFootPosition[0], leftFootPosition[1],
                                       strideSize * 3.0f + leftFootPosition[2] + leftFootLift);
            if (ability == AbilitySneak)
                m_leftFootMatrix.rotateX(leftFootSwing * -1.5f);
            m_leftFootMatrix.scale(feetScale, feetScale, feetScale);
            if (leftFootSideways)
                m_leftFootMatrix.rotateZ(60.0f);
            else
                m_leftFootMatrix.rotate(leftFootSwing + m_feetTurn, 1.0f, 0.0f, 0.0f);
            m_leftFootMatrix.translate((float)m_feetSprite->m_size[0] * -0.5f, (float)m_feetSprite->m_size[1] * -0.5f,
                                       (float)m_feetSprite->m_size[2] * -0.5f);
            batch.add(m_feetSprite, m_leftFootMatrix, color, alpha, glow,
                      m_creature->m_entityData.equipment.feet.shininess(), m_creature->m_hitFlash, depthOffset);
        }
        if ((hiddenParts & HiddenPartRightFoot) == 0) {
            m_rightFootMatrix = creatureMatrix;
            m_rightFootMatrix.rotateZ(m_bodyTurn);
            m_rightFootMatrix.rotate(rightFootSwing + m_feetTurn, 1.0f, 0.0f, 0.0f);
            m_rightFootMatrix.rotateY(-footRoll);
            m_rightFootMatrix.translate(rightFootPosition[0], rightFootPosition[1],
                                        strideSize * 3.0f + rightFootPosition[2] + rightFootLift);
            if (ability == AbilitySneak)
                m_rightFootMatrix.rotateX(rightFootSwing * -1.5f);
            m_rightFootMatrix.scale(-feetScale, feetScale, feetScale);
            m_rightFootMatrix.rotate(rightFootSwing + m_feetTurn + m_creature->m_entityData.appearance.feetRotation,
                                     1.0f, 0.0f, 0.0f);
            m_rightFootMatrix.translate((float)m_feetSprite->m_size[0] * -0.5f, (float)m_feetSprite->m_size[1] * -0.5f,
                                        (float)m_feetSprite->m_size[2] * -0.5f);
            batch.addMirrored(m_feetSprite, m_rightFootMatrix, color, alpha, glow,
                              m_creature->m_entityData.equipment.feet.shininess(), m_creature->m_hitFlash, depthOffset);
        }
    }
}

}
