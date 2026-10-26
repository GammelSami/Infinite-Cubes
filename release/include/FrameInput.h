// The per-frame keyboard, mouse and gamepad handling shared by the Windows and Linux entry points (src/FrameInput.cpp).
#pragma once
#include <dinput.h>

namespace cube {
class Game;
}
namespace plasma {
class D3D11Engine;
}

// Moves the system cursor to a point of the window's client area; each entry point implements it.
void setPointerInClient(int x, int y);
// While a control is being assigned: the first key newly pressed becomes the binding (Escape and the Enter keys only
// end the assignment), as does a newly pressed mouse button.
void assignControlFromDevices(cube::Game *game, const unsigned char keys[256], const DIMOUSESTATE &mouseState,
                              const unsigned char previousKeys[256], const DIMOUSESTATE &previousMouseState);
// Sets every control from its keyboard or mouse binding.
void applyDeviceBindings(cube::Game *game, const unsigned char keys[256], const DIMOUSESTATE &mouseState);
// Reads the first gamepad (XInputGetState) and drives the controls, the menu pointer and the panels with it.
void applyGamepad(cube::Game *game, plasma::D3D11Engine *engine);
