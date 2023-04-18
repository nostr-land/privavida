//
//  GameViewController.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import "GameViewController.h"
#import <app.h>

@implementation GameViewController {
    MTKView *_view;
    Renderer *_renderer;
    BOOL _wantsKeyboardOpen;
    CGRect _keyboardRect;
}

static void app_open_keyboard(void* ptr) {
    [(__bridge GameViewController*)ptr openKeyboard];
}

static void app_close_keyboard(void* ptr) {
    [(__bridge GameViewController*)ptr closeKeyboard];
}

static void app_keyboard_rect(void* ptr, float* x, float* y, float* width, float* height) {
    CGRect rect = [(__bridge GameViewController*)ptr keyboardRect];
    *x = rect.origin.x;
    *y = rect.origin.y;
    *width = rect.size.width;
    *height = rect.size.height;
}

static AppWebsocketHandle app_ws_open(void* ptr, const char* url) {
    return -1;
}

static void app_ws_send(void* ptr, AppWebsocketHandle ws, const char* data) {
    
}

static void app_ws_close(void* ptr, AppWebsocketHandle ws, unsigned short code, const char* reason) {
    
}

- (void)viewDidLoad {

    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();
    _view.backgroundColor = UIColor.blackColor;
    _view.multipleTouchEnabled = YES;
    _view.preferredFramesPerSecond = 60;

    if(!_view.device) {
        NSLog(@"Metal is not supported on this device");
        self.view = [[UIView alloc] initWithFrame:self.view.frame];
        return;
    }

    AppKeyboard app_keyboard;
    app_keyboard.opaque_ptr = (__bridge void*)self;
    app_keyboard.open = app_open_keyboard;
    app_keyboard.close = app_close_keyboard;
    app_keyboard.rect = app_keyboard_rect;

    AppNetworking app_networking;
    app_networking.opaque_ptr = (__bridge void*)self;
    app_networking.websocket_open = app_ws_open;
    app_networking.websocket_send = app_ws_send;
    app_networking.websocket_close = app_ws_close;
    
    _renderer = [[Renderer alloc] initWithMetalKitView:_view
                                        andAppKeyboard:app_keyboard
                                      andAppNetworking:app_networking];

    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];

    _view.delegate = _renderer;
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillChange:) name:UIKeyboardWillChangeFrameNotification object:nil];
}

- (int)convertTouches:(NSSet<UITouch *>*)touches withOutput:(AppTouch *)touches_out {
    NSEnumerator* enumerator = [touches objectEnumerator];
    UITouch* touch_in;
    int count = 0;
    while ((touch_in = [enumerator nextObject])) {
        AppTouch* touch_out = &touches_out[count++];
        CGPoint point = [touch_in locationInView: _view];

        touch_out->id = (int)touch_in.hash;
        touch_out->x = point.x;
        touch_out->y = point.y;
        touch_out->opaque_ptr = NULL;
    }

    return count;
}

- (int)convertTouches:(NSSet<UITouch *>*)touches withOutput:(AppTouch *)touches_out excluding:(NSSet<UITouch *>*)touches_exclude {
    NSEnumerator* enumerator = [touches objectEnumerator];
    UITouch* touch_in;
    int count = 0;
    while ((touch_in = [enumerator nextObject])) {
        if ([touches_exclude member:touch_in]) {
            continue;
        }
        AppTouch* touch_out = &touches_out[count++];
        CGPoint point = [touch_in locationInView: _view];

        touch_out->id = (int)touch_in.hash;
        touch_out->x = point.x;
        touch_out->y = point.y;
        touch_out->opaque_ptr = NULL;
    }

    return count;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    AppTouchEvent next_event;
    next_event.type = TOUCH_START;
    next_event.num_touches = [self convertTouches:[event allTouches] withOutput:next_event.touches];
    next_event.num_touches_changed = [self convertTouches:touches withOutput:next_event.touches_changed];
    app_touch_event(&next_event);
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    AppTouchEvent next_event;
    next_event.type = TOUCH_MOVE;
    next_event.num_touches = [self convertTouches:[event allTouches] withOutput:next_event.touches];
    next_event.num_touches_changed = [self convertTouches:touches withOutput:next_event.touches_changed];
    app_touch_event(&next_event);
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    AppTouchEvent next_event;
    next_event.type = TOUCH_END;
    next_event.num_touches = [self convertTouches:[event allTouches] withOutput:next_event.touches excluding:touches];
    next_event.num_touches_changed = [self convertTouches:touches withOutput:next_event.touches_changed];
    app_touch_event(&next_event);
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    AppTouchEvent next_event;
    next_event.type = TOUCH_CANCEL;
    next_event.num_touches = [self convertTouches:[event allTouches] withOutput:next_event.touches excluding:touches];
    next_event.num_touches_changed = [self convertTouches:touches withOutput:next_event.touches_changed];
    app_touch_event(&next_event);
}

- (BOOL)canBecomeFirstResponder {
    return _wantsKeyboardOpen;
}

- (BOOL)hasText {
    return NO;
}

- (void)deleteBackward {
    app_key_backspace();
}

- (void)insertText:(nonnull NSString *)text {
    char ch[8];
    [text getCString:ch maxLength:8 encoding:NSUTF8StringEncoding];
    app_key_character(ch);
}

- (void)openKeyboard {
    _wantsKeyboardOpen = YES;
    [self becomeFirstResponder];
}

- (void)closeKeyboard {
    _wantsKeyboardOpen = NO;
    [self resignFirstResponder];
}

- (CGRect)keyboardRect {
    return _keyboardRect;
}

- (void)keyboardWillChange:(NSNotification *)notification {
    _keyboardRect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    _keyboardRect = [self.view convertRect:_keyboardRect fromView:nil];
}

@end
