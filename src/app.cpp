//
//  app.c
//  privavida-core
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#include <platform.h>
#include <app.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "utils/animation.hpp"
#include "utils/timer.hpp"
#include "utils/text_rendering.hpp"
#include "views/Root.hpp"
#include "network/network.hpp"
#include "data_layer/accounts.hpp"

static bool redraw_requested;

static void process_immediate_callbacks();
static void process_touch_queue();
static bool has_key_events_to_process();
static void process_next_key_event();
static void text_input_begin_frame();
static void text_input_end_frame();
static void clear_scroll();

void app_init(NVGcontext* vg_) {
    redraw_requested = true;
    ui::text_rendering_init();
    ui::vg = vg_;
    if (!data_layer::accounts_load()) return;
    timer::init();
    network::init();

    // nvgCreateFont(vg_, "mono",     app::get_asset_name("PTMono",          "ttf"));
    nvgCreateFont(vg_, "regular",  app::get_asset_name("SFRegular",       "ttf"));
    // nvgCreateFont(vg_, "regulari", app::get_asset_name("SFRegularItalic", "ttf"));
    // nvgCreateFont(vg_, "medium",   app::get_asset_name("SFMedium",        "ttf"));
    // nvgCreateFont(vg_, "mediumi",  app::get_asset_name("SFMediumItalic",  "ttf"));
    nvgCreateFont(vg_, "bold",     app::get_asset_name("SFBold",          "ttf"));
    // nvgCreateFont(vg_, "boldi",    app::get_asset_name("SFBoldItalic",    "ttf"));
    // nvgCreateFont(vg_, "thin",     app::get_asset_name("SFThin",          "ttf"));
    // nvgCreateImage(vg_, app::get_asset_name("profile", "jpeg"), 0);

    Root::init();
}

int app_wants_to_render() {
    timer::update();
    return (redraw_requested || has_key_events_to_process() || animation::is_animating());
}

void app_render(float window_width, float window_height, float pixel_density) {
    process_touch_queue();

    while (true) {
        nvgBeginFrame(ui::vg, window_width, window_height, pixel_density);
        redraw_requested = false;

        text_input_begin_frame();
        animation::update_animation();

        ui::reset();
        ui::view.width = window_width;
        ui::view.height = window_height;

        process_immediate_callbacks();
        process_next_key_event();
        Root::update();
        clear_scroll();

        if (redraw_requested) {
            nvgCancelFrame(ui::vg);
            continue;
        } else {
            break;
        }
    }

    nvgEndFrame(ui::vg);
    text_input_end_frame();
}



// Redraw
void ui::redraw() {
    redraw_requested = true;
}

static std::vector<std::function<void()>> immediate_callbacks;
void app::set_immediate(std::function<void()> callback) {
    immediate_callbacks.push_back(std::move(callback));
    ui::redraw();
}
void process_immediate_callbacks() {
    static std::vector<std::function<void()>> immediate_callbacks_copy;
    std::swap(immediate_callbacks_copy, immediate_callbacks);
    for (auto& fn : immediate_callbacks_copy) {
        fn();
    }
    immediate_callbacks_copy.clear();
}




// Rendering & Viewport
NVGcontext* ui::vg;
ui::Viewport ui::view;
constexpr int MAX_SAVED_VIEWS = 128;
static ui::Viewport saved_views[MAX_SAVED_VIEWS];
static int num_saved_views = 0;

void ui::save() {
    nvgSave(ui::vg);
    saved_views[num_saved_views++] = view;
}

void ui::restore() {
    nvgRestore(ui::vg);
    view = saved_views[--num_saved_views];
}

void ui::reset() {
    nvgReset(ui::vg);
    view = saved_views[0];
    num_saved_views = 1;
}

void ui::sub_view(float x, float y, float width, float height) {
    save();
    nvgTranslate(ui::vg, x, y);
    view.width = width;
    view.height = height;
}

void ui::to_screen_point(float x, float y, float* sx, float* sy) {
    float xform[6], inv_xform[6];
    nvgCurrentTransform(ui::vg, xform);
    nvgTransformInverse(inv_xform, xform);
    nvgTransformPoint(sx, sy, xform, x, y);
}

void ui::to_screen_rect(float x, float y, float width, float height, float* sx, float* sy, float* swidth, float* sheight) {
    float xform[6], inv_xform[6];
    nvgCurrentTransform(ui::vg, xform);
    nvgTransformInverse(inv_xform, xform);
    nvgTransformPoint(sx, sy, xform, x, y);
    nvgTransformPoint(swidth, sheight, xform, x + width, y + height);
    *swidth -= *sx;
    *sheight -= *sy;
}

void ui::to_view_point(float x, float y, float* vx, float* vy) {
    float xform[6];
    nvgCurrentTransform(ui::vg, xform);
    nvgTransformPoint(vx, vy, xform, x, y);
}

void ui::to_view_rect(float x, float y, float width, float height, float* vx, float* vy, float* vwidth, float* vheight) {
    float xform[6];
    nvgCurrentTransform(ui::vg, xform);
    nvgTransformPoint(vx, vy, xform, x, y);
    nvgTransformPoint(vwidth, vheight, xform, x + width, y + height);
    *vwidth -= *vx;
    *vheight -= *vy;
}







// Touches
static AppTouchEvent touch_event_queue[1024];
static int touch_event_queue_size = 0;
static ui::Touch touches[MAX_TOUCHES];
static int num_touches = 0;

void app_touch_event(const AppTouchEvent* event) {
    touch_event_queue[touch_event_queue_size++] = *event;
    ui::redraw();
}

void process_touch_queue() {

    // Remove touches flagged as TOUCH_ENDED
    {
        ui::Touch touches_next[MAX_TOUCHES];
        int num_touches_next = 0;
        for (int i = 0; i < num_touches; ++i) {
            if (!(touches[i].flags & ui::TOUCH_ENDED)) {
                touches_next[num_touches_next++] = touches[i];
            }
        }
        memcpy(touches, touches_next, sizeof(ui::Touch) * num_touches_next);
        num_touches = num_touches_next;
    }

    // Process new touch events
    for (int i = 0; i < touch_event_queue_size; ++i) {
        auto& event = touch_event_queue[i];
        for (int j = 0; j < event.num_touches_changed; ++j) {
            auto& touch = event.touches_changed[j];

            // Find our representation of the touch
            ui::Touch* gtouch = NULL;
            for (int k = 0; k < num_touches; ++k) {
                if (touches[k].id == touch.id) {
                    gtouch = &touches[k];
                    break;
                }
            }

            switch (event.type) {
                case TOUCH_START: {
                    gtouch = &touches[num_touches++];
                    gtouch->id = touch.id;
                    gtouch->x = gtouch->initial_x = touch.x;
                    gtouch->y = gtouch->initial_y = touch.y;
                    gtouch->flags = 0;
                    break;
                }

                case TOUCH_MOVE: {
                    if (!gtouch) {
                        break;
                    }

                    gtouch->x = touch.x;
                    gtouch->y = touch.y;
                    if (!(gtouch->flags & ui::TOUCH_MOVED) &&
                        (abs(gtouch->x - gtouch->initial_x) + abs(gtouch->y - gtouch->initial_y) > 4)) {
                        gtouch->flags |= ui::TOUCH_MOVED;
                    }
                    break;
                }

                case TOUCH_END:
                case TOUCH_CANCEL: {
                    if (!gtouch) {
                        break;
                    }

                    gtouch->flags |= ui::TOUCH_ENDED;
                    break;
                }
            }
        }
    }

    touch_event_queue_size = 0;
}




/////// WE NEED PRIVATE ACCESS TO NVGstate TO FETCH THE SCISSOR
#define NVG_MAX_STATES 32

struct NVGstate {
    NVGcompositeOperationState compositeOperation;
    int shapeAntiAlias;
    NVGpaint fill;
    NVGpaint stroke;
    float strokeWidth;
    float miterLimit;
    int lineJoin;
    int lineCap;
    float alpha;
    float xform[6];
    NVGscissor scissor;
    float fontSize;
    float letterSpacing;
    float lineHeight;
    float fontBlur;
    int textAlign;
    int fontId;
};

struct NVGcontext_ {
    NVGparams params;
    float* commands;
    int ccommands;
    int ncommands;
    float commandx, commandy;
    NVGstate states[NVG_MAX_STATES];
    int nstates;
    void* cache;
    float tessTol;
    float distTol;
    float fringeWidth;
    float devicePxRatio;
};

float ui::device_pixel_ratio() {
    auto vg_ = (NVGcontext_*)ui::vg;
    return vg_->devicePxRatio;
}

static NVGscissor* get_scissor() {
    auto vg_ = (NVGcontext_*)ui::vg;
    return &vg_->states[vg_->nstates - 1].scissor;
}


static bool is_point_inside_rect(float* xform, float* extent, float x, float y) {
    float inv_xform[6];
    nvgTransformInverse(inv_xform, xform);

    float x2, y2;
    nvgTransformPoint(&x2, &y2, inv_xform, x, y);

    return (
        (-extent[0] <= x2 && x2 < extent[0]) &&
        (-extent[1] <= y2 && y2 < extent[1])
    );
}

static bool touch_inside(float touch_x, float touch_y, float x, float y, float width, float height) {
    float xform[6], extent[2];
    nvgCurrentTransform(ui::vg, xform);
    
    extent[0] = width / 2;
    extent[1] = height / 2;

    nvgTransformPoint(&x, &y, xform, x + extent[0], y + extent[1]);
    xform[4] = x;
    xform[5] = y;
    
    if (!is_point_inside_rect(xform, extent, touch_x, touch_y)) {
        return false;
    }

    auto scissor = get_scissor();
    if (scissor->extent[0] < 0) {
        return true;
    }

    return is_point_inside_rect(scissor->xform, scissor->extent, touch_x, touch_y);
}

bool ui::touch_start(float x, float y, float width, float height, ui::Touch* touch) {
    for (int i = 0; i < num_touches; ++i) {
        if ((touches[i].flags & (TOUCH_ENDED | TOUCH_ACCEPTED)) == 0 &&
            touch_inside(touches[i].initial_x, touches[i].initial_y, x, y, width, height)) {
            *touch = touches[i];
            return true;
        }
    }

    return false;
}

void ui::touch_accept(int touch_id) {
    for (int i = 0; i < num_touches; ++i) {
        if (touches[i].id == touch_id) {
            touches[i].flags |= TOUCH_ACCEPTED;
            break;
        }
    }
}

bool ui::touch_ended(int touch_id, ui::Touch* touch) {
    for (int i = 0; i < num_touches; ++i) {
        if (touches[i].id == touch_id) {
            *touch = touches[i];
            return touches[i].flags & TOUCH_ENDED;
        }
    }

    return true;
}

bool ui::simple_tap(float x, float y, float width, float height) {
    for (int i = 0; i < num_touches; ++i) {
        // Looking for a touch that ended, that was never accepted,
        // and that never moved
        if (touches[i].flags == ui::TOUCH_ENDED &&
            touch_inside(touches[i].x, touches[i].y, x, y, width, height)) {
            ui::touch_accept(touches[i].id);
            return true;
        }
    }
    
    return false;
}




// Scroll events
struct ScrollData {
    int x, y;
    int dx, dy;
};

static ScrollData scroll_data = { 0 }; 

void app_scroll_event(int x, int y, int dx, int dy) {
    scroll_data.x = x;
    scroll_data.y = y;
    scroll_data.dx = dx;
    scroll_data.dy = dy;
    ui::redraw();
}

void clear_scroll() {
    scroll_data = { 0 };
}

bool ui::get_scroll(float x, float y, float width, float height, float* dx, float* dy) {
    if ((scroll_data.dx == 0 && scroll_data.dy == 0) ||
        !touch_inside(scroll_data.x, scroll_data.y, x, y, width, height)) {
        return false;
    }

    *dx = scroll_data.dx;
    *dy = scroll_data.dy;
    scroll_data.dx = 0;
    scroll_data.dy = 0;
    return true;
}





// Keyboard
AppKeyEvent key_state;
constexpr int KEY_EVENT_QUEUE_SIZE = 64;
static AppKeyEvent key_event_queue[KEY_EVENT_QUEUE_SIZE];
static long key_event_queue_write = 0;
static long key_event_queue_read = 0;
static float keyboard_y_;
static bool keyboard_is_showing_;

static char text_content_buffer[1024];
static char* text_content_big_buffer = NULL;
static size_t text_content_big_buffer_size = 0;

static ui::TextInput text_input_ = { 0 };
static bool text_input_was_set = false;
static bool text_input_is_set = false;
const ui::TextInput* ui::text_input = &text_input_;

void app_key_event(AppKeyEvent event) {
    if (key_event_queue_write - key_event_queue_read < KEY_EVENT_QUEUE_SIZE) {
        key_event_queue[key_event_queue_write++ % KEY_EVENT_QUEUE_SIZE] = event;
    }
    ui::redraw();
}

static const char* text_content_copy(const char* string) {
    auto len = (int)strlen(string);

    // If it fits in our small buffer
    if (len + 1 <= sizeof(text_content_buffer)) {
        strncpy(text_content_buffer, string, len + 1);
        return text_content_buffer;
    }

    // It's too big for our small buffer, but fits in our big buffer
    if (len + 1 <= text_content_big_buffer_size) {
        strncpy(text_content_big_buffer, string, len + 1);
        return text_content_big_buffer;
    }

    // We're gonna need a bigger buffer!
    if (text_content_big_buffer) {
        free(text_content_big_buffer);
    }
    text_content_big_buffer_size = len * 2;
    text_content_big_buffer = (char*)malloc(text_content_big_buffer_size);
    strncpy(text_content_big_buffer, string, len + 1);
    return text_content_big_buffer;
}

void app_text_input_content_changed(const char* new_content) {
    text_input_.content = text_content_copy(new_content);
    ui::redraw();
}

void app_keyboard_changed(int is_showing, float x, float y, float width, float height) {
    keyboard_is_showing_ = is_showing;
    keyboard_y_ = y;
    ui::redraw();
}

void text_input_begin_frame() {
    text_input_is_set = false;
}

void text_input_end_frame() {
    if (text_input_was_set && !text_input_is_set) {
        platform_remove_text_input();
    } else if (text_input_is_set) {
        AppTextInputConfig config = {
            .x = text_input_.x,
            .y = text_input_.y,
            .width = text_input_.width,
            .height = text_input_.height,
            .font_size = text_input_.font_size,
            .line_height = text_input_.line_height,
            .text_color = text_input_.text_color,
            .flags = text_input_.flags,
            .content = text_input_.content,
        };
        platform_update_text_input(&config);
    }
    text_input_was_set = text_input_is_set;
}

void ui::text_input_set(const ui::TextInput* text_input) {
    text_input_is_set = true;
    bool content_changed = (text_input_.content != text_input->content);
    text_input_ = *text_input;
    if (content_changed) {
        text_input_.content = text_content_copy(text_input_.content);
    }
}

void ui::text_input_clear() {
    text_input_is_set = false;
    text_input_ = { 0 };
}

float ui::keyboard_y() {
    float screen_y;
    if (keyboard_is_showing_) {
        screen_y = keyboard_y_;
    } else {
        screen_y = (num_saved_views == 1) ? view.height : saved_views[1].height;
    }

    float view_x, view_y;
    to_view_point(0, screen_y, &view_x, &view_y);
    return view_y;
}

bool has_key_events_to_process() {
    return (key_event_queue_write > key_event_queue_read);
}

void process_next_key_event() {
    if (key_event_queue_write > key_event_queue_read) {
        key_state = key_event_queue[key_event_queue_read++ % KEY_EVENT_QUEUE_SIZE];
    } else {
        memset(&key_state, 0, sizeof(AppKeyEvent));
    }
}



// External linking
void app::open_url(const char* url) {
    platform_open_url(url);
}



// Storage
const char* app::get_asset_name(const char* asset_name, const char* asset_type) {
    return platform_get_asset_name(asset_name, asset_type);
}
const char* app::get_user_data_path(const char* filename) {
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s/%s", platform_user_data_dir, filename);
    return buf;
}
void app::user_data_flush() {
    platform_user_data_flush();
}
