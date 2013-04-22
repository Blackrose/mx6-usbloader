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

#ifndef __packed
#  define __packed	__attribute__((__packed__))
#endif

typedef uint8_t		be8_t;
typedef uint16_t	be16_t;
typedef uint32_t	be32_t;

struct sdp;

struct sdp_dcd_hdr {
	uint8_t	tag;
	be16_t	length;
	uint8_t	version;
} __packed;

struct sdp_dcd_buffer {
	struct sdp_dcd_hdr	hdr;
	uint8_t			data[];
} __packed;

struct sdp_dcd {
	struct sdp_dcd_buffer	*buf;
	size_t			sz;
	size_t			allocated;
};

struct sdp_dcd_write_data {
	uint32_t		addr;
	uint32_t		val_mask;
};

bool	sdp_dcd_init(struct sdp_dcd *dcd);
bool	sdp_dcd_free(struct sdp_dcd *dcd);
bool	sdp_dcd_data(struct sdp_dcd *dcd, uint8_t flags,
		     struct sdp_dcd_write_data const *data,
		     size_t cnt);
bool	sdp_dcd_check(struct sdp_dcd *dcd, uint8_t flags,
		      uint32_t address, uint32_t mask, uint32_t count);
bool	sdp_dcd_nop(struct sdp_dcd *dcd);
bool	sdp_dcd_unlock(struct sdp_dcd *dcd, uint8_t eng,
		       uint32_t const values[], size_t cnt);


struct sdp	*sdp_open(bool (*match)(void *));
void	sdp_close(struct sdp *sdp);

bool	sdp_read_regb(struct sdp *, uint32_t addr, uint8_t *val);
bool	sdp_read_regw(struct sdp *, uint32_t addr, uint16_t *val);
bool	sdp_read_regl(struct sdp *, uint32_t addr, uint32_t *val);

bool	sdp_read_writeb(struct sdp *, uint8_t val, uint32_t addr);
bool	sdp_read_writew(struct sdp *, uint16_t val, uint32_t addr);
bool	sdp_read_writel(struct sdp *, uint32_t val, uint32_t addr);

bool	sdp_write_file(struct sdp *, uint32_t addr,
		       void const *data, size_t count);

//bool	sdp_write_dcd(struct sdp *, struct sdp_dcd const *dcd);
bool	sdp_write_dcd(struct sdp *, void const *dcd, size_t len);

bool	sdp_jump(struct sdp *, uint32_t addr);

#endif	/* H_MX6_LOAD_SDP_H */
