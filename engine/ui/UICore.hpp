#pragma once

#include <imgui.h>

namespace App {
class Main;
}

class UI {
  friend class App::Main;

  UI() = delete;

  static void init();
  static void term();

  static void initRender();
  static void termRender();

  static void run();
  static void render();
};
