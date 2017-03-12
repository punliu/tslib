
#include <errno.h>
#include <string.h>
#ifdef __FreeBSD__
#include <dev/evdev/input.h>
#include <dev/evdev/uinput.h>
#include <sys/types.h>
#include <sys/fbio.h>
#else
#include <linux/input.h>
#include <linux/uinput.h>
#include <linux/fb.h>
#endif

#include <tslib.h>

#define MAX_CODES_PER_SLOT 20
int send_ts_events(struct input_event *ev, int fd_uinput,
		   struct ts_sample_mt **s, int nr, int max_slots,
		   short mt_type_a);
