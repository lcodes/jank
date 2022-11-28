#pragma once

#include "core/CoreTypes.hpp"

namespace App::Input {

// Intro
// - Debug UI
// - Any Key -> Skip

// Menu
// - Debug UI
// - Navigation
// - Change Values
// - Ok / Cancel

// Game
// - Debug UI
// - Game controls (pause menu, panel toggles, console trigger, ...)
// - Camera controls
// - Character controls

// Advanced
// - Button sequences (ie konami code)
// - Hold button
// - Dead zone


// Device -> Controls -> Controller

// Devices
// - User (player 1, player 2, remote, etc)
// - Name
// - GUID
// - Mapping

// Controls
// - Button
// - Axis
// - Stick

// Action
// - Source
// - Condition
// - Trigger

class Control : NonCopyable {

};

class Button : public Control {

};

class Axis : public Control {

};

class Stick : public Control {
  Axis x;
  Axis y;
};

class Device : NonCopyable {

};

class Touch : public Device {
  Button button;
  Stick position;
  Stick delta;
};

class Mouse : public Device {
  Button buttons[5];
  Axis   scroll[2];
  Stick  delta;
};

class Keyboard : public Device {
  Button keys[128];
  // key states
  // key press times
  // keys pressed this frame
  // keys released this frame
};

class GamePad : public Device {
  Button buttons[15];
  Stick sticks[2];
  Axis shoulders;
  // axes values
  // axes last frame values
  // button states
  // buttons pressed this frame
  // buttons released this frame
};

void init();
void term();
void run();

} // namespace App::Input

