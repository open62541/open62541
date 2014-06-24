/*
 * raspberrypi_io.c
 *
 *  Created on: 23.06.2014
 *      Author: root
 */

#include "raspberrypi_io.h"



#ifdef RASPI
int readTemp(float *temp){
	FILE *ptr_file;
	char buf[1000];
	char delimiter[] = "=";
	char *ptr_temp;
	long temperaturInmC;

	ptr_file =fopen("/sys/bus/w1/devices/10-000802c607f3/w1_slave","r");
	if (!ptr_file){
		puts("ERROR: no sensor");
		return 1;
	}

	fgets(buf, 1000, ptr_file);

	fgets(buf, 1000, ptr_file);

	ptr_temp = strtok(buf, (char*)delimiter);

	ptr_temp = strtok((void*)0, (char*)delimiter);

	temperaturInmC = atol(ptr_temp);

	fclose(ptr_file);

	*temp = (float)temperaturInmC/1000;
	return 0;
}

int writeLEDred(int state){
	if (wiringPiSetup() == -1){
		return 1;
	}
	pinMode(0, OUTPUT);

	digitalWrite(0, state);

	return 0;
}
#endif
