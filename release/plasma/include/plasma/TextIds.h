#pragma once

namespace plasma {

// TEXT_WRAP applies to TextShape; legacy fonts do not test it.
enum TextFlags : int {
    TEXT_CENTER = 0, // horizontally centred
    TEXT_RIGHT = 1,  // right-aligned
    TEXT_MIDDLE = 2, // vertically centred
    TEXT_BOTTOM = 3,
    TEXT_WRAP = 4 // plasma: "text:wrap", wrapped at TextShape's wrap width
};

enum TextHintingFlags {
    TEXT_HINTED = 0 // measured and drawn hinted
};

}
