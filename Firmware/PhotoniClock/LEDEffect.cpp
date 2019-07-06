/*
 * Copyright (c) 2018 PhotonicFusion LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * @file        LEDEffect.cpp
 * @summary     LED Effects for PhotoniClock
 * @version     1.0
 * @author      nitacku
 * @data        14 July 2018
 */

#include "LEDEffect.h"
 
extern StateStruct g_state;
extern Config g_config;
extern CLED g_led_controller;
extern uint8_t g_leds[CLED::COUNT::LED];
extern CRTC::RTC* g_rtc_struct;


struct CArray
{
    union
    {
        uint16_t array16[CLED::COUNT::RGB / 2];
        uint8_t array8[CLED::COUNT::RGB] = {0};
    };

    // Array access operator to index into the CArray object
	inline uint8_t& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return array8[x];
    }

    // Array access operator to index into the CArray object
    inline const uint8_t& operator[] (uint8_t x) const __attribute__((always_inline))
    {
        return array8[x];
    }
};

// Static globals to reduce SRAM usage
static int8_t g_addend = 1;
static uint8_t g_hue = 0;
static uint8_t g_value = 0;
static CRGB g_rgb;
static CArray g_array;

// Called periodically on 1/60Hz intervals
void LEDProcess(const uint8_t effect)
{
    // If in menu, fake changes to RTC
    if (g_state.menu == State::ENABLE)
    {
        static uint8_t cycle = 0;
        
        // Update every second
        if (cycle++ > 59)
        {
            g_rtc_struct->second++;
            cycle = 0;
        }
    }
    
    switch (effect)
    {
    default:
    case LED_EFFECT_STATIC:
        LEDEffectStatic();
        break;
    case LED_EFFECT_PULSE:
        LEDEffectPulse();
        break;
    case LED_EFFECT_FIRE:
        LEDEffectFire();
        break;
    case LED_EFFECT_1982:
        LEDEffectPong(true);
        break;
    case LED_EFFECT_PONG:
        LEDEffectPong(false);
        break;
    case LED_EFFECT_FADE:
        LEDEffectFade();
        break;
    case LED_EFFECT_RAINBOW:
        LEDEffectRainbow();
        break;
    case LED_EFFECT_CSHIFT:
        LEDEffectRainbowShift();
        break;
    case LED_EFFECT_GHOST:
        LEDEffectGhost();
        break;
    case LED_EFFECT_DROPS:
        LEDEffectDigitDrops();
        break;
    case LED_EFFECT_DIGIT:
        LEDEffectDigit();
        break;
    case LED_EFFECT_TETRIS:
        LEDEffectTetris();
        break;
    case LED_EFFECT_CYCLE:
        LEDEffectCycle();
        break;
    case LED_EFFECT_DISABLE:
        LEDEffectDisable();
        break;
    }
}


void LEDEffectStatic(void)
{
    CHSV hsv = {g_config.led_hue, 255, 255};
    g_led_controller.SetColor(hsv);
}


void LEDEffectPulse(void)
{
    g_value += (uint8_t)g_addend;
    
    if (g_value == 0)
    {
        g_addend = -g_addend;
        g_value += (uint8_t)g_addend;
    }

    CHSV hsv = {g_config.led_hue, 255, g_value};
    g_led_controller.SetColor(hsv);
}


void LEDEffectFire(void)
{    
    CHSV hsv = {0, 255, 255};
    CRGB low;
    CRGB mid;
    CRGB high;
    
    hsv.hue = g_config.led_hue + 25;
    hsv2rgb_spectrum(hsv, mid);
    hsv.hue = g_config.led_hue + 00;
    hsv2rgb_spectrum(hsv, low);
    hsv.saturation = 200;
    hsv2rgb_spectrum(hsv, high);
    
    CRGBPalette16 palette = CRGBPalette16(CRGB::Black, low, mid, high);

    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < CLED::COUNT::RGB; i++)
    {
        g_array[i] = qsub8(g_array[i], random8(0, (18 / CLED::COUNT::RGB) + 3));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = CLED::COUNT::RGB - 1; k >= 2; k--)
    {
        g_array[k] = (g_array[k - 1] + g_array[k - 2] + g_array[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks'
    if (random8() < 100)
    {
        uint8_t y = random8(4);
        g_array[y] = qadd8(g_array[y], random8(15, 25));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t pixel = 0; pixel < CLED::COUNT::RGB; pixel++)
    {
        CRGB color = ColorFromPalette(palette, g_array[pixel]);
        g_led_controller.SetColor(color, pixel);
    }
}


void LEDEffectPong(bool use_led_hue = true)
{
    const uint8_t VALUE_ADJUST = (use_led_hue ? 12 : 5);
    static uint8_t position = 0;

    // Delay position update
    if (g_value++ > ((use_led_hue) ? 3 : 8))
    {
        g_value = 0;
        position += (uint8_t)g_addend;
        
        if (use_led_hue)
        {
            g_hue = g_config.led_hue;
        }
        else
        {
            g_hue += 8;
        }
        
        // Position allowed to travel "off screen"
        if ((position == 0) || (position == (CLED::COUNT::RGB + 3)))
        {
            g_addend = -g_addend;
        }
        
        // Position could go out of bounds if the routine enters when position
        // is at the boundary and g_addend is reverse polarity
        if (position > (CLED::COUNT::RGB + 3))
        {
            // Reset to safe position
            position = 2;
        }
        
        // Re-illuminate pixels
        if ((position > 1) && (position < CLED::COUNT::RGB + 2))
        {
            g_array[position - 2] = 255;
        }
    }

    CHSV hsv = {g_hue, 255, 0};
    
    // Iterate through all pixels and update value
    for (uint8_t pixel = 0; pixel < CLED::COUNT::RGB; pixel++)
    {
        hsv.value = g_array[pixel];
        g_led_controller.SetColor(hsv, pixel);
        g_array[pixel] = qsub8(g_array[pixel], VALUE_ADJUST);
    }
}


void LEDEffectFade(void)
{
    enum FADE : uint8_t
    {
        FADE_GENERATE,
        FADE_TRANSITION,
        FADE_WAIT,
    };

    static uint8_t state = FADE_GENERATE;
    
    switch (state)
    {
    default:
    case FADE_GENERATE:
    {
        // Calculate RGB values of current and next hue
        CHSV current_hsv = {g_hue, 255, 255};
        CRGB current_rgb;
        hsv2rgb_spectrum(current_hsv, current_rgb);
        g_hue = random8();
        CHSV next_hsv = {g_hue, 255, 255};
        CRGB next_rgb;
        hsv2rgb_spectrum(next_hsv, next_rgb);
        
        // Calculate hue offsets in RGB values
        for (uint8_t index = 0; index < 3; index++)
        {
            g_rgb[index] = ((next_rgb[index] - current_rgb[index]) / 2); // Sign conversion
            g_array.array16[index] = (100 * current_rgb[index]); // Pseudo float
        }

        g_value = 0;
        state = FADE_TRANSITION;
        break;
    }
    case FADE_TRANSITION:
    {
        CRGB color;
        
        for (uint8_t index = 0; index < 3; index++)
        {
            g_array.array16[index] += ((int8_t)g_rgb[index]); // Apply signed offset
            color[index] = (g_array.array16[index] / 100); // Pseudo float
        }
        
        g_led_controller.SetColor(color);
        
        if (g_value++ >= 199)
        {
            state = FADE_WAIT;
            g_value = 0;
        }
        
        break;
    }
    case FADE_WAIT:
        if (g_value++ >= 199)
        {
            state = FADE_GENERATE;
        }
        break;
    }
}


void LEDEffectRainbow(void)
{
    uint8_t value = ++g_hue;
    CHSV hsv = {value, 255, 255};
    g_led_controller.SetColor(hsv);
}


void LEDEffectRainbowShift(void)
{
    g_hue++;
    
    for (uint8_t pixel = 0; pixel < CLED::COUNT::RGB; pixel++)
    {
        uint8_t value = g_hue + (pixel * 16);
        CHSV hsv = {value, 255, 255};
        g_led_controller.SetColor(hsv, pixel);
    }
}


void LEDEffectGhost(void)
{
    // Allow "off screen" writes
    const uint8_t WIDTH = 2; // pixels left/right of orb center
    const uint8_t OVERSCAN = 1; // non-visible pixels left/right of display
    static CRGB led[CLED::COUNT::RGB + (OVERSCAN * 2)] = {0};
    static uint8_t position = 0;
    static uint8_t color_index = 0;

    if (g_value < 2)
    {
        color_index = random8(3);
        position = WIDTH + random8(CLED::COUNT::RGB + (OVERSCAN * 2) - (WIDTH * 2));
    }
    
    g_value += 2;
    
    for (uint8_t offset = 0; offset < ((WIDTH * 2) + 1); offset++) // Add one for center
    {
        uint8_t fade = 50 * abs(offset - WIDTH);

        // Create fade effect from center of orb   
        if (((fade + g_value) < 250) && ((g_value + 10) > fade))
        {
            uint8_t value = qadd8(led[position + offset - WIDTH][color_index], 2);
            led[position + offset - WIDTH][color_index] = value;
        }
    }
    
    // Iterate through all visible pixels and update value
    for (uint8_t pixel = 0; pixel < CLED::COUNT::RGB; pixel++)
    {
        g_led_controller.SetColor(led[pixel + OVERSCAN], pixel);
        led[pixel + OVERSCAN]--;
    }
}


void LEDEffectDigitDrops(void)
{
    static uint8_t previous_second = 0;
    const uint8_t decay = 2; //Higher is faster
    const uint8_t pull = 5; //Lower is faster

    //Just ticked a second
    if (previous_second != g_rtc_struct->second)
    {
        previous_second = g_rtc_struct->second;
        uint8_t digit = 3 * random8(CLED::COUNT::RGB);

        uint8_t offset = (g_rtc_struct->second % 3);
        g_leds[digit + offset] = 255;
        g_rgb[offset] = 255;
    }

    //Make temporary array
    uint8_t temp[CLED::COUNT::LED] = {0};
    
    //Spread
    //1 = min(((2 >> pull) + 1), max)
    for (uint8_t index = 0; index < 3; index++)
    {
        temp[index] = min((g_leds[index + 3] >> pull) + g_leds[index], g_rgb[index]);
    }

    //2 = min(((3 >> pull) + (1 >> pull) + 2), max) (same through 5)
    for (uint8_t i = 3; i < 15; i += 3)
    {
        for (uint8_t j = 0; j < 3; j++)
        {
            temp[(i + j)] = min((g_leds[(i + j)+3] >> pull) + (g_leds[(i + j)-3] >> pull) + g_leds[(i + j)], g_rgb[j]);
        }
    }

    //6 = min(((5 >> pull) + 1), max)
    for (uint8_t index = 0; index < 3; index++)
    {
        temp[index + 15] = min((g_leds[index + 12] >> pull) + g_leds[index + 15], g_rgb[index]);
        g_rgb[index] = qsub8(g_rgb[index], decay); // Decay
    }
    
    //Copy data from temp to g_leds
    memcpy8(g_leds, temp, CLED::COUNT::LED);
}


void LEDEffectDigit(void)
{
    CHSV hsv = {0, 255, 255};
    
    for (uint8_t group = 0; group < 3; group++)
    {        
        switch (group)
        {
        case 0: // hour
            hsv.hue = 10 * g_rtc_struct->hour;
            break;
        case 1: // minute
            hsv.hue = 4 * g_rtc_struct->minute;
            break;
        case 2: // second
            hsv.hue = 4 * g_rtc_struct->second;
            break;
        }

        g_led_controller.SetColor(hsv, (2 * group) + 0);
        g_led_controller.SetColor(hsv, (2 * group) + 1);
    }
}


void LEDEffectTetris(void)
{
    enum TETRIS : uint8_t
    {
        TETRIS_GENERATE,
        TETRIS_SHIFT_IN,
        TETRIS_SHIFT_OUT,
    };
    
    if (++g_value > 8)
    {
        static uint8_t state = TETRIS_GENERATE;
        g_value = 0;
        
        switch (state)
        {
        default:
        case TETRIS_GENERATE:
            // Check if full
            if (g_array[CLED::COUNT::RGB - 1] > 0)
            {
                state = TETRIS_SHIFT_OUT;
            }
            else
            {
                g_array[CLED::COUNT::RGB - 1] = random8(1, 255);
                state = TETRIS_SHIFT_IN;
            }
            break;
        case TETRIS_SHIFT_IN:
        {
            bool shift = false;
            
            // Shift color until non-zero value
            for (uint8_t index = 0; index < CLED::COUNT::RGB - 1; index++)
            {
                // Check if space exists
                if (g_array[index] == 0)
                {
                    // Check if next space occupied
                    if (g_array[index + 1] > 0)
                    {
                        shift = true;
                        g_array[index] = g_array[index + 1]; // Slide
                        g_array[index + 1] = 0; // Create empty space
                    }
                }
            }

            // Check if all shifts occurred
            if (shift == false) { state = TETRIS_GENERATE; }
            
            break;
        }
        case TETRIS_SHIFT_OUT:
        {
            bool shift = false;
            
            for (uint8_t index = 0; index < CLED::COUNT::RGB; index++)
            {
                // Check if space occupied
                if (g_array[index] > 0)
                {
                    if (index > 0)
                    {
                        g_array[index - 1] = g_array[index];
                    }
                    
                    shift = true;
                    g_array[index] = 0;
                    break; // End loop
                }
            }

            // Check if all shifts occurred
            if (shift == false) { state = TETRIS_GENERATE; }
            
            break;
        }
        }
        
        CHSV hsv = {0, 255, 255};
        
        // Iterate through all pixels and update value
        for (uint8_t pixel = 0; pixel < CLED::COUNT::RGB; pixel++)
        {
            if (g_array[pixel] > 0)
            {
                hsv.hue = g_array[pixel];
                hsv.value = 255;
            }
            else
            {
                hsv.value = 0;
            }
            
            g_led_controller.SetColor(hsv, pixel);
        }
    }
}


void LEDEffectCycle(void)
{
    static uint8_t previous_minute = 0;
    static uint8_t effect = 0;
    
    if (previous_minute != g_rtc_struct->minute)
    {
        previous_minute = g_rtc_struct->minute;
        effect++;
    }
    
    // Select effect based on current minute
    // Skip Static and Disabled settings
    LEDProcess(1 + (effect % (LED_EFFECT_CYCLE - 1)));
}


void LEDEffectDisable(void)
{
    CHSV hsv = {0, 0, 0};
    g_led_controller.SetColor(hsv);
    //g_led_controller.SetColor(CRGB::Black);
}
