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

#ifndef H_ENSC_MX6_LOAD_UTIL_H
#define H_ENSC_MX6_LOAD_UTIL_H

#ifndef __packed
#  define __packed	__attribute__((__packed__))
#endif

typedef uint8_t		be8_t;
typedef uint16_t	be16_t;
typedef uint32_t	be32_t;

#define container_of(_ptr, _type, _attr) __extension__		\
	({								\
		__typeof__( ((_type *)0)->_attr) *_tmp_mptr = (_ptr);	\
		(_type *)((uintptr_t)_tmp_mptr -			\
			  __builtin_offsetof(_type, _attr));		\
	})



#endif	/* H_ENSC_MX6_LOAD_UTIL_H */
