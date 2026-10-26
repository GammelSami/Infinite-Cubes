// MethodN binds an object's member function; FunctionN binds a free function. Call pops arguments with the last
// argument on top, invokes the target, then pushes an int or float as four bytes, a string as its characters, or
// zero for a void result. The binders serve WindowsSystemAdapter, Workspace, OnlineModule and Machine.
#pragma once
#include <string>

#include "abstr/Function.h"
#include "abstr/Machine.h"

namespace abstr {

// A string argument is its characters.
template<> inline std::string Machine::pop<std::string>()
{
    const std::vector<char> bytes = popValue();
    return std::string(bytes.begin(), bytes.end());
}

// Result<R>::Call pops a binder's arguments through Invoke, calls the target and pushes the result.
template<class R> struct Result {
    template<class F> static void __stdcall call(Machine *machine, F *function)
    {
        push(machine, function->invoke(machine));
    }
    static void push(Machine *machine, R value) { machine->push(value); }
};

// Pushes a returned string as a temporary.
template<> struct Result<std::string> {
    template<class F> static void __stdcall call(Machine *machine, F *function)
    {
        push(machine, function->invoke(machine));
    }
    static void push(Machine *machine, const std::string &value)
    {
        machine->pushBytesToStack(value.c_str(), value.size());
    }
};
template<> struct Result<void> {
    template<class F> static void __stdcall call(Machine *machine, F *function)
    {
        function->invoke(machine);
        machine->push(0);
    }
};

template<class C, class R> class Method0 : public Function {
public:
    typedef R (C::*Target)();
    Method0(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }
    R invoke(Machine *machine) { return (m_object->*m_method)(); }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1> class Method1 : public Function {
public:
    typedef R (C::*Target)(A1);
    Method1(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }
    R invoke(Machine *machine)
    {
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1);
    }

    C *m_object;
    Target m_method;
};

template<class R> class Function0 : public Function {
public:
    typedef R (*Target)();
    // The machine's key takes the first words of the function's code (Workspace::Init).
    explicit Function0(Target function)
        : Function(reinterpret_cast<const void *>(function)),
          m_function(function)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }
    R invoke(Machine *machine) { return m_function(); }

    Target m_function;
};

template<class R, class A1> class Function1 : public Function {
public:
    typedef R (*Target)(A1);
    explicit Function1(Target function)
        : Function(reinterpret_cast<const void *>(function)),
          m_function(function)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }
    R invoke(Machine *machine)
    {
        A1 a1 = machine->pop<A1>();
        return m_function(a1);
    }

    Target m_function;
};

// Two and more arguments (Method4 and Method7 are not instantiated).
template<class C, class R, class A1, class A2> class Method2 : public Function {
public:
    typedef R (C::*Target)(A1, A2);
    Method2(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }
    R invoke(Machine *machine)
    {
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2);
    }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2, class A3> class Method3 : public Function {
public:
    typedef R (C::*Target)(A1, A2, A3);
    Method3(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }

    // Pops the arguments and calls the target.
    R invoke(Machine *machine)
    {
        A3 a3 = machine->pop<A3>();
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2, a3);
    }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2, class A3, class A4, class A5> class Method5 : public Function {
public:
    typedef R (C::*Target)(A1, A2, A3, A4, A5);
    Method5(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }

    // Pops the arguments and calls the target.
    R invoke(Machine *machine)
    {
        A5 a5 = machine->pop<A5>();
        A4 a4 = machine->pop<A4>();
        A3 a3 = machine->pop<A3>();
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2, a3, a4, a5);
    }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6> class Method6 : public Function {
public:
    typedef R (C::*Target)(A1, A2, A3, A4, A5, A6);
    Method6(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }

    // Pops the arguments and calls the target.
    R invoke(Machine *machine)
    {
        A6 a6 = machine->pop<A6>();
        A5 a5 = machine->pop<A5>();
        A4 a4 = machine->pop<A4>();
        A3 a3 = machine->pop<A3>();
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2, a3, a4, a5, a6);
    }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
class Method8 : public Function {
public:
    typedef R (C::*Target)(A1, A2, A3, A4, A5, A6, A7, A8);
    Method8(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }

    // Pops the arguments and calls the target.
    R invoke(Machine *machine)
    {
        A8 a8 = machine->pop<A8>();
        A7 a7 = machine->pop<A7>();
        A6 a6 = machine->pop<A6>();
        A5 a5 = machine->pop<A5>();
        A4 a4 = machine->pop<A4>();
        A3 a3 = machine->pop<A3>();
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
class Method9 : public Function {
public:
    typedef R (C::*Target)(A1, A2, A3, A4, A5, A6, A7, A8, A9);
    Method9(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { Result<R>::call(machine, this); }

    // Pops the arguments and calls the target.
    R invoke(Machine *machine)
    {
        A9 a9 = machine->pop<A9>();
        A8 a8 = machine->pop<A8>();
        A7 a7 = machine->pop<A7>();
        A6 a6 = machine->pop<A6>();
        A5 a5 = machine->pop<A5>();
        A4 a4 = machine->pop<A4>();
        A3 a3 = machine->pop<A3>();
        A2 a2 = machine->pop<A2>();
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }

    C *m_object;
    Target m_method;
};

// Registers object->method under the given name.
template<class C, class R> inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)())
{
    addNamedMethod(name, new Method0<C, R>(object, method));
}
template<class C, class R, class A1>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1))
{
    addNamedMethod(name, new Method1<C, R, A1>(object, method));
}
template<class C, class R, class A1, class A2>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2))
{
    addNamedMethod(name, new Method2<C, R, A1, A2>(object, method));
}
template<class C, class R, class A1, class A2, class A3>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3))
{
    addNamedMethod(name, new Method3<C, R, A1, A2, A3>(object, method));
}
template<class C, class R, class A1, class A2, class A3, class A4, class A5>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5))
{
    addNamedMethod(name, new Method5<C, R, A1, A2, A3, A4, A5>(object, method));
}
template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6))
{
    addNamedMethod(name, new Method6<C, R, A1, A2, A3, A4, A5, A6>(object, method));
}
template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6, A7, A8))
{
    addNamedMethod(name, new Method8<C, R, A1, A2, A3, A4, A5, A6, A7, A8>(object, method));
}
template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
inline void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6, A7, A8, A9))
{
    addNamedMethod(name, new Method9<C, R, A1, A2, A3, A4, A5, A6, A7, A8, A9>(object, method));
}
template<class R> inline void Machine::addFunction(const std::string &name, R (*function)())
{
    addNamedMethod(name, new Function0<R>(function));
}
template<class R, class A1> inline void Machine::addFunction(const std::string &name, R (*function)(A1))
{
    addNamedMethod(name, new Function1<R, A1>(function));
}

} // Namespace abstr.
