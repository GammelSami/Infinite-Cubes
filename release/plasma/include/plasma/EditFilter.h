// EditFilter validates Edit text. The available filters accept signed or unsigned integer or decimal input.
#pragma once
#include <string>

namespace plasma {

class EditFilter {
public:
    virtual bool validate(const std::wstring &text) = 0;
};

class IntegerFilter : public EditFilter {
public:
    // Digits, optionally after a leading minus sign; empty is valid.
    virtual bool validate(const std::wstring &text);
};

class UnsignedIntegerFilter : public EditFilter {
public:
    // Digits only; empty is valid.
    virtual bool validate(const std::wstring &text);
};

class FloatFilter : public EditFilter {
public:
    // Digits with at most one '.' or ',', optionally after a leading minus.
    virtual bool validate(const std::wstring &text);
};

class UnsignedFloatFilter : public EditFilter {
public:
    // Digits with at most one '.' or ','.
    virtual bool validate(const std::wstring &text);
};

static IntegerFilter g_integerFilter;
static FloatFilter g_floatFilter;
static UnsignedIntegerFilter g_unsignedIntegerFilter;
static UnsignedFloatFilter g_unsignedFloatFilter;

} // namespace plasma
