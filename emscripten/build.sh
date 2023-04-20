emcc \
    emscripten/index.cpp \
    lib/nanovg/nanovg.c \
    lib/sha256/sha256.c \
    src/app.cpp \
    src/ui.cpp \
    src/utils/animation.cpp \
    src/views/Root.cpp \
    src/views/Conversations.cpp \
    src/views/ScrollView.cpp \
    src/models/event.cpp \
    src/models/event_parse.cpp \
    src/models/relay_message_parse.cpp \
    src/models/event_stringify.cpp \
    src/models/hex.cpp \
    src/models/secp256k1.c \
    src/network/network.cpp \
    -I include/ \
    -I lib/ \
    -I lib/rapidjson/include/ \
    -I lib/secp256k1/include/ \
    -o emscripten/index.js \
    --embed-file assets/SFBold.ttf \
    --embed-file assets/SFRegular.ttf \
    --embed-file assets/profile.jpeg \
    -sEXPORTED_FUNCTIONS=_window_size,_main,_fs_mounted \
    -sEXPORTED_RUNTIME_METHODS=cwrap,ccall \
    -lidbfs.js \
    -lwebsocket.js
