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
 * @file        crc8.cpp
 * @summary     8-bit CRC algorithm
 * @version     1.0
 * @author      nitacku
 * @data        20 November 2017
 */

#include "crc8.h"

#ifndef USING_COMPILED_CRC_TABLE
bool is_table_computed = false;

void crc_precompute_table(void)
{
    uint8_t polynomial = 0x1D; // Maximize Hamming distance

    for (uint32_t dividend = 0; dividend < 256; dividend++)
    {
        uint8_t remainder = dividend;

        for (uint32_t bit = 0; bit < 8; ++bit)
        {
            if ((remainder & 0x01) != 0)
            {
                remainder = (remainder >> 1) ^ polynomial;
            }
            else
            {
                remainder >>= 1;
            }
        }

        crc_table[dividend] = remainder;
    }

    is_table_computed = true;
}
#endif


uint8_t crc_compute(uint8_t data[], uint32_t length)
{
#ifndef USING_COMPILED_CRC_TABLE
    if (!is_table_computed)
    {
        crc_precompute_table();
    }
#endif
    
    uint8_t crc = 0;

    for (uint32_t index = 0; index < length; index++)
    {
        uint8_t b = data[index] ^ crc;
#ifdef USING_COMPILED_CRC_TABLE
        uint8_t value = pgm_read_byte(&(crc_table[b & 0xFF]));
#else
        uint8_t value = crc_table[b & 0xFF];
#endif
        crc = (value ^ (crc << 8));
    }

    return crc;
}
