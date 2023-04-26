//
//  Renderer.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import "Renderer.h"
#import <platform.h>
#import "platform_ios.h"
#include "nanovg_mtl.h"

@implementation Renderer
{}

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view {
    self = [super init];
    if (!self) {
        return self;
    }

    // Initialise NanoVG    
    vg = nvgCreateMTL((__bridge void*)(view.layer), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!vg) {
        printf("Couldn't init nanovg.\n");
        exit(1);
    }

    app_init(vg);

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
