#include <emscripten/html5.h>
#include <emscripten/websocket.h>
#include <emscripten/fetch.h>
#include <GLES2/gl2.h>

#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION
#include <nanovg/nanovg_gl.h>
#include <nanovg/nanovg_gl_utils.h>

#include <platform.h>
#include <rapidjson/writer.h>
#include <vector>

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
void app_http_image_response_tex(int error, int tex_id, int width, int height, int request_id);
void platform_did_emoji_measure(int bounding_height, int bounding_width, int baseline, int left, int right);
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

void platform_open_url(const char* url) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.String(url, strlen(url));
    auto url_encoded = sb.GetString();

    int command_size = 64 + strlen(url_encoded);
    char command[command_size];
    snprintf(command, command_size, "window.location.assign(%s)", url_encoded);

    emscripten_run_script(command);
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

static bool text_input_showing = false;
static AppTextInputConfig text_input_config = { 0 };

void platform_update_text_input(const AppTextInputConfig* config) {
    if (text_input_showing &&
        memcmp(&text_input_config, config, sizeof(AppTextInputConfig)) == 0) {
        return; // No change
    }

    bool resend_all = !text_input_showing;

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();

    if (resend_all || text_input_config.x != config->x) {
        writer.String("x");
        writer.Int(config->x);
    }
    if (resend_all || text_input_config.y != config->y) {
        writer.String("y");
        writer.Int(config->y);
    }
    if (resend_all || text_input_config.width != config->width) {
        writer.String("width");
        writer.Int(config->width);
    }
    if (resend_all || text_input_config.height != config->height) {
        writer.String("height");
        writer.Int(config->height);
    }
    if (resend_all || text_input_config.font_size != config->font_size) {
        writer.String("fontSize");
        writer.Int(config->font_size);
    }
    if (resend_all || text_input_config.line_height != config->line_height) {
        writer.String("lineHeight");
        writer.Int(config->line_height);
    }
    if (resend_all || memcmp(&text_input_config.text_color, &config->text_color, sizeof(NVGcolor)) != 0) {
        writer.String("textColor");
        char text_color[64];
        if (config->text_color.a == 1.0) {
            snprintf(text_color, sizeof(text_color), "#%02x%02x%02x", (int)(config->text_color.r * 255.0), (int)(config->text_color.g * 255.0), (int)(config->text_color.b * 255.0));
        } else {
            snprintf(text_color, sizeof(text_color), "rgba(%d,%d,%d,%f)", (int)(config->text_color.r * 255.0), (int)(config->text_color.g * 255.0), (int)(config->text_color.b * 255.0), config->text_color.a);
        }
        writer.String(text_color);
    }
    if (resend_all || text_input_config.flags != config->flags) {
        writer.String("flags");
        writer.Int(config->flags);
    }
    if (resend_all || strcmp(text_input_config.content, config->content) != 0) {
        writer.String("content");
        writer.String(config->content);
    }

    writer.EndObject();

    auto config_json = sb.GetString();

    int command_size = 64 + strlen(config_json);
    char command[command_size];
    snprintf(command, command_size, "__platform_update_text_input(%s)", config_json);
    emscripten_run_script(command);

    text_input_showing = true;
    text_input_config = *config;
}

void platform_remove_text_input() {
    text_input_showing = false;

    emscripten_run_script("__platform_update_text_input(null)");
}


int platform_supports_emoji = 1;

static bool emoji_success;
static PlatformEmojiMetrics* emoji_metrics;
void platform_did_emoji_measure(int bounding_height, int bounding_width, int baseline, int left, int width) {
    emoji_metrics->bounding_height = bounding_height;
    emoji_metrics->bounding_width = bounding_width;
    emoji_metrics->baseline = baseline;
    emoji_metrics->left = left;
    emoji_metrics->width = width;
    emoji_success = true;
}

int platform_emoji_measure(const char* data, int data_length, int text_size, PlatformEmojiMetrics* metrics) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.String(data, (rapidjson::SizeType)data_length);
    auto data_encoded = sb.GetString();

    emoji_success = false;
    emoji_metrics = metrics;

    int command_size = 64 + strlen(data_encoded);
    char command[command_size];
    snprintf(command, command_size, "__platform_emoji_measure(%s,%d)", data_encoded, text_size);
    emscripten_run_script(command);

    return emoji_success;
}

void platform_emoji_render(const char* data, int data_length, int text_size, NVGcolor color, const PlatformEmojiRenderTarget* render_target) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.String(data, (rapidjson::SizeType)data_length);
    auto data_encoded = sb.GetString();

    char color_encoded[32];
    if (color.a == 1.0) {
        snprintf(color_encoded, sizeof(color_encoded), "\"#%02x%02x%02x\"", (int)(color.r * 255.0), (int)(color.g * 255.0), (int)(color.b * 255.0));
    } else {
        snprintf(color_encoded, sizeof(color_encoded), "\"rgba(%d,%d,%d,%f)\"", (int)(color.r * 255.0), (int)(color.g * 255.0), (int)(color.b * 255.0), color.a);
    }

    auto tex_id = nvglImageHandleGLES2(vg, render_target->image_id);
    auto x = render_target->left;
    auto y = render_target->top;

    int command_size = 64 + strlen(data_encoded);
    char command[command_size];
    snprintf(command, command_size, "__platform_emoji_render(%s,%d,%s,%d,%d,%d)", data_encoded, text_size, color_encoded, tex_id, x, y);

    emscripten_run_script(command);
}

const char* platform_user_data_dir = "/idbfs";

const char* platform_get_asset_name(const char* asset_name, const char* asset_type) {
    static char buf[256];
    sprintf(buf, "assets/%s.%s", asset_name, asset_type);
    return buf;
}

void platform_user_data_flush() {
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


EM_BOOL websocket_open_event(int event_type, const EmscriptenWebSocketOpenEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_OPEN;
    app_event.socket = event->socket;
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_message_event(int event_type, const EmscriptenWebSocketMessageEvent* event, void* user_data) {
    if (!event->isText) return 0;
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_MESSAGE;
    app_event.socket = event->socket;
    app_event.data = (const char*)event->data;
    app_event.data_length = event->data ? strlen((const char*)event->data) : 0;
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_error_event(int event_type, const EmscriptenWebSocketErrorEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_ERROR;
    app_event.socket = event->socket;
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

EM_BOOL websocket_close_event(int event_type, const EmscriptenWebSocketCloseEvent* event, void* user_data) {
    AppWebsocketEvent app_event;
    app_event.type = WEBSOCKET_CLOSE;
    app_event.socket = event->socket;
    app_event.code = event->code;
    app_event.data = event->reason;
    app_event.data_length = strlen(event->reason);
    app_event.user_data = user_data;
    app_websocket_event(&app_event);
    return 1;
}

AppWebsocketHandle platform_websocket_open(const char* url, void* user_data) {
    EmscriptenWebSocketCreateAttributes attributes;
    emscripten_websocket_init_create_attributes(&attributes);

    attributes.url = url;
    attributes.protocols = "binary";
    attributes.createOnMainThread = 1;

    auto socket = emscripten_websocket_new(&attributes);
    if (socket <= 0) {
        return -1;
    }

    emscripten_websocket_set_onopen_callback   (socket, user_data, websocket_open_event);
    emscripten_websocket_set_onerror_callback  (socket, user_data, websocket_error_event);
    emscripten_websocket_set_onclose_callback  (socket, user_data, websocket_close_event);
    emscripten_websocket_set_onmessage_callback(socket, user_data, websocket_message_event);

    printf("Websocket open: %s\n", url);
    return socket;
}

void platform_websocket_send(AppWebsocketHandle socket, const char* data) {
    emscripten_websocket_send_utf8_text(socket, data);
}

void platform_websocket_close(AppWebsocketHandle socket, unsigned short code, const char* reason) {
    emscripten_websocket_close(socket, code, reason);
    emscripten_websocket_delete(socket);
}

static void fetch_success(emscripten_fetch_t* fetch) {
    app_http_response(fetch->status, (const uint8_t*)fetch->data, fetch->numBytes, fetch->userData);
    emscripten_fetch_close(fetch);
}

static void fetch_failed(emscripten_fetch_t* fetch) {
    app_http_response(fetch->status, NULL, 0, fetch->userData);
    emscripten_fetch_close(fetch);
}

void platform_http_request(const char* url, void* user_data) {
    {
        // Disable fetches for now
        app_http_response(-1, NULL, 0, user_data);
        return;
    }

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.userData = user_data;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = &fetch_success;
    attr.onerror = &fetch_failed;
    emscripten_fetch(&attr, url);
}

static std::vector<void*> image_request_user_datas;

void platform_http_image_request(const char* url, void* user_data) {
    assert(user_data);
    rapidjson::StringBuffer sb;

    int request_id;
    {
        for (int i = 0; i < image_request_user_datas.size(); ++i) {
            if (!image_request_user_datas[i]) {
                request_id = i;
                break;
            }
        }
        request_id = image_request_user_datas.size();
        image_request_user_datas.push_back(user_data);
    }

    const char* url_escaped;
    {
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.String(url);
        url_escaped = sb.GetString();
    }

    int command_size = 64 + strlen(url_escaped);
    char command[command_size];
    snprintf(command, command_size, "__platform_http_image_request(%s, %d)", url_escaped, request_id);
    emscripten_run_script(command);
}

void app_http_image_response_tex(int error, int tex_id, int width, int height, int request_id) {
    void* user_data = image_request_user_datas[request_id];
    image_request_user_datas[request_id] = NULL;
    if (error) {
        app_http_image_response(0, user_data);
        return;
    }

    int image_id = nvglCreateImageFromHandleGLES2(vg, tex_id, width, height, 0);
    app_http_image_response(image_id, user_data);
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

        const app_http_image_response_tex = Module.cwrap('app_http_image_response_tex', 'void', ['int', 'int', 'int', 'int', 'int']);

        window.__platform_http_image_request = (url, request_id) => {
            const image = new Image();
            image.crossOrigin = 'anonymous';
            image.onload = () => {
                const tex = GLctx.createTexture();
                const texId = GL.getNewId(GL.textures);
                GL.textures[texId] = tex;
                GLctx.bindTexture(GLctx.TEXTURE_2D, tex);
                GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_WRAP_S, GLctx.CLAMP_TO_EDGE);
                GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_WRAP_T, GLctx.CLAMP_TO_EDGE);
                GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MIN_FILTER, GLctx.NEAREST);
                GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MAG_FILTER, GLctx.NEAREST);
                GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, GLctx.RGBA, GLctx.UNSIGNED_BYTE, image);
                app_http_image_response_tex(0, texId, image.width, image.height, request_id);
            };
            image.onerror = () => app_http_image_response_tex(1, 0, 0, 0, request_id);
            image.src = url;
        };
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

    app_init(vg);
    emscripten_set_main_loop(&main_loop, 0, 0);
}
