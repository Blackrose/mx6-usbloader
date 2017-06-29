/*	--*- c -*--
 * Copyright (C) 2013 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "sdp.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <sysexits.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <libudev.h>

#include "util.h"

enum {
	CMD_HELP = 0x1000,
	CMD_VERSION,
};

static struct option const		CMDLINE_OPTIONS[] = {
	{ "help",         no_argument,       0, CMD_HELP },
	{ "version",      no_argument,       0, CMD_VERSION },
	{ "offset",       required_argument, 0, 'o' },
	{ "addr",         required_argument, 0, 'a' },
	{ NULL, 0, 0, 0 }
};

struct ivt {
	uint32_t	header;
	uint32_t	entry;
	uint32_t	rsrvd1;
	uint32_t	dcd;
	uint32_t	boot_data;
	uint32_t	self;
	uint32_t	csf;
	uint32_t	rsrvd2;
} __packed;

struct bdata {
	uint32_t	start;
	uint32_t	length;
	uint32_t	flag;
} __packed;

struct dcd {
	be32_t		header;
	uint8_t		data[];
} __packed;

struct mx6_info {
	struct sdp_context	sdp;
	struct udev		*udev;
	struct udev_monitor	*udev_monitor;
};

static void show_help(void)
{
	printf("Usage: mx6-usbload [--offset|-o <ofs>] <file>\n");
	exit(0);
}

static void show_version(void)
{
	/* \todo: implement me */
	exit(0);
}

static bool wait_for_device(struct sdp_context *info)
{
	struct mx6_info		*mx6 = container_of(info, struct mx6_info, sdp);
	struct udev_monitor	*mon = NULL;
	bool			rc = false;

	if (!mx6->udev)
		/* \todo: only sleep in this case? */
		mon = NULL;
	else if (!mx6->udev_monitor)
		mon = udev_monitor_new_from_netlink(mx6->udev, "udev");
	else
		mon = udev_monitor_ref(mx6->udev_monitor);

	if (!mon) {
		; /* noop */
	} else if (!mx6->udev_monitor) {
		/* first wait cycle; we are not listening on udev events yet
		 * and might run into a race.  Initialize listener only and
		 * return immediately. */
		if (udev_monitor_filter_add_match_subsystem_devtype(
			    mon, "hid", NULL) < 0 ||
		    udev_monitor_filter_update(mon) < 0 ||
		    udev_monitor_enable_receiving(mon) < 0)
			goto out;

		mx6->udev_monitor = udev_monitor_ref(mon);
		rc = true;
	} else {
		struct udev_device	*dev;
		int			fd;
		fd_set			fds;

		fd = udev_monitor_get_fd(mon);
		if (fd < 0)
			goto out;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		select(fd + 1, &fds, NULL, NULL, NULL);

		dev = udev_monitor_receive_device(mon);
		if (!dev)
			goto out;

		udev_device_unref(dev);
		rc = true;
	}

out:
	if (mon)
		udev_monitor_unref(mon);

	return rc;
}

static struct sdp *create_sdp(void) {
	struct mx6_info		mx6 = {
		.sdp	= {
			.wait_for_device = wait_for_device,
		},
		.udev	= udev_new(),
	};
	struct sdp		*sdp = sdp_open(&mx6.sdp);

	if (mx6.udev_monitor)
		udev_monitor_unref(mx6.udev_monitor);

	if (mx6.udev)
		udev_unref(mx6.udev);

	return sdp;
}

int main(int argc, char *argv[])
{
	struct stat		st;
	void			*data;
	unsigned int		offset = 0x400;
	unsigned long		addr = 0x00907000;
	unsigned long		self_addr;
	struct sdp		*sdp;
	char const		*file_name;
	int			fd;

	while (1) {
		int         c = getopt_long(argc, argv, "o:a:",
					    CMDLINE_OPTIONS, NULL);

		if (c==-1)
			break;

		switch (c) {
		case CMD_HELP     :  show_help();
		case CMD_VERSION  :  show_version();
		case 'o'	  :  offset = strtoul(optarg, NULL, 0); break;
		case 'a'	  :  addr   = strtoul(optarg, NULL, 0); break;
		default:
			fprintf(stderr, "Try --help for more information\n");
			return EX_USAGE;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "missing filename\n");
		return EX_USAGE;
	}

	file_name = argv[optind];

	sdp = create_sdp();
	if (!sdp)
		return EX_UNAVAILABLE;

	if (getuid() != geteuid()) {
		uid_t	id = getuid();

		if (setresuid(id, id, id) < 0) {
			perror("setresuid()");
			return EX_OSERR;
		}
	}

	fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open '%s': %m\n", file_name);
		return EX_NOINPUT;
	}

#if 0
	struct ivt	ivt = {
		.header	= htobe32((0xd1u << 24) | ((sizeof ivt) << 8) | 0x40),
		.entry	= htole32(addr + 0x100),
		.dcd	= 0,
		.boot_data = htole32(addr + sizeof ivt),
		.self	= htole32(addr),
	};
	struct bdata	bdata = {
		.start	= htole32(addr + 0x100),
	};
#else
	(void)addr;
#endif

	struct ivt	*ivt;
	struct dcd	*dcd;
	size_t		dcd_len;
	size_t		fsize;

//	uint32_t	tmp;

	if (fstat(fd, &st) < 0) {
		perror("fstat()");
		return EX_OSERR;
	}

	fsize = st.st_size;
	if (offset > fsize) {
		fprintf(stderr, "offset %u out of file (%zu)\n",
			offset, fsize);
		return EX_DATAERR;
	}

	data = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		perror("mmap()");
		return EX_OSERR;
	}

	ivt = data + offset;
	self_addr = le32toh(ivt->self);

	if (le32toh(ivt->dcd) < self_addr ||
	    le32toh(ivt->dcd) - self_addr > fsize) {
		fprintf(stderr,
			"invalid IVT settings: self=%#08x, dcd=%#08x, size=%#08zx\n",
			(unsigned int)le32toh(ivt->self),
			(unsigned int)le32toh(ivt->dcd),
			fsize);
		return EX_DATAERR;
	}

	if (self_addr < offset) {
		fprintf(stderr, "ivt->self=%lx in padding (%x)\n",
			self_addr, offset);
		return EX_DATAERR;
	}

	self_addr -= offset;
	dcd = data + le32toh(ivt->dcd) - self_addr;
	dcd_len = (be32toh(dcd->header) >> 8) & 0xffff;

	printf("Uploading image to %s...", sdp_get_devpath(sdp));

	printf(" DCD[%zu]", dcd_len);
	fflush(stdout);

	if (!sdp_write_dcd(sdp, dcd, dcd_len))
		return EX_OSERR;

	ivt->dcd = 0;

	printf(" FILE[%08lx+%zu]", self_addr, fsize);
	fflush(stdout);
	if (!sdp_write_file(sdp, self_addr, data, fsize) ||
	    !sdp_jump(sdp, self_addr + offset))
		return EX_OSERR;

#if 0
	if (!sdp_write_file(sdp, addr, &ivt, sizeof ivt) ||
	    !sdp_write_file(sdp, addr + sizeof ivt, &bdata, sizeof bdata) ||
	    !sdp_write_file(sdp, addr + 0x100, data, fsize) ||
	    !sdp_jump(sdp, addr))
		return EX_OSERR;
#endif

#if 0
	sdp_read_regl(sdp, addr, &tmp);
	sdp_read_regl(sdp, 0xd0, &tmp);
	sdp_read_regl(sdp, be32toh(tmp) + 4, &tmp);
#endif


	sdp_close(sdp);

	printf(" done\n");
}
