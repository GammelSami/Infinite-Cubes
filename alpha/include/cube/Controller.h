// Controller receives window input events and records held keys and mouse buttons. GameController handles the game-specific events.
#pragma once
#include "plasma/Vector.h"

struct IDirect3DDevice9;

namespace cube {

// Indices of Controller::m_inputs, from the DirectInput states WinMain copies : mouse buttons and
// keyboard scan codes. Names are the keys'; their game meaning is GameController's.
enum ControllerInput {
    InputMouseLeft = 0,
    InputMouseRight = 1,
    InputKey1 = 2, // keys 1 to 4
    InputKey2 = 3,
    InputKey3 = 4,
    InputKey4 = 5,
    InputMouseMiddle = 6,
    InputKeyW = 7,
    InputKeyS = 8,
    InputKeyA = 9,
    InputKeyD = 10,
    InputKeyR = 11,
    InputKeyT = 12,
    InputKeyE = 13,
    InputKeySpace = 14,
    InputKeyControl = 15, // left control
    InputKeyShift = 16,   // left shift
    InputKeyShift2 = 17,  // left shift again (stored after the mouse)
    InputTargetLock = 18, // not stored
    InputKeyQ = 19,
    InputTeleportToCity = 20, // update starts ability 105; WinMain does not map a key
};
// Number of entries, not an ID.
enum {
    InputCount = 21
};

class Controller {
public:
    explicit Controller(IDirect3DDevice9 *device);
    virtual ~Controller() {}                        // Destructor.
    virtual bool isCursorVisible() { return true; } // GameController overrides this event.
    virtual void handleKeyDown(unsigned char key);
    virtual void handleKeyUp(unsigned char key);
    virtual void resize(int width, int height) {}
    virtual void onKeyTyped(char character) {}
    virtual void handleMouseMove(float x, float y);
    virtual void handleButtonDown(int button);
    virtual void handleButtonUp(int button);
    virtual void handleMouseWheel(int delta) {}
    virtual void update(int milliseconds) {}
    virtual void render() {}

    // Keeps the window's client size and reports it through resize.
    void setWindowSize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;
        resize(width, height);
    }

    // The input states WinMain stores each frame from DirectInput (the mouse buttons and the keys of
    // ControllerInput); zeroed by the constructor.
    bool m_inputs[InputCount];
    bool m_keys[0x100]; // held keys, by key code
    bool m_buttons[3];  // held mouse buttons
    int m_windowWidth;  // the client size (setWindowSize)
    int m_windowHeight;
    plasma::Vector<2, int> m_moveStick; // a movement stick (-32768..32767 per axis; WinMain zeroes it each frame)
    plasma::Vector<2, int>
        m_lookStick; // (0, 0); a second stick: GameController::update turns the camera with it like the mouse
    IDirect3DDevice9 *m_device; // the constructor's argument (GameController passes its device)
};

}
