/*
 * cmd.h
 *
 *
 *      Author: mbialowa
 */

#ifndef CMD_H_
#define CMD_H_

#include <stdint.h>

#define ILOSC_STANOW 7

typedef enum tStan {
    USPIJ = 0x01,
    ODCZYT_NAPIECIA,
    OBSLUGA_TEMP_CIECZY_CHLODZACEJ,
    WARUNEK_PRACY_PRZEKAZNIK_1,
    WARUNEK_PRACY_PRZEKAZNIK_2,
    WARUNEK_KONCA_PRACY,
    OBSLUGA_PARAMETROW
} Stan;

typedef struct tStanType {
    uint8_t kodStanu;
    void (*wykonajStan)();
} stanType;


void USPIJf();
void ODCZYT_NAPIECIAf();
void OBSLUGA_TEMP_CIECZY_CHLODZACEJf();
void WARUNEK_PRACY_PRZEKAZNIK_1f();
void WARUNEK_PRACY_PRZEKAZNIK_2f();
void WARUNEK_KONCA_PRACYf();
void OBSLUGA_PARAMETROWf();



int8_t daj_funkcje_stanu(uint8_t kodStanu);

extern stanType tablicaStanow[];

#endif /* CMD_H_ */
