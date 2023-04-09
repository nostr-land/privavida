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

static void redraw(void* ptr) {
    [(__bridge Renderer*)ptr redraw];
}

@implementation Renderer
{
    id <MTLDevice> _device;
    NVGcontext* _vg;
    BOOL _should_redraw;
}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view andAppKeyboard:(AppKeyboard)app_keyboard {
    self = [super init];
    if (!self) {
        return self;
    }

    _device = view.device;
    // ((CAMetalLayer*)view.layer).maximumDrawableCount = 2;

    // Initialise NanoVG    
    _vg = nvgCreateMTL((__bridge void *)(view.layer), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!_vg) {
        printf("Couldn't init nanovg.\n");
        exit(1);
    }

    // Load our fonts
    nvgCreateFont(_vg, "mono",     get_asset_name("PTMono",          "ttf"));
    nvgCreateFont(_vg, "regular",  get_asset_name("SFRegular",       "ttf"));
    nvgCreateFont(_vg, "regulari", get_asset_name("SFRegularItalic", "ttf"));
    nvgCreateFont(_vg, "medium",   get_asset_name("SFMedium",        "ttf"));
    nvgCreateFont(_vg, "mediumi",  get_asset_name("SFMediumItalic",  "ttf"));
    nvgCreateFont(_vg, "bold",     get_asset_name("SFBold",          "ttf"));
    nvgCreateFont(_vg, "boldi",    get_asset_name("SFBoldItalic",    "ttf"));
    nvgCreateFont(_vg, "thin",     get_asset_name("SFThin",          "ttf"));
    
    _should_redraw = YES;
    AppRedraw app_redraw;
    app_redraw.opaque_ptr = (__bridge void*)self;
    app_redraw.redraw = redraw;

    app_init(_vg, app_redraw, app_keyboard);

    char temp[1024];
    [[[[NSFileManager defaultManager] temporaryDirectory] path] getCString:temp maxLength:1024 encoding:NSUTF8StringEncoding];
    app_set_temp_directory(temp);

    return self;
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    /// Per frame updates here
    if (!_should_redraw) {
        return;
    }
    _should_redraw = NO;

    float width = view.drawableSize.width;
    float height = view.drawableSize.height;
    float pixel_ratio = view.contentScaleFactor;
    width = width / pixel_ratio;
    height = height / pixel_ratio;
    app_render(width, height, pixel_ratio);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    /// No-op
    [self drawInMTKView:view];
}

- (void)redraw {
    _should_redraw = YES;
}

@end
