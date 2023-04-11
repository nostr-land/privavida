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

typedef struct {
    void* opaque_ptr;
    void (*open)(void* opaque_ptr);
    void (*close)(void* opaque_ptr);
    void (*rect)(void* opaque_ptr, float* x, float* y, float* width, float* height);
} AppKeyboard;

typedef struct {
    const char* (*get_asset_name)(const char* asset_name, const char* asset_type);
    const char* user_data_dir;
    void (*user_data_flush)(void);
} AppStorage;

void app_init(NVGcontext* vg, AppKeyboard keyboard, AppStorage storage);
int  app_wants_to_render(void);
void app_render(float window_width, float window_height, float pixel_density);
void app_touch_event(AppTouchEvent* event);
void app_scroll_event(int x, int y, int dx, int dy);
void app_key_backspace(void);
void app_key_character(const char* ch);

#ifdef __cplusplus
}
#endif

#endif /* app_h */
