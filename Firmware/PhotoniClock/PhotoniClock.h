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
 * @file        PhotoniClock.h
 * @summary     Digital Clock for vacuum fluorescent display tubes
 * @version     1.3
 * @author      nitacku
 * @data        19 June 2019
 */

#ifndef _PHOTONICLOCK_H
#define _PHOTONICLOCK_H

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <nAudio.h>
#include <nCoder.h>
#include <nDisplay.h>
#include <nEEPROM.h>
#include <PCF2129.h>
#include "LEDController.h"
#include "Music.h"

const uint8_t VERSION       = 3;
const uint8_t DISPLAY_COUNT = 6;
const char CONFIG_KEY       = '$';
const uint8_t ALARM_COUNT   = 3;

// Macros to simplify port manipulation without additional overhead
#define getPinPort(pin)         ((pin < 8) ? PORTD : ((pin < A0) ? PORTB : PORTC))
#define getPinMask(pin)         _BV((pin < 8) ? pin : ((pin < A0) ? pin - 8 : pin - A0))
#define getPinMode(pin)         ((pin < 8) ? DDRD : ((pin < A0) ? DDRB : DDRC))
#define getPinState(pin)        (!!(getPinPort(pin) & getPinMask(pin)))
#define setPinHigh(pin)         (getPinPort(pin) |= getPinMask(pin))
#define setPinLow(pin)          (getPinPort(pin) &= ~getPinMask(pin))
#define setPinState(pin, state) (getPinPort(pin) ^= ((-state ^ getPinPort(pin)) & getPinMask(pin)))
#define setPinModeOutput(pin)   (getPinMode(pin) |= getPinMask(pin))
#define setPinModeInput(pin)    (getPinMode(pin) &= ~getPinMask(pin))

/* === Digit Representation ===

 777777
 6    5
 6    5
 444444
 0    1
 0    1 22
 333333 22

 Where bit == (1 << #)
=============================*/

static const uint8_t BITMAP[96] PROGMEM =
{
    0x00, 0x24, 0x60, 0x51, 0xDA, 0x35, 0xCA, 0x20, //  !"#$%&'
    0x41, 0x22, 0xF0, 0x32, 0x0C, 0x10, 0x04, 0x31, // ()*+,-./
    0xEB, 0x22, 0xB9, 0xBA, 0x72, 0xDA, 0xDB, 0xA2, // 01234567
    0xFB, 0xFA, 0x88, 0x8A, 0x39, 0x18, 0x5A, 0xB5, // 89:;<=>?
    0xBB, 0xF3, 0x5B, 0xC9, 0x3B, 0xD9, 0xD1, 0xFA, // @ABCDEFG
    0x73, 0x22, 0x2B, 0x73, 0x49, 0xE3, 0xE3, 0xEB, // HIJKLMNO
    0xF1, 0xF2, 0xC1, 0xDA, 0x59, 0x6B, 0x6B, 0x6B, // PQRSTUVW
    0x73, 0x7A, 0xB9, 0xC9, 0x52, 0xAA, 0xE0, 0x08, // XYZ[\]^_
    0x40, 0xBB, 0x5B, 0x19, 0x3B, 0xF9, 0xD1, 0xFA, // `abcdefg
    0x53, 0x02, 0x2B, 0x73, 0x49, 0x13, 0x13, 0x1B, // hijklmno
    0xF1, 0xF2, 0x11, 0xDA, 0x59, 0x0B, 0x0B, 0x0B, // pqrstuvw
    0x73, 0x7A, 0xB9, 0x01, 0x41, 0x02, 0x80, 0xFF, // xyz{|}~█
};

enum digital_pin_t : uint8_t
{
    DIGITAL_PIN_ENCODER_0 = 2,
    DIGITAL_PIN_ENCODER_1 = 3,
    DIGITAL_PIN_BUTTON = 4,
    DIGITAL_PIN_CLOCK = 10,
    DIGITAL_PIN_SDATA = 8,
    DIGITAL_PIN_LATCH = 9,
    DIGITAL_PIN_BLANK = 7,
    DIGITAL_PIN_HV_ENABLE = 5,
    DIGITAL_PIN_CATHODE_ENABLE = 6,
    DIGITAL_PIN_TRANSDUCER_0 = A0,
    DIGITAL_PIN_TRANSDUCER_1 = A1,
};

enum analog_pin_t : uint8_t
{
    ANALOG_PIN_PHOTODIODE = A3,
};

enum interrupt_speed_t : uint8_t
{
    INTERRUPT_FAST = 23, // 16MHz / (115Hz * 6 tubes * 8 levels * 128 prescale)
    INTERRUPT_SLOW = 255,
};

enum led_scale_t : uint8_t
{
    LED_SCALE_R = 255,
    LED_SCALE_G = 96,
    LED_SCALE_B = 96,
};

enum class FormatDate : uint8_t
{
    YYMMDD,
    MMDDYY,
    DDMMYY,
};

enum class FormatTime : uint8_t
{
    H24,
    H12,
};

enum class Cycle : uint8_t
{
    AM,
    PM,
};

enum class RTCSelect : uint8_t
{
    TIME,
    DATE,
};

enum class Effect : uint8_t
{
    NONE,
    SPIRAL,
    DATE,
    PHRASE,
};

enum class State : bool
{
    DISABLE,
    ENABLE,
};

struct StateStruct
{
    StateStruct()
    : voltage(State::DISABLE)
    , display(State::DISABLE)
    , alarm(State::DISABLE)
    , leds(State::DISABLE) // Start disabled to prevent IRQ lock-up
    , menu(State::DISABLE)
    {
        // empty
    }

    State voltage;
    State display;
    State alarm;
    State leds;
    State menu;
};

struct AlarmStruct
{
    AlarmStruct()
    : state(State::DISABLE)
    , music(0)
    , days(0)
    , time(0)
    {
        // empty
    }
    
    State       state;
    uint8_t     music;
    uint8_t     days;
    uint32_t    time;
};

struct Config
{
    Config()
    : validate(CONFIG_KEY)
    , noise(State::ENABLE)
    , night_mode(State::DISABLE)
    , brightness(CDisplay::Brightness::L8)
    , gain(8)
    , offset(8)
    , date_format(FormatDate::MMDDYY)
    , time_format(FormatTime::H12)
    , effect(Effect::NONE)
    , blank_begin(0)
    , blank_end(0)
    , music_timer(0)    
    , led_hue(240) // Retro arcade look
    , led_effect(7) // Color shift
    , alarm() // Default initialization
    {
        memcpy_P(phrase, PSTR("Photon"), DISPLAY_COUNT + 1);
    }

    char                    validate;
    State                   noise;
    State                   night_mode;
    CDisplay::Brightness    brightness;
    uint8_t                 gain;
    uint8_t                 offset;
    FormatDate              date_format;
    FormatTime              time_format;
    Effect                  effect;
    uint32_t                blank_begin;
    uint32_t                blank_end;
    uint8_t                 music_timer;
    uint8_t                 led_hue;
    uint8_t                 led_effect;
    AlarmStruct             alarm[ALARM_COUNT];
    char                    phrase[DISPLAY_COUNT + 1];
};

// Return integral value of Enumeration
template<typename T> constexpr auto getValue(const T e) noexcept
{
    return static_cast<uint8_t>(e);
}

//---------------------------------------------------------------------
// Implicit Function Prototypes
//---------------------------------------------------------------------

// delay            PhotoniClock
// analogRead       PhotoniClock
// attachInterrupt  CNcoder
// strlen           CDisplay
// strlen_P         CDisplay
// strncpy          CDisplay
// strcpy_P         CDisplay
// snprintf_P       PhotoniClock
// memcpy           PhotoniClock
// memset           PhotoniClock
// memcpy_P         PhotoniClock

//---------------------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------------------

// Mode functions
void Timer(const uint8_t hour, const uint8_t minute, const uint8_t second);
void Detonate(void);
void PlayAlarm(const uint8_t song_index, const char* phrase);

// Automatic functions
void AutoBrightness(void);
void AutoBlanking(void);
void AutoAlarm(void);

// Update functions
void UpdateAlarmIndicator(void);
void UpdateLEDBrightness(CDisplay::Brightness brightness);

// Format functions
uint8_t FormatHour(const uint8_t hour);
void FormatRTCString(const CRTC::RTC& rtc, char* s, const RTCSelect type);
uint32_t GetSeconds(const uint8_t hour, const uint8_t minute, const uint8_t second);

// Analog functions
CDisplay::Brightness ReadLightIntensity(void);

// EEPROM functions
void GetConfig(Config& g_config);
void SetConfig(const Config& g_config);

// State functions
void VoltageState(const State state);
void LEDState(const State state);
void DisplayState(const State state);

// Interrupt functions
void InterruptSpeed(const uint8_t speed);

// Callback functions
void EncoderCallback(void);
bool IsInputIncrement(void);
bool IsInputSelect(void);
bool IsInputUpdate(void);

#endif
