// Transformation provides animated translation, pivot, rotation and deformation, with cached forward and inverse
// matrices.
#pragma once
#include <list>
#include <set>
#include <string>
#include "plasma/Keyable.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Node;

class Transformation : public Keyable {
    friend class PLXStreamReader; // Reads Transformation.deformation and the other animation chunks.
public:
    Transformation(Engine *engine, const std::wstring &name);
    virtual ~Transformation();

    virtual void attributesChanged(bool all); // (ignores the argument)
    virtual Transformation *clone();

    int countNodes() const { return m_nodes.size(); } // Matrix accessor used by Node comparisons.

    ContinuousAttribute<Vector<2, float>> m_translation;
    ContinuousAttribute<Matrix<float>> m_deformation; // current element starts as the identity
    ContinuousAttribute<Vector<3, float>> m_rotation;
    ContinuousAttribute<Vector<2, float>> m_pivot;

protected:
    friend class Node;        // registers itself in m_nodes
    friend class Engine;      // erases m_engineEntry when deleting the transformation
    std::set<Node *> m_nodes; // the nodes that use this transformation
public:
    Matrix<float> m_matrix;        // Forward matrix, initially identity.
    Matrix<float> m_inverseMatrix; // Inverse of m_matrix, initially identity.
protected:
    std::list<Transformation *>::iterator m_engineEntry; // Registration entry in the engine's transformation list.
};

}
