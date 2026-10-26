// Fixed is a signed 64-bit world coordinate counting 1/65536 of a terrain block. Creature positions and the renderer's
// relative origin use this representation.
#pragma once

namespace cube {

// A Fixed counts 1/65536 of a block: 16 fraction bits.
const int FixedFractionBits = 16;
const int FixedUnitsPerBlock = 1 << FixedFractionBits; // Fixed-point units per terrain block.

struct Fixed {
    Fixed() {}
    Fixed &operator=(const Fixed &other) { return setRaw(other.m_raw); }
    // The raw value (1/65536 blocks). Explicit: an integer converts to a Fixed as blocks (the int constructor below),
    // never silently as raw.
    explicit Fixed(long long raw)
        : m_raw(raw)
    {}
    Fixed(int blocks)
        : Fixed((long long)blocks * FixedUnitsPerBlock)
    {}
    Fixed &operator=(int blocks) { return setRaw((long long)blocks << FixedFractionBits); }
    Fixed(double blocks)
        : Fixed((long long)(blocks * FixedUnitsPerBlock))
    {}
    // The same from a float (the float times 65536.0f, truncated; Zone::generate converts a terrain height with it).
    Fixed(float blocks)
        : Fixed((long long)(blocks * FixedUnitsPerBlock))
    {}
    Fixed &operator=(float blocks) { return setRaw((long long)(blocks * FixedUnitsPerBlock)); }

    Fixed operator-(float blocks) const { return Fixed(raw() - (long long)(blocks * FixedUnitsPerBlock)); }
    Fixed operator+(const Fixed &other) const { return Fixed(other.raw() + raw()); }
    Fixed operator+(float blocks) const { return Fixed((long long)(blocks * FixedUnitsPerBlock) + raw()); }
    Fixed operator+(double blocks) const { return Fixed(raw() - (long long)(-blocks * FixedUnitsPerBlock)); }
    // Below a block coordinate (Zone::generate moves a lore entity south until it leaves the zone).
    bool operator<(int blocks) const { return raw() < (long long)blocks << FixedFractionBits; }
    Fixed operator*(const Fixed &other) const { return Fixed(raw() * other.raw() / FixedUnitsPerBlock); }
    Fixed operator-(int blocks) const { return Fixed(raw() - ((long long)blocks << FixedFractionBits)); }
    Fixed operator+(int blocks) const { return Fixed(raw() + ((long long)blocks << FixedFractionBits)); }
    Fixed operator-(double blocks) const { return Fixed(raw() - (long long)(blocks * FixedUnitsPerBlock)); }

    // The difference of two coordinates, and the compound forms the Vector<3, Fixed> operators expand.
    Fixed operator-(const Fixed &other) const { return Fixed(raw() - other.raw()); }
    Fixed &operator+=(const Fixed &other) { return setRaw(m_raw + other.m_raw); }
    Fixed &operator-=(const Fixed &other) { return setRaw(m_raw - other.m_raw); }
    Fixed &operator*=(const Fixed &other) { return setRaw(m_raw * other.m_raw / FixedUnitsPerBlock); }
    Fixed operator-() const { return Fixed(-raw()); }
    Fixed operator*(float scale) const { return Fixed((long long)((float)raw() * scale)); }
    Fixed operator*(double scale) const { return Fixed((long long)((double)raw() * scale)); }
    Fixed operator/(float divisor) const { return Fixed((long long)((float)raw() / divisor)); }

    bool operator<(const Fixed &other) const { return raw() < other.raw(); }
    bool operator>(const Fixed &other) const { return raw() > other.raw(); }
    bool operator<=(const Fixed &other) const { return raw() <= other.raw(); }
    bool operator>=(const Fixed &other) const { return raw() >= other.raw(); }
    bool operator==(const Fixed &other) const { return raw() == other.raw(); }
    bool operator!=(const Fixed &other) const { return raw() != other.raw(); }

    // Converting Vector constructors use these casts. Integer conversion truncates toward zero.
    explicit operator float() const { return (float)raw() * (1.0f / FixedUnitsPerBlock); }
    explicit operator double() const { return (double)raw() * (1.0 / FixedUnitsPerBlock); }
    explicit operator int() const { return (int)(raw() / FixedUnitsPerBlock); }
    bool operator>=(int blocks) const { return raw() >= (long long)blocks * FixedUnitsPerBlock; }
    float toFloat() const { return (float)raw() * (1.0f / FixedUnitsPerBlock); }
    // Preserves the narrowing for coordinates outside the int block range.
    int toBlock() const
    {
        if (raw() < 0 && raw() != Fixed((int)*this).m_raw)
            return (int)*this - 1;
        return (int)*this;
    }

    // The raw value.
    const long long &raw() const { return m_raw; }

    long long m_raw;

private:
    Fixed &setRaw(long long raw)
    {
        m_raw = raw;
        return *this;
    }
};

// A float minus a coordinate.
inline Fixed operator-(float blocks, const Fixed &value)
{
    return Fixed((long long)(blocks * FixedUnitsPerBlock) - value.m_raw);
}
inline Fixed operator+(int blocks, const Fixed &value)
{
    return Fixed(((long long)blocks << FixedFractionBits) + value.m_raw);
}
// Adds a coordinate to a float in blocks through Fixed.
inline float &operator+=(float &blocks, const Fixed &value)
{
    blocks = (float)(Fixed(blocks) + value);
    return blocks;
}

}
