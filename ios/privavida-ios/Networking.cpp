//
//  Networking.cpp
//  privavida-ios
//
//  Created by Bartholomew Joyce on 20/05/2023.
//

extern "C" {
#include "Networking.h"
}

#include <iostream>
#include <fstream>

#include <websocketpp/config/core.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <stdio.h>

typedef websocketpp::client<websocketpp::config::asio_tls_client> WebsocketClient;
static WebsocketClient client;

static AppWebsocketHandle app_ws_open(void* ptr, const char* url) {
    printf("app_ws_open: %s\n", url);
    return -1;
}

static void app_ws_send(void* ptr, AppWebsocketHandle ws, const char* data) {
    printf("app_ws_send: %s\n", data);
}

static void app_ws_close(void* ptr, AppWebsocketHandle ws, unsigned short code, const char* reason) {
    printf("app_ws_close: %d reason = %s\n", (int)code, reason);
}

AppNetworking init_app_networking() {

    AppNetworking app_networking;
    app_networking.opaque_ptr = NULL;
    app_networking.websocket_open = app_ws_open;
    app_networking.websocket_send = app_ws_send;
    app_networking.websocket_close = app_ws_close;
    return app_networking;

}
