#
#  tslib/tools/Makefile.am
#
#  Copyright (C) 2016 Martin Kepplinger
#
# This file is placed under the LGPL.  Please see the file
# COPYING for more details.
#
#

if ENABLE_TOOLS
if LINUX
EXTRA_DIST		= ts_uinput_start.sh
AM_CFLAGS               = -DTS_POINTERCAL=\"@TS_POINTERCAL@\" $(DEBUGFLAGS)
AM_CPPFLAGS		= -I$(top_srcdir)/src

bin_PROGRAMS		= ts_uinput ts_verify

ts_uinput_SOURCES	= ts_uinput.c ts_uinput.h ts_uinput_common.c
ts_uinput_LDADD		= $(top_builddir)/src/libts.la

ts_verify_SOURCES	= ts_verify.c ts_uinput.h ts_uinput_common.c
ts_verify_LDADD		= $(top_builddir)/src/libts.la
endif
endif
