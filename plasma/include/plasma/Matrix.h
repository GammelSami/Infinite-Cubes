// The engine's 4x4 matrix. Transform uses indices 0/4/12 for x, 1/5/13 for y and 3/7/15 for perspective w.
// TransformDirection uses the same linear indices; GL consumes the array as column-major data. operator*= uses row
// * 4 + column indexing.
#pragma once
#include <math.h>
#include "plasma/Vector.h"

namespace plasma {

template<typename T> class Matrix {
public:
    // Leaves the elements uninitialised (the Transformation constructor passes such a matrix as the deformation
    // attribute's initial value).
    Matrix() {}
    // Copies all 16 elements, such as a matrix read back from GL.
    explicit Matrix(const T *elements);

    void identity()
    {
        m_elements[0] = 1;
        m_elements[1] = 0;
        m_elements[2] = 0;
        m_elements[3] = 0;
        m_elements[4] = 0;
        m_elements[5] = 1;
        m_elements[6] = 0;
        m_elements[7] = 0;
        m_elements[8] = 0;
        m_elements[9] = 0;
        m_elements[10] = 1;
        m_elements[11] = 0;
        m_elements[12] = 0;
        m_elements[13] = 0;
        m_elements[14] = 0;
        m_elements[15] = 1;
    }

    // Element-wise arithmetic used for animated matrix interpolation. Binary operators copy, then apply the compound
    // operation.
    Matrix &operator+=(const Matrix &other);
    Matrix &operator-=(const Matrix &other);
    Matrix &operator*=(T scale);
    Matrix operator+(const Matrix &other) const
    {
        Matrix result = *this;
        result += other;
        return result;
    }
    Matrix operator-(const Matrix &other) const
    {
        Matrix result = *this;
        result -= other;
        return result;
    }
    Matrix operator*(T scale) const
    {
        Matrix result = *this;
        result *= scale;
        return result;
    }

    // Returns one matrix element by value.
    T operator[](int index) const { return m_elements[index]; }

    // Transforms a point with perspective division: computes w and the two coordinates, then scales them by 1 / w.
    Vector<2, T> transform(const Vector<2, T> &point) const
    {
        T w = (*this)[3] * point[0] + (*this)[7] * point[1] + (*this)[15];
        return Vector<2, T>((*this)[0] * point[0] + (*this)[4] * point[1] + (*this)[12],
                            (*this)[1] * point[0] + (*this)[5] * point[1] + (*this)[13]) *
               (1 / w);
    }

    // Transforms a point with perspective division, using the same arithmetic as Transform.
    Vector<2, T> operator*(const Vector<2, T> &point) const
    {
        const Matrix &m = *this;
        T w = m[3] * point.m_elements[0] + m[7] * point.m_elements[1] + m[15];
        Vector<2, T> result;
        result = Vector<2, T>(m[4] * point.m_elements[1] + point.m_elements[0] * m[0] + m[12],
                              m[1] * point.m_elements[0] + m[5] * point.m_elements[1] + m[13]);
        T scale = 1 / w;
        result.m_elements[0] = scale * result.m_elements[0];
        result.m_elements[1] = scale * result.m_elements[1];
        return result;
    }

    // Transforms a point with perspective division, scaling the resulting coordinates by 1 / w.
    Vector<2, T> transformPoint(const Vector<2, T> &point) const
    {
        const Matrix &m = *this;
        T w = m[3] * point.m_elements[0] + m[7] * point.m_elements[1] + m[15];
        return Vector<2, T>(m[4] * point.m_elements[1] + point.m_elements[0] * m[0] + m[12],
                            m[1] * point.m_elements[0] + m[5] * point.m_elements[1] + m[13]) *
               (1 / w);
    }

    // A direction (no translation, no division).
    Vector<2, T> transformDirection(const Vector<2, T> &direction) const
    {
        return Vector<2, T>(m_elements[0] * direction.m_elements[0] + m_elements[4] * direction.m_elements[1],
                            m_elements[1] * direction.m_elements[0] + m_elements[5] * direction.m_elements[1]);
    }

    // Transforms a direction in place, without translation or perspective division.
    Vector<2, T> &transformDirectionInPlace(Vector<2, T> &direction) const;

    // Inverts the matrix in place. Returns false and leaves it unchanged if singular.
    bool invert();
    // Scales rows 0 and 1 by x and y, skipping factors of 1.
    void scale(float x, float y);
    // Scales rows 0, 1 and 2 by x, y and z, skipping factors of 1.
    void scale(float x, float y, float z);
    // Post-translates the matrix by (x, y) in place.
    void translate(T x, T y)
    {
        m_elements[12] += m_elements[0] * x + m_elements[4] * y;
        m_elements[13] += m_elements[1] * x + m_elements[5] * y;
        m_elements[14] += m_elements[2] * x + m_elements[6] * y;
        m_elements[15] += m_elements[3] * x + m_elements[7] * y;
    }
    // Translates by the vector's x and y components.
    void translate(const Vector<2, T> &offset) { translate(offset.m_elements[0], offset.m_elements[1]); }

    // Premultiplies this matrix by other, using row * 4 + column indexing, and returns *this.
    Matrix &operator*=(const Matrix &other);
    // Returns a copy premultiplied by other.
    Matrix operator*(const Matrix &other) const
    {
        Matrix result = *this;
        result *= other;
        return result;
    }

    // Rotates rows 1 and 2, 0 and 2, or 0 and 1 by an angle in degrees for rotation about x, y, or z.
    void rotateX(T degrees);
    void rotateY(T degrees);
    void rotateZ(T degrees);

    T m_elements[16];
};

// Transforms a point in place using Matrix::Transform's arithmetic.
template<unsigned N, typename T> void Vector<N, T>::transform(const Matrix<T> &matrix)
{
    const T *m = matrix.m_elements;
    const Vector<N, T> &point = *this;
    T w = m[3] * point.m_elements[0] + m[7] * point.m_elements[1] + m[15];
    *this = Vector<2, T>(m[4] * point.m_elements[1] + point.m_elements[0] * m[0] + m[12],
                         m[1] * point.m_elements[0] + m[5] * point.m_elements[1] + m[13]) *
            (1 / w);
}

template<typename T> Vector<2, T> &Matrix<T>::transformDirectionInPlace(Vector<2, T> &direction) const
{
    Vector<2, T> transformed;
    transformed.m_elements[0] = m_elements[4] * direction.m_elements[1] + m_elements[0] * direction.m_elements[0];
    transformed.m_elements[1] = m_elements[5] * direction.m_elements[1] + m_elements[1] * direction.m_elements[0];
    direction = transformed;
    return direction;
}

}

namespace plasma {

template<typename T> Matrix<T>::Matrix(const T *elements)
{
    for (int i = 0; i < 16; i++)
        m_elements[i] = elements[i];
}

template<typename T> Matrix<T> &Matrix<T>::operator+=(const Matrix &other)
{
    for (int i = 0; i < 16; i++)
        m_elements[i] += other.m_elements[i];
    return *this;
}

template<typename T> Matrix<T> &Matrix<T>::operator-=(const Matrix &other)
{
    for (int i = 0; i < 16; i++)
        m_elements[i] -= other.m_elements[i];
    return *this;
}

template<typename T> Matrix<T> &Matrix<T>::operator*=(T scale)
{
    for (int i = 0; i < 16; i++)
        m_elements[i] *= scale;
    return *this;
}

template<typename T> void Matrix<T>::scale(float x, float y)
{
    if (x != 1.0f) {
        m_elements[0] *= x;
        m_elements[1] *= x;
        m_elements[2] *= x;
        m_elements[3] *= x;
    }
    if (y != 1.0f) {
        for (int i = 4; i < 8; ++i)
            m_elements[i] *= y;
    }
}

template<typename T> void Matrix<T>::rotateX(T degrees)
{
    T radians = degrees * (double)0.017453292f;
    T cosine = cos(radians);
    T sine = sin(radians);
    for (int column = 0; column < 4; column++) {
        T y = m_elements[4 + column];
        m_elements[4 + column] = cosine * y + sine * m_elements[8 + column];
        m_elements[8 + column] = cosine * m_elements[8 + column] - sine * y;
    }
}

template<typename T> void Matrix<T>::rotateY(T degrees)
{
    T radians = degrees * (double)0.017453292f;
    T cosine = cos(radians);
    T sine = sin(radians);
    for (int column = 0; column < 4; column++) {
        T x = m_elements[column];
        m_elements[column] = cosine * x - sine * m_elements[8 + column];
        m_elements[8 + column] = sine * x + cosine * m_elements[8 + column];
    }
}

template<typename T> void Matrix<T>::rotateZ(T degrees)
{
    T radians = degrees * (double)0.017453292f;
    T cosine = cos(radians);
    T sine = sin(radians);
    for (int column = 0; column < 4; column++) {
        T x = m_elements[column];
        m_elements[column] = cosine * x + sine * m_elements[4 + column];
        m_elements[4 + column] = cosine * m_elements[4 + column] - sine * x;
    }
}

template<unsigned N, typename T> void Vector<N, T>::applyShear(Matrix<T> &matrix) const
{
    Matrix<T> shear;
    shear.identity();
    shear.m_elements[4] = m_elements[0];
    shear.m_elements[1] = m_elements[1];
    matrix *= shear;
}

template<typename T> Matrix<T> &Matrix<T>::operator*=(const Matrix &other)
{
    for (int column = 0; column < 4; column++) {
        T a0 = m_elements[column];
        T a1 = m_elements[4 + column];
        T a2 = m_elements[8 + column];
        T a3 = m_elements[12 + column];
        m_elements[column] =
            other.m_elements[0] * a0 + a1 * other.m_elements[1] + other.m_elements[2] * a2 + other.m_elements[3] * a3;
        m_elements[4 + column] =
            other.m_elements[5] * a1 + other.m_elements[4] * a0 + a2 * other.m_elements[6] + other.m_elements[7] * a3;
        m_elements[8 + column] =
            other.m_elements[9] * a1 + other.m_elements[8] * a0 + a2 * other.m_elements[10] + other.m_elements[11] * a3;
        m_elements[12 + column] = other.m_elements[13] * a1 + other.m_elements[12] * a0 + a2 * other.m_elements[14] +
                                  other.m_elements[15] * a3;
    }
    return *this;
}

// Gauss-Jordan elimination with partial pivoting on the columns, each row extended by the identity (the rows are
// swapped by pointer).
template<typename T> bool Matrix<T>::invert()
{
    T rows[4][8];
    T *r0 = rows[0], *r1 = rows[1], *r2 = rows[2], *r3 = rows[3];
    T *swap;
    T m0, m1, m2, m3, s;

    r0[0] = m_elements[0], r0[1] = m_elements[4], r0[2] = m_elements[8], r0[3] = m_elements[12];
    r0[4] = 1, r0[5] = r0[6] = r0[7] = 0;
    r1[0] = m_elements[1], r1[1] = m_elements[5], r1[2] = m_elements[9], r1[3] = m_elements[13];
    r1[5] = 1, r1[4] = r1[6] = r1[7] = 0;
    r2[0] = m_elements[2], r2[1] = m_elements[6], r2[2] = m_elements[10], r2[3] = m_elements[14];
    r2[6] = 1, r2[4] = r2[5] = r2[7] = 0;
    r3[0] = m_elements[3], r3[1] = m_elements[7], r3[2] = m_elements[11], r3[3] = m_elements[15];
    r3[7] = 1, r3[4] = r3[5] = r3[6] = 0;

    // The first column: the largest pivot to the top, or singular.
    if ((T)fabs((double)r3[0]) > (T)fabs((double)r2[0])) {
        swap = r3;
        r3 = r2;
        r2 = swap;
    }
    if ((T)fabs((double)r2[0]) > (T)fabs((double)r1[0])) {
        swap = r2;
        r2 = r1;
        r1 = swap;
    }
    if ((T)fabs((double)r1[0]) > (T)fabs((double)r0[0])) {
        swap = r1;
        r1 = r0;
        r0 = swap;
    }
    if (0 == r0[0])
        return false;
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    // The second column.
    if ((T)fabs((double)r3[1]) > (T)fabs((double)r2[1])) {
        swap = r3;
        r3 = r2;
        r2 = swap;
    }
    if ((T)fabs((double)r2[1]) > (T)fabs((double)r1[1])) {
        swap = r2;
        r2 = r1;
        r1 = swap;
    }
    if (0 == r1[1])
        return false;
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0 != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0 != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0 != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0 != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    // The third column.
    if ((T)fabs((double)r3[2]) > (T)fabs((double)r2[2])) {
        swap = r3;
        r3 = r2;
        r2 = swap;
    }
    if (0 == r2[2])
        return false;
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4], r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    if (0 == r3[3])
        return false;

    // Back substitution, from the last row up.
    s = 1 / r3[3];
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    m2 = r2[3];
    s = 1 / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1, r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0, r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

    m1 = r1[2];
    s = 1 / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0, r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

    m0 = r0[1];
    s = 1 / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

    m_elements[0] = r0[4], m_elements[4] = r0[5], m_elements[8] = r0[6], m_elements[12] = r0[7];
    m_elements[1] = r1[4], m_elements[5] = r1[5], m_elements[9] = r1[6], m_elements[13] = r1[7];
    m_elements[2] = r2[4], m_elements[6] = r2[5], m_elements[10] = r2[6], m_elements[14] = r2[7];
    m_elements[3] = r3[4], m_elements[7] = r3[5], m_elements[11] = r3[6], m_elements[15] = r3[7];
    return true;
}

}
