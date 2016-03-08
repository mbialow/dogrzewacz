/*
 * 24LC256.h
 *
 *  Created on: 16 lut 2016
 *      Author: mbialowa
 */

#ifndef _24LC256_H_
#define _24LC256_H_

#include "main.h"


//adres 24lc256 A0 A1 A2 spiete do masy
#define EEPROM_DEVICE_ID 0x50

#define ILOSC_PROBEK_NA_STRONE 9
#define ILOSC_STRON 512
#define ROZMIAR_STRONY 64

typedef struct tEEPROM_DANE {
    Probka   biezacaProbka[ILOSC_PROBEK_NA_STRONE];
    uint8_t  dopelnienie; //dopelnienie do 64 bajtow, 1 strona w 24lc256 ma 64 bajty

} EEPROM_DANE;

//struktura biezacych probek
EEPROM_DANE eeprom_dane;

// prototype local functions
void eeprom_init();
EEPROM_DANE* read_eeprom(uint16_t memory_address);
void write_eeprom(uint16_t memory_address, EEPROM_DANE *w_data);


#endif /* _24LC256_H_ */
