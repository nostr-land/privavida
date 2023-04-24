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

    network::fetch(url,
        [image_idx](bool err, int status_code, const uint8_t* data, uint32_t len) {
            auto& image = images[image_idx];
            printf("Fetch: %s - error %d - status %d - len %d\n", image.url, err, status_code, len);

            if (err || status_code != 200 || len == 0 || len > 250000) {
                image.state = Image::ERROR;
                return;
            }

            image.image_id = nvgCreateImageMem(ui::vg, 0, const_cast<uint8_t*>(data), len);
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
