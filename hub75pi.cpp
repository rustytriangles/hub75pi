#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"

#include "hub75.hpp"
#include "typewriter_font.hpp"
#include "pi.hpp"

// Display size in pixels
// Should be either 64x64 or 32x32 but perhaps 64x32 an other sizes will work.
// Note: this example uses only 5 address lines so it's limited to 64 pixel high displays (32*2).
const uint8_t WIDTH = 128;
const uint8_t HEIGHT = 64;

Hub75 hub75(WIDTH, HEIGHT, nullptr, PANEL_GENERIC, true);

void __isr dma_complete() {
    hub75.dma_complete();
}

inline uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}

Pixel alpha_blend(unsigned char alpha, const Pixel& foreground, const Pixel& background) {
    if (alpha == 0) {
        return background;
    } else if (alpha == 255) {
        return foreground;
    }

    float a = (float)alpha / 255.f;
    float r = a * (float)( foreground.color      & 0x3ff) / 1023.f + (1.f - a)*(float)( background.color      & 0x3ff) / 1023.f;
    float g = a * (float)((foreground.color>>10) & 0x3ff) / 1023.f + (1.f - a)*(float)((background.color>>10) & 0x3ff) / 1023.f;
    float b = a * (float)((foreground.color>>20) & 0x3ff) / 1023.f + (1.f - a)*(float)((background.color>>20) & 0x3ff) / 1023.f;

    Pixel result = {};
    result.color = (unsigned int)(r * 1023.f) | (unsigned int)(g * 1023.f)<<10 | (unsigned int)(b * 1023.f) << 20;
    return result;
}

bool choose_glyph(unsigned int &width, unsigned int &height, const unsigned char* &pixels, char digit) {
    switch (digit) {
    case '0':
        width = zero::width;
        height = zero::height;
        pixels = &zero::pixels[0][0];
        break;
    case '1':
        width = one::width;
        height = one::height;
        pixels = &one::pixels[0][0];
        break;
    case '2':
        width = two::width;
        height = two::height;
        pixels = &two::pixels[0][0];
        break;
    case '3':
        width = three::width;
        height = three::height;
        pixels = &three::pixels[0][0];
        break;
    case '4':
        width = four::width;
        height = four::height;
        pixels = &four::pixels[0][0];
        break;
    case '5':
        width = five::width;
        height = five::height;
        pixels = &five::pixels[0][0];
        break;
    case '6':
        width = six::width;
        height = six::height;
        pixels = &six::pixels[0][0];
        break;
    case '7':
        width = seven::width;
        height = seven::height;
        pixels = &seven::pixels[0][0];
        break;
    case '8':
        width = eight::width;
        height = eight::height;
        pixels = &eight::pixels[0][0];
        break;
    case '9':
        width = nine::width;
        height = nine::height;
        pixels = &nine::pixels[0][0];
        break;
    case '.':
        width = decimal::width;
        height = decimal::height;
        pixels = &decimal::pixels[0][0];
        break;
    default:
        width = 0;
        height = 0;
        pixels = nullptr;
        return false;
    }
    return true;
}

bool draw_glyph(int x_off, int y_off, unsigned int width, unsigned int height, const unsigned char *pixels, Pixel& foreground, Pixel& background) {
    if (x_off >= WIDTH) {
        return true;
    }
    if ((x_off + (int)width) < 0) {
        return true;
    }

    bool restart = true;
    for (uint y=0; y<std::min((unsigned int)HEIGHT, height); y++) {
        for (uint x=0; x<width; x++) {
            int pixel_x = (int)x + x_off;
            if (pixel_x >= 0 && pixel_x < WIDTH) {
                restart = false;
                unsigned char alpha = pixels[y*width+x];
                hub75.set_color((unsigned int)pixel_x, y + y_off, alpha_blend(alpha, foreground, background));
            }
        }
    }

    return restart;
}

int main() {
    stdio_init_all();

    sleep_us(100);
    set_sys_clock_khz(266000, true);

    hub75.start(dma_complete);

    int x_start = 75;
    char prev_digit = 'X';
    while (true) {

        hub75.background = hsv_to_rgb(millis() / 10000.0f, 1.0f, 0.5f);
        Pixel foreground = Pixel(255,255,255);

        bool restart = true;
        int x_off = x_start;
        int y_off = 14;

        const unsigned int num_char = strlen(digits);
        for (unsigned int i=0; i<num_char; i++) {
            unsigned int width = 0;
            unsigned int height = 0;
            const unsigned char* pixels;
            char digit = digits[i];
            if (!choose_glyph(width, height, pixels, digit)) {
                continue;
            }
            // This is my kerning table!
            if (prev_digit == '7' && digit == '4') {
                x_off -= 2;
            } else {
                x_off += 2;
            }
            restart &= draw_glyph(x_off, y_off,  width, height, pixels, foreground, hub75.background);
            x_off += width;

            prev_digit = digit;
        }

        hub75.flip(false);
        sleep_ms(100 / 60);

        x_start -= 1;
        if (restart) {
            x_start = 75;
            prev_digit = 'X';
        }
    }
}
