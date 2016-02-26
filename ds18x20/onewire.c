/*
Access Dallas 1-Wire Devices with ATMEL AVRs
Author of the initial code: Peter Dannegger (danni(at)specs.de)
modified by Martin Thomas (mthomas(at)rhrk.uni-kl.de)
 9/2004 - use of delay.h, optional bus configuration at runtime
10/2009 - additional delay in ow_bit_io for recovery
 5/2010 - timing modifcations, additonal config-values and comments,
          use of atomic.h macros, internal pull-up support
 7/2010 - added method to skip recovery time after last bit transfered
          via ow_command_skip_last_recovery
*/

/*
 * Data 18 mar 2015
 * Marcin Bialowas -  mbialow@gmail.com
 *
 * Usunieto kod do obslugi wiecej niz 1 czujnika
 * Usunieto tryb pasozytniczy
 *
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "onewire.h"

#ifdef OW_ONE_BUS

#define OW_GET_IN()   ( OW_IN & (1<<OW_PIN))
#define OW_OUT_LOW()  ( OW_OUT &= (~(1 << OW_PIN)) )
#define OW_OUT_HIGH() ( OW_OUT |= (1 << OW_PIN) )
#define OW_DIR_IN()   ( OW_DDR &= (~(1 << OW_PIN )) )
#define OW_DIR_OUT()  ( OW_DDR |= (1 << OW_PIN) )

#else

/* set bus-config with ow_set_bus() */
uint8_t OW_PIN_MASK;
volatile uint8_t* OW_IN;
volatile uint8_t* OW_OUT;
volatile uint8_t* OW_DDR;

#define OW_GET_IN()   ( *OW_IN & OW_PIN_MASK )
#define OW_OUT_LOW()  ( *OW_OUT &= (uint8_t) ~OW_PIN_MASK )
#define OW_OUT_HIGH() ( *OW_OUT |= (uint8_t)  OW_PIN_MASK )
#define OW_DIR_IN()   ( *OW_DDR &= (uint8_t) ~OW_PIN_MASK )
#define OW_DIR_OUT()  ( *OW_DDR |= (uint8_t)  OW_PIN_MASK )

void ow_set_bus(volatile uint8_t* in,
    volatile uint8_t* out,
    volatile uint8_t* ddr,
    uint8_t pin)
{
    OW_DDR=ddr;
    OW_OUT=out;
    OW_IN=in;
    OW_PIN_MASK = (1 << pin);
    ow_reset();
}

#endif

uint8_t ow_input_pin_state()
{
    return OW_GET_IN();
}

uint8_t ow_reset(void)
{
    uint8_t err;

    OW_OUT_LOW();
    OW_DIR_OUT();            // pull OW-Pin low for 480us
    _delay_us(480);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // set Pin as input - wait for clients to pull low
        OW_DIR_IN(); // input
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_HIGH();
#endif

        _delay_us(64);       // was 66
        err = OW_GET_IN();   // no presence detect
                             // if err!=0: nobody pulled to low, still high
    }

    // after a delay the clients should release the line
    // and input-pin gets back to high by pull-up-resistor
    _delay_us(480 - 64);       // was 480-66
    if( OW_GET_IN() == 0 ) {
        err = 1;             // short circuit, expected low but got high
    }

    return err;
}


/* Timing issue when using runtime-bus-selection (!OW_ONE_BUS):
   The master should sample at the end of the 15-slot after initiating
   the read-time-slot. The variable bus-settings need more
   cycles than the constant ones so the delays had to be shortened
   to achive a 15uS overall delay
   Setting/clearing a bit in I/O Register needs 1 cyle in OW_ONE_BUS
   but around 14 cyles in configureable bus (us-Delay is 4 cyles per uS) */
static uint8_t ow_bit_io_intern( uint8_t b )
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_LOW();
#endif
        OW_DIR_OUT();    // drive bus low
        _delay_us(2);    // T_INT > 1usec accoding to timing-diagramm
        if ( b ) {
            OW_DIR_IN(); // to write "1" release bus, resistor pulls high
#if OW_USE_INTERNAL_PULLUP
            OW_OUT_HIGH();
#endif
        }

        // "Output data from the DS18B20 is valid for 15usec after the falling
        // edge that initiated the read time slot. Therefore, the master must
        // release the bus and then sample the bus state within 15ussec from
        // the start of the slot."
        _delay_us(15-2-OW_CONF_DELAYOFFSET);

        if( OW_GET_IN() == 0 ) {
            b = 0;  // sample at end of read-timeslot
        }

        _delay_us(60-15-2+OW_CONF_DELAYOFFSET);
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_HIGH();
#endif
        OW_DIR_IN();

    } /* ATOMIC_BLOCK */

    _delay_us(OW_RECOVERY_TIME); // may be increased for longer wires

    return b;
}

uint8_t ow_bit_io( uint8_t b )
{
    return ow_bit_io_intern( b & 1 );
}

uint8_t ow_byte_wr( uint8_t b )
{
    uint8_t i = 8, j;

    do {
        j = ow_bit_io( b & 1 );
        b >>= 1;
        if( j ) {
            b |= 0x80;
        }
    } while( --i );

    return b;
}

uint8_t ow_byte_rd( void )
{
    // read by sending only "1"s, so bus gets released
    // after the init low-pulse in every slot
    return ow_byte_wr( 0xFF );
}


void ow_command( uint8_t command ){

        ow_reset();
        ow_byte_wr( OW_SKIP_ROM );
        ow_byte_wr( command );
}

