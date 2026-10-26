// ContinuousArrayAttribute stores arrays of floats or vectors for positions, colours and widths. Each element owns
// an array. Cubic interpolation uses quarter-chord and quarter-neighbor-difference tangents with float and double
// arithmetic.
#pragma once
#include "plasma/Attribute.h"

namespace plasma {

template<typename T> class ContinuousArrayAttribute : public Attribute {
public:
    // Starts with two elements, each a copy of the initial array.
    ContinuousArrayAttribute(const std::string &name, const std::vector<T> &initial)
        : Attribute(name)
    {
        m_values.resize(2);
        m_values[0] = new std::vector<T>(initial);
        m_values[1] = new std::vector<T>(initial);
    }

    // Deep-copies the other attribute's elements into an unnamed attribute.
    ContinuousArrayAttribute(const ContinuousArrayAttribute &other);

    virtual ~ContinuousArrayAttribute()
    {
        for (int i = 0; i < (int)m_values.size(); i++)
            delete m_values[i];
    }

    virtual int size();
    virtual int duplicateElement();
    virtual void copyToCurrent(int element);
    virtual void removeElement(int element);
    virtual void copyFromCurrent(int element);
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3);
    // Replaces the elements with copies of the other attribute's elements.
    ContinuousArrayAttribute &operator=(const ContinuousArrayAttribute &other);

    std::vector<T> &value() { return *m_values[m_current]; }

    void append(const T &value);
    // Removes the signed array index from every element.
    void erase(unsigned index);
    // Gives every element the same length.
    void resize(unsigned size);

    // Appends the other attribute's elements, also at its keys (the mesh merge, MeshInterface.cpp).
    void appendElements(ContinuousArrayAttribute &other);

    std::vector<std::vector<T> *> m_values;
};

template<typename T> void ContinuousArrayAttribute<T>::erase(unsigned index)
{
    for (int i = 0; i < (int)m_values.size(); ++i)
        m_values[i]->erase(m_values[i]->begin() + index);
}

template<typename T>
ContinuousArrayAttribute<T> &ContinuousArrayAttribute<T>::operator=(const ContinuousArrayAttribute &other)
{
    Attribute::operator=(other);
    for (int i = 0; i < (int)m_values.size(); i++)
        delete m_values[i];
    m_values.resize(other.m_values.size());
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i] = new std::vector<T>(*other.m_values[i]);
    return *this;
}

// Appends the other attribute's entries in the current element and at every key frame of either attribute's
// sequences. Each sequence is walked once per key. Element 1 holds the current element meanwhile. Used by mesh
// merging.
template<typename T> void ContinuousArrayAttribute<T>::appendElements(ContinuousArrayAttribute &other)
{
    int size = m_values[0]->size();
    resize(m_values[0]->size() + other.m_values[0]->size());
    for (int i = 0; i < (int)other.m_values[0]->size(); i++)
        value()[size + i] = other.value()[i];
    other.copyFromCurrent(STORED_ELEMENT);
    copyFromCurrent(STORED_ELEMENT);
    for (std::map<std::wstring, Sequence *>::iterator it = other.m_sequences.begin(); it != other.m_sequences.end();
         ++it) {
        for (int pass = 0; pass < it->second->count(); pass++) {
            Sequence *sequence = it->second;
            for (int key = 0; key < sequence->count(); key++) {
                other.copyToCurrent(STORED_ELEMENT);
                copyToCurrent(STORED_ELEMENT);
                showFrame(it->first, (*sequence)[key].frame);
                other.showFrame(it->first, (*sequence)[key].frame);
                for (int i = 0; i < (int)other.m_values[0]->size(); i++)
                    value()[size + i] = other.value()[i];
                setKey(it->first, (*sequence)[key].frame, 1.0f);
            }
        }
    }
    for (std::map<std::wstring, Sequence *>::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it) {
        for (int pass = 0; pass < it->second->count(); pass++) {
            Sequence *sequence = it->second;
            for (int key = 0; key < sequence->count(); key++) {
                other.copyToCurrent(STORED_ELEMENT);
                copyToCurrent(STORED_ELEMENT);
                showFrame(it->first, (*sequence)[key].frame);
                other.showFrame(it->first, (*sequence)[key].frame);
                for (int i = 0; i < (int)other.m_values[0]->size(); i++)
                    value()[size + i] = other.value()[i];
                setKey(it->first, (*sequence)[key].frame, 1.0f);
            }
        }
    }
    other.copyToCurrent(STORED_ELEMENT);
    copyToCurrent(STORED_ELEMENT);
}

// Adds an element: a copy of the current one, or an empty one when there is none
// (ContinuousArrayAttribute<float>).
template<typename T> int ContinuousArrayAttribute<T>::duplicateElement()
{
    if (m_values.empty())
        m_values.push_back(new std::vector<T>());
    else
        m_values.push_back(new std::vector<T>(*m_values[m_current]));
    return m_values.size() - 1;
}

template<typename T> void ContinuousArrayAttribute<T>::copyToCurrent(int element)
{
    // Writes into the first element rather than the current one.
    if (element >= 0 && element < (int)m_values.size())
        *m_values[0] = *m_values[element];
}

template<typename T> void ContinuousArrayAttribute<T>::removeElement(int element)
{
    if (element >= 0 && element < (int)m_values.size()) {
        delete m_values[element];
        m_values.erase(m_values.begin() + element);
    }
}

template<typename T> void ContinuousArrayAttribute<T>::copyFromCurrent(int element)
{
    if (element >= 0 && element < (int)m_values.size())
        *m_values[element] = *m_values[m_current];
}

// ContinuousAttribute's Bezier blend of the start and end elements, entry by entry.
template<typename T>
void ContinuousArrayAttribute<T>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                              float endSmoothness, float t, float weight0, float weight1, float weight2,
                                              float weight3)
{
    for (int i = 0; i < (int)m_values[start]->size(); i++) {
        T chord = ((*m_values[end])[i] - (*m_values[start])[i]) * 0.25;
        T endTangent = ((*m_values[after])[i] - (*m_values[start])[i]) * 0.25;
        T endControl = (*m_values[end])[i] - (endTangent * endSmoothness + chord * (1 - endSmoothness));
        T startTangent = ((*m_values[end])[i] - (*m_values[before])[i]) * 0.25;
        T startControl = startTangent * startSmoothness + chord * (1 - startSmoothness) + (*m_values[start])[i];
        (*m_values[m_current])[i] = startControl * weight1 + (*m_values[start])[i] * weight0 + endControl * weight2 +
                                    (*m_values[end])[i] * weight3;
    }
}

template<typename T> void ContinuousArrayAttribute<T>::resize(unsigned size)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->resize(size);
}

template<typename T>
ContinuousArrayAttribute<T>::ContinuousArrayAttribute(const ContinuousArrayAttribute &other)
    : Attribute()
{
    *this = other;
}

}
