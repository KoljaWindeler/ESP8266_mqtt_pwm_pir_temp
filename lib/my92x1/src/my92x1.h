/*

my92x1 LED Driver Arduino library 2.0.0

Copyright (c) 2016 - 2026 MaiKe Labs
Copyright (C) 2017 - Xose Pérez

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _my92x1_h
#define _my92x1_h

#include <Arduino.h>

#ifdef DEBUG_my92x1
    #if ARDUINO_ARCH_ESP8266
        #define DEBUG_MSG_my92x1(...) DEBUG_my92x1.printf( __VA_ARGS__ )
    #elif ARDUINO_ARCH_AVR
        #define DEBUG_MSG_my92x1(...) { char buffer[80]; snprintf(buffer, sizeof(buffer),  __VA_ARGS__ ); DEBUG_my92x1.print(buffer); }
    #endif
#else
    #define DEBUG_MSG_my92x1(...)
#endif

typedef enum my92x1_cmd_one_shot_t {
        my92x1_CMD_ONE_SHOT_DISABLE = 0X00,
        my92x1_CMD_ONE_SHOT_ENFORCE = 0X01,
} my92x1_cmd_one_shot_t;

typedef enum my92x1_cmd_reaction_t {
        my92x1_CMD_REACTION_FAST = 0X00,
        my92x1_CMD_REACTION_SLOW = 0X01,
} my92x1_cmd_reaction_t;

typedef enum my92x1_cmd_bit_width_t {
        my92x1_CMD_BIT_WIDTH_16 = 0X00,
        my92x1_CMD_BIT_WIDTH_14 = 0X01,
        my92x1_CMD_BIT_WIDTH_12 = 0X02,
        my92x1_CMD_BIT_WIDTH_8 = 0X03,
} my92x1_cmd_bit_width_t;

typedef enum my92x1_cmd_frequency_t {
        my92x1_CMD_FREQUENCY_DIVIDE_1 = 0X00,
        my92x1_CMD_FREQUENCY_DIVIDE_4 = 0X01,
        my92x1_CMD_FREQUENCY_DIVIDE_16 = 0X02,
        my92x1_CMD_FREQUENCY_DIVIDE_64 = 0X03,
} my92x1_cmd_frequency_t;

typedef enum my92x1_cmd_scatter_t {
        my92x1_CMD_SCATTER_APDM = 0X00,
        my92x1_CMD_SCATTER_PWM = 0X01,
} my92x1_cmd_scatter_t;

typedef struct {
        my92x1_cmd_scatter_t scatter:1;
        my92x1_cmd_frequency_t frequency:2;
        my92x1_cmd_bit_width_t bit_width:2;
        my92x1_cmd_reaction_t reaction:1;
        my92x1_cmd_one_shot_t one_shot:1;
        unsigned char resv:1;
} __attribute__ ((aligned(1), packed)) my92x1_cmd_t;

typedef struct {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int white;
    unsigned int warm;
} my92x1_color_t;

#define my92x1_COMMAND_DEFAULT { \
    .scatter = my92x1_CMD_SCATTER_APDM, \
    .frequency = my92x1_CMD_FREQUENCY_DIVIDE_1, \
    .bit_width = my92x1_CMD_BIT_WIDTH_8, \
    .reaction = my92x1_CMD_REACTION_FAST, \
    .one_shot = my92x1_CMD_ONE_SHOT_DISABLE, \
    .resv = 0 \
}

class my92x1 {
    public:
				my92x1();
        void setColor(my92x1_color_t color);
        my92x1_color_t getColor();
        void setState(bool state);
        bool getState();
        void init(bool b1True_aiFalse,unsigned char di, unsigned char dcki, unsigned char channels);

    private:

        void _di_pulse(unsigned int times);
        void _dcki_pulse(unsigned int times);
        void _set_cmd();
        void _send();
        void _write(unsigned int data, unsigned char bit_length);

        my92x1_cmd_t _command;
        unsigned char _channels = 4;
        bool _b1True_aiFalse = false;
        bool _state = false;
        my92x1_color_t _color = {0, 0, 0, 0, 0};
        unsigned char _pin_di;
        unsigned char _pin_dcki;


};

#endif
