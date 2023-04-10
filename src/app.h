//
//  app.h
//  privavida-core
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#ifndef app_h
#define app_h

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
    void (*redraw)(void* opaque_ptr);
} AppRedraw;

typedef struct {
    void* opaque_ptr;
    void (*open)(void* opaque_ptr);
    void (*close)(void* opaque_ptr);
    void (*rect)(void* opaque_ptr, float* x, float* y, float* width, float* height);
} AppKeyboard;

void app_init(NVGcontext* vg, AppRedraw redraw, AppKeyboard keyboard);
void app_set_temp_directory(const char* temp_directory);
void app_render(float window_width, float window_height, float pixel_density);
void app_touch_event(AppTouchEvent* event);
void app_key_backspace(void);
void app_key_character(const char* ch);

#endif /* app_h */
