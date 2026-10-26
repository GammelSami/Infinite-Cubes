// Script comparison helper returning one at or above the threshold.
#pragma once
namespace abstr {
class HelperCmpClass {
public:
    HelperCmpClass(int value)
        : m_one(1),
          m_value(value)
    {}

    virtual int below(int value)
    {
        if (m_value > value)
            return 0;
        return m_one;
    }
    int m_one;   // comparison result
    int m_value; // threshold
};
}
