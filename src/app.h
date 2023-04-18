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


// Keyboard control
typedef struct {
    void* opaque_ptr;
    void (*open)(void* opaque_ptr);
    void (*close)(void* opaque_ptr);
    void (*rect)(void* opaque_ptr, float* x, float* y, float* width, float* height);
} AppKeyboard;


// Storage
typedef struct {
    const char* (*get_asset_name)(const char* asset_name, const char* asset_type);
    const char* user_data_dir;
    void (*user_data_flush)(void);
} AppStorage;


// Networking
typedef int AppWebsocketHandle;

typedef struct {
    void* opaque_ptr;
    AppWebsocketHandle (*websocket_open)(void* opaque_ptr, const char* url);
    void (*websocket_send)(void* opaque_ptr, AppWebsocketHandle ws, const char* data);
    void (*websocket_close)(void* opaque_ptr, AppWebsocketHandle ws, unsigned short code, const char* reason);
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
} AppWebsocketEvent;

void app_init(NVGcontext* vg, AppKeyboard keyboard, AppStorage storage, AppNetworking networking);
int  app_wants_to_render(void);
void app_render(float window_width, float window_height, float pixel_density);
void app_touch_event(AppTouchEvent* event);
void app_scroll_event(int x, int y, int dx, int dy);
void app_key_backspace(void);
void app_key_character(const char* ch);
void app_websocket_event(const AppWebsocketEvent* event);

#ifdef __cplusplus
}
#endif

#endif /* app_h */
