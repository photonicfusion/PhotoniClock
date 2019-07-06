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
 * @file        UnitTest.cpp
 * @summary     Unit test procedure
 * @version     1.0
 * @author      nitacku
 * @data        28 June 2018
 */

#include "UnitTest.h"

//---------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------

// Object variables
StateStruct     g_state;
CAudio          g_audio{DIGITAL_PIN_TRANSDUCER_0, DIGITAL_PIN_TRANSDUCER_1};
CLED            g_led_controller{LED_SCALE_R, LED_SCALE_G, LED_SCALE_B}; // Color calibration
CDisplay        g_display{DISPLAY_COUNT};
CNcoder         g_encoder{DIGITAL_PIN_BUTTON, CNcoder::ButtonMode::NORMAL, CNcoder::RotationMode::NORMAL};

// Container variables
uint8_t         g_leds[CLED::COUNT::LED];

//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

void loop(void)
{
    //Initialize LED Controller
    g_led_controller.Initialize();
    g_led_controller.AssignLEDs(g_leds);
    
    // Initialize Display
    g_display.SetDisplayBrightness(CDisplay::Brightness::MAX);
    DisplayState(State::ENABLE);
    
    for (uint8_t count = 0; count < 15; count++)
    {
        ReadLightIntensity();
        delay(10);
    }
    
    // Create enough delay to allow RTC oscillator to stabilize
    if (ReadLightIntensity() == CDisplay::Brightness::MIN)
    {
        g_display.SetDisplayValue("Photod");
        g_led_controller.SetColor(CRGB::Yellow);
        g_led_controller.Update();
        delay(1500);
    }
    else
    {
        uint8_t count = 1750 / 16;

        while (--count)
        {
            g_display.SetDisplayValue(count);
            delay(16);
        }
    }
    
    // Test High Voltage
    TestVoltage();
    
    // Test I2C Devices
    TestI2CDevices();
    
    g_led_controller.SetColor(CRGB::White);
    g_led_controller.Update();
    
    // Initialize Encoder
    g_encoder.SetCallback(EncoderCallback); // Register callback function

    while (true)
    {
        static uint8_t count = 1;
        static uint8_t segment = 0;
        char string[DISPLAY_COUNT + 1];
        
        if (--count == 0)
        {
            switch(segment++)
            {
            case 0:
                strncpy(string, "~~~~~~", sizeof(string));
                break;
            case 1:
                strncpy(string, "``````", sizeof(string));
                break;
            case 2:
                strncpy(string, "{{{{{{", sizeof(string));
                break;
            case 3:
                strncpy(string, "______", sizeof(string));
                break;
            case 4:
                strncpy(string, "}}}}}}", sizeof(string));
                break;
            case 5:
                strncpy(string, "''''''", sizeof(string));
                break;
            case 6:
                strncpy(string, "------", sizeof(string));
                break;
            case 7:
                strncpy(string, "......", sizeof(string));
                segment = 0;
                break;
            default:
                strncpy(string, "      ", sizeof(string));
                segment = 0;
                break;
            }
            
            g_display.SetDisplayValue(string);
            count = 5;
        }
        
        if (IsInputSelect())
        {
            g_audio.Play(CAudio::Functions::MemStream, music_test_A, music_test_B);
        }
        
        delay(100);
    }
}


Status TestVoltage(void)
{
    uint32_t measurement = 0;
    
    for (uint8_t count = 0; count < 10; count++)
    {
        analogRead(6); // Initialize reading
    }
    
    for (uint8_t count = 0; count < 100; count++)
    {
        measurement += analogRead(6);
    }
    
    float value = ((float)measurement) / 100.0;
    value *= ((100000.0 + 3400.0) / (3400.0 * (1023.0 / 5.0)));
    
    /*if ((value > (VOLTAGE_EXPECTED + VOLTAGE_THRESHOLD)) || \
          (value < (VOLTAGE_EXPECTED - VOLTAGE_THRESHOLD))) */
    if (value < (VOLTAGE_EXPECTED - VOLTAGE_THRESHOLD))
    {
        g_display.SetDisplayValue((uint32_t)(100 * value));
        g_display.SetUnitIndicator(3, true);
        EnterErrorState();
    }
    
    return Status::OK;
}


Status TestI2CDevices(void)
{
    uint8_t buffer[1];
    uint8_t status = 0;
    
#ifdef REVISION_A
#pragma message "Compiling for Revision A"
    CI2C::Handle handle_eeprom{nI2C->RegisterDevice(0x50, 1, CI2C::Speed::FAST)};
#else
    CI2C::Handle handle_eeprom{nI2C->RegisterDevice(0x57, 1, CI2C::Speed::FAST)};
#endif
    CI2C::Handle handle_rtc{nI2C->RegisterDevice(0x51, 1, CI2C::Speed::FAST)};

    // EEPROM - Read address 0x100
    status = nI2C->Read(handle_eeprom, 0x100, buffer, sizeof(buffer));
    
    if (!status)
    {
        // RTC - Read Time
        status = nI2C->Read(handle_rtc, 0x03, buffer, sizeof(buffer));
        
        if (status)
        {
            g_display.SetDisplayValue(" RTC  ");
            EnterErrorState();
        }
    }
    else
    {
        g_display.SetDisplayValue("EEPROM");
        EnterErrorState();
    }

    return Status::OK; 
}


CDisplay::Brightness ReadLightIntensity(void)
{
    const uint8_t SAMPLES = 10;
    static uint16_t history[SAMPLES];
    static uint8_t result = 0;
    static uint32_t previous_average = 0;
    uint32_t average = 0;
    int32_t difference;

    for (uint8_t index = 0; index < (SAMPLES - 1); index++)
    {
        average += (history[index] = history[index + 1]);
    }

    average += (history[SAMPLES - 1] = (analogRead(ANALOG_PIN_PHOTODIODE - A0)));
    average /= SAMPLES;
    average *= 50;
    average /= 10; // pseudo float
    difference = (average - previous_average);
    difference = (difference > 0) ? difference : -difference;

    if (difference > 3)
    {
        uint8_t value;
        
        if (average < 100)
        {
            value = (average / 25);
        }
        else
        {
            value = 2 + (average / 100);
        }

        uint8_t max = getValue(CDisplay::Brightness::MAX);

        if (value > max)
        {
            value = max;
        }
        
        result = value;
        previous_average = average;
    }

    return static_cast<CDisplay::Brightness>(result);
}


void EnterErrorState(void)
{
    g_led_controller.SetColor(CRGB::Red);
    g_led_controller.Update();

    DisplayState(State::ENABLE);
    while(true);
}


void VoltageState(State state)
{
    g_state.voltage = state;
    digitalWrite(DIGITAL_PIN_HV_ENABLE, !getValue(state));
}


void DisplayState(State state)
{
    g_state.display = state;
    digitalWrite(DIGITAL_PIN_BLANK, !getValue(state));
    digitalWrite(DIGITAL_PIN_CATHODE_ENABLE, getValue(state));
    VoltageState(state);
}


__attribute__((optimize("-O3")))
void EncoderCallback(void)
{
    g_audio.Play(CAudio::Functions::MemStream, music_blip, music_blip);
}


bool IsInputIncrement(void)
{
    return (g_encoder.GetRotation() == CNcoder::Rotation::CW);
}


bool IsInputSelect(void)
{
    return (g_encoder.GetButtonState() == CNcoder::Button::DOWN);
}


bool IsInputUpdate(void)
{
    return (g_encoder.IsUpdateAvailable());
}


ISR(TIMER2_COMPA_vect)
{
    uint8_t digit_bitmap = 0;
    static uint8_t pwm_cycle = 0;
    static uint8_t tube = 0;
    static const uint16_t toggle[] = {0xFFF, 0x001, 0x041, 0x111, 0x249, 0x555, 0x5AD, 0x777, 0xFFF};
    
    // No need to update display if disabled
    if (g_state.display == State::DISABLE)
    {
        return;
    }

    setPinHigh(DIGITAL_PIN_BLANK); // blank display
    
    tube++;

    if (tube >= DISPLAY_COUNT)
    {
        // change cycle after all tubes have cycled
        pwm_cycle++;

        if (pwm_cycle > 11)
        {
            pwm_cycle = 0;
        }
        
        tube = 0;
    }

    if ((toggle[getValue(g_display.GetUnitBrightness(tube))] >> pwm_cycle) & 0x1)
    {
        uint8_t unit = g_display.GetUnitValue(tube);
        uint8_t indicator = g_display.GetUnitIndicator(tube);
    
        digit_bitmap = (pgm_read_byte_near(BITMAP + unit - 32) | (indicator << 2));
    }

    setPinLow(DIGITAL_PIN_LATCH); // latch

    // Toggle tube grid
    for (uint8_t index = DISPLAY_COUNT - 1; index < 255; index--)
    {
        setPinLow(DIGITAL_PIN_CLOCK); // clock

        if (digit_bitmap && (index == tube))
        {
            setPinHigh(DIGITAL_PIN_SDATA); // sdata
        }
        else
        {
            setPinLow(DIGITAL_PIN_SDATA); // sdata
        }

        asm volatile("nop");
        setPinHigh(DIGITAL_PIN_CLOCK); // clock
        asm volatile("nop");
    }

    // Toggle tube anodes
    for (uint8_t index = 0; index < 8; index++)
    {
        setPinLow(DIGITAL_PIN_CLOCK); // clock

        if ((digit_bitmap >> (7 - index)) & 0x1)
        {
            setPinHigh(DIGITAL_PIN_SDATA); // sdata
        }
        else
        {
            setPinLow(DIGITAL_PIN_SDATA); // sdata
        }

        asm volatile("nop");
        setPinHigh(DIGITAL_PIN_CLOCK); // clock
        asm volatile("nop");
    }

    asm volatile("nop");
    asm volatile("nop");
    
    setPinHigh(DIGITAL_PIN_LATCH); // latch
    setPinLow(DIGITAL_PIN_BLANK); // unblank display
}


void setup(void)
{
    pinMode(DIGITAL_PIN_CLOCK, OUTPUT);             // Clock
    pinMode(DIGITAL_PIN_SDATA, OUTPUT);             // Serial Data
    pinMode(DIGITAL_PIN_LATCH, OUTPUT);             // Latch Enable
    pinMode(DIGITAL_PIN_BLANK, OUTPUT);             // Blank
    pinMode(DIGITAL_PIN_HV_ENABLE, OUTPUT);         // High Voltage Enable
    pinMode(DIGITAL_PIN_CATHODE_ENABLE, OUTPUT);    // Cathode Enable
    pinMode(ANALOG_PIN_PHOTODIODE, INPUT);          // Photodiode Voltage
    pinMode(DIGITAL_PIN_TRANSDUCER_0, OUTPUT);      // Transducer A
    pinMode(DIGITAL_PIN_TRANSDUCER_1, OUTPUT);      // Transducer B
    
    // Configure Timer2 (Display)
    TCCR2A = 0; // Reset register
    TCCR2B = 0; // Reset register
    TCNT2  = 0; // Initialize counter value to 0
    OCR2A = 255;
    TCCR2A |= _BV(WGM21); // Enable CTC mode
    TCCR2B |= _BV(CS22) | _BV(CS20); // Set CS22 bit for 128 prescaler
    TIMSK2 |= _BV(OCIE2A); // Enable timer compare interrupt
}
