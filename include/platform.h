//
//  platform.h
//  privavida-core
//
//  Created by Bartholomew Joyce on 24/04/2023.
//

#ifndef platform_h
#define platform_h

// This is the platform C interface for the app.
// As long as you implement the bindings for the app on your
// specific platform you can run the privavida-core.

#ifdef __cplusplus
extern "C" {
#endif

#include <nanovg.h>



// Initialization and render function
void app_init(NVGcontext* vg);
int  app_wants_to_render(void);
void app_render(float window_width, float window_height, float pixel_density);



// Touch event handling
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

void app_touch_event(const AppTouchEvent* event);



// Scroll event handling
void app_scroll_event(int x, int y, int dx, int dy);



// Text input control and event handling
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
    const char* content;
} AppTextInputConfig;

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

void platform_update_text_input(const AppTextInputConfig* config);
void platform_remove_text_input();
void app_text_input_content_changed(const char* new_content);

void app_keyboard_changed(int is_showing, float x, float y, float width, float height);
void app_key_event(AppKeyEvent event);



// User data storage & asset storage
const char* platform_get_asset_name(const char* asset_name, const char* asset_type);
extern const char* platform_user_data_dir;
void platform_user_data_flush(void);



// Websockets
typedef int AppWebsocketHandle;

enum AppWebsocketEventType {
    WEBSOCKET_OPEN,
    WEBSOCKET_CLOSE,
    WEBSOCKET_MESSAGE,
    WEBSOCKET_ERROR
};

typedef struct {
    enum AppWebsocketEventType type;
    AppWebsocketHandle socket;
    void* user_data;
    unsigned short code;
    const char* data;
    int data_length;
} AppWebsocketEvent;

AppWebsocketHandle platform_websocket_open(const char* url, void* user_data);
void platform_websocket_send(AppWebsocketHandle socket, const char* data);
void platform_websocket_close(AppWebsocketHandle socket, unsigned short code, const char* reason);
void app_websocket_event(const AppWebsocketEvent* event);



// HTTP requests
void platform_http_request(const char* url, void* user_data);
void app_http_response(int status_code, const unsigned char* data, int data_length, void* user_data);

void platform_http_image_request(const char* url, void* user_data);
void app_http_image_response(int image_id, void* user_data);



// Platform Emoji rendering extension
extern int platform_supports_emoji; // Set to 0 if not available

typedef struct {
    int bounding_height;
    int bounding_width;
    int baseline;
    int left;
    int width;
} PlatformEmojiMetrics;
int platform_emoji_measure(const char* data, int data_length, int text_size, PlatformEmojiMetrics* metrics);

typedef struct {
    int image_id;
    int left, top;
} PlatformEmojiRenderTarget;
void platform_emoji_render(const char* data, int data_length, int text_size, NVGcolor color, const PlatformEmojiRenderTarget* render_target);


#ifdef __cplusplus
}
#endif

#endif /* platform_h */
