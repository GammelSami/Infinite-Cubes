// Machine implements the script loader, interpreter and 150 opcode handlers. Many handlers contain obfuscating
// arithmetic, identical branches, constant-table loops and zero-valued globals. Signed arithmetic and stack access
// are unchecked. Binary handlers pop the left operand first because bytecode pushes the right operand first: for
// v1 % 256 it pushes 256, then v1, then Modulo. Handler names and left/right locals follow that order.
#include "plasma/FilePath.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "abstr/Machine.h"
#include "abstr/Method.h"

using abstr::Machine;

// The g_zero* globals are never written; every use adds or multiplies by zero.
int g_zeroOffset;
int g_zeroTable[13];
int g_counterStep; // The last CallNativeCounted id.
int g_counter;     // CallNativeCounted's running 10 * id sum.
int g_zeroFactor;
int g_zeroFactor2;
int g_zeroBias;
int g_zeroScale;
double g_compareScale = 1.35; // Both sides of GreaterScaled.
std::string g_text;           // SetText's formatted text.

void Machine::pushBytesToStack(const void *bytes, size_t size)
{
    std::vector<char> value(size);

    // Preserve the unchecked access when size is zero.
    memcpy(&value[0], bytes, size);
    m_stack.push_back(value);
}

// Return: drops the innermost frame and leaves the top value (the result) as the frame's only value.
static void popFrame(Machine *machine)
{
    int frame = machine->m_frames.back();
    machine->m_frames.pop_back();
    std::vector<char> result(machine->m_stack.back());
    machine->m_stack.resize(frame + 1);
    machine->m_stack.back() = result;
}

// Returns the machine's accumulated key.
static int PushOperandA(Machine *machine, int operand)
{
    // An unused int-to-double-to-int conversion.
    double value = operand;
    machine->push<int>((int)value);
    return 0;
}

// Counts down to zero, first negating a negative count, and returns value.
static double addCountdown(int count, double value)
{
    if (count > 0)
        return addCountdown(count - 1, value);
    if (count < 0)
        return addCountdown(-count, value);
    return count + value;
}

// Pushes the next `count` decoded words as a byte string and skips them.
static int PushBytes(Machine *machine, int count)
{
    int startPosition = machine->m_pc;
    std::vector<int>::iterator words = machine->m_program.begin();
    words += startPosition;
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i) {
        int position = machine->m_pc + i;
        decoded[i] = position - words[i];
    }
    std::vector<char> bytes(count);
    for (int i = 0; i < count; ++i)
        bytes[i] = (char)decoded[i];
    machine->m_stack.push_back(bytes);
    return count;
}

static int pushBytesOffset(Machine *machine, int count)
{
    count -= g_zeroOffset;
    int startPosition = machine->m_pc;
    std::vector<int>::iterator words = machine->m_program.begin();
    words += startPosition;
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i) {
        int value = machine->m_pc - words[i];
        decoded[i] = value + i;
    }
    std::vector<int>::iterator first = decoded.begin();
    std::vector<char> bytes(decoded.size());
    for (int i = 0; i < (int)decoded.size(); ++i)
        bytes[i] = (char)first[i];
    machine->m_stack.push_back(bytes);
    return count;
}

template<int opcode> static int PushOperand(Machine *machine, int operand)
{
    machine->push<int>(operand);
    return 0;
}

// Stores the selected variable from stack data.
inline int zeroTableSum()
{
    int sum = 0;
    for (int i = 0; i < 13; ++i)
        sum += g_zeroTable[i];
    return g_zeroFactor * sum;
}

static int PushOperandTimesThree(Machine *machine, int operand)
{
    machine->push<int>(3 * operand / 3);
    return 0;
}

template<int opcode> static int FloatToInt(Machine *machine, int)
{
    // Keep the raw float bytes intact while Pop destroys the stack vector.
    int bits = machine->pop<int>();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    machine->push<int>((int)value);
    return 0;
}

static int FloatToIntB(Machine *machine, int)
{
    // Keep the raw float bytes intact while Pop destroys the stack vector.
    int bits = machine->pop<int>();
    float rawValue;
    std::memcpy(&rawValue, &bits, sizeof(rawValue));
    double value = rawValue;
    int result = (int)value;
    machine->push<int>(result);
    return 0;
}

template<int opcode> static int PushVariable(Machine *machine, int index)
{
    machine->m_stack.push_back(machine->variable(index));
    return 0;
}

static int PushVariableCopy(Machine *machine, int index)
{
    std::vector<char> value(machine->variable(index * (g_zeroFactor + 1)));
    machine->m_stack.push_back(value);
    return 0;
}

static int PushVariableAddress(Machine *machine, int index)
{
#if defined(PLASMA_PORTABLE) || defined(_WIN64)

    // References occupy a host pointer; bytecode integers remain 32 bits.
    machine->push<char *>(&machine->m_stack[machine->m_frames.back() + index][0]);
#else

    // The machine's values are 32-bit ints: a reference is the variable's address.
    machine->push<int>((int)&machine->m_stack[machine->m_frames.back() + index][0]);
#endif
    return 0;
}

static int PushBytesRange(Machine *machine, int count)
{
    const int *words = &machine->m_program[machine->m_pc];
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i) {
        int value = machine->m_pc - words[i];
        decoded[i] = value + i;
    }
    std::vector<char> bytes(decoded.begin(), decoded.end());
    machine->m_stack.push_back(bytes);
    return count;
}

static int PushIndirect(Machine *machine, int index)
{
    int *value;
    if (index < 0) {
        // Stack values contain integer pointers; dereferences m_stack[-index], rather than Variable's global slot -1 -
        // index.
        value = *reinterpret_cast<int **>(&machine->m_stack[-index][0]);
    } else {
        value = *reinterpret_cast<int **>(&machine->m_stack[machine->m_frames.back() + index][0]);
    }
    index = *value;
    machine->push<int>(index);
    return 0;
}

static int PushOperandGrid(Machine *machine, int operand)
{
    int result;
    int grid[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i + j == 3)
                grid[i][j] = i + 3 * operand;
            else
                grid[i][j] = operand + j * i;
        }
    }
    result = (grid[2][1] - 2) / 3;
    machine->push<int>(result);
    return 0;
}

static int StoreVariable(Machine *machine, int index)
{
    machine->variable(index) = machine->popValue();
    return 0;
}

// Keeps the value const so StoreVariableB copies it.
static const std::vector<char> zerosOrPop(int count, Machine *machine)
{
    if (!count)
        return machine->popValue();
    return std::vector<char>(count, 0);
}

static int StoreVariableCopy(Machine *machine, int index)
{
    std::vector<char> value = machine->popValue();
    std::vector<char> copy(value.begin(), value.end());
    value = copy;
    machine->variable(index) = value;
    return 0;
}

static int PushOperandC(Machine *machine, int operand)
{
    // Obfuscating array reference and unreachable branch.
    int values[3] = {operand, operand, operand};
    int &value = values[1];
    if (value != operand) {
        std::vector<char> bytes;
        machine->m_stack.push_back(bytes);
        return 0;
    }
    machine->push<int>(value);
    return 0;
}

static int PushBytesChecked(Machine *machine, int count)
{
    const int *words = &machine->m_program[machine->m_pc];
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i)
        decoded[i] = g_zeroFactor - words[i] + machine->m_pc + i;
    std::vector<char> bytes(decoded.size());
    if (zeroTableSum())
        return -32;
    for (int i = 0; i < (int)decoded.size(); ++i)
        bytes[i] = (char)decoded[i];
    machine->m_stack.push_back(bytes);
    return count;
}

static int PushOperandD(Machine *machine, int operand)
{
    // Both arms are the operand: the test is junk code.
    int value = g_zeroFactor ? operand : operand;
    machine->push<int>(value);
    return 0;
}

// kOpcodeReturn: the interpreter loops stop after it.
static int Return(Machine *machine, int)
{
    popFrame(machine);
    return 0;
}

static int PushOperandRepeated(Machine *machine, int operand)
{
    for (int i = 3; i; --i) {
        machine->push<int>(operand);
        machine->m_stack.pop_back();
    }
    machine->push<int>(operand);
    return 0;
}

static int PushBytesCopy(Machine *machine, int count)
{
    const int *words = &machine->m_program[machine->m_pc];
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i) {
        int value = machine->m_pc - words[i];
        decoded[i] = value + i;
    }
    std::vector<char> bytes(count);
    for (int i = 0; i < count; ++i)
        bytes[i] = (char)decoded[i];
    std::vector<char> copy(count);
    memcpy(&copy[0], &bytes[0], count);
    machine->m_stack.push_back(copy);
    return count;
}

namespace {
struct DiscardValue {
    DiscardValue(Machine *machine)
        : machine(machine)
    {}
    void operator()(float) const { machine->popValue(); }
    Machine *machine;
};
} // Namespace.

static int DiscardValues(Machine *machine, int count)
{
    std::vector<float> values(count, 0.0f);
    DiscardValue discard(machine);
    std::for_each(values.begin(), values.end(), discard);
    return 0;
}

static int DiscardSwitch(Machine *machine, int count)
{
    switch (count) {
    case 1:
        machine->popValue();
        break;
    case 2:
        machine->popValue();
        machine->popValue();
        break;
    default:
        for (int i = 0; i < count; ++i)
            machine->popValue();
        break;
    }
    return 0;
}

static int CallButton(Machine *machine, int pc)
{
    std::string name("checkbutton");
    machine->runProgram(pc);
    return 0;
}

static int CallScaled(Machine *machine, int pc)
{
    machine->runProgram(pc - pc * g_zeroFactor / 3);
    return 0;
}

static int CallNative(Machine *machine, int id)
{
    machine->callFunction(id);
    return 0;
}

static int CallNativeB(Machine *machine, int id)
{
    // An unused int-to-double-to-int conversion.
    double value = id;
    machine->callFunction((int)value);
    return 0;
}

static void SetText(const std::string &prefix, float value)
{
    std::stringstream stream;
    stream << prefix;
    stream << value;
    g_text = stream.str();
}

static int Add(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right + left);
    return 0;
}

static int AddRecursive(int left, int right, int depth)
{
    if (depth == 1)
        return right + left;
    return AddRecursive(left, right, depth - 1);
}

static int AddB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(AddRecursive(right, left, 1));
    return 0;
}

static int Subtract(Machine *machine, int)
{
    int left = machine->pop<int>();
    ++left;
    int right = machine->pop<int>();
    machine->push<int>(left - right - 1);
    return 0;
}

static int subtractB(Machine *machine)
{
    int left = machine->pop<int>();
    ++left;
    int right = machine->pop<int>();
    machine->push<int>(left - right - 1);
    return 0;
}

// PUSHSTR (kOpcodePushString): pushes the next count decoded words as a string and skips them.
static int PushString(Machine *machine, int count)
{
    count += g_zeroOffset;
    return pushBytesOffset(machine, count);
}

static int Multiply(Machine *machine, int)
{
    int left = machine->pop<int>();
    left *= 2;
    int right = machine->pop<int>();
    machine->push<int>((left / 2) * right);
    return 0;
}

static int CallCaption(Machine *machine, int id)
{
    float value = cos(id * 0.1f);
    std::string name("caption");
    SetText(name, value);
    machine->callFunction(id);
    return 0;
}

static int Modulo(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left % right);
    return 0;
}

static int Divide(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left / right);
    return 0;
}

static int SubtractFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<float>(left - right);
    return 0;
}

static int MultiplyFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<float>(right * left);
    return 0;
}

static int AddFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<float>(right + left);
    return 0;
}

static int CallNativeSwitch(Machine *machine, int id)
{
    switch (id) {
    case 0:
        machine->callFunction(0);
        return 0;
    case 1:
        machine->callFunction(1);
        return 0;
    case 2:
        if (g_counterStep)
            g_counter += 10 * g_counterStep;
        machine->callFunction(2);
        return 0;
    default:
        machine->callFunction(id);
        return 0;
    }
}

static int MultiplyNonZero(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();

    // The scale can overflow before its nonzero test.
    machine->push<int>(right * left * 32 != 0);
    return 0;
}

template<int opcode> static int Or(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left || right);
    return 0;
}

static int AddViaNegation(Machine *machine, int)
{
    int left = machine->pop<int>();
    left = -left;
    int right = machine->pop<int>();
    machine->push<int>(right - left);
    return 0;
}

static int OrVector(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    std::vector<int> values(2, right);
    machine->push<int>(left || values.back());
    return 0;
}

static int Not(Machine *machine, int)
{
    int value = machine->pop<int>();
    machine->push<int>(value == 0);
    return 0;
}

static int NotSquare(Machine *machine, int)
{
    int value = machine->pop<int>();
    machine->push<int>(value * value == 0);
    return 0;
}

static int AddViaDouble(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    double sum = (double)right + left;
    sum *= 3.25;
    sum *= 4 / 13.0;
    machine->push<int>((int)sum);
    return 0;
}

template<int opcode> static int Equal(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();

    // The equality opcode subtracts before testing, including overflow.
    machine->push<int>(left - right == 0);
    return 0;
}

template<int opcode> static int SubtractC(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left - right);
    return 0;
}

static int EqualOffset(Machine *machine, int)
{
    int left = machine->pop<int>();
    left += 3;
    int right = machine->pop<int>();
    machine->push<int>(left - right + 1 == 4);
    return 0;
}

static int NotEqualOffset(Machine *machine, int)
{
    int left = machine->pop<int>();
    left += 8;
    int right = machine->pop<int>();
    machine->push<int>(left - right + 4 != 12);
    return 0;
}

static int NotEqualTable(Machine *machine, int)
{
    int sum = zeroTableSum();
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left == right)
        ++sum;
    machine->push<int>(sum == 0);
    return 0;
}

static int SubtractD(Machine *machine, int)
{
    return subtractB(machine);
}

static int LessViaDouble(Machine *machine, int)
{
    double left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right > left);
    return 0;
}

template<int opcode> static int MultiplyB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left * right);
    return 0;
}

template<int opcode> static int Less(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right > left);
    return 0;
}

static int GreaterViaBits(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    unsigned int bits;
    if (left == right)
        bits = 0xf0;
    else
        bits = left < right ? 0xf3 : 0x7f;
    machine->push<int>((bits >> 3) & 1);
    return 0;
}

static int MultiplyC(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right * left);
    return 0;
}

static int And(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left)
        left = 1;
    if (right)
        right = 1;
    machine->push<int>(left * right);
    return 0;
}

static int GreaterOffset(Machine *machine, int)
{
    int left = machine->pop<int>();
    left += 10;
    int shifted = -3;
    if (left)
        shifted = left - 3;
    int right = machine->pop<int>();
    machine->push<int>(shifted - right > 7);
    return 0;
}

static int DivideFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<float>(left / right);
    return 0;
}

static int AndB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left && right) {
        machine->push<int>(1);
        return 0;
    }
    machine->push<int>(0);
    return 0;
}

static int Mixed(Machine *machine, int operand)
{
    switch (operand) {
    case 3:
        machine->push<int>(32);
        return -1;
    case 4: {
        int value = machine->pop<int>();
        machine->push<int>(operand * value);
        return 0;
    }
    case 5:
        machine->push<float>(machine->pop<float>() / 3.51f);
        return 0;
    case 6:
        machine->push<float>(machine->pop<float>() / 7.02f);
        return 0;
    default: {
        int left = machine->pop<int>();
        int right = machine->pop<int>();
        machine->push<int>(left > right);
        return 0;
    }
    }
}

static int LessEqual(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left < right)
        machine->push<int>(1);
    else if (left == right)
        machine->push<int>(1);
    else
        machine->push<int>(0);
    return 0;
}

namespace {

// The virtual call of LessEqualObject.
struct Threshold {
    Threshold(int limit)
        : m_one(1),
          m_limit(limit)
    {}
    virtual int atLeast(int value) { return m_limit > value ? 0 : m_one; }
    int m_one;
    int m_limit;
};
} // Namespace.

// (Threshold::AtLeast).
static int AddB2(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left < right)
        machine->push<int>(right + left);
    else
        machine->push<int>(right + left);
    return 0;
}

static int LessEqualObject(Machine *machine, int)
{
    Threshold *threshold = new Threshold(machine->pop<int>());
    int value = machine->pop<int>();
    machine->push<int>(threshold->atLeast(value));
    operator delete(threshold);
    return 0;
}

template<int opcode> static int AndC(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left && right);
    return 0;
}

static int LessEqualOffset(Machine *machine, int)
{
    const std::vector<char> &leftBytes = machine->m_stack.back();
    int left = *reinterpret_cast<const int *>(&leftBytes[0]);
    machine->m_stack.pop_back();
    const std::vector<char> &rightBytes = machine->m_stack.back();
    int right = *reinterpret_cast<const int *>(&rightBytes[0]);
    machine->m_stack.pop_back();
    machine->push<int>(left + 6 <= right + 6);
    return 0;
}

template<int opcode> static int IntToFloat(Machine *machine, int)
{
    int value = machine->pop<int>();
    machine->push<float>((float)value);
    return 0;
}

static int OrAbs(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(abs(left) + abs(right) > 0);
    return 0;
}

static int LessEqualOffsetB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left + 3 <= right + 3);
    return 0;
}

static int OrB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    int result = 0;
    if (left || right)
        result = 1;
    machine->push<int>(result);
    return 0;
}

template<int opcode> static int GreaterEqual(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left >= right);
    return 0;
}

static int GreaterEqualNegated(Machine *machine, int)
{
    int left = -machine->pop<int>();
    int right = -machine->pop<int>();
    int result = left == right ? 1 : left <= right;
    machine->push<int>(result);
    return 0;
}

// Counts down recursively, as AddCountdown does.
static void NotRepeated(int *value, int times)
{
    *value = !*value;
    if (times != 1)
        NotRepeated(value, times - 1);
}

static int NotScaled(Machine *machine, int)
{
    int value = machine->pop<int>();
    machine->push<int>(4 * value == 0);
    return 0;
}

static int FloatToIntScaled(Machine *machine, int)
{
    // Keep the raw float bytes intact while Pop destroys the stack vector.
    int bits = machine->pop<int>();
    float rawValue;
    std::memcpy(&rawValue, &bits, sizeof(rawValue));
    double value = rawValue;
    // The divisor folds to 0.42599999999999993, one ulp below 0.426.
    machine->push<int>((int)(value * 3.0 * 0.0142 * 10.0 / (3.0 * 0.142)));
    return 0;
}

static int GreaterEqualNot(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    int result = left - right >= 0;
    NotRepeated(&result, 4);
    machine->push<int>(result != 0);
    return 0;
}

static int GreaterEqualB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();

    // The equality test subtracts first, including signed overflow.
    if (left - right == 0)
        machine->push<int>((right - left) * right * left <= 0);
    else if (left < right)
        machine->push<int>(0);
    else
        machine->push<int>(1);
    return 0;
}

static int EqualFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left == right));
    return 0;
}

static int NotAbs(Machine *machine, int)
{
    int value = machine->pop<int>();
    machine->push<int>(abs(value) <= 0);
    return 0;
}

template<int opcode> static int PushFrame(Machine *machine, int count)
{
    if (count > 0)
        machine->m_frames.push_back(machine->m_stack.size() - count);
    else
        machine->m_frames.push_back(machine->m_stack.size() - count);
    return 0;
}

static int NotEqualFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left != right));
    return 0;
}

static int EqualSquare(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();

    // The subtraction and squaring retain signed overflow.
    left -= right;
    machine->push<int>(left * left <= 0);
    return 0;
}

static int LessFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left < right));
    return 0;
}

static int GreaterFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left > right));
    return 0;
}

static int NotB(Machine *machine, int)
{
    int value = machine->pop<int>();
    if (value == 0)
        machine->push<int>(1);
    else
        machine->push<int>(0);
    return 0;
}

static int LessEqualFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left <= right));
    return 0;
}

static int FloatToIntC(Machine *machine, int)
{
    const std::vector<char> &bytes = machine->m_stack.back();
    int bits = *reinterpret_cast<const int *>(&bytes[0]);
    machine->m_stack.pop_back();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    machine->push<int>((int)value);
    return 0;
}

static int EqualScaled(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(10 * left == 10 * right);
    return 0;
}

static int PushFrameOffset(Machine *machine, int count)
{
    machine->m_frames.push_back(g_zeroFactor2 + machine->m_stack.size() - count);
    return 0;
}

static int GreaterEqualFloat(Machine *machine, int)
{
    float left = machine->pop<float>();
    float right = machine->pop<float>();
    machine->push<int>((int)(left >= right));
    return 0;
}

static int PushOperandE(Machine *machine, int operand)
{
    // An unused array reference.
    int values[3] = {operand, operand, operand};
    int &value = values[1];
    machine->push<int>(value);
    return 0;
}

static int JumpHalved(Machine *, int offset)
{
    return 2 * offset >> 1;
}

static int JumpTable(Machine *, int offset)
{
    int offsets[5] = {offset, offset, offset, offset, offset};
    return offsets[abs(offset) % 5];
}

static int PushOperandCountdown(Machine *machine, int operand)
{
    machine->push<int>((int)addCountdown(2, operand));
    return 0;
}

template<int opcode> static int PushFrameB(Machine *machine, int count)
{
    machine->m_frames.push_back(machine->m_stack.size() - count);
    return 0;
}

static int JumpBiased(Machine *, int offset)
{
    switch (offset) {
    case 0:
        return 0;
    case 1:
        return 1;
    default:
        return offset - g_zeroBias;
    }
}

static int PushOperandCompare(Machine *machine, int operand)
{
    const char *name = "button:press";

    // Both arms are the operand: the comparison is junk code.
    int value = strcmp(name, name) ? operand : operand;
    machine->push<int>(value);
    return 0;
}

static int Jump(Machine *, int offset)
{
    // Both paths return the offset; the code test is junk code.
    if (offset > 0 && abs(-offset) == offset)
        return offset;
    return offset;
}

inline int PushOperandString(Machine *machine, int operand)
{
    std::string text("");
    text.append(1, (char)operand);
    machine->push<int>(operand);
    return 0;
}

static int PushOperandSwitch(Machine *machine, int operand)
{
    switch (operand) {
    case 0:
        machine->push<int>(0);
        return 0;
    case 1:
        machine->push<int>(1);
        return 0;
    case 3:
        machine->push<int>(3);
        return 0;
    case 4:
        machine->push<int>(4);
        return 0;
    default:
        machine->push<int>(2 * operand / 2);
        return 0;
    }
}

static int NotEqualB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (right == left)
        right -= left;
    else
        right = 1;
    machine->push<int>(right);
    return 0;
}

static int StoreVariableB(Machine *machine, int index)
{
    int count = zeroTableSum();
    machine->variable(index) = zerosOrPop(count, machine);
    return 0;
}

namespace {

// The devirtualized call of MultiplyOne.
struct One {
    One(int value)
        : m_one(1),
          m_value(value)
    {}
    virtual int get() { return m_one; }
    int m_one;
    int m_value;
};
} // Namespace.

// (One::Get).
static int StoreVariableC(Machine *machine, int index)
{
    return g_zeroFactor2 * (StoreVariableB(machine, index) & 0xf7);
}

static int JumpOne(Machine *, int offset)
{
    One one(offset);
    One &object = one;
    return object.get() * one.m_value;
}

static int DiscardLoop(Machine *machine, int count)
{
    for (int i = 0; i < count && i >= 0; ++i) {
        if (i == 0)
            machine->popValue();
        else
            machine->popValue();
    }
    return 0;
}

static int NotEqualAbs(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(abs(left - right) > 0);
    return 0;
}

static int JumpIfNotPositive(Machine *machine, int offset)
{
    int value = machine->pop<int>();
    return value > 0 ? 0 : offset;
}

template<int opcode> static int JumpIfZero(Machine *machine, int offset)
{
    int value = machine->pop<int>();
    return value == 0 ? offset : 0;
}

static int StoreVariableD(Machine *machine, int index)
{
    std::vector<char> value = machine->popValue();

    // An unused int-to-double-to-int conversion.
    double position = index;
    machine->variable((int)position) = value;
    return 0;
}

static int NotEqual(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right != left);
    return 0;
}

template<int opcode> static int Discard(Machine *machine, int count)
{
    for (int i = 0; i < count; ++i)
        machine->popValue();
    return 0;
}

static int Call(Machine *machine, int pc)
{
    machine->runProgram(pc);
    return 0;
}

static int JumpIfZeroViaFloat(Machine *machine, int offset)
{
    const std::vector<char> &bytes = machine->m_stack.back();
    int value = *reinterpret_cast<const int *>(&bytes[0]);
    machine->m_stack.pop_back();
    float sum = 0.0f;
    sum += sum;
    sum += value;
    sum += 2 * value;
    sum += 3 * value;
    if (fabs(sum) > 0.0f)
        return 0;
    return offset;
}

static int LessViaMask(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();

    // The sign test follows signed multiplication, including overflow.
    machine->push<int>(10 * (left - right) < 0);
    return 0;
}

static int PushOperandPlus32(Machine *machine, int operand)
{
    if ((operand & 0xf0f0f8) == 3) {
        machine->push<int>(operand);
        return 0;
    }
    operand += 32;
    machine->push<int>(operand);
    return 0;
}

static int LessEqualPairs(Machine *machine, int)
{
    int pairs[4];
    for (int i = 0, j = 1; i < 6; i += 3, j += 2) {
        pairs[j - 1] = i;
        pairs[j] = i + 1;
    }
    const std::vector<char> &leftBytes = machine->m_stack.back();
    int left = *reinterpret_cast<const int *>(&leftBytes[0]);
    machine->m_stack.pop_back();
    const std::vector<char> &rightBytes = machine->m_stack.back();
    int right = *reinterpret_cast<const int *>(&rightBytes[0]);
    machine->m_stack.pop_back();
    int &offset = pairs[0];
    offset += left;
    offset -= right;
    offset -= pairs[3];
    machine->push<int>(offset <= -4);
    return 0;
}

static int CallB(Machine *machine, int pc)
{
    if (pc)
        machine->runProgram(pc);
    else
        machine->runProgram(pc);
    return 0;
}

static int GreaterOffsetB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left - (right + 2) - 1 > -3);
    return 0;
}

static int CallNativeCounted(Machine *machine, int id)
{
    g_counterStep = id;
    if (id)
        g_counter += 10 * id;
    machine->callFunction(id);
    return 0;
}

static int LessScaled(Machine *machine, int)
{
    int left = machine->pop<int>();
    int doubled = left * 2;
    int right = machine->pop<int>();
    if (left > right) {
        machine->push<int>(doubled * g_zeroScale);
        return 0;
    }
    if (left == right) {
        machine->push<int>(-g_zeroScale);
        return 0;
    }
    machine->push<int>(1);
    return 0;
}

static int JumpIfZeroCount(Machine *machine, int offset)
{
    int value = machine->pop<int>();
    if (!value)
        return offset;
    const char *name = "btn:check";
    int count = 0;
    for (int i = 0; i < 9; ++i, ++count)
        if (name[i] == ':')
            return 0;
    return count;
}

static int RunFrom(Machine *machine, int pc)
{
    for (int i = 0; i < 3; ++i) {
        if (i % 2 != 0) {
            if (pc >= 0 && pc < (int)machine->m_program.size()) {
                int opcode;
                do {
                    opcode = machine->decode(pc++);
                    int operand = machine->decode(pc++);
                    Machine::Opcode handler = machine->m_opcodes[opcode];
                    machine->m_pc = pc;
                    if (handler)
                        pc += handler(machine, operand);
                } while (opcode != Machine::kOpcodeReturn);
            }
            return 0;
        }
    }
    return -1;
}

static int GreaterScaled(Machine *machine, int)
{
    double left = machine->pop<int>() * g_compareScale;
    int right = machine->pop<int>();
    machine->push<int>(right * g_compareScale < left);
    return 0;
}

// Opcode dispatch indices are serialized instruction IDs. Repeated templated handlers are obfuscated aliases with
// distinct numeric IDs.
Machine::Machine()
    : m_pc(0)
{
    m_stack.reserve(1024);
    // Resizes with the supplied value.
    m_opcodes.resize(kOpcodeCount, 0);
    m_opcodes[0] = PushOperand<0>;
    m_opcodes[51] = PushOperandSwitch;
    m_opcodes[16] = StoreVariableB;
    m_opcodes[69] = PushOperand<69>;
    m_opcodes[25] = IntToFloat<25>;
    m_opcodes[42] = PushBytesCopy;
    m_opcodes[6] = IntToFloat<6>;
    m_opcodes[63] = PushVariable<63>;
    m_opcodes[60] = PushOperandA;
    m_opcodes[65] = PushVariable<65>;
    m_opcodes[43] = FloatToInt<43>;
    m_opcodes[13] = PushOperandGrid;
    m_opcodes[46] = FloatToIntC;
    m_opcodes[34] = PushOperand<34>;
    m_opcodes[64] = FloatToIntB;
    m_opcodes[4] = PushFrameB<4>;
    m_opcodes[97] = OrVector;
    m_opcodes[138] = JumpIfZero<138>;
    m_opcodes[149] = Equal<149>;
    m_opcodes[100] = Multiply;
    m_opcodes[68] = PushOperandRepeated;
    m_opcodes[30] = FloatToInt<30>;
    m_opcodes[102] = PushVariableCopy;
    m_opcodes[90] = IntToFloat<90>;
    m_opcodes[41] = PushOperand<41>;
    m_opcodes[36] = StoreVariableCopy;
    m_opcodes[24] = PushOperand<24>;
    m_opcodes[101] = PushBytesRange;
    m_opcodes[140] = MultiplyB<140>;
    m_opcodes[86] = Divide;
    m_opcodes[91] = DiscardLoop;
    m_opcodes[61] = IntToFloat<61>;
    m_opcodes[70] = PushOperand<70>;
    m_opcodes[5] = PushOperandPlus32;
    m_opcodes[8] = StoreVariableD;
    m_opcodes[67] = PushOperandCountdown;
    m_opcodes[10] = PushOperandC;
    m_opcodes[73] = AddB;
    m_opcodes[37] = CallNativeSwitch;
    m_opcodes[9] = PushOperandCompare;
    m_opcodes[39] = PushOperandD;
    m_opcodes[27] = PushVariable<27>;
    m_opcodes[kOpcodePushString] = PushString;
    m_opcodes[26] = PushVariable<26>;
    m_opcodes[2] = StoreVariable;
    m_opcodes[1] = FloatToIntScaled;
    m_opcodes[38] = PushOperand<38>;
    m_opcodes[3] = PushOperandTimesThree;
    m_opcodes[40] = StoreVariableC;
    m_opcodes[47] = IntToFloat<47>;
    m_opcodes[29] = MultiplyB<29>;
    m_opcodes[52] = Discard<52>;
    m_opcodes[95] = SubtractC<95>;
    m_opcodes[71] = PushFrameOffset;
    m_opcodes[55] = Add;
    m_opcodes[32] = Subtract;
    m_opcodes[76] = Or<76>;
    m_opcodes[99] = SubtractC<99>;
    m_opcodes[129] = OrAbs;
    m_opcodes[23] = AddViaNegation;
    m_opcodes[104] = MultiplyC;
    m_opcodes[125] = SubtractD;
    m_opcodes[45] = PushBytesChecked;
    m_opcodes[33] = PushOperandString;
    m_opcodes[116] = PushVariableAddress;
    m_opcodes[131] = PushIndirect;
    m_opcodes[44] = PushFrame<44>;
    m_opcodes[kOpcodeReturn] = Return;
    m_opcodes[28] = PushOperand<28>;
    m_opcodes[15] = PushBytes;
    m_opcodes[127] = AddFloat;
    m_opcodes[58] = Discard<58>;
    m_opcodes[14] = CallButton;
    m_opcodes[56] = SubtractFloat;
    m_opcodes[50] = PushFrame<50>;
    m_opcodes[18] = CallScaled;
    m_opcodes[22] = CallNative;
    m_opcodes[17] = PushOperandE;
    m_opcodes[110] = CallNativeCounted;
    m_opcodes[35] = PushFrameB<35>;
    m_opcodes[88] = AddB2;
    m_opcodes[92] = Modulo;
    m_opcodes[124] = DivideFloat;
    m_opcodes[109] = AndC<109>;
    m_opcodes[54] = CallNativeB;
    m_opcodes[85] = And;
    m_opcodes[48] = DiscardSwitch;
    m_opcodes[120] = MultiplyNonZero;
    m_opcodes[93] = Call;
    m_opcodes[59] = AndB;
    m_opcodes[53] = MultiplyFloat;
    m_opcodes[77] = NotScaled;
    m_opcodes[107] = AddViaDouble;
    m_opcodes[148] = GreaterScaled;
    m_opcodes[96] = DiscardValues;
    m_opcodes[112] = LessScaled;
    m_opcodes[7] = CallB;
    m_opcodes[79] = JumpIfNotPositive;
    m_opcodes[123] = NotAbs;
    m_opcodes[49] = SubtractC<49>;
    m_opcodes[128] = Mixed;
    m_opcodes[89] = NotSquare;
    m_opcodes[98] = GreaterEqualNegated;
    m_opcodes[106] = GreaterEqualB;
    m_opcodes[145] = GreaterViaBits;
    m_opcodes[87] = EqualFloat;
    m_opcodes[57] = LessFloat;
    m_opcodes[105] = EqualScaled;
    m_opcodes[94] = GreaterFloat;
    m_opcodes[31] = NotEqualAbs;
    m_opcodes[117] = NotB;
    m_opcodes[111] = NotEqualFloat;
    m_opcodes[20] = GreaterEqualNot;
    m_opcodes[72] = JumpTable;
    m_opcodes[66] = GreaterEqual<66>;
    m_opcodes[118] = JumpHalved;
    m_opcodes[143] = NotEqual;
    m_opcodes[147] = JumpBiased;
    m_opcodes[139] = Not;
    m_opcodes[103] = GreaterOffsetB;
    m_opcodes[130] = LessEqual;
    m_opcodes[142] = NotEqualTable;
    m_opcodes[78] = Equal<78>;
    m_opcodes[135] = LessEqualObject;
    m_opcodes[83] = LessEqualOffsetB;
    m_opcodes[108] = LessViaDouble;
    m_opcodes[136] = JumpOne;
    m_opcodes[74] = MultiplyB<74>;
    m_opcodes[134] = Jump;
    m_opcodes[126] = EqualSquare;
    m_opcodes[141] = JumpIfZeroCount;
    m_opcodes[122] = LessEqualOffset;
    m_opcodes[144] = Less<144>;
    m_opcodes[121] = JumpIfZero<121>;
    m_opcodes[82] = LessViaMask;
    m_opcodes[11] = LessEqualFloat;
    m_opcodes[81] = GreaterEqual<81>;
    m_opcodes[62] = LessEqualPairs;
    m_opcodes[114] = GreaterEqualFloat;
    m_opcodes[21] = RunFrom;
    m_opcodes[132] = OrB;
    m_opcodes[84] = EqualOffset;
    m_opcodes[80] = NotEqualOffset;
    m_opcodes[137] = CallCaption;
    m_opcodes[75] = AndC<75>;
    m_opcodes[133] = NotEqualB;
    m_opcodes[113] = Less<113>;
    m_opcodes[115] = JumpIfZeroViaFloat;
    m_opcodes[119] = Or<119>;
    m_opcodes[146] = GreaterOffset;
}

Machine::~Machine()
{
#ifdef PLASMA_PORTABLE
    for (std::map<int, abstr::Function *>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
        delete it->second;
#endif
}

bool Machine::loadProgramFromFile(const std::string &path)
{
    std::ifstream in(PLASMA_FILE_PATH(path.c_str()), std::ios::in | std::ios::binary);
    if (!in.is_open())
        return false;
    return parseFile(in);
}

bool Machine::parseFile(std::istream &in)
{
    m_program.clear();
    m_symbols.clear();
    m_initializers.clear();
    int count;
    int value; // The header's three unused words, then each symbol's value.

    // The code checks eofbit only; other read failures can leave words uninitialized.
    in.read((char *)&count, 4);
    if (count != kProgramVersion)
        return false;
    in.read((char *)&value, 4);
    in.read((char *)&value, 4);
    in.read((char *)&value, 4);
    char magic[] = "PlasmaGraphics";
    in.read(magic, 15);
    if (strcmp(magic, "PlasmaGraphics"))
        return false;
    in.read((char *)&count, 4);
    if (in.eof())
        return false;
    for (int i = 0; i < count; ++i) {
        int length;
        in.read((char *)&length, 4);
        if (length > kMaxSymbolLength)
            return false;
        if (in.eof())
            return false;

        // The code trusts the stored length and does not add a terminator.
        char name[kMaxSymbolLength];
        in.read(name, length);
        if (in.eof())
            return false;
        in.read((char *)&value, 4);
        if (in.eof())
            return false;
        std::string symbolName(name);
        m_symbols[symbolName] = value;
    }
    in.read((char *)&count, 4);
    if (count > 0) {
        m_initializers.resize(count);
        in.read((char *)&m_initializers[0], sizeof(int) * count);
    }
    in.read((char *)&count, 4);
    if (count <= 0 || in.eof())
        return false;
    m_program.resize(count);
    in.read((char *)&m_program[0], sizeof(int) * count);
    for (int i = 0; i < (int)m_initializers.size(); ++i) {
        int position = m_initializers[i];
        int opcode = decode(position);
        int operand = decode(position + 1);
        if (Opcode handler = m_opcodes[opcode])
            handler(this, operand);
    }
    m_key = 0;
    addMethod("sys", this, &Machine::getKey);
    return true;
}

int Machine::runProgram(int pc)
{
    int result = pc;
    if (pc >= 0) {
        result = (int)m_program.size();
        if (pc < result) {
            int opcode;
            do {
                opcode = decode(pc++);
                result = decode(pc++);
                Opcode handler = m_opcodes[opcode];
                m_pc = pc;
                if (handler) {
                    result = handler(this, result);
                    pc += result;
                }
            } while (opcode != kOpcodeReturn);
        }
    }
    return result;
}

void Machine::callFunction(int id)
{
    int arguments = id - m_program[id + 1] + 1;
    std::map<int, Function *>::iterator it = m_functions.find(id);
    if (it == m_functions.end()) {
        for (int i = 0; i < arguments; ++i)
            popValue();
        push(0);
    } else {
        it->second->call(this);
    }
}
