// The PDF object model of PDFReader.cpp. Every parsed object starts with its type tag; the parser allocates the
// concrete object with new, records it in PDFReader::m_allObjects and the reader deletes them all through Object*
// (Object has no virtual destructor, see PDFReader::~PDFReader).
#pragma once
#include <map>
#include <string>
#include <vector>

namespace pdf {

enum ObjectType {
    kDictionary = 1,
    kArray = 2,
    kString = 3,
    kName = 4,
    kNumber = 5,
    kBoolean = 6,
    kReference = 7
};

struct Object {
    Object(ObjectType type)
        : type(type)
    {}

    ObjectType type;
};

// << /Key value... >>; also the dictionary of a stream object.
struct Dictionary : Object {
    Dictionary();
    std::map<std::string, Object *> entries;
};

// [ value... ].
struct Array : Object {
    Array()
        : Object(kArray)
    {}
    std::vector<Object *> elements;
};

// ( text ), without escape processing.
struct String : Object {
    String()
        : Object(kString)
    {}
    std::string value;
};

// /Name, including the slash.
struct Name : Object {
    Name()
        : Object(kName)
    {}
    std::string value;
};

struct Number : Object {
    Number()
        : Object(kNumber)
    {}
    double value;
};

// True or false.
struct Boolean : Object {
    Boolean()
        : Object(kBoolean)
    {}
    bool value;
};

// N g R; resolved through PDFReader::m_objects by object number.
struct Reference : Object {
    Reference()
        : Object(kReference)
    {}
    int objectNumber;
};

inline Dictionary *asDictionary(Object *object)
{
    return object && object->type == kDictionary ? static_cast<Dictionary *>(object) : 0;
}

inline Array *asArray(Object *object)
{
    return object && object->type == kArray ? static_cast<Array *>(object) : 0;
}

inline Name *asName(Object *object)
{
    return object && object->type == kName ? static_cast<Name *>(object) : 0;
}

inline Boolean *asBoolean(Object *object)
{
    return object && object->type == kBoolean ? static_cast<Boolean *>(object) : 0;
}

inline Number *asNumber(Object *object)
{
    return object && object->type == kNumber ? static_cast<Number *>(object) : 0;
}

inline Reference *asReference(Object *object)
{
    return object && object->type == kReference ? static_cast<Reference *>(object) : 0;
}

}
