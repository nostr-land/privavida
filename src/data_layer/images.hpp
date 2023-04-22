//
//  images.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#pragma once
#include <vector>

namespace data_layer {

struct Image {

    enum LoadedState {
        LOADING,
        LOADED,
        ERROR
    };

    const char* url;
    LoadedState state;
    int image_id;
    int width;
    int height;
};

int get_image(const char* url);
int get_image(const char* url, int* width, int* height);

}
