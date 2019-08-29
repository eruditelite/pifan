/*
  fan.c
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <syslog.h>
#include <pigpiod_if2.h>

#include "fan.h"

enum state {UNSET, OFF, ON, VAR};

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static enum state previous_state = UNSET;
static int previous_duty = -1;


/*
  ------------------------------------------------------------------------------
  get_temp
*/

static int
get_temp(void)
{
	FILE *thermal;
	int temp;

	thermal = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	(void)fscanf(thermal, "%d", &temp);
	fclose(thermal);
	temp /= 1000;

	return temp;
}

/*
  ------------------------------------------------------------------------------
  fan_cleanup
*/

void
fan_cleanup(void *input)
{	
	struct fan_settings *settings;

	settings = (struct fan_settings *)input;

	hardware_PWM(settings->pi, settings->pin, 100, 0);

	syslog(LOG_NOTICE, "pifan ending");
	closelog();

	return;
}

/*
  ------------------------------------------------------------------------------
  fan
*/

void *
fan(void *input)
{
	struct timespec sleep;
	struct fan_settings *settings;
	int temp;
	int rc;
	enum state state;
	int duty;

	settings = (struct fan_settings *)input;

	sleep.tv_sec = 5;
	sleep.tv_nsec = 0;

	pthread_cleanup_push(fan_cleanup, input);

	setlogmask(LOG_UPTO(LOG_NOTICE));

	openlog("pifan", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog(LOG_NOTICE, "pifan started by %d", getuid());

	for (;;) {
		nanosleep(&sleep, NULL);
		temp = get_temp();

		if (settings->upper <= temp) {
			/*
			  Above the upper temperature, run at full speed.
			*/
			rc = hardware_PWM(settings->pi,
					  settings->pin, 100, 1000000);
			state = ON;
		} else if (settings->lower > temp) {
			/*
			  Below the lower temperature, don't run.
			*/
			rc = hardware_PWM(settings->pi,
					  settings->pin, 100, 0);
			state = OFF;
		} else {
			/*
			  Between lower and upper, range from 200000 to 1000000.
			*/
			duty = ((((temp - settings->lower) * 5) *
				     (PI_HW_PWM_RANGE - 200000)) / 100) +
				200000;
			rc = hardware_PWM(settings->pi,
					  settings->pin, 100, duty);
			state = VAR;
		}

		if (0 > rc)
			fprintf(stderr, "hardware_PWM() failed: %s\n",
				pigpio_error(rc));

		pthread_mutex_lock(&state_mutex);

		if ((state != previous_state) ||
		    (state == VAR && (duty != previous_duty))) {
			if (VAR == state) {
				syslog(LOG_NOTICE, "%d%%",
				       (((duty - 200000) * 100) / 800000));
			}
			else if (ON == state)
				syslog(LOG_NOTICE, "ON");
			else
				syslog(LOG_NOTICE, "OFF");
		}

		previous_state = state;
		previous_duty = duty;
		pthread_mutex_unlock(&state_mutex);

		pthread_testcancel();
	}

	pthread_cleanup_pop(1);
	syslog(LOG_NOTICE, "pifan ending");
	closelog();
	pthread_exit(NULL);
}
