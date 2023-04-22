#include <emscripten/html5.h>
#include <emscripten/websocket.h>
#include <emscripten/fetch.h>
#include <GLES2/gl2.h>

#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include <nanovg/nanovg_gl.h>
#include <nanovg/nanovg_gl_utils.h>

#include <app.h>

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;
static NVGcontext* vg;

static float window_width, window_height, pixel_ratio;
static bool force_render = false;

extern "C" {
void window_size(int window_width_, int window_height_, int pixel_ratio_) {
    window_width = window_width_;
    window_height = window_height_;
    pixel_ratio = pixel_ratio_;
    force_render = true;
}
void fs_mounted(void);
}

void main_loop() {
    if (!app_wants_to_render() && !force_render) {
        return;
    }
    force_render = false;

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

static void user_data_flush() {
    EM_ASM(
        if (!window.flushing) {
            window.flushing = true;
            FS.syncfs(false, err => {
                if (err) {
                    console.error(err)
                }
                window.flushing = false;
            });
        }
    );
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

EM_BOOL websocket_open_event(int event_type, const EmscriptenWebSocketOpenEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_OPEN;
    app_event.ws = event->socket;
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_message_event(int event_type, const EmscriptenWebSocketMessageEvent* event, void* user_data) {
    if (!event->isText) return 0;
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_MESSAGE;
    app_event.ws = event->socket;
    app_event.data = (const char*)event->data;
    app_event.data_length = strlen((const char*)event->data);
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_error_event(int event_type, const EmscriptenWebSocketErrorEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_ERROR;
    app_event.ws = event->socket;
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_close_event(int event_type, const EmscriptenWebSocketCloseEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_CLOSE;
    app_event.ws = event->socket;
    app_event.code = event->code;
    app_event.data = event->reason;
    app_event.data_length = strlen(event->reason);
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

AppWebsocketHandle websocket_open(const char* url, void* user_data) {
    EmscriptenWebSocketCreateAttributes attributes;
    emscripten_websocket_init_create_attributes(&attributes);

    attributes.url = url;
    attributes.protocols = "binary";
    attributes.createOnMainThread = 1;

    auto socket = emscripten_websocket_new(&attributes);
    if (socket <= 0) {
        return socket;
    }

    emscripten_websocket_set_onopen_callback   (socket, user_data, websocket_open_event);
    emscripten_websocket_set_onerror_callback  (socket, user_data, websocket_error_event);
    emscripten_websocket_set_onclose_callback  (socket, user_data, websocket_close_event);
    emscripten_websocket_set_onmessage_callback(socket, user_data, websocket_message_event);

    return socket;
}

void websocket_send(AppWebsocketHandle ws, const char* data) {
    emscripten_websocket_send_utf8_text(ws, data);
}

void websocket_close(AppWebsocketHandle ws, unsigned short code, const char* reason) {
    emscripten_websocket_close(ws, code, reason);
    emscripten_websocket_delete(ws);
}

void fetch_success(emscripten_fetch_t* fetch) {
    AppHttpEvent event;
    event.type = HTTP_RESPONSE_DATA;
    event.user_data = fetch->userData;
    event.status_code = fetch->status;
    event.data = (const uint8_t*)fetch->data;
    event.data_length = fetch->numBytes;
    app_http_event(&event);

    event.type = HTTP_RESPONSE_END;
    app_http_event(&event);

    emscripten_fetch_close(fetch);
}

void fetch_failed(emscripten_fetch_t* fetch) {
    AppHttpEvent event;
    event.type = HTTP_RESPONSE_ERROR;
    event.user_data = fetch->userData;
    event.status_code = fetch->status;
    app_http_event(&event);

    emscripten_fetch_close(fetch);
}

void http_request_send(const char* url, void* user_data) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.userData = user_data;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = &fetch_success;
    attr.onerror = &fetch_failed;
    emscripten_fetch(&attr, url);
}

int main() {

    EM_ASM(
        FS.mkdir('/idbfs');
        FS.mount(IDBFS, {}, '/idbfs');
        FS.syncfs(true, err => {
            if (err) {
                console.error(err)
            } else {
                Module.ccall('fs_mounted', 'void', [], [])
            }
        });
    );

}

void fs_mounted() {

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;

    ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (vg == NULL) {
        printf("Could not init nanovg.\n");
        return;
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

    AppStorage storage;
    storage.get_asset_name = &get_asset_name;
    storage.user_data_dir = "/idbfs";
    storage.user_data_flush = &user_data_flush;

    AppNetworking networking;
    networking.websocket_open  = &websocket_open;
    networking.websocket_send  = &websocket_send;
    networking.websocket_close = &websocket_close;
    networking.http_request_send = &http_request_send;

    app_init(vg, keyboard, storage, networking);
    emscripten_set_main_loop(&main_loop, 0, 1);
    // nvgDeleteGLES2(vg);
}
