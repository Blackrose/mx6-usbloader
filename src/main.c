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
#include <sys/mman.h>
#include <sys/stat.h>

//#define addr	0x10000000
#define addr	0x00907000

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

int main(int argc, char *argv[])
{
	struct sdp *sdp = sdp_open(NULL);
	int		fd = open(argv[1], O_RDONLY);
	struct stat	st;
	void		*data;

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
#endif

	struct ivt	*ivt;
	struct dcd	*dcd;
	size_t		dcd_len;

//	uint32_t	tmp;

	if (!sdp || fd < 0)
		abort();

	if (fstat(fd, &st) < 0)
		abort();

//	bdata.length = htobe32(st.st_size);

	data = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		abort();

	ivt = data + 0x400;
	if (le32toh(ivt->dcd) < le32toh(ivt->self) ||
	    le32toh(ivt->dcd) - le32toh(ivt->self) > st.st_size)
		abort();

	dcd = data + 0x400 + le32toh(ivt->dcd) - le32toh(ivt->self);
	dcd_len = (be32toh(dcd->header) >> 8) & 0xffff;

	if (!sdp_write_dcd(sdp, dcd, dcd_len))
		abort();

	ivt->dcd = 0;

	if (!sdp_write_file(sdp, le32toh(ivt->self) - 0x400, data, st.st_size) ||
	    !sdp_jump(sdp, le32toh(ivt->self)))
		abort();

#if 0
	if (!sdp_write_file(sdp, addr, &ivt, sizeof ivt) ||
	    !sdp_write_file(sdp, addr + sizeof ivt, &bdata, sizeof bdata) ||
	    !sdp_write_file(sdp, addr + 0x100, data, st.st_size) ||
	    !sdp_jump(sdp, addr))
		abort();
#endif

#if 0
	sdp_read_regl(sdp, addr, &tmp);
	sdp_read_regl(sdp, 0xd0, &tmp);
	sdp_read_regl(sdp, be32toh(tmp) + 4, &tmp);
#endif


	sdp_close(sdp);
}
