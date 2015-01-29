/*
 *  @file queue.h
 *
 *  @brief queue functions
 *
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "define.h"
#include <stdlib.h>
#include <stdint.h>

void insert(packet_t*, uint8_t*, uint8_t);
int find(packet_t*, uint8_t*);
void my_delete(packet_t*, int, int );

#endif /* QUEUE_H_ */
