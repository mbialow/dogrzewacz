
#ifndef MAIN_H_
#define MAIN_H_

#include <time.h>
#include "cmd.h"


#define PRZEKAZNIK_PORT  PORTB
#define PRZEKAZNIK_DDR   DDRB
#define PRZEKAZNIK_1_PIN PIN0
#define PRZEKAZNIK_2_PIN PIN2

#define UART_BAUD_RATE 4800


#define CZAS_PRACY_GRZALEK_SEKUNDY 60*5 //(60 s * 5 minut)
#define CZAS_PRACY_SILNIKA_SEKUNDY 10 //po 10 sekundach od wzrostu napiecia mozemy weryfikowac warunki wlaczenia dogrzewania
#define INTERWAL_ZAPISU_PROBEK_SPOCZYNEK 60*30
#define INTERWAL_ZAPISU_PROBEK_PRACA 60 //co minute

#define CZAS_ZWLOKI_POMIEDZY_KOLEJNYM_WLACZENIEM_DOGRZEWANIA_SEKUNDY 60*60*2
#define CZAS_PRACY_DO_USPIENIA_SEKUNDY 2

#define NAPIECIE_PRACY 130 //130 = 13V powyzej uklad weryfikuje pozostale warunki

#define TEMPERATURA_WYLACZENIE_DOGRZEWANIA 65 //temperatura cieczy chlodzacej po osiagnieciu ktorej wylaczy sie dogrzewanie
#define TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_1   5 //ponizej 5 stopni mozemy wlaczyc dogrzewanie
#define TEMERATURA_ZEWNETRZNA_WLACZENIA_PRZEKAZNIK_2   -7 // ponizej -7 wlaczmy przekazniki 2

typedef struct tProbka {

     uint8_t	 napiecieAkumulatora;
     int8_t      temperaturaZewnetrzna;
     int8_t		 temperaturaCieczyChlodzacej;

     time_t 	 czasPomiaru;

} Probka;

typedef struct tStanUkladu {

    uint8_t		 napiecieAkumulatora;
    int8_t       temperaturaZewnetrzna;
    int8_t		 temperaturaCieczyChlodzacej;

    uint16_t	 sekundaPracyUkladu;
    uint16_t     momentWlaczeniaSilnika;
    uint16_t	 momentWlaczeniaGrzalek;

    Stan         bierzacyStan;

    time_t       czasWylaczeniaDogrzewania;

} StanUkladu ;


extern volatile StanUkladu stanUkladu   __attribute__((section(".noinit")));
extern time_t RTC_time  		 	    __attribute__((section(".noinit")));


#endif /* MAIN_H_ */
