//
//  platform_ios.h
//  privavida-ios
//
//  Created by Bartholomew Joyce on 24/04/2023.
//

#ifndef platform_ios_h
#define platform_ios_h

#import <platform.h>

extern NVGcontext* vg;
void init_platform_user_data_dir(void);
void app_http_response_for_image(int status_code, const unsigned char* data, int data_length, void* user_data);

#endif /* platform_ios_h */
