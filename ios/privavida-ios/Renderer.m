//
//  Renderer.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import "Renderer.h"
#include <app.h>
#include "nanovg_mtl.h"

static const char* get_asset_name(const char* asset_name, const char* asset_type) {
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

@implementation Renderer
{
    NVGcontext* _vg;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view andAppKeyboard:(AppKeyboard)app_keyboard {
    self = [super init];
    if (!self) {
        return self;
    }

    // Initialise NanoVG    
    _vg = nvgCreateMTL((__bridge void *)(view.layer), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!_vg) {
        printf("Couldn't init nanovg.\n");
        exit(1);
    }

    char* user_data_dir = (char*)malloc(1024);
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    [[paths firstObject] getCString:user_data_dir maxLength:1024 encoding:NSUTF8StringEncoding];

    AppStorage storage;
    storage.get_asset_name = &get_asset_name;
    storage.user_data_dir = user_data_dir;
    storage.user_data_flush = NULL; // Automatic on iOS!

    app_init(_vg, app_keyboard, storage);

    return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    if (!app_wants_to_render()) {
        return;
    }

    float width = view.drawableSize.width;
    float height = view.drawableSize.height;
    float pixel_ratio = view.contentScaleFactor;
    width = width / pixel_ratio;
    height = height / pixel_ratio;
    app_render(width, height, pixel_ratio);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    [self drawInMTKView:view];
}

@end
