/*
 * raspberrypi_io.h
 *
 *  Created on: 23.06.2014
 *      Author: root
 */

#ifndef RASPBERRYPI_IO_H_
#define RASPBERRYPI_IO_H_
#ifdef RASPI
	#include <wiringPi.h>
#endif
#include <stdio.h>
#include <stdlib.h>

int readTemp(float *temp);
int writeLEDred(int state);

#endif /* RASPBERRYPI_IO_H_ */
