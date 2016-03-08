/*
 * menu.c
 *
 *  Author: Marcin Bialowas -  mbialow@gmail.com
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/power.h>


#include "ds18x20/onewire.h"
#include "ds18x20/ds18x20.h"
#include "cmd.h"
#include "main.h"
#include "uart.h"
#include "adc.h"
#include "24LC256.h"
#include "debug.h"


void zapis_parametrow();
void obsluga_uart();
void wyswietl_probki();

inline void wylacz_przekazniki();
inline void zalacz_przekaznik_1();
inline void zalacz_przekaznik_2();
inline void io_init();
inline void timer_init();


//zapisna pelna strona w 24lc256
uint16_t EEMEM indeksZapisanychStron = 0;
volatile uint16_t indeksBiezacejStrony;


ParametryPracy EEMEM parametryPracyEEPROM = {

		.CZAS_PRACY_GRZALEK_SEKUNDY = 60 * 3, //(60 s * 5 minut)
		.CZAS_PRACY_SILNIKA_SEKUNDY = 10, //po 10 sekundach od wzrostu napiecia mozemy weryfikowac warunki wlaczenia dogrzewania

		.CZAS_ZWLOKI_POMIEDZY_KOLEJNYM_WLACZENIEM_DOGRZEWANIA_SEKUNDY = 60 * 60,
		.CZAS_PRACY_DO_USPIENIA_SEKUNDY = 2,

		.INTERWAL_ZAPISU_PROBEK_SPOCZYNEK = 60 * 60 * 3, //co 3h
		.INTERWAL_ZAPISU_PROBEK_PRACA = 60, //co minute

		.NAPIECIE_PRACY = 120, //133 = 13,3v powyzej uklad weryfikuje pozostale warunki

		.TEMPERATURA_WYLACZENIE_DOGRZEWANIA = 65, //temperatura cieczy chlodzacej po osiagnieciu ktorej wylaczy sie dogrzewanie
		.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_1 = 5, //ponizej 5 stopni mozemy wlaczyc dogrzewanie
		.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_2 = -7 // ponizej -7 wlaczmy przekazniki 2

		};



volatile ParametryPracy parametryPracy;

volatile uint8_t indeksBiezacejProbki;
volatile time_t  czasZebraniaOstatniejProbki;

volatile uint8_t flagaInicjalizacji 	__attribute__((section(".noinit")));
volatile StanUkladu stanUkladu  __attribute__((section(".noinit")));
time_t RTC_time  		 	    __attribute__((section(".noinit")));



/**
 * ISR  - START
 */

ISR(TIMER1_COMPB_vect) {
    ++stanUkladu.sekundaPracyUkladu;
    ADCSRA |= (1 << ADSC);
}

ISR(TIMER2_OVF_vect, ISR_NAKED) {
    system_tick();
    reti();
}

/**
 * ISR  - END
 */


inline void timer_init() {

	//TIMER 1
    OCR1A = 15625; //1s
    TCCR1B |= (1 << WGM12);
    // Mode 4, CTC on OCR1B
    TIMSK1 |= (1 << OCIE1B);
    //TIMSK1 |= (1 << OCIE1A);

    //prescaler 64
    TCCR1B |= (1 << CS11) | (1 << CS10);

    //TIMER 2
    TCNT2 = 0;
    TCCR2B |= (1<<CS22) /*|(1<<CS21)*/ |(1<<CS20); //1s

    //TCCR2B |= (1<<CS21) |(1<<CS20);//250ms
    TIMSK2 |= (1<<TOIE2);
    ASSR |= (1<<AS2);

}

inline void io_init() {
    PRZEKAZNIK_DDR |= (1 << PRZEKAZNIK_1_PIN) | (1 << PRZEKAZNIK_2_PIN);
    PRZEKAZNIK_PORT = 0x00;
}


inline void zalacz_przekaznik_1() {
    //stan wysoki na pin
    PRZEKAZNIK_PORT |= (1 << PRZEKAZNIK_1_PIN);
}

inline void zalacz_przekaznik_2() {
    //stan wysoki na pin
    PRZEKAZNIK_PORT |= (1 << PRZEKAZNIK_2_PIN);
}

inline void wylacz_przekazniki() {
    PRZEKAZNIK_PORT = 0x00;
}

void zapis_parametrow() {

    const time_t time_ = time(&RTC_time);
    const uint16_t diff = difftime(time_, czasZebraniaOstatniejProbki);

    uint16_t interwal = parametryPracy.INTERWAL_ZAPISU_PROBEK_SPOCZYNEK;

    if (stanUkladu.momentWlaczeniaGrzalek > 0) {
        interwal = parametryPracy.INTERWAL_ZAPISU_PROBEK_PRACA;
    }

    if (diff >= interwal && indeksBiezacejStrony < ILOSC_STRON) {

        czasZebraniaOstatniejProbki = time_;

       // memcpy((void *)&stanUkladu, &eeprom_dane.biezacaProbka[indeksBiezacejProbki], 3);
        eeprom_dane.biezacaProbka[indeksBiezacejProbki].czasPomiaru = time_;
        eeprom_dane.biezacaProbka[indeksBiezacejProbki].napiecieAkumulatora = stanUkladu.napiecieAkumulatora;
        eeprom_dane.biezacaProbka[indeksBiezacejProbki].temperaturaZewnetrzna = stanUkladu.temperaturaZewnetrzna;
        eeprom_dane.biezacaProbka[indeksBiezacejProbki].temperaturaCieczyChlodzacej = stanUkladu.temperaturaCieczyChlodzacej;

        indeksBiezacejProbki++;

#ifdef DEBUG_INFO

        debugInfo(("co_interwal"));

#endif

    }

    if (indeksBiezacejProbki > ILOSC_PROBEK_NA_STRONE - 1) {
        write_eeprom(indeksBiezacejStrony * ROZMIAR_STRONY, &eeprom_dane);
        indeksBiezacejStrony++;
        indeksBiezacejProbki = 0;

#ifdef DEBUG_INFO1
        uart_puts_P("Zapis do EEPROM\n");
#endif

        eeprom_update_word(&indeksZapisanychStron, indeksBiezacejStrony);

        //przekroczony zakres stron zapisujemy od 0
        if (indeksBiezacejStrony >= ILOSC_STRON) {
            indeksBiezacejStrony = 0;
        }

    }


}

void wyswietl_probki() {

    EEPROM_DANE *temp;
    char buforUARTTmp[100];


    sprintf_P(buforUARTTmp, PSTR("ilosc probek=%d\n"), indeksBiezacejStrony * ILOSC_PROBEK_NA_STRONE);
    uart_puts(buforUARTTmp);

    for (uint16_t strona = 0; strona < indeksBiezacejStrony; strona++) {

        temp = read_eeprom(strona * ROZMIAR_STRONY);

        for (uint8_t i = 0; i < ILOSC_PROBEK_NA_STRONE; i++) {

            char *czasBufor = ctime(&temp->biezacaProbka[i].czasPomiaru);
            sprintf_P(buforUARTTmp, PSTR("tc[C]: %d tz[C]: %d n[V]: %d T: %s\n"),
                temp->biezacaProbka[i].temperaturaCieczyChlodzacej,
                temp->biezacaProbka[i].temperaturaZewnetrzna,
                temp->biezacaProbka[i].napiecieAkumulatora,
                czasBufor
               );

            uart_puts(buforUARTTmp);
            wdt_reset();
        }
    }
    //poczekaj az uart zakonczy wysylke
    _delay_ms(100);
}


//TODO: funkcja do przepisania
void obsluga_uart() {

    static char command[26];
    static uint8_t idx;
    static uint8_t odbr;

    unsigned int uartDate = uart_getc();

    if (uartDate != UART_NO_DATA && uartDate >> 8 == 0) {
        if(uartDate == 0x0A){
            odbr = 1;
            command[idx++] = 0;
        }else{
        	command[idx++] = uartDate;
        }
    }

	if (odbr == 1) {
		idx = 0;
		odbr = 0;

		if (strncmp(command, "dump", 5) == 0) {
			wyswietl_probki();

		} else if (strncmp(command, "uczas", 5) == 0) {

			uint32_t _time = atol(command + 6);
			set_system_time(_time - UNIX_OFFSET);

		} else if (strncmp(command, "czas", 4) == 0) {

			char buforUARTTmp[50];
			sprintf_P(buforUARTTmp, PSTR("czas: %ld \n"), time(&RTC_time));
			uart_puts(buforUARTTmp);

		} else if (strncmp(command, "stan", 4) == 0) {
			debugInfo("stan");

		} else if (strncmp(command, "param", 5) == 0) {
			char buforUARTTmp[450];

			sprintf_P(buforUARTTmp,
					   PSTR("--- Parametry pracy ---\n"
							"czas pracy grzalek [s]: %d\n"
							"czas pracy silnika [s]: %d\n"
							"interwal zapisu probek spoczynek [s]: %d\n"
							"interwal zapisu probek praca [s]: %d\n"
							"czas zwloki pomiedzy kolejnym wlaczeniem dogrzewania [s]: %d\n"
							"czas pracy do uspienia [s]: %d\n"
							"napiecie pracy np. 123 = 12,3 [V]: %d\n"
							"temperatura wylaczenie dogrzewania [C]: %d\n"
							"temeratura zew. wla. przek 1 [C]: %d\n"
							"temeratura zew. wla. przek 2 [C]: %d\n"
						),
						parametryPracy.CZAS_PRACY_GRZALEK_SEKUNDY,
						parametryPracy.CZAS_PRACY_SILNIKA_SEKUNDY,
						parametryPracy.INTERWAL_ZAPISU_PROBEK_SPOCZYNEK,
						parametryPracy.INTERWAL_ZAPISU_PROBEK_PRACA,
						parametryPracy.CZAS_ZWLOKI_POMIEDZY_KOLEJNYM_WLACZENIEM_DOGRZEWANIA_SEKUNDY,
						parametryPracy.CZAS_PRACY_DO_USPIENIA_SEKUNDY,
						parametryPracy.NAPIECIE_PRACY,
						parametryPracy.TEMPERATURA_WYLACZENIE_DOGRZEWANIA,
						parametryPracy.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_1,
						parametryPracy.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_2

			);
			uart_puts(buforUARTTmp);
		}

		uart_puts_P("\n");

	}


}

void OBSLUGA_PARAMETROWf() {

    stanUkladu.biezacyStan = ODCZYT_NAPIECIA;

    zapis_parametrow();
    obsluga_uart();

}

void USPIJf() {

    if (stanUkladu.sekundaPracyUkladu > parametryPracy.CZAS_PRACY_DO_USPIENIA_SEKUNDY) {

        sleep_enable();
        stanUkladu.sekundaPracyUkladu = 0;

        ADCSRA |= (1 << ADSC);

        time(&RTC_time);

        sleep_cpu();
        sleep_disable();
    }

    stanUkladu.biezacyStan = OBSLUGA_PARAMETROW;
}

void ODCZYT_NAPIECIAf() {

    stanUkladu.biezacyStan = USPIJ;

    if (stanUkladu.napiecieAkumulatora > parametryPracy.NAPIECIE_PRACY && stanUkladu.momentWlaczeniaSilnika == 0) {
        stanUkladu.momentWlaczeniaSilnika = stanUkladu.sekundaPracyUkladu;
    }else if (stanUkladu.napiecieAkumulatora > parametryPracy.NAPIECIE_PRACY && stanUkladu.momentWlaczeniaSilnika > 0) {
        stanUkladu.biezacyStan = OBSLUGA_TEMP_CIECZY_CHLODZACEJ;
    }else if (stanUkladu.napiecieAkumulatora < parametryPracy.NAPIECIE_PRACY && stanUkladu.momentWlaczeniaGrzalek > 0) {
        stanUkladu.biezacyStan = WARUNEK_KONCA_PRACY;
    }

}

void OBSLUGA_TEMP_CIECZY_CHLODZACEJf() {

     stanUkladu.biezacyStan = WARUNEK_PRACY_PRZEKAZNIK_1;

    //odczyt temperatury po konwersji
    if (!DS18X20_IsInProgress() && DS18X20_OK == DS18X20_ReadTemperature()) {
        //interesuje mnie tylko czesc calkowita
        stanUkladu.temperaturaCieczyChlodzacej = (temperature >> 4);

#ifdef DEBUG_INFO1

       time_t time_ = time(&RTC_time);
       uint8_t sec = gmtime(&time_)->tm_sec;

       if( (sec > 30 && sec < 32) ) {
           debugInfo("");
       }
#endif

       DS18X20_StartMeasurement();
    }

}

void WARUNEK_PRACY_PRZEKAZNIK_1f() {

    stanUkladu.biezacyStan = WARUNEK_KONCA_PRACY;

    //weryfikacja czy dogrzewanie jest wylaczone
    if (stanUkladu.momentWlaczeniaGrzalek == 0
            && (stanUkladu.sekundaPracyUkladu - stanUkladu.momentWlaczeniaSilnika) > parametryPracy.CZAS_PRACY_SILNIKA_SEKUNDY) {

        uint16_t uplywCzasuOdPoprzedniegoWylaczenia = 0;

        if(stanUkladu.czasWylaczeniaDogrzewania != 0) {
            uplywCzasuOdPoprzedniegoWylaczenia = difftime(time(&RTC_time), stanUkladu.czasWylaczeniaDogrzewania);
        }

        //weryfikacja czy mozna ponownie wlaczyc dogrzewanie
        if (stanUkladu.czasWylaczeniaDogrzewania == 0 || (stanUkladu.czasWylaczeniaDogrzewania != 0
                  && uplywCzasuOdPoprzedniegoWylaczenia > parametryPracy.CZAS_ZWLOKI_POMIEDZY_KOLEJNYM_WLACZENIEM_DOGRZEWANIA_SEKUNDY)) {

        	//warunek temperatury
            if (stanUkladu.temperaturaZewnetrzna <= parametryPracy.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_1
                    && stanUkladu.temperaturaCieczyChlodzacej < parametryPracy.TEMPERATURA_WYLACZENIE_DOGRZEWANIA) {

                stanUkladu.czasWylaczeniaDogrzewania = 0;
                stanUkladu.momentWlaczeniaGrzalek = stanUkladu.sekundaPracyUkladu;

#ifdef DEBUG_INFO
                debugInfo(("zalacz_p1"));
#endif
                zalacz_przekaznik_1();
                stanUkladu.biezacyStan = WARUNEK_PRACY_PRZEKAZNIK_2;

            }
        }
    }

}

void WARUNEK_PRACY_PRZEKAZNIK_2f() {

    if (stanUkladu.temperaturaZewnetrzna <= parametryPracy.TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_2) {

#ifdef DEBUG_INFO
        debugInfo(("zalacz_p2"));
#endif
        zalacz_przekaznik_2();

    }

    stanUkladu.biezacyStan = WARUNEK_KONCA_PRACY;

}

void WARUNEK_KONCA_PRACYf() {

    stanUkladu.biezacyStan = OBSLUGA_PARAMETROW;

    //weryfikacja czy dogrzewanie wlaczone
    if (stanUkladu.momentWlaczeniaGrzalek > 0) {

        if (stanUkladu.napiecieAkumulatora < parametryPracy.NAPIECIE_PRACY
            || stanUkladu.temperaturaCieczyChlodzacej >= parametryPracy.TEMPERATURA_WYLACZENIE_DOGRZEWANIA
            || (stanUkladu.sekundaPracyUkladu - stanUkladu.momentWlaczeniaGrzalek) >= parametryPracy.CZAS_PRACY_GRZALEK_SEKUNDY) {

				stanUkladu.momentWlaczeniaGrzalek = 0;
				stanUkladu.momentWlaczeniaSilnika = 0;
				stanUkladu.sekundaPracyUkladu = 0;
				stanUkladu.czasWylaczeniaDogrzewania = time(&RTC_time);
				//nie bedzie zasmiecalo danych
				stanUkladu.temperaturaCieczyChlodzacej = 0;

#ifdef DEBUG_INFO
				debugInfo(("koniec_pracy"));
#endif

				wylacz_przekazniki();
				stanUkladu.biezacyStan = USPIJ;

        }
    }


}

inline void param_init() {

    indeksBiezacejStrony = eeprom_read_word(&indeksZapisanychStron);
    eeprom_read_block((void *)&parametryPracy, &parametryPracyEEPROM, sizeof(ParametryPracy));

    //ustaw domyslne parametry
    memset((void *)&stanUkladu, 0x00, sizeof(StanUkladu));
    stanUkladu.biezacyStan = OBSLUGA_PARAMETROW;

    if(flagaInicjalizacji != 119) {
        struct tm rtc_time;

        rtc_time.tm_min = 35;
        rtc_time.tm_hour = 21;

        rtc_time.tm_year = 116;
        rtc_time.tm_mday = 5;
        rtc_time.tm_mon = 1;

        rtc_time.tm_isdst = 0;

        RTC_time = mktime(&rtc_time) ;

    }

    set_system_time(RTC_time );
    czasZebraniaOstatniejProbki = RTC_time;
    set_zone(1 * ONE_HOUR);

    flagaInicjalizacji = 119;

}



int main(void) {

    io_init();
    param_init();
    adc_init();
    timer_init();
    eeprom_init();

    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));

#ifdef DEBUG_INFO

    char buforUARTTmp[25];
    sprintf_P(buforUARTTmp, PSTR("index strony %d \n"), indeksBiezacejStrony);
    uart_puts(buforUARTTmp);

#endif

    DS18X20_StartMeasurement();

    power_spi_disable();
    set_sleep_mode(SLEEP_MODE_ADC);
    wdt_enable(WDTO_4S);

    sei();

    for (;;) {

        wdt_reset();

        int8_t indexFunStanu = daj_funkcje_stanu(stanUkladu.biezacyStan);
        if (indexFunStanu != -1) {
            tablicaStanow[indexFunStanu].wykonajStan();
        }
    }

    return 0;
}

