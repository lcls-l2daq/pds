/* tub.c
 *
 * A holding tank for large amounts of data.  Works with "splice".
 * 
 * Author: Matthew Weaver <weaver@slac.stanford.edu>
 * 
 * 2008 (c) SLAC, Stanford University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/pipe_fs_i.h>
#include <linux/splice.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include "tub.h"

#define DRIVER_NAME TUB_DEVNAME
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matthew Weaver");
MODULE_DESCRIPTION("big reservoir for splice");
MODULE_VERSION("1.0");

#define NDESCRIPTORS	(1024)

struct tub_desc {
        struct pipe_buffer      buf;
        struct pipe_inode_info* pipe;
};
      
struct tub_info {
        int fill;
        int drain;
        struct tub_desc desc[NDESCRIPTORS];
};

static void spd_release_page(struct splice_pipe_desc *spd, unsigned int i)
{
	page_cache_release(spd->pages[i]);
}

/* Copied from fs/pipe.c because it is not exported to modules */
void pipe_wait(struct pipe_inode_info *pipe)
{
	DEFINE_WAIT(wait);

	/*
	 * Pipes are system-local resources, so sleeping on them
	 * is considered a noninteractive wait:
	 */
	prepare_to_wait(&pipe->wait, &wait, TASK_INTERRUPTIBLE);
	if (pipe->inode)
		mutex_unlock(&pipe->inode->i_mutex);
	schedule();
	finish_wait(&pipe->wait, &wait);
	if (pipe->inode)
		mutex_lock(&pipe->inode->i_mutex);
}

/* Copied from fs/splice.c because it is not exported to modules */
ssize_t splice_to_pipe(struct pipe_inode_info *pipe,
		       struct splice_pipe_desc *spd)
{
	unsigned int spd_pages = spd->nr_pages;
	int ret, do_wakeup, page_nr;

	ret = 0;
	do_wakeup = 0;
	page_nr = 0;

	if (pipe->inode)
		mutex_lock(&pipe->inode->i_mutex);

	for (;;) {
		if (!pipe->readers) {
			send_sig(SIGPIPE, current, 0);
			if (!ret)
				ret = -EPIPE;
			break;
		}

		if (pipe->nrbufs < PIPE_BUFFERS) {
			int newbuf = (pipe->curbuf + pipe->nrbufs) & (PIPE_BUFFERS - 1);
			struct pipe_buffer *buf = pipe->bufs + newbuf;

			buf->page = spd->pages[page_nr];
			buf->offset = spd->partial[page_nr].offset;
			buf->len = spd->partial[page_nr].len;
			buf->private = spd->partial[page_nr].private;
			buf->ops = spd->ops;
			if (spd->flags & SPLICE_F_GIFT)
				buf->flags |= PIPE_BUF_FLAG_GIFT;

			pipe->nrbufs++;
			page_nr++;
			ret += buf->len;

			if (pipe->inode)
				do_wakeup = 1;

			if (!--spd->nr_pages)
				break;
			if (pipe->nrbufs < PIPE_BUFFERS)
				continue;

			break;
		}

		if (spd->flags & SPLICE_F_NONBLOCK) {
			if (!ret)
				ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			if (!ret)
				ret = -ERESTARTSYS;
			break;
		}

		if (do_wakeup) {
			smp_mb();
			if (waitqueue_active(&pipe->wait))
				wake_up_interruptible_sync(&pipe->wait);
			kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
			do_wakeup = 0;
		}

		pipe->waiting_writers++;
		pipe_wait(pipe);
		pipe->waiting_writers--;
	}

	if (pipe->inode) {
		mutex_unlock(&pipe->inode->i_mutex);

		if (do_wakeup) {
			smp_mb();
			if (waitqueue_active(&pipe->wait))
				wake_up_interruptible(&pipe->wait);
			kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
		}
	}

	while (page_nr < spd_pages)
		spd->spd_release(spd, page_nr++);

	return ret;
}

/* Device access routines. The device major is the one of misc drivers
 * and if the system has udev it will be created automatically
 */

static struct miscdevice tub_dev;

static int tub_open     (struct inode *inode, struct file *filp);
static int tub_release  (struct inode *inode, struct file *filp);
static ssize_t tub_fill (struct pipe_inode_info *pipe,
			 struct file *filp, loff_t *ppos, size_t len, unsigned int flags);
static ssize_t tub_drain(struct file *filp, loff_t *ppos,
			 struct pipe_inode_info *pipe, size_t len, unsigned int flags);

static struct file_operations tub_fops = {
	.owner        = THIS_MODULE,
	.open         = tub_open,
	.release      = tub_release,
	.splice_read  = tub_drain,
	.splice_write = tub_fill,
};

static int tub_open(struct inode *inode, struct file *filp)
{
	struct tub_info *pdata;

	/* Allocate and initialize open handle data structure */
	pdata = kmalloc(sizeof(struct tub_info), GFP_KERNEL);
	if (pdata == NULL) {
		printk(KERN_ERR "%s: could not allocate %d bytes of memory.\n",
			DRIVER_NAME, sizeof(struct tub_info));
		return -ENOMEM;
	}
	pdata->fill  = 0;
	pdata->drain = 0;
	filp->private_data = pdata;

	return 0;
}

static int tub_release(struct inode *inode, struct file *filp)
{
	struct tub_info *pdata = filp->private_data;

	/* Free any descriptor still owned by this driver */
	int i = pdata->drain;
	while (i != pdata->fill) {
	        struct tub_desc* d = &pdata->desc[i];
                d->buf.ops->release(d->pipe,&d->buf);
		i = (i+1)%NDESCRIPTORS;
	}
	if (filp->private_data!=NULL)
		kfree(filp->private_data);
	
	return 0;
}

static int pipe_to_tub(struct pipe_inode_info *pipe,
		       struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *filp = sd->u.file;
	struct tub_info *pdata = filp->private_data;
	struct tub_desc *d = &pdata->desc[pdata->fill];
	int i = pdata->fill;

	d->pipe = pipe;
	d->buf  = *buf;

	i = (i+1)%NDESCRIPTORS;
	if (i==pdata->drain) return -ENOMEM;

	buf->ops->get(pipe,buf);
	pdata->fill  = i;

	return sd->len;
}

static ssize_t tub_fill(struct pipe_inode_info *pipe, 
			struct file *filp, loff_t *ppos, size_t len, unsigned int flags)
{
	ssize_t ret;
	struct inode *inode = filp->f_mapping->host;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = filp,
	};

	/*
	 * The actor worker might be calling ->prepare_write and
	 * ->commit_write. Most of the time, these expect i_mutex to
	 * be held. Since this may result in an ABBA deadlock with
	 * pipe->inode, we have to order lock acquiry here.
	 */
	inode_double_lock(inode, pipe->inode);
	ret = __splice_from_pipe(pipe, &sd, pipe_to_tub);
	inode_double_unlock(inode, pipe->inode);

	return ret;
}

static ssize_t tub_drain(struct file *filp, loff_t *ppos, 
			 struct pipe_inode_info *pipe, size_t len, unsigned int flags)
{
	struct tub_info *pdata = filp->private_data;
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages = 0,
		.ops = 0,
		.flags = flags,
		.spd_release = spd_release_page,
	};
	int i = pdata->drain;

	while((len > 0) && (spd.nr_pages < PIPE_BUFFERS)) {
		struct tub_desc *d = &pdata->desc[i];
		pages  [spd.nr_pages]         = d->buf.page;
		partial[spd.nr_pages].offset  = d->buf.offset;
		partial[spd.nr_pages].private = d->buf.private;

		if (!spd.ops) {
		        spd.ops = d->buf.ops;
		}
		else if (spd.ops != d->buf.ops) {
		        break;
		}

		if (len < d->buf.len) {
		        partial[spd.nr_pages].len = len;
			d->buf.offset += len;
			d->buf.len    -= len;
			/* we need to tell the system we still own
			   part of this page */
			d->buf.ops->get(d->pipe,&d->buf);
			len = 0;
		} else {
			partial[spd.nr_pages].len = d->buf.len;
			len -= d->buf.len;
			/* This descriptor does not need to
			   reference the page anymore */
			i = (i+1)%NDESCRIPTORS;
		}

		spd.nr_pages++;
		if (i == pdata->fill)
			break;
	}
	pdata->drain = i;
	if (spd.nr_pages == 0)
		return 0;
	return splice_to_pipe(pipe, &spd);
}

static int tub_init(void)
{
	int err;

	/* Initialize the device structure */
	tub_dev.minor = MISC_DYNAMIC_MINOR;
	tub_dev.name = DRIVER_NAME;
	tub_dev.fops = &tub_fops;

	/* Register this driver as a miscellaneous driver */
	err=misc_register(&tub_dev);
	if (err < 0)
		goto err_miscdev_register;

	/* Set the driver internal data pointer */
	dev_set_drvdata(tub_dev.this_device, &tub_dev);

	printk(KERN_INFO "driver %s initialized successfully.\n", DRIVER_NAME);
	return 0;

	misc_deregister(&tub_dev);
err_miscdev_register:
	printk(KERN_ERR "%s: Initialization failed with error %d.\n", 
		DRIVER_NAME, err);
	return err;
}
module_init(tub_init);

static void tub_exit(void)
{
	misc_deregister(&tub_dev);
	printk(KERN_INFO "driver %s unregistered successfully.\n", DRIVER_NAME);
}
module_exit(tub_exit);
