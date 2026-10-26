#pragma once

namespace cube {

// HSV hue sector identifiers.
enum HueSector : int {
    HueSectorRedToYellow = 0,
    HueSectorYellowToGreen = 1,
    HueSectorGreenToCyan = 2,
    HueSectorCyanToBlue = 3,
    HueSectorBlueToMagenta = 4,
    HueSectorMagentaToRed = 5,
};

}
