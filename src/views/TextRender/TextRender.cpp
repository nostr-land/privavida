//
//  TextRender.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-27.
//

#include "TextRender.hpp"
#include <assert.h>

namespace TextRender {


void set_string(Props* props, const char* data, uint32_t length) {
    props->data = Array<const char>(length, data);
}

void set_styles(Props* props, Attribute* attr, NVGcolor text_color, const char* font_face, float font_size, float line_spacing) {
    attr->index = 0;
    attr->text_color = text_color;
    attr->font_face = font_face;
    attr->font_size = font_size;
    attr->line_spacing = line_spacing;
    props->attributes = Array<Attribute>(1, attr);
}

void set_bounds(Props* props, float bounding_width, float bounding_height) {
    props->bounding_width = bounding_width;
    props->bounding_height = bounding_height;
}



constexpr float SPACE_WIDTH = 4.0;

struct Token {

    enum Type {
        TEXT,
        SPACE,
        LINE_BREAK
    };

    Type type;
    int data_start;
    int data_end;
    int attribute_index;
    float space_before;
    float space_after;
};

void layout(State* state, const Props* props) {

    // Step 1. Setup our state
    auto& data            = state->data       = props->data;
    auto& attributes      = state->attributes = props->attributes;
    auto  bounding_width  = props->bounding_width;
    auto  bounding_height = props->bounding_height;
    auto& runs            = state->runs;
    auto& lines           = state->lines;
    auto& width           = state->width  = 0;
    auto& height          = state->height = 0;
    runs.size = 0;
    lines.size = 0;
    if (!data.size) return;
    assert(attributes.size > 0 && attributes[0].index == 0); // You must have at least one style attribute

    // Step 2. Break up the input string into spaces
    Token tokens_on_stack[props->data.size];
    StackArray<Token> tokens(tokens_on_stack, props->data.size);

    int index = 0;
    int start_index = 0;
    int attribute_index = 0;
    while (index < data.size) {

        // If we've arrived at the next attribute, create a TEXT token
        if (attribute_index + 1 < attributes.size &&
            attributes[attribute_index + 1].index == index) {
            if (start_index < index) {
                auto& token = tokens.push_back();
                token.type = Token::TEXT;
                token.data_start = start_index;
                token.data_end = index;
                token.attribute_index = attribute_index;
            }
            ++attribute_index;
            start_index = index;
        }

        // If we've arrived at a space, create a TEXT and SPACE token
        auto ch = data[index];
        if (ch == ' ') {
            if (start_index < index) {
                auto& token = tokens.push_back();
                token.type = Token::TEXT;
                token.data_start = start_index;
                token.data_end = index;
                token.attribute_index = attribute_index;
            }
            {
                auto& token = tokens.push_back();
                token.type = Token::SPACE;
            }
            start_index = index + 1;
            ++index;
            continue;
        }

        // If we've arrived at a new line, create a TEXT and LINE_BREAK token
        if (ch == '\n') {
            if (start_index < index) {
                auto& token = tokens.push_back();
                token.type = Token::TEXT;
                token.data_start = start_index;
                token.data_end = index;
                token.attribute_index = attribute_index;
            }
            {
                auto& token = tokens.push_back();
                token.type = Token::SPACE;
            }
            start_index = index + 1;
            ++index;
            continue;
        }

        ++index;
    }

    // Create a final TEXT token, if any
    if (start_index < index) {
        auto& token = tokens.push_back();
        token.type = Token::TEXT;
        token.data_start = start_index;
        token.data_end = index;
        token.attribute_index = attribute_index;
    }

    // Step 3. Run through tokens again, this time removing SPACE tokens,
    //         and computing the space_before & space_after for each TEXT
    //         token. When TEXT tokens were split because of a SPACE, we
    //         want to give them some spacing. If, however, they were
    //         split because of a change in attribute style, we don't want
    //         to add any spacing.

    int i_src = 0;
    int i_dst = 0;
    for (; i_src < tokens.size; ++i_src) {
        auto token = tokens[i_src];

        if (token.type == Token::TEXT) {
            token.space_before = (i_dst > 0) ? tokens[i_dst - 1].space_after : 0;
            token.space_after = 0;
            tokens[i_dst++] = token;
        } else if (token.type == Token::SPACE) {
            if (i_dst > 0) {
                tokens[i_dst - 1].space_after = SPACE_WIDTH;
            }
        } else if (token.type == Token::LINE_BREAK) {
            if (i_dst > 0) {
                tokens[i_dst - 1].space_after = 0;
            }
            tokens[i_dst++] = token;
        }
    }
    tokens.size = i_dst;

    // Step 4. We've now broken all content up into "tokens", which are
    //         almost like glyph runs, except that some may need to be
    //         broken further due to word wrapping. Now we can truly lay
    //         out each token one at a time to create lines and runs.

    int last_attribute_index = -1;
    float max_line_spacing = 0;
    float ascender, descender, line_height;

    float y = 0;
    int i = 0;
    while (i < tokens.size) {

        int line_start_index = i;
        float x = 0;
        float max_height = 0;
        float max_line_height = 0;
        float max_ascender = 0;
        if (last_attribute_index != -1) {
            max_line_spacing = attributes[last_attribute_index].line_spacing;
        }

        auto& line = lines.push_back();
        line.run_start = (int)runs.size;

        // Fit tokens on one line
        for (; i < tokens.size; ++i) {
            auto& token = tokens[i];

            if (token.type == Token::LINE_BREAK) {
                ++i;
                break;
            }

            // New attribute styling?
            if (token.attribute_index != last_attribute_index) {
                auto& attr = props->attributes[token.attribute_index];
                ui::font_face(attr.font_face);
                ui::font_size(attr.font_size);
                ui::text_metrics(&ascender, &descender, &line_height);
                if (max_line_spacing < attr.line_spacing) {
                    max_line_spacing = attr.line_spacing;
                }
                last_attribute_index = token.attribute_index;
            }

            if (max_line_height < line_height) {
                max_line_height = line_height;
            }
            if (max_ascender < ascender) {
                max_ascender = ascender;
            }
            if (max_height < line_height) {
                max_height = line_height;
            }

            // Measure text
            float bounds[4];
            ui::text_bounds(0, 0, &data[token.data_start], &data[token.data_end], bounds);

            auto width = bounds[2] - bounds[0];

            if (x + width > bounding_width && x == 0) {
                // We've got a TOKEN here that is larger than our allowed
                // size, gotta cut it up more!

                int max_positions = (int)(token.data_end - token.data_start);
                NVGglyphPosition positions[max_positions];
                int num_positions = ui::text_glyph_positions(0, 0, &data[token.data_start], &data[token.data_end], positions, max_positions);

                // Find the longest set of glyphs we can place on this line
                int count = num_positions;
                for (; count > 0; --count) {
                    if (positions[count - 1].minx < bounding_width) {
                        break;
                    }
                }

                // Turn this into a run
                auto& run = runs.push_back();
                run.attribute_index = token.attribute_index;
                run.data_start = token.data_start;
                run.data_end = (int)(positions[count].str - data.data);
                run.x = 0;
                run.width = positions[count].minx;
                x = run.width;

                // Mutate our token to contain what is left over
                token.data_start = run.data_end;
                token.space_before = 0;
                break;

            } else if (x + width > bounding_width) {
                break;
            }

            // It's a Run!
            auto& run = runs.push_back();
            run.attribute_index = token.attribute_index;
            run.data_start = token.data_start;
            run.data_end = token.data_end;
            run.x = x;
            run.width = width;

            x += width + token.space_after;
        }

        // Calculate baseline
        float baseline = (max_height - max_line_height) / 2 + max_ascender;

        line.y = y;
        line.y_baseline = y + baseline;
        line.run_end = (int)runs.size;
        line.width = x;
        line.height = max_height;

        // Check if we've exceeded our bounding height
        if (y + max_height > bounding_height) {
            runs.size = line.run_start;
            --lines.size;
            break;
        }

        y += max_height + max_line_spacing;

        if (width < line.width) {
            width = line.width;
        }
    }

    height = y - max_line_spacing;
}

void render(const State* state) {
    ui::text_align(NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    int last_attribute_index = -1;

    for (auto& line : state->lines) {

        for (int i = line.run_start; i < line.run_end; ++i) {
            auto& run = state->runs[i];

            if (run.attribute_index != last_attribute_index) {
                auto& attr = state->attributes[run.attribute_index];
                nvgFillColor(ui::vg, attr.text_color);
                ui::font_face(attr.font_face);
                ui::font_size(attr.font_size);
                last_attribute_index = run.attribute_index;
            }

            ui::text(run.x, line.y_baseline, &state->data[run.data_start], &state->data[run.data_end]);
        }
    }
}

int simple_tap(const State* state) {
    for (auto& line : state->lines) {

        for (int i = line.run_start; i < line.run_end; ++i) {
            auto& run = state->runs[i];
            auto& attr = state->attributes[run.attribute_index];
            if (attr.action_id != -1) {
                if (ui::simple_tap(run.x, line.y, run.width, line.height)) {
                    return attr.action_id;
                }
            }
        }
    }

    return -1;
}

}
