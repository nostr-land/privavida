//
//  network.hpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#pragma once
#include "../models/event.hpp"
#include "subscription.hpp"
#include <app.hpp>
#include <functional>

namespace network {

void relay_add_task_request(int32_t relay_id, const Filters* filters);
void relay_add_task_stream(int32_t relay_id,  const Filters* filters);
void relay_add_task_publish(int32_t relay_id, const Event* event);
void stop_all_tasks();

typedef std::function<void(bool error, int status_code, const uint8_t* data, uint32_t data_length)> FetchCallback;
void fetch(const char* url, FetchCallback callback);

typedef std::function<void(int image_id)> FetchImageCallback;
void fetch_image(const char* url, FetchImageCallback callback);

}
