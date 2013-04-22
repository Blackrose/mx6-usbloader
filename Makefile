PKG_CONFIG	?= pkg-config

AM_CFLAGS = -std=gnu11 $(WARN_OPTS)
AM_LDFLAGS = -Wl,--as-needed
WARN_OPTS = -Wall -W -Wno-unused-parameter -Wstrict-prototypes -Wno-missing-field-initializers
CFLAGS = -Werror -O1 -g -D_FORTIFY_SOURCE=2 -fstack-protector

LIBUSB_MODULE = libusb-1.0
LIBUSB_CFLAGS = $(shell ${PKG_CONFIG} --cflags $(LIBUSB_MODULE))
LIBUSB_LIBS =	$(shell ${PKG_CONFIG} --libs $(LIBUSB_MODULE))

abs_top_srcdir = $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
VPATH += $(abs_top_srcdir)

bin_PROGRAMS = mx6-usbload

mx6-usbload_SOURCES = \
	src/main.c \
	src/sdp.c \
	src/sdp.h \

CFLAGS_mx6-usbload = $(LIBUSB_CFLAGS)
LIBS_mx6-usbload = $(LIBUSB_LIBS)

_buildflags = $(foreach k,CPP $1 LD, $(AM_$kFLAGS) $($kFLAGS) $($kFLAGS_$@))

mx6-usbload:	$(mx6-usbload_SOURCES)
	$(CC) $(call _buildflags,C) $(filter %.c,$^) -o $@ $(LIBS_$@)
