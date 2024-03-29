//
//  images.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "images.hpp"
#include "../network/network.hpp"
#include <stdio.h>
#include <app.hpp>

namespace data_layer {

static std::vector<Image> images;

int get_image(const char* url) {
    int width, height;
    return get_image(url, &width, &height);
}

int get_image(const char* url, int* width, int* height) {
    for (auto& image : images) {
        if (strcmp(image.url, url) == 0) {
            if (image.state == Image::LOADED) {
                *width = image.width;
                *height = image.height;
                return image.image_id;
            } else {
                return 0;
            }
        }
    }

    int image_idx = (int)images.size();
    images.push_back(Image());
    auto& image = images[image_idx];
    image.url = url;
    image.state = Image::LOADING;

    network::fetch_image(url,
        [image_idx](int image_id) {
            auto& image = images[image_idx];
            image.image_id = image_id;
            if (!image.image_id) {
                image.state = Image::ERROR;
            } else {
                nvgImageSize(ui::vg, image.image_id, &image.width, &image.height);
                image.state = Image::LOADED;
                ui::redraw();
            }
        }
    );

    return 0;
}

}
