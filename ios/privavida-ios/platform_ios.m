//
//  platform_ios.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 24/04/2023.
//

#import "platform_ios.h"
#import "GameViewController.h"
#import "privavida_ios-Swift.h"

const char* platform_user_data_dir = NULL;

void init_platform_user_data_dir(void) {
    char* user_data_dir = (char*)malloc(1024);
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    [[paths firstObject] getCString:user_data_dir maxLength:1024 encoding:NSUTF8StringEncoding];
    platform_user_data_dir = user_data_dir;
}

void platform_update_text_input(const AppTextInputConfig* config) {
    [(__bridge GameViewController*)game_view_controller updateTextInput: config];
}

void platform_remove_text_input(void) {
    [(__bridge GameViewController*)game_view_controller removeTextInput];
}

const char* platform_get_asset_name(const char* asset_name, const char* asset_type) {
    @autoreleasepool {
        NSString* ns_asset_name = [NSString stringWithCString:asset_name encoding:NSUTF8StringEncoding];
        NSString* ns_asset_type = [NSString stringWithCString:asset_type encoding:NSUTF8StringEncoding];
        NSString* ns_filename = [[NSBundle mainBundle] pathForResource:ns_asset_name ofType:ns_asset_type];
        if (ns_filename == nil) {
            return NULL;
        }

        static char filename[512];
        if ([ns_filename getCString:filename maxLength:512 encoding:NSUTF8StringEncoding]) {
            return filename;
        }

        return NULL;
    }
}

void platform_user_data_flush(void) {
    // No-op
}

AppWebsocketHandle platform_websocket_open(const char* url, void* user_data) {
    return [[Networking sharedInstance] websocketOpenWithUrl:(const int8_t*)url userData:user_data];
}

void platform_websocket_send(AppWebsocketHandle socket, const char* data) {
    [[Networking sharedInstance] websocketSendWithWs:socket data:(const int8_t*)data];
}

void platform_websocket_close(AppWebsocketHandle socket, unsigned short code, const char* reason) {
    [[Networking sharedInstance] websocketCloseWithWs:socket code:code reason:(const int8_t*)reason];
}

void platform_http_request_send(const char* url, void* user_data) {
    [[Networking sharedInstance] httpRequestSendWithUrl:url userData:user_data];
}
