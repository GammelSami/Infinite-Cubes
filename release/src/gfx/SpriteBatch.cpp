// SpriteBatch constructor and initial state.
#include "cube/Noise.h"
#include "gfx/SpriteBatch.h"
#include "cube/Item.h"
#include "cube/Sprite.h"

namespace gfx {

SpriteBatch::SpriteBatch()
    : m_position(0.0f, 0.0f)
{
    m_matrix.identity();
}

void SpriteBatch::add(cube::Sprite *sprite, const plasma::Matrix<float> &matrix, const plasma::Vector<4, float> &color,
                      float alpha, float glow, float shininess, float white, float depthOffset)
{
    if (alpha <= 0.0f)
        return;
    if (alpha < 1.0f) {
        TranslucentInstance translucent = {sprite, Instance(m_matrix * matrix, color, alpha, glow, shininess, white,
                                                            depthOffset, m_position[0], m_position[1])};
        m_translucent.push_back(translucent);
    } else {
        Instance instance(m_matrix * matrix, color, alpha, glow, shininess, white, depthOffset, m_position[0],
                          m_position[1]);
        m_instances[sprite].push_back(instance);
    }
}

void SpriteBatch::addAt(cube::Sprite *sprite, float x, float y, const plasma::Vector<3, float> &rotation, float scale,
                        const plasma::Vector<4, float> &color, float glow, float shininess, float depthOffset)
{
    if (sprite) {
        plasma::Matrix<float> matrix;
        matrix.identity();
        matrix.translate(0.0f, 0.0f, 1.0f);
        matrix.rotateX(rotation[0]);
        matrix.rotateY(rotation[1]);
        matrix.rotateZ(rotation[2]);
        matrix.scale(scale, scale, scale);
        matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                         (float)sprite->m_size[2] * -0.5f);
        if (color[3] < 1.0f) {
            TranslucentInstance translucent = {
                sprite, Instance(matrix, plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f), color[3], glow,
                                 shininess, 0.0f, depthOffset, x, y)};
            m_translucent.push_back(translucent);
        } else {
            Instance instance(matrix, plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f), color[3], glow,
                              shininess, 0.0f, depthOffset, x, y);
            m_instances[sprite].push_back(instance);
        }
    }
}

void SpriteBatch::addMirrored(cube::Sprite *sprite, const plasma::Matrix<float> &matrix,
                              const plasma::Vector<4, float> &color, float alpha, float glow, float shininess,
                              float white, float depthOffset)
{
    if (alpha <= 0.0f)
        return;
    if (alpha < 1.0f) {
        TranslucentInstance translucent = {sprite, Instance(m_matrix * matrix, color, alpha, glow, shininess, white,
                                                            depthOffset, m_position[0], m_position[1])};
        m_mirroredTranslucent.push_back(translucent);
    } else {
        Instance instance(m_matrix * matrix, color, alpha, glow, shininess, white, depthOffset, m_position[0],
                          m_position[1]);
        m_mirroredInstances[sprite].push_back(instance);
    }
}

void SpriteBatch::addSpirits(const cube::Item *item, cube::Sprite *sprite, const plasma::Matrix<float> &matrix,
                             const plasma::Vector<4, float> &color, float materialGlow, float alpha, float glow,
                             float shininess, float white, float depthOffset)
{
    for (int i = 0; i < item->spiritCount; i++) {
        plasma::Matrix<float> spiritMatrix = matrix;
        plasma::Vector<3, float> position(item->spirits[i].position);
        spiritMatrix.translate(position[0], position[1], position[2]);
        add(sprite, spiritMatrix, cube::materialColor(item->spirits[i].material, color, materialGlow), alpha, glow,
            shininess, white, depthOffset);
    }
}

void SpriteBatch::clear()
{
    m_instances.clear();
    m_translucent.clear();
    m_mirroredInstances.clear();
    m_mirroredTranslucent.clear();
    m_position[0] = 0.0f;
    m_position[1] = 0.0f;
    m_matrix.identity();
}

}
