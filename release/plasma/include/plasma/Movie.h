// Movie stores named (name, value) entries without duplicates. Their meaning is otherwise unknown.
#pragma once
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

namespace plasma {

class Movie : public NamedObject {
public:
    Movie(Engine *engine, const std::wstring &name);

    // Appends the entry unless an equal one exists.
    void addEntry(const std::wstring &name, int value);

protected:
    struct Entry {
        Entry(const std::wstring &n, int v)
            : name(n),
              value(v)
        {}
        std::wstring name;
        int value;
    };

    std::vector<Entry> m_entries;
    int m_unused50; // Initially zero; no reader in Movie.cpp.
};

}
