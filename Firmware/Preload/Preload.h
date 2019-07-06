/*
 * Copyright (c) 2017 PhotonicFusion LLC
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
 * @file        Preload.h
 * @summary     SPI/UART Slave preloader for EEPROM
 * @version     1.2
 * @author      nitacku
 * @data        28 June 2018
 */

#ifndef _PRELOAD_H
#define _PRELOAD_H
 
#include <avr/pgmspace.h>
#include <SPI.h>
#include "nEEPROM.h"
#include <nDisplay.h>
#include "LEDController.h"
#include "crc8.h"

const uint8_t       DISPLAY_COUNT = 6;
const uint8_t       RETRY_LIMIT = 3;
const uint8_t       POLL_SECTION_INDICATOR = 0xFC;
const uint8_t       POLL_STATUS_INDICATOR = 0xFD;
const uint8_t       SECTION_INDICATOR = 0xFE;
const uint8_t       ESCAPE_INDICATOR = 0xFF;
const float         VOLTAGE_EXPECTED = 58.0;
const float         VOLTAGE_THRESHOLD = 3.0;
const uint16_t      BUFFER_SIZE = 300;

// Macros to simplify port manipulation without additional overhead
#define getPort(pin)    ((pin < 8) ? PORTD : ((pin < A0) ? PORTB : PORTC))
#define getMask(pin)    _BV((pin < 8) ? pin : ((pin < A0) ? pin - 8 : pin - A0))
#define setPinHigh(pin) (getPort(pin) |= getMask(pin))
#define setPinLow(pin)  (getPort(pin) &= ~getMask(pin))

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

enum led_scale_t : uint8_t
{
    LED_SCALE_R = 255,
    LED_SCALE_G = 96,
    LED_SCALE_B = 96,
};

enum Status : uint8_t
{
    ERROR, // "Live 0" for SPI
    OK,
    BUSY,
    RETRY,
    COMPLETE,
};

enum class S : bool
{
    DISABLE,
    ENABLE,
};

enum class State : uint8_t
{
    INITIAL,
    COMPLETE,
    LOAD_A,
    LOAD_B,
    ERROR,
};

struct StateStruct
{
    StateStruct()
    : voltage(S::DISABLE)
    , display(S::DISABLE)
    , SPI(S::ENABLE)
    {
        // empty
    }

    S voltage;
    S display;
    S SPI;
};

// Return integral value of Enumeration
template<typename T> constexpr uint8_t getValue(const T e)
{
    return static_cast<uint8_t>(e);
}

//---------------------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------------------

Status WriteData(volatile uint8_t data[], volatile uint16_t bytes);
Status VerifyCRC(volatile uint8_t data[], volatile uint16_t bytes);
Status VerifyData(volatile const uint16_t address, volatile const uint8_t data[], volatile uint16_t bytes);

// State functions
void VoltageState(const S state);
void DisplayState(const S state);
void SPIState(const S state);
Status EnterErrorState(bool allow_retry);
Status TestVoltage(void);
uint8_t process(uint8_t byte);

#endif
