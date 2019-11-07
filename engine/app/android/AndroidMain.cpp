#include "app/App.hpp"

#include "app/GfxTest.hpp"

#include <android_native_app_glue.h>
#include <android/window.h>

#include <pthread.h>
#include <time.h>

#include <EGL/egl.h>

DECL_LOG_SOURCE(Input, Info);
class AndroidController {

};

static double getTime() {
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_nsec) / 1E9;
}

#if GFX_PRESENT_THREAD
static void* presentMain(void*);
#endif

class AndroidOpenGL : public OpenGL {
public:
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;

  f64 currentTime;

  void init(android_app* app) {
    currentTime = getTime();

    width  = ANativeWindow_getWidth(app->window);
    height = ANativeWindow_getHeight(app->window);

    {
      auto config{AConfiguration_new()};
      AConfiguration_fromAssetManager(config, app->activity->assetManager);
      dpi = AConfiguration_getDensity(config);
      AConfiguration_delete(config);
      dpi = 1;
    }

    LOG(App, Info, "%fx%f @ %fx", width, height, dpi);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ASSERT(display != EGL_NO_DISPLAY);
    eglInitialize(display, 0, 0);

    EGLint configAttrs[]{
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
      EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, configAttrs, &config, 1, &numConfigs);

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, app->window, nullptr);
    ASSERT(surface != EGL_NO_SURFACE);

    EGLint contextAttrs[]{
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
    };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
    ASSERT(context != EGL_NO_CONTEXT);

    pthread_t render, present;
    pthread_create(&render,  nullptr, &renderMain,  this);
#if GFX_PRESENT_THREAD
    pthread_create(&present, nullptr, &presentMain, this);
#endif
  }

  void present() override {
    auto result{eglSwapBuffers(display, surface)};
    ASSERT(result);
  }

  void clearCurrent() override {
    auto result{eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)};
    ASSERT(result);
  }

  void makeCurrent() override {
    auto result{eglMakeCurrent(display, surface, surface, context)};
    ASSERT(result);
  }

  f64 getDeltaTime() override {
    auto now{getTime()};
    auto delta{now - currentTime};
    currentTime = now;
    return delta;
  }
};

#if GFX_PRESENT_THREAD
static void* presentMain(void* arg) {
  auto gl{reinterpret_cast<AndroidOpenGL*>(arg)};
  while (true) {
    gl->presentReady.wait();
    gl->makeCurrent();
    gl->present();
    gl->clearCurrent();
    gl->renderReady.set();
  }
}
#endif

static AndroidOpenGL gl;

class AndroidApp : public App {
public:
  android_app* app;

  AndroidApp(android_app* app) : app(app) {}

  static AndroidApp* get() { return static_cast<AndroidApp*>(App::get()); }

  static void onCmd(android_app* app, i32 cmd);
  static void onInput(android_app* app, AInputEvent* event);
};

void App::runOnMainThread(Fn&& fn) {
  // TODO
}

void App::quit() {
  AndroidApp::get()->app->destroyRequested = true;
}

void AndroidApp::onCmd(android_app* app, i32 cmd) {
  auto& main{ *reinterpret_cast<AndroidApp*>(app->userData) };

  switch (cmd) {
  case APP_CMD_INIT_WINDOW:
    LOG(App, Trace, "Init Window");
    if (!gl.context) gl.init(app);
    break;
  case APP_CMD_TERM_WINDOW:
    LOG(App, Trace, "Term Window");
    break;
  case APP_CMD_WINDOW_RESIZED:
    LOG(App, Trace, "Window Resized");
    break;
  case APP_CMD_WINDOW_REDRAW_NEEDED:
    LOG(App, Trace, "Window Redraw Needed");
    break;
  case APP_CMD_CONTENT_RECT_CHANGED:
    LOG(App, Trace, "Content Rect Changed");
    break;
  case APP_CMD_START:
    LOG(App, Trace, "Start");
    break;
  case APP_CMD_STOP:
    LOG(App, Trace, "Stop");
    break;
  case APP_CMD_DESTROY:
    LOG(App, Trace, "Destroy");
    break;
  case APP_CMD_PAUSE:
    LOG(App, Trace, "Pause");
    main.pause();
    break;
  case APP_CMD_RESUME:
    LOG(App, Trace, "Resume");
    main.resume();
    break;
  case APP_CMD_GAINED_FOCUS:
    LOG(App, Trace, "Gained Focus");
    main.gainFocus();
    break;
  case APP_CMD_LOST_FOCUS:
    LOG(App, Trace, "Lost Focus");
    main.lostFocus();
    break;
  case APP_CMD_LOW_MEMORY:
    LOG(App, Trace, "Low Memory");
    main.lowMemory();
    break;
  case APP_CMD_CONFIG_CHANGED:
    LOG(App, Trace, "Config Changed");
    break;
  case APP_CMD_INPUT_CHANGED:
    LOG(App, Trace, "Input Changed");
    break;
  case APP_CMD_SAVE_STATE:
    LOG(App, Trace, "Save State");
    break;
  default:
    LOG(App, Debug, "Unknown cmd: 0x%x", cmd);
    break;
  }
}

i32 AndroidApp::onInput(android_app* app, AInputEvent* event) {
  auto& main{ *reinterpret_cast<AndroidApp*>(app->userData) };

  LOG(Input, Trace, "Device ID: %i", AInputEvent_getDeviceId(event));
  auto src{ AInputEvent_getSource(event) };

  auto type{ AInputEvent_getType(event) };
  switch (type) {
  case AINPUT_EVENT_TYPE_KEY: {
    auto k = AKeyEvent_getKeyCode(event);
    LOG(App, Trace, "Input Key: %d", k);
    return true;
  }

  case AINPUT_EVENT_TYPE_MOTION: {
    // auto pointerCount{AMotionEvent_getPointerCount(event)};
    auto x = AMotionEvent_getX(event, 0);
    auto y = AMotionEvent_getY(event, 0);
    LOG(App, Trace, "Input Motion: %f %f", x, y);
    return true;
  }

  default:
    LOG(App, Debug, "Unknown event type: 0x%x", type);
    return false;
  }
}

void android_main(android_app* app) {
  AndroidApp main(app);
  app->userData     = &main;
  app->onAppCmd     = AndroidApp::onCmd;
  app->onInputEvent = AndroidApp::onInput;

  {
    auto windowFlags{
      AWINDOW_FLAG_FULLSCREEN |
      AWINDOW_FLAG_KEEP_SCREEN_ON
    };
    ANativeActivity_setWindowFlags(app->activity, windowFlags, 0);

    auto javaVM{app->activity->vm};
    auto jniEnv{app->activity->env};
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    args.name = "NativeMain";
    args.group = nullptr;
    javaVM->AttachCurrentThread(&jniEnv, &args);

    auto versionClass{jniEnv->FindClass("android/os/Build$VERSION")};
    auto sdkIntField {jniEnv->GetStaticFieldID(versionClass, "SDK_INT", "I")};
    auto apiLevel    {jniEnv->GetStaticIntField(versionClass, sdkIntField)};
    LOG(App, Info, "Android API Level: %d", apiLevel);

    auto activityObject{app->activity->clazz};
    auto activityClass {jniEnv->GetObjectClass(activityObject)};
    auto getWindow     {jniEnv->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;")};
    auto windowObject  {jniEnv->CallObjectMethod(activityObject, getWindow)};
    auto windowClass   {jniEnv->FindClass("android/view/Window")};
    auto getDecorView  {jniEnv->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;")};
    auto viewObject    {jniEnv->CallObjectMethod(windowObject, getDecorView)};
    auto viewClass     {jniEnv->FindClass("android/view/View")};

    jint flags{0};
    auto addFlag = [jniEnv, viewClass, &flags](char const* name) {
      auto field{jniEnv->GetStaticFieldID(viewClass, name, "I")};
      flags |= jniEnv->GetStaticIntField(viewClass, field);
    };

    if (apiLevel >= 14) {
      addFlag("SYSTEM_UI_FLAG_LOW_PROFILE");
      addFlag("SYSTEM_UI_FLAG_HIDE_NAVIGATION");
    }
    if (apiLevel >= 16) {
      addFlag("SYSTEM_UI_FLAG_FULLSCREEN");
    }
    if (apiLevel >= 19) {
      addFlag("SYSTEM_UI_FLAG_IMMERSIVE_STICKY");
    }

    auto setSystemUiVisibility{jniEnv->GetMethodID(viewClass, "setSystemUiVisibility", "(I)V")};
    jniEnv->CallVoidMethod(viewObject, setSystemUiVisibility, flags);
  }

  main.init();

  i32 result;
  android_poll_source* source;

  while ((result = ALooper_pollAll(-1, nullptr, nullptr, reinterpret_cast<void**>(&source))) > 0) {
    if (source) {
      source->process(app, source);
    }

    if (app->destroyRequested) {
      break;
    }

    // assert(result == 0);
  }

  main.term();
}
