/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_PRIVATE_H__
#define __NOUVEAU_PRIVATE_H__

#include <stdint.h>
#include <xf86drm.h>
#include <nouveau_drm.h>

#include "nouveau_drmif.h"
#include "nouveau_device.h"
#include "nouveau_channel.h"
#include "nouveau_grobj.h"
#include "nouveau_notifier.h"
#include "nouveau_bo.h"
#include "nouveau_resource.h"
#include "nouveau_pushbuf.h"

#define CALPB_BUFFERS 4
#define CALPB_BUFSZ   16384
struct nouveau_pushbuf_priv {
	struct nouveau_pushbuf base;

	int use_cal;
	uint32_t cal_suffix0;
	uint32_t cal_suffix1;
	struct nouveau_bo *buffer[CALPB_BUFFERS];
	int current;
	int current_offset;

	unsigned *pushbuf;
	unsigned  size;

	struct drm_nouveau_gem_pushbuf_bo *buffers;
	unsigned nr_buffers;
	struct drm_nouveau_gem_pushbuf_reloc *relocs;
	unsigned nr_relocs;
};
#define nouveau_pushbuf(n) ((struct nouveau_pushbuf_priv *)(n))

int
nouveau_pushbuf_init(struct nouveau_channel *);

struct nouveau_channel_priv {
	struct nouveau_channel base;

	struct drm_nouveau_channel_alloc drm;

	struct nouveau_bo *notifier_bo;

	struct nouveau_pushbuf_priv pb;
};
#define nouveau_channel(n) ((struct nouveau_channel_priv *)(n))

struct nouveau_grobj_priv {
	struct nouveau_grobj base;
};
#define nouveau_grobj(n) ((struct nouveau_grobj_priv *)(n))

struct nouveau_notifier_priv {
	struct nouveau_notifier base;

	struct drm_nouveau_notifierobj_alloc drm;
	volatile void *map;
};
#define nouveau_notifier(n) ((struct nouveau_notifier_priv *)(n))

struct nouveau_bo_priv {
	struct nouveau_bo base;
	int refcount;

	/* Buffer configuration + usage hints */
	unsigned flags;
	unsigned size;
	unsigned align;
	int user;

	/* Tracking */
	struct drm_nouveau_gem_pushbuf_bo *pending;
	struct nouveau_channel *pending_channel;
	int write_marker;

	/* Userspace object */
	void *sysmem;

	/* Kernel object */
	uint32_t global_handle;
	drm_handle_t handle;
	uint64_t map_handle;
	void *map;

	/* Last known information from kernel on buffer status */
	int pinned;
	uint64_t offset;
	uint32_t domain;
};
#define nouveau_bo(n) ((struct nouveau_bo_priv *)(n))

int
nouveau_bo_init(struct nouveau_device *);

void
nouveau_bo_takedown(struct nouveau_device *);

struct drm_nouveau_gem_pushbuf_bo *
nouveau_bo_emit_buffer(struct nouveau_channel *, struct nouveau_bo *);

#endif
