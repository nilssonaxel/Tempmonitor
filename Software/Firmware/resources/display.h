/*--------------------------------------------------------------------------
Copyright (C) 2016  Temp monitor UF

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
--------------------------------------------------------------------------*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <avr/interrupt.h>

#include "shiftout.h"

#define _a (1 << 0)
#define _b (1 << 2)
#define _c (1 << 4)
#define _d (1 << 6)
#define _e (1 << 7)
#define _f (1 << 1)
#define _g (1 << 3)
#define _dp (1 << 5)

const uchar display_map[] = {
    _a | _b | _c | _d | _e | _f,        // 0
    _b | _c,                            // 1
    _a | _b | _g | _e | _d,             // 2
    _a | _b | _g | _c | _d,             // 3
    _f | _g | _b | _c,                  // 4
    _a | _f | _g | _c | _d,             // 5
    _a | _f | _e | _g | _e | _d | _c,   // 6
    _a | _b | _c,                       // 7
    _a | _b | _c | _d | _e | _f | _g,   // 8
    _a | _f | _b | _g | _c | _d,        // 9
};

unsigned char display_raw_buffer[5];
unsigned char display_buffer[] = {0,0,0,0,0,0,0,0,0,0,0,0};
unsigned char display_column = 2;

void DisplayInit()
{
    //Configure interrupts
        sei();  //Global interrupts enable
        TIMSK |= 1 << TOIE1 | 1 << OCIE1A; //Enable Timer1 Overflow and compare match A

    //Configure timer1
        PLLCSR &= ~(1 << PCKE);         //System clock as Timer0 clock source
        TCCR1 |= 1 << CS13 | 1 << CS10; //Div 256
        OCR1A = 100;
}

void DisplayWrite(unsigned char display_number, unsigned int value)
{
        if(value > 99)
            display_buffer[display_number * 3] = display_map[value / 100];
        else
            display_buffer[display_number * 3] = 0;

        value = value % 100;

        if(value > 9)
            display_buffer[(display_number * 3) + 1] = display_map[value / 10];
        else
            display_buffer[(display_number * 3) + 1] = 0;

        display_buffer[(display_number * 3) + 2] = display_map[value % 10];
}

void DisplayWriteRaw(unsigned char display_number, unsigned char* data)
{
    display_buffer[display_number * 3] = data[0];
    display_buffer[(display_number * 3) + 1] = data[1];
    display_buffer[(display_number * 3) + 2] = data[2];
}

void DisplayUpdateRawBuffer()
{
    //Copies values from display buffer to raw buffer
    for(uint8_t row = 0; row != 4; row++)
    {
        display_raw_buffer[row] = display_buffer[(row * 3) + display_column];
    }

    //Connect correct column to ground
    display_raw_buffer[4] = 1 << (display_column + 5);

    //loop display column 2 to 0
    if(display_column != 0)
        display_column--;
    else
        display_column = 2;
}

void DisplayBrightness(unsigned char brightness)
{
    OCR1A = brightness;

    if(brightness > 9)
    {
        //Start Timer0
        TCCR1 |= 1 << CS13 | 1 << CS10;
    }
    else
    {
        //Stop Timer0
        TCCR1 &= ~(1 << CS13) & ~(1 << CS10);

        //Shift out 0 to turn of displays
        ShiftOutByte(0);
        ShiftOutUpdate();
    }

}



//Called when Timer1 overflows, used to multiplex the displays
ISR(TIMER1_OVF_vect)
{
    sei();
    //Shift out raw buffer
    DisplayUpdateRawBuffer();
    ShiftOutBytes(display_raw_buffer, 5);
    ShiftOutUpdate();
}

//Called when a compare match between Timer1 and OCR1A occurs. Turns of the display to change brightness.
ISR(TIMER1_COMPA_vect)
{
    sei();

    //Disconnect all segments from ground
    ShiftOutByte(0);
    ShiftOutUpdate();
}

#endif // _DISPLAY_H_
