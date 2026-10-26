// Vector stores fixed-length coordinates and provides element-wise arithmetic, dot and cross products and
// transformations.
#pragma once
#include <algorithm>
#include <math.h>

namespace plasma {

template<typename T> class Matrix;

template<unsigned N, typename T> class Vector {
public:
    Vector() {}
    Vector(T x, T y)
    {
        m_elements[0] = x;
        m_elements[1] = y;
    }
    Vector(T x, T y, T z)
    {
        m_elements[0] = x;
        m_elements[1] = y;
        m_elements[2] = z;
    }
    Vector(T x, T y, T z, T w)
    {
        m_elements[0] = x;
        m_elements[1] = y;
        m_elements[2] = z;
        m_elements[3] = w;
    }

    // Copies vector elements individually.
    Vector(const Vector &other)
    {
        for (int i = 0; i < (int)N; i++)
            m_elements[i] = other.m_elements[i];
    }

    Vector &operator=(const Vector &other)
    {
        for (int i = 0; i < (int)N; i++)
            (*this)[i] = other[i];
        return *this;
    }

    template<typename U> explicit Vector(const Vector<N, U> &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = (T)other.m_elements[i];
    }

    template<typename U> Vector &operator=(const Vector<N, U> &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = (T)other.m_elements[i];
        return *this;
    }

    T &operator[](int index) { return m_elements[index]; }
    const T &operator[](int index) const { return m_elements[index]; }

    Vector operator*(T scale) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] *= scale;
        return result;
    }

    Vector operator*(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] *= other.m_elements[i];
        return result;
    }

    Vector operator+(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] += other.m_elements[i];
        return result;
    }

    Vector &operator*=(T scale)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] *= scale;
        return *this;
    }

    Vector &operator/=(T divisor)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] /= divisor;
        return *this;
    }

    Vector &operator*=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] *= other.m_elements[i];
        return *this;
    }

    // Adds in place.
    Vector &operator+=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] += other.m_elements[i];
        return *this;
    }

    Vector operator-() const
    {
        Vector result = *this;
        result *= (T)-1;
        return result;
    }

    Vector operator-(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] -= other.m_elements[i];
        return result;
    }

    Vector operator/(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= other.m_elements[i];
        return result;
    }

    Vector operator/(T divisor) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= divisor;
        return result;
    }

    // The dot product, accumulated from the first element on: the loop over the remaining elements is kept, and
    // Renderer::attenuation adds x*x + y*y first, then z*z.
    T dot(const Vector &other) const
    {
        T result = m_elements[0] * other.m_elements[0];
        for (unsigned i = 1; i < N; i++)
            result += m_elements[i] * other.m_elements[i];
        return result;
    }
    // Dot product for Vector<3, float>, evaluated as y*y' + x*x' followed by z*z'.
    T operator&(const Vector &other) const;

    bool operator!=(const Vector &other) const
    {
        for (int i = 0; i < (int)N; i++) {
            if (m_elements[i] != other.m_elements[i])
                return true;
        }
        return false;
    }

    bool operator==(const Vector &other) const
    {
        for (int i = 0; i < (int)N; i++) {
            if (m_elements[i] != other.m_elements[i])
                return false;
        }
        return true;
    }

    // Lexicographic order, for ordered containers of vectors.
    bool operator<(const Vector &other) const
    {
        return std::lexicographical_compare(m_elements, m_elements + N, other.m_elements, other.m_elements + N);
    }

    // Vector operations used by Creature::AnimationState::addSprites.
    T squaredLength() const
    {
        T result = m_elements[0] * m_elements[0];
        for (unsigned i = 1; i < N; i++)
            result += m_elements[i] * m_elements[i];
        return result;
    }
    // The length.
    T length() const { return sqrt(squaredLength()); }
    Vector &normalize()
    {
        T scale = (T)1 / sqrt(squaredLength());
        for (unsigned i = 0; i < N; i++)
            m_elements[i] *= scale;
        return *this;
    }
    // A copy scaled to length 1.
    Vector normalized() const
    {
        T scale = (T)1 / length();
        return *this * scale;
    }
    // The cross product, for three elements.
    Vector cross(const Vector &other) const
    {
        return Vector(m_elements[1] * other.m_elements[2] - m_elements[2] * other.m_elements[1],
                      m_elements[2] * other.m_elements[0] - m_elements[0] * other.m_elements[2],
                      m_elements[0] * other.m_elements[1] - m_elements[1] * other.m_elements[0]);
    }

    // Transforms a 2D point in place with perspective division; defined in Matrix.h.
    void transform(const Matrix<T> &matrix);

    T m_elements[N];
};

// The float instances of two and three elements add their squares in one expression.
template<> inline float Vector<2, float>::squaredLength() const
{
    return m_elements[0] * m_elements[0] + m_elements[1] * m_elements[1];
}

template<> inline float Vector<3, float>::squaredLength() const
{
    return m_elements[0] * m_elements[0] + m_elements[1] * m_elements[1] + m_elements[2] * m_elements[2];
}

template<> inline float Vector<3, float>::operator&(const Vector &other) const
{
    return m_elements[0] * other.m_elements[0] + m_elements[1] * other.m_elements[1] +
           m_elements[2] * other.m_elements[2];
}

}

template<unsigned N, typename T>
inline plasma::Vector<N, T> operator*(const plasma::Vector<N, T> &a, const plasma::Vector<N, T> &b)
{
    plasma::Vector<N, T> result;
    result = a;
    for (unsigned i = 0; i < N; i++)
        result.m_elements[i] *= b.m_elements[i];
    return result;
}

// Global template operation on vectors.
template<unsigned N, typename T> inline plasma::Vector<N, T> operator*(T scale, const plasma::Vector<N, T> &vector)
{
    plasma::Vector<N, T> result;
    for (unsigned i = 0; i < N; i++)
        result.m_elements[i] = scale * vector.m_elements[i];
    return result;
}
