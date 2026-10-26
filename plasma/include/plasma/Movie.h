// Movie is a named engine object holding a list of unique (name, value) entries.
#pragma once
#include <iosfwd>
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

namespace plasma {

class Movie : public NamedObject {
    friend void writePLAMovie(std::ostream &, Movie *); // Writes the entries.
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
    int m_unused38; // Zeroed by the constructor; no reader or other writer in the program.
};

}
