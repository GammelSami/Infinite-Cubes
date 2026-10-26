// abstr::Machine: the stack machine that runs the program's compiled "PlasmaGraphics" scripts (the Workspace
// embeds one and loads its script at start-up). A program is a vector of int words. Word i encodes the value i -
// word, so a value depends on its position (Decode). Each instruction is two words: an opcode (an index into
// m_opcodes) and an operand. An opcode handler returns the number of words to skip after the instruction (a
// relative jump). Values on the stack are byte strings (std::vector<char>): ints and floats are pushed as their
// four bytes, strings as their characters. m_frames holds the stack depth at each call: a non-negative variable
// index counts from the innermost frame, a negative index -1-k addresses global k (Variable). Native functions
// (abstr::Function) are registered under the names of the program's symbol table and called by symbol value.
#pragma once
#include <cstring>
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "abstr/Function.h"

namespace abstr {

class Machine {
public:
    typedef int (*Opcode)(Machine *machine, int operand);

    // The return opcode ends the interpreter loop after returning from a call frame. PUSHSTR is the variable-length
    // instruction: its operand is the character count, followed by one word per character. gui.plx uses 24 of the 150
    // opcodes.
    enum {
        kOpcodeReturn = 0x0c,
        kOpcodePushString = 0x13,
        kOpcodeCount = 150
    };

    // The file header ParseFile accepts: version 1, then "PlasmaGraphics".
    enum {
        kProgramVersion = 1,
        kMaxSymbolLength = 512
    };

    Machine();
    virtual ~Machine();

    bool loadProgramFromFile(const std::string &path);
    bool parseFile(std::istream &in);

    // Runs from word pc to the return opcode (kOpcodeReturn); returns the last handler's result or the decoded
    // operand.
    int runProgram(int pc);

    // Calls the native function registered under symbol value id, or pops its arguments and pushes 0 when none is
    // registered.
    void callFunction(int id);

    // Registers function under a symbol of the loaded program; the machine takes ownership. Unknown names leave
    // function unowned.
    void addNamedMethod(const std::string &name, Function *function)
    {
        if (m_symbols.find(name) != m_symbols.end()) {
            int id = m_symbols[name];
            if (m_functions[id])
                delete m_functions[id];
            m_functions[id] = function;
#ifndef PLASMA_NO_LICENSING
            if (const int *words = function->m_keyWords) {
                m_key += words[0];
                m_key += words[1];
                m_key += words[2];
                m_key += words[3];
            }
#endif
        }
    }
    // Registers object->method or a function under the given name through the binders in abstr/Method.h.
    template<class C, class R> void addMethod(const std::string &name, C *object, R (C::*method)());
    template<class C, class R, class A1> void addMethod(const std::string &name, C *object, R (C::*method)(A1));
    template<class C, class R, class A1, class A2>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2));
    template<class C, class R, class A1, class A2, class A3>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3));
    template<class C, class R, class A1, class A2, class A3, class A4, class A5>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5));
    template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6));
    template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6, A7, A8));
    template<class C, class R, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    void addMethod(const std::string &name, C *object, R (C::*method)(A1, A2, A3, A4, A5, A6, A7, A8, A9));
    template<class R> void addFunction(const std::string &name, R (*function)());
    template<class R, class A1> void addFunction(const std::string &name, R (*function)(A1));
    // The "sys" function: the key accumulated while registering functions.
    int getKey() { return m_key; }

    void pushBytesToStack(const void *bytes, size_t size);
    template<class T> void push(T value) { pushBytesToStack(&value, sizeof(T)); }
    template<class T> T pop()
    {
#if defined(PLASMA_PORTABLE) || defined(_WIN64)
        T value;
        memcpy(&value, &m_stack.back()[0], sizeof(value));
#else
        T value = *reinterpret_cast<T *>(&m_stack.back()[0]);
#endif
        m_stack.pop_back();
        return value;
    }
    // Stack values are returned as const copies.
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

    std::map<int, Function *> m_functions; // By symbol value.
    std::vector<int> m_program;            // Encoded words.
    std::vector<int> m_initializers;       // Instruction positions run by ParseFile.
    std::map<std::string, int> m_symbols;  // Symbol table of the program.
    std::vector<std::vector<char>> m_stack;
    std::list<int> m_frames;       // Stack depth at each call.
    std::vector<Opcode> m_opcodes; // kOpcodeCount handlers.
    int m_key;                     // See AddNamedMethod; zeroed by ParseFile.
    int m_pc;                      // Word after the current instruction.
};

} // Namespace abstr.
