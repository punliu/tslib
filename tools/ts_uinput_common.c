
#include "ts_uinput.h"

int send_ts_events(struct input_event *ev, int fd_uinput,
		   struct ts_sample_mt **s, int nr, int max_slots,
		   short mt_type_a)
{
	int i;
	int j;
	int c = 0;

	for (j = 0; j < nr; j++) {
		memset(ev,
		       0,
		       sizeof(struct input_event) * MAX_CODES_PER_SLOT * max_slots);

		for (i = 0; i < max_slots; i++) {
			if (s[j][i].pen_down == 1 || s[j][i].pen_down == 0) {
				ev[c].time = s[j][i].tv;
				ev[c].type = EV_KEY;
				ev[c].code = BTN_TOUCH;
				ev[c].value = s[j][i].pen_down;
				c++;
			}

			if (s[j][i].valid != 1)
				continue;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_SLOT;
			ev[c].value = s[j][i].slot;
			c++;

			/*
			 * This simply supports legacy input events when only
			 * one finger is used.
			 * XXX We should track slot 0, and if it is gone
			 * we should use slot 1 and so on.
			 */
			if (i == 0) {
				ev[c].time = s[j][i].tv;
				ev[c].type = EV_ABS;
				ev[c].code = ABS_X;
				ev[c].value = s[j][i].x;
				c++;

				ev[c].time = s[j][i].tv;
				ev[c].type = EV_ABS;
				ev[c].code = ABS_Y;
				ev[c].value = s[j][i].y;
				c++;

				ev[c].time = s[j][i].tv;
				ev[c].type = EV_ABS;
				ev[c].code = ABS_PRESSURE;
				ev[c].value = s[j][i].pressure;
				c++;
			}

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_POSITION_X;
			ev[c].value = s[j][i].x;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_POSITION_Y;
			ev[c].value = s[j][i].y;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_PRESSURE;
			ev[c].value = s[j][i].pressure;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TOUCH_MAJOR;
			ev[c].value = s[j][i].touch_major;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_WIDTH_MAJOR;
			ev[c].value = s[j][i].width_major;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TOUCH_MINOR;
			ev[c].value = s[j][i].touch_minor;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_WIDTH_MINOR;
			ev[c].value = s[j][i].width_minor;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TOOL_TYPE;
			ev[c].value = s[j][i].tool_type;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TOOL_X;
			ev[c].value = s[j][i].tool_x;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TOOL_Y;
			ev[c].value = s[j][i].tool_y;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_ORIENTATION;
			ev[c].value = s[j][i].orientation;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_DISTANCE;
			ev[c].value = s[j][i].distance;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_BLOB_ID;
			ev[c].value = s[j][i].blob_id;
			c++;

			ev[c].time = s[j][i].tv;
			ev[c].type = EV_ABS;
			ev[c].code = ABS_MT_TRACKING_ID;
			ev[c].value = s[j][i].tracking_id;
			c++;

			if (mt_type_a == 1) {
				ev[c].time = s[j][i].tv;
				ev[c].type = EV_SYN;
				ev[c].code = SYN_MT_REPORT;
				ev[c].value = 0;
				c++;
			}
		}

		if (c > 0) {
			ev[c].time = s[j][i].tv;
			ev[c].type = EV_SYN;
			ev[c].code = SYN_REPORT;
			ev[c].value = 0;


			if (write(fd_uinput,
				  ev,
				  sizeof(struct input_event) * (c + 1)) < 0) {
				perror("write");
				return errno;
			}
		}

		c = 0;
	}

	return 0;
}

