//
//  LoginView.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 06/05/2023.
//

#include "LoginView.hpp"
#include <app.hpp>
#include "SubView.hpp"
#include "TextInput/TextInput.hpp"
#include "../theme.hpp"
#include "Root.hpp"
#include "../models/nostr_entity.hpp"
#include "../data_layer/accounts.hpp"

constexpr float VERTICAL_SPACING = 16;
constexpr float MARGIN = 24;
constexpr float ELEMENT_HEIGHT = 32;

static TextInput::State text_state;

void LoginView::update() {

    // Background
    nvgBeginPath(ui::vg);
    nvgFillColor(ui::vg, COLOR_SUBDUED);
    nvgRect(ui::vg, 0, 0, ui::view.width, ui::view.height);
    nvgFill(ui::vg);

    float y = 200.0;

    // Header text
    {
        nvgFillColor(ui::vg, ui::color(0xffffff));
        ui::text_align(NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        ui::font_face("regular");
        ui::font_size(14.0);
        ui::text(0.5 * ui::view.width, y, "Sign in to PrivaVida", NULL);

        float ascender, descender, line_height;
        ui::text_metrics(&ascender, &descender, &line_height);
        y += line_height + VERTICAL_SPACING;
    }

    // Text input
    {
        SubView sv(MARGIN, y, ui::view.width - 2 * MARGIN, ELEMENT_HEIGHT);

        auto styles = *TextInput::get_global_styles();
        styles.padding = 8;
        styles.border_radius = 0.5 * ui::view.height;
        styles.border_width = 2;
        styles.font_size = 18;
        styles.text_color = ui::color(0xffffff);
        styles.border_color = COLOR_SECONDARY;
        styles.border_color_focused = COLOR_PRIMARY;
        styles.bg_color = COLOR_BACKGROUND;
        styles.bg_color_focused = COLOR_BACKGROUND;

        TextInput(&text_state)
            .set_styles(&styles)
            .set_flags(ui::APP_TEXT_FLAGS_TYPE_PASSWORD)
            .update();

        y += ELEMENT_HEIGHT + VERTICAL_SPACING;
    }

    // Check text input
    const char* message = TextInput::get_text(&text_state);
    if (message) {
        NostrEntity entity;
        auto message_len = (uint32_t)strlen(message);
        auto entity_size = NostrEntity::decoded_size(message, message_len);
        if (entity_size == sizeof(NostrEntity) && NostrEntity::decode(&entity, message, message_len)) {

            bool success = false;
            if (entity.type == NostrEntity::NPUB) {
                success = data_layer::open_account_with_pubkey(&entity.pubkey);
            } else if (entity.type == NostrEntity::NSEC) {
                success = data_layer::open_account_with_seckey(&entity.seckey);
            }

            // Clear the entity, in case we've decoded an nsec, don't want to spill that
            memset(&entity, 0, sizeof(NostrEntity));

            if (success) {
                TextInput::dismiss(&text_state);
                Root::pop_view();
            }
        }
    }

    // Dismiss ?
    if (data_layer::current_account() && ui::simple_tap(0, 0, ui::view.width, ui::view.height)) {
        Root::pop_view();
    }
}
