/*
 * Copyright (c) 2004, 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2006-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013 Intel Corp., Inc.  All rights reserved.
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

MODULE_AUTHER("Frank Yang");
MODULE_DESCRIPTION("Generic Interface Framework");
MODULE_LICENSE("Dual BSD/GPL");

struct kfi_prov {
	struct kfi_prov		*next;
	struct kfi_provider	*provider;
};

static struct kfi_prov *kfi_getprov(const char *prov_name);

static struct kfi_prov *prov_head, *prov_tail;

static void cleanup_provider(struct kfi_provider *provider)
{
	if (provider && provider->kfreeinfo) {
		provider->kfreeinfo();
	}
}

void kfi_provider_register(struct kfi_provider *provider);
{
	struct kfi_prov *prov;
	int ret;

	if (!provider) {
		ret = -1;
		goto cleanup;
	}

	prov = kfi_getprov(provider->name);
	if (prov) {
		/* If this provider is older than an already-loaded
		 * provider of the same name, then discard this one.
		 */
		if (prov->provider->version > provider->version) {
			printk(KERN_INFO
			       "a newer %s provider was already loaded; ignoring this one\n",
			       provider->name);
			ret = -1;
			goto cleanup;
		}

		/* This provider is newer than an already-loaded
		 * provider of the same name, so discard the
		 * already-loaded one.
		 */
		printk(KERN_INFO
		       "an older %s provider was already loaded; keeping this one and ignoring the older one\n",
		       provider->name);
		cleanup_provider(prov->provider);

		prov->provider = provider;
		return 0;
	}

	prov = kmalloc(sizeof(*prov), GFP_KERNEL);
	if (!prov) {
		ret = -1;
		goto cleanup;
	}

	prov->provider = provider;
	if (prov_tail){
		prov_tail->next = prov;
	} else {
		prov_head = prov;
	}
	prov_tail = prov;
	return 0;

cleanup:
	cleanup_provider(provider);
	return ret;
}

static void __init kfi_init(void)
{

	prov_head = NULL;
	prov_tail = NULL;
}

static void __exit kfi_exit(void)
{
	struct kfi_prov *prov;

	while (prov_head) {
		prov = prov_head;
		prov_head = prov->next;
		cleanup_provider(prov->provider);
		kfree(prov);
	}

}

static struct kfi_prov *kfi_getprov(const char *prov_name)
{
	struct kfi_prov *prov;

	for (prov = prov_head; prov; prov = prov->next) {
		if (!strcmp(prov_name, prov->provider->name))
			return prov;
	}

	return NULL;
}

struct kfi_info *kfi_allocinfo(void)
{
        struct kfi_info *info;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
        if (!info) {
                return NULL;
	}

        info->tx_attr = kmalloc(sizeof(*info->tx_attr), GFP_KERNEL);
        info->rx_attr = kmalloc(sizeof(*info->rx_attr), GFP_KERNEL);
        info->ep_attr = kmalloc(sizeof(*info->ep_attr), GFP_KERNEL);
        info->domain_attr = kmalloc(sizeof(*info->domain_attr), GFP_KERNEL);
        info->fabric_attr = kmalloc(sizeof(*info->fabric_attr), GFP_KERNEL);
        if (!info->tx_attr|| !info->rx_attr || !info->ep_attr ||
            !info->domain_attr || !info->fabric_attr)
                goto err;

        return info;
err:
        kfi_freeinfo(info);
        return NULL;
}

void kfi_freeinfo(struct kfi_info *info)
{
	struct kfi_info *next;

	for (; info; info = next) {
		next = info->next;
		kfree(info->tx_attr);
		kfree(info->rx_attr);
		kfree(info->ep_attr);
		kfree(info->domain_attr);
		kfree(info->fabric_attr);
		kfree(info);
	}
}

int kfi_getinfo(uint32_t version, struct kfi_info *hints, struct kfi_info **info)
{
	struct kfi_prov *prov;
	struct kfi_info *tail, *cur;
	int ret;

	*info = tail = NULL;
	for (prov = prov_head; prov; prov = prov->next) {
		if (!prov->provider->kgetinfo)
			continue;

		if (hints && hints->fabric_attr && hints->fabric_attr->prov_name &&
		    strcmp(prov->provider->name, hints->fabric_attr->prov_name))
			continue;

		ret = prov->provider->kgetinfo(version, hints, &cur);
		if (ret) {
			printk(KERN_WARNING
			       "kfi_getinfo: provider %s returned -%d\n",
			       prov->provider->name, ret);
			continue;
		}

		if (!*info) {
			*info = cur;
		} else {
			tail->next = cur;
		}
		for (tail = cur; tail->next; tail = tail->next) {
			tail->fabric_attr->prov_name = strdup(prov->provider->name);
			tail->fabric_attr->prov_version = prov->provider->version;
		}
		tail->fabric_attr->prov_name = strdup(prov->provider->name);
		tail->fabric_attr->prov_version = prov->provider->version;
	}

	return *info ? 0 : ret;
}

int kfi_fabric(struct kfi_fabric_attr *attr, struct kfid_fabric **fabric, void *context)
{
	struct kfi_prov *prov;

	if (!attr || !attr->prov_name || !attr->name)
		return -1;

	prov = kfi_getprov(attr->prov_name);
	if (!prov || !prov->provider->fabric)
		return -1;

	return prov->provider->fabric(attr, fabric, context);
}

module_init(kfi_init);
module_exit(kfi_exit);
