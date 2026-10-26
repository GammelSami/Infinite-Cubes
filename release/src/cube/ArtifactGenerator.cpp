#include <algorithm>
// Procedural artifact voxel generation; see cube/ArtifactGenerator.h and cube/Noise.h.
#include "cube/ArtifactGenerator.h"
#include "cube/MathConstants.h"
#include "cube/ArtifactGeneratorIds.h"
#include "cube/ItemIds.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

#include "cube/Noise.h"
#include "cube/Sprite.h"

namespace cube {

// World generator's integer noise helper.
static inline unsigned int fastRandomInline(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 100000 - (unsigned int)((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}

ArtifactGenerator::ArtifactGenerator(Sprite *sprite, int type, int seed)
{
    m_sprite = sprite;
    m_seed = seed;
    m_type = static_cast<ArtifactId>(type);
    m_random = fastRandomInline(type, seed);
    sprite->resize(24, 24, 24);
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    m_colors[2] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    m_colors[3] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    switch (type) {
    case Artifact_ArtifactVase:
        vase();
        break;
    case Artifact_ArtifactAmulet:
        amulet();
        break;
    case Artifact_ArtifactRing:
        ring();
        break;
    case Artifact_ArtifactStone:
        stone();
        break;
    case Artifact_ArtifactGem:
        gem(sprite->sizeX() / 2, sprite->sizeY() / 2, sprite->sizeZ() / 2, 8, m_colors[0], m_colors[0] * 0.5f,
            (float)(hash(m_random++) % 1000) / 1000.0f, (float)(hash(m_random++) % 1000) / 1000.0f,
            (float)(hash(m_random++) % 1000) / 1000.0f, (float)(hash(m_random++) % 1000) / 1000.0f);
        break;
    case Artifact_ArtifactCrown:
        crown();
        break;
    case Artifact_ArtifactScepter:
        scepter();
        break;
    default:
        m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) & 127), (float)(hash(m_random++) & 127),
                                               (float)(hash(m_random++) & 127));
        sphere(sprite->sizeX() / 2, sprite->sizeY() / 2, sprite->sizeZ() / 2, 8, m_colors[0], m_colors[0] * 0.5f);
        break;
    }
    sprite->updateMesh(0);
}

void ArtifactGenerator::amulet()
{
    int cx = m_sprite->sizeX() / 2;
    int cy = m_sprite->sizeY() / 2;
    int size = hash(m_random++) % 3 + 3;
    int flat = hash(m_random++) & 1;
    int z = size + 2;
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 50 + 100), (float)(hash(m_random++) % 50 + 100),
                                           (float)(hash(m_random++) % 50 + 100));
    switch (hash(m_random++) & 3) {
    default: {
        int depth = 1;
        switch (flat) {
        default:
            cylinderXZ(cx, cy - 1, z, size + 1, 2, m_colors[1], m_colors[1]);
            break;
        case 1:
            topCylinderXZ(cx, cy, z, size + 1, 1, m_colors[1], m_colors[1]);
            depth = 0;
            break;
        }
        cylinderXZ(cx, cy + depth, z, size, 1, m_colors[0], m_colors[0] * 0.5f);
        m_sprite->setVoxel(cx - 2 + size, cy + depth, size - 2 + z,
                           m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
        break;
    }
    case 1:
        switch (flat) {
        default:
            sphere(cx, cy, z + size, size / 2, m_colors[1], m_colors[1]);
            break;
        case 1:
            topCylinderXZ(cx, cy, z, size + 1, 2, m_colors[1], m_colors[1]);
            topCylinderYZ(cx, cy, z, size + 1, 2, m_colors[1], m_colors[1]);
            break;
        }
        sphere(cx, cy, z, size, m_colors[0], m_colors[0] * 0.5f);
        break;
    case 2:
        switch (flat) {
        default: {
            int half = size * 3 / 4;
            box(cx - half + 1, cy - half + 1, z + size + 2, half * 2 - 2, half * 2 - 2, 1, m_colors[1], m_colors[1]);
            box(cx - half, cy - half, z + size + 1, half * 2, half * 2, 1, m_colors[1], m_colors[1]);
            break;
        }
        case 1:
            box(cx - size - 1, cy - 1, size + 2, size * 2 + 2, 2, size + 1, m_colors[1], m_colors[1]);
            box(cx - 1, cy - size - 1, size + 2, 2, size * 2 + 2, size + 1, m_colors[1], m_colors[1]);
            break;
        }
        box(cx - size, cy - size, 2, size * 2, size * 2, size * 2, m_colors[0], m_colors[0] * 0.5f);
        break;
    case 3: {
        int depth = 1;
        switch (flat) {
        default:
            box(cx - size - 1, cy - 1, 1, size * 2 + 2, 2, size * 2 + 2, m_colors[1], m_colors[1]);
            break;
        case 1:
            box(cx - size - 1, cy, z, size * 2 + 2, 1, size + 1, m_colors[1], m_colors[1]);
            depth = 0;
            break;
        }
        box(cx - size, cy + depth, 2, size * 2, 1, size * 2, m_colors[0], m_colors[0] * 0.5f);
        m_sprite->setVoxel(cx - 2 + size, cy + depth, size - 2 + z,
                           m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
        break;
    }
    }
    int top = z + size;
    int height = m_sprite->sizeZ() - top;
    float spread = (float)height / 3.0f;
    int count = height * 4;
    for (int i = 0; i < count; i++) {
        float fz = (float)top + (float)i * 0.25f;
        float t = (float)i / (float)(count - 1);
        t = t * t;
        float a = fabs((t - 0.5) * 2.0);
        float w = 1.0f - a * a;
        float sway = noise(w * 0.25 + m_seed + 8741.0, m_seed * 13 + fz * 0.25 + 32992.0) * (t * 0.5f * spread);
        m_sprite->setVoxel((int)((float)cx + 0.5f + w * spread + sway), cy, (int)fz, m_colors[1]);
        m_sprite->setVoxel((int)((float)cx + 0.5f - w * spread + sway), cy, (int)fz, m_colors[1]);
    }
}

void ArtifactGenerator::box(int x, int y, int z, int sizeX, int sizeY, int sizeZ,
                            const plasma::Vector<3, float> &bottom, const plasma::Vector<3, float> &top)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            for (int k = z; k < z + sizeZ; k++) {
                float f = (float)(k - z) / (float)sizeZ;
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::crown()
{
    int cx = m_sprite->sizeX() / 2;
    int cy = m_sprite->sizeY() / 2;
    int radius = m_sprite->sizeX() / 2 - hash(m_random++) % 3 - 3;
    int count = hash(m_random++) % 3 * 2 + 2;
    int size = (hash(m_random++) & 1) + 2;
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    for (int i = 0; i < count; i++) {
        float angle = ((float)i - (float)count * 2.0f) / (float)count * 2.0f * Pi + HalfPi;
        int x = (int)(cosf(angle) * (float)(radius + 1) + ((float)cx + 1.0f));
        int y = (int)(sinf(angle) * (float)(radius + 1) + ((float)cy + 1.0f));
        int z = m_sprite->sizeZ() / 4 + size / 2 + 2;
        box(x - size / 2, y - size / 2, z - size / 2, size, size, size, m_colors[0], m_colors[0] * 0.5f);
        m_sprite->setVoxel(x - size / 2 + size - 1, y - size / 2 + size - 1, z - size / 2 + size - 1,
                           m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
    }
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 70 + 180), (float)(hash(m_random++) % 130 + 120),
                                           (float)(hash(m_random++) % 100 + 10));
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 70 + 180), (float)(hash(m_random++) % 130 + 120),
                                           (float)(hash(m_random++) % 100 + 10));
    int spike = hash(m_random++) % 6 + 2;
    for (int i = cx - radius; i <= cx + radius; i++) {
        for (int j = cy - radius; j <= cy + radius; j++) {
            float dy = (float)(j - cx) - 0.5f;
            float dx = (float)(i - cx) - 0.5f;
            float distance = dx * dx + dy * dy;
            if (distance > (float)(radius * radius))
                continue;
            if ((float)((radius - 1) * (radius - 1)) > distance)
                continue;
            int base = m_sprite->sizeZ() / 4;
            int top = spike * 2 + base - abs(spike - (j + cy - spike) % (spike * 2)) -
                      abs(spike - (i + cx - spike) % (spike * 2)) + 2;
            for (int k = base; k < top; k++) {
                float f = (float)(k - base) / (float)(top - base);
                m_sprite->setVoxel(i, j, k, m_colors[0] * (1.0f - f) + m_colors[1] * f);
            }
        }
    }
}

void ArtifactGenerator::cylinder(int x, int y, int z, int radius, int height, const plasma::Vector<3, float> &bottom,
                                 const plasma::Vector<3, float> &top)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            float dy = (float)(j - y) - 0.5f;
            float dx = (float)(i - x) - 0.5f;
            if (dx * dx + dy * dy > (float)(radius * radius))
                continue;
            for (int k = z; k < z + height; k++) {
                float f = (float)(k - z) / (float)height;
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::cylinderXZ(int x, int y, int z, int radius, int length, const plasma::Vector<3, float> &bottom,
                                   const plasma::Vector<3, float> &top)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int k = z - radius; k <= z + radius; k++) {
            float dz = (float)(k - z) - 0.5f;
            float dx = (float)(i - x) - 0.5f;
            if (dx * dx + dz * dz > (float)(radius * radius))
                continue;
            for (int j = y; j < y + length; j++) {
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::gem(int x, int y, int z, int radius, const plasma::Vector<3, float> &bottom,
                            const plasma::Vector<3, float> &top, float cutXY, float cutXZ, float cutYZ, float cutXYZ)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            for (int k = z - radius; k <= z + radius; k++) {
                float dx = (float)abs(i - x);
                float dy = (float)abs(j - y);
                float dz = (float)abs(k - z);
                float distance =
                    std::max(dx, std::max(dy, std::max(dz, std::max((dx + dy) * cutXY,
                                                                    std::max((dx + dz) * cutXZ,
                                                                             std::max((dy + dz) * cutYZ,
                                                                                      (dx + dy + dz) * cutXYZ))))));
                if (distance > (float)radius)
                    continue;
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

plasma::Vector<3, float> hsv2rgb(const plasma::Vector<3, float> &hsv)
{
    plasma::Vector<3, float> rgb;
    if (hsv[1] <= 0.0f) {
        rgb[0] = hsv[2];
        rgb[1] = hsv[2];
        rgb[2] = hsv[2];
        return rgb;
    }
    double hh = hsv[0];
    if (hh >= 360.0)
        hh = 0.0;
    hh /= 60.0;
    HueSector i = static_cast<HueSector>((int)hh);
    double ff = hh - i;
    double p = hsv[2] * (1.0 - hsv[1]);
    double q = hsv[2] * (1.0 - (hsv[1] * ff));
    double t = hsv[2] * (1.0 - (hsv[1] * (1.0 - ff)));
    switch (i) {
    case HueSectorRedToYellow:
        rgb[0] = hsv[2];
        rgb[1] = (float)t;
        rgb[2] = (float)p;
        break;
    case HueSectorYellowToGreen:
        rgb[0] = (float)q;
        rgb[1] = hsv[2];
        rgb[2] = (float)p;
        break;
    case HueSectorGreenToCyan:
        rgb[0] = (float)p;
        rgb[1] = hsv[2];
        rgb[2] = (float)t;
        break;
    case HueSectorCyanToBlue:
        rgb[0] = (float)p;
        rgb[1] = (float)q;
        rgb[2] = hsv[2];
        break;
    case HueSectorBlueToMagenta:
        rgb[0] = (float)t;
        rgb[1] = (float)p;
        rgb[2] = hsv[2];
        break;
    case HueSectorMagentaToRed:
    default:
        rgb[0] = hsv[2];
        rgb[1] = (float)p;
        rgb[2] = (float)q;
        break;
    }
    return rgb;
}

plasma::Vector<3, float> rgb2hsv(const plasma::Vector<3, float> &rgb)
{
    plasma::Vector<3, float> hsv;
    double min, max, delta;

    min = rgb[0] < rgb[1] ? rgb[0] : rgb[1];
    min = min < rgb[2] ? min : rgb[2];

    max = rgb[0] > rgb[1] ? rgb[0] : rgb[1];
    max = max > rgb[2] ? max : rgb[2];

    hsv[2] = (float)max;
    delta = max - min;
    if (delta < 0.00001) {
        hsv[1] = 0;
        hsv[0] = 0;
        return hsv;
    }
    if (max > 0.0) {
        hsv[1] = (float)(delta / max);
    } else {
        hsv[1] = 0.0;
        hsv[0] = NAN;
        return hsv;
    }
    if (rgb[0] >= max)
        hsv[0] = (float)((rgb[1] - rgb[2]) / delta);
    else if (rgb[1] >= max)
        hsv[0] = (float)(2.0 + (rgb[2] - rgb[0]) / delta);
    else
        hsv[0] = (float)(4.0 + (rgb[0] - rgb[1]) / delta);

    hsv[0] *= 60.0;

    if (hsv[0] < 0.0)
        hsv[0] += 360.0;

    return hsv;
}

void ArtifactGenerator::ring()
{
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 70 + 100), (float)(hash(m_random++) % 70 + 100),
                                           (float)(hash(m_random++) % 70 + 100));
    int cx = m_sprite->sizeX() / 2;
    int cy = m_sprite->sizeY() / 2;
    int cz = m_sprite->sizeZ() / 2;
    int radius = m_sprite->sizeX() / 4 + hash(m_random++) % (m_sprite->sizeX() / 4 - 2);
    int width = hash(m_random++) % 5 + 1;
    int thickness = hash(m_random++) % 3 + 1;
    int kind = hash(m_random++) & 3;
    switch (kind) {
    default:
        ringXZ(cx, cy - width / 2, cz, radius, width, thickness, m_colors[1], m_colors[1] * 0.75f);
        break;
    case 1:
        ringXZ(cx, cy - width / 2, cz, radius - 1, width, thickness, m_colors[2], m_colors[2] * 0.75f);
        ringXZ(cx, cy - width / 2 - 2, cz, radius, 2, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 + width, cz, radius, 2, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        radius--;
        break;
    case 2:
        ringXZ(cx, cy - width / 2, cz, radius + 1, width, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 - 2, cz, radius, 2, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 + width, cz, radius, 2, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        break;
    case 3:
        ringXZ(cx, cy - width / 2, cz, radius - 1, width, thickness, m_colors[2], m_colors[2] * 0.75f);
        ringXZ(cx, cy - width / 2 - 2, cz, radius - 1, 1, thickness, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 - 1, cz, radius, 1, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 + width, cz, radius, 1, thickness + 1, m_colors[1], m_colors[1] * 0.75f);
        ringXZ(cx, cy - width / 2 + width + 1, cz, radius - 1, 1, thickness, m_colors[1], m_colors[1] * 0.75f);
        radius--;
        break;
    }
    if (hash(m_random++) & 1) {
        int count = (hash(m_random++) & 3) + 2;
        int size = hash(m_random++) % width + 2;
        for (int i = 0; i < count; i++) {
            float angle = ((float)i - (float)count * 2.0f) / (float)count * 2.0f * Pi + HalfPi;
            int x = (int)(cosf(angle) * (float)(radius + 1) + ((float)cx + 1.5f)) - size / 2;
            int y = cy - size / 2;
            int z = (int)(sinf(angle) * (float)(radius + 1) + ((float)cz + 1.5f)) - size / 2;
            box(x, y, z, size, size, size, m_colors[0], m_colors[0] * 0.5f);
            m_sprite->setVoxel(x - 1 + size, y - 1 + size, z - 1 + size,
                               m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
        }
    } else {
        int gemRadius = width / 2 + 1 + (hash(m_random++) & 1);
        int gemHeight = (hash(m_random++) & 1) + 1;
        switch (hash(m_random++) & 1) {
        default:
            cylinder(cx, cy, radius + cz + 1, gemRadius + 1, 1, m_colors[1], m_colors[1]);
            cylinder(cx, cy, radius + cz + 2, gemRadius, gemHeight, m_colors[0], m_colors[0]);
            m_sprite->setVoxel((int)((float)(cx + 1) - (float)gemRadius * 0.5f),
                               (int)((float)(cy + 1) - (float)gemRadius * 0.5f), gemHeight + 1 + radius + cz,
                               m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
            break;
        case 1:
            box(cx - gemRadius, cy - gemRadius - 1, cz + radius + 1, gemRadius * 2 + 2, gemRadius * 2 + 2, 1,
                m_colors[1], m_colors[1]);
            box(cx - gemRadius + 1, cy - gemRadius, cz + radius + 2, gemRadius * 2, gemRadius * 2, gemHeight,
                m_colors[0], m_colors[0]);
            m_sprite->setVoxel(cx + gemRadius, gemRadius - 1 + cy, cz + 1 + (radius + gemHeight),
                               m_colors[0] + plasma::Vector<3, float>(150.0f, 150.0f, 150.0f));
            break;
        }
    }
}

void ArtifactGenerator::ringXZ(int x, int y, int z, int radius, int length, int thickness,
                               const plasma::Vector<3, float> &bottom, const plasma::Vector<3, float> &top)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int k = z - radius; k <= z + radius; k++) {
            float dz = (float)(k - z) - 0.5f;
            float dx = (float)(i - x) - 0.5f;
            float distance = dx * dx + dz * dz;
            if (distance > (float)(radius * radius))
                continue;
            if ((float)((radius - thickness) * (radius - thickness)) > distance)
                continue;
            for (int j = y; j < y + length; j++) {
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::scepter()
{
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 70 + 180), (float)(hash(m_random++) % 130 + 120),
                                           (float)(hash(m_random++) % 100 + 10));
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 70 + 180), (float)(hash(m_random++) % 130 + 120),
                                           (float)(hash(m_random++) % 100 + 10));
    int radius = hash(m_random++) % 3 + 3;
    int cx = m_sprite->sizeX() / 2;
    int cy = m_sprite->sizeY() / 2;
    int height = m_sprite->sizeZ() - radius * 2 - 1;
    box(cx - 1, cy - 1, 0, 3, 3, height, m_colors[0], m_colors[1]);
    int rings = hash(m_random++) % 3 + 1;
    for (int i = 0; i < rings; i++) {
        float f = (float)i / (float)rings;
        plasma::Vector<3, float> color = m_colors[1] * (1.0f - f) + m_colors[0] * f;
        box(cx - 2, cy - 2, (int)((float)height * (1.0f - f)), 5, 5, 1, color, color);
    }
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    m_colors[1] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    gem(cx, cy, height + radius, radius, m_colors[0], m_colors[0] * 0.5f, (float)(hash(m_random++) % 1000) / 1000.0f,
        (float)(hash(m_random++) % 1000) / 1000.0f, (float)(hash(m_random++) % 1000) / 1000.0f,
        (float)(hash(m_random++) % 1000) / 1000.0f);
}

void ArtifactGenerator::sphere(int x, int y, int z, int radius, const plasma::Vector<3, float> &bottom,
                               const plasma::Vector<3, float> &top)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            for (int k = z - radius; k <= z + radius; k++) {
                float dz = (float)(k - z) - 0.5f;
                float dy = (float)(j - y) - 0.5f;
                float dx = (float)(i - x) - 0.5f;
                if (dx * dx + dy * dy + dz * dz > (float)(radius * radius))
                    continue;
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::stone()
{
    int sizeX = hash(m_random++) % (m_sprite->sizeX() / 2) + m_sprite->sizeX() / 4;
    int sizeY = hash(m_random++) % (m_sprite->sizeY() / 2) + m_sprite->sizeY() / 4;
    int sizeZ = hash(m_random++) % (m_sprite->sizeZ() / 2) + m_sprite->sizeZ() / 4;
    box((m_sprite->sizeX() - sizeX) / 2, (m_sprite->sizeY() - sizeY) / 2, (m_sprite->sizeZ() - sizeZ) / 2, sizeX, sizeY,
        sizeZ, m_colors[0], m_colors[1]);
}

void ArtifactGenerator::topCylinderXZ(int x, int y, int z, int radius, int length,
                                      const plasma::Vector<3, float> &bottom, const plasma::Vector<3, float> &top)
{
    for (int i = x - radius; i <= x + radius; i++) {
        for (int k = z; k <= z + radius; k++) {
            float dz = (float)(k - z) - 0.5f;
            float dx = (float)(i - x) - 0.5f;
            if (dx * dx + dz * dz > (float)(radius * radius))
                continue;
            for (int j = y; j < y + length; j++) {
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::topCylinderYZ(int x, int y, int z, int radius, int length,
                                      const plasma::Vector<3, float> &bottom, const plasma::Vector<3, float> &top)
{
    for (int j = y - radius; j <= y + radius; j++) {
        for (int k = z; k <= z + radius; k++) {
            float dz = (float)(k - z) - 0.5f;
            float dy = (float)(j - y) - 0.5f;
            if (dy * dy + dz * dz > (float)(radius * radius))
                continue;
            for (int i = x; i < x + length; i++) {
                float f = (float)(k - z + radius) / (float)(radius * 2);
                m_sprite->setVoxel(i, j, k, bottom * (1.0f - f) + top * f);
            }
        }
    }
}

void ArtifactGenerator::vase()
{
    float widest = m_sprite->sizeX() / 2 - hash(m_random++) % (m_sprite->sizeX() / 4) - 4;
    float bottom = widest * 0.5f;
    float top = m_sprite->sizeX() / 2 - hash(m_random++) % (m_sprite->sizeX() / 4) - 4;
    int height = m_sprite->sizeZ() - hash(m_random++) % (m_sprite->sizeZ() / 2);
    int profile = hash(m_random++) % 3;
    int pattern = hash(m_random++) & 3;
    int cx = m_sprite->sizeX() / 2;
    int cy = m_sprite->sizeY() / 2;
    m_colors[0] = plasma::Vector<3, float>((float)(hash(m_random++) % 250 + 10), (float)(hash(m_random++) % 250 + 10),
                                           (float)(hash(m_random++) % 250 + 10));
    m_colors[1] = m_colors[0] + plasma::Vector<3, float>(30.0f, 30.0f, 30.0f);
    long long handles = (int)(hash(m_random++) % 3);
    int handleStart = hash(m_random++) % (height / 4 + 1);
    bool rim = hash(m_random++) & 1;
    bool foot = hash(m_random++) & 1;
    bool plainHandles = hash(m_random++) & 1;
    plasma::Vector<3, float> hsv = rgb2hsv(m_colors[0] / 255.0f);
    hsv[0] += 180.0f;
    m_colors[2] = hsv2rgb(hsv) * 255.0f;
    for (int z = 0; z < height; z++) {
        float t = (float)z / ((float)height - 1.0f);
        plasma::Vector<3, float> color = m_colors[0] * (1.0f - t) + m_colors[1] * t;
        switch (profile) {
        case 1:
            t = t * t;
            break;
        case 2:
            t = 1.0f - (1.0f - t) * (1.0f - t);
            break;
        }
        float u = 1.0f - t;
        float radius = (u * bottom + t * widest) * u + (u * widest + t * top) * t;
        for (int x = (int)((float)cx - radius - 2.0f); x <= (float)cx + radius + 2.0f; x++) {
            for (int y = (int)((float)cy - radius - 2.0f); y <= radius + 2.0f + (float)cy; y++) {
                float dy = (float)(y - cy) + 0.5f;
                float dx = (float)(x - cx) + 0.5f;
                float distance = dx * dx + dy * dy;
                if (distance > radius * radius)
                    continue;
                if (z > 0 && (radius - 1.0f) * (radius - 1.0f) > distance)
                    continue;
                plasma::Vector<3, float> c = color;
                switch (pattern) {
                case 1:
                    if (z / 2 % 3 == 0)
                        c = m_colors[2];
                    break;
                case 2:
                    if (((z / 4 + y / 2 + x / 2) & 1) == 0 && (z & 2) == 0)
                        c = m_colors[2];
                    break;
                case 3: {
                    float n = noise(y * 0.3 + 155.0 + m_seed, z * 0.5 + 1003.0 + m_seed) +
                              noise(x * 0.3 + 3411.0 + m_seed, z * 0.5 + 2131.0 + m_seed);
                    float v = n < 0.0f ? 0.0f : n;
                    v = v > 1.0f ? 1.0f : v;
                    v = v * v * v;
                    c = color + (m_colors[2] - color) * v;
                    break;
                }
                }
                if (rim && z >= height - 2)
                    c = m_colors[3];
                if (foot && z <= 1)
                    c = m_colors[3];
                m_sprite->setVoxel(x, y, z, c);
            }
        }
        if (handles > 0 && z >= handleStart && z <= height - 1) {
            float f = (float)(z - handleStart) / (float)(height - 1 - handleStart);
            float bend = (0.5f - f) * 2.0f;
            int hx = (int)((float)cx + 0.5f + radius + (1.0f - bend * bend) * 5.0f);
            if (plainHandles)
                color = m_colors[3];
            for (int y = cy - 1; y <= cy; y++) {
                if (plasma::Vector<3, unsigned char>(0, 0, 0) == m_sprite->voxel(hx, y, z))
                    m_sprite->setVoxel(hx, y, z, color);
                if (plasma::Vector<3, unsigned char>(0, 0, 0) == m_sprite->voxel(hx - 1, y, z))
                    m_sprite->setVoxel(hx - 1, y, z, color);
            }
            if (handles > 1) {
                // The second handle, mirrored in x.
                int mirroredX = m_sprite->sizeX() - hx;
                for (int y = cy - 1; y <= cy; y++) {
                    if (plasma::Vector<3, unsigned char>(0, 0, 0) == m_sprite->voxel(mirroredX - 1, y, z))
                        m_sprite->setVoxel(mirroredX - 1, y, z, color);
                    if (plasma::Vector<3, unsigned char>(0, 0, 0) == m_sprite->voxel(mirroredX, y, z))
                        m_sprite->setVoxel(mirroredX, y, z, color);
                }
            }
        }
    }
}

}
