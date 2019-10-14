#include "App.hpp"

#import <CoreFoundation/CoreFoundation.h>

#include "GfxTest.hpp"

class AppleOpenGL : public OpenGL {
public:
  CFAbsoluteTime currentTime;

  AppleOpenGL() : currentTime(CFAbsoluteTimeGetCurrent()) {}

  double getDeltaTime() override {
    auto now{CFAbsoluteTimeGetCurrent()};
    auto delta{now - currentTime};
    currentTime = now;
    return delta;
  }
};

#if PLATFORM_MACOS
#import <AppKit/AppKit.h>

class MacOSOpenGL : public AppleOpenGL {
public:
  NSOpenGLPixelFormat* format;
  NSOpenGLContext*     context;

  void clearCurrent() override {
    [NSOpenGLContext clearCurrentContext];
  }

  void makeCurrent() override {
    [context makeCurrentContext];
  }

  void present() override {
    [context flushBuffer];
  }
};

static CVReturn displayLinkCallback(CVDisplayLinkRef   displayLink UNUSED,
                                    CVTimeStamp const* now         UNUSED,
                                    CVTimeStamp const* outputTime  UNUSED,
                                    CVOptionFlags      flagsIn     UNUSED,
                                    CVOptionFlags*     flagsOut    UNUSED,
                                    void*              context     UNUSED)
{
  //LOG(App, Info, "DisplayLink Callback (time=%lu)", (outputTime->hostTime - now->hostTime)/1000000);
  auto ctx{reinterpret_cast<MacOSOpenGL*>(context)};
  ctx->presentReady.wait(); // TODO timeout to support frame skips
  LOG(App, Info, "PRESENT READY");
  [ctx->context flushBuffer];
  ctx->renderReady.set();
  return kCVReturnSuccess;
}


// OpenGL Layer
// -----------------------------------------------------------------------------

#if 0
@interface OpenGLLayer : NSOpenGLLayer {
@private
  OpenGL* gl;
}
@end

@implementation OpenGLLayer

- (instancetype)initWithGL:(OpenGL*)gl {
  if (self = [super init]) {
    self->gl = gl;

    self.needsDisplayOnBoundsChange = YES;
    self.asynchronous = YES;
  }
  return self;
}

- (NSOpenGLPixelFormat*)openGLPixelFormatForDisplayMask:(u32)UNUSED mask {
  return gl->format;
}

- (NSOpenGLContext*)openGLContextForPixelFormat:(NSOpenGLPixelFormat*)format {
  ASSERT(format == gl->format);
  return gl->context;
}

- (BOOL)canDrawInOpenGLContext:(NSOpenGLContext*)context
                   pixelFormat:(NSOpenGLPixelFormat*)format
                  forLayerTime:(CFTimeInterval)t
                   displayTime:(CVTimeStamp const*)ts
{
  ASSERT(context == gl->context);
  ASSERT(format == gl->format);
  auto canDraw{[super canDrawInOpenGLContext:context
                                 pixelFormat:format
                                forLayerTime:t
                                 displayTime:ts]};
  if (canDraw) {
    // TODO LOCK
    return YES;
  }
  return NO;
}

- (void)drawInOpenGLContext:(NSOpenGLContext*)UNUSED context
                pixelFormat:(NSOpenGLPixelFormat*)UNUSED format
               forLayerTime:(CFTimeInterval)UNUSED t
                displayTime:(CVTimeStamp const*)UNUSED ts
{
  glClearColor(.4, .3, .2, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  [super drawInOpenGLContext:context
                 pixelFormat:format
                forLayerTime:t
                 displayTime:ts];
}

@end
#endif


// OpenGL View
// -----------------------------------------------------------------------------

@interface OpenGLView : NSView {
@private
  MacOSOpenGL* gl;
}
@end

@implementation OpenGLView

- (instancetype)initWithFrame:(NSRect)frameRect GL:(MacOSOpenGL*)gl {
  if (self = [super initWithFrame:frameRect]) {
    self->gl  = gl;

#if 1
    [gl->context setView:self];
#else
    self.wantsBestResolutionOpenGLSurface = YES;
    self.wantsLayer = YES;

    auto swap{1};
    [gl->context setValues:&swap forParameter:NSOpenGLCPSwapInterval];
#endif

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(needsUpdate:)
                                                 name:NSViewGlobalFrameDidChangeNotification
                                               object:self];
  }
  return self;
}

#if 0
- (CALayer*)makeBackingLayer {
  return [[OpenGLLayer alloc] initWithGL:gl];
}
#endif

- (void)needsUpdate:(NSNotification*)UNUSED notification {
  LOG(App, Debug, "Needs Update");
  //[self update];
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];

  self.layer.contentsScale = self.window.backingScaleFactor;

  LOG(App, Debug, "View did change backing properties (scaleFactor=%f)", self.layer.contentsScale);
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  LOG(App, Debug, "View did move to window");
}

- (BOOL)isOpaque {
  return YES;
}

- (BOOL)canBecomeKeyView {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)keyDown:(NSEvent*)event {

}

- (void)keyUp:(NSEvent*)event {

}

- (void)flagsChanged:(NSEvent*)event {

}

- (void)mouseDown:(NSEvent*)event {

}

- (void)mouseUp:(NSEvent*)event {

}

- (void)mouseMoved:(NSEvent*)event {

}

- (void)mouseDragged:(NSEvent*)event {

}

- (void)scrollWheel:(NSEvent*)event {

}

@end

@interface AppDelegate : NSObject<NSApplicationDelegate> {
@private
  MacOSOpenGL gl;
  NSWindow*   window;
  NSView*     view;
}
@end

@implementation AppDelegate : NSObject

- (id)init {
  if (self = [super init]) {
    auto aboutItem{[[NSMenuItem alloc] initWithTitle:@"About Jank"
                                              action:@selector(showAbout:)
                                       keyEquivalent:@""]};
    auto prefItem{[[NSMenuItem alloc] initWithTitle:@"Preferences..."
                                             action:@selector(showPreferences:)
                                      keyEquivalent:@","]};
    auto quitItem{[[NSMenuItem alloc] initWithTitle:@"Quit"
                                             action:@selector(terminate:)
                                      keyEquivalent:@"q"]};

    auto appMenu{[NSMenu new]};
    [appMenu addItem:aboutItem];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItem:prefItem];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItem:quitItem];

    auto appItem{[NSMenuItem new]};
    [appItem setSubmenu:appMenu];

    auto mainMenu{[NSMenu new]};
    [mainMenu addItem:appItem];
    [NSApp setMainMenu:mainMenu];
  }
  return self;
}

- (void)showAbout:(id)UNUSED sender {
  LOG(App, Info, "Show About");
}

- (void)showPreferences:(id)UNUSED sender {
  LOG(App, Info, "Show Preferences");
}

- (void)applicationWillFinishLaunching:(NSNotification*)UNUSED notification {
  NSOpenGLPixelFormatAttribute attrs[]{
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
    NSOpenGLPFAColorSize,     24,
    NSOpenGLPFAAlphaSize,     8,
    NSOpenGLPFADepthSize,     0,
    NSOpenGLPFAStencilSize,   0,
    NSOpenGLPFASampleBuffers, 0,
    0
  };

  gl.format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  ASSERT(gl.format, "Failed to create pixel format");

  gl.context = [[NSOpenGLContext alloc] initWithFormat:gl.format shareContext:nil];

  auto styleMask{
    NSTitledWindowMask |
    NSResizableWindowMask |
    NSClosableWindowMask |
    NSMiniaturizableWindowMask
  };

  gl.width = 800;
  gl.height = 600;
  auto frame{NSMakeRect(100, 100, gl.width, gl.height)};
  auto glView{[[OpenGLView alloc] initWithFrame:frame GL:&gl]};
  self->view   = glView;
  self->window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];

  [self->window setTitle:@"Jank"];
  [self->window setContentView:view];
  [self->window makeFirstResponder:view];
  [self->window center];

#if 0
  auto window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];

  auto view = [[OpenGLView alloc] initWithFrame:frame format:format shareContext:glView.context];
  [window setTitle:@"Jank"];
  [window setContentView:view];
  [window makeFirstResponder:view];
  [window makeKeyAndOrderFront:self];
#endif

#if 0
  CVDisplayLinkRef displayLink;
  // TODO option to lock link to a single display,
  //      then call CVDisplayLinkGetCurrentCGDisplay once.
  //CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
  CVDisplayLinkCreateWithCGDisplay(CGMainDisplayID(), &displayLink);
  CVDisplayLinkSetOutputCallback(displayLink, displayLinkCallback, &gl);
  //CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink,
  //                                                  gl.context.CGLContextObj,
  //                                                  gl.format.CGLPixelFormatObj);
  CVDisplayLinkStart(displayLink);
  //CVDisplayLinkStop(displayLink);
  //CVDisplayLinkRelease(displayLink);
#endif

  pthread_t renderThread;
  pthread_create(&renderThread, nullptr, renderMain, &gl);
}

- (void)applicationDidFinishLaunching:(NSNotification*)UNUSED notification {
  [window makeKeyAndOrderFront:self];
}

- (void)applicationWillBecomeActive:(NSNotification*)UNUSED notification {

}

- (void)applicationDidBecomeActive:(NSNotification*)UNUSED notification {

}

- (void)applicationWillResignActive:(NSNotification*)UNUSED notification {

}

- (void)applicationDidResignActive:(NSNotification*)UNUSED notification {

}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)UNUSED sender {
  return NSTerminateNow;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)UNUSED sender {
  return YES;
}

- (void)applicationWillTerminate:(NSNotification*)UNUSED notification {

}

@end

#else

#import <UIKit/UIKit.h>

@class EAGLView;

class IphoneOpenGL : public AppleOpenGL {
public:
  EAGLContext* context;

  void clearCurrent() override {
    [EAGLContext setCurrentContext:nil];
  }

  void makeCurrent() override {
    [EAGLContext setCurrentContext:context];
  }
};

@interface EAGLView : UIView {
@private
  IphoneOpenGL   gl;
  CADisplayLink* displayLink;
}

@end

@implementation EAGLView

+ (Class)layerClass {
  return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    gl.width  = static_cast<u32>(frame.size.width);
    gl.height = static_cast<u32>(frame.size.height);

    auto eaglLayer{static_cast<CAEAGLLayer*>(self.layer)};
    eaglLayer.opaque = YES;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8,        kEAGLDrawablePropertyColorFormat,
                                    nil];

    gl.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    ASSERT(gl.context, "Failed to create GLES3 context");

  }
  return self;
}

- (void)displayLinkCallback {
  LOG(App, Info, "Display link!");
  gl.presentReady.wait();
  //[context presentRenderbuffer:];
  gl.renderReady.set();
}

- (void)pause {

}

- (void)resume {
  // TODO only pause/resume here
  pthread_t renderThread;
  pthread_create(&renderThread, nullptr, renderMain, &gl);

  displayLink = [self.window.screen displayLinkWithTarget:self selector:@selector(displayLinkCallback)];
  [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

@end

@interface ViewController : UIViewController
@end

@implementation ViewController
@end

@interface AppDelegate : UIResponder<UIApplicationDelegate> {
@private
  UIWindow* window;
  EAGLView* view;
  ViewController* viewController;
}
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication*)UNUSED application
willFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id>*)UNUSED launchOptions
{
  LOG(App, Info, "willFinishLaunching");
  return YES;
}

- (BOOL)application:(UIApplication*)UNUSED application
didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id>*)UNUSED launchOptions
{
  LOG(App, Info, "didFinishLaunching");
  auto mainFrame{[[UIScreen mainScreen] bounds]};
  window = [[UIWindow alloc] initWithFrame:mainFrame];
  view   = [[EAGLView alloc] initWithFrame:mainFrame];

  viewController = [ViewController new];
  [window setRootViewController:viewController];

  [window addSubview:view];
  [window makeKeyAndVisible];

  return YES;
}
#if 0
- (UISceneConfiguration*)application:(UIApplication*)UNUSED application
configurationForConnectingSceneSession:(nonnull UISceneSession*)UNUSED connectingSceneSession
                             options:(nonnull UISceneConnectionOptions*)UNUSED options
{
  LOG(App, Info, "configurationForConnectingSceneSession");
  return nil;
}

- (void)application:(UIApplication*)UNUSED application
didDiscardSceneSessions:(NSSet<UISceneSession*>*)UNUSED sceneSessions
{
  LOG(App, Info, "didDiscardSceneSessions");
}
#endif

- (void)applicationDidBecomeActive:(UIApplication*)UNUSED application {
  LOG(App, Info, "didBecomeActive");
  [view resume];
}

- (void)applicationWillResignActive:(UIApplication*)UNUSED application {
  LOG(App, Info, "willBecomeActive");
}

- (void)applicationDidEnterBackground:(UIApplication*)UNUSED application {
  LOG(App, Info, "didEnterBackground");
}

- (void)applicationWillEnterForeground:(UIApplication*)UNUSED application {
  LOG(App, Info, "willEnterForeground");
}

- (void)applicationWillTerminate:(UIApplication*)UNUSED application {
  LOG(App, Info, "willTerminate");
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)UNUSED application {
  LOG(App, Info, "didReceiveMemoryWarning");
}

- (void)applicationSignificantTimeChange:(UIApplication*)UNUSED application {
  LOG(App, Info, "significantTimeChange");
}

- (BOOL)application:(UIApplication*)UNUSED app
            openURL:(NSURL*)UNUSED url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id>*)UNUSED options
{
  LOG(App, Info, "openURL");
  return NO;
}

@end

#endif

i32 main(i32 argc UNUSED, char* argv UNUSED[]) {
  LOG(App, Info, "Hello World %u!", 42);
  pthread_setname_np("Main");

#if PLATFORM_MACOS
  [NSApplication sharedApplication];
  [NSApp setDelegate:[[AppDelegate alloc] init]];
  [NSApp run];
  return 0;
#else
  NSString* appDelegateClassName;
  @autoreleasepool {
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
  }
  return UIApplicationMain(argc, argv, nil, appDelegateClassName);
#endif
}
