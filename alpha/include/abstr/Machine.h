#pragma once
#include <cstring>
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "abstr/Function.h"

namespace abstr {

// Encoded program words decode as (word position - stored value). Stack values are byte vectors;
// negative variable indices address global values.
class Machine {
public:
    typedef int (*Opcode)(Machine *, int);
    enum OpcodeId {
        kOpcodeReturn = 12,
        kOpcodePushString = 19
    };
    enum {
        kOpcodeCount = 150
    };
    enum {
        kProgramVersion = 1
    };
    enum {
        kMaxSymbolLength = 512
    };

    Machine();
    virtual ~Machine();
    bool loadProgramFromFile(const std::string &path);
    bool parseFile(std::istream &in);
    void runProgram(int pc);
    void callFunction(int id);
    void popFrame();
    void pushBytesToStack(const void *bytes, size_t size)
    {
        std::vector<char> value(size);
        // Preserve the code unchecked access when size is zero.
        memcpy(&value[0], bytes, size);
        m_stack.push_back(value);
    }
    template<class T> void push(T value) { pushBytesToStack(&value, sizeof(T)); }
    template<class T> T pop()
    {
        T value = *reinterpret_cast<T *>(&m_stack.back()[0]);
        m_stack.pop_back();
        return value;
    }
    const std::vector<char> popValue()
    {
        std::vector<char> value(m_stack.back());
        m_stack.pop_back();
        return value;
    }
    std::vector<char> &variable(int index)
    {
        if (index < 0)
            return m_stack[-1 - index];
        return m_stack[m_frames.back() + index];
    }
    int decode(int position) { return position - m_program[position]; }
    int getKey() { return m_key; }
    template<class C, class R> void addMethod(const std::string &name, C *object, R (C::*method)());
    template<class C, class R, class A1> void addMethod(const std::string &name, C *object, R (C::*method)(A1));
    template<class C, class R, class A1, class A2>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2));
    template<class C, class R, class A1, class A2, class A3>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3));
    void addNamedMethod(const std::string &name, Function *function)
    {
        if (m_symbols.find(name) != m_symbols.end()) {
            int id = m_symbols[name];
            if (m_functions[id])
                delete m_functions[id];
            m_functions[id] = function;
            if (const int *words = function->m_keyWords) {
                m_key += words[0];
                m_key += words[1];
                m_key += words[2];
                m_key += words[3];
            }
        }
    }

    // Calls the program's function name with one argument and drops its result.
    void callFunction(const std::string &name, int argument);
    // Calls the program's function name with one argument, when the program has it, leaving its result
    // on the stack.
    void callFunctionKeepingResult(const std::string &name, int argument);
    // Calls the program's function name with two arguments, when the program has it.
    template<class A1, class A2> void callFunction(const std::string &name, A1 first, A2 second)
    {
        int position = m_symbols[name];
        if (position >= 0) {
            push(first);
            push(second);
            runProgram(position);
        }
    }
    // The same, returning the function's result.
    template<class R, class A1, class A2> R evaluate(const std::string &name, A1 first, A2 second)
    {
        callFunction(name, first, second);
        return pop<R>();
    }

    std::map<int, Function *> m_functions; // registered functions
    std::vector<int> m_program;            // encoded words
    std::vector<int> m_initializers;       // entry positions run after loading
    std::map<std::string, int> m_symbols;
    std::vector<std::vector<char>> m_stack;
    std::list<int> m_frames; // saved stack depths
    std::vector<Opcode> m_opcodes;
    int m_key; // registration checksum
    int m_pc;  // next instruction word
};
// A string value is pushed as its characters, without a terminator.
template<> inline void Machine::push<std::string>(std::string value)
{
    pushBytesToStack(value.c_str(), value.size());
}
// A string value is popped as its characters.
template<> inline std::string Machine::pop<std::string>()
{
    std::vector<char> value = popValue();
    return std::string(value.begin(), value.end());
}
}
