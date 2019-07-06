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
 * @file        IS31FL3218.cpp
 * @summary     IS31FL3218 LED Driver Library
 * @version     1.1
 * @author      nitacku
 * @data        14 July 2018
 */

#include "IS31FL3218.h"


CIS31FL3218::CIS31FL3218(void)
{
    reset();
}


void CIS31FL3218::Initialize(void)
{
    m_i2c_handle = nI2C->RegisterDevice(I2C_ADDRESS, 1, CI2C::Speed::FAST);
    ResetDevice();
    SetDeviceState(State::ENABLE);
    SetAllState(State::ENABLE);
}


CIS31FL3218::status_t CIS31FL3218::SetDeviceState(const State state)
{
    m_device_state = state;
    return I2CWriteByte(ADDRESS_SHUTDOWN, getValue(state));
}


CIS31FL3218::status_t CIS31FL3218::SetLEDPWM(const uint8_t led, const uint8_t value)
{
    if (LEDInRange(led))
    {
        m_led_pwm[led] = value;
        return I2CWriteByte(ADDRESS_PWM_00 + led, value);
    }
    
    return STATUS_OUT_OF_BOUNDS;
}


CIS31FL3218::status_t CIS31FL3218::SetLEDState(const uint8_t led, const State state)
{
    if (LEDInRange(led))
    {
        uint8_t address = ADDRESS_CONTROL_0 + (led / COUNT::GROUP);
        uint8_t byte = m_led_control[led / COUNT::GROUP];

        // Modify bit
        byte ^= (-getValue(state) ^ byte) & (0x1 << (led % COUNT::GROUP));
        m_led_control[led / COUNT::GROUP] = byte;

        return I2CWriteByte(address, byte);
    }
    
    return STATUS_OUT_OF_BOUNDS;
}


CIS31FL3218::status_t CIS31FL3218::SetAllPWM(const uint8_t value)
{
    memset(m_led_pwm, value, sizeof(m_led_pwm));
    return I2CWrite(ADDRESS_PWM_00, m_led_pwm, COUNT::LED);
}


CIS31FL3218::status_t CIS31FL3218::SetAllState(const State state)
{
    uint8_t byte = (state == State::ENABLE) ? 0x3F : 0x00;
    memset(m_led_control, byte, sizeof(m_led_control));
    return I2CWrite(ADDRESS_CONTROL_0, m_led_control, COUNT::BANK);
}


CIS31FL3218::status_t CIS31FL3218::SetAllPWM(const uint8_t pwm_array[COUNT::LED])
{
    memcpy(m_led_pwm, pwm_array, sizeof(m_led_pwm));
    return I2CWrite(ADDRESS_PWM_00, m_led_pwm, COUNT::LED);
}


CIS31FL3218::status_t CIS31FL3218::SetAllState(const State state_array[COUNT::LED])
{
    for (uint8_t led = 0; led < COUNT::LED; led++)
    {
        uint8_t byte = m_led_control[led / COUNT::GROUP];

        // Modify bit
        byte ^= (-getValue(state_array[led]) ^ byte) & (0x1 << (led % COUNT::GROUP));
        m_led_control[led / COUNT::GROUP] = byte;
    }
    
    return I2CWrite(ADDRESS_CONTROL_0, m_led_control, COUNT::BANK); 
}


CIS31FL3218::status_t CIS31FL3218::SetRGBPWM(const uint8_t rgb, const uint8_t pwm_array[3])
{
    if (rgb < COUNT::RGB)
    {
        m_led_pwm[(rgb * 3) + 0] = pwm_array[0];
        m_led_pwm[(rgb * 3) + 1] = pwm_array[1];
        m_led_pwm[(rgb * 3) + 2] = pwm_array[2];
        
        return I2CWrite(ADDRESS_PWM_00 + (rgb * 3), m_led_pwm + (rgb * 3), 3);
    }
    
    return STATUS_OUT_OF_BOUNDS;
}


CIS31FL3218::status_t CIS31FL3218::SetRGBPWM(const uint8_t rgb, const uint32_t color)
{
    uint8_t pwm_array[3];
    
    pwm_array[2] = ((color >>  0) & 0xFF); // B
    pwm_array[1] = ((color >>  8) & 0xFF); // G
    pwm_array[0] = ((color >> 16) & 0xFF); // R
    
    return SetRGBPWM(rgb, pwm_array);
}


CIS31FL3218::status_t CIS31FL3218::Update(void)
{
    return I2CWriteByte(ADDRESS_UPDATE, 0x55);
}


CIS31FL3218::status_t CIS31FL3218::ResetDevice(void)
{
    reset();
    return I2CWriteByte(ADDRESS_RESET, 0x55);
}


CIS31FL3218::State CIS31FL3218::GetDeviceState(void)
{
    return m_device_state;
}


uint8_t CIS31FL3218::GetLEDPWM(const uint8_t led)
{
    if (LEDInRange(led))
    {
        return m_led_pwm[led];
    }
    
    return STATUS_OUT_OF_BOUNDS;
}


CIS31FL3218::State CIS31FL3218::GetLEDState(const uint8_t led)
{
    if (LEDInRange(led))
    {
        uint8_t byte = m_led_control[led / COUNT::GROUP];
        bool state = (byte & (0x1 << (led % COUNT::GROUP)));
        return (state ? State::ENABLE : State::DISABLE);
    }
    
    return State::DISABLE;
}


void CIS31FL3218::GetAllPWM(uint8_t pwm_array[COUNT::LED])
{
    memcpy(pwm_array, m_led_pwm, COUNT::LED);
}


void CIS31FL3218::GetAllState(State state_array[COUNT::LED])
{
    for (uint8_t led = 0; led < COUNT::LED; led++)
    {
        State state = (m_led_control[led / COUNT::GROUP] & \
                      (0x1 << (led % COUNT::GROUP))) ? State::ENABLE : State::DISABLE;
        state_array[led] = state;
    }
}


void CIS31FL3218::GetRGBPWM(const uint8_t rgb, uint8_t pwm_array[3])
{
    if (rgb < COUNT::RGB)
    {
        memcpy(pwm_array, m_led_pwm + (rgb * 3), 3);
    }
}


uint32_t CIS31FL3218::GetRGBPWM(const uint8_t rgb)
{
    uint8_t pwm_array[3] = {0};
    GetRGBPWM(rgb, pwm_array);
    return (((uint32_t)pwm_array[0] << 16) | ((uint32_t)pwm_array[1] << 8) | ((uint32_t)pwm_array[2] << 0));
}


void CIS31FL3218::reset(void)
{
    memset(m_led_pwm, 0, sizeof(m_led_pwm));
    memset(m_led_control, 0, sizeof(m_led_control));
    m_device_state = State::DISABLE;
}


CIS31FL3218::status_t CIS31FL3218::I2CWrite(const uint8_t address, const uint8_t data[], const uint8_t bytes)
{
    return (status_t)nI2C->Write(m_i2c_handle, address, data, bytes);
}


CIS31FL3218::status_t CIS31FL3218::I2CWriteByte(const uint8_t address, const uint8_t data)
{
    uint8_t data_array[] = {data};
    return I2CWrite(address, data_array, 1);
}
