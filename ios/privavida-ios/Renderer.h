//
//  Renderer.h
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import <MetalKit/MetalKit.h>
#import <app.h>

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

-(nonnull instancetype)initWithMetalKitView:(nonnull MTKView *)view andAppKeyboard:(AppKeyboard)app_keyboard;

@end

