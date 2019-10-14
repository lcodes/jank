#include "App.hpp"

#if PLATFORM_ANDROID

#include "GfxTest.hpp"

#include <android_native_app_glue.h>
#include <android/window.h>

#include <pthread.h>
#include <time.h>

#include <EGL/egl.h>

static double getTime() {
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_nsec) / 1E9;
}

static void* presentMain(void*);

class AndroidOpenGL : public OpenGL {
public:
  EGLDisplay display;
  EGLSurface surface;
  EGLContext context;

  double currentTime;

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
    pthread_create(&present, nullptr, &presentMain, this);
  }

  void clearCurrent() override {
    auto result{eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)};
    ASSERT(result);
  }

  void makeCurrent() override {
    auto result{eglMakeCurrent(display, surface, surface, context)};
    ASSERT(result);
  }

  double getDeltaTime() override {
    auto now{getTime()};
    auto delta{now - currentTime};
    currentTime = now;
    return delta;
  }
};

static void* presentMain(void* arg) {
  auto gl{reinterpret_cast<AndroidOpenGL*>(arg)};
  while (true) {
    gl->presentReady.wait();
    gl->makeCurrent();
    auto result{eglSwapBuffers(gl->display, gl->surface)};
    ASSERT(result);
    gl->clearCurrent();
    gl->renderReady.set();
  }
}

static AndroidOpenGL gl;

static void jank_android_onAppCmd(android_app* app, i32 cmd) {
  switch (cmd) {
  case APP_CMD_INPUT_CHANGED:
    LOG(App, Info, "Input Changed");
    break;
  case APP_CMD_INIT_WINDOW:
    LOG(App, Info, "Init Window");
    if (!gl.context) gl.init(app);
    break;
  case APP_CMD_TERM_WINDOW:
    LOG(App, Info, "Term Window");
    break;
  case APP_CMD_WINDOW_RESIZED:
    LOG(App, Info, "Window Resized");
    break;
  case APP_CMD_WINDOW_REDRAW_NEEDED:
    LOG(App, Info, "Window Redraw Needed");
    break;
  case APP_CMD_CONTENT_RECT_CHANGED:
    LOG(App, Info, "Content Rect Changed");
    break;
  case APP_CMD_GAINED_FOCUS:
    LOG(App, Info, "Gained Focus");
    break;
  case APP_CMD_LOST_FOCUS:
    LOG(App, Info, "Lost Focus");
    break;
  case APP_CMD_CONFIG_CHANGED:
    LOG(App, Info, "Config Changed");
    break;
  case APP_CMD_LOW_MEMORY:
    LOG(App, Info, "Low Memory");
    break;
  case APP_CMD_START:
    LOG(App, Info, "Start");
    break;
  case APP_CMD_RESUME:
    LOG(App, Info, "Resume");
    break;
  case APP_CMD_SAVE_STATE:
    LOG(App, Info, "Save State");
    break;
  case APP_CMD_PAUSE:
    LOG(App, Info, "Pause");
    break;
  case APP_CMD_STOP:
    LOG(App, Info, "Stop");
    break;
  case APP_CMD_DESTROY:
    LOG(App, Info, "Destroy");
    break;
  default:
    break;
  }
}

static i32 jank_android_onInputEvent(android_app* app UNUSED, AInputEvent* event) {
  switch (AInputEvent_getType(event)) {
  case AINPUT_EVENT_TYPE_KEY:
    AKeyEvent_getKeyCode(event);
    return true;

  case AINPUT_EVENT_TYPE_MOTION:
    // auto pointerCount{AMotionEvent_getPointerCount(event)};
    AMotionEvent_getX(event, 0);
    AMotionEvent_getY(event, 0);
    return true;

  default:
    return false;
  }
}

void android_main(android_app* app) {
  auto windowFlags{
    AWINDOW_FLAG_FULLSCREEN |
    AWINDOW_FLAG_KEEP_SCREEN_ON
  };
  ANativeActivity_setWindowFlags(app->activity, windowFlags, 0);

  {
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

  app->userData     = nullptr; // TODO
  app->onAppCmd     = jank_android_onAppCmd;
  app->onInputEvent = jank_android_onInputEvent;

  android_poll_source* source;
  i32 result;

  while (true) {
    while ((result = ALooper_pollAll(0, nullptr, nullptr, reinterpret_cast<void**>(&source))) > 0) {
      if (source) {
        source->process(app, source);
      }
    }

    // assert(result == 0);

    if (app->destroyRequested) {
      break;
    }

    // Run
  }

  // Terminate
}

#endif
