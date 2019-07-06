/*
 * Copyright (c) 2017 nitacku
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
 * @file        LEDController.cpp
 * @summary     Generic LED Controller Library
 * @version     1.0
 * @author      nitacku
 * @data        18 July 2017
 */

#include "LEDController.h"
#include <FastLED.h>

CLED::CLED(const uint8_t r_scale, const uint8_t g_scale, const uint8_t b_scale)
    : m_leds{nullptr}
{
    SetScale(r_scale, g_scale, b_scale);
}

void CLED::Initialize(void)
{
    m_driver.Initialize();
}


void CLED::AssignLEDs(uint8_t leds[COUNT::LED])
{
    m_leds = leds; // Assign pointer
}


uint8_t CLED::Update(void)
{
#ifdef REVISION_A
    static const uint8_t swap[COUNT::LED] = {0, 1, 2, 9, 10, 11, 3, 4, 5, 12, 13, 14, 6, 7, 8, 15, 16, 17};
    uint8_t reorder[COUNT::LED];
#endif
    uint8_t scaled_leds[COUNT::LED];
    
    if (m_leds != nullptr)
    {
        // Create local copy
        memcpy8(scaled_leds, m_leds, sizeof(scaled_leds));

        for (uint8_t led = 0; led < COUNT::LED; led++)
        {
            nscale8_LEAVING_R1_DIRTY(scaled_leds[led], m_scale[led % 3]);
        }

        cleanup_R1();

#ifdef REVISION_A
        // Reorder LEDs according to physical mapping
        for (uint8_t index = 0; index < COUNT::LED; index++)
        {
            reorder[swap[index]] = scaled_leds[index];
        }

        m_driver.SetAllPWM(reorder);
#else
        m_driver.SetAllPWM(scaled_leds);
#endif
        return m_driver.Update();
    }
    
    return -1; // Error
}


void CLED::SetColor(const CRGB color)
{
    for (uint8_t led = 0; led < COUNT::LED; led += 3)
    {
        m_leds[led + 0] = color.r;
        m_leds[led + 1] = color.g;
        m_leds[led + 2] = color.b;
    }
}


void CLED::SetColor(const CHSV hsv)
{
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb); // Convert Hue to RGB
    SetColor(rgb);
}


void CLED::SetColor(const CRGB color, const uint8_t pixel)
{
    if (pixel < COUNT::RGB)
    {
        m_leds[(3 * pixel) + 0] = color.r;
        m_leds[(3 * pixel) + 1] = color.g;
        m_leds[(3 * pixel) + 2] = color.b;
    }
}


void CLED::SetColor(const CHSV hsv, const uint8_t pixel)
{
    CRGB rgb;
    hsv2rgb_spectrum(hsv, rgb); // Convert Hue to RGB
    SetColor(rgb, pixel);
}


void CLED::SetScale(const uint8_t r_scale, const uint8_t g_scale, const uint8_t b_scale)
{
    m_scale[0] = r_scale;
    m_scale[1] = g_scale;
    m_scale[2] = b_scale;
}
