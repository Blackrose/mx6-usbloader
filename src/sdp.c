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

#include <stdio.h>
#include <string.h>
#include <libusb.h>
#include <sys/param.h>

#include "util.h"

#define FREESCALE_VENDOR_ID     	0x15a2
#define FREESCALE_PRODUCT_MX6_ID    	0x0054

struct sdp {
	struct libusb_context		*ctx;
	struct libusb_device		*dev;
	struct libusb_device_handle	*h;
	bool				own_context;
};

struct sdp *sdp_open(struct sdp_context *info)
{
	struct sdp		*sdp;
	int			rc;
	ssize_t			dev_list_cnt;
	struct libusb_device	**dev_list = NULL;
	size_t			i;
	bool			claimed = false;

	sdp = calloc(1, sizeof *sdp);
	if (!sdp)
		return NULL;

	if (info && info->usb) {
		sdp->ctx = info->usb;
		sdp->own_context = false;
	} else {
		rc = libusb_init(&sdp->ctx);
		if (rc != 0) {
			fprintf(stderr, "libusb_init(): %s\n", libusb_error_name(rc));
			goto err;
		}

		sdp->own_context = true;
	}

again:
	dev_list_cnt = libusb_get_device_list(sdp->ctx, &dev_list);
	if (dev_list_cnt < 0) {
		fprintf(stderr, "libusb_get_device_list(): %s\n",
			libusb_error_name(rc));
		goto err;
	}

	for (i = 0; i < (size_t)(dev_list_cnt) && !sdp->dev; ++i) {
		struct libusb_device			*dev = dev_list[i];
		struct libusb_device_descriptor		desc;

		rc = libusb_get_device_descriptor(dev, &desc);
		if (rc < 0) {
			fprintf(stderr, "libusb_get_device_descriptor(): %s\n",
				libusb_error_name(rc));
			continue;
		}

		if (desc.idVendor != FREESCALE_VENDOR_ID &&
		    desc.idProduct != FREESCALE_PRODUCT_MX6_ID)
			continue;

		/* \todo: call match() */

		sdp->dev = libusb_ref_device(dev);
	}

	libusb_free_device_list(dev_list, 1);

	if (sdp->dev) {
		; /* noop */
	} else if (info && info->wait_for_device && 
		   info->wait_for_device(info)) {
		goto again;
	} else {
		fprintf(stderr, "no mx6 device found\n");
		goto err;
	}

	rc = libusb_open(sdp->dev, &sdp->h);
	if (rc < 0) {
		fprintf(stderr, "libusb_open(): %s\n", libusb_error_name(rc));
		sdp->h = NULL;
		goto err;
	}

	libusb_detach_kernel_driver(sdp->h, 0);

	rc = libusb_claim_interface(sdp->h, 0);
	if (rc < 0) {
		fprintf(stderr, "libusb_claim_interface(): %s\n",
			libusb_error_name(rc));
		goto err;
	}

	claimed = true;

	return sdp;

err:
	if (claimed)
		libusb_release_interface(sdp->h, 0);

	if (sdp->h)
		libusb_close(sdp->h);

	if (sdp->dev)
		libusb_unref_device(sdp->dev);

	if (sdp->ctx && sdp->own_context)
		libusb_exit(sdp->ctx);

	free(sdp);
	return NULL;
}

void	sdp_close(struct sdp *sdp)
{
	if (!sdp)
		return;

	libusb_release_interface(sdp->h, 0);
	libusb_close(sdp->h);
	libusb_unref_device(sdp->dev);

	if (sdp->own_context)
		libusb_exit(sdp->ctx);

	free(sdp);
}

struct sdp_data_report1 {
	be8_t		id;
	be16_t		cmd;
	be32_t		address;
	be8_t		format;
	be32_t		count;
	be32_t		data;
	be8_t		reserved;
} __packed;

static bool sdp_write_data_report1(struct sdp *sdp,
				   struct sdp_data_report1 const *rep)
{
	int		rc;

	rc = libusb_control_transfer(sdp->h,
				     LIBUSB_REQUEST_TYPE_CLASS | 
				     LIBUSB_RECIPIENT_INTERFACE,
				     0x09, /* SET_REPORT */
				     0x0200 | 1,
				     LIBUSB_ENDPOINT_OUT | 0,
				     (void *)rep, sizeof *rep,
				     2000);
	if (rc < 0) {
		fprintf(stderr, "libusb_control_transfer(<report1>): %s\n",
			libusb_error_name(rc));
		return false;
	}

	return true;
}

static bool sdp_send_payload_report2(struct sdp *sdp,
				     void const *data, size_t len)
{
	unsigned char	buf[1025];
	void const	*ptr = data;
	int		rc = 0;

	while (len > 0 && rc >= 0) {
		size_t		l = MIN(sizeof buf - 1u, len);

		buf[0] = 2;
		memcpy(buf+1, ptr, l);

		rc = libusb_control_transfer(sdp->h,
					     LIBUSB_REQUEST_TYPE_CLASS | 
					     LIBUSB_RECIPIENT_INTERFACE,
					     0x09, /* SET_REPORT */
					     0x0200 | 2,
					     LIBUSB_ENDPOINT_OUT | 0,
					     buf, l+1, 2000);

		ptr += l;
		len -= l;
	}

	if (rc < 0) {
		fprintf(stderr, "libusb_control_transfer(<payload>): %s\n",
			libusb_error_name(rc));
		return false;
	}

	return true;
}

static bool sdp_verify_sec_report3(struct sdp *sdp, uint32_t val)
{
	struct {
		be8_t	id;
		be32_t	code;
	} __packed	buf;

	int		len;
	int		rc;

	rc = libusb_interrupt_transfer(sdp->h,
				       LIBUSB_ENDPOINT_IN | 1,
				       (void *)&buf, sizeof buf, &len,
				       2000);
	if (rc < 0) {
		fprintf(stderr, "libusb_interrupt_transfer(<report3>): %s\n",
			libusb_error_name(rc));
		return false;
	}
	
	if ((size_t)len != sizeof buf) {
		fprintf(stderr, "unexpected report3 len: %d\n", len);
		return false;
	}

	if (buf.id != 3) {
		fprintf(stderr, "unexpected report3 tag: %02x\n", buf.id);
		return false;
	}

	if (be32toh(buf.code) != val) {
		fprintf(stderr, "unexpected report3 status: %08x vs. %08x\n",
			be32toh(buf.code), val);
		return false;
	}

	return true;
}

static bool _sdp_get_data_report4(struct sdp *sdp, void *dst, size_t cnt)
{
	struct {
		be8_t	id;
		uint8_t	data[64];
	} __packed	buf;

	int		len;
	int		rc;

	if (cnt > sizeof buf.data) {
		fprintf(stderr, "internal error; report4 data too large\n");
		abort();
	}

	rc = libusb_interrupt_transfer(sdp->h,
				       LIBUSB_ENDPOINT_IN | 1,
				       (void *)&buf, sizeof buf, &len,
				       2000);
	if (rc < 0) {
		fprintf(stderr, "libusb_interrupt_transfer(<report4>): %s\n",
			libusb_error_name(rc));
		return false;
	}
	
	if ((size_t)len < cnt + 1u) {
		fprintf(stderr, "unexpected report4 len: %d\n", len);
		return false;
	}

	if (buf.id != 4) {
		fprintf(stderr, "unexpected report4 tag: %02x\n", buf.id);
		return false;
	}

	memcpy(dst, buf.data, cnt);

	return true;
}

static bool sdp_get_data_report4(struct sdp *sdp, void *dst, size_t cnt)
{
	while (cnt > 0) {
		size_t	l = MIN(64u, cnt);

		if (!_sdp_get_data_report4(sdp, dst, l))
			return false;

		dst += l;
		cnt -= l;
	}

	return true;
}

static bool _sdp_read_reg(struct sdp *sdp, uint32_t addr, void *dst,
			  size_t elem_sz, size_t cnt)
{
	struct sdp_data_report1		rep = {
		.id		= 1,
		.cmd		= htobe16(0x0101), /* READ_REGISTER */
		.address	= htobe32(addr),
		.count		= htobe32(elem_sz * cnt),
		.format		= elem_sz * 8,
	};

	if (cnt == 0)
		return true;

	if (!sdp_write_data_report1(sdp, &rep) ||
	    !sdp_verify_sec_report3(sdp, 0x56787856) ||
	    !sdp_get_data_report4(sdp, dst, cnt * elem_sz))
		return false;

	return true;
}

bool sdp_read_regb(struct sdp *sdp, uint32_t addr, uint8_t val[], size_t cnt)
{
	if (!_sdp_read_reg(sdp, addr, val, sizeof val[0], cnt))
		return false;

	return true;
}

bool sdp_read_regw(struct sdp *sdp, uint32_t addr, uint16_t val[], size_t cnt)
{
	size_t		i;

	if (!_sdp_read_reg(sdp, addr, val, sizeof val[0], cnt))
		return false;

	for (i = 0; i < cnt; ++i)
		val[i] = be16toh(val[i]);

	return true;
}

bool sdp_read_regl(struct sdp *sdp, uint32_t addr, uint32_t val[], size_t cnt)
{
	size_t		i;

	if (!_sdp_read_reg(sdp, addr, val, sizeof val[0], cnt))
		return false;

	for (i = 0; i < cnt; ++i)
		val[i] = be32toh(val[i]);

	return true;
}

bool	sdp_read_writeb(struct sdp *, uint8_t val, uint32_t addr);
bool	sdp_read_writew(struct sdp *, uint16_t val, uint32_t addr);
bool	sdp_read_writel(struct sdp *, uint32_t val, uint32_t addr);

bool	sdp_read_error_status(struct sdp *sdp, int *status)
{
	struct sdp_data_report1		rep = {
		.id		= 1,
		.cmd		= htobe16(0x0505), /* ERROR_STATUS */
	};
	uint32_t			tmp;

	if (!sdp_write_data_report1(sdp, &rep) ||
	    !sdp_verify_sec_report3(sdp, 0x56787856) ||
	    !sdp_get_data_report4(sdp, &tmp, 4))
		return false;

	*status = be32toh(tmp);
	return true;
}

bool	sdp_write_file(struct sdp *sdp, uint32_t addr,
		       void const *data, size_t count)
{
	struct sdp_data_report1		rep = {
		.id		= 1,
		.cmd		= htobe16(0x0404), /* WRITE_FILE */
		.address	= htobe32(addr),
		.count		= htobe32(count),
	};
	uint32_t			tmp;

	if (!sdp_write_data_report1(sdp, &rep) ||
	    !sdp_send_payload_report2(sdp, data, count) ||
	    !sdp_verify_sec_report3(sdp, 0x56787856) ||
	    !sdp_get_data_report4(sdp, &tmp, 4))
		return false;

	return true;
}

//bool	sdp_write_dcd(struct sdp *, struct sdp_dcd const *dcd)
bool	sdp_write_dcd(struct sdp *sdp, void const *dcd, size_t len)
{
	struct sdp_data_report1		rep = {
		.id		= 1,
		.cmd		= htobe16(0x0a0a), /* DCD_WRITE */
		.address	= htobe32(0x00907000),
		.count		= htobe32(len),
	};
	uint32_t		tmp;

	if (len > 1768) {
		fprintf(stderr, "DCD too large (%zu)\n", len);
		return false;
	}

	if (!sdp_write_data_report1(sdp, &rep) ||
	    !sdp_send_payload_report2(sdp, dcd, len) ||
	    !sdp_verify_sec_report3(sdp, 0x56787856) ||
	    !sdp_get_data_report4(sdp, &tmp, 4))
		return false;

	return true;
}

bool	sdp_jump(struct sdp *sdp, uint32_t addr)
{
	struct sdp_data_report1		rep = {
		.id		= 1,
		.cmd		= htobe16(0x0b0b), /* WRITE_FILE */
		.address	= htobe32(addr),
	};
	uint32_t		tmp;

	if (!sdp_write_data_report1(sdp, &rep) ||
	    !sdp_verify_sec_report3(sdp, 0x56787856))
		return false;

	if (0)
		sdp_get_data_report4(sdp, &tmp, 4);

	return true;
}

char const *sdp_get_devpath(struct sdp *sdp)
{
	uint8_t		ports[8];
	int		num_ports;
	char		*path;
	char		*ptr;

	if (sdp->devpath)
		return sdp->devpath;

	if (!sdp->dev)
		return NULL;

	num_ports = libusb_get_port_numbers(sdp->dev, ports, 8);
	if (num_ports < 0)
		return NULL;

	path = malloc(num_ports * (sizeof "XXX." - 1));
	if (!path)
		return NULL;

	ptr = path;
	for (int i = 0; i < num_ports; ++i)
		ptr += sprintf(ptr, "%s%u", i == 0 ? "" : ".", ports[i]);

	sdp->devpath = path;
	return sdp->devpath;
}
