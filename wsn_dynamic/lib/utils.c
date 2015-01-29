/*
 * utils.c
 *
 *  Created on: 04/dic/2012
 *      Author: Daniele
 */

#include <stdint.h>
#include "utils.h"
#include "g2553.h"
#include "cc2500.h"
#include "define.h"

uint8_t* make_dhcp_request(uint8_t next_hop){
	uint8_t* buffer;
	buffer = (uint8_t*)malloc(PACKET_SIZE*sizeof(uint8_t*));
	buffer[0]=PACKET_SIZE;  // packet type dhcp_discover
	buffer[1]=0x03;  // packet type dhcp_discover
	buffer[2]=ID;
	buffer[3]=next_hop;
	return buffer;
}

void Read_i2c()
{
	UCB0CTL1 |= UCTXSTT + UCTR; // I2C start condition (UCTR, so we are going to trasmit a byte through the i2c bus)
	while (UCB0CTL1 & UCTXSTT); // Start condition sent?
	UCB0CTL1 |= UCTXSTP; // I2C stop condition
	while(UCB0CTL1 & UCTXSTP); // ensure stop condition got sent
	UCB0CTL1 &= ~UCTR; // I2C RX (I'm going to read a byte)
	UCB0CTL1 |= UCTXSTT; // I2C start condition
	while (UCB0CTL1 & UCTXSTT); // Start condition sent?
	UCB0CTL1 |= UCTXSTP; // I2C stop condition
	while(UCB0CTL1 & UCTXSTP);
}

void inizialize_window(int window[]){
	unsigned int i;
	for (i=0;i<N_SENSORS;i++)
		window[i]=3;
}

void inc_window(int window[], int i){
	window[i]= (window[i]<3) ? ++window[i] : window[i];
}

void dec_window(int window[], int i){
	window[i]= (window[i]>0) ? --window[i] : 0;
}

void get_min_max(int window[], int* min, int* max){
	int i=0,j=N_SENSORS-1;
	while (!window[i++]);
	*min=i-1;
	//max=0;
	while (!window[j--]);
	*max=j+1;
	//TODO
	//useless or not? i would be in panic if it happens
	//max = (!max) ? min : max;
}

void reset_tar(int n)
{
	TACTL = MC_0 + TACLR; // ACLK, /8, upmode
	TAR=n;
	TACTL = TASSEL_1 + ID_3 + MC_1;
}


void blink(int q, unsigned long int z)
{
	int j;
	P1OUT &= 0xFE; // to be sure the led is switched off
	for(j=0;j<2*q;j++)
	{
		volatile unsigned long int w;            // volatile to prevent optimization
		P1OUT ^= 0x01;                      // Toggle P1.0 using exclusive-OR
		w = z;                          // SW Delay
		do w--;
		while (w != 0);
	}
}

int16_t rssi_to_dbm(uint8_t rssi)
{
	if( rssi>= 128)
	{
		return (rssi-256)/2 - RSSI_OFFSET;
	}
	else if( rssi < 128)
	{
		return rssi/2 - RSSI_OFFSET;
	}
	else {
		// This shouldn't happen, but if it does, return the minimum
		return -136;
	}
}
