// Vector stores N contiguous elements and supports arithmetic for byte, integer, floating-point and Fixed coordinates.
#pragma once
#include <math.h>

namespace plasma {

template<unsigned N, typename T> class Vector {
public:
    Vector() {}
    Vector(T x, T y)
    {
        m_elements[0] = x;
        m_elements[1] = y;
    }
    // Stores each component through the element accessor.
    Vector(T x, T y, T z)
    {
        (*this)[0] = x;
        (*this)[1] = y;
        (*this)[2] = z;
    }
    Vector(T x, T y, T z, T w)
    {
        m_elements[0] = x;
        m_elements[1] = y;
        m_elements[2] = z;
        m_elements[3] = w;
    }

    // Copies components in order through the element accessor.
    Vector(const Vector &other)
    {
        for (int i = 0; i < (int)N; i++)
            (*this)[i] = other[i];
    }

    // Assigns each element through its accessor.
    Vector &operator=(const Vector &other)
    {
        for (int i = 0; i < (int)N; i++)
            (*this)[i] = other[i];
        return *this;
    }

    // Constructs a vector by converting each source element.
    template<typename U> explicit Vector(const Vector<N, U> &other)
    {
        for (int i = 0; i < (int)N; i++)
            (*this)[i] = (T)other[i];
    }

    // Assigns a vector of another element type, converting each element, with a signed index like the
    // converting constructor.
    template<typename U> Vector &operator=(const Vector<N, U> &other)
    {
        for (int i = 0; i < (int)N; i++)
            (*this)[i] = (T)other[i];
        return *this;
    }

    T &operator[](int index) { return m_elements[index]; }
    const T &operator[](int index) const { return m_elements[index]; }

    // Returns a scaled copy by copying and multiplying in place.
    Vector operator*(T scale) const
    {
        Vector result = *this;
        result *= scale;
        return result;
    }

    // Returns the element-wise product.
    Vector operator*(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] *= other.m_elements[i];
        return result;
    }

    // Returns the element-wise sum.
    Vector operator+(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] += other.m_elements[i];
        return result;
    }

    // Scales every element in place.
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

    // Multiplies the corresponding elements in place.
    Vector &operator*=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] *= other.m_elements[i];
        return *this;
    }

    // Adds the corresponding elements in place.
    Vector &operator+=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] += other.m_elements[i];
        return *this;
    }

    Vector &operator-=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] -= other.m_elements[i];
        return *this;
    }

    // Returns a copy multiplied by negative one.
    Vector operator-() const { return *this * (T)-1; }

    // Returns the element-wise difference.
    Vector operator-(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] -= other.m_elements[i];
        return result;
    }

    // Returns the element-wise quotient.
    Vector operator/(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= other.m_elements[i];
        return result;
    }

    // Divides every element by the scalar.
    Vector operator/(T divisor) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= divisor;
        return result;
    }

    // Dot product, accumulated in element order.
    T dot(const Vector &other) const
    {
        T result = m_elements[0] * other.m_elements[0];
        for (unsigned i = 1; i < N; i++)
            result += m_elements[i] * other.m_elements[i];
        return result;
    }
    // Dot product as one expression; defined for Vector<3, float> below.
    T operator&(const Vector &other) const;

    // True when any component differs.
    bool operator!=(const Vector &other) const
    {
        for (int i = 0; i < (int)N; i++) {
            if ((*this)[i] != other[i])
                return true;
        }
        return false;
    }

    // True when all corresponding elements are equal.
    bool operator==(const Vector &other) const
    {
        for (int i = 0; i < (int)N; i++) {
            if ((*this)[i] != other[i])
                return false;
        }
        return true;
    }

    // Dot product with itself.
    T squaredLength() const { return dot(*this); }
    // Euclidean length.
    T length() const { return sqrt(squaredLength()); }
    // Scales to length one in place; zero length is unchecked.
    Vector &normalize()
    {
        T scale = (T)1 / sqrt(squaredLength());
        for (unsigned i = 0; i < N; i++)
            m_elements[i] *= scale;
        return *this;
    }
    // Returns a copy scaled to length one.
    Vector normalized() const
    {
        T scale = (T)1 / length();
        return *this * scale;
    }
    // Cross product, for three elements.
    Vector cross(const Vector &other) const
    {
        return Vector(m_elements[1] * other.m_elements[2] - m_elements[2] * other.m_elements[1],
                      m_elements[2] * other.m_elements[0] - m_elements[0] * other.m_elements[2],
                      m_elements[0] * other.m_elements[1] - m_elements[1] * other.m_elements[0]);
    }

    T m_elements[N];
};

// Float lengths evaluate their squared components as one expression.
template<> inline float Vector<2, float>::squaredLength() const
{
    return m_elements[0] * m_elements[0] + m_elements[1] * m_elements[1];
}

// Squared length of a double vector, adding x squared before y squared.
template<> inline double Vector<2, double>::squaredLength() const
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

// Element-wise product, also available as a global operator.
template<unsigned N, typename T>
inline plasma::Vector<N, T> operator*(const plasma::Vector<N, T> &a, const plasma::Vector<N, T> &b)
{
    plasma::Vector<N, T> result;
    result = a;
    for (unsigned i = 0; i < N; i++)
        result.m_elements[i] *= b.m_elements[i];
    return result;
}

// A scalar multiplied by every element of a vector.
template<unsigned N, typename T> inline plasma::Vector<N, T> operator*(T scale, const plasma::Vector<N, T> &vector)
{
    plasma::Vector<N, T> result;
    for (unsigned i = 0; i < N; i++)
        result.m_elements[i] = scale * vector.m_elements[i];
    return result;
}
