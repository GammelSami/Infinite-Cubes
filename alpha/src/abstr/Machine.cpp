// abstr::Machine loads and interprets scripts through a table of opcode handlers.
//
// Many handlers contain arithmetic that cancels out, identical branches and loops over constant tables or
// zero-valued globals; they are kept as they are. Signed arithmetic and unchecked stack conversions keep their
// behaviour. Binary handlers consume the top stack value first.
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "abstr/Machine.h"
#include "abstr/Method.h"
#include "abstr/HelperIdentityClass.h"
#include "abstr/HelperCmpClass.h"
#include "cube/Scripting.h"

#if CUBE_SCRIPTING

namespace abstr {

// The g_zero* globals are never written: every use adds or multiplies by zero.
int g_zeroOffset;

int g_zeroTable[13];

int g_counterStep;

int g_counter;

int g_zeroFactor;

int g_zeroFactor2;

int g_zeroBias;

int g_zeroScale;

int g_runLoopCount = 3;

int g_variableDivisor = 1;

int g_variableReturnBase = 13;

int g_operandBias = 32;

int g_branchLoopEnabled = 1;

int g_notCount = 5;

int g_addOne = 1;

int g_minusOne = -1;

int g_notScale = 4;

int g_true = 1;

int g_lessOffsetCount = 3;

int g_lessFallback = 2048;

double g_compareScale = 1.35;

double g_compareScaleRight = 1.35;

std::string g_text = "";

// Return: drops the innermost frame and leaves the top value (the result) as the frame's only
// value.
void Machine::popFrame()
{
    int frame = m_frames.back();
    m_frames.pop_back();
    std::vector<char> result(m_stack.back());
    m_stack.resize(frame + 1);
    m_stack.back() = result;
}

// Adds a value after counting down to zero.
static double AddCountdown(int count, double value)
{
    if (count > 0)
        return AddCountdown(count - 1, value);
    if (count < 0)
        return AddCountdown(-count, value);
    return count + value;
}

// Pushes the next `count` decoded words as a byte string and skips them.
static int PushBytes(Machine *machine, int count)
{
    int startPosition = machine->m_pc;
    int *words = &machine->m_program[startPosition];
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

static int PushBytesOffset(Machine *machine, int count, int adjustment)
{
    count -= adjustment;
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

static int PushOperand(Machine *machine, int operand)
{
    machine->push<int>(operand);
    return 0;
}

static void PushBiasedOperand(Machine *machine, int operand);

static int PushOperandBiased(Machine *machine, int operand)
{
    PushBiasedOperand(machine, operand - g_variableReturnBase - 19);
    return 0;
}

// Returns the zero table's scaled sum. Both parameters are unused; callers pass a string or local pointer and the machine.
inline int ZeroTableSum(const void *, Machine *)
{
    int count = g_variableReturnBase;
    int sum = 0;
    for (int i = 0; i < count; ++i)
        sum += g_zeroTable[i];
    return g_zeroFactor * sum;
}

static int PushOperandTimesThree(Machine *machine, int operand)
{
    machine->push<int>(3 * operand / 3);
    return 0;
}

static int FloatToInt(Machine *machine, int)
{
    // Keep the raw float bytes intact while pop destroys the stack vector.
    int bits = machine->pop<int>();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    machine->push<int>((int)value);
    return 0;
}

static int FloatToIntValue(Machine *machine, int)
{
    int bits = machine->pop<int>();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    machine->push<int>((int)value);
    return 0;
}

static int FloatToIntHalved(Machine *machine, int operand)
{
    return 2 * FloatToIntValue(machine, operand) / 2;
}

static int PushVariable(Machine *machine, int index)
{
    machine->m_stack.push_back(machine->variable(index));
    return 0;
}

static int PushVariableDivided(Machine *machine, int index)
{
    machine->m_stack.push_back(machine->variable(index / g_variableDivisor));
    return 0;
}

static int PushVariableBiased(Machine *machine, int index)
{
    machine->m_stack.push_back(machine->variable(index));
    return g_variableReturnBase - 13;
}

static int PushVariableCopy(Machine *machine, int index)
{
    std::vector<char> value(machine->variable(index * (g_zeroFactor + 1)));
    machine->m_stack.push_back(value);
    return 0;
}

static int PushVariableAddress(Machine *machine, int index)
{
    // The machine's values are 32-bit ints: a reference is the variable's address.
    machine->push<int>((int)&machine->m_stack[machine->m_frames.back() + index][0]);
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
        // Stack values contain integer pointers; dereference the selected pointer.
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

// Keep the value const so StoreVariableB copies it.
static const std::vector<char> ZerosOrPop(Machine *machine, int count)
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
    machine->push<int>(operand);
    return 0;
}

static int PushBytesChecked(Machine *machine, int count)
{
    const int *words = &machine->m_program[machine->m_pc];
    std::vector<int> decoded(count);
    for (int i = 0; i < count; ++i)
        decoded[i] = g_zeroFactor - words[i] + machine->m_pc + i;
    std::vector<char> bytes(decoded.size());
    if (ZeroTableSum(&bytes, machine))
        return -32;
    for (int i = 0; i < (int)decoded.size(); ++i)
        bytes[i] = (char)decoded[i];
    machine->m_stack.push_back(bytes);
    return count;
}

// KOpcodeReturn (0x0c): the interpreter loops stop after it.
static int Return(Machine *machine, int)
{
    machine->popFrame();
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
} // namespace

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

static void setText(const std::string &prefix, float value)
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
    machine->push<int>(left + g_addOne * right);
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
    machine->push<int>(AddRecursive(right, left, 2));
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

// Subtract's result through a negated right operand (SubtractD's body).
static int SubtractB(Machine *machine)
{
    int left = machine->pop<int>();
    ++left;
    int right = -machine->pop<int>();
    machine->push<int>(left + right - 1);
    return 0;
}

// PUSHSTR (kOpcodePushString): pushes the next count decoded words as a string and skips them.
static int PushString(Machine *machine, int count)
{
    count += g_zeroOffset;
    return PushBytesOffset(machine, count, g_zeroOffset);
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
    setText(name, value);
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
    // The code scale can overflow before its nonzero test.
    machine->push<int>(right * left * 32 != 0);
    return 0;
}

static int Or(Machine *machine, int)
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

static int Equal(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    // The code equality opcode subtracts before testing, including overflow.
    machine->push<int>(left - right == 0);
    return 0;
}

static int SubtractC(Machine *machine, int)
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
    right -= 1;
    machine->push<int>(left - right == 4);
    return 0;
}

static int NotEqualOffset(Machine *machine, int)
{
    int left = machine->pop<int>();
    left += 8;
    int right = machine->pop<int>();
    right -= g_notScale;
    machine->push<int>(left - right != 12);
    return 0;
}

static int NotEqualTable(Machine *machine, int)
{
    int sum = ZeroTableSum("frame:text", machine);
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left == right)
        ++sum;
    machine->push<int>(sum == 0);
    return 0;
}

static int SubtractD(Machine *machine, int)
{
    return SubtractB(machine);
}

static int LessViaDouble(Machine *machine, int)
{
    double left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right > left);
    return 0;
}

static int MultiplyB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left * right);
    return 0;
}

static int Less(Machine *machine, int)
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
    --left;
    int right = machine->pop<int>();
    machine->push<int>((g_minusOne + 1 + right) * (g_variableDivisor + left));
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
        machine->push<int>(g_runLoopCount - 2);
    else if (left == right)
        machine->push<int>(g_true);
    else
        machine->push<int>(0);
    return 0;
}

static int AddB2(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    if (left < right)
        machine->push<int>(right + left);
    else
        machine->push<int>(right - g_minusOne * left);
    return 0;
}

static int LessEqualObject(Machine *machine, int)
{
    HelperCmpClass *threshold = new HelperCmpClass(machine->pop<int>());
    int value = machine->pop<int>();
    machine->push<int>(threshold->below(value));
    operator delete(threshold);
    return 0;
}

static int AndC(Machine *machine, int)
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
    for (int i = 1; i <= g_lessOffsetCount; ++i) {
        left += i;
        right += i;
    }
    machine->push<int>(left <= right);
    return 0;
}

static int IntToFloat(Machine *machine, int)
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
    if (left)
        result = 1;
    else if (right)
        result = 1;
    machine->push<int>(result);
    return 0;
}

static void NotRepeated(int *value, int times)
{
    if (times == 0)
        return;
    *value = !*value;
    NotRepeated(value, times - 1);
}

static int GreaterEqual(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(left >= right);
    return 0;
}

// Alternate greater-than-or-equal opcode.
static int GreaterEqualAlternate(Machine *machine, int)
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

static int NotScaled(Machine *machine, int)
{
    int value = machine->pop<int>();
    int result = g_notScale * value;
    machine->push<int>(!result);
    return 0;
}

static int FloatToIntScaled(Machine *machine, int)
{
    // Keep the raw float bytes intact while pop destroys the stack vector.
    int bits = machine->pop<int>();
    float rawValue;
    std::memcpy(&rawValue, &bits, sizeof(rawValue));
    double value = rawValue;
    machine->push<int>((int)(value * 3.0 * 0.0142 * 10.0 / (3.0 * 0.142)));
    return 0;
}

static int GreaterEqualNot(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    int result = left - right < 0;
    NotRepeated(&result, g_notCount);
    machine->push<int>(result != 0);
    return 0;
}

static int GreaterEqualB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    // The code equality test subtracts first, including signed overflow.
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

static void PushFrameProduct(Machine *machine, int factor, int count)
{
    machine->m_frames.push_back(machine->m_stack.size() + factor * count);
}

static int PushFrameScaled(Machine *machine, int count)
{
    if (count > 0)
        PushFrameProduct(machine, -1, count);
    else
        machine->m_frames.push_back(machine->m_stack.size() + g_minusOne * count);
    return 0;
}

static int PushFrame(Machine *machine, int count)
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
    // Subtraction and squaring retain signed overflow.
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
        machine->push<int>(g_operandBias > value);
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
    int bits = machine->pop<int>();
    for (int i = 0; i <= g_runLoopCount; ++i)
        bits += i;
    bits -= 2 * g_runLoopCount;
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
    machine->push<int>(operand);
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
    machine->push<int>((int)AddCountdown(3, operand));
    return 0;
}

static int PushFrameB(Machine *machine, int count)
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
    int count = ZeroTableSum("", machine);
    machine->variable(index) = ZerosOrPop(machine, count);
    return 0;
}

static int StoreVariableC(Machine *machine, int index)
{
    return g_zeroFactor2 * (StoreVariableB(machine, index) & 0xf7);
}

static int JumpOne(Machine *, int offset)
{
    HelperIdentityClass one(offset);
    HelperIdentityClass &object = one;
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
    for (int i = 0; i < value; ++i) {
        if (g_branchLoopEnabled)
            return 0;
        offset += i;
    }
    return offset;
}

static int JumpIfZero(Machine *machine, int offset)
{
    int value = machine->pop<int>();
    return value == 0 ? offset : 0;
}

static int StoreVariableD(Machine *machine, int index)
{
    std::vector<char> value = machine->popValue();
    machine->variable(index) = value;
    return 0;
}

static int NotEqual(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    machine->push<int>(right != left);
    return 0;
}

static int Discard(Machine *machine, int count)
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

static int JumpIfZeroBranch(Machine *machine, int offset)
{
    int value = machine->pop<int>();
    if (value != 0)
        return 0;
    return offset;
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
    // The code sign test follows signed multiplication, including overflow.
    machine->push<int>(10 * (left - right) < 0);
    return 0;
}

static void PushBiasedOperand(Machine *machine, int operand)
{
    machine->push<int>(g_operandBias + operand);
}

static int PushOperandPlus32(Machine *machine, int operand)
{
    if ((operand & 0xf0f0f8) == 3)
        machine->push<int>(operand);
    else
        PushBiasedOperand(machine, operand);
    return 0;
}

static int LessEqualPairs(Machine *machine, int)
{
    int pairs[2][2];
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            pairs[i][j] = i * 3 + j;
    const std::vector<char> &leftBytes = machine->m_stack.back();
    int left = *reinterpret_cast<const int *>(&leftBytes[0]);
    machine->m_stack.pop_back();
    pairs[0][1] = left;
    const std::vector<char> &rightBytes = machine->m_stack.back();
    int right = *reinterpret_cast<const int *>(&rightBytes[0]);
    machine->m_stack.pop_back();
    pairs[1][0] = right;
    // The first pair adds, the second subtracts: left - right - 4.
    int sum = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (i == 0)
                sum += pairs[i][j];
            else
                sum -= pairs[i][j];
        }
    }
    machine->push<int>(sum <= -4);
    return 0;
}

static int CallB(Machine *machine, int pc)
{
    if (!pc)
        machine->runProgram(pc);
    else
        machine->runProgram(pc);
    return 0;
}

static int GreaterOffsetB(Machine *machine, int)
{
    int left = machine->pop<int>();
    int right = machine->pop<int>();
    right += 2;
    machine->push<int>(left - right - 1 > -3);
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
    machine->push<int>(g_lessFallback / 2048);
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
    for (int i = 0; i < g_runLoopCount; ++i) {
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
    machine->push<int>(right * g_compareScaleRight < left);
    return 0;
}

void Machine::callFunction(const std::string &name, int argument)
{
    callFunctionKeepingResult(name, argument);
    if (!m_stack.empty())
        m_stack.pop_back();
}

void Machine::callFunctionKeepingResult(const std::string &name, int argument)
{
    int position = m_symbols[name];
    if (position >= 0) {
        push(argument);
        runProgram(position);
    }
}

// The opcode table: the slots are the script format's opcode numbers, several of them aliases of
// one handler (the handlers' names say what each does); only kOpcodeReturn and kOpcodePushString
// are named, the loops and the decoder testing them.
Machine::Machine()
    : m_pc(0)
{
    m_stack.reserve(1024);
    // Use the value-taking resize overload.
    m_opcodes.resize(kOpcodeCount, 0);
    m_opcodes[0] = PushOperand;
    m_opcodes[51] = PushOperandSwitch;
    m_opcodes[16] = StoreVariableB;
    m_opcodes[69] = PushOperand;
    m_opcodes[25] = IntToFloat;
    m_opcodes[42] = PushBytesCopy;
    m_opcodes[6] = IntToFloat;
    m_opcodes[63] = PushVariableDivided;
    m_opcodes[60] = PushOperand;
    m_opcodes[65] = PushVariableDivided;
    m_opcodes[43] = FloatToInt;
    m_opcodes[13] = PushOperandGrid;
    m_opcodes[46] = FloatToIntC;
    m_opcodes[34] = PushOperand;
    m_opcodes[64] = FloatToInt;
    m_opcodes[4] = PushFrameB;
    m_opcodes[97] = OrVector;
    m_opcodes[138] = JumpIfZeroBranch;
    m_opcodes[149] = Equal;
    m_opcodes[100] = Multiply;
    m_opcodes[68] = PushOperandRepeated;
    m_opcodes[30] = FloatToIntHalved;
    m_opcodes[102] = PushVariableCopy;
    m_opcodes[90] = IntToFloat;
    m_opcodes[41] = PushOperand;
    m_opcodes[36] = StoreVariableCopy;
    m_opcodes[24] = PushOperand;
    m_opcodes[101] = PushBytesRange;
    m_opcodes[140] = MultiplyB;
    m_opcodes[86] = Divide;
    m_opcodes[91] = DiscardLoop;
    m_opcodes[61] = IntToFloat;
    m_opcodes[70] = PushOperandBiased;
    m_opcodes[5] = PushOperandPlus32;
    m_opcodes[8] = StoreVariableD;
    m_opcodes[67] = PushOperandCountdown;
    m_opcodes[10] = PushOperandC;
    m_opcodes[73] = AddB;
    m_opcodes[37] = CallNativeSwitch;
    m_opcodes[9] = PushOperandCompare;
    m_opcodes[39] = PushOperand;
    m_opcodes[27] = PushVariableBiased;
    m_opcodes[kOpcodePushString] = PushString;
    m_opcodes[26] = PushVariable;
    m_opcodes[2] = StoreVariable;
    m_opcodes[1] = FloatToIntScaled;
    m_opcodes[38] = PushOperand;
    m_opcodes[3] = PushOperandTimesThree;
    m_opcodes[40] = StoreVariableC;
    m_opcodes[47] = IntToFloat;
    m_opcodes[29] = MultiplyB;
    m_opcodes[52] = Discard;
    m_opcodes[95] = SubtractC;
    m_opcodes[71] = PushFrameOffset;
    m_opcodes[55] = Add;
    m_opcodes[32] = Subtract;
    m_opcodes[76] = Or;
    m_opcodes[99] = SubtractC;
    m_opcodes[129] = OrAbs;
    m_opcodes[23] = AddViaNegation;
    m_opcodes[104] = MultiplyC;
    m_opcodes[125] = SubtractD;
    m_opcodes[45] = PushBytesChecked;
    m_opcodes[33] = PushOperandString;
    m_opcodes[116] = PushVariableAddress;
    m_opcodes[131] = PushIndirect;
    m_opcodes[44] = PushFrameScaled;
    m_opcodes[kOpcodeReturn] = Return;
    m_opcodes[28] = PushOperand;
    m_opcodes[15] = PushBytes;
    m_opcodes[127] = AddFloat;
    m_opcodes[58] = Discard;
    m_opcodes[14] = CallButton;
    m_opcodes[56] = SubtractFloat;
    m_opcodes[50] = PushFrame;
    m_opcodes[18] = CallScaled;
    m_opcodes[22] = CallNative;
    m_opcodes[17] = PushOperandE;
    m_opcodes[110] = CallNativeCounted;
    m_opcodes[35] = PushFrameB;
    m_opcodes[88] = AddB2;
    m_opcodes[92] = Modulo;
    m_opcodes[124] = DivideFloat;
    m_opcodes[109] = AndC;
    m_opcodes[54] = CallNative;
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
    m_opcodes[49] = SubtractC;
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
    m_opcodes[66] = GreaterEqualAlternate;
    m_opcodes[118] = JumpHalved;
    m_opcodes[143] = NotEqual;
    m_opcodes[147] = JumpBiased;
    m_opcodes[139] = Not;
    m_opcodes[103] = GreaterOffsetB;
    m_opcodes[130] = LessEqual;
    m_opcodes[142] = NotEqualTable;
    m_opcodes[78] = Equal;
    m_opcodes[135] = LessEqualObject;
    m_opcodes[83] = LessEqualOffsetB;
    m_opcodes[108] = LessViaDouble;
    m_opcodes[136] = JumpOne;
    m_opcodes[74] = MultiplyB;
    m_opcodes[134] = Jump;
    m_opcodes[126] = EqualSquare;
    m_opcodes[141] = JumpIfZeroCount;
    m_opcodes[122] = LessEqualOffset;
    m_opcodes[144] = Less;
    m_opcodes[121] = JumpIfZero;
    m_opcodes[82] = LessViaMask;
    m_opcodes[11] = LessEqualFloat;
    m_opcodes[81] = GreaterEqual;
    m_opcodes[62] = LessEqualPairs;
    m_opcodes[114] = GreaterEqualFloat;
    m_opcodes[21] = RunFrom;
    m_opcodes[132] = OrB;
    m_opcodes[84] = EqualOffset;
    m_opcodes[80] = NotEqualOffset;
    m_opcodes[137] = CallCaption;
    m_opcodes[75] = AndC;
    m_opcodes[133] = NotEqualB;
    m_opcodes[113] = Less;
    m_opcodes[115] = JumpIfZeroViaFloat;
    m_opcodes[119] = Or;
    m_opcodes[146] = GreaterOffset;
}

Machine::~Machine() {}

bool Machine::loadProgramFromFile(const std::string &path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
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
    int value; // the header's three unused words, then each symbol's value checks eofbit only; other read
    // failures can leave words uninitialized.
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
        // Trusts the stored length and does not add a terminator.
        char name[kMaxSymbolLength];
        in.read(name, length);
        if (in.eof())
            return false;
        in.read((char *)&value, 4);
        if (in.eof())
            return false;
        m_symbols[std::string(name)] = value;
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

void Machine::runProgram(int pc)
{
    if (pc >= 0 && pc < (int)m_program.size()) {
        int opcode;
        do {
            opcode = decode(pc++);
            int operand = decode(pc++);
            Opcode handler = m_opcodes[opcode];
            m_pc = pc;
            if (handler)
                pc += handler(this, operand);
        } while (opcode != kOpcodeReturn);
    }
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

} // namespace abstr

#endif // CUBE_SCRIPTING
