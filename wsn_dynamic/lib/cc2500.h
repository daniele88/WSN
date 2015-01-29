/** @file cc2500.h
*
* @brief cc2500 radio functions
*
*     derived from SLAA465 examples
*/
#ifndef _CC2500_H
#define _CC2500_H

#include <stdint.h>

#define CC2500_BUFFER_LENGTH 64

void setup_cc2500( uint8_t (*)(uint8_t*, uint8_t) );
void cc2500_tx( uint8_t*, uint8_t );

void cc2500_tx_packet( uint8_t*, uint8_t, uint8_t );

void cc2500_set_address( uint8_t );
void cc2500_set_channel( uint8_t );
void cc2500_set_power( uint8_t );

void cc2500_enable_addressing();
void cc2500_disable_addressing();

void cc2500_turn_off();
void cc2500_turn_on();
void cc2500_set_rx();
void cc2500_set_tx();

void writeRFSettings(void);

#endif /* _CC2500_H */
