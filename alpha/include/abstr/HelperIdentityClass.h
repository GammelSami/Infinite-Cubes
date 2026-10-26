// Script jump helper returning one while retaining its operand.
#pragma once
namespace abstr {
class HelperIdentityClass {
public:
    HelperIdentityClass(int value)
        : m_one(1),
          m_value(value)
    {}
    virtual int get() { return m_one; }
    int m_one;   // fixed result, initialized to one
    int m_value; // jump operand
};
}
