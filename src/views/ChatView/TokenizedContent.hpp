//
//  TokenizedContent.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-21.
//

#pragma once

#include <app.hpp>
#include <functional>
#include <vector>
#include <string>

namespace TokenizedContent {

struct Token {

    enum Type {
        TEXT,
        OBJECT,
        LINE_BREAK
    };

    Type type;
    float space_before, space_after;
    int action_id;

    // type = TEXT
    struct {
        NVGcolor font_color;
        float font_size;
        const char* font_face;
        std::string content;
    } text;

    // type = OBJECT
    struct {
        float width;
        float height;
        std::function<void()> update;
    } object;

};

struct State {
    std::vector<Token> content_tokens;
    int action;

    struct {
        NVGcolor font_color;
        float font_size;
        const char* font_face;
    } settings;
};

void set_font_settings(State* state, NVGcolor font_color, float font_size, const char* font_face);
void tokenize_and_append_text(State* state, std::string text, int action_id = -1);
void tokenize_and_append_text(State* state, float space_before, float space_after, std::string text, int action_id = -1);
void append_object(State* state, float width, float height, std::function<void()> update, int action_id = -1);
void append_object(State* state, float space_before, float space_after, float width, float height, std::function<void()> update, int action_id = -1);

void measure_content(State* state, float total_width, float* width, float* height);
void update(State* state);

}
