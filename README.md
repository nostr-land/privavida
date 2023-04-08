PrivaVida iOS
=============

An iOS app for privavida-ios.

Essentially a playground for experimenting with building a native C++
iOS app that renders to the screen using NanoVG + Metal.

This is very unusual. Since the bulk of the app is written in C++ with
a very limited set of bindings to the OS, porting this app natively to
other operating systems won't be too difficult.

# Build

```bash
mkdir build
cd build
cmake .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios/ios.toolchain.cmake -DPLATFORM=OS64
```
