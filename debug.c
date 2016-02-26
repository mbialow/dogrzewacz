/*
 * debug.c
 *
 *  Author: mbialowa
 */

#include "debug.h"
#include "main.h"
#include <time.h>
#include "uart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef DEBUG_INFO

void debugInfo(const char *info) {

    const time_t time_ = time(&RTC_time);
    char *timeBuff =  ctime(&time_);
    char buforUARTTmp[100];

    sprintf_P(buforUARTTmp, PSTR("n: %d tc: %d tz: %d sp: %d wg: %d ws: %d toff: %ld T: %s %s\n"),
            stanUkladu.napiecieAkumulatora,
            stanUkladu.temperaturaCieczyChlodzacej,
            stanUkladu.temperaturaZewnetrzna,
            stanUkladu.sekundaPracyUkladu,
            stanUkladu.momentWlaczeniaGrzalek,
            stanUkladu.momentWlaczeniaSilnika,
            stanUkladu.czasWylaczeniaDogrzewania,
            timeBuff,
            info
    );

    uart_puts(buforUARTTmp);

}

#else

void debugInfo(const char *info) {}

#endif
