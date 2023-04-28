//
//  platform_ios.m
//  privavida-ios
//
//  Created by Bartholomew Joyce on 24/04/2023.
//

#import "platform_ios.h"
#import "GameViewController.h"
#import "privavida_ios-Swift.h"
#import <CoreText/CoreText.h>
#import "nanovg_mtl/nanovg_mtl.h"

NVGcontext* vg;
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

void platform_open_url(const char* url) {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]]];
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

void platform_http_request(const char* url, void* user_data) {
    [[Networking sharedInstance] httpRequestWithUrl:(const int8_t*)url userData:user_data];
}

void platform_http_image_request(const char* url, void* user_data) {
    [[Networking sharedInstance] httpRequestImageWithUrl:(const int8_t*)url userData:user_data];
}

void app_http_response_for_image(int status_code, const unsigned char* data, int data_length, void* user_data) {
    if (status_code != 200) {
        app_http_image_response(0, user_data);
        return;
    }
    
    int image_id = nvgCreateImageMem(vg, 0, data, data_length);
    app_http_image_response(image_id, user_data);
}

static PlatformEmojiMetrics metrics_cached;

static BOOL emoji_did_init = NO;
static CGContextRef context;
static void* bitmap;
static int bitmapWidth, bitmapHeight, bitmapBytesPerRow;

static void emoji_init(void) {
    emoji_did_init = YES;

    bitmapWidth = 512;
    bitmapHeight = 512;

    CGColorSpaceRef colorSpace;
    bitmapBytesPerRow = (bitmapWidth * 4);
 
    colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    bitmap = malloc(bitmapBytesPerRow * bitmapHeight * sizeof(uint8_t));
    assert(bitmap);
    context = CGBitmapContextCreate(bitmap,
                                    bitmapWidth,
                                    bitmapHeight,
                                    8 /* bits per component*/,
                                    bitmapBytesPerRow,
                                    colorSpace,
                                    kCGImageAlphaPremultipliedLast);
    assert(context);
    CGColorSpaceRelease(colorSpace);

    // Set the text matrix.
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
}

int platform_supports_emoji = 1;
int platform_emoji_measure(const char* data, int data_length, int text_size, PlatformEmojiMetrics* metrics) {

    if (!emoji_did_init) emoji_init();

    // Initialize a string.
    CFStringRef textString = CFStringCreateWithBytes(NULL, (const uint8_t*)data, data_length, kCFStringEncodingUTF8, NO);

    // Create a mutable attributed string with a max length of 0.
    // The max length is a hint as to how much internal storage to reserve.
    // 0 means no hint.
    CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);

    // Copy the textString into the newly created attrString
    CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), textString);
    CFRelease(textString);

    CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, text_size, NULL);
    
    // Set color
    CFRange textRange = CFRangeMake(0, CFAttributedStringGetLength(attrString));
    CFAttributedStringSetAttribute(attrString, textRange, kCTFontAttributeName, font);

    // Create the typesetter with the attributed string.
    CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString(attrString);
    CFRelease(attrString);
    
    // Create a frame.
    CTLineRef line = CTTypesetterCreateLine(typesetter, textRange);
    CGFloat ascent, descent;
    CTLineGetTypographicBounds(line, &ascent, &descent, NULL);
    CGRect bounds = CTLineGetImageBounds(line, NULL);

    metrics->bounding_width = (int)bounds.size.width;
    metrics->bounding_height = (int)(ascent + descent);
    metrics->left = (int)bounds.origin.x;
    metrics->baseline = (int)ascent;
    metrics->width = (int)(bounds.size.width - bounds.origin.x);
    metrics_cached = *metrics;

    CFRelease(line);
    CFRelease(typesetter);
    CFRelease(font);
    return 1;
}

void platform_emoji_render(const char* data, int data_length, int text_size, NVGcolor color, const PlatformEmojiRenderTarget* render_target) {
    
    if (!emoji_did_init) emoji_init();
    
    memset(bitmap, 0, bitmapBytesPerRow * bitmapHeight);

    // Create a path which bounds the area where you will be drawing text.
    // The path need not be rectangular.
    CGMutablePathRef path = CGPathCreateMutable();

    // Initialize a string.
    CFStringRef textString = CFStringCreateWithBytes(NULL, (const uint8_t*)data, data_length, kCFStringEncodingUTF8, NO);
    
    // Create a mutable attributed string with a max length of 0.
    // The max length is a hint as to how much internal storage to reserve.
    // 0 means no hint.
    CFMutableAttributedStringRef attrString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);

    // Copy the textString into the newly created attrString
    CFAttributedStringReplaceString(attrString, CFRangeMake(0, 0), textString);
    CFRelease(textString);

    // Create a color that will be added as an attribute to the attrString.
    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
    CGFloat components[] = { color.r, color.g, color.b, color.a };
    CGColorRef cgColor = CGColorCreate(rgbColorSpace, components);
    CGColorSpaceRelease(rgbColorSpace);

    CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, text_size, NULL);
    
    // Set color
    CFRange textRange = CFRangeMake(0, CFAttributedStringGetLength(attrString));
    CFAttributedStringSetAttribute(attrString, textRange, kCTForegroundColorAttributeName, cgColor);
    CFAttributedStringSetAttribute(attrString, textRange, kCTFontAttributeName, font);

    // Create the framesetter with the attributed string.
    CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(attrString);
    CFRelease(attrString);

    // In this simple example, initialize a rectangular path.
    CGRect bounds = CGRectMake(0, 0, bitmapWidth, bitmapHeight);
    CGPathAddRect(path, NULL, bounds);
    
    // Create a frame.
    CTFrameRef frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
    CTFrameDraw(frame, context);

    // Release the objects we used.
    CFRelease(frame);
    CFRelease(path);
    CFRelease(framesetter);
    CFRelease(font);

    id<MTLTexture> texture = (__bridge id<MTLTexture>)mnvgImageHandle(vg, render_target->image_id);
    [texture replaceRegion:MTLRegionMake2D(render_target->left, render_target->top, metrics_cached.bounding_width, metrics_cached.bounding_height)
               mipmapLevel:0
                 withBytes:bitmap
               bytesPerRow:bitmapBytesPerRow];
}
