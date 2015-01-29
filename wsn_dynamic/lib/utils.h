/*
 * utils.h
 *
 *  Created on: 04/dic/2012
 *      Author: Daniele
 */

#ifndef UTILS_H_
#define UTILS_H_

uint8_t* make_dhcp_request(uint8_t);
void Read_i2c();
void inizialize_window(int []);
void inc_window(int [], int);
void dec_window(int [], int);
void get_min_max(int[], int*, int*);
void reset_tar(int);
void blink(int, unsigned long int);
int16_t rssi_to_dbm(uint8_t);
#endif /* UTILS_H_ */
