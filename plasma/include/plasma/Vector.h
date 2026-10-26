// A small fixed-size vector, used for two, three and four floats. Constructors and assignment copy components in
// order; arithmetic operates element by element.
#pragma once

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

    // Copies components in order.
    Vector(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = other.m_elements[i];
    }

    Vector &operator=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = other.m_elements[i];
        return *this;
    }

    // Assigns another element type's vector, converting each element (the modules' Build functions set their int
    // cursor from a float position).
    template<typename U> Vector &operator=(const Vector<N, U> &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = (T)other.m_elements[i];
        return *this;
    }

    // Returns an element by index.
    T &operator[](int index) { return m_elements[index]; }
    const T &operator[](int index) const { return m_elements[index]; }

    // Compares positions element by element.
    bool operator!=(const Vector &other) const
    {
        for (int i = 0; i < (int)N; i++) {
            if (m_elements[i] != other.m_elements[i])
                return true;
        }
        return false;
    }

    // Element-wise arithmetic (ContinuousAttribute<Vector<..>>::Interpolate).
    Vector operator+(const Vector &other) const
    {
        Vector result = *this;
        result += other;
        return result;
    }
    // Adds in place (the GLU combine callback of MeshInterface accumulates its weighted attributes with it).
    Vector &operator+=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] += other.m_elements[i];
        return *this;
    }
    // Subtracts in place (ProceduralShapeModule moves the generated vertices by the bounds).
    Vector &operator-=(const Vector &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] -= other.m_elements[i];
        return *this;
    }
    Vector operator-(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] -= other.m_elements[i];
        return result;
    }
    // Element-wise quotient (Workspace: a frame's position and size as fractions of the viewport).
    Vector operator/(const Vector &other) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= other.m_elements[i];
        return result;
    }
    // Copies the vector, then scales it in place.
    Vector operator*(T scale) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] *= scale;
        return result;
    }

    // Divides every element by the scalar (Workspace::Render).
    Vector operator/(T divisor) const
    {
        Vector result = *this;
        for (unsigned i = 0; i < N; i++)
            result.m_elements[i] /= divisor;
        return result;
    }

    // Converts each component to the destination type; float-to-int conversion truncates.
    template<typename U> explicit Vector(const Vector<N, U> &other)
    {
        for (unsigned i = 0; i < N; i++)
            m_elements[i] = (T)other.m_elements[i];
    }

    // Transforms a 2D point in place with perspective division; defined in Matrix.h.
    void transform(const Matrix<T> &matrix);
    // Premultiplies the matrix by the 2D shear held in this vector: x gains [0] * y and y gains [1] * x. Defined in
    // Matrix.h.
    void applyShear(Matrix<T> &matrix) const;

    T m_elements[N];
};

}

// Returns the element-wise product of two vectors.
template<unsigned N, typename T>
inline plasma::Vector<N, T> operator*(const plasma::Vector<N, T> &a, const plasma::Vector<N, T> &b)
{
    plasma::Vector<N, T> result;
    result = a;
    for (unsigned i = 0; i < N; i++)
        result.m_elements[i] *= b.m_elements[i];
    return result;
}
