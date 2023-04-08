//
//  GameViewController.h
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "Renderer.h"

// Our iOS view controller
@interface GameViewController : UIViewController <UIKeyInput>

- (void)openKeyboard;
- (void)closeKeyboard;

@end
