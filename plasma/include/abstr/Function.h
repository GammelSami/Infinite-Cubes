// abstr::Function: a function a compiled script calls by name. The machine owns registered functions
// (AddNamedMethod deletes the one it replaces with plain operator delete: the class has no virtual destructor).
// Call pops the arguments from the machine's stack, calls the native target and pushes the result (void results
// push the int 0).
#pragma once

namespace abstr {

class Machine;

class Function {
public:
    Function()
        : m_keyWords(0)
    {}
#ifdef PLASMA_PORTABLE
    virtual ~Function() {}
#endif

// The function binders key the machine with the first words of their target's code (Function0/Function1,
// abstr/Method.h).
#ifdef PLASMA_NO_LICENSING
    explicit Function(const void *)
        : m_keyWords(0)
    {}
#else
    explicit Function(const void *code)
        : m_keyWords(static_cast<const int *>(code))
    {}
#endif
    virtual void call(Machine *machine) = 0;

    // Four words AddNamedMethod adds to the machine's key when the function is registered. Method binders leave it
    // null; Function0/Function1 point it at their target's code (abstr/Method.h).
    const int *m_keyWords;
};

} // Namespace abstr.
