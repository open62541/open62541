/*
 * raspberrypi_io.c
 *
 *  Created on: 23.06.2014
 *      Author: root
 */

#include "raspberrypi_io.h"



#ifdef RASPI
static _Bool initialized = 0;
int initIO()
{
	if (wiringPiSetup() == -1){
		initialized = 0;
		return 1;
	}
	initialized = 1;
	return 0;
}
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

	ptr_temp = strtok(buf, (char*) delimiter);

	ptr_temp = strtok((void*)0, (char*) delimiter);

	temperaturInmC = atol(ptr_temp);

	fclose(ptr_file);

	*temp = (float)temperaturInmC/1000;
	return 0;
}

int writePin(_Bool state, int pin){
	if(initialized)
	{
		pinMode(0, OUTPUT);
		if(state==(1==1)){
			digitalWrite(pin, 1);
		}else{
			digitalWrite(pin, 0);
		}
		return 0;
	}
	return 1; //ERROR
}
#endif
