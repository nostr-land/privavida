//
//  app.h
//  privavida-core
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#ifndef app_h
#define app_h

// This is the platform-independent C interface for the app
// As long as you implement the bindings for the app on your
// specific platform you can run the privavida-core.

#ifdef __cplusplus
extern "C" {
#endif

#include <nanovg.h>

// Touch events
#define MAX_TOUCHES 16

typedef struct {
    int id;
    float x;
    float y;
    void* opaque_ptr;
} AppTouch;

enum AppTouchEventType {
    TOUCH_START,
    TOUCH_MOVE,
    TOUCH_END,
    TOUCH_CANCEL
};

typedef struct {
    enum AppTouchEventType type;
    int num_touches;
    int num_touches_changed;
    AppTouch touches[MAX_TOUCHES];
    AppTouch touches_changed[MAX_TOUCHES];
} AppTouchEvent;


// Text input control
// enum AppTextAnchor {
//     APP_TEXT_ANCHOR_TOP,
//     APP_TEXT_ANCHOR_BOTTOM
// };
enum AppTextFlags {
    APP_TEXT_FLAGS_NONE = 0,
    APP_TEXT_FLAGS_WORD_WRAP = 1,
    APP_TEXT_FLAGS_MULTILINE = 2,
    APP_TEXT_FLAGS_TYPE_EMAIL = 4,
    APP_TEXT_FLAGS_TYPE_PASSWORD = 8
};

typedef struct {
    // Positioning of the text input
    // enum AppTextAnchor anchor;
    float x, y, width, height;

    // Font and text settings
    float font_size, line_height;
    NVGcolor text_color;
    int flags;

    // Content
    const char* text_content;
} AppTextInputConfig;

typedef struct {
    void* opaque_ptr;
    void (*update_text_input)(void* opaque_ptr, const AppTextInputConfig* config);
    void (*remove_text_input)(void* opaque_ptr);
} AppText;

enum AppKeyAction {
    KEY_PRESS,
    KEY_RELEASE,
    KEY_REPEAT
};

typedef struct {
    enum AppKeyAction action;
    char ch[8];
    int key;
    int mods;
} AppKeyEvent;


// Storage
typedef struct {
    const char* (*get_asset_name)(const char* asset_name, const char* asset_type);
    const char* user_data_dir;
    void (*user_data_flush)(void);
} AppStorage;


// Networking
typedef int AppWebsocketHandle;

typedef struct {
    AppWebsocketHandle (*websocket_open)(const char* url, void* user_data);
    void (*websocket_send)(AppWebsocketHandle ws, const char* data);
    void (*websocket_close)(AppWebsocketHandle ws, unsigned short code, const char* reason);
    void (*http_request_send)(const char* url, void* user_data);
} AppNetworking;

enum AppWebsocketEventType {
    WEBSOCKET_OPEN,
    WEBSOCKET_CLOSE,
    WEBSOCKET_MESSAGE,
    WEBSOCKET_ERROR
};

typedef struct {
    enum AppWebsocketEventType type;
    AppWebsocketHandle ws;
    unsigned short code;
    const char* data;
    int data_length;
    void* user_data;
} AppWebsocketEvent;

enum AppHttpEventType {
    HTTP_RESPONSE_ERROR,
    HTTP_RESPONSE_OPEN,
    HTTP_RESPONSE_DATA,
    HTTP_RESPONSE_END
};

typedef struct {
    enum AppHttpEventType type;
    int status_code;
    const unsigned char* data;
    int data_length;
    void* user_data;
} AppHttpEvent;


void app_init(NVGcontext* vg, AppText text, AppStorage storage, AppNetworking networking);
int  app_wants_to_render(void);
void app_render(float window_width, float window_height, float pixel_density);
void app_touch_event(AppTouchEvent* event);
void app_scroll_event(int x, int y, int dx, int dy);
void app_keyboard_changed(int is_showing, float x, float y, float width, float height);
void app_key_event(AppKeyEvent event);
void app_websocket_event(const AppWebsocketEvent* event);
void app_http_event(const AppHttpEvent* event);

#ifdef __cplusplus
}
#endif

#endif /* app_h */
