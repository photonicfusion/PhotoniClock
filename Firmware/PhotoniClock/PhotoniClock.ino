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
 * @file        PhotoniClock.ino
 * @summary     Digital Clock for vacuum fluorescent display tubes
 * @version     1.3
 * @author      nitacku
 * @data        19 June 2019
 */

#include "PhotoniClock.h"
#include "Menu.h"
#include "LEDEffect.h"

//---------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------

// Object variables
StateStruct     g_state;
Config          g_config;
CPCF2129        g_rtc;
CAudio          g_audio{DIGITAL_PIN_TRANSDUCER_0, DIGITAL_PIN_TRANSDUCER_1};
CLED            g_led_controller{LED_SCALE_R, LED_SCALE_G, LED_SCALE_B}; // Color calibration
CDisplay        g_display{DISPLAY_COUNT};
CNcoder         g_encoder{DIGITAL_PIN_BUTTON, CNcoder::ButtonMode::NORMAL, CNcoder::RotationMode::NORMAL};
#ifdef REVISION_A
#pragma message "Compiling for Revision A"
CEEPROM         g_eeprom{CEEPROM::Address::A0, 64, 512}; // 32768 (64 * 512) bytes
#else
CEEPROM         g_eeprom{CEEPROM::Address::A7, 64, 512}; // 32768 (64 * 512) bytes
#endif

// Container variables
uint8_t         g_leds[CLED::COUNT::LED];
CRTC::RTC*      g_rtc_struct;

// Integral variables
uint8_t         g_encoder_timeout = 0;
uint8_t         g_song_entries = 0;

//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

void loop(void)
{
    CRTC::RTC rtc; // struct
    uint8_t decimal_counter = 0;
    uint8_t previous_second;
    char s[DISPLAY_COUNT + 1];
        
    g_rtc_struct = &rtc; // Assign global pointer
    
    GetConfig(g_config);

    if (g_config.validate != CONFIG_KEY)
    {
        Config new_config; // Use default constructor values
        SetConfig(new_config); // Write to EEPROM
        GetConfig(g_config); // Read from EEPROM
    }
    
    // Initialize LED Controller
    g_led_controller.Initialize();
    g_led_controller.AssignLEDs(g_leds);
    UpdateLEDBrightness(g_config.brightness);
    
    // Initialize Display
    g_display.SetCallbackIsIncrement(IsInputIncrement);
    g_display.SetCallbackIsSelect(IsInputSelect);
    g_display.SetCallbackIsUpdate(IsInputUpdate);
    g_display.SetDisplayBrightness(g_config.brightness);
    g_display.SetDisplayValue(F("readng"));
    InterruptSpeed(INTERRUPT_FAST);
    delay(1); // Wait for interrupt to occur
    DisplayState(State::ENABLE); // Enable voltage after update
    
    // Initialize RTC
    g_rtc.Initialize();
    UpdateAlarmIndicator();

    // Initialize Encoder
    g_encoder.SetCallback(EncoderCallback); // Register callback function
    
    // Initialize EEPROM
    g_eeprom.Initialize();
    
    // Get number of song entries
    g_eeprom.Read(0, &g_song_entries, 1);
    
    // Check if initial EEPROM read was successful
    if ((g_song_entries == 0) || (g_song_entries == 255))
    {
        g_song_entries = INBUILT_SONG_COUNT; // Limit to internal music
    }
    else
    {
        g_song_entries += INBUILT_SONG_COUNT; // Add internal music to list
    }
    
    // Ensure timer music setting is valid
    if (g_config.music_timer >= g_song_entries)
    {
        g_config.music_timer = 0; SetConfig(g_config);
    }
    
    // Ensure alarm music setting is valid
    for (uint8_t index = 0; index < ALARM_COUNT; index++)
    {
        if (g_config.alarm[index].music >= g_song_entries)
        {
            g_config.alarm[index].music = 0;
            SetConfig(g_config);
        }
    }
    
    while (true)
    {
        AutoBrightness();
        previous_second = rtc.second;
        g_rtc.GetRTC(rtc);
        
        if (rtc.second != previous_second)
        {
            decimal_counter = 0; // Used for indicator strobe
            
            switch (rtc.second)
            {
            case 0:
                AutoBlanking();
                AutoAlarm(); // Do after AutoBlanking - Alarm will enable display

                if (g_config.effect == Effect::SPIRAL)
                {
                    // Display spiral effect
                    g_display.SetDisplayIndicator(false);
                    g_display.EffectScroll(F("~`{_}'~`{_}'~`{_}'"), CDisplay::Direction::LEFT, 50);
                    g_rtc.GetRTC(rtc); // Refresh RTC
                    FormatRTCString(rtc, s, RTCSelect::TIME);
                    g_display.EffectScroll(s, CDisplay::Direction::LEFT, 50);
                    break;
                }
                [[gnu::fallthrough]]; // Fall-through
            case 30:
                if (rtc.second && ((g_config.effect == Effect::DATE) || (g_config.effect == Effect::PHRASE)))
                {
                    if (g_config.effect == Effect::PHRASE)
                    {
                        // Display phrase
                        g_display.SetDisplayValue(g_config.phrase);
                    }
                    else
                    {
                        // Display date
                        FormatRTCString(rtc, s, RTCSelect::DATE);
                        g_display.SetDisplayValue(s);
                    }

                    g_display.SetDisplayIndicator(false);
                    g_display.EffectSlotMachine(20);
                    delay(3050);
                    break;
                }
                [[gnu::fallthrough]]; // Fall-through
            default:
                // Show indicators for AM/PM and alarm
                bool pip = (!rtc.am && (g_config.time_format == FormatTime::H12));
                g_display.SetUnitIndicator(0, getValue(g_state.alarm)); // Alarm
                g_display.SetUnitIndicator(1, true); // 1Hz
                g_display.SetUnitIndicator(3, true); // 1Hz
                g_display.SetUnitIndicator(5, pip); // AM/PM
                
                // Display time & indicators
                FormatRTCString(rtc, s, RTCSelect::TIME);
                g_display.SetDisplayValue(s);
                break;
            }
        }

        if (IsInputUpdate() || IsInputSelect())
        {
            // Check if time threshold elapsed
            if (g_encoder_timeout == 0)
            {
                // Check if display is disabled
                if (g_state.display == State::DISABLE)
                {
                    DisplayState(State::ENABLE);
                }
                else
                {
                    decimal_counter = 0;
                    g_display.SetDisplayIndicator(false);
                    g_state.menu = State::ENABLE;
                    
                    // Check if button was pressed
                    if (IsInputSelect())
                    {
                        FormatRTCString(rtc, s, RTCSelect::DATE);
                        g_display.SetDisplayValue(s);
                        MenuInfo();
                    }
                    else
                    {
                        MenuSettings();
                    }

                    g_state.menu = State::DISABLE;
                    UpdateAlarmIndicator();
                }
            }
            
            g_encoder_timeout = 5;
        }

        // Check if next strobe state
        if (++decimal_counter > 10)
        {
            g_display.SetUnitIndicator(1, false);
            g_display.SetUnitIndicator(3, false);
        }
        
        if (g_encoder_timeout)
        {
            g_encoder_timeout--;
        }
        
        delay(50); // Idle
    }
}


void Timer(const uint8_t hour, const uint8_t minute, const uint8_t second)
{
    uint8_t previous_second;
    uint32_t countdown = (10000 * (uint32_t)hour)
                       + (100 * (uint32_t)minute)
                       + (uint32_t)second;

    CRTC::RTC rtc;
    g_rtc.GetRTC(rtc);
    previous_second = rtc.second;
    
    // Restore user defined brightness setting
    g_display.SetDisplayBrightness(g_config.brightness);
    UpdateLEDBrightness(g_config.brightness);

    while (countdown)
    {
        if (previous_second != rtc.second)
        {
            if ((countdown % 100) == 0)
            {
                if ((countdown % 10000) == 0)
                {
                    countdown -= 4000; // Subtract hours
                }

                countdown -= 40; // Subtract minutes
            }

            countdown--; // Subtract seconds
            previous_second = rtc.second;
            g_display.SetDisplayValue(countdown);
        }

        delay(50);
        AutoBrightness();
        g_rtc.GetRTC(rtc);

        if (IsInputSelect())
        {
            g_encoder_timeout = 5; // Prevent encoder interaction
            return;
        }
    }

    PlayAlarm(g_config.music_timer, "Count!");
}


void Detonate(void)
{
#ifdef DETONATE
    uint32_t countdown = 99999;
    uint16_t value = 2500;
    uint16_t timer = 0;
    uint8_t count = 0;
    uint8_t pwm = 0;

    g_encoder.SetCallback(nullptr); // Disable encoder
    g_display.SetDisplayValue(countdown);
    g_display.SetDisplayBrightness(CDisplay::Brightness::MAX);
    LEDState(State::DISABLE);
    InterruptSpeed(INTERRUPT_SLOW);
    
    delay(500);
    g_audio.Play(CAudio::Functions::PGMStream, music_detonate_begin, music_detonate_begin);
    delay(500);
    
    do
    {
        g_display.SetDisplayValue(countdown);

        if (++timer >= value)
        {
            count++;
            value -= 50;
            value += count / 2;
            timer = 0;
            countdown -= 150;

            g_audio.Play(CAudio::Functions::PGMStream, music_detonate_beep, music_detonate_beep);
            g_led_controller.SetColor((uint32_t)(++pwm) << 16);
            g_led_controller.Update();
        }
    } while (--countdown > 150);
    
    g_led_controller.SetColor(CRGB::Red); // Full intensity
    g_led_controller.Update();
    g_display.SetDisplayValue(F("000000"));
    g_audio.Play(CAudio::Functions::PGMStream, music_detonate_end_A, music_detonate_end_B);
    delay(500);

    for (uint8_t i = 0; i < 3; i++)
    {
        g_audio.Play(CAudio::Functions::PGMStream, music_detonate_fuse_A, music_detonate_fuse_B);
        
        for (uint8_t j = 0; j < 30; j++)
        {
            for (uint8_t k = 0; k < DISPLAY_COUNT / 2; k++)
            {
                uint8_t digit = random8(0, DISPLAY_COUNT);
                g_display.SetUnitValue(digit, random8(128, 255));
            }
            
            delay(50);
            g_display.SetDisplayValue(F("ERROR#")); // Restore
        }
    }

    g_display.SetDisplayValue(F("\xFF\xFF\xFF\xFF\xFF\xFF")); // Connect all anodes
    g_audio.Play(CAudio::Functions::PGMStream, music_detonate_end_B, music_detonate_end_C);
    delay(1000);
    g_display.SetDisplayValue(F("      "));
    delay(3000);
    g_encoder.SetCallback(EncoderCallback); // Enable encoder
    LEDState(State::ENABLE);
    InterruptSpeed(INTERRUPT_FAST);
    g_display.SetDisplayBrightness(g_config.brightness);
#endif
}


void PlayAlarm(const uint8_t song_index, const char* phrase)
{
    uint8_t elapsed_seconds = 0;
    bool toggle_state = false;
    bool audio_active = false;

    DisplayState(State::ENABLE);
    g_state.menu = State::ENABLE;
    g_display.SetDisplayIndicator(false);
    g_display.SetDisplayBrightness(CDisplay::Brightness::MAX);
    UpdateLEDBrightness(CDisplay::Brightness::MAX);
    InterruptSpeed(INTERRUPT_SLOW);
    IsInputUpdate(); // Clear any pending interrupt
    
    // Alarm for at least 120 seconds until music ends or until user interrupt
    do
    {
        if (!audio_active)
        {
            PlayMusic(song_index);
        }
        
        if (g_rtc.GetTimeSeconds() & 0x1)
        {
            if (toggle_state == true)
            {
                toggle_state = false;
                elapsed_seconds++;
                g_display.SetDisplayValue(phrase);
            }
        }
        else
        {
            if (toggle_state == false)
            {
                toggle_state = true;
                elapsed_seconds++;
                g_display.SetDisplayValue(F("      "));
            }
        }

        delay(50);
        audio_active = g_audio.IsActive();
        
    } while (((elapsed_seconds < 120) || audio_active) &&
             !(IsInputUpdate() || IsInputSelect()));

    g_audio.Stop(); // Ensure music is stopped
    
    g_encoder_timeout = 5; // Prevent encoder interaction
    g_state.menu = State::DISABLE;
    InterruptSpeed(INTERRUPT_FAST);
    g_display.SetDisplayBrightness(g_config.brightness);
    UpdateLEDBrightness(g_config.brightness);
}


void AutoBrightness(void)
{
    if (g_config.brightness == CDisplay::Brightness::AUTO)
    {
        CDisplay::Brightness brightness;
        brightness = ReadLightIntensity();

        if (brightness == CDisplay::Brightness::MIN)
        {
            if (g_state.voltage == State::ENABLE)
            {
                VoltageState(State::DISABLE);

                // Compensate for non-linear brightness due to voltage drop
                g_display.SetUnitBrightness(0, CDisplay::Brightness::L7);
                g_display.SetUnitBrightness(1, CDisplay::Brightness::L3);
                g_display.SetUnitBrightness(2, CDisplay::Brightness::L2);
                g_display.SetUnitBrightness(3, CDisplay::Brightness::L7);
                g_display.SetUnitBrightness(4, CDisplay::Brightness::L3);
                g_display.SetUnitBrightness(5, CDisplay::Brightness::L2);
            }
        }
        else
        {
            if (g_state.voltage == State::DISABLE)
            {
                VoltageState(State::ENABLE);
            }
            
            g_display.SetDisplayBrightness(brightness);
        }

        UpdateLEDBrightness(brightness);
    }
}


void AutoBlanking(void)
{
    if (g_config.blank_begin != g_config.blank_end)
    {
        uint32_t seconds = GetSeconds(g_rtc_struct->hour, g_rtc_struct->minute, g_rtc_struct->second);

        if (seconds == g_config.blank_end)
        {
            DisplayState(State::ENABLE);
        }
        else if (seconds == g_config.blank_begin)
        {
            DisplayState(State::DISABLE);
        }
    }
}


void AutoAlarm(void)
{
    uint32_t current_time = GetSeconds(g_rtc_struct->hour, g_rtc_struct->minute, 0);
    
    for (uint8_t index = 0; index < ALARM_COUNT; index++)
    {
        // Check if alarm is enabled
        if (g_config.alarm[index].state == State::ENABLE)
        {
            // Check if alarm day matches current day
            if ((g_config.alarm[index].days >> g_rtc_struct->week_day) & 0x1)
            {
                // Check if alarm time matches current time
                if (g_config.alarm[index].time == current_time)
                {
                    PlayAlarm(g_config.alarm[index].music, g_config.phrase);
                    break; // No need to process remaining alarms
                }
            }
        }
    }
    
    UpdateAlarmIndicator();
}

void UpdateAlarmIndicator(void)
{
    CRTC::RTC rtc;
    uint8_t day;
    State next_state = State::DISABLE; // Assume disabled
    
    g_rtc.GetRTC(rtc);
    uint32_t current_time = GetSeconds(rtc.hour, rtc.minute, 0);
    
    for (uint8_t index = 0; index < ALARM_COUNT; index++)
    {
        // Check if alarm is enabled
        if (g_config.alarm[index].state == State::ENABLE)
        {
            // Determine which day to check
            if (g_config.alarm[index].time > current_time)
            {
                day = rtc.week_day; // Today
            }
            else
            {
                day = (rtc.week_day > 6) ? 1 : (rtc.week_day + 1); // Tomorrow
            }

            // Check if alarm is active for selected day
            if ((g_config.alarm[index].days >> day) & 0x1)
            {
                next_state = State::ENABLE;
                break; // Alarm will be indicated - no need to continue
            }
        }
    }

    // Update alarm indicator
    g_state.alarm = static_cast<decltype(g_state.alarm)>(next_state);
}


void UpdateLEDBrightness(CDisplay::Brightness brightness)
{
    uint16_t r_scale, g_scale, b_scale;
    
    r_scale = (LED_SCALE_R * getValue(brightness)) >> 3;
    g_scale = (LED_SCALE_G * getValue(brightness)) >> 3;
    b_scale = (LED_SCALE_B * getValue(brightness)) >> 3;

    g_led_controller.SetScale(r_scale, g_scale, b_scale);
}


uint8_t FormatHour(const uint8_t hour)
{
    if (g_config.time_format == FormatTime::H24)
    {
        return hour;
    }
    else
    {
        uint8_t conversion = hour % 12;
        conversion += (conversion == 0) ? 12 : 0;
        return conversion;
    }
}


void FormatRTCString(const CRTC::RTC& rtc, char* s, const RTCSelect type)
{
    const char* format_string = PSTR("%02u%02u%02u");

    switch (type)
    {
    case RTCSelect::TIME:
        snprintf_P(s, DISPLAY_COUNT + 1, format_string, FormatHour(rtc.hour), rtc.minute, rtc.second);
        break;

    case RTCSelect::DATE:
        switch (g_config.date_format)
        {
        default:
        case FormatDate::YYMMDD:
            snprintf_P(s, DISPLAY_COUNT + 1, format_string, rtc.year, rtc.month, rtc.day);
            break;

        case FormatDate::MMDDYY:
            snprintf_P(s, DISPLAY_COUNT + 1, format_string, rtc.month, rtc.day, rtc.year);
            break;

        case FormatDate::DDMMYY:
            snprintf_P(s, DISPLAY_COUNT + 1, format_string, rtc.day, rtc.month, rtc.year);
            break;
        }

        break;
    }
}


uint32_t GetSeconds(const uint8_t hour, const uint8_t minute, const uint8_t second)
{
    return ((3600 * (uint32_t)hour) + (60 * (uint32_t)minute) + (uint32_t)second);
}


CDisplay::Brightness ReadLightIntensity(void)
{
    const uint8_t SAMPLES = 32;
    static uint16_t history[SAMPLES];
    static uint8_t result = 0;
    static uint32_t previous_average = 0;
    uint32_t average = 0;
    int32_t difference;

    for (uint8_t index = 0; index < (SAMPLES - 1); index++)
    {
        average += (history[index] = history[index + 1]);
    }

    average += (history[SAMPLES - 1] = (analogRead(ANALOG_PIN_PHOTODIODE - A0) + g_config.offset));
    average /= SAMPLES;
    average *= g_config.gain;
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
    
    // Keep above minimum if night-mode disabled
    if (g_config.night_mode == State::DISABLE)
    {
        if (result == getValue(CDisplay::Brightness::MIN))
        {
            return static_cast<CDisplay::Brightness>(1 + result);
        }
    }

    return static_cast<CDisplay::Brightness>(result);
}


void GetConfig(Config& config)
{
    while (!eeprom_is_ready());
    cli();
    eeprom_read_block((void*)&config, (void*)0, sizeof(Config));
    sei();
}


void SetConfig(const Config& config)
{
    while (!eeprom_is_ready());
    cli();
    eeprom_update_block((const void*)&config, (void*)0, sizeof(Config));
    sei();
}


void VoltageState(State state)
{
    g_state.voltage = state;
    setPinState(DIGITAL_PIN_HV_ENABLE, !getValue(state));
}


void LEDState(State state)
{
    g_state.leds = state;
    g_led_controller.SetColor(CRGB::Black); // Clear existing display
    g_led_controller.Update();
}


void DisplayState(State state)
{
    g_state.display = state;
    setPinState(DIGITAL_PIN_BLANK, !getValue(state));
    setPinState(DIGITAL_PIN_CATHODE_ENABLE, getValue(state));
    VoltageState(state);
    LEDState(state);
}


void InterruptSpeed(const uint8_t speed)
{
    // set compare match register for xHz increments
    OCR2A = speed; // = (16MHz) / (x*prescale) - 1 (must be <255)
}


__attribute__((optimize("-O3")))
void EncoderCallback(void)
{
    using streams = CAudio::Functions;
    if (g_config.noise == State::ENABLE)
    {
        g_audio.Play(streams::MemStream, music_blip, music_blip);
    }
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


// Interrupt is called every millisecond
ISR(TIMER0_COMPA_vect) 
{
    static volatile bool active = false;

    // Prevent interrupt from preempting itself
    if (!active)
    {
        active = true;
        
        // Don't process LEDs if update is not possible
        if (!nI2C->IsCommActive())
        {
            // If I2C is blocked for more than 1s, reset
            wdt_reset(); // Reset watchdog timer
            sei(); // Re-enable interrupts

            if (g_state.leds == State::ENABLE)
            {
                static uint8_t count = 0;
                
                // Update LEDs at 60Hz (~ 1/16ms)
                if (++count == 16)
                {
                    count = 0;
                    LEDProcess(g_config.led_effect);
                    g_led_controller.Update(); // Update LEDs
                }
            }
        }
        
        active = false;
    }
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
    setPinModeOutput(DIGITAL_PIN_CLOCK);            // Clock
    setPinModeOutput(DIGITAL_PIN_SDATA);            // Serial Data
    setPinModeOutput(DIGITAL_PIN_LATCH);            // Latch Enable
    setPinModeOutput(DIGITAL_PIN_BLANK);            // Blank
    setPinModeOutput(DIGITAL_PIN_HV_ENABLE);        // High Voltage Enable
    setPinModeOutput(DIGITAL_PIN_CATHODE_ENABLE);   // Cathode Enable
    setPinModeInput(ANALOG_PIN_PHOTODIODE);         // Photodiode Voltage
    setPinModeOutput(DIGITAL_PIN_TRANSDUCER_0);     // Transducer A
    setPinModeOutput(DIGITAL_PIN_TRANSDUCER_1);     // Transducer B
    
    // Watchdog timer
    wdt_enable(WDTO_1S); // Set for 1 second
    
    // Millisecond timer
    OCR0A = 0x7D;
    TIMSK0 |= _BV(OCIE0A);
    
    // Configure Timer2 (Display)
    TCCR2A = 0; // Reset register
    TCCR2B = 0; // Reset register
    TCNT2  = 0; // Initialize counter value to 0
    //OCR2A set by InterruptSpeed() function - 8 bit register
    TCCR2A |= _BV(WGM21); // Enable CTC mode
    TCCR2B |= _BV(CS22) | _BV(CS20); // Set CS22 bit for 128 prescaler
    TIMSK2 |= _BV(OCIE2A); // Enable timer compare interrupt
}
