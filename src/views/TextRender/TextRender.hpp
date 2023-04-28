//
//  TextRender.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-27.
//

#pragma once

#include <app.hpp>
#include "../../utils/stackbuffer.hpp"
#include "../../models/relative.hpp"

namespace TextRender {

struct Attribute {
    int index;
    NVGcolor text_color;
    const char* font_face;
    float font_size;
    float line_spacing;
    int action_id;
};

struct Props {
    Array<const char> data;
    Array<Attribute> attributes;
    float bounding_width;
    float bounding_height;
};

struct Line {
    int run_start;
    int run_end;
    float y;
    float y_baseline;
    float width;
    float height;
};

struct Run {
    int attribute_index;
    int data_start;
    int data_end;
    float x;
    float width;
};

struct State {
    State(StackArray<Line>& lines, StackArray<Run>& runs) : lines(lines), runs(runs) {}

    Array<const char> data;
    Array<Attribute> attributes;
    StackArray<Line>& lines;
    StackArray<Run>& runs;
    float width;
    float height;
};

struct StateFixed : State {
    StateFixed() : State(lines_fixed, runs_fixed) {}

    StackArrayFixed<Line, 4> lines_fixed;
    StackArrayFixed<Run, 32> runs_fixed;
};

void layout(State* state, const Props* props);
void render(const State* state);
int  simple_tap(const State* state);

}
