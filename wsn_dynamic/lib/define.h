/*
 *  @file define.h
 *
 *  @brief most important constants used
 *
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#include <stdint.h>

#define N_SENSORS 32 // how many nodes there are in the network
#define ID 34 // unique id set by constructor
#define WINDOW_TIME 2048 // how many TAR's increment i have to wait until the entire transmission window is over
#define N_PANIC_LISTEN 3 // how many times i try to receive a beacon in panic mode
#define N_PANIC_SLEEP 3  // how many times i go to sleep before trying to receive a beacon in panic mode
#define N_BEACON_PANIC 3 // how many beacon i can lost before going to panic mode
#define OFFSET 15 // wake up time before beacon
#define N_HOP_CASCADE 16 // how many sensors you can have in cascade (it determines the time to live of a beacon)
#define MIN_SOC 10 // if my next hop state of charge goes under this value i start searching for a new next hop
#define BEACON_HEADER 5 //how many elements there are in beacon header (before the acknowledgements)
#define PACKET_SIZE 7
#define N_DISCOVERY 10 //how often you do a discovery to look for a new sensors
#define RSSI_THRESHOLD -135;
#define RSSI_RX -135;
#define RSSI_OFFSET (72)


typedef struct packet {
	uint8_t* data;
	struct packet* next;
} packet_t;


#endif /* DEFINE_H_ */
