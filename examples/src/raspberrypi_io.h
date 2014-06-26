/*
 * raspberrypi_io.h
 *
 *  Created on: 23.06.2014
 *      Author: root
 */

#ifndef RASPBERRYPI_IO_H_
#define RASPBERRYPI_IO_H_

#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"
int readTemp(float *temp);
int writePin(_Bool state, int pin);

#endif /* RASPBERRYPI_IO_H_ */
