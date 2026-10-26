// Array-attribute template implementations for float, Vector2 and Vector4 elements used by mesh shapes.
#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include "MeshInterface.h"

namespace plasma {

template<typename T> int ContinuousArrayAttribute<T>::size()
{
    return m_values.size();
}

template<typename T> void ContinuousArrayAttribute<T>::append(const T &value)
{
    for (int i = 0; i < (int)m_values.size(); ++i)
        m_values[i]->push_back(value);
}

template class ContinuousArrayAttribute<Vector<2, float>>;
template class ContinuousArrayAttribute<Vector<4, float>>;
template class ContinuousArrayAttribute<float>;

}
