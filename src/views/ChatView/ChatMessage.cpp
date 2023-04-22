//
//  ChatMessage.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 15/08/2018.
//

#include "ChatMessage.hpp"
#include "../SubView.hpp"
#include "../Root.hpp"
#include "../../data_layer/accounts.hpp"

extern "C" {
#include "../../../lib/nanovg/stb_image.h"
}

constexpr auto HORIZONTAL_MARGIN = 14;
constexpr auto VERTICAL_MARGIN = 1;
constexpr auto PADDING = 12;
constexpr auto SPACING = 10;
constexpr auto BUBBLE_CORNER_RADIUS = 17;
constexpr auto BUBBLE_COLOR_SENDER    = (NVGcolor){ 72/255.0, 140/255.0, 247/255.0, 1.0 };
constexpr auto BUBBLE_COLOR_RECIPIENT = (NVGcolor){ 38/255.0,  37/255.0,  42/255.0, 1.0 };

static bool author_is_me(const Event* event) {
    return compare_keys(&event->pubkey, &data_layer::accounts[0].pubkey);
}

static bool a_moment_passed(const Event* earlier, const Event* later) {
    return (later->created_at - earlier->created_at) > 5 * 60; // 5 minutes
}

enum BubbleTipType {
    BUBBLE_TIP_SENDER,
    BUBBLE_TIP_RECIPIENT
};

static int get_bubble_tip(BubbleTipType tip_type, float* width, float* height);

ChatMessage ChatMessage::create(const Event* event) {
    ChatMessage message;

    message.event = event;
    
    // Prepare TokenizedContent
    auto tc = &message.tokenized_content;

    if (event->content_encryption == EVENT_CONTENT_DECRYPTED) {
        TokenizedContent::set_font_settings(tc, (NVGcolor){ 1.0, 1.0, 1.0, 1.0 }, 18.0, "regular");
        TokenizedContent::tokenize_and_append_text(tc, event->content.data.get(event));
    } else {
        TokenizedContent::set_font_settings(tc, (NVGcolor){ 0.8, 0.3, 0.4, 1.0 }, 18.0, "regular");
        TokenizedContent::tokenize_and_append_text(tc, "Failed to decrypt");
    }

    return message;
}

float ChatMessage::measure_height(float width, const Event* event_before, const Event* event_after) {

    space_above = !event_before;
    space_below = (
        !event_after ||
        author_is_me(event) != author_is_me(event_after) ||
        a_moment_passed(event, event_after)
    );

    float max_bubble_width = width - 2 * HORIZONTAL_MARGIN - 0.2 * ui::view.width;
    float max_content_width = max_bubble_width - 2 * PADDING;

    float content_height;
    TokenizedContent::measure_content(&tokenized_content, max_content_width, &content_width, &content_height);

    float bubble_height = content_height + 2 * PADDING + (space_above ? SPACING : 0) + (space_below ? SPACING : 0);
    return bubble_height + VERTICAL_MARGIN;
}

void ChatMessage::update() {

    float bubble_height = ui::view.height - VERTICAL_MARGIN - (space_above ? SPACING : 0) - (space_below ? SPACING : 0);
    float bubble_width = content_width + 2 * PADDING;
    float bubble_x = author_is_me(event) ? ui::view.width - HORIZONTAL_MARGIN - bubble_width : HORIZONTAL_MARGIN;
    float bubble_y = (space_above ? SPACING : 0);
    float content_x = bubble_x + PADDING;
    float content_y = bubble_y + PADDING;
    float content_height = bubble_height - 2 * PADDING;

    if (event->content_encryption == EVENT_CONTENT_DECRYPTED && author_is_me(event)) {
        nvgFillColor(ui::vg, BUBBLE_COLOR_SENDER);
    } else {
        nvgFillColor(ui::vg, BUBBLE_COLOR_RECIPIENT);
    }

    nvgBeginPath(ui::vg);
    nvgRoundedRect(ui::vg, bubble_x, bubble_y, bubble_width, bubble_height, BUBBLE_CORNER_RADIUS);
    nvgFill(ui::vg);

    // Render the little speech bubble flick
    
    if (space_below) {
        nvgSave(ui::vg);
        float width, height;
        if (author_is_me(event)) {
            int img_id = get_bubble_tip(BUBBLE_TIP_SENDER, &width, &height);
            nvgTranslate(ui::vg, bubble_x + bubble_width - 0.5 * width, bubble_y + bubble_height - height);
            nvgFillPaint(ui::vg, nvgImagePattern(ui::vg, 0, 0, width, height, 0, img_id, 1));
        } else {
            int img_id = get_bubble_tip(BUBBLE_TIP_RECIPIENT, &width, &height);
            nvgTranslate(ui::vg, bubble_x + 0.5 * width, bubble_y + bubble_height - height);
            nvgScale(ui::vg, -1.0, 1.0);
            nvgFillPaint(ui::vg, nvgImagePattern(ui::vg, 0, 0, width, height, 0, img_id, 1));
        }
        nvgBeginPath(ui::vg);
        nvgRect(ui::vg, 0, 0, width, height);
        nvgFill(ui::vg);

        // Originally I was just drawing a path for the bubble tip, which is
        // much nicer, but for some reason paths are broken when using the WebGL
        // backend. So images will do...
        // nvgMoveTo(ui::vg, 0, 0);
        // nvgLineTo(ui::vg, 0, 6);
        // nvgBezierTo(ui::vg, 0, 10.5, 4.5, 14, 7, 16);
        // nvgBezierTo(ui::vg, 7, 16, -2.5, 17, -9, 12);
        // nvgClosePath(ui::vg);
        // nvgFill(ui::vg);
        nvgRestore(ui::vg);
    }

    {
        SubView sv(content_x, content_y, content_width, content_height);
        TokenizedContent::update(&tokenized_content);
    }

    if (ui::simple_tap(bubble_x, bubble_y, bubble_width, bubble_height)) {
        Root::push_view_message_inspect(event);
    }

}

int get_bubble_tip(BubbleTipType tip_type, float* width, float* height) {

    // Load the bubble tip asset (if not already loaded)
    static bool did_load = false;
    static uint8_t* img;
    static int x, y, comp;
    if (!did_load) {
        did_load = true;
        img = stbi_load(ui::get_asset_name("bubble-tip", "png"), &x, &y, &comp, 4);
    }
    if (!img) {
        *width = *height = 1;
        return -1;
    }
    *width  = 0.5 * x;
    *height = 0.5 * y;

    static int img_id_sender = -1;
    static int img_id_recipient = -1;

    // Fetch the image id (if already loaded)
    int& img_id = tip_type == BUBBLE_TIP_SENDER ? img_id_sender : img_id_recipient;
    if (img_id != -1) return img_id;

    // Image has not been created, create it now
    auto color = tip_type == BUBBLE_TIP_SENDER ? BUBBLE_COLOR_SENDER : BUBBLE_COLOR_RECIPIENT;
    uint8_t color_r = (uint8_t)(color.r * 255);
    uint8_t color_g = (uint8_t)(color.g * 255);
    uint8_t color_b = (uint8_t)(color.b * 255);
    for (int i = 0; i < x * y * comp; i += comp) {
        img[i+0] = color_r;
        img[i+1] = color_g;
        img[i+2] = color_b;
    }
    img_id = nvgCreateImageRGBA(ui::vg, x, y, 0, img);
    return img_id;

}
