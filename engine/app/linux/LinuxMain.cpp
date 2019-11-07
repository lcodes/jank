#include "app/App.hpp"
#include "app/GfxTest.hpp"

#include <GL/glx.h>
#include <X11/Xlib-xcb.h>

#include <stdlib.h>

class LinuxOpenGL : public OpenGL {
public:
  Display*    display;
  GLXDrawable drawable;
  GLXContext  context;

  void* getProcAddress(char const* name) override {
    return reinterpret_cast<void*>(glXGetProcAddress(reinterpret_cast<GLubyte const*>(name)));
  }

  void clearCurrent() override {
    auto result{ glXMakeContextCurrent(display, 0, 0, None) };
    ASSERT(result);
  }

  void makeCurrent() override {
    auto result{ glXMakeContextCurrent(display, drawable, drawable, context) };
    ASSERT(result);
  }

  void present() override {
    glXSwapBuffers(display, drawable);
  }

  f64 getDeltaTime() override {
    return .016;
  }
};

static void* presentMain(void* arg) {
  auto gl{ reinterpret_cast<LinuxOpenGL*>(arg) };
  gl->makeCurrent();
  while (true) {
#if GFX_PRESENT_THREAD
    gl->presentReady.wait();
    gl->present();
    gl->renderReady.set();
#else
    renderMain(arg);
#endif
  }
  return nullptr;
}

static i32 visualAttrs[]{
  GLX_X_RENDERABLE, True,
  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
  GLX_RENDER_TYPE, GLX_RGBA_BIT,
  GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
  GLX_RED_SIZE, 8,
  GLX_GREEN_SIZE, 8,
  GLX_BLUE_SIZE, 8,
  GLX_ALPHA_SIZE, 0,
  GLX_DEPTH_SIZE, 0,
  GLX_STENCIL_SIZE, 0,
  GLX_DOUBLEBUFFER, True,
  None
};

class LinuxApp : public App {
public:
  Display display;
  xcbConnection conn;

  LinuxApp();
  ~LinuxApp();

  void main();
};

LinuxApp::LinuxApp() {
  display = okC(XOpenDisplay(nullptr));
  conn = okC(XGetXCBConnection(display));

  XSetEventQueueOwner(display, XCBOwnsEventQueue);
}

LinuxApp::~LinuxApp() {
  XCloseDisplay(display);
}

void LinuxApp::main() {
  init();

  auto running{ true };
  while (running) {
    auto event{ xcb_wait_for_event(conn) };
    ASSERT(event);

    switch (event->response_type & ~0x80) {
    case XCB_KEY_PRESS:
      running = false;
      break;

    case XCB_EXPOSE:
      break;
    }

    free(event);
  }

  term();
}

void App::runOnMainThread(Fn&& fn) {

}

void App::quit() {

}

i32 main(i32 argc UNUSED, char* argv UNUSED[]) {
  LinuxApp app;

  LinuxOpenGL gl;
  gl.width = 800;
  gl.height = 600;
  gl.display = app.display;

  auto defaultScreen{ DefaultScreen(gl.display) };

  xcb_screen_t* screen;
  {
    auto iter{ xcb_setup_roots_iterator(xcb_get_setup(conn)) };
    for (auto n{ defaultScreen }; iter.rem && n > 0; xcb_screen_next(&iter), n--);
    screen = iter.data;
  }

  i32 numConfigs;
  auto configs{ glXChooseFBConfig(gl.display, defaultScreen, visualAttrs, &numConfigs) };
  ASSERT(configs && numConfigs);

  i32 visualId;
  glXGetFBConfigAttrib(gl.display, configs[0], GLX_VISUAL_ID, &visualId);

  gl.context = glXCreateNewContext(gl.display, configs[0], GLX_RGBA_TYPE, 0, True);
  ASSERT(gl.context);

  auto colormap{ xcb_generate_id(conn) };
  auto window  { xcb_generate_id(conn) };

  xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, visualId);

  auto valueMask{ XCB_CW_EVENT_MASK | XCB_CW_COLORMAP };
  u32 valueList[]{
    XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS,
    colormap,
    0
  };
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, gl.width, gl.height,
                    0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visualId, valueMask, valueList);

  xcb_map_window(conn, window);

  gl.drawable = glXCreateWindow(gl.display, configs[0], window, 0);
  ASSERT(gl.drawable);

  pthread_t render, present;
#if GFX_PRESENT_THREAD
  pthread_create(&render, nullptr, &renderMain, &gl);
#endif
  pthread_create(&present, nullptr, &presentMain, &gl);

  app.main();

  glXDestroyWindow(gl.display, gl.drawable);
  xcb_destroy_window(conn, window);
  glXDestroyContext(gl.display, gl.context);

  return 0;
}
