/*********************************************************************************
Title:    DS18X20-Functions via One-Wire-Bus
Author:   Martin Thomas <eversmith@heizung-thomas.de>
          http://www.siwawi.arubi.uni-kl.de/avr-projects
Software: avr-gcc 4.3.3 / avr-libc 1.6.7 (WinAVR 3/2010)
Hardware: any AVR - tested with ATmega16/ATmega32/ATmega324P and 3 DS18B20

Partly based on code from Peter Dannegger and others.

**********************************************************************************/

// ------------------------------------------------------------------------------ //
// Date: 7.01.11
// Edited by Mike Shatohin (mikeshatohin@gmail.com)
// Project: USBtinyThermometer
// Changes: made it just for two temperature sensor
// ------------------------------------------------------------------------------ //


/*
 * Data 18 mar 2015
 * Marcin Bialowas -  mbialow@gmail.com
 *
 * Usunieto kod do obslugi wiecej niz 1 czujnika
 * Usunieto tryb pasozytniczy
 *
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ds18x20.h"
#include "onewire.h"
#include "crc8.h"

uint8_t scratchPad[DS18X20_SP_SIZE];
uint16_t temperature = 0xf1f2 ;

/*
 * Start measurement for all sensors
 */
uint8_t DS18X20_StartMeasurement(void){

    ow_reset();
    if( ow_input_pin_state() ) { // only send if bus is "idle" = high
        ow_command( DS18X20_CONVERT_T);
        return DS18X20_OK;
    }

    return DS18X20_START_FAIL;
}

/*
 * Returns 1 if conversion is in progress, 0 if finished;
 * not available when parasite powered.
 */
uint8_t DS18X20_IsInProgress(void)
{
    return ow_bit_io( 1 ) ? DS18X20_CONVERSION_DONE : DS18X20_CONVERTING;
}

/*
 * Odczyt temperatury
 */
uint8_t DS18X20_ReadTemperature(void){

    ow_reset();
    ow_command( DS18X20_READ);

    for( uint8_t i = 0; i < DS18X20_SP_SIZE; i++ ) {
        scratchPad[i] = ow_byte_rd();
    }

    if( crc8( &scratchPad[0], DS18X20_SP_SIZE ) ) {
        return DS18X20_ERROR_CRC;
    }

    temperature = ((uint16_t)scratchPad[1] << 8) | scratchPad[0];

    return DS18X20_OK;
}
