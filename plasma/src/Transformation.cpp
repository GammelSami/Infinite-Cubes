#include "plasma/Transformation.h"

namespace plasma {

Transformation::Transformation(Engine *engine, const std::wstring &name)
    : Keyable(engine, name),
      m_translation("translation", Vector<2, float>(0.0f, 0.0f)),
      m_deformation("deformation", Matrix<float>()),
      m_rotation("rotation", Vector<3, float>(0.0f, 0.0f, 0.0f)),
      m_pivot("pivot", Vector<2, float>(0.0f, 0.0f))
{
    m_engineEntry = engine->addTransformation(this);
    addAttribute(&m_translation);
    addAttribute(&m_deformation);
    addAttribute(&m_rotation);
    addAttribute(&m_pivot);
    m_deformation.value().identity();
    m_matrix.identity();
    m_inverseMatrix.identity();
}

Transformation::~Transformation() {}

// Rebuilds the matrix from the attributes: translation, then rotation and deformation about the pivot, flattened
// to 2D; m_inverseMatrix is its inverse.
void Transformation::attributesChanged(int reason)
{
    m_matrix.identity();
    m_matrix.translate(m_translation.value());
    m_matrix.translate(m_pivot.value());
    m_matrix.rotateX(m_rotation.value().m_elements[0]);
    m_matrix.rotateY(m_rotation.value().m_elements[1]);
    m_matrix.rotateZ(m_rotation.value().m_elements[2]);
    m_matrix *= m_deformation.value();

    // Back by the pivot (the translation written out here).
    Vector<2, float> back = m_pivot.value();
    back.m_elements[0] *= -1.0;
    back.m_elements[1] *= -1.0;
    float *m = m_matrix.m_elements;
    m[12] += m[4] * back.m_elements[1] + m[0] * back.m_elements[0];
    m[13] += m[5] * back.m_elements[1] + m[1] * back.m_elements[0];
    m[14] += m[6] * back.m_elements[1] + m[2] * back.m_elements[0];
    m[15] += m[7] * back.m_elements[1] + m[3] * back.m_elements[0];
    m_matrix.m_elements[2] = 0;
    m_matrix.m_elements[6] = 0;
    m_matrix.m_elements[10] = 1;
    m_matrix.m_elements[14] = 0;
    m_inverseMatrix = m_matrix;
    m_inverseMatrix.invert();
    m_changed = true;
}

// Rotates the node about the pivot (in the parent's coordinates): the angles are added to the rotation attribute,
// the deformation is turned about the z axis through the node's matrices, and the translation keeps the pivot in
// place.
void Transformation::rotate(const Vector<3, float> &rotation, const Matrix<float> &matrix,
                            const Vector<2, float> &pivot, const Matrix<float> &inverseMatrix)
{
    Vector<2, float> local = m_inverseMatrix.transformPoint(pivot);
    Vector<3, float> &angles = m_rotation.value();
    angles.m_elements[0] = rotation.m_elements[0] + angles.m_elements[0];
    angles.m_elements[1] = rotation.m_elements[1] + angles.m_elements[1];
    angles.m_elements[2] = rotation.m_elements[2] + angles.m_elements[2];
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
    Matrix<float> turn;
    turn.identity();
    turn *= inverseMatrix;
    turn.rotateZ(rotation.m_elements[2]);
    turn *= matrix;
    turn.m_elements[12] = 0;
    turn.m_elements[13] = 0;
    turn.m_elements[14] = 0;
    Matrix<float> back;
    back.identity();
    back.rotateZ(-rotation.m_elements[2]);
    Matrix<float> &deformation = m_deformation.value();
    deformation = back * deformation * turn;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
    Vector<2, float> moved = m_matrix * local;
    m_translation.value() = m_translation.value() + (pivot - moved);
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

// Scales the node by scale about the point (in the parent's coordinates): the deformation is scaled in the
// unrotated frame, and the translation keeps the point in place.
void Transformation::scaleAbout(const Vector<2, float> &point, const Vector<2, float> &scale)
{
    // The point in the node's coordinates.
    Vector<2, float> local = m_inverseMatrix.transformPoint(point);
    Vector<3, float> &angles = m_rotation.value();
    Matrix<float> rotation;
    rotation.identity();
    rotation.rotateX(angles.m_elements[0]);
    rotation.rotateY(angles.m_elements[1]);
    rotation.rotateZ(angles.m_elements[2]);
    rotation *= m_deformation.value();
    rotation.m_elements[2] = 0;
    rotation.m_elements[6] = 0;
    rotation.m_elements[10] = 1;
    rotation.m_elements[14] = 0;
    Matrix<float> inverse = rotation;
    inverse.invert();
    m_deformation.value() *= inverse;
    m_deformation.value().scale(scale.m_elements[0], scale.m_elements[1]);
    m_deformation.value() *= rotation;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
    Vector<2, float> moved = m_matrix.transform(local);
    Vector<2, float> offset = point;
    offset.m_elements[0] -= moved.m_elements[0];
    offset.m_elements[1] -= moved.m_elements[1];
    m_translation.value() += offset;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

// Scales the node about the centre (in sheet coordinates, mapped through the parent's inverse matrix): the
// deformation is scaled in the parent's frame, and the translation keeps the centre in place.
void Transformation::scale(const Vector<2, float> &center, const Vector<2, float> &scale,
                           const Matrix<float> &parentMatrix, const Matrix<float> &parentInverseMatrix)
{
    Matrix<float> toLocal = m_inverseMatrix * parentInverseMatrix;
    Vector<2, float> local = toLocal.transform(center);
    Matrix<float> linear = m_matrix;
    linear.m_elements[12] = 0;
    linear.m_elements[13] = 0;
    linear.m_elements[14] = 0;
    linear.m_elements[15] = 1;
    Matrix<float> linearInverse = linear;
    linearInverse.invert();
    Matrix<float> parentLinear = parentMatrix;
    parentLinear.m_elements[12] = 0;
    parentLinear.m_elements[13] = 0;
    parentLinear.m_elements[14] = 0;
    parentLinear.m_elements[15] = 1;
    Matrix<float> parentLinearInverse = parentLinear;
    parentLinearInverse.invert();
    m_deformation.value() *= linearInverse * parentLinearInverse;
    m_deformation.value().scale(scale.m_elements[0], scale.m_elements[1]);
    m_deformation.value() *= parentLinear * linear;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
    Matrix<float> toParent = parentMatrix * m_matrix;
    Vector<2, float> moved = parentInverseMatrix.transform(toParent.transform(local));
    Vector<2, float> target = parentInverseMatrix.transform(center);
    m_translation.value() += target - moved;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

// Moves the pivot to the point (in the node's coordinates) without moving the node: the translation takes up the
// difference.
void Transformation::setPivot(const Vector<2, float> &pivot)
{
    Vector<2, float> before = m_matrix.transform(pivot);
    m_pivot.value() = pivot;
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
    Vector<2, float> after = m_matrix.transform(pivot);
    m_translation.value() = m_translation.value() + (before - after);
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

Transformation *Transformation::clone()
{
    Transformation *transformation = m_engine->createTransformation(m_name);
    transformation->m_translation = m_translation;
    transformation->m_deformation = m_deformation;
    transformation->m_rotation = m_rotation;
    transformation->m_pivot = m_pivot;
    transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return transformation;
}

}
