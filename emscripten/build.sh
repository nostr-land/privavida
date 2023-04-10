emcc \
    emscripten/index.cpp \
    lib/nanovg/nanovg.c \
    src/app.cpp \
    src/gestures.cpp \
    src/ui.cpp \
    src/views/Root.cpp \
    src/views/ScrollView.cpp \
    -I include/ \
    -o emscripten/index.js \
    --embed-file ios/privavida-ios/Resources/SFBold.ttf \
    --embed-file ios/privavida-ios/Resources/SFRegular.ttf \
    -sEXPORTED_FUNCTIONS=_window_size,_main \
    -sEXPORTED_RUNTIME_METHODS=cwrap
