PrivaVida iOS
=============

An iOS app for PrivaVida.

Essentially a playground for experimenting with building a native C++
iOS app that renders to the screen using NanoVG + Metal.

This is a very unusual iOS app architecture, but might be interesting
to some. Since the bulk of the app is written in C++ with a very
limited set of bindings to the OS, porting this app to other platforms
is not too difficult.

Any platform that can run NanoVG (which has bindings for Metal, OpenGL,
and Direct3D) and can call the app code with [this](src/app.h) interface
can run the app.

# Build

```bash
mkdir build
cd build
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios/ios.toolchain.cmake -DPLATFORM=OS64 -DENABLE_BITCODE=0
```
