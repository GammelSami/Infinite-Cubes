#pragma once

namespace abstr {
class Machine;

// Script-call binding. There is no virtual destructor.
class Function {
public:
    Function()
        : m_keyWords(0)
    {}
    explicit Function(const void *code)
        : m_keyWords(static_cast<const int *>(code))
    {}
    virtual void call(Machine *machine) = 0;
    const int *m_keyWords; // four code words added when registering a function
};
}
