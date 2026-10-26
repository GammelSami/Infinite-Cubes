// Transformation supplies an animated matrix for scene nodes.
#pragma once
#include <list>
#include <set>
#include "plasma/Keyable.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace svg {
class SVGReader;
}

namespace plasma {

class Node;

class Transformation : public Keyable {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Transformation(Engine *engine, const std::wstring &name);
    virtual ~Transformation();
    virtual Transformation *clone();
    virtual void attributesChanged(int reason);

    Matrix<float> &getDeformation() { return m_deformation.value(); }
    int countNodes() const { return (int)m_nodes.size(); }

protected:
    friend class Widget;
    friend class Edit;
    friend class Node;
    friend class Engine;
    friend class ScalableFont;
    friend class PlasmaFont;
    friend class GLEngine;
    friend class D3D9Engine;

public:
    // Game code sets the attributes directly (GameController's constructor).
    ContinuousAttribute<Vector<2, float>> m_translation;
    ContinuousAttribute<Matrix<float>> m_deformation;
    ContinuousAttribute<Vector<3, float>> m_rotation;
    ContinuousAttribute<Vector<2, float>> m_pivot;

protected:
    std::set<Node *> m_nodes;
    Matrix<float> m_matrix;
    Matrix<float> m_inverseMatrix;
    std::list<Transformation *>::iterator m_engineEntry;
};

}
