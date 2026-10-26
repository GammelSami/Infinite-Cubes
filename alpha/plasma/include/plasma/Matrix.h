// Matrix supplies transforms, projection, multiplication and inversion.
#pragma once
#include <math.h>
#include "plasma/Vector.h"

namespace plasma {

template<typename T> class Matrix {
public:
    Matrix() {}
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
    T &operator[](int index) { return m_elements[index]; }
    const T &operator[](int index) const { return m_elements[index]; }
    T &operator()(int row, int column) { return (*this)[(column << 2) + row]; }
    const T &operator()(int row, int column) const { return m_elements[(column << 2) + row]; }
    Vector<2, T> transform(const Vector<2, T> &point) const
    {
        T w = (*this)[3] * point[0] + (*this)[7] * point[1] + (*this)[15];
        return Vector<2, T>((*this)[0] * point[0] + (*this)[4] * point[1] + (*this)[12],
                            (*this)[1] * point[0] + (*this)[5] * point[1] + (*this)[13]) *
               (1 / w);
    }
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
    Vector<2, T> transformDirection(const Vector<2, T> &direction) const
    {
        return Vector<2, T>(m_elements[0] * direction.m_elements[0] + m_elements[4] * direction.m_elements[1],
                            m_elements[1] * direction.m_elements[0] + m_elements[5] * direction.m_elements[1]);
    }
    Vector<2, T> &transformDirectionInPlace(Vector<2, T> &direction) const;
    // A direction in three dimensions (no translation, no division); the release's form.
    Vector<3, T> transformDirection(const Vector<3, T> &direction) const
    {
        return Vector<3, T>((*this)(0, 1) * direction[1] + (*this)(0, 0) * direction[0] + (*this)(0, 2) * direction[2],
                            (*this)(1, 0) * direction[0] + (*this)(1, 1) * direction[1] + (*this)(1, 2) * direction[2],
                            (*this)(2, 0) * direction[0] + (*this)(2, 1) * direction[1] + (*this)(2, 2) * direction[2]);
    }
    // A point in three dimensions with the perspective division (the release's form; moveCharacter's
    // lock-on projects creatures with it).
    Vector<3, T> operator*(const Vector<3, T> &point) const
    {
        T w = (*this)(3, 0) * point[0] + (*this)(3, 1) * point[1] + (*this)(3, 2) * point[2] + (*this)(3, 3);
        Vector<3, T> result;
        result = Vector<3, T>(
            (*this)(0, 0) * point[0] + (*this)(0, 1) * point[1] + (*this)(0, 2) * point[2] + (*this)(0, 3),
            (*this)(1, 0) * point[0] + (*this)(1, 1) * point[1] + (*this)(1, 2) * point[2] + (*this)(1, 3),
            (*this)(2, 0) * point[0] + (*this)(2, 1) * point[1] + (*this)(2, 2) * point[2] + (*this)(2, 3));
        T scale = 1 / w;
        result.m_elements[0] = scale * result.m_elements[0];
        result.m_elements[1] = scale * result.m_elements[1];
        result.m_elements[2] = scale * result.m_elements[2];
        return result;
    }
    void transpose()
    {
        Matrix original = *this;
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                (*this)(row, column) = original(column, row);
    }
    // Projection matrix construction.
    void setOrthographic(T left, T right, T bottom, T top, T zNear, T zFar)
    {
        (*this)(0, 0) = 2 / (right - left);
        (*this)(1, 0) = 0;
        (*this)(2, 0) = 0;
        (*this)(3, 0) = 0;
        (*this)(0, 1) = 0;
        (*this)(1, 1) = 2 / (top - bottom);
        (*this)(2, 1) = 0;
        (*this)(3, 1) = 0;
        (*this)(0, 2) = 0;
        (*this)(1, 2) = 0;
        (*this)(2, 2) = 1 / (zNear - zFar);
        (*this)(3, 2) = 0;
        (*this)(0, 3) = (left + right) / (left - right);
        (*this)(1, 3) = (bottom + top) / (bottom - top);
        (*this)(2, 3) = zNear / (zNear - zFar);
        (*this)(3, 3) = 1;
    }
    // Perspective projection from vertical field of view, aspect ratio and clipping planes.
    void setPerspective(T fieldOfView, T aspect, T zNear, T zFar)
    {
        T cotangent = 1 / tan(fieldOfView * 0.5f);
        (*this)(0, 0) = cotangent / aspect;
        (*this)(0, 1) = 0;
        (*this)(0, 2) = 0;
        (*this)(0, 3) = 0;
        (*this)(1, 0) = 0;
        (*this)(1, 1) = cotangent;
        (*this)(1, 2) = 0;
        (*this)(1, 3) = 0;
        (*this)(2, 0) = 0;
        (*this)(2, 1) = 0;
        (*this)(2, 2) = zFar / (zFar - zNear);
        (*this)(2, 3) = -zNear * zFar / (zFar - zNear);
        (*this)(3, 0) = 0;
        (*this)(3, 1) = 0;
        (*this)(3, 2) = 1;
        (*this)(3, 3) = 0;
    }
    // Applies translation row by row.
    void translate(T x, T y, T z)
    {
        (*this)[12] += (*this)[0] * x + (*this)[4] * y + (*this)[8] * z;
        (*this)[13] += (*this)[1] * x + (*this)[5] * y + (*this)[9] * z;
        (*this)[14] += (*this)[2] * x + (*this)[6] * y + (*this)[10] * z;
        (*this)[15] += (*this)[3] * x + (*this)[7] * y + (*this)[11] * z;
    }
    // Translation by a vector offset.
    void translate(const Vector<3, T> &offset) { translate(offset[0], offset[1], offset[2]); }
    bool invert();
    void scale(float x, float y);
    void scale(float x, float y, float z);
    void translate(T x, T y)
    {
        for (int row = 0; row < 4; ++row)
            (*this)(row, 3) += (*this)(row, 0) * x + (*this)(row, 1) * y;
    }
    void translate(const Vector<2, T> &offset) { translate(offset.m_elements[0], offset.m_elements[1]); }
    Matrix &operator*=(const Matrix &other);
    Matrix operator*(const Matrix &other) const
    {
        Matrix result = *this;
        result *= other;
        return result;
    }
    void rotateX(T degrees);
    void rotateY(T degrees);
    void rotateZ(T degrees);
    // Transforms a point in place with the perspective division.
    void transform(Vector<3, T> &point) const
    {
        T w = (*this)(3, 0) * point[0] + (*this)(3, 1) * point[1] + (*this)(3, 2) * point[2] + (*this)(3, 3);
        point = Vector<3, T>(
                    (*this)(0, 0) * point[0] + (*this)(0, 1) * point[1] + (*this)(0, 2) * point[2] + (*this)(0, 3),
                    (*this)(1, 0) * point[0] + (*this)(1, 1) * point[1] + (*this)(1, 2) * point[2] + (*this)(1, 3),
                    (*this)(2, 0) * point[0] + (*this)(2, 1) * point[1] + (*this)(2, 2) * point[2] + (*this)(2, 3)) *
                (1 / w);
    }
    // Rotates by the angle in degrees around the axis (x, y, z), which need not be normalised.
    void rotate(T degrees, T x, T y, T z);
    // The same with the axis as a vector.
    void rotate(T degrees, const Vector<3, T> &axis) { rotate(degrees, axis[0], axis[1], axis[2]); }
    // Rotates by the angle between two directions about their cross product; nothing when either is
    // (nearly) zero or they are parallel. GameController::render turns its ray model with it. The
    // release has the same body as the renderer's free function rotateTowards.
    void rotateTowards(const Vector<3, T> &from, const Vector<3, T> &to)
    {
        T fromLength = sqrt(from[0] * from[0] + from[1] * from[1] + from[2] * from[2]);
        // Nothing for a direction shorter than 0.0001, or for directions whose cross product's squared
        // length is below 0.0001.
        if (fabs(fromLength) < 0.0001)
            return;
        T toLength = sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
        if (fabs(toLength) < 0.0001)
            return;
        Vector<3, T> axis(to[2] * from[1] - to[1] * from[2], to[0] * from[2] - to[2] * from[0],
                          to[1] * from[0] - to[0] * from[1]);
        if (fabs(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]) < 0.0001)
            return;
        T angle = acos((to[0] * from[0] + to[1] * from[1] + to[2] * from[2]) / (toLength * fromLength));
        rotate(angle * 57.29578f, axis[0], axis[1], axis[2]);
    }

    T m_elements[16];
};

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

// Scales the first two matrix columns.
template<typename T> void Matrix<T>::scale(float x, float y)
{
    if (x != 1.0f) {
        (*this)[0] *= x;
        (*this)[1] *= x;
        (*this)[2] *= x;
        (*this)[3] *= x;
    }
    if (y != 1.0f) {
        (*this)[4] *= y;
        (*this)[5] *= y;
        (*this)[6] *= y;
        (*this)[7] *= y;
    }
}

// Scales the first three columns, skipping unit factors.
template<typename T> void Matrix<T>::scale(float x, float y, float z)
{
    if (x != 1.0f) {
        (*this)[0] *= x;
        (*this)[1] *= x;
        (*this)[2] *= x;
        (*this)[3] *= x;
    }
    if (y != 1.0f) {
        (*this)[4] *= y;
        (*this)[5] *= y;
        (*this)[6] *= y;
        (*this)[7] *= y;
    }
    if (z != 1.0f) {
        (*this)[8] *= z;
        (*this)[9] *= z;
        (*this)[10] *= z;
        (*this)[11] *= z;
    }
}

template<typename T> Matrix<T> &Matrix<T>::operator*=(const Matrix &other)
{
    for (int i = 0; i < 4; i++) {
        const T a0 = (*this)(i, 0), a1 = (*this)(i, 1), a2 = (*this)(i, 2), a3 = (*this)(i, 3);
        (*this)(i, 0) = a0 * other(0, 0) + a1 * other(1, 0) + a2 * other(2, 0) + a3 * other(3, 0);
        (*this)(i, 1) = a0 * other(0, 1) + a1 * other(1, 1) + a2 * other(2, 1) + a3 * other(3, 1);
        (*this)(i, 2) = a0 * other(0, 2) + a1 * other(1, 2) + a2 * other(2, 2) + a3 * other(3, 2);
        (*this)(i, 3) = a0 * other(0, 3) + a1 * other(1, 3) + a2 * other(2, 3) + a3 * other(3, 3);
    }
    return *this;
}

// GlRotatef's matrix (Mesa's form, : the axis divided by its length, then x * x * (1 - c) + c, x *
// y * (1 - c) - z * s,...), multiplied in with operator*=.
template<typename T> void Matrix<T>::rotate(T degrees, T x, T y, T z)
{
    T length = sqrt(x * x + y * y + z * z);
    if (length == 0)
        return;
    T radians = degrees * 0.017453292f;
    T cosine = cos(radians);
    T sine = sin(radians);
    x /= length;
    y /= length;
    z /= length;
    T xx = x * x;
    T yy = y * y;
    T zz = z * z;
    T xy = x * y;
    T yz = y * z;
    T zx = x * z;
    T xs = sine * x;
    T ys = sine * y;
    T zs = sine * z;
    T t = 1 - cosine;
    Matrix rotation;
    rotation[0] = xx * t + cosine;
    rotation[4] = xy * t - zs;
    rotation[8] = zx * t + ys;
    rotation[12] = 0;
    rotation[1] = xy * t + zs;
    rotation[5] = yy * t + cosine;
    rotation[9] = yz * t - xs;
    rotation[13] = 0;
    rotation[2] = zx * t - ys;
    rotation[6] = yz * t + xs;
    rotation[10] = zz * t + cosine;
    rotation[14] = 0;
    rotation[3] = 0;
    rotation[7] = 0;
    rotation[11] = 0;
    rotation[15] = 1;
    *this *= rotation;
}

// Rotates matrix rows about the corresponding axis.
template<typename T> void Matrix<T>::rotateX(T degrees)
{
    T cosine = cos(degrees * 0.017453292f);
    T sine = sin(degrees * 0.017453292f);
    {
        T t = (*this)(0, 1);
        T u = cosine * t;
        T w = sine * (*this)(0, 2);
        (*this)(0, 1) = u + w;
        (*this)(0, 2) = cosine * (*this)(0, 2) - sine * t;
    }
    {
        T t = (*this)(1, 1);
        T u = cosine * t;
        T w = sine * (*this)(1, 2);
        (*this)(1, 1) = u + w;
        (*this)(1, 2) = cosine * (*this)(1, 2) - sine * t;
    }
    {
        T t = (*this)(2, 1);
        T u = cosine * t;
        T w = sine * (*this)(2, 2);
        (*this)(2, 1) = u + w;
        (*this)(2, 2) = cosine * (*this)(2, 2) - sine * t;
    }
    {
        T t = (*this)(3, 1);
        T u = cosine * t;
        T w = sine * (*this)(3, 2);
        (*this)(3, 1) = u + w;
        (*this)(3, 2) = cosine * (*this)(3, 2) - sine * t;
    }
}

template<typename T> void Matrix<T>::rotateY(T degrees)
{
    T cosine = cos(degrees * 0.017453292f);
    T sine = sin(degrees * 0.017453292f);
    {
        T t = (*this)(0, 0);
        T u = cosine * t;
        T w = sine * (*this)(0, 2);
        (*this)(0, 0) = u - w;
        (*this)(0, 2) = cosine * (*this)(0, 2) + sine * t;
    }
    {
        T t = (*this)(1, 0);
        T u = cosine * t;
        T w = sine * (*this)(1, 2);
        (*this)(1, 0) = u - w;
        (*this)(1, 2) = cosine * (*this)(1, 2) + sine * t;
    }
    {
        T t = (*this)(2, 0);
        T u = cosine * t;
        T w = sine * (*this)(2, 2);
        (*this)(2, 0) = u - w;
        (*this)(2, 2) = cosine * (*this)(2, 2) + sine * t;
    }
    {
        T t = (*this)(3, 0);
        T u = cosine * t;
        T w = sine * (*this)(3, 2);
        (*this)(3, 0) = u - w;
        (*this)(3, 2) = cosine * (*this)(3, 2) + sine * t;
    }
}

template<typename T> void Matrix<T>::rotateZ(T degrees)
{
    T cosine = cos(degrees * 0.017453292f);
    T sine = sin(degrees * 0.017453292f);
    {
        T t = (*this)(0, 0);
        T u = cosine * t;
        T w = sine * (*this)(0, 1);
        (*this)(0, 0) = u + w;
        (*this)(0, 1) = cosine * (*this)(0, 1) - sine * t;
    }
    {
        T t = (*this)(1, 0);
        T u = cosine * t;
        T w = sine * (*this)(1, 1);
        (*this)(1, 0) = u + w;
        (*this)(1, 1) = cosine * (*this)(1, 1) - sine * t;
    }
    {
        T t = (*this)(2, 0);
        T u = cosine * t;
        T w = sine * (*this)(2, 1);
        (*this)(2, 0) = u + w;
        (*this)(2, 1) = cosine * (*this)(2, 1) - sine * t;
    }
    {
        T t = (*this)(3, 0);
        T u = cosine * t;
        T w = sine * (*this)(3, 1);
        (*this)(3, 0) = u + w;
        (*this)(3, 1) = cosine * (*this)(3, 1) - sine * t;
    }
}

template<typename T> bool Matrix<T>::invert()
{
    T rows[4][8];
    T m0, m1, m2, m3, s;
    T *r0, *r1, *r2, *r3;
    T *swap;

    r0 = rows[0], r1 = rows[1], r2 = rows[2], r3 = rows[3];

    r0[0] = (*this)[0], r0[1] = (*this)[4], r0[2] = (*this)[8], r0[3] = (*this)[12];
    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0;
    r1[0] = (*this)[1], r1[1] = (*this)[5], r1[2] = (*this)[9], r1[3] = (*this)[13];
    r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0;
    r2[0] = (*this)[2], r2[1] = (*this)[6], r2[2] = (*this)[10], r2[3] = (*this)[14];
    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0;
    r3[0] = (*this)[3], r3[1] = (*this)[7], r3[2] = (*this)[11], r3[3] = (*this)[15];
    r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
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
    if (0.0 == r0[0])
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
    if (s != 0.0) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.0) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.0) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.0) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
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
    if (0.0 == r1[1])
        return false;
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0.0 != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0.0 != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0.0 != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0.0 != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }
    if ((T)fabs((double)r3[2]) > (T)fabs((double)r2[2])) {
        swap = r3;
        r3 = r2;
        r2 = swap;
    }
    if (0.0 == r2[2])
        return false;
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4], r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
    if (0.0 == r3[3])
        return false;
    s = 1.0f / r3[3];
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    m2 = r2[3];
    s = 1.0f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1, r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0, r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

    m1 = r1[2];
    s = 1.0f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0, r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

    m0 = r0[1];
    s = 1.0f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

    (*this)[0] = r0[4], (*this)[4] = r0[5], (*this)[8] = r0[6], (*this)[12] = r0[7];
    (*this)[1] = r1[4], (*this)[5] = r1[5], (*this)[9] = r1[6], (*this)[13] = r1[7];
    (*this)[2] = r2[4], (*this)[6] = r2[5], (*this)[10] = r2[6], (*this)[14] = r2[7];
    (*this)[3] = r3[4], (*this)[7] = r3[5], (*this)[11] = r3[6], (*this)[15] = r3[7];
    return true;
}

}
