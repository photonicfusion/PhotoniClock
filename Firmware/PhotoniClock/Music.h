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
 * @file        Music.h
 * @summary     Music collection for nAudio library
 * @version     1.2
 * @author      nitacku
 * @author      smbrown
 * @data        18 August 2018
 */

#ifndef _MUSIC_H
#define _MUSIC_H

#include <nEEPROM.h>
#include <nAudio.h>
#include <nDisplay.h>

enum Music : uint16_t
{
    OFFSET = 256,
    ADDRESS_SIZE = 2,
    CHANNEL_COUNT = 2,
};

static const uint8_t music_blip[] = { 1, NC8, DBLIP, END };
static const uint8_t music_menu[] PROGMEM = { 12, NG3, DE, ND4, DS, END };
static const uint8_t music_detonate_begin[] PROGMEM = { 30, NC3, DW, END };
static const uint8_t music_detonate_beep[] PROGMEM = { 30, NFS8, DS, END };
static const uint8_t music_detonate_end_A[] PROGMEM = { 30, NFS8, DW, END };
static const uint8_t music_detonate_end_B[] PROGMEM = { 30, NS7, DW, END };
static const uint8_t music_detonate_end_C[] PROGMEM = { 30, NS5, DW, END };

static const uint8_t music_detonate_fuse_A[] PROGMEM =
{
    50, NS7, DW, NS7, DE, END,
};

static const uint8_t music_detonate_fuse_B[] PROGMEM =
{
    50, NS7, DS, NS6, NS5, NS4, NS3, NS2, NS1, NS0, NRS, DE, END,
};

static const uint8_t music_alarm_beep[] PROGMEM =
{
    15, NC6, DDH, NRS, NC6, NRS, END,
};

static const uint8_t music_alarm_pulse[] PROGMEM =
{
    14, NC6, DE, NRS, NC6, NRS, NC6, NRS, NC6, NRS, DH,
    NC6, DE, NRS, NC6, NRS, NC6, NRS, NC6, NRS, DH, END,
};

static const uint8_t* const music_list[][2] PROGMEM =
{
    {music_alarm_beep, music_alarm_beep},
    {music_alarm_pulse, music_alarm_pulse},
};

const uint8_t INBUILT_SONG_COUNT = (sizeof(music_list) / sizeof(music_list[0]));

struct I2CStreamData
{
    uint8_t* prev; //Kept because of the look-ahead
    uint8_t* data; //Operating on
    uint8_t* next; //Writing into
    uint32_t start = 0;
    uint16_t length = 0;
    uint16_t pos = 0;
    void (*callback)(const uint8_t) = nullptr;
    bool loadNext = true;
    volatile bool valid = false;
};

uint8_t GetMusicEEPROM(const uint8_t index, const uint8_t channel, const I2CStreamData* stream);
void PlayMusic(const uint8_t index);

#endif
