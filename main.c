/*
  main.c

  == Defaults...

  fan gpio pin : 18
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
	printf("pifan <fan pin>\n" \
	       "<fan pin> : The fan control pin (18)\n");

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
	char *token;
	unsigned i;
	unsigned j;
	int fan_pin;
	int callback_id;
	struct fan_params fan_input;

	static struct option long_options[] = {
		{"help",       required_argument, 0,  'h' },
		{0,            0,                 0,  0   }
	};

	strcpy(ip_address, "localhost");
	strcpy(ip_port, "8888");

	while ((opt = getopt_long(argc, argv, "h", 
				  long_options, &long_index )) != -1) {
		switch (opt) {
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, "Invalid Option\n");
			usage(EXIT_FAILURE);
			break;
		}
	}

	if (1 != (argc - optind)) {
		fprintf(stderr, "Fan GPIO Pin Must Be Specified\n");
		usage(EXIT_FAILURE);
	}

	for (i = 0; i < 3; ++i) {
		for (j = 0; j < (sizeof(pins[i]) / sizeof(int)); ++j)
			pins[i][j] = -1;

		j = 0;
		token = strtok(argv[optind++], ":");

		while (NULL != token) {
			pins[i][j++] = atoi(token);
			token = strtok(NULL, ":");
		}

		for (j = 0; j < (sizeof(pins[i]) / sizeof(int)); ++j) {
			if (-1 == pins[i][j]) {
				fprintf(stderr, "Invalid GPIO Pin\n");
				return EXIT_FAILURE;
			}
		}
	}

	pi = pigpio_start(ip_address, ip_port);

	if (0 > pi) {
		fprintf(stderr, "pigpio_start() failed: %d\n", pi);

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

	fan_pin = atoi(argv[optind++]);

	fan_input.pin = fan_pin;
	fan_input.pi = pi;

	rc = pthread_create(&fan_thread, NULL, fan, (void *)&fan_input);

	if (0 != rc) {
		fprintf(stderr, "pthread_create() failed: %d\n", rc);

		return EXIT_FAILURE;
	}

	pthread_join(fan_thread, NULL);
	pigpio_stop(pi);

	pthread_exit(NULL);
}
