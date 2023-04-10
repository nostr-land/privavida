//
//  SubView.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 10/05/2023.
//

#pragma once

#include "../ui.hpp"

struct SubView {
    SubView(float x, float y, float width, float height) {
        ui::sub_view(x, y, width, height);
    }
    SubView(float x, float y) : SubView(x, y, ui::view.width - x, ui::view.height - y) {}
    ~SubView() {
        ui::restore();
    }
};
