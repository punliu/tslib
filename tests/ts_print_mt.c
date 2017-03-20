/*
 *  tslib/tests/ts_print_mt.c
 *
 *  Copyright (C) 2016 Martin Kepplinger
 *
 * Just prints touchscreen events -- does not paint them on framebuffer
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

#include "tslib.h"
#include "testutils.h"

static void usage(char **argv)
{
	printf("Usage: %s [--raw] [--non-blocking] [-s samples] [-i <device>]\n", argv[0]);
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	char *tsdevice = NULL;
	struct ts_sample_mt **samp_mt = NULL;
	struct input_absinfo slot;
	unsigned short max_slots = 1;
	int ret, i, j;
	int read_samples = 1;
	short non_blocking = 0;
	short raw = 0;

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "idev",         required_argument, NULL, 'i' },
			{ "samples",      required_argument, NULL, 's' },
			{ "non-blocking", no_argument,       NULL, 'n' },
			{ "raw",          no_argument,       NULL, 'r' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:s:nr", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv);
			return 0;

		case 'i':
			tsdevice = optarg;
			break;

		case 'n':
			non_blocking = 1;
			break;

		case 'r':
			raw = 1;
			break;

		case 's':
			read_samples = atoi(optarg);
			if (read_samples <= 0) {
				usage(argv);
				return 0;
			}
			break;

		default:
			usage(argv);
			break;
		}

		if (errno) {
			char *str = "option ?";
			str[7] = c & 0xff;
			perror(str);
		}
	}

	if (non_blocking)
		ts = ts_setup(tsdevice, 1);
	else
		ts = ts_setup(tsdevice, 0);

	if (!ts) {
		perror("ts_setup");
		return errno;
	}

	if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return errno;
	}

	max_slots = slot.maximum + 1 - slot.minimum;

	samp_mt = malloc(read_samples * sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	for (i = 0; i < read_samples; i++) {
		samp_mt[i] = calloc(max_slots, sizeof(struct ts_sample_mt));
		if (!samp_mt[i]) {
			free(samp_mt);
			ts_close(ts);
			return -ENOMEM;
		}
	}

	while (1) {
		if (raw)
			ret = ts_read_raw_mt(ts, samp_mt, max_slots, read_samples);
		else
			ret = ts_read_mt(ts, samp_mt, max_slots, read_samples);

		if (ret < 0) {
			if (non_blocking) {
				printf("ts_print_mt: read returns %d\n", ret);
				continue;
			}

			perror("ts_read_mt");
			ts_close(ts);
			exit(1);
		}

		for (j = 0; j < ret; j++) {
			for (i = 0; i < max_slots; i++) {
				if (samp_mt[j][i].valid != 1)
					continue;

				printf(YELLOW "sample %d - %ld.%06ld -" RESET " (slot %d) %6d %6d %6d\n",
				       j,
				       samp_mt[j][i].tv.tv_sec,
				       samp_mt[j][i].tv.tv_usec,
				       samp_mt[j][i].slot,
				       samp_mt[j][i].x,
				       samp_mt[j][i].y,
				       samp_mt[j][i].pressure);
			}
		}
	}

	ts_close(ts);
}
