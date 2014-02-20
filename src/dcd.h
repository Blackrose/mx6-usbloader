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

#ifndef H_ENSC_MX6_LOAD_DCD_H
#define H_ENSC_MX6_LOAD_DCD_H

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


#endif	/* H_ENSC_MX6_LOAD_DCD_H */
