//
//  GameViewController.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 08/05/2023.
//

#import "GameViewController.h"
#import "platform_ios.h"

void* game_view_controller = NULL;

@implementation GameViewController {
    MTKView *_view;
    UITextField *_textField;
    Renderer *_renderer;
    BOOL _wantsKeyboardOpen;
    CGRect _keyboardRect;
    AppTextInputConfig _lastConfig;
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

    game_view_controller = (__bridge void*)self;
    init_platform_user_data_dir();

    _textField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, 200, 30)];
    _textField.placeholder = @"Enter text";
    [_textField setHidden:YES];
    [_textField setDelegate:self];
    [_view addSubview:_textField];

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];

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
    return NO; // _wantsKeyboardOpen;
}

- (BOOL)hasText {
    return NO;
}

- (void)deleteBackward {
    AppKeyEvent key_event = { 0 };
    app_key_event(key_event);
}

- (void)insertText:(nonnull NSString *)text {
    AppKeyEvent key_event;
    key_event.action = KEY_PRESS;
    [text getCString:key_event.ch maxLength:8 encoding:NSUTF8StringEncoding];
    key_event.mods = 0;
    key_event.key = 0;
    app_key_event(key_event);
}

- (void)updateTextInput:(const AppTextInputConfig *)config {
    _wantsKeyboardOpen = YES;

    // Open text field
    if ([_textField isHidden]) {
        _textField.font = [UIFont systemFontOfSize:config->font_size];
        _textField.textColor = [UIColor colorWithRed:config->text_color.r
                                               green:config->text_color.g
                                                blue:config->text_color.b
                                               alpha:config->text_color.a];
        [_textField setText:[NSString stringWithUTF8String:config->content]];
        [_textField setFrame:CGRectMake(config->x, config->y, config->width, config->height)];
        if (config->flags & APP_TEXT_FLAGS_TYPE_PASSWORD) {
            [_textField setTextContentType:UITextContentTypePassword];
        }
        [_textField setHidden:NO];
        [_textField becomeFirstResponder];
        _lastConfig = *config;
        return;
    }

    // Update text field
    if (_lastConfig.font_size != config->font_size) {
        _textField.font = [UIFont systemFontOfSize:config->font_size];
    }
//    if (_lastConfig.text_color != config->text_color) {
//        _textField.textColor = [UIColor colorWithRed:config->text_color.r
//                                               green:config->text_color.g
//                                                blue:config->text_color.b
//                                               alpha:config->text_color.a];
//    }
    if (_lastConfig.x != config->x || _lastConfig.y != config->y ||
        _lastConfig.width != config->width || _lastConfig.height != config->height) {
        [_textField setFrame:CGRectMake(config->x, config->y, config->width, config->height)];
    }
    // [self becomeFirstResponder];
}

- (void)removeTextInput {
    _wantsKeyboardOpen = NO;
    
    [_textField resignFirstResponder];
    [_textField setHidden:YES];
    // [self resignFirstResponder];
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    NSString* newString = [[textField text] stringByReplacingCharactersInRange:range
                                                                    withString:string];
    app_text_input_content_changed([newString UTF8String]);
    return YES;
}

- (void)keyboardWillChange:(NSNotification *)notification {
    CGRect rect = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    rect = [self.view convertRect:rect fromView:nil];
    
    app_keyboard_changed([_textField isFirstResponder], rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

@end
