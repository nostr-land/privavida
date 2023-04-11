emcc \
    emscripten/index.cpp \
    lib/nanovg/nanovg.c \
    src/app.cpp \
    src/ui.cpp \
    src/utils/animation.cpp \
    src/views/Root.cpp \
    src/views/Conversations.cpp \
    src/views/ScrollView.cpp \
    -I include/ \
    -I lib/ \
    -o emscripten/index.js \
    --embed-file assets/SFBold.ttf \
    --embed-file assets/SFRegular.ttf \
    --embed-file assets/profile.jpeg \
    -sEXPORTED_FUNCTIONS=_window_size,_main,_fs_mounted \
    -sEXPORTED_RUNTIME_METHODS=cwrap,ccall \
    -lidbfs.js
