#include "app/Input.hpp"

namespace App::Windows::Input {

// - XInput (limited to 4 gamepads, very basic)
// - DirectInput (deprecated, but supports more than 4 gamepads and force feedback)
// - RAWINPUT (high precision mouse, but tricky gamepads)
// - WM_* (legacy mouse/keyboard, but has touch)

// Touch
// - WM_TOUCH

// Keyboard
// - WM_INPUT

// Mouse
// - WM_* buttons
// - WM_* for absolute positions in screen pixels
// - WM_INPUT for high precision relative movement

// Gamepad
// - XInput is just sugar over RAWINPUT
// - DirectInput is deprecated
// - WM_INPUT is the solution!
//   - tricky force feedback?
//   - tricky button/axis mappings?

// Already have dedicated main thread pumping the WM message queue
// - Just need to accumulate, and sync once per logical frame.

} // namespace App::Windows::Input

void App::Input::init() {

}

void App::Input::term() {

}

void App::Input::run() {

}

