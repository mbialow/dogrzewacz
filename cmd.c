/*
 * cmd.c
 *
 *  Author: mbialowa
 */

#include "cmd.h"

stanType tablicaStanow[ILOSC_STANOW] = {

   { USPIJ,    					     USPIJf },
   { ODCZYT_NAPIECIA, 		         ODCZYT_NAPIECIAf },
   { OBSLUGA_TEMP_CIECZY_CHLODZACEJ, OBSLUGA_TEMP_CIECZY_CHLODZACEJf },
   { WARUNEK_PRACY_PRZEKAZNIK_1,     WARUNEK_PRACY_PRZEKAZNIK_1f },
   { WARUNEK_PRACY_PRZEKAZNIK_2,     WARUNEK_PRACY_PRZEKAZNIK_2f },
   { WARUNEK_KONCA_PRACY, 			 WARUNEK_KONCA_PRACYf },
   { OBSLUGA_PARAMETROW,             OBSLUGA_PARAMETROWf }

 };


int8_t daj_funkcje_stanu(uint8_t kodStanu){

    for(uint8_t i = 0; i < ILOSC_STANOW; ++i){
        if (tablicaStanow[i].kodStanu == kodStanu){
            return i;
        }
    }

    return -1;
}
