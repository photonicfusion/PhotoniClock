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
 * @file        LEDEffect.h
 * @summary     LED Effects for PhotoniClock
 * @version     1.0
 * @author      nitacku
 * @data        14 July 2018
 */

#ifndef _LEDEFFECT_H
#define _LEDEFFECT_H

#include "PhotoniClock.h"

// Menu order determined by enum order
enum LED_EFFECT : uint8_t
{
    // Effects that use Hue
    LED_EFFECT_STATIC, // Must be first
    LED_EFFECT_PULSE,
    LED_EFFECT_FIRE,
    LED_EFFECT_1982,
    
    // Effects that do not use Hue
    LED_EFFECT_PONG,
    LED_EFFECT_FADE,
    LED_EFFECT_RAINBOW,
    LED_EFFECT_CSHIFT,
    LED_EFFECT_GHOST,
    LED_EFFECT_DROPS,
    LED_EFFECT_DIGIT,
    LED_EFFECT_TETRIS,
    LED_EFFECT_CYCLE,   // Must be penultimate
    LED_EFFECT_DISABLE, // Must be last
    LED_EFFECT_COUNT,   // Number of entries
};

static const char LEDEffect_item_STATIC[] PROGMEM   = "Static";
static const char LEDEffect_item_PULSE[] PROGMEM    = "Pulse ";
static const char LEDEffect_item_FIRE[] PROGMEM     = " Fire ";
static const char LEDEffect_item_1982[] PROGMEM     = " 1982 ";
static const char LEDEffect_item_PONG[] PROGMEM     = " Pong ";
static const char LEDEffect_item_FADE[] PROGMEM     = " Fade ";
static const char LEDEffect_item_RAINBOW[] PROGMEM  = "Rainbo";
static const char LEDEffect_item_CSHIFT[] PROGMEM   = "CShift";
static const char LEDEffect_item_GHOST[] PROGMEM    = "Ghost ";
static const char LEDEffect_item_DROPS[] PROGMEM    = "Drops ";
static const char LEDEffect_item_DIGIT[] PROGMEM    = "Digit ";
static const char LEDEffect_item_TETRIS[] PROGMEM   = "Tetris";
static const char LEDEffect_item_CYCLE[] PROGMEM    = "Cycle ";
static const char LEDEffect_item_DISABLE[] PROGMEM  = "Dsable";

static PGM_P const LEDEffect_item_array[LED_EFFECT_COUNT] PROGMEM =
{
    [LED_EFFECT_STATIC] = LEDEffect_item_STATIC,
    [LED_EFFECT_PULSE] = LEDEffect_item_PULSE,
    [LED_EFFECT_FIRE] = LEDEffect_item_FIRE,
    [LED_EFFECT_1982] = LEDEffect_item_1982,
    [LED_EFFECT_PONG] = LEDEffect_item_PONG,
    [LED_EFFECT_FADE] = LEDEffect_item_FADE,
    [LED_EFFECT_RAINBOW] = LEDEffect_item_RAINBOW,
    [LED_EFFECT_CSHIFT] = LEDEffect_item_CSHIFT,
    [LED_EFFECT_GHOST] = LEDEffect_item_GHOST,
    [LED_EFFECT_DROPS] = LEDEffect_item_DROPS,
    [LED_EFFECT_DIGIT] = LEDEffect_item_DIGIT,
    [LED_EFFECT_TETRIS] = LEDEffect_item_TETRIS,
    [LED_EFFECT_CYCLE] = LEDEffect_item_CYCLE,
    [LED_EFFECT_DISABLE] = LEDEffect_item_DISABLE,
};

void LEDProcess(const uint8_t effect);
void LEDEffectStatic(void);
void LEDEffectPulse(void);
void LEDEffectFire(void);
void LEDEffectPong(bool);
void LEDEffectFade(void);
void LEDEffectRainbow(void);
void LEDEffectRainbowShift(void);
void LEDEffectGhost(void);
void LEDEffectDigitDrops(void);
void LEDEffectDigit(void);
void LEDEffectTetris(void);
void LEDEffectCycle(void);
void LEDEffectDisable(void);

#endif
