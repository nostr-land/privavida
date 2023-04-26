//
//  network.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include <app.hpp>
#include <functional>

namespace network {

void init();
void send(const char* message);

typedef std::function<void(bool error, int status_code, const uint8_t* data, uint32_t data_length)> FetchCallback;
void fetch(const char* url, FetchCallback callback);

typedef std::function<void(int image_id)> FetchImageCallback;
void fetch_image(const char* url, FetchImageCallback callback);

}
