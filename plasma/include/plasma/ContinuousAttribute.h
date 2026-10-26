// ContinuousAttribute stores values that blend between keys: floats, vectors and matrices.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"

class Workspace;

class ClipboardModule;

namespace plasma {

template<unsigned N, typename T> class Vector;
template<typename T> class Matrix;

template<typename T> class ContinuousAttribute : public Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    // MoveNode rewrites every key of the translation and deformation.
    friend class ::Workspace;
    friend class ::ClipboardModule;

    // The legacy PLA reader and writer (Engine.cpp) index the values directly.
    friend void writePLAVector2Attribute(std::ostream &, ContinuousAttribute<Vector<2, float>> *);
    friend void writePLAVector3Attribute(std::ostream &, ContinuousAttribute<Vector<3, float>> *);
    friend void writePLAMatrixAttribute(std::ostream &, ContinuousAttribute<Matrix<float>> *);
    friend void writePLAVector4Attribute(std::ostream &, ContinuousAttribute<Vector<4, float>> *);
    friend void readPLAVector2Attribute(std::istream &, ContinuousAttribute<Vector<2, float>> *,
                                        std::map<int, std::string> &);
    friend void readPLAVector3Attribute(std::istream &, ContinuousAttribute<Vector<3, float>> *,
                                        std::map<int, std::string> &);
    friend void readPLAMatrixAttribute(std::istream &, ContinuousAttribute<Matrix<float>> *,
                                       std::map<int, std::string> &);
    friend void readPLAVector4Attribute(std::istream &, ContinuousAttribute<Vector<4, float>> *,
                                        std::map<int, std::string> &);

public:
    // Starts with two copies of the initial value.
    ContinuousAttribute(const std::string &name = "", const T &initial = T());

    // Copies an unnamed attribute, including its elements and animation state.
    ContinuousAttribute(const ContinuousAttribute &other);

    // The element in use.
    T &operator=(const T &value);

    T &value() { return m_values[m_current]; }

    virtual int size() { return m_values.size(); }

    virtual int duplicateElement()
    {
        if (m_values.empty())
            m_values.push_back(T());
        else
            m_values.push_back(m_values[m_current]);
        return m_values.size() - 1;
    }

    // Replaces the current element; used by TextInterface::RecordColorEdit.
    virtual void copyToCurrent(int element);

    virtual void removeElement(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values.erase(m_values.begin() + element);
    }

    virtual void copyFromCurrent(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values[element] = m_values[m_current];
    }

    // A cubic Bezier segment from the start to the end element. Each key's control point lies a quarter along a blend,
    // by the key's smoothness, of the chord and the tangent through the neighbouring elements. The float instance is
    // specialized.
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3)
    {
        const T &beforeValue = m_values[before];
        const T &startValue = m_values[start];
        const T &endValue = m_values[end];
        const T &afterValue = m_values[after];
        T startTangent = (endValue - beforeValue) * 0.25f;
        T endTangent = (afterValue - startValue) * 0.25f;
        T chord = (endValue - startValue) * 0.25f;
        T startControl = startValue + (startTangent * startSmoothness + chord * (float)(1.0 - startSmoothness));
        T endControl = endValue - (endTangent * endSmoothness + chord * (float)(1.0 - endSmoothness));
        m_values[m_current] = startValue * weight0 + startControl * weight1 + endControl * weight2 + endValue * weight3;
    }

protected:
    std::vector<T> m_values;
};

template<typename T>
ContinuousAttribute<T>::ContinuousAttribute(const std::string &name, const T &initial)
    : Attribute(name)
{
    m_values.resize(2, initial);
}

template<typename T>
ContinuousAttribute<T>::ContinuousAttribute(const ContinuousAttribute &other)
    : Attribute()
{
    *this = other;
}

template<typename T> T &ContinuousAttribute<T>::operator=(const T &value)
{
    this->value() = value;
    return this->value();
}

template<typename T> void ContinuousAttribute<T>::copyToCurrent(int element)
{
    if (element >= 0 && element < (int)m_values.size())
        m_values[m_current] = m_values[element];
}

template<>
void ContinuousAttribute<float>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                             float endSmoothness, float t, float weight0, float weight1, float weight2,
                                             float weight3);

}
