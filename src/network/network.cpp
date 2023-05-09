//
//  network.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 20/07/2023.
//

#include "network.hpp"
#include <app.hpp>
#include <platform.h>
#include "../models/event_parse.hpp"
#include "../models/event_stringify.hpp"
#include "../models/relay_message.hpp"
#include "../models/client_message.hpp"
#include "../models/hex.hpp"
#include "../data_layer/events.hpp"
#include "../data_layer/relays.hpp"
#include <string.h>
#include <memory>

constexpr int MAX_CONCURRENT_REQUESTS_PER_RELAY = 3;

struct RelayTask {

    enum Type {
        REQUEST,
        STREAM,
        PUBLISH
    };

    enum State {
        QUEUED,
        WAITING_FOR_CONNECTION,
        READY,
        ACTIVE,
        COMPLETED
    };

    Type type;
    State state;
    RelayId relay_id;
    char subscription_id[65];
    std::unique_ptr<Filters> filters;
    std::unique_ptr<Event> event;
};

struct RelayConnection {

    enum State {
        CONNECTING,
        OPEN,
        CLOSED
    };

    RelayId relay_id;
    State state;
    AppWebsocketHandle socket;
    int32_t num_concurrent_requests;
};

static std::vector<RelayConnection> connections;
static std::vector<RelayTask> tasks;

static void process_tasks();
static void generate_new_subscription_id(RelayTask* task) {
    static int next_sub_id = 0;
    memset(task->subscription_id, 0, sizeof(RelayTask::subscription_id));
    snprintf(task->subscription_id, sizeof(RelayTask::subscription_id), "sub%d", next_sub_id++);
}
static RelayConnection* get_connection_for_relay(int32_t relay_id) {
    for (auto& conn : connections) {
        if (conn.relay_id == relay_id) {
            return &conn;
        }
    }

    return NULL;
}
static RelayConnection& get_or_create_connection_for_relay(int32_t relay_id) {
    auto conn = get_connection_for_relay(relay_id);
    if (conn) {
        return *conn;
    }

    auto relay_info = data_layer::get_relay_info(relay_id);
    assert(relay_info);

    connections.push_back(RelayConnection());
    auto& new_conn = connections.back();
    new_conn.relay_id = relay_id;
    new_conn.state = RelayConnection::CONNECTING;
    new_conn.socket = platform_websocket_open(relay_info->url.data.get(relay_info), NULL);
    new_conn.num_concurrent_requests = 0;
    return new_conn;
}
static RelayTask* get_task_for_subscription_id(const char* subscription_id) {
    for (auto& task : tasks) {
        if (strcmp(task.subscription_id, subscription_id) == 0) {
            return &task;
        }
    }
    return NULL;
}
static RelayTask* get_task_for_event_id(const EventId* event_id) {
    for (auto& task : tasks) {
        if (task.type == RelayTask::PUBLISH && compare_keys(&task.event->id, event_id)) {
            return &task;
        }
    }
    return NULL;
}

void network::relay_add_task_request(RelayId relay_id, const Filters* filters) {
    auto filters_copy = (Filters*)malloc(Filters::size_of(filters));
    memcpy(filters_copy, filters, Filters::size_of(filters));

    tasks.push_back(RelayTask());
    auto& task = tasks.back();
    task.relay_id = relay_id;
    task.type = RelayTask::REQUEST;
    task.state = RelayTask::QUEUED;
    task.filters = std::unique_ptr<Filters>(filters_copy);
    generate_new_subscription_id(&task);

    process_tasks();
}

void network::relay_add_task_stream(RelayId relay_id, const Filters* filters) {
    auto filters_copy = (Filters*)malloc(Filters::size_of(filters));
    memcpy(filters_copy, filters, Filters::size_of(filters));
    filters_copy->limit = 0;

    tasks.push_back(RelayTask());
    auto& task = tasks.back();
    task.relay_id = relay_id;
    task.type = RelayTask::STREAM;
    task.state = RelayTask::QUEUED;
    task.filters = std::unique_ptr<Filters>(filters_copy);
    generate_new_subscription_id(&task);

    process_tasks();
}

void network::relay_add_task_publish(RelayId relay_id, const Event* event) {
    auto event_copy = (Event*)malloc(Event::size_of(event));
    memcpy(event_copy, event, Event::size_of(event));

    tasks.push_back(RelayTask());
    auto& task = tasks.back();
    task.relay_id = relay_id;
    task.type = RelayTask::PUBLISH;
    task.state = RelayTask::QUEUED;
    task.event = std::unique_ptr<Event>(event_copy);
    task.subscription_id[0] = '\0';

    process_tasks();
}

void network::stop_all_tasks() {
    // On any active tasks we want to unsubscribe
    for (auto& task : tasks) {
        if (task.state != RelayTask::ACTIVE) {
            task.state = RelayTask::COMPLETED;
            continue;
        }

        auto conn = get_connection_for_relay(task.relay_id);
        if (!conn) {
            continue;
        }

        switch (task.type) {
            case RelayTask::REQUEST:
            case RelayTask::STREAM: {
                StackBufferFixed<64> req_buffer;
                auto req = client_message_close(task.subscription_id, &req_buffer);
                printf("Request: %s\n", req);
                platform_websocket_send(conn->socket, req);
                break;
            }
            case RelayTask::PUBLISH: {
                // @TODO report a failure to send this event
                break;
            }
        }
        task.state = RelayTask::COMPLETED;
    }

    process_tasks();
}

void process_tasks() {

    StackBufferFixed<1024> req_buffer;

    // For QUEUED tasks:
    //     If the tasks are STREAM or PUBLISH events we
    //     bump them automatically to WAITING_FOR_CONNECTION.
    //     If the tasks are REQUEST events we want to only
    //     bump them when the relay is not at its concurrent
    //     requests limit.
    for (auto& task : tasks) {
        if (task.state != RelayTask::QUEUED) continue;

        switch (task.type) {
            case RelayTask::STREAM:
            case RelayTask::PUBLISH: {
                task.state = RelayTask::WAITING_FOR_CONNECTION;
                break;
            }
            case RelayTask::REQUEST: {
                auto conn = get_connection_for_relay(task.relay_id);
                if (!conn || conn->num_concurrent_requests < MAX_CONCURRENT_REQUESTS_PER_RELAY) {
                    task.state = RelayTask::WAITING_FOR_CONNECTION;
                }
                break;
            }
        }
    }

    // For WAITING_FOR_CONNECTION tasks:
    //     Get or open a connection to the desired relay
    //     If the connection is open -> bump the task to READY
    for (auto& task : tasks) {
        if (task.state != RelayTask::WAITING_FOR_CONNECTION) continue;

        auto& conn = get_or_create_connection_for_relay(task.relay_id);
        if (conn.state == RelayConnection::OPEN) {
            task.state = RelayTask::READY;
        } else if (conn.state == RelayConnection::CLOSED) {
            auto relay_info = data_layer::get_relay_info(task.relay_id);
            printf("WARNING: task has been created for relay %s, but the connection is CLOSED\n", relay_info->url.data.get(relay_info));
        }
    }

    // For each READY task, start the task
    for (auto& task : tasks) {
        if (task.state != RelayTask::READY) continue;

        auto conn = get_connection_for_relay(task.relay_id);
        if (!conn || conn->state != RelayConnection::OPEN) continue;

        switch (task.type) {
            case RelayTask::REQUEST: {
                auto req = client_message_req(task.subscription_id, task.filters.get(), &req_buffer);
                printf("Request: %s\n", req);
                platform_websocket_send(conn->socket, req);
                conn->num_concurrent_requests++;
                break;
            }
            case RelayTask::STREAM: {
                auto req = client_message_req(task.subscription_id, task.filters.get(), &req_buffer);
                printf("Request: %s\n", req);
                platform_websocket_send(conn->socket, req);
                break;
            }
            case RelayTask::PUBLISH: {
                auto req = client_message_event(task.event.get(), &req_buffer);
                printf("Request: %s\n", req);
                platform_websocket_send(conn->socket, req);
                break;
            }
        }
        task.state = RelayTask::ACTIVE;
    }

    // Finally, remove all COMPLETED tasks
    for (long i = tasks.size() - 1; i >= 0; --i) {
        if (tasks[i].state == RelayTask::COMPLETED) {
            tasks.erase(tasks.begin() + i);
        }
    }

}

void app_websocket_event(const AppWebsocketEvent* event) {

    uint64_t event_time = time(NULL);

    RelayConnection* conn = NULL;
    for (auto& other_conn : connections) {
        if (other_conn.socket == event->socket) {
            conn = &other_conn;
        }
    }
    if (!conn) return;

    auto relay_info = data_layer::get_relay_info(conn->relay_id);
    auto relay_url  = relay_info->url.data.get(relay_info);

    // Process open, close and error
    if (event->type == WEBSOCKET_OPEN) {
        printf("Websocket open: %s\n", relay_url);
        conn->state = RelayConnection::OPEN;
    } else if (event->type == WEBSOCKET_CLOSE) {
        printf("Websocket close: %s\n", relay_url);
        conn->state = RelayConnection::CLOSED;
    } else if (event->type == WEBSOCKET_ERROR) {
        printf("Websocket error: %s\n", relay_url);
        conn->state = RelayConnection::CLOSED;
    }

    if (event->type != WEBSOCKET_MESSAGE) {
        process_tasks();
        return;
    }
    // Past this point we are processing only WEBSOCKET_MESSAGE events

    if (event->data_length == 0) {
        printf("received empty string\n");
        return;
    }

    uint8_t stack_buffer_data[event->data_length * 2];
    StackBuffer stack_buffer(stack_buffer_data, event->data_length * 2);

    RelayMessage message;
    if (!relay_message_parse(event->data, event->data_length, &stack_buffer, &message)) {
        printf("message parse error\n");
        return;
    }

    switch (message.type) {
        case RelayMessage::AUTH: {
            printf("%s AUTH: (not supported yet!)\n", relay_url);
            break;
        }
        case RelayMessage::COUNT: {
            printf("%s COUNT: (we didn't ask for this?)\n", relay_url);
            break;
        }
        case RelayMessage::EOSE: {
            printf("%s EOSE: %s\n", relay_url, message.eose.subscription_id);
            auto task = get_task_for_subscription_id(message.eose.subscription_id);
            if (!task || task->type != RelayTask::REQUEST) break;

            // For REQUEST tasks, upon receiving EOSE, we close the subscription
            StackBufferFixed<64> req_buffer;
            auto req = client_message_close(task->subscription_id, &req_buffer);
            printf("Request: %s\n", req);
            platform_websocket_send(conn->socket, req);
            conn->num_concurrent_requests--;

            task->state = RelayTask::COMPLETED;
            process_tasks();
            break;
        }
        case RelayMessage::EVENT: {
            Event* nostr_event;
            ParseError err = event_parse(message.event.input, message.event.input_len, &stack_buffer, &nostr_event);
            if (err) {
                printf("event invalid (parse error): %d\n", (int)err);
                break;
            }

            data_layer::receive_event(nostr_event, relay_info->id, event_time);

            break;
        }
        case RelayMessage::NOTICE: {
            printf("%s NOTICE: %s\n", relay_url, message.notice.message);
            break;
        }
        case RelayMessage::OK: {
            printf("%s OK: %s - %s\n", relay_url, message.ok.ok ? "true" : "false", message.ok.message);
            auto task = get_task_for_event_id(&message.ok.event_id);
            if (task) {
                // @TODO: handle publish errors
                task->state = RelayTask::COMPLETED;
                process_tasks();
            }
            break;
        }
    }
}

void network::fetch(const char* url, network::FetchCallback callback) {
    auto cb = new network::FetchCallback(std::move(callback));
    platform_http_request(url, cb);
}

void network::fetch_image(const char* url, network::FetchImageCallback callback) {
    auto cb = new network::FetchImageCallback(std::move(callback));
    platform_http_image_request(url, cb);
}

void app_http_response(int status_code, const unsigned char* data, int data_length, void* user_data) {
    auto cb = (network::FetchCallback*)user_data;
    (*cb)(status_code == -1, status_code, data, data_length);
    delete cb;
}

void app_http_image_response(int image_id, void* user_data) {
    auto cb = (network::FetchImageCallback*)user_data;
    (*cb)(image_id);
    delete cb;
}
