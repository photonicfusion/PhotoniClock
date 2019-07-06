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
 * @file        LEDController.h
 * @summary     Generic LED Controller Library
 * @version     1.0
 * @author      nitacku
 * @data        18 July 2017
 */

#ifndef _LEDCONTROLLER_H
#define _LEDCONTROLLER_H

#include "IS31FL3218.h"
#define FASTLED_INTERNAL
#include <FastLED.h>

class CLED
{
    public:
    
    enum COUNT : uint8_t
    {
        LED = CIS31FL3218::COUNT::LED,
        RGB = CIS31FL3218::COUNT::RGB,
    };
    
    CLED(const uint8_t r_scale, const uint8_t g_scale, const uint8_t b_scale);
    
    void Initialize(void);
    void AssignLEDs(uint8_t leds[COUNT::LED]);
    uint8_t Update(void);
    void SetColor(const CRGB color);
    void SetColor(const CHSV hsv);
    void SetColor(const CRGB color, const uint8_t pixel);
    void SetColor(const CHSV hsv, const uint8_t pixel);
    void SetScale(const uint8_t r_scale, const uint8_t g_scale, const uint8_t b_scale);
    
    private:
    
    CIS31FL3218 m_driver; // LED Driver
    
    uint8_t* m_leds; // LED array
    uint8_t m_scale[3]; // Scaling values
};

#endif
