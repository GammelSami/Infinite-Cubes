// Shared pointer-release handling for Edit.
#include "plasma/Edit.h"
#include "plasma/ListWidget.h"
#include "plasma/ScrollSlider.h"
#include "plasma/ScrollButton.h"
#include "plasma/PopUpButton.h"
#include "plasma/TextShape.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/FontEngine.h"
#include <algorithm>

namespace plasma {

void Edit::pointerReleased()
{
    m_engine->releasePointerCapture();
}

}
