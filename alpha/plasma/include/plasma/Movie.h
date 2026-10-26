// Movie holds unique named entries; their higher-level meaning is unknown.
#pragma once
#include <iosfwd>
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

namespace plasma {

class Movie : public NamedObject {
    friend void writePLAMovie(std::ostream &, Movie *);

public:
    Movie(Engine *engine, const std::wstring &name);

    // Appends the entry unless an equal one exists.
    void addEntry(const std::wstring &name, int value);

protected:
    struct Entry {
        std::wstring name;
        int value;
    };

    std::vector<Entry> m_entries;
    int m_unused30; // zeroed by the constructor; no reader or other writer in the program
};

}
