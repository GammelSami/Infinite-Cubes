// SpriteBatch groups opaque sprites by model and stores translucent sprites (alpha below 1) in vectors for ordering.
// It keeps separate ordinary and mirrored instance groups.
#pragma once
#include <list>
#include <map>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace cube {
struct Item;
class Sprite;
}

namespace gfx {

class SpriteBatch {
public:
    class Instance {
    public:
        // Each add function constructs one sprite instance.
        Instance(const plasma::Matrix<float> &matrix, const plasma::Vector<4, float> &color, float alpha, float glow,
                 float shininess, float white, float depthOffset, float x, float y)
            : matrix(matrix),
              color(color),
              alpha(alpha),
              parameters(glow, shininess, white, depthOffset),
              position(x, y)
        {}

        plasma::Matrix<float> matrix; // the batch's matrix times the sprite's
        plasma::Vector<4, float> color;
        float alpha; // below 1 the sprite is translucent
        plasma::Vector<4, float> parameters;
        plasma::Vector<2, float> position; // the batch's m_position when the sprite was added
    };

    struct TranslucentInstance {
        cube::Sprite *sprite;
        Instance instance;
    };

    SpriteBatch();

    // Opaque sprites go to m_instances, translucent ones to m_translucent.
    void add(cube::Sprite *sprite, const plasma::Matrix<float> &matrix, const plasma::Vector<4, float> &color,
             float alpha, float glow, float shininess, float white, float depthOffset);
    // The colour's alpha selects the translucent vector and is stored as the instance's alpha, with 1 in the
    // instance's colour; glow, shininess and depthOffset are passed, white is 0. The batch's matrix is not applied.
    void addAt(cube::Sprite *sprite, float x, float y, const plasma::Vector<3, float> &rotation, float scale,
               const plasma::Vector<4, float> &color, float glow, float shininess, float depthOffset);
    void addMirrored(cube::Sprite *sprite, const plasma::Matrix<float> &matrix, const plasma::Vector<4, float> &color,
                     float alpha, float glow, float shininess, float white, float depthOffset);
    void addSpirits(const cube::Item *item, cube::Sprite *sprite, const plasma::Matrix<float> &matrix,
                    const plasma::Vector<4, float> &color, float materialGlow, float alpha, float glow, float shininess,
                    float white, float depthOffset);
    void clear();

    std::map<cube::Sprite *, std::list<Instance>> m_instances;
    std::map<cube::Sprite *, std::list<Instance>> m_mirroredInstances;
    std::vector<TranslucentInstance> m_translucent;
    std::vector<TranslucentInstance> m_mirroredTranslucent;
    plasma::Matrix<float> m_matrix; // applied to the matrix of each sprite added
    plasma::Vector<2, float> m_position;
};

}
