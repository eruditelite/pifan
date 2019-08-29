/*
  fan.h
*/

#ifndef _FAN_H_
#define _FAN_H_

struct fan_settings {
	int pi;			/* pigpiod to use... */
	int pin;		/* GPIO Pin */
	int lower;		/* Low Temperature in Degrees C */
	int upper;		/* Upper Temperature in Degrees C */
};

void *fan(void *);

#endif	/* _FAN_H_ */
