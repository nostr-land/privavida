//
//  Composer.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once

#include "../TextInput/TextInput.hpp"

struct Composer {
    TextInput::State text_state;

    float height() const;
    const char* update();
};
