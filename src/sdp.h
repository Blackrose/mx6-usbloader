/*	--*- c -*--
 * Copyright (C) 2013 Enrico Scholz <enrico.scholz@sigma-chemnitz.de>
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

#ifndef H_MX6_LOAD_SDP_H
#define H_MX6_LOAD_SDP_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

struct sdp;
struct libusb_context;

struct sdp_context {
	struct libusb_context	*usb;
	bool			(*match)(struct sdp_context *, void *);
	bool			(*wait_for_device)(struct sdp_context *);
};

struct sdp *sdp_open(struct sdp_context *info);
void	sdp_close(struct sdp *sdp);

bool	sdp_read_regb(struct sdp *, uint32_t addr, uint8_t val[], size_t cnt);
bool	sdp_read_regw(struct sdp *, uint32_t addr, uint16_t val[], size_t cnt);
bool	sdp_read_regl(struct sdp *, uint32_t addr, uint32_t val[], size_t cnt);

bool	sdp_read_writeb(struct sdp *, uint8_t val, uint32_t addr);
bool	sdp_read_writew(struct sdp *, uint16_t val, uint32_t addr);
bool	sdp_read_writel(struct sdp *, uint32_t val, uint32_t addr);

bool	sdp_write_file(struct sdp *, uint32_t addr,
		       void const *data, size_t count);

//bool	sdp_write_dcd(struct sdp *, struct sdp_dcd const *dcd);
bool	sdp_write_dcd(struct sdp *, void const *dcd, size_t len);

bool	sdp_jump(struct sdp *, uint32_t addr);

char const	*sdp_get_devpath(struct sdp *);

#endif	/* H_MX6_LOAD_SDP_H */
