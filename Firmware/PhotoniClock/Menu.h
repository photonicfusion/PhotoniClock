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
 * @file        Menu.h
 * @summary     Menu for PhotoniClock
 * @version     1.2
 * @author      nitacku
 * @data        14 July 2018
 */

#ifndef _MENU_H
#define _MENU_H
 
#include "PhotoniClock.h"
#include "LEDEffect.h"

typedef type_array type_const_char_ptr;
typedef type_item type_const_uint8;

enum Timeout : uint32_t
{
    INFO   = 200000,
    MENU   =    150,
    SELECT =    500,
    VALUE  =   5000,
};

// Menu order determined by enum order
enum MENU_ITEM : uint8_t
{
    MENU_ITEM_ALARM,
    MENU_ITEM_BRIGHTNESS,
    MENU_ITEM_CONFIG,
    MENU_ITEM_BLANK,
    MENU_ITEM_TIME,
    MENU_ITEM_DATE,
    MENU_ITEM_LED,
    MENU_ITEM_MUSIC,
    MENU_ITEM_TIMER,
    MENU_ITEM_COUNT, // Number of menu items
};

static const char menu_item_ALARM[] PROGMEM         = "Alarm ";
static const char menu_item_BRIGHTNESS[] PROGMEM    = "Bright";
static const char menu_item_CONFIG[] PROGMEM        = "Config";
static const char menu_item_BLANK[] PROGMEM         = "Dsplay";
static const char menu_item_TIME[] PROGMEM          = " Time ";
static const char menu_item_DATE[] PROGMEM          = " Date ";
static const char menu_item_LED[] PROGMEM           = " LEDs ";
static const char menu_item_MUSIC[] PROGMEM         = "Audio ";
static const char menu_item_TIMER[] PROGMEM         = "CountD";

static PGM_P const menu_item_array[MENU_ITEM_COUNT] PROGMEM =
{
    [MENU_ITEM_ALARM] = menu_item_ALARM,
    [MENU_ITEM_BRIGHTNESS] = menu_item_BRIGHTNESS,
    [MENU_ITEM_CONFIG] = menu_item_CONFIG,
    [MENU_ITEM_BLANK] = menu_item_BLANK,
    [MENU_ITEM_TIME] = menu_item_TIME,
    [MENU_ITEM_DATE] = menu_item_DATE,
    [MENU_ITEM_LED] = menu_item_LED,
    [MENU_ITEM_MUSIC] = menu_item_MUSIC,
    [MENU_ITEM_TIMER] = menu_item_TIMER,
};

void MenuInfo(void);
void MenuSettings(void);
int8_t SelectCycle(const Cycle init_value);
int8_t SelectState(CDisplay::PromptSelectStruct& prompt_select);
bool SelectRTCValue(CDisplay::PromptValueStruct& prompt_value);
bool RestoreOutOfBox(void);
bool SetBlank(void);
bool SetBrightness(void);
bool SetNightMode(void);
bool SetGain(void);
bool SetOffset(void);
bool SetTimeFormat(void);
bool SetDateFormat(void);
bool SetEffect(void);
bool SetBlip(void);
bool SetTime(void);
bool SetDate(void);
bool SetAlarmState(uint8_t& alarm);
bool SetAlarmTime(const uint8_t alarm);
bool SetAlarmDays(const uint8_t alarm);
bool SetPhrase(void);
bool SetMusic(uint8_t& music);
void SetTimer(void);
bool SetLEDEffect(void);
bool SetLEDHue(void);

#endif
