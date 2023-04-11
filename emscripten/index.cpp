#include <emscripten/html5.h>
#include <GLES2/gl2.h>

#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include <app.h>

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;
static NVGcontext* vg;
static bool redraw_requested = false;

static float window_width, window_height, pixel_ratio;

extern "C" {
void window_size(int window_width_, int window_height_, int pixel_ratio_) {
    window_width = window_width_;
    window_height = window_height_;
    pixel_ratio = pixel_ratio_;
}
}

void main_loop() {
    emscripten_webgl_make_context_current(ctx);

    glViewport(0, 0, window_width * pixel_ratio, window_height * pixel_ratio);
    glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    app_render(window_width, window_height, pixel_ratio);
}

EM_BOOL touch_event(int event_type, const EmscriptenTouchEvent* event, void* user_data) {

    AppTouchEvent app_event;

    switch (event_type) {
        case EMSCRIPTEN_EVENT_TOUCHSTART:  app_event.type = TOUCH_START;  break;
        case EMSCRIPTEN_EVENT_TOUCHEND:    app_event.type = TOUCH_END;    break;
        case EMSCRIPTEN_EVENT_TOUCHMOVE:   app_event.type = TOUCH_MOVE;   break;
        case EMSCRIPTEN_EVENT_TOUCHCANCEL: app_event.type = TOUCH_CANCEL; break;
    }

    app_event.num_touches = event->numTouches;
    app_event.num_touches_changed = 0;
    for (int i = 0; i < event->numTouches; ++i) {
        auto& touch = event->touches[i];
        AppTouch app_touch;
        app_touch.id = (int)touch.identifier;
        app_touch.x = touch.targetX;
        app_touch.y = touch.targetY;
        app_event.touches[i] = app_touch;
        if (touch.isChanged) {
            app_event.touches_changed[app_event.num_touches_changed++] = app_touch;
        }
    }

    app_touch_event(&app_event);
    return 1;
}

static int mouse_x, mouse_y;

EM_BOOL mouse_event(int event_type, const EmscriptenMouseEvent* event, void* user_data) {

    AppTouchEvent app_event;

    AppTouch touch;
    touch.id = 1;
    touch.x = mouse_x = event->targetX;
    touch.y = mouse_y = event->targetY;

    app_event.num_touches = 1;
    app_event.touches[0] = touch;

    app_event.num_touches_changed = 1;
    app_event.touches_changed[0] = touch;

    switch (event_type) {
        case EMSCRIPTEN_EVENT_MOUSEDOWN: app_event.type = TOUCH_START; break;
        case EMSCRIPTEN_EVENT_MOUSEUP: {
            app_event.type = TOUCH_END;
            app_event.num_touches = 0;
            break;
        }
        case EMSCRIPTEN_EVENT_MOUSEMOVE: app_event.type = TOUCH_MOVE;  break;
    }

    app_touch_event(&app_event);
    return 1;
}

EM_BOOL scroll_event(int event_type, const EmscriptenWheelEvent* event, void* user_data) {
    app_scroll_event(mouse_x, mouse_y, event->deltaX, event->deltaY);
    return 1;
}

static const char* get_asset_name(const char* asset_name, const char* asset_type) {
    static char buf[256];
    sprintf(buf, "assets/%s.%s", asset_name, asset_type);
    return buf;
}

int main() {

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;

    ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (vg == NULL) {
        printf("Could not init nanovg.\n");
        return -1;
    }

    emscripten_set_touchstart_callback ("#canvas", NULL, 0, touch_event);
    emscripten_set_touchend_callback   ("#canvas", NULL, 0, touch_event);
    emscripten_set_touchmove_callback  ("#canvas", NULL, 0, touch_event);
    emscripten_set_touchcancel_callback("#canvas", NULL, 0, touch_event);
    emscripten_set_mousedown_callback  ("#canvas", NULL, 0, mouse_event);
    emscripten_set_mouseup_callback    ("#canvas", NULL, 0, mouse_event);
    emscripten_set_mousemove_callback  ("#canvas", NULL, 0, mouse_event);
    emscripten_set_wheel_callback      ("#canvas", NULL, 0, scroll_event);

    AppKeyboard keyboard;
    keyboard.open = [](void* opaque_ptr) {};
    keyboard.close = [](void* opaque_ptr) {};
    keyboard.rect = [](void* opaque_ptr, float* x, float* y, float* width, float* height) {
        *x = 0;
        *y = 0;
        *width = 0;
        *height = 0;
    };

    app_init(vg, keyboard, get_asset_name);
    emscripten_set_main_loop(&main_loop, 0, 1);
    // nvgDeleteGLES2(vg);

    return 0;
}
