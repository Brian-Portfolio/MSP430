/*
 * STACK.h
 *
 *  Created on: Oct 17, 2019
 *      Author: vselabs
 */

#ifndef STACK_H_
#define STACK_H_

#include "msp430.h"
#include "stdint.h"

void display_init();
void push(unsigned long num);
int pop();

#endif /* STACK_H_ */
