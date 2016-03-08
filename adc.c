/*
 * adc.c
 *
 *  Author: mbialowa
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "adc.h"

//#define DEBUG_ADC_INFO

inline void procesujRezultatADC();

ISR(ADC_vect/*, ISR_NOBLOCK*/) {
    procesujRezultatADC();
}


inline void procesujRezultatADC() {

    long result = ADCL;
    result |= ADCH << 8;

#ifdef DEBUG_ADC_INFO
        char buforUARTTmp[100];
#endif

    if (ADMUX & (1 << MUX0)) {
        stanUkladu.temperaturaZewnetrzna = (result * 2500 / 1024 - 500) / 10;
        //dodatkowy offset
        if (stanUkladu.temperaturaZewnetrzna < 5) {
            stanUkladu.temperaturaZewnetrzna -= 1;
        }

#ifdef DEBUG_ADC_INFO
        sprintf_P(buforUARTTmp, PSTR("tz[C]: %ld\n"), result);
#endif

    } else {
        stanUkladu.napiecieAkumulatora = result * 250/1024 + 3;//+3 z uwagi na dzielnik napiecia ktory nie jest idealnie 1:10

#ifdef DEBUG_ADC_INFO
        sprintf_P(buforUARTTmp, PSTR("[V]: %d - %ld\n"), stanUkladu.napiecieAkumulatora, result);
#endif

    }

#ifdef DEBUG_ADC_INFO
        uart_puts(buforUARTTmp);
#endif


    ADCSRA &= ~(1 << ADEN);
    ADMUX ^= 1 << MUX0;
    ADCSRA |= (1 << ADEN);

}


void adc_init() {

    DIDR0 = 0x3F;

    ADMUX |= (1 << MUX0);

            //ADC Enable
    ADCSRA |= (1 << ADEN) |
            //ADC Interrupt Enable
            (1 << ADIE) |
            //ADC Prescaler Selections
            (1 << ADPS1) | (1 << ADPS0);


    // ADC Auto Trigger Source ->Timer/Counter1 Compare Match B
  //  ADCSRB |= (1 << ADTS2) | (1 << ADTS0);

}


