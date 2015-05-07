/*
 * Copyright (c) 2013-2015 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _KFABRIC_H_
#define _KFABRIC_H_

struct kfi_tx_attr {
	uint64_t		caps;
	uint64_t		mode;
	uint64_t		op_flags;
	uint64_t		msg_order;
	uint64_t		comp_order;
	size_t			inject_size;
	size_t			size;
	size_t			iov_limit;
	size_t			rma_iov_limit;
};

struct kfi_rx_attr {
	uint64_t		caps;
	uint64_t		mode;
	uint64_t		op_flags;
	uint64_t		msg_order;
	uint64_t		comp_order;
	size_t			total_buffered_recv;
	size_t			size;
	size_t			iov_limit;
};

struct kfi_ep_attr {
	uint32_t		protocol;
	uint32_t		protocol_version;
	size_t			max_msg_size;
	size_t			msg_prefix_size;
	size_t			max_order_raw_size;
	size_t			max_order_war_size;
	size_t			max_order_waw_size;
	uint64_t		mem_tag_format;
	size_t			tx_ctx_cnt;
	size_t			rx_ctx_cnt;
};

struct kfi_domain_attr {
	struct kfid_domain	*domain;
	char			*name;
	enum kfi_threading	threading;
	enum kfi_progress	control_progress;
	enum kfi_progress	data_progress;
	enum kfi_resource_mgmt	resource_mgmt;
	enum kfi_av_type		av_type;
	enum kfi_mr_mode		mr_mode;
	size_t			mr_key_size;
	size_t			cq_data_size;
	size_t			cq_cnt;
	size_t			ep_cnt;
	size_t			tx_ctx_cnt;
	size_t			rx_ctx_cnt;
	size_t			max_ep_tx_ctx;
	size_t			max_ep_rx_ctx;
	size_t			max_ep_stx_ctx;
	size_t			max_ep_srx_ctx;
};

struct kfi_fabric_attr {
	struct kfid_fabric	*fabric;
	char			*name;
	char			*prov_name;
	uint32_t		prov_version;
};

struct kfi_info {
	struct kfi_info		*next;
	uint64_t		caps;
	uint64_t		mode;
	uint32_t		addr_format;
	size_t			src_addrlen;
	size_t			dest_addrlen;
	void			*src_addr;
	void			*dest_addr;
	struct kfi_tx_attr	*tx_attr;
	struct kfi_rx_attr	*rx_attr;
	struct kfi_ep_attr	*ep_attr;
	struct kfi_domain_attr	*domain_attr;
	struct kfi_fabric_attr	*fabric_attr;
};

struct kfi_ops {
	int	(*close)(struct kfid *fid);
	int	(*bind)(struct kfid *fid, struct kfid *bfid, uint64_t flags);
	int	(*control)(struct kfid *fid, int command, void *arg);
	int	(*ops_open)(struct kfid *fid, const char *name,
			uint64_t flags, void **ops, void *context);
};

struct kfid {
	void			*context;
	struct kfi_ops		*ops;
};

int kfi_getinfo(uint32_t version, struct kfi_info *hints, struct kfi_info **info);
void kfi_freeinfo(struct kfi_info *info);

void kfi_provider_register(uint32_t version, struct kfi_provider *provider);
void kfi_provider_deregister(struct kfi_provider *provider);

struct kfi_ops_fabric {
	int	(*domain)(struct kfid_fabric *fabric, struct kfi_info *info,
			struct kfid_domain **dom, void *context);
	int	(*passive_ep)(struct kfid_fabric *fabric, struct kfi_info *info,
			struct kfid_pep **pep, void *context);
	int	(*eq_open)(struct kfid_fabric *fabric, struct kfi_eq_attr *attr,
			struct kfid_eq **eq, void *context);
	int	(*wait_open)(struct kfid_fabric *fabric, struct kfi_wait_attr *attr,
			struct kfid_wait **waitset);
};

struct kfid_fabric {
	struct kfid		fid;
	struct kfi_ops_fabric	*ops;
};

int kfi_fabric(struct kfi_fabric_attr *attr, struct kfid_fabric **fabric, void *context);

static inline int
kfi_open_ops(struct kfid *fid, const char *name, uint64_t flags,
	    void **ops, void *context)
{
	return fid->ops->ops_open(fid, name, flags, ops, context);
}

#endif /* _FABRIC_H_ */
