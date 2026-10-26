#include "plasma/Transformation.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

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

Transformation *Transformation::clone()
{
    Transformation *transformation = m_engine->createTransformation(m_name);
    transformation->m_translation = m_translation;
    transformation->m_deformation = m_deformation;
    transformation->m_rotation = m_rotation;
    transformation->m_pivot = m_pivot;
    transformation->attributesChanged(true);
    return transformation;
}

void Transformation::attributesChanged(bool all)
{
    m_matrix.identity();
    m_matrix.translate(m_translation.value());
    m_matrix.translate(m_pivot.value());
    m_matrix.rotateX(m_rotation.value()[0]);
    m_matrix.rotateY(m_rotation.value()[1]);
    m_matrix.rotateZ(m_rotation.value()[2]);
    m_matrix *= m_deformation.value();
    m_matrix.translate(m_pivot.value() * -1.0f);
    m_matrix.m_elements[2] = 0;
    m_matrix.m_elements[6] = 0;
    m_matrix.m_elements[10] = 1;
    m_matrix.m_elements[14] = 0;
    m_inverseMatrix = m_matrix;
    m_inverseMatrix.invert();
    m_changed = true;
}

}
