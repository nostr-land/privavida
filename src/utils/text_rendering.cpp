//
//  text_rendering.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 27-04-2023.
//

#include <app.hpp>
#include <platform.h>
#include <stdio.h>
#include <vector>


// This implementation by no means covers all use-cases.
// All I've added is some basic Emoji detection and support.
// If you ask to render or measure the bounds for a bit of text
// that contains no emojis, I will just call the regular nvgText
// functions. If however, I do detect some parts to be emoji,
// I will render all non-emoji sections with the regular nvgText
// and will call out to the platform code to give me emoji
// textures.

// At present there is no texture atlas for the emojis. I'm
// just allocating a separate texture for each emoji, which is
// pretty wasteful, but for now doesn't seem necessary to optimise
// further.


namespace ui {

void (*text)(float x, float y, const char* string, const char* end);
void (*text_box)(float x, float y, float breakRowWidth, const char* string, const char* end);
void (*text_bounds)(float x, float y, const char* string, const char* end, float* bounds);
void (*text_metrics)(float* ascender, float* descender, float* lineh);

static void text_no_emoji(float x, float y, const char* string, const char* end);
static void text_box_no_emoji(float x, float y, float breakRowWidth, const char* string, const char* end);
static void text_bounds_no_emoji(float x, float y, const char* string, const char* end, float* bounds);
static void text_metrics_no_emoji(float* ascender, float* descender, float* lineh);

static void text_emoji(float x, float y, const char* string, const char* end);
static void text_box_emoji(float x, float y, float breakRowWidth, const char* string, const char* end);
static void text_bounds_emoji(float x, float y, const char* string, const char* end, float* bounds);
static void text_metrics_emoji(float* ascender, float* descender, float* lineh);

void text_rendering_init() {
    if (platform_supports_emoji) {
        text = text_emoji;
        text_box = text_box_emoji;
        text_bounds = text_bounds_emoji;
        text_metrics = text_metrics_emoji;
    } else {
        text = text_no_emoji;
        text_box = text_box_no_emoji;
        text_bounds = text_bounds_no_emoji;
        text_metrics = text_metrics_no_emoji;
    }
}

static float font_size_;
static float line_height_;
static int text_align_;
static int font_;

void font_size(float size) {
    font_size_ = size;
    nvgFontSize(vg, size);
}

void text_line_height(float line_height) {
    line_height_ = line_height;
    nvgTextLineHeight(vg, line_height);
}

void text_align(int align) {
    text_align_ = align;
    nvgTextAlign(vg, align);
}

void font_face_id(int font) {
    font_ = font;
    nvgFontFaceId(vg, font);
}

void font_face(const char* name) {
    font_ = nvgFindFont(vg, name);
    nvgFontFaceId(vg, font_);
}

void text_no_emoji(float x, float y, const char* string, const char* end) {
    nvgText(vg, x, y, string, end);
}

void text_box_no_emoji(float x, float y, float breakRowWidth, const char* string, const char* end) {
    nvgTextBox(vg, x, y, breakRowWidth, string, end);
}

void text_bounds_no_emoji(float x, float y, const char* string, const char* end, float* bounds) {
    nvgTextBounds(vg, x, y, string, end, bounds);
}

void text_metrics_no_emoji(float* ascender, float* descender, float* lineh) {
    nvgTextMetrics(vg, ascender, descender, lineh);
}

static int break_into_parts(const uint8_t* string, const uint8_t* end, int* part_start, bool* part_is_emoji);

struct EmojiTexture {
    int font_size;
    const char* content;
    int content_len;
    int bounding_height;
    int bounding_width;
    int baseline;
    int left;
    int width;
    int image_id;
};

std::vector<EmojiTexture> emoji_textures;

#define MAX_PARTS 64

static float get_font_scale();
static EmojiTexture* get_emoji_texture(int font_size, const char* string, int length);

// TODO: make this work for center and right aligned text!!
void text_emoji(float x, float y, const char* string, const char* end) {
    if (!end) end = string + strlen(string);

    int part_start[MAX_PARTS];
    bool part_is_emoji[MAX_PARTS];
    int num_parts = break_into_parts((const uint8_t*)string, (const uint8_t*)end, part_start, part_is_emoji);
    if (num_parts == 1 && !part_is_emoji[0]) {
        nvgText(vg, x, y, string, end);
        return;
    }

    nvgSave(vg);

    float ascender, descender, baseline;
    nvgTextMetrics(vg, &ascender, &descender, &baseline);
    nvgTextAlign(vg, NVG_ALIGN_BASELINE);

    if (text_align_ & NVG_ALIGN_TOP) {
        y += ascender;
    } else if (text_align_ & NVG_ALIGN_MIDDLE) {
        y += 0.5 * ascender;
    } else if (text_align_ & NVG_ALIGN_BOTTOM) {
        y -= descender;
    }

    for (int i = 0; i < num_parts; ++i) {
        if (!part_is_emoji[i]) {
            x = nvgText(vg, x, y, string + part_start[i], string + part_start[i + 1]);
            continue;
        }

        float font_size_emoji = font_size_ * 1.2; // Bump them up a little
        float scale = get_font_scale();
        int font_size = (int)(scale * font_size_emoji);
        float inv_scale = font_size_emoji / font_size;
        EmojiTexture* em = get_emoji_texture(font_size, string + part_start[i], part_start[i + 1] - part_start[i]);
        if (!em) {
            x = nvgText(vg, x, y, "😀", NULL); // This will render as a question mark box
            continue;
        }

        float img_x = x - em->left*inv_scale;
        float img_y = y - em->baseline*inv_scale;
        float img_w = em->bounding_width*inv_scale;
        float img_h = em->bounding_height*inv_scale;

        auto paint = nvgImagePattern(vg, img_x, img_y, img_w, img_h, 0, em->image_id, 1);
        nvgFillPaint(vg, paint);
        nvgBeginPath(vg);
        nvgRect(vg, img_x, img_y, img_w, img_h);
        nvgFill(vg);

        x += em->width*inv_scale;
    }

    nvgRestore(vg);
}

void text_box_emoji(float x, float y, float breakRowWidth, const char* string, const char* end) {
    nvgTextBox(vg, x, y, breakRowWidth, string, end);
}

void text_bounds_emoji(float x, float y, const char* string, const char* end, float* bounds) {
    if (!end) end = string + strlen(string);

    int part_start[MAX_PARTS];
    bool part_is_emoji[MAX_PARTS];
    int num_parts = break_into_parts((const uint8_t*)string, (const uint8_t*)end, part_start, part_is_emoji);
    if (num_parts == 1 && !part_is_emoji[0]) {
        nvgTextBounds(vg, x, y, string, end, bounds);
        return;
    }

    float ascender, descender, baseline;
    nvgTextMetrics(vg, &ascender, &descender, &baseline);

    float y_baseline = y;
    if (text_align_ & NVG_ALIGN_TOP) {
        y_baseline += ascender;
    } else if (text_align_ & NVG_ALIGN_MIDDLE) {
        y_baseline += 0.5 * ascender;
    } else if (text_align_ & NVG_ALIGN_BOTTOM) {
        y_baseline -= descender;
    }

    for (int i = 0; i < num_parts; ++i) {

        float part_bounds[4];
        if (!part_is_emoji[i]) {
            x = nvgTextBounds(vg, x, y, string + part_start[i], string + part_start[i + 1], part_bounds);
            goto got_bounds;
        }

        {
            float scale = get_font_scale();
            float font_size_emoji = font_size_ * 1.2; // Bump them up a little
            int font_size = (int)(scale * font_size_emoji);
            float inv_scale = font_size_emoji / font_size;
            EmojiTexture* em = get_emoji_texture(font_size, string + part_start[i], part_start[i + 1] - part_start[i]);
            if (!em) {
                x = nvgTextBounds(vg, x, y, "😀", NULL, part_bounds); // This will render as a question mark box
                goto got_bounds;
            }

            float img_x = x - em->left*inv_scale;
            float img_y = y_baseline - em->baseline*inv_scale;
            float img_w = em->bounding_width*inv_scale;
            float img_h = em->bounding_height*inv_scale;

            part_bounds[0] = img_x;
            part_bounds[1] = img_y;
            part_bounds[2] = img_x + img_w;
            part_bounds[3] = img_y + img_h;

            x += em->width*inv_scale;
        }

    got_bounds:
        if (i == 0) {
            bounds[0] = part_bounds[0];
            bounds[1] = part_bounds[1];
            bounds[2] = part_bounds[2];
            bounds[3] = part_bounds[3];
            continue;
        }
        if (bounds[0] > part_bounds[0]) bounds[0] = part_bounds[0];
        if (bounds[1] > part_bounds[1]) bounds[1] = part_bounds[1];
        if (bounds[2] < part_bounds[2]) bounds[2] = part_bounds[2];
        if (bounds[3] < part_bounds[3]) bounds[3] = part_bounds[3];
    }
}

void text_metrics_emoji(float* ascender, float* descender, float* lineh) {
    nvgTextMetrics(vg, ascender, descender, lineh);
}

// Single one-off code points for emoji
static const uint32_t EMOJI_CODEPOINTS_SINGLE[] = {
    0x00A9,
    0x00AE,
    0x203C,
    0x2049,
    0x20E3,
    0x2122,
    0x2139,
    0x231A,
    0x231B,
    0x2328,
    0x23CF,
    0x24C2,
    0x25AA,
    0x25AB,
    0x25B6,
    0x25C0,
    0x2934,
    0x2935,
    0x3030,
    0x303D,
    0x3297,
    0x3299,
};

// Ranges of emoji
static const uint32_t EMOJI_CODEPOINTS_RANGES[] = {
    0x2194, 0x2199,
    0x21A9, 0x21AA,
    0x23E9, 0x23F3,
    0x23F8, 0x23FA,
    0x25FB, 0x25FE,
    0x2600, 0x27EF,
    0x2B00, 0x2BFF,
    0x1F000, 0x1F02F,
    0x1F0A0, 0x1F0FF,
    0x1F100, 0x1F64F,
    0x1F600, 0x1F6FF,
    0x1F900, 0x1F9FF,
    0x1F980, 0x1F9E0
};

// Joiner characters that may appear after a valid emoji
// and should be considered as part of the emoji. There
// is much more complex logic to determine how to join
// multiple codepoints into a single glyph, and I don't
// think it's worthwhile to try to sort that out here.
// Perhaps on iOS I could call out to CoreText to tell
// me exactly what glyph is what.
static const uint32_t EMOJI_CODEPOINTS_JOINERS[] = {
    0x200D, // Zero-width joiner
    0xFE0E, // Text variation selector
    0xFE0F, // Emoji variation selector
};

int break_into_parts(const uint8_t* string, const uint8_t* end, int* part_start, bool* part_is_emoji) {

    int i = 0;
    part_start[i] = 0;
    part_is_emoji[i] = 0;

    auto ch = string;
    while (ch < end) {
        int start = (int)(ch - string);
        auto ch_len = (
            !(*ch & 0x80) ? 1 :
            !(*ch & 0x20) ? 2 :
            !(*ch & 0x10) ? 3 : 4
        );
        int end = start + ch_len;
        bool is_emoji;

        if (ch_len == 1) {
            is_emoji = false;
            ch++;
            goto next;
        }

        // Decode unicode codepoint
        {
            uint8_t ch_1, ch_2, ch_3, ch_4;
            uint32_t codepoint;
            if (ch_len == 4) {
                ch_1 = *ch++;
                ch_2 = *ch++;
                ch_3 = *ch++;
                ch_4 = *ch++;
                codepoint = ((ch_1 & 0x07) << 18) + ((ch_2 & 0x3F) << 12) + ((ch_3 & 0x3F) << 6) + ((ch_4 & 0x3F));
            } else if (ch_len == 3) {
                ch_1 = *ch++;
                ch_2 = *ch++;
                ch_3 = *ch++;
                codepoint = ((ch_1 & 0x0F) << 12) + ((ch_2 & 0x3F) << 6) + ((ch_3 & 0x3F));
            } else {
                ch_1 = *ch++;
                ch_2 = *ch++;
                codepoint = ((ch_1 & 0x1F) << 6) + ((ch_2 & 0x3F));
            }

            // Check for emoji
            for (int i = 0; i < sizeof(EMOJI_CODEPOINTS_SINGLE) / sizeof(uint32_t); ++i) {
                if (codepoint == EMOJI_CODEPOINTS_SINGLE[i]) {
                    is_emoji = true;
                    goto next;
                }
            }
            for (int i = 0; i < sizeof(EMOJI_CODEPOINTS_RANGES) / sizeof(uint32_t); i += 2) {
                if (codepoint >= EMOJI_CODEPOINTS_RANGES[i] &&
                    codepoint <= EMOJI_CODEPOINTS_RANGES[i + 1]) {
                    is_emoji = true;
                    goto next;
                }
            }
            if (part_is_emoji[i]) {
                for (int i = 0; i < sizeof(EMOJI_CODEPOINTS_JOINERS) / sizeof(uint32_t); ++i) {
                    if (codepoint == EMOJI_CODEPOINTS_JOINERS[i]) {
                        is_emoji = true;
                        goto next;
                    }
                }
            }
            is_emoji = false;
        }

    next:
        if (start == 0) {
            part_is_emoji[0] = is_emoji;
        } else if (part_is_emoji[i] != is_emoji) {
            i++;
            part_start[i] = start;
            part_is_emoji[i] = is_emoji;
        }
    }

    i++;
    part_start[i] = (int)(ch - string);
    return i;
}

float get_font_scale() {
    float t[6];
    nvgCurrentTransform(ui::vg, t);
    float sx = sqrtf(t[0]*t[0] + t[2]*t[2]);
    float sy = sqrtf(t[1]*t[1] + t[3]*t[3]);
    
    float scale = 0.5f * (sx + sy);
    if (scale > 4.0f) {
        scale = 4.0f;
    }
    return scale * ui::device_pixel_ratio();
}

EmojiTexture* get_emoji_texture(int font_size, const char* string, int length) {

    // Have we already got the emoji texture?
    for (auto& emoji_texture : emoji_textures) {
        if (emoji_texture.font_size == font_size &&
            emoji_texture.content_len == length &&
            strncmp(emoji_texture.content, string, length) == 0) {
            return &emoji_texture;
        }
    }

    // Gotta generate this one
    PlatformEmojiMetrics metrics;
    if (!platform_emoji_measure(string, length, font_size, &metrics)) {
        return NULL;
    }

    int image_id = nvgCreateImageRGBA(ui::vg, metrics.bounding_width, metrics.bounding_height, 0, NULL);
    if (!image_id) {
        return NULL;
    }

    PlatformEmojiRenderTarget target;
    target.image_id = image_id;
    target.top = 0;
    target.left = 0;
    platform_emoji_render(string, length, font_size, color(0xffffff), &target);

    // Create our texture
    emoji_textures.push_back(EmojiTexture());
    auto emoji_texture = &emoji_textures.back();
    emoji_texture->font_size       = font_size;
    emoji_texture->content_len     = length;
    emoji_texture->bounding_height = metrics.bounding_height;
    emoji_texture->bounding_width  = metrics.bounding_width;
    emoji_texture->baseline        = metrics.baseline;
    emoji_texture->left            = metrics.left;
    emoji_texture->width           = metrics.width;
    {
        char* data = (char*)malloc(length + 1);
        strncpy(data, string, length);
        data[length] = '\0';
        emoji_texture->content = data;
    }
    emoji_texture->image_id = image_id;

    return emoji_texture;
}


}
