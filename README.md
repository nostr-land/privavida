PrivaVida
=========
### A privacy-focused messaging app built on Nostr
:warning: WORK IN PROGRESS

PrivaVida is written in C++ and is intended to be as portable as possible.
Anywhere where you have the C++17 runtime, where you can render graphics to
a GPU surface, and where you can make HTTP requests, you should be able
to run PrivaVida.

Currently, iOS and Web (Emscripten) are supported targets.

- [`privavida-core`](src/) contains the guts of the app.
- [`privavida-ios`](ios/) contains the iOS bindings.
- [`privavida-web`](emscripten/) contains the web (Emscripten) bindings.

Essentially a playground for experimenting with building a native C++
iOS app that renders to the screen using
[NanoVG](https://github.com/memononen/nanovg) + Metal.

To port to other platforms, you need to find a way to run a NanoVG
instance (which has bindings for Metal, OpenGL, and Direct3D). Once you've
got that you can call the app code using [this](src/app.h) C interface.

## Build iOS

```bash
mkdir build
cd build
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios/ios.toolchain.cmake
```

This will prepare the `privavida-core` XCode project. Once complete
you can open `ios/privavida-ios.xcodeproject` and you should be able
to compile and run the iOS app.

## Build Web (Emscripten)

```bash
cd .
sh emscripten/build.sh
```

This will produce an `index.js` and an `index.wasm` file. You can
access the app by serving these files + `index.html` on a basic
web server.

## `app.h` Interface

To port PrivaVida to other platforms, you need to find a way to get
NanoVG working on your desired platform (bindings exist that work with
Metal, OpenGL, Direct3D, and many others). Once you've got a way to
get graphics to work, you can interface with `privavida-core` by
calling the functions described in [`app.h`](src/app.h).

Currently, the interface is as follows:

```c
void app_init(NVGcontext* vg, AppKeyboard keyboard, AppStorage storage);
int  app_wants_to_render(void);
void app_render(float window_width, float window_height, float pixel_density);
void app_touch_event(AppTouchEvent* event);
void app_scroll_event(int x, int y, int dx, int dy);
void app_key_backspace(void);
void app_key_character(const char* ch);
```

Call `app_init()` once to start, passing in a NanoVG context, and keyboard
and storage drivers. Then, you can call `app_render()` in your main UI thread
to render the UI. You don't need to render a new frame if `app_wants_to_render()`
returns false. Finally, you can send user input events with `app_touch_event()`,
`app_scroll_event()`, `app_key_backspace()` and `app_key_character()`.
