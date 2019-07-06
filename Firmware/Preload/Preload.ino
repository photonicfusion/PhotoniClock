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
 * @file        Preload.cpp
 * @summary     SPI/UART Slave preloader for EEPROM
 * @version     1.2
 * @author      nitacku
 * @data        28 June 2018
 */

#include "Preload.h"

//---------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------

// Object variables
StateStruct         g_state;
CLED                g_led_controller{LED_SCALE_R, LED_SCALE_G, LED_SCALE_B}; // Color calibration
CDisplay            g_display{DISPLAY_COUNT};
#ifdef REVISION_A
#pragma message "Compiling for Revision A"
CEEPROM             g_eeprom{CEEPROM::Address::A0, 64, 512}; // 32768 (64 * 512) bytes
#else
CEEPROM             g_eeprom{CEEPROM::Address::A7, 64, 512}; // 32768 (64 * 512) bytes
#endif

// Container variables
uint8_t             g_leds[CLED::COUNT::LED];

volatile State      state = State::INITIAL;
volatile Status     status = Status::OK;
uint8_t             buffer_A[BUFFER_SIZE + 1];
uint8_t             buffer_B[BUFFER_SIZE + 1];
uint16_t            buffer_A_position = 0;
uint16_t            buffer_B_position = 0;
uint16_t            eeprom_address = 0;
volatile bool       buffer_A_busy = false;
volatile bool       buffer_B_busy = false;
volatile uint16_t   number_of_sections = 0;
volatile uint16_t   completed_sections = 0;
volatile uint8_t    retry_count = 0;
volatile bool       polled = false;

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
    delay(100); // Prevent lockup on SPI
    DisplayState(S::DISABLE);
    
    // Test High Voltage
    TestVoltage();
    
    // Initialize EEPROM
    g_eeprom.Initialize();

    g_led_controller.SetColor(CRGB::Purple);
    g_led_controller.Update();

    while (true)
    {
        static uint8_t previous_completed_sections = 0;
        
        // Write Buffer A
        if (buffer_A_busy)
        {
            if (VerifyCRC(buffer_A, buffer_A_position) == Status::OK)
            {
                // Skip CRC
                if (WriteData(buffer_A, buffer_A_position - 1) == Status::OK)
                {
                    if (VerifyData(eeprom_address, buffer_A, buffer_A_position - 1) == Status::OK)
                    {
                        // Advance memory address
                        eeprom_address += buffer_A_position - 1; // Skip CRC
                        completed_sections++;
                        retry_count = 0;
                        buffer_A_busy = false;
                        status = Status::OK;
                    }
                }
            }
        }
        
        // Write Buffer B
        if (buffer_B_busy)
        {
            if (VerifyCRC(buffer_B, buffer_B_position) == Status::OK)
            {
                // Skip CRC
                if (WriteData(buffer_B, buffer_B_position - 1) == Status::OK)
                {
                    if (VerifyData(eeprom_address, buffer_B, buffer_B_position - 1) == Status::OK)
                    {
                        // Advance memory address
                        eeprom_address += buffer_B_position - 1; // Skip CRC
                        completed_sections++;
                        retry_count = 0;
                        buffer_B_busy = false;
                        status = Status::OK;
                    }
                }
            }
        }
        
        // Progress bar
        if (previous_completed_sections != completed_sections)
        {
            uint8_t unit = (((uint32_t)((uint32_t)256 * DISPLAY_COUNT * completed_sections) / number_of_sections) / 256);
            g_led_controller.SetColor(CRGB::Green, unit);
            g_led_controller.Update();
            previous_completed_sections = completed_sections;
        }
        
        // Completion
        if ((completed_sections == number_of_sections) && (number_of_sections > 0))
        {
            state = State::COMPLETE;
            status = Status::COMPLETE;
            delay(250); // Listen for additional data
            DisplayState(S::ENABLE);
            g_led_controller.SetColor(CRGB::Green);
            g_led_controller.Update();
            g_display.SetDisplayValue(completed_sections);
            g_display.SetUnitValue(0, 'S');
            g_display.SetUnitValue(1, 'E');
            g_display.SetUnitValue(2, 'C');
            while(true);
        }
    }
}


Status WriteData(uint8_t data[], uint16_t bytes)
{
    uint8_t const* c_data = data;
    uint8_t _status = 0;
    uint8_t count = 255;
    
    do
    {
        delay(1); // Delay to allow up to 255ms
        _status = g_eeprom.Write(eeprom_address, c_data, bytes);
    } while ((_status == CI2C::STATUS_BUSY) && count--); // Check if busy
    
    if (_status)
    {
        if (_status == 0xFF)
        {
            g_display.SetDisplayValue("--FULL");
            return EnterErrorState(true);
        }
        else
        {
            g_display.SetDisplayValue("SEND  ");
            g_display.SetUnitValue(5, '0' + _status);
        }
        
        return EnterErrorState(true);
    }
    
    return Status::OK;
}


Status VerifyCRC(uint8_t data[], uint16_t bytes)
{
    if (bytes > 1)
    {
        uint8_t crc = crc_compute(data, bytes - 1);

        if (crc != data[bytes - 1])
        {
            g_display.SetDisplayValue(completed_sections);
            g_display.SetUnitValue(0, 'C');
            g_display.SetUnitValue(1, 'R');
            g_display.SetUnitValue(2, 'C');
            return EnterErrorState(true);
        }
    }
    else
    {
        g_display.SetDisplayValue("--NULL");
        return EnterErrorState(true);
    }
    
    return Status::OK;
}


Status VerifyData(const uint16_t address, const uint8_t data[], uint16_t bytes)
{
    uint8_t read[BUFFER_SIZE];
    uint8_t _status = 0;
    uint8_t count = 255;
    do
    {
        delay(1); // Delay to allow up to 255ms
        _status = g_eeprom.Read(address, read, bytes);
    } while ((_status == CI2C::STATUS_BUSY) && count--); // Check if busy
    
    if (_status)
    {
        g_display.SetDisplayValue("READ  ");
        g_display.SetUnitValue(5, '0' + _status);

        return EnterErrorState(true);
    }
    
    for (uint16_t index = 0; index < bytes; index++)
    {
        if (read[index] != data[index])
        {
            g_display.SetDisplayValue("Audit ");
            return EnterErrorState(true);
        }
    }
    
    return Status::OK;
}


Status TestVoltage(void)
{
    VoltageState(S::ENABLE);
    delay(250); // Wait for voltage to stabilize
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
    VoltageState(S::DISABLE);
    
    /*if ((value > (VOLTAGE_EXPECTED + VOLTAGE_THRESHOLD)) || \
          (value < (VOLTAGE_EXPECTED - VOLTAGE_THRESHOLD))) */
    if (value < (VOLTAGE_EXPECTED - VOLTAGE_THRESHOLD))
    {
        g_display.SetDisplayValue((uint32_t)(100 * value));
        g_display.SetUnitIndicator(3, true);
        EnterErrorState(false);
    }
    
    return Status::OK;
}


Status EnterErrorState(bool allow_retry)
{
    static bool waiting_for_poll = false;
    
    if (waiting_for_poll == true)
    {
        // This function is already on the call stack
        return status;
    }
    
    waiting_for_poll = true;
    polled = false;
    
    if (allow_retry)
    {
        if (retry_count++ < RETRY_LIMIT)
        {
            buffer_A_position = 0;
            buffer_B_position = 0;
            buffer_A_busy = false;
            buffer_B_busy = false;
            status = Status::RETRY;
            // Wait for Master to poll
            while (polled == false);
            // Wait for Master to poll again
            polled = false;
            while (polled == false);
            waiting_for_poll = false;
            status = Status::OK; // Clear the error
            // Reset buffer positions and status
            return Status::RETRY;
        }
    }        
    
    state = State::ERROR;
    status = Status::ERROR;
 
    uint32_t count = 0;
    while ((polled == false && (count++ < 25000000)));
    waiting_for_poll = false;
    
    g_led_controller.SetColor(CRGB::Red);
    g_led_controller.Update();
    
    DisplayState(S::ENABLE);
    while(true);
    return Status::ERROR; // Avoid compiler error
}


uint8_t process(uint8_t byte)
{
    static bool escape_active = false;

    if (escape_active == false)
    {
        switch (byte)
        {
        case ESCAPE_INDICATOR:
            escape_active = true;
            return 0;
            break;
        case POLL_STATUS_INDICATOR:
            polled = true;
            return status + 1;
            break;
        case POLL_SECTION_INDICATOR:
            return completed_sections + /*(buffer_A_busy || buffer_B_busy)*/ + 1;
            break;
        default:
            break;
        }
    }
    
    switch (state)
    {
    case State::INITIAL:
        if (polled == true)
        {
            static uint8_t byte_count = 0;
            number_of_sections |= (((uint16_t)byte) << (8 * byte_count++));
            
            if (byte_count < 2)
            {
                return 0;
            }
        }
        
        // Check if value even or out of range
        if ((number_of_sections < 2))// ||
            //!(number_of_sections & 0x01) ||
            //(number_of_sections > 0xFE))
        {
            number_of_sections = -1; // Reset value
            sei();
            g_display.SetDisplayValue(number_of_sections);
            g_display.SetUnitValue(0, 'L');
            g_display.SetUnitValue(1, 'D');
            g_display.SetUnitValue(2, ' ');
            EnterErrorState(true);
        }
        else
        {
            state = State::LOAD_A;
        }
        break;
    case State::LOAD_A:
        if (status == Status::OK)
        {
            if ((byte == SECTION_INDICATOR) && (escape_active == false))
            {
                buffer_B_position = 0;
                buffer_A_busy = true;
                state = State::LOAD_B;
                // Assume previous section will succeed
                // Check if Buffer B has been processed
                if (buffer_B_busy)
                {
                    status = Status::BUSY;
                }
            }
            else
            {
                if (buffer_A_position < (sizeof(buffer_A) - 1))
                {
                    buffer_A[buffer_A_position++] = byte;
                    escape_active = false;
                }
                else
                {
                    sei();
                    g_display.SetDisplayValue("BUF OF");
                    EnterErrorState(true);
                }
            }
        }
        break;
    case State::LOAD_B:
        if (status == Status::OK)
        {
            if ((byte == SECTION_INDICATOR) && (escape_active == false))
            {
                buffer_A_position = 0;
                buffer_B_busy = true;
                state = State::LOAD_A;
                // Assume previous section will succeed
                // Check if Buffer A has been processed
                if (buffer_A_busy)
                {
                    status = Status::BUSY;
                }
            }
            else
            {
                if (buffer_B_position < (sizeof(buffer_B) - 1))
                {
                    buffer_B[buffer_B_position++] = byte;
                    escape_active = false;
                }
                else
                {
                    sei();
                    g_display.SetDisplayValue("BUF OF");
                    EnterErrorState(true);
                }
            }
        }
        break;        
    case State::COMPLETE:
        sei();
        g_display.SetDisplayValue("Halted");
        EnterErrorState(false);
        break;
    case State::ERROR:
    default:
        sei();
        //g_display.SetDisplayValue("ERROR ");
        EnterErrorState(false);
        break;
    }
    
    return 0;
}


// SPI interrupt routine
ISR(SPI_STC_vect)
{
    uint8_t byte = process(SPDR);
    
    if (byte)
    {
        SPDR = byte - 1;
    }
}


//ISR(USART_RX_vect)
void UART(void)
{
    while (Serial.available())
    {
        uint8_t input = Serial.read();
        uint8_t output = process(input);
        
        if (output)
        {
            Serial.write(output - 1);
        }
    }
}


void VoltageState(S s)
{
    g_state.voltage = s;
    digitalWrite(DIGITAL_PIN_HV_ENABLE, !getValue(s));
}


void SPIState(const S s)
{
    g_state.SPI = s;
    
    if (s == S::ENABLE)
    {
        pinMode(SS, INPUT);
        digitalWrite(SS, LOW);
        pinMode(MISO, OUTPUT);
        pinMode(MOSI, INPUT);
        pinMode(SCK, INPUT);
        SPCR |= bit(SPE); // Enable SPI slave mode
        SPI.attachInterrupt();
    }
    else
    {
        // Disable SPI slave mode
        SPCR &= ~bit(SPE);
        SPI.detachInterrupt();
        pinMode(DIGITAL_PIN_CLOCK, OUTPUT);
        pinMode(DIGITAL_PIN_CATHODE_ENABLE, OUTPUT);      
    }
}


void DisplayState(S s)
{
    g_state.display = s;
    digitalWrite(DIGITAL_PIN_BLANK, !getValue(s));
    digitalWrite(DIGITAL_PIN_CATHODE_ENABLE, getValue(s));
    VoltageState(s);
    SPIState((s == S::ENABLE) ? S::DISABLE : S::ENABLE);
}


ISR(TIMER2_COMPA_vect)
{
    uint8_t digit_bitmap = 0;
    static uint8_t pwm_cycle = 0;
    static uint8_t tube = 0;
    static const uint16_t toggle[] = {0xFFF, 0x001, 0x041, 0x111, 0x249, 0x555, 0x5AD, 0x777, 0xFFF};
    
    // No need to update display if disabled
    if (g_state.display == S::DISABLE)
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
    
    Serial.begin(115200);
    Serial.onReceive(UART);
}
