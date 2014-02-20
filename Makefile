PKG_CONFIG	?= pkg-config
TAR		?= tar

AM_CFLAGS = -std=gnu11 -D_GNU_SOURCE $(WARN_OPTS)
AM_LDFLAGS = -Wl,--as-needed
WARN_OPTS = -Wall -W -Wno-unused-parameter -Wstrict-prototypes -Wno-missing-field-initializers
CFLAGS = -Werror -O1 -g -D_FORTIFY_SOURCE=2 -fstack-protector

LIBUSB_MODULE = libusb-1.0
LIBUDEV_MODULE = libudev

LIBUSB_CFLAGS = $(shell ${PKG_CONFIG} --cflags $(LIBUSB_MODULE))
LIBUSB_LIBS =	$(shell ${PKG_CONFIG} --libs $(LIBUSB_MODULE))
LIBUDEV_CFLAGS = $(shell ${PKG_CONFIG} --libs $(LIBUDEV_MODULE))
LIBUDEV_LIBS =	$(shell ${PKG_CONFIG} --libs $(LIBUDEV_MODULE))

abs_top_srcdir = $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
VPATH += $(abs_top_srcdir)

bin_PROGRAMS = mx6-usbload

mx6-usbload_SOURCES = \
	src/main.c \
	src/sdp.c \
	src/sdp.h \
	src/util.h \

SOURCES = \
	${mx6-usbload_SOURCES} \
	Makefile

CFLAGS_mx6-usbload = $(LIBUSB_CFLAGS) $(LIBUDEV_CFLAGS)
LIBS_mx6-usbload = $(LIBUSB_LIBS) $(LIBUDEV_LIBS)

_buildflags = $(foreach k,CPP $1 LD, $(AM_$kFLAGS) $($kFLAGS) $($kFLAGS_$@))

mx6-usbload:	$(mx6-usbload_SOURCES)
	$(CC) $(call _buildflags,C) $(filter %.c,$^) -o $@ $(LIBS_$@)

dist:
	${TAR} cJf mx6-usbloader.tar.xz $(sort ${SOURCES}) --transform='s!^!mx6-usbloader/!' --owner root --group root --mode go-w,a+rX
