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
 * @file        Music.cpp
 * @summary     Music collection for nAudio library
 * @version     1.2
 * @author      nitacku
 * @author      smbrown
 * @data        18 August 2018
 */

#include "Music.h"

extern CAudio g_audio;
extern CEEPROM g_eeprom;

static const uint16_t BUFFER_SIZE = 32;
static const uint16_t PADDED_BUFFER_SIZE = (BUFFER_SIZE * 3) + 1; //32*3 + an end
static uint8_t music_channel_A[PADDED_BUFFER_SIZE] = { NOTE::END };
static uint8_t music_channel_B[PADDED_BUFFER_SIZE] = { NOTE::END };
static I2CStreamData I2CStreamA = {music_channel_A, &music_channel_A[BUFFER_SIZE], &music_channel_A[BUFFER_SIZE * 2]};
static I2CStreamData I2CStreamB = {music_channel_B, &music_channel_B[BUFFER_SIZE], &music_channel_A[BUFFER_SIZE * 2]};

void I2CStreamACallback(const uint8_t err)
{
    if(err == 0)
    {
        I2CStreamA.valid = true;
    }
    else
    {    
        //Abandon all hope ye who enter here
        //(bail on error)
        asm volatile ("jmp 0");
    }
}

void I2CStreamBCallback(const uint8_t err)
{
    if(err == 0)
    {
        I2CStreamB.valid = true;
    }
    else
    {    
        //Abandon all hope ye who enter here
        //(bail on error)
        asm volatile ("jmp 0");
    }
}

static uint8_t I2CStream(uint16_t offset, void* data)
{
    I2CStreamData* stream = ((I2CStreamData*) data);
    if(offset > (stream->length + 1))
    {
        //Done
        return NOTE::END;
    }

    //Rotate the buffers if necessary (at beginning of next buffer)
    if(offset == (stream->pos + BUFFER_SIZE))
    {
        if(!stream->valid)
        {
            //Didn't load in time, abort and reset
            //TODO: maybe should return NOTE::END instead?
            asm volatile ("jmp 0");
        }

        uint8_t* temp = stream->prev;
        stream->prev = stream->data;
        stream->data = stream->next;
        stream->next = temp;
        stream->pos = offset; //Set position to beginning of second buffer
        stream->valid = false;
        stream->loadNext = true; //We need to load the buffer
    }

    //Load next buffer if necessary (beginning of buffer or failed last attempt)
    if(stream->loadNext)
    {
        uint32_t remaining = stream->length - offset;
        if(remaining >= BUFFER_SIZE)
        {
            remaining = BUFFER_SIZE;
        }

        //Attempt to load next buffer
        if(g_eeprom.Read(stream->start + stream->pos + BUFFER_SIZE, stream->next, remaining, stream->callback) == 0)
        {
            stream->loadNext = false;
        }
        else
        {
            //Something went wrong, retry the read next call
            stream->loadNext = true;
        }
    }

    if(offset >= stream->pos)
    {
        return stream->data[offset % BUFFER_SIZE];
    }
    else
    {
        return stream->prev[offset % BUFFER_SIZE];
    }
}

uint8_t* GetMusicDATA(const uint8_t index, const uint8_t channel)
{
    return reinterpret_cast<uint8_t*>(pgm_read_word(&(music_list[index][channel])));
}

uint8_t GetMusicEEPROM(const uint8_t index, const uint8_t channel, I2CStreamData* stream)
{
    uint8_t status = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    uint8_t channel_offset = 0;
    uint8_t offset[Music::ADDRESS_SIZE * 2];

    // Channels may be duplicates. Continue until non-duplicate channel to calculate length
    while((stream->length == 0) || (status != 0))
    {
        start = 0;
        end = 0;
        
        // Calculate offset address
        uint32_t address = ((Music::ADDRESS_SIZE * ((index * Music::CHANNEL_COUNT) + channel + channel_offset)) + Music::ADDRESS_SIZE);
        channel_offset++;
        
        // Read offsets from EEPROM
        status |= g_eeprom.Read(address, offset, sizeof(offset));

        if(status == 0)
        {
            // Calculate start and end offsets
            for(uint8_t offset_index = 0; offset_index < Music::ADDRESS_SIZE; offset_index++)
            {
                start |= (offset[offset_index] << (8 * offset_index));
                end |= (offset[offset_index + Music::ADDRESS_SIZE] << (8 * offset_index));
            }
        }

        //Set up stream data
        stream->start = start;
        stream->length = end - start;
    }

    //Load first buffer
    if(stream->length >= BUFFER_SIZE)
    {
        status |= g_eeprom.Read(stream->start, stream->data, BUFFER_SIZE);
    }
    else
    {
        status |= g_eeprom.Read(stream->start, stream->data, stream->length);
    }

    return status;
}


//TODO: can't use nullptr to disable stream anymore, have to use nullstream
void PlayMusic(const uint8_t index)
{
    using streams = CAudio::Functions;
    // Entries < INBUILT_SONG_COUNT are stored in DATA
    if (index < INBUILT_SONG_COUNT)
    {
        g_audio.Play(streams::PGMStream, GetMusicDATA(index, 0), GetMusicDATA(index, 1));
    }
    else
    {
        //Reset stream data
        I2CStreamA = {music_channel_A, &music_channel_A[BUFFER_SIZE], &music_channel_A[BUFFER_SIZE * 2]};
        I2CStreamB = {music_channel_B, &music_channel_B[BUFFER_SIZE], &music_channel_B[BUFFER_SIZE * 2]};
        I2CStreamA.callback = I2CStreamACallback;
        I2CStreamB.callback = I2CStreamBCallback;

        //Fill out the stream information
        GetMusicEEPROM(index - INBUILT_SONG_COUNT, 0, &I2CStreamA);
        GetMusicEEPROM(index - INBUILT_SONG_COUNT, 1, &I2CStreamB);

        //Play streams
        g_audio.Play(I2CStream, &I2CStreamA, &I2CStreamB);
    }
}
