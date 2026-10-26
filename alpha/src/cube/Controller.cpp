// Controller stores held keys and buttons for the window input handlers.
#include "cube/Controller.h"

#include <string.h>

namespace cube {

// Clears the input state.
Controller::Controller(IDirect3DDevice9 *device)
    : m_moveStick(0, 0),
      m_lookStick(0, 0),
      m_device(device)
{
    memset(m_keys, 0, sizeof(m_keys));
    memset(m_buttons, 0, sizeof(m_buttons));
    memset(m_inputs, 0, sizeof(m_inputs));
}

// The base mouse-move handler does nothing; GameController handles movement.
void Controller::handleMouseMove(float, float) {}

void Controller::handleButtonDown(int button)
{
    m_buttons[button] = true;
}

void Controller::handleButtonUp(int button)
{
    m_buttons[button] = false;
}

void Controller::handleKeyDown(unsigned char key)
{
    m_keys[key] = true;
}

void Controller::handleKeyUp(unsigned char key)
{
    m_keys[key] = false;
}

}
