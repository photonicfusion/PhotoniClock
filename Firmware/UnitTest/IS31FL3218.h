/*
 * Copyright (c) 2018 nitacku
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
 * @file        IS31FL3218.h
 * @summary     IS31FL3218 LED Driver Library
 * @version     1.1
 * @author      nitacku
 * @data        14 July 2018
 */

#ifndef _IS31FL3218_H
#define _IS31FL3218_H

#include <inttypes.h>
#include <nI2C.h>

class CIS31FL3218
{
    public:

    enum status_t : uint8_t
    {
        STATUS_OK = 0,
        STATUS_OUT_OF_BOUNDS,
    };
    
    enum class State : bool
    {
        DISABLE,
        ENABLE,
    };
    
    enum COUNT : uint8_t
    {
        LED = 18,
        RGB = (LED / 3),
        BANK = 3,
        GROUP = (LED / BANK),
    };
    
    private:

    static const uint8_t I2C_ADDRESS = (0xA8 >> 1);
    CI2C::Handle m_i2c_handle;
    
    State m_device_state;
    uint8_t m_led_pwm[COUNT::LED];
    uint8_t m_led_control[COUNT::BANK];
    
    enum address_t : uint8_t
    {
        ADDRESS_SHUTDOWN        = 0x00,
        ADDRESS_PWM_00          = 0x01,
        ADDRESS_PWM_01          = 0x02,
        ADDRESS_PWM_02          = 0x03,
        ADDRESS_PWM_03          = 0x04,
        ADDRESS_PWM_04          = 0x05,
        ADDRESS_PWM_05          = 0x06,
        ADDRESS_PWM_06          = 0x07,
        ADDRESS_PWM_07          = 0x08,
        ADDRESS_PWM_08          = 0x09,
        ADDRESS_PWM_09          = 0x0A,
        ADDRESS_PWM_10          = 0x0B,
        ADDRESS_PWM_11          = 0x0C,
        ADDRESS_PWM_12          = 0x0D,
        ADDRESS_PWM_13          = 0x0E,
        ADDRESS_PWM_14          = 0x0F,
        ADDRESS_PWM_15          = 0x10,
        ADDRESS_PWM_16          = 0x11,
        ADDRESS_PWM_17          = 0x12,
        ADDRESS_CONTROL_0       = 0x13,
        ADDRESS_CONTROL_1       = 0x14,
        ADDRESS_CONTROL_2       = 0x15,
        ADDRESS_UPDATE          = 0x16,
        ADDRESS_RESET           = 0x17,
    };
    
    public:

    CIS31FL3218(void);
    
    void Initialize(void);
    status_t SetDeviceState(const State state);
    status_t SetLEDPWM(const uint8_t led, const uint8_t value);
    status_t SetLEDState(const uint8_t led, const State state);
    status_t SetAllPWM(const uint8_t value);
    status_t SetAllState(const State state);
    status_t SetAllPWM(const uint8_t pwm_array[COUNT::LED]);
    status_t SetAllState(const State state_array[COUNT::LED]);
    status_t SetRGBPWM(const uint8_t rgb, const uint8_t pwm_array[3]);
    status_t SetRGBPWM(const uint8_t rgb, const uint32_t color);
    status_t Update(void);
    status_t ResetDevice(void);

    State GetDeviceState(void);
    uint8_t GetLEDPWM(const uint8_t led);
    State GetLEDState(const uint8_t led);
    void GetAllPWM(uint8_t pwm_array[COUNT::LED]);
    void GetAllState(State state_array[COUNT::LED]);
    void GetRGBPWM(const uint8_t rgb, uint8_t pwm_array[3]);
    uint32_t GetRGBPWM(const uint8_t rgb);

    private:
    
    // Return integral value of Enumeration
    template<typename T> constexpr auto getValue(const T e)
    {
        return static_cast<uint8_t>(e);
    }
    
    constexpr inline bool LEDInRange(const uint8_t led) __attribute__((always_inline))
    {
        return (led < COUNT::LED);
    }
    
    void reset(void);
    status_t I2CWrite(const uint8_t address, const uint8_t data[], const uint8_t bytes);
    status_t I2CWriteByte(const uint8_t address, const uint8_t data);
};

#endif
