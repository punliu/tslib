/*
 *  tslib/src/ts_test_mt.c
 *
 *  Copyright (C) 2016 Martin Kepplinger
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Basic multitouch test program for touchscreen library.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>

#ifdef __FreeBSD__
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

#include "tslib.h"
#include "testutils.h"

#if USE_SDL2
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#else
#include "fbutils.h"
#endif

static int palette[] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (int)(sizeof (palette) / sizeof (palette [0]))

#define NR_BUTTONS 3
#if USE_SDL2
#else
static struct ts_button buttons[NR_BUTTONS];
#endif

static void sig(int sig)
{
#if USE_SDL2
	SDL_Quit();
#else
	close_framebuffer();
#endif
	fflush(stderr);
	printf("signal %d caught\n", sig);
	fflush(stdout);
	exit(1);
}

static void refresh_screen()
{
#if USE_SDL2
#else
	int i;

	fillrect (0, 0, xres - 1, yres - 1, 0);
	put_string_center (xres/2, yres/4,   "TSLIB multitouch test program", 1);
	put_string_center (xres/2, yres/4+20,"Touch screen to move crosshairs", 2);

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]);
#endif
}

static void help()
{
	printf("Usage: ts_test_mt [-v] [-i <device>]\n");
}

int main(int argc, char **argv)
{
	struct tsdev *ts;
	int *x = NULL;
	int *y = NULL;
	int i, j;
	unsigned int mode = 0;
	unsigned int *mode_mt = NULL;
	int quit_pressed = 0;
	struct input_absinfo slot;
	unsigned short max_slots = 1;
	struct ts_sample_mt **samp_mt = NULL;
	short verbose = 0;
#if USE_SDL2
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Surface *surface;
	SDL_Texture *texture;
	SDL_Event event;
#endif

	const char *tsdevice = NULL;

	signal(SIGSEGV, sig);
	signal(SIGINT, sig);
	signal(SIGTERM, sig);

	while (1) {
		const struct option long_options[] = {
			{ "help",         no_argument,       0, 'h' },
			{ "verbose",      no_argument,       0, 'v' },
			{ "idev",         required_argument, 0, 'i' },
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hi:v", long_options, &option_index);

		errno = 0;
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			help();
			return 0;

		case 'v':
			verbose = 1;
			break;

		case 'i':
			tsdevice = optarg;
			break;

		default:
			help();
			break;
		}

		if (errno) {
			char *str = "option ?";
			str[7] = c & 0xff;
			perror(str);
		}
	}

	ts = ts_setup(NULL, 0);
	if (!ts) {
		perror("ts_setup");
		return errno;
	}
	if (verbose)
		printf("ts_test_mt: using input device " GREEN "%s" RESET "\n", tsdevice);

	if (ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot) < 0) {
		perror("ioctl EVIOGABS");
		ts_close(ts);
		return errno;
	}

	max_slots = slot.maximum + 1 - slot.minimum;

	samp_mt = malloc(sizeof(struct ts_sample_mt *));
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	samp_mt[0] = calloc(max_slots, sizeof(struct ts_sample_mt));
	if (!samp_mt[0]) {
		free(samp_mt);
		ts_close(ts);
		return -ENOMEM;
	}

#if USE_SDL2
	printf("starting SDL Video\n");
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("Unable to initialize SDL: %s\n", SDL_GetError());
		free(samp_mt[0]);
		free(samp_mt);
		ts_close(ts);
		return 1;
	}

	if (SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN, &window, &renderer)) {
		SDL_Log("Unable to create window and renderer: %s\n", SDL_GetError());
		goto out;
	}

	if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP)) {
		SDL_Log("Unable to set fullscreen: %s\n", SDL_GetError());
		goto out;
	}

	surface = SDL_LoadBMP("sample.bmp");
	if (!surface) {
		SDL_Log("Unable to load BMP: %s\n", SDL_GetError());
		goto out;
	}

#else
	if (open_framebuffer()) {
		close_framebuffer();
		free(samp_mt[0]);
		free(samp_mt);
		ts_close(ts);
		exit(1);
	}
#endif

	x = calloc(max_slots, sizeof(int));
	if (!x)
		goto out;

	y = calloc(max_slots, sizeof(int));
	if (!y)
		goto out;

	mode_mt = calloc(max_slots, sizeof(unsigned int));
	if (!mode_mt)
		goto out;

	for (i = 0; i < max_slots; i++) {
	#if USE_SDL2
	#else
		x[i] = xres/2;
		y[i] = yres/2;
	#endif
	}

#if USE_SDL2
#else
	for (i = 0; i < NR_COLORS; i++)
		setcolor (i, palette [i]);
#endif

	/* Initialize buttons */
#if USE_SDL2
#else
	memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = buttons [2].w = xres / 4;
	buttons [0].h = buttons [1].h = buttons [2].h = 20;
	buttons [0].x = 0;
	buttons [1].x = (3 * xres) / 8;
	buttons [2].x = (3 * xres) / 4;
	buttons [0].y = buttons [1].y = buttons [2].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	buttons [2].text = "Quit";
#endif

	refresh_screen ();

	while (1) {
		int ret;

		/* Show the cross */
		for (j = 0; j < max_slots; j++) {
			if ((mode & 15) != 1) { /* not in draw mode */
				/* Hide slots > 0 if released */
				if (j > 0 && (mode_mt[j] & 0x00000008))
					continue;

			#if USE_SDL2
			#else
				put_cross(x[j], y[j], 2 | XORMODE);
			#endif
			}
		}

		ret = ts_read_mt(ts, samp_mt, max_slots, 1);
	#if USE_SDL2
	#else
		SDL_DestroyWindow(window);
	#endif

		/* Hide it */
		for (j = 0; j < max_slots; j++) {
			if ((mode & 15) != 1) { /* not in draw mode */
				if (j > 0 && (mode_mt[j] & 0x00000008))
					continue;

			#if USE_SDL2
			#else
				put_cross(x[j], y[j], 2 | XORMODE);
			#endif
			}
		}

		if (ret < 0) {
			perror("ts_read");
		#if USE_SDL2
			SDL_Quit();
		#else
			close_framebuffer();
		#endif
			free(samp_mt);
			ts_close(ts);
			exit(1);
		}

		if (ret != 1)
			continue;

		for (j = 0; j < max_slots; j++) {
			if (samp_mt[0][j].valid != 1)
				continue;

			for (i = 0; i < NR_BUTTONS; i++) {
			#if USE_SDL2
			#else
				if (button_handle(&buttons[i],
						  samp_mt[0][j].x,
						  samp_mt[0][j].y,
						  samp_mt[0][j].pressure)) {
			#endif
					switch (i) {
					case 0:
						mode = 0;
						refresh_screen();
						break;
					case 1:
						mode = 1;
						refresh_screen();
						break;
					case 2:
						quit_pressed = 1;
					}
			#if USE_SDL2
			#else
				}
			#endif
			}

			if (verbose) {
				printf(YELLOW "%ld.%06ld:" RESET " (slot %d) %6d %6d %6d\n",
					samp_mt[0][j].tv.tv_sec,
					samp_mt[0][j].tv.tv_usec,
					samp_mt[0][j].slot,
					samp_mt[0][j].x,
					samp_mt[0][j].y,
					samp_mt[0][j].pressure);
			}

			if (samp_mt[0][j].pressure > 0) {
				if (mode == 0x80000001) { /* draw mode while drawing */
					if (mode_mt[j] == 0x80000000) { /* slot while drawing */
					#if USE_SDL2
					#else
						line (x[j], y[j], samp_mt[0][j].x, samp_mt[0][j].y, 2);
					#endif
					}
				}
				x[j] = samp_mt[0][j].x;
				y[j] = samp_mt[0][j].y;
				mode |= 0x80000000;
				mode_mt[j] |= 0x80000000;
				mode_mt[j] &= ~0x00000008;
			} else {
				mode &= ~0x80000000;
				mode_mt[j] &= ~0x80000000;
				mode_mt[j] |= 0x00000008; /* hide the cross */
			}
			if (quit_pressed)
				goto out;
		}
	}

out:
#if USE_SDL2
	SDL_Quit();
#else
	close_framebuffer();
#endif

	if (ts)
		ts_close(ts);

	if (samp_mt) {
		if (samp_mt[0])
			free(samp_mt[0]);

		free(samp_mt);
	}

	if (x)
		free(x);

	if (y)
		free(y);

	if (mode_mt)
		free(mode_mt);

	return 0;
}
