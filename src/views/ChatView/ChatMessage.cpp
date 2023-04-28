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
#include "../../models/nip31.hpp"
#include <time.h>

extern "C" {
#include "../../../lib/nanovg/stb_image.h"
}

constexpr auto HORIZONTAL_MARGIN = 14;
constexpr auto VERTICAL_MARGIN = 1;
constexpr auto HORIZONTAL_PADDING = 12;
constexpr auto VERTICAL_PADDING = 10;
constexpr auto SPACING = 10;
constexpr auto BUBBLE_CORNER_RADIUS = 16; //17;
constexpr auto TIME_SPACING_Y = 4.0;
constexpr auto TIME_SPACING_X = 6.0;

static bool author_is_me(const Event* event) {
    return compare_keys(&event->pubkey, &data_layer::accounts[0].pubkey);
}

static bool a_moment_passed(const Event* earlier, const Event* later) {
    return (later->created_at - earlier->created_at) > 5 * 60; // 5 minutes
}

enum BubbleTipType {
    BUBBLE_TIP_SENDER,
    BUBBLE_TIP_SENDER_FAIL,
    BUBBLE_TIP_RECIPIENT
};

static int get_bubble_tip(BubbleTipType tip_type, float* width, float* height);

static int max(int a, int b) {
    return (a < b) ? b : a;
}

ChatMessage* ChatMessage::create(const Event* event) {
    auto message = new ChatMessage;
    message->event = event;

    char text_content[max(100, event->content.size + 1)];

    TextRender::Attribute default_attr;
    default_attr.index = 0;
    default_attr.text_color = ui::color(0xffffff);
    default_attr.font_face = "regular";
    default_attr.font_size = 17.0;
    default_attr.line_spacing = 3.0;
    default_attr.action_id = -1;

    // Create our text_content
    if (event->content_encryption != EVENT_CONTENT_DECRYPTED) {
        strcpy(text_content, "Failed to decrypt");

        default_attr.text_color = COLOR_ERROR;
        message->text_attrs.push_back(default_attr);

    } else {

        int offset = 0;
        auto tokens = event->content_tokens.get(event);
        for (int i = 0; i < tokens.size; ++i) {
            auto& token = tokens[i];
            auto& attr = message->text_attrs.push_back();
            attr = default_attr;
            attr.index = offset;

            if (token.type == EventContentToken::ENTITY) {
                auto entity = token.entity.get(event);
                
                if (entity->type == NostrEntity::NPUB ||
                    entity->type == NostrEntity::NOTE) {
                    char data[200];
                    uint32_t len;
                    NostrEntity::encode(entity, data, &len);
                    data[12] = '\0';

                    sprintf(&text_content[offset], "@%s:%s", &data[0], &data[len - 8]);
                    offset += (int)strlen(&text_content[offset]);

                    attr.text_color = ui::color(0xffffff, 0.8);
                    if (entity->type == NostrEntity::NPUB ||
                        entity->type == NostrEntity::NOTE) {
                        attr.action_id = i;
                    }
                    continue;
                } else if (entity->type == NostrEntity::NINVITE) {
                    if (entity->invite_signature_state == INVITE_SIGNATURE_NOT_CHECKED) {
                        nip31_verify_invite(const_cast<NostrEntity*>(entity), &event->pubkey);
                    }
                    if (entity->invite_signature_state == INVITE_SIGNATURE_VALID) {
                        sprintf(&text_content[offset], "<INVITATION VALID>");
                    } else {
                        sprintf(&text_content[offset], "<INVITATION INVALID>");
                    }
                    offset += (int)strlen(&text_content[offset]);

                    attr.text_color = COLOR_ERROR;
                    attr.font_face = "bold";
                    continue;
                }
            }

            if (token.type == EventContentToken::NIP08_MENTION) {
                NostrEntity entity;
                bool found = false;
                for (auto& p_tag : event->p_tags.get(event)) {
                    if (p_tag.index == token.nip08_mention_index) {
                        entity.type = NostrEntity::NPUB;
                        entity.pubkey = p_tag.pubkey;
                        found = true;
                        break;
                    }
                }
                for (auto& e_tag : event->e_tags.get(event)) {
                    if (e_tag.index == token.nip08_mention_index) {
                        entity.type = NostrEntity::NOTE;
                        entity.event_id = e_tag.event_id;
                        found = true;
                        break;
                    }
                }
                
                if (found) {
                    char data[200];
                    uint32_t len;
                    NostrEntity::encode(&entity, data, &len);
                    data[12] = '\0';

                    sprintf(&text_content[offset], "@%s:%s", &data[0], &data[len - 8]);
                    offset += (int)strlen(&text_content[offset]);

                    attr.text_color = ui::color(0xffffff, 0.8);
                    attr.action_id = i;
                    continue;
                }
            }

            strncpy(&text_content[offset], token.text.data.get(event), token.text.size);
            offset += token.text.size;
                
            if (token.type == EventContentToken::URL) {
                attr.text_color = ui::color(0xffffff, 0.8);
                attr.action_id = i;
            }
        }
        text_content[offset] = '\0';
    }

    // Copy our text content into the message
    message->text_content.reserve(strlen(text_content) + 1);
    message->text_content.size = strlen(text_content);
    strcpy(message->text_content.begin(), text_content);

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
    float max_content_width = max_bubble_width - 2 * HORIZONTAL_PADDING;

    TextRender::Props props;
    props.data = Array<const char>((uint32_t)text_content.size, &text_content[0]);
    props.attributes = Array<TextRender::Attribute>((uint32_t)text_attrs.size, &text_attrs[0]);
    props.bounding_width = max_content_width;
    props.bounding_height = 1E10;

    TextRender::State state(text_lines, text_runs);
    TextRender::layout(&state, &props);

    content_width = state.width;
    float content_height = state.height;

    float time_width, time_height;
    {
        char time_string[16];
        auto created_at = (time_t)event->created_at;
        struct tm *t = localtime(&created_at);
        strftime(time_string, sizeof(time_string), "%H:%M", t);
        ui::font_size(10.0);
        float bounds[4];
        ui::text_bounds(0, 0, time_string, NULL, bounds);
        time_width = bounds[2] - bounds[0];
        time_height = bounds[3] - bounds[1];
        
        if (!text_lines.size) {
            // No-op
        } else if (text_lines.size == 1 && (content_width + time_width + TIME_SPACING_X) < max_content_width) {
            content_width += time_width + TIME_SPACING_X;
        } else if (content_width - text_lines.back().width < time_width) {
            content_height += time_height + TIME_SPACING_Y;
        }
    }

    float bubble_height = content_height + 2 * VERTICAL_PADDING + (space_above ? SPACING : 0) + (space_below ? SPACING : 0);
    return bubble_height + VERTICAL_MARGIN;
}

void ChatMessage::update() {

    float bubble_height = ui::view.height - VERTICAL_MARGIN - (space_above ? SPACING : 0) - (space_below ? SPACING : 0);
    float bubble_width = content_width + 2 * HORIZONTAL_PADDING;
    float bubble_x = author_is_me(event) ? ui::view.width - HORIZONTAL_MARGIN - bubble_width : HORIZONTAL_MARGIN;
    float bubble_y = (space_above ? SPACING : 0);
    float content_x = bubble_x + HORIZONTAL_PADDING;
    float content_y = bubble_y + VERTICAL_PADDING;
    float content_height = bubble_height - 2 * VERTICAL_PADDING;

    BubbleTipType tip_type;
    if (event->content_encryption == EVENT_CONTENT_DECRYPTED && author_is_me(event)) {
        nvgFillColor(ui::vg, COLOR_PRIMARY);
        tip_type = BUBBLE_TIP_SENDER;
    } else {
        nvgFillColor(ui::vg, COLOR_SECONDARY);
        tip_type = BUBBLE_TIP_RECIPIENT;
    }

    nvgBeginPath(ui::vg);
    nvgRoundedRect(ui::vg, bubble_x, bubble_y, bubble_width, bubble_height, BUBBLE_CORNER_RADIUS);
    nvgFill(ui::vg);

    // Render the little speech bubble flick
    
    if (space_below) {
        nvgSave(ui::vg);
        float width, height;
        if (author_is_me(event)) {
            int img_id = get_bubble_tip(tip_type, &width, &height);
            nvgTranslate(ui::vg, bubble_x + bubble_width - 0.5 * width, bubble_y + bubble_height - height);
            nvgFillPaint(ui::vg, nvgImagePattern(ui::vg, 0, 0, width, height, 0, img_id, 1));
        } else {
            int img_id = get_bubble_tip(tip_type, &width, &height);
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

        TextRender::State state(text_lines, text_runs);
        state.data = Array<const char>((uint32_t)text_content.size, &text_content[0]);
        state.attributes = Array<TextRender::Attribute>((uint32_t)text_attrs.size, &text_attrs[0]);

        TextRender::render(&state);
        int action_id = TextRender::simple_tap(&state);
        if (action_id != -1) {
            auto& token = event->content_tokens.get(event, action_id);
            if (token.type == EventContentToken::URL) {
                char url[token.text.size + 1];
                strncpy(url, token.text.data.get(event), token.text.size);
                url[token.text.size] = '\0';
                app::open_url(url);
            } else {
                NostrEntity entity;

                if (token.type == EventContentToken::ENTITY) {
                    entity = *token.entity.get(event);
                } else {
                    for (auto& p_tag : event->p_tags.get(event)) {
                        if (p_tag.index == token.nip08_mention_index) {
                            entity.type = NostrEntity::NPUB;
                            entity.pubkey = p_tag.pubkey;
                            break;
                        }
                    }
                    for (auto& e_tag : event->e_tags.get(event)) {
                        if (e_tag.index == token.nip08_mention_index) {
                            entity.type = NostrEntity::NOTE;
                            entity.event_id = e_tag.event_id;
                            break;
                        }
                    }
                }

                char data[200];
                uint32_t len;
                NostrEntity::encode(&entity, data, &len);

                const char* prefix = "https://iris.to/";
                char url[len + strlen(prefix) + 1];
                snprintf(url, len + strlen(prefix) + 1, "%s%s", prefix, data);
                app::open_url(url);
            }
        }
    }

    {
        char time_string[16];
        auto created_at = (time_t)event->created_at;
        struct tm *t = localtime(&created_at);
        strftime(time_string, sizeof(time_string), "%H:%M", t);
        ui::font_size(10.0);

        float bounds[4];
        ui::text_bounds(0, 0, time_string, NULL, bounds);

        float time_width  = bounds[2] - bounds[0];
        float time_height = bounds[3] - bounds[1];

        nvgFillColor(ui::vg, ui::color(0xffffff, 0.7));
        ui::text_align(NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        ui::text(content_x + content_width, content_y + content_height, time_string, NULL);
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
        img = stbi_load(app::get_asset_name("bubble-tip", "png"), &x, &y, &comp, 4);
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
    auto color = tip_type == BUBBLE_TIP_SENDER ? COLOR_PRIMARY : COLOR_SECONDARY;
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
