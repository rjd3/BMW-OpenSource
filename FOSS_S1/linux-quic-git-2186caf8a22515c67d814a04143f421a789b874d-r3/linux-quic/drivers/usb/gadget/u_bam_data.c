/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/usb/gadget.h>

#include <mach/bam_dmux.h>
#include <mach/usb_gadget_xport.h>
#include <mach/usb_bam.h>

#define BAM2BAM_DATA_N_PORTS	1

static struct workqueue_struct *bam_data_wq;
static int n_bam2bam_data_ports;

#define SPS_PARAMS_SPS_MODE		BIT(5)
#define SPS_PARAMS_TBE		        BIT(6)
#define MSM_VENDOR_ID			BIT(16)

struct data_port {
	struct usb_composite_dev	*cdev;
	struct usb_function		*func;
	struct usb_ep			*in;
	struct usb_ep			*out;
};

struct bam_data_ch_info {
	unsigned long		flags;
	unsigned		id;

	struct bam_data_port	*port;
	struct work_struct	write_tobam_w;

	struct usb_request	*rx_req;
	struct usb_request	*tx_req;

	u32			src_pipe_idx;
	u32			dst_pipe_idx;
	u8			connection_idx;
};

struct bam_data_port {
	unsigned			port_num;
	struct data_port		*port_usb;
	struct bam_data_ch_info		data_ch;

	struct work_struct		connect_w;
	struct work_struct		disconnect_w;
};

struct bam_data_port *bam2bam_data_ports[BAM2BAM_DATA_N_PORTS];

/*------------data_path----------------------------*/

static void bam_data_endless_rx_complete(struct usb_ep *ep,
					 struct usb_request *req)
{
	int status = req->status;

	pr_debug("%s: status: %d\n", __func__, status);
}

static void bam_data_endless_tx_complete(struct usb_ep *ep,
					 struct usb_request *req)
{
	int status = req->status;

	pr_debug("%s: status: %d\n", __func__, status);
}

static void bam_data_start_endless_rx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	if (!port->port_usb)
		return;

	status = usb_ep_queue(port->port_usb->out, d->rx_req, GFP_ATOMIC);
	if (status)
		pr_err("error enqueuing transfer, %d\n", status);
}

static void bam_data_start_endless_tx(struct bam_data_port *port)
{
	struct bam_data_ch_info *d = &port->data_ch;
	int status;

	if (!port->port_usb)
		return;

	status = usb_ep_queue(port->port_usb->in, d->tx_req, GFP_ATOMIC);
	if (status)
		pr_err("error enqueuing transfer, %d\n", status);
}

static int bam_data_peer_reset_cb(void *param)
{
	struct bam_data_port	*port = (struct bam_data_port *)param;
	struct bam_data_ch_info *d;
	int ret;

	d = &port->data_ch;

	pr_debug("%s: reset by peer\n", __func__);

	/* Disable BAM */
	msm_hw_bam_disable(1);

	/* Reset BAM */
	ret = usb_bam_reset(0);
	if (ret) {
		pr_err("%s: BAM reset failed %d\n", __func__, ret);
		return ret;
	}

	/* Enable BAM */
	msm_hw_bam_disable(0);

	/* Unregister the peer reset callback */
	usb_bam_register_peer_reset_cb(d->connection_idx, NULL, NULL);

	return 0;
}

static void bam2bam_data_connect_work(struct work_struct *w)
{
	struct bam_data_port *port = container_of(w, struct bam_data_port,
						  connect_w);
	struct bam_data_ch_info *d = &port->data_ch;
	u32 sps_params;
	int ret;

	pr_debug("%s: Connect workqueue started", __func__);
	usb_bam_reset_complete();

	ret = usb_bam_connect(d->connection_idx, &d->src_pipe_idx,
						  &d->dst_pipe_idx);
	d->src_pipe_idx = 11;
	d->dst_pipe_idx = 10;

	if (ret) {
		pr_err("usb_bam_connect failed: err:%d\n", ret);
		return;
	}

	if (!port->port_usb) {
		pr_err("port_usb is NULL");
		return;
	}

	if (!port->port_usb->out) {
		pr_err("port_usb->out (bulk out ep) is NULL");
		return;
	}

	d->rx_req = usb_ep_alloc_request(port->port_usb->out, GFP_KERNEL);
	if (!d->rx_req)
		return;

	d->rx_req->context = port;
	d->rx_req->complete = bam_data_endless_rx_complete;
	d->rx_req->length = 0;
	sps_params = (SPS_PARAMS_SPS_MODE | d->src_pipe_idx |
				 MSM_VENDOR_ID) & ~SPS_PARAMS_TBE;
	d->rx_req->udc_priv = sps_params;
	d->tx_req = usb_ep_alloc_request(port->port_usb->in, GFP_KERNEL);
	if (!d->tx_req)
		return;

	d->tx_req->context = port;
	d->tx_req->complete = bam_data_endless_tx_complete;
	d->tx_req->length = 0;
	sps_params = (SPS_PARAMS_SPS_MODE | d->dst_pipe_idx |
				 MSM_VENDOR_ID) & ~SPS_PARAMS_TBE;
	d->tx_req->udc_priv = sps_params;

	/* queue in & out requests */
	bam_data_start_endless_rx(port);
	bam_data_start_endless_tx(port);

	/* Register for peer reset callback */
	usb_bam_register_peer_reset_cb(d->connection_idx,
			bam_data_peer_reset_cb, port);

	ret = usb_bam_client_ready(true);
	if (ret) {
		pr_err("%s: usb_bam_client_ready failed: err:%d\n",
			__func__, ret);
		return;
	}

	pr_debug("%s: Connect workqueue done", __func__);
}

static void bam2bam_data_port_free(int portno)
{
	kfree(bam2bam_data_ports[portno]);
	bam2bam_data_ports[portno] = NULL;
}

static int bam2bam_data_port_alloc(int portno)
{
	struct bam_data_port	*port = NULL;
	struct bam_data_ch_info	*d = NULL;

	port = kzalloc(sizeof(struct bam_data_port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	port->port_num = portno;

	INIT_WORK(&port->connect_w, bam2bam_data_connect_work);

	/* data ch */
	d = &port->data_ch;
	d->port = port;
	bam2bam_data_ports[portno] = port;

	pr_debug("port:%p portno:%d\n", port, portno);

	return 0;
}

void bam_data_disconnect(struct data_port *gr, u8 port_num)
{
	struct bam_data_port	*port;

	pr_debug("dev:%p port#%d\n", gr, port_num);

	if (port_num >= n_bam2bam_data_ports) {
		pr_err("invalid bam2bam portno#%d\n", port_num);
		return;
	}

	if (!gr) {
		pr_err("mbim data port is null\n");
		return;
	}

	port = bam2bam_data_ports[port_num];

	if (port->port_usb && port->port_usb->in &&
	  port->port_usb->in->driver_data) {
		/* disable endpoints */
		usb_ep_disable(port->port_usb->out);
		usb_ep_disable(port->port_usb->in);

		port->port_usb->in->driver_data = NULL;
		port->port_usb->out->driver_data = NULL;

		port->port_usb = 0;
	}

	if (usb_bam_client_ready(false))
		pr_err("%s: usb_bam_client_ready failed\n", __func__);
}

int bam_data_connect(struct data_port *gr, u8 port_num,
				 u8 connection_idx)
{
	struct bam_data_port	*port;
	struct bam_data_ch_info	*d;
	int			ret;

	pr_debug("dev:%p port#%d\n", gr, port_num);

	if (port_num >= n_bam2bam_data_ports) {
		pr_err("invalid portno#%d\n", port_num);
		return -ENODEV;
	}

	if (!gr) {
		pr_err("mbim data port is null\n");
		return -ENODEV;
	}

	port = bam2bam_data_ports[port_num];

	d = &port->data_ch;

	if (!gr) {
		pr_err("data port is null\n");
		return -ENODEV;
	}

	ret = usb_ep_enable(gr->in);
	if (ret) {
		pr_err("usb_ep_enable failed eptype:IN ep:%p", gr->in);
		return ret;
	}
	gr->in->driver_data = port;

	ret = usb_ep_enable(gr->out);
	if (ret) {
		pr_err("usb_ep_enable failed eptype:OUT ep:%p", gr->out);
		gr->in->driver_data = 0;
		return ret;
	}
	gr->out->driver_data = port;

	port->port_usb = gr;

	d->connection_idx = connection_idx;

	queue_work(bam_data_wq, &port->connect_w);

	return 0;
}

int bam_data_destroy(unsigned int no_bam2bam_port)
{
	struct bam_data_ch_info	*d;
	struct bam_data_port	*port;

	port = bam2bam_data_ports[no_bam2bam_port];
	d = &port->data_ch;

	pr_debug("bam_data_destroy: Freeing ports\n");
	bam2bam_data_port_free(no_bam2bam_port);
	if (bam_data_wq)
		destroy_workqueue(bam_data_wq);
	bam_data_wq = NULL;

	return 0;
}

int bam_data_setup(unsigned int no_bam2bam_port)
{
	int	i;
	int	ret;

	pr_debug("requested %d BAM2BAM ports", no_bam2bam_port);

	if (!no_bam2bam_port || no_bam2bam_port > BAM2BAM_DATA_N_PORTS) {
		pr_err("Invalid num of ports count:%d\n", no_bam2bam_port);
		return -EINVAL;
	}

	if (bam_data_wq) {
		pr_debug("bam_data is already setup");
		return 0;
	}

	bam_data_wq = alloc_workqueue("k_bam_data",
				      WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!bam_data_wq) {
		pr_err("Failed to create workqueue\n");
		return -ENOMEM;
	}

	for (i = 0; i < no_bam2bam_port; i++) {
		n_bam2bam_data_ports++;
		ret = bam2bam_data_port_alloc(i);
		if (ret) {
			n_bam2bam_data_ports--;
			pr_err("Failed to alloc port:%d\n", i);
			goto free_bam_ports;
		}
	}

	return 0;

free_bam_ports:
	for (i = 0; i < n_bam2bam_data_ports; i++)
		bam2bam_data_port_free(i);
	destroy_workqueue(bam_data_wq);

	return ret;
}

static int bam_data_wake_cb(void *param)
{
	struct bam_data_port *port = (struct bam_data_port *)param;
	struct data_port *d_port = port->port_usb;

	pr_debug("%s: woken up by peer\n", __func__);

	if (!d_port) {
		pr_err("FAILED: d_port == NULL");
		return -ENODEV;
	}

	if (!d_port->cdev) {
		pr_err("FAILED: d_port->cdev == NULL");
		return -ENODEV;
	}

	if (!d_port->cdev->gadget) {
		pr_err("FAILED: d_port->cdev->gadget == NULL");
		return -ENODEV;
	}

	return usb_gadget_wakeup(d_port->cdev->gadget);
}

void bam_data_suspend(u8 port_num)
{

	struct bam_data_port	*port;
	struct bam_data_ch_info *d;

	port = bam2bam_data_ports[port_num];
	d = &port->data_ch;

	pr_debug("%s: suspended port %d\n", __func__, port_num);
	usb_bam_register_wake_cb(d->connection_idx, bam_data_wake_cb, port);
}

void bam_data_resume(u8 port_num)
{

	struct bam_data_port	*port;
	struct bam_data_ch_info *d;

	port = bam2bam_data_ports[port_num];
	d = &port->data_ch;

	pr_debug("%s: resumed port %d\n", __func__, port_num);
	usb_bam_register_wake_cb(d->connection_idx, NULL, NULL);
}

