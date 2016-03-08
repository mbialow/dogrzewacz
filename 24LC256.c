/*
 * 24LC256.c
 *
 *  Created on: 16 lut 2016
 *      Author: mbialowa
 */


#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "24LC256.h"
#include "uart.h"
#include "twi.h"

// Define eeprom commands according to 24LC256 datasheet
typedef struct {
    uint8_t     high_byte;
    uint8_t     low_byte;
    EEPROM_DANE eeprom_dane;
} WRITE_EEPROM;

typedef struct {
    uint8_t     high_byte;
    uint8_t     low_byte;
} SET_EEPROM_ADDRESS;

typedef struct {
    EEPROM_DANE eeprom_dane;
} READ_EEPROM;

// Create structure pointers for the TWI/I2C buffer
WRITE_EEPROM            *p_write_eeprom;
SET_EEPROM_ADDRESS      *p_set_eeprom_address;
READ_EEPROM             *p_read_eeprom;

// Create TWI/I2C buffer, size to largest command
char    TWI_buffer[sizeof(WRITE_EEPROM)];

void handle_TWI_result(uint8_t return_code);

void eeprom_init() {

    // Specify startup parameters for the TWI/I2C driver
    TWI_init(   F_CPU,                      // clock frequency
                100000L,                    // desired TWI/IC2 bitrate
                TWI_buffer,                 // pointer to comm buffer
                sizeof(TWI_buffer),         // size of comm buffer
                &handle_TWI_result          // pointer to callback function
                );


    // Set our structure pointers to the TWI/I2C buffer
    p_write_eeprom = (WRITE_EEPROM *)TWI_buffer;
    p_set_eeprom_address = (SET_EEPROM_ADDRESS *)TWI_buffer;
    p_read_eeprom = (READ_EEPROM *)TWI_buffer;

}

EEPROM_DANE* read_eeprom(uint16_t memory_address) {
    // send 'set memory address' command to eeprom and then read data
    while(TWI_busy);
    p_set_eeprom_address->high_byte = memory_address >> 8;
    p_set_eeprom_address->low_byte = memory_address;
    TWI_master_start_write_then_read(   EEPROM_DEVICE_ID,               // device address of eeprom chip
                                        sizeof(SET_EEPROM_ADDRESS),     // number of bytes to write
                                        sizeof(EEPROM_DANE)             // number of bytes to read
                                        );

    // nothing else to do - wait for the data
    while(TWI_busy);
    // return the data
    return(&p_read_eeprom->eeprom_dane);
}

// write eeprom - note: page boundaries are not considered in this example
void write_eeprom(uint16_t memory_address, EEPROM_DANE *w_data) {
    while(TWI_busy);
    p_write_eeprom->high_byte = memory_address >> 8;
    p_write_eeprom->low_byte = memory_address;
    p_write_eeprom->eeprom_dane = *w_data;
    TWI_master_start_write(     EEPROM_DEVICE_ID,       // device address of eeprom chip
                                sizeof(WRITE_EEPROM)    // number of bytes to write
                                );

}

// optional callback function for TWI/I2C driver
void handle_TWI_result(uint8_t return_code) {
    if(return_code!=TWI_success){
        char buforUARTTmp[65];
        sprintf_P(buforUARTTmp,PSTR("I2C ERROR - %02X \n"), return_code);
        uart_puts(buforUARTTmp);
    }
}
