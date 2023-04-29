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
#include "../../utils/icons.hpp"
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

static NVGcolor interpolate_colors(NVGcolor a, NVGcolor b, float i) {
    NVGcolor c;
    c.r = a.r + (b.r - a.r) * i;
    c.g = a.g + (b.g - a.g) * i;
    c.b = a.b + (b.b - a.b) * i;
    c.a = a.a + (b.a - a.a) * i;
    return c;
}

enum BubbleTipType {
    BUBBLE_TIP_SENDER,
    BUBBLE_TIP_SENDER_FAIL,
    BUBBLE_TIP_RECIPIENT
};

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

    // Create the gradient colour for the bubble background
    NVGcolor gradient_top_color, gradient_bottom_color;
    float gradient_top_y, gradient_bottom_y;
    {
        if (event->content_encryption == EVENT_CONTENT_DECRYPTED && author_is_me(event)) {
            gradient_top_color = gradient_bottom_color = COLOR_PRIMARY;
        } else {
            gradient_top_color = gradient_bottom_color = COLOR_SECONDARY;
        }

        // Lighten the bottom colour
        gradient_bottom_color.r *= 1.4;
        gradient_bottom_color.g *= 1.4;
        gradient_bottom_color.b *= 1.4;

        // We need the absolute screen coordinates for this
        float x, y, width, height;
        ui::to_view_rect(0, 0, ui::screen.width, ui::screen.height, &x, &y, &width, &height);
        gradient_top_y = y;
        gradient_bottom_y = y + height;

        auto paint = nvgLinearGradient(ui::vg, 0, gradient_top_y, 0, gradient_bottom_y, gradient_top_color, gradient_bottom_color);
        nvgFillPaint(ui::vg, paint);
    }

    // Render the background
    nvgBeginPath(ui::vg);
    nvgRoundedRect(ui::vg, bubble_x, bubble_y, bubble_width, bubble_height, BUBBLE_CORNER_RADIUS);
    nvgFill(ui::vg);

    // Render the little speech bubble tip
    if (space_below) {
        float tip_y = bubble_y + bubble_height;
        float tip_x = author_is_me(event) ? bubble_x + bubble_width : bubble_x;
        auto  icon  = author_is_me(event) ? ui::ICON_BUBBLE_TIP_RIGHT : ui::ICON_BUBBLE_TIP_LEFT;

        float ratio = (tip_y - gradient_top_y) / (gradient_bottom_y - gradient_top_y);
        auto  color = interpolate_colors(gradient_top_color, gradient_bottom_color, ratio);

        ui::font_face("icons");
        ui::font_size(BUBBLE_CORNER_RADIUS);
        nvgFillColor(ui::vg, color);
        ui::text(tip_x, tip_y, icon, NULL);
    }

    // Render the text content
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

    // Render the message time
    {
        char time_string[16];
        auto created_at = (time_t)event->created_at;
        struct tm *t = localtime(&created_at);
        strftime(time_string, sizeof(time_string), "%H:%M", t);
        ui::font_face("regular");
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
