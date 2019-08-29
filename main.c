/*
  main.c

  == Defaults...

  fan  gpio pin : 18
  fan  low temp : 50 degrees C
  fan high temp : 70 degrees C

  == Driver...

  Uses an NPN transistor (S8050) to driver the fan with a 1N4148
  flyback diode.

  See the Schematic in the "hardware" directory.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <pigpiod_if2.h>
#include <arpa/inet.h>

#include "fan.h"

static int pi = -1;

static pthread_t fan_thread;

/*
  ------------------------------------------------------------------------------
  handler
*/

static void
handler(__attribute__((unused)) int signal)
{
	pthread_cancel(fan_thread);
	pthread_join(fan_thread, NULL);

	if (-1 != pi)
		pigpio_stop(pi);

	pthread_exit(NULL);
}

/*
  ------------------------------------------------------------------------------
  usage
*/

static void
usage(int exit_code)
{
	printf("pifan -pin n -lower n -upper n [-help]\n"
	       "\n"
	       "\t-pin : The fan control gpio pin\n"
	       "\t-lower : Turn the fan off below this temperature (deg C)\n"
	       "\t-upper : Turn the fan on above this temperature (deg C)\n"
	       "\t-help  : Display help and exit.\n"
	       "\n"
	       "Between the low and high temperatures, PWM is used to vary\n"
	       "the speed of the fan.\n");

	exit(exit_code);
}

/*
  ------------------------------------------------------------------------------
  main
*/

int
main(int argc, char *argv[])
{
	int rc;
	int opt = 0;
	int long_index = 0;
	char ip_address[NAME_MAX];
	char ip_port[NAME_MAX];
	struct fan_settings settings;

	static struct option long_options[] = {
		{"pin",   required_argument, NULL, 'p'},
		{"lower", required_argument, NULL, 'l'},
		{"upper", required_argument, NULL, 'u'},
		{"help",  no_argument,       NULL, 'h'},
		{NULL,    0,                 NULL,   0}
	};

	/*
	  Only support using the pigpio daemon on the local host for now.
	*/
	strcpy(ip_address, "localhost");
	strcpy(ip_port, "8888");

	/*
	  Don't allow default values.
	*/
	settings.pin = -1;
	settings.lower = -1;
	settings.upper = -1;

	while ((opt = getopt_long_only(argc, argv, "p:l:u:h", 
				       long_options, &long_index )) != -1) {
		switch (opt) {
		case 'p':
			settings.pin = atoi(optarg);
			break;
		case 'l':
			settings.lower = atoi(optarg);
			break;
		case 'u':
			settings.upper = atoi(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, "Invalid Option\n");
			usage(EXIT_FAILURE);
			break;
		}
	}

	if (-1 == settings.pin || -1 == settings.lower || -1 == settings.upper)
		usage(EXIT_FAILURE);

	settings.pi = pigpio_start(ip_address, ip_port);

	if (0 > settings.pi) {
		fprintf(stderr, "pigpio_start() failed: %d\n", settings.pi);

		return EXIT_FAILURE;
	}

	/*
	  Catch Signals
	*/

	signal(SIGHUP, handler);
	signal(SIGINT, handler);
	signal(SIGCONT, handler);
	signal(SIGTERM, handler);

	/*
	  Start the Fan Thread
	*/

	rc = pthread_create(&fan_thread, NULL, fan, (void *)&settings);

	if (0 != rc) {
		fprintf(stderr, "pthread_create() failed: %d\n", rc);

		return EXIT_FAILURE;
	}

	pthread_join(fan_thread, NULL);
	pigpio_stop(pi);

	pthread_exit(NULL);
}
