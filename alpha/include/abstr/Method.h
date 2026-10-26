#pragma once
#include "abstr/Machine.h"

namespace abstr {
// Member bindings registered by name (Machine::addMethod). Their call pops the arguments (the last
// one first), calls the member and pushes its result; a void member pushes 0. Overloading on the
// member pointer's type selects the void form.
template<class C, class R> class Method0 : public Function {
public:
    typedef R (C::*Target)();
    Method0(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { machine->push<R>(invoke(machine)); }
    R invoke(Machine *) { return (m_object->*m_method)(); }
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
    virtual void call(Machine *machine) { call(m_method, machine); }
    template<class T> void call(T (C::*)(A1), Machine *machine) { machine->push<R>(invoke(machine)); }
    void call(void (C::*)(A1), Machine *machine)
    {
        invoke(machine);
        machine->push<int>(0);
    }
    R invoke(Machine *machine)
    {
        A1 a1 = machine->pop<A1>();
        return (m_object->*m_method)(a1);
    }
    C *m_object;
    Target m_method;
};

template<class C, class R, class A1, class A2> class Method2 : public Function {
public:
    typedef R (C::*Target)(A1, A2);
    Method2(C *object, Target method)
        : m_object(object),
          m_method(method)
    {}
    virtual void call(Machine *machine) { call(m_method, machine); }
    template<class T> void call(T (C::*)(A1, A2), Machine *machine) { machine->push<R>(invoke(machine)); }
    void call(void (C::*)(A1, A2), Machine *machine)
    {
        invoke(machine);
        machine->push<int>(0);
    }
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
    virtual void call(Machine *machine) { call(m_method, machine); }
    template<class T> void call(T (C::*)(A1, A2, A3), Machine *machine) { machine->push<R>(invoke(machine)); }
    void call(void (C::*)(A1, A2, A3), Machine *machine)
    {
        invoke(machine);
        machine->push<int>(0);
    }
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

template<class C, class R> void Machine::addMethod(const std::string &name, C *object, R (C::*method)())
{
    addNamedMethod(name, new Method0<C, R>(object, method));
}

template<class C, class R, class A1> void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1))
{
    addNamedMethod(name, new Method1<C, R, A1>(object, method));
}

template<class C, class R, class A1, class A2>
void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2))
{
    addNamedMethod(name, new Method2<C, R, A1, A2>(object, method));
}

template<class C, class R, class A1, class A2, class A3>
void Machine::addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3))
{
    addNamedMethod(name, new Method3<C, R, A1, A2, A3>(object, method));
}

}
