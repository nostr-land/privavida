emcc \
    emscripten/index.cpp \
    lib/nanovg/nanovg.c \
    src/app.cpp \
    src/utils/animation.cpp \
    src/utils/timer.cpp \
    src/utils/text_rendering.cpp \
    src/views/Root.cpp \
    src/views/LoginView.cpp \
    src/views/Conversations.cpp \
    src/views/ChatView/ChatView.cpp \
    src/views/ChatView/ChatViewEntry.cpp \
    src/views/ChatView/VirtualizedList.cpp \
    src/views/ChatView/ChatMessage.cpp \
    src/views/ChatView/Composer.cpp \
    src/views/MessageInspect/MessageInspect.cpp \
    src/views/TextInput/TextInput.cpp \
    src/views/TextRender/TextRender.cpp \
    src/views/ScrollView.cpp \
    src/models/keys.cpp \
    src/models/event.cpp \
    src/models/event_parse.cpp \
    src/models/relay_message.cpp \
    src/models/client_message.cpp \
    src/models/event_stringify.cpp \
    src/models/event_content.cpp \
    src/models/profile.cpp \
    src/models/hex.cpp \
    src/models/nostr_entity.cpp \
    src/models/nip04.cpp \
    src/models/nip31.cpp \
    src/models/account.cpp \
    src/models/c/aes.c \
    src/models/c/secp256k1.c \
    src/models/c/base64.c \
    src/models/c/bech32.c \
    src/models/c/sha256.c \
    src/network/network.cpp \
    src/data_layer/accounts.cpp \
    src/data_layer/events.cpp \
    src/data_layer/relays.cpp \
    src/data_layer/conversations.cpp \
    src/data_layer/profiles.cpp \
    src/data_layer/contact_lists.cpp \
    src/data_layer/images.cpp \
    -I include/ \
    -I lib/ \
    -I lib/rapidjson/include/ \
    -I lib/secp256k1/include/ \
    -o emscripten/index.js \
    --embed-file assets/SFBold.ttf \
    --embed-file assets/SFRegular.ttf \
    --embed-file assets/privavida-icons.ttf \
    -sEXPORTED_FUNCTIONS=_window_size,_main,_fs_mounted,_app_http_image_response_tex,_app_text_input_content_changed,_platform_did_emoji_measure \
    -sEXPORTED_RUNTIME_METHODS=cwrap,ccall,writeStringToMemory \
    -lidbfs.js \
    -lwebsocket.js \
    -sFETCH
