// Dialog stores a block of built text as coloured phrases, with optional item and command data. Speech builds dialogs
// for conversation, question and preview widgets and BaseWidget text rendering.
#pragma once
#include <array>
#include <list>
#include <string>
#include "plasma/Vector.h"
#include "cube/Item.h"

namespace cube {

struct Dialog {
    // A run of text in one colour.
    struct Phrase {
        Phrase() {}
        Phrase(std::wstring text, plasma::Vector<4, float> color, int shake)
            : text(text),
              color(color),
              shake(shake)
        {}

        std::wstring text;              // Text node supplied by SpeechWidget.
        plasma::Vector<4, float> color; // RGBA text colour used by BaseWidget.
        unsigned char shake;
    };

    // Initialises takenCount and givenCount to zero and unused188 to -1.
    Dialog()
        : takenCount(0),
          givenCount(0),
          unused188(-1)
    {}

    // Appends blank-separated words as coloured phrases. Joins apostrophes to adjacent phrases; a final "=" joins the
    // next word to the previous phrase. Defined in Speech.cpp.
    void write(std::wstring text, const plasma::Vector<4, float> &color, int shake);

    std::list<Phrase> phrases;
    // A narrow string: the script command that opened the dialog when it ends in '?', or the one character of a text
    // id above 10000.
    std::string answer; // Text stored with the dialog.
    int takenCount;
    Item takenItem; // Item carried by the dialog.
    // Script command 8 stores 1 here and its item in item; command 9 stores its int here and makes item category 12,
    // material 11.
    int givenCount;
    Item item;
    // What the record described is not observable.
    struct ScriptRecord {
        unsigned char byte0;
        unsigned char byte1;
        plasma::Vector<2, int> ints0;
        long long value64;
        plasma::Vector<2, int> ints1;
    };

    std::list<ScriptRecord> scriptRecords;
    int unused188; // Initially -1; copied with the dialog but never read.
    // Two-coordinate cell keys used by the destination sets.
    std::list<std::array<int, 2>> revealedLoreCells;
    // The map cells that the script commands "mark_location" (two ints) and 12 (the cell of its "zone") append.
    std::list<std::array<int, 2>> markedLocations;
};

}
