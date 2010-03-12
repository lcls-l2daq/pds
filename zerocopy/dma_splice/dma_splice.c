/* dma_splice.c
 *
 * A module for splicing data received via dma.
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

/*  cache attributes  */
#include <asm/cacheflush.h>

#include "dma_splice_util.c"

#include "dma_splice.h"

#define DRIVER_NAME DMA_SPLICE_DEVNAME
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matthew Weaver");
MODULE_DESCRIPTION("single-ended splice interface for dma");
MODULE_VERSION("1.0");

#define NDESCRIPTORS 4096
#define NRELEASE_DESCRIPTORS 4096
#define NNOTIFY_DESCRIPTORS 4096
#define NPAGES 8192

struct dma_splice_info;

/*
**  arg is valid from "released" until "notify"
*/
struct dma_splice_notify {
        unsigned      fill;
        unsigned      drain;
        unsigned long arg[NNOTIFY_DESCRIPTORS];
};

/*
**  Contents are valid from "queue" until "released"
*/
struct dma_splice_release {
        unsigned long           arg;
        atomic_t                _count;
        struct dma_splice_info* pdata;
};

/*
**  contents are valid from "queue" until "read" finished 
*/
struct dma_splice_desc {
        unsigned long addr;
        unsigned long end;
        unsigned long arg;
        struct dma_splice_release* rel_info;
};

/*
**  The driver holds three lists for each buffer:
**    dma_splice_desc    : buffer queued for splice
**    dma_splice_release : tracks use of spliced data and releases
**    dma_splice_notify  : holds released buffers for notifying user app
**  The first list is traversed linearly - buffers are spliced in the order
**    that they are queued.
**  The second list may not be linear.  Spliced buffers may persist elsewhere
**    for an unknown length of time.
**  The third list is linear.  The user app is notified of released buffers
**    in the order that they are released.
*/
struct dma_splice_info {
        unsigned long              baseaddr;
        unsigned                   npages;
        unsigned                   desc_fill;
        unsigned                   desc_drain;
        struct dma_splice_desc     desc[NDESCRIPTORS];
        struct dma_splice_notify   notify;
        struct semaphore           release_mutex;
        struct semaphore           notify_mutex;
        unsigned                   rel_fill;
        unsigned                   rel_drain;
        struct dma_splice_release* rel_info_queue[NRELEASE_DESCRIPTORS];
        struct dma_splice_release* rel_info_master;
        struct page*               pages[NPAGES];
};

/*
**  Helper functions for dma_splice_release
*/
static inline struct dma_splice_release* 
rel_info_get(struct dma_splice_info* pdata,
	     unsigned long arg)
{
        struct dma_splice_release* rel_info;
        unsigned i = pdata->rel_drain;
	if (arg) {
	      rel_info = pdata->rel_info_queue[i];
	      pdata->rel_drain = (i+1)%NRELEASE_DESCRIPTORS;
	      rel_info->arg = arg;
	      atomic_set(&rel_info->_count, 1);
	      pdata->rel_info_master = rel_info;
	}
	else {
	      rel_info = pdata->rel_info_master;
	      atomic_inc(&rel_info->_count);
	}
	return rel_info;
}

static void rel_info_release( struct dma_splice_release* rel_info )
{
    struct dma_splice_info*   pdata  = rel_info->pdata;
    struct dma_splice_notify* notify = &pdata->notify;
    bool empty = notify->fill == notify->drain;
    unsigned long i;

    if (down_interruptible( &pdata->release_mutex ))
           printk(KERN_ALERT "rel_info_release interrupted\n");

    i = notify->fill;
    notify->arg[i] = rel_info->arg;
    notify->fill = (i+1)%NNOTIFY_DESCRIPTORS;

    i = pdata->rel_fill;
    pdata->rel_info_queue[i] = rel_info;
    pdata->rel_fill = (i+1)%NRELEASE_DESCRIPTORS;
    up( &pdata->release_mutex );

    if (empty)
           up( &pdata->notify_mutex );
}

/*
**  Helper functions for dma_splice_desc
*/
static inline struct dma_splice_desc* desc_dequeue(struct dma_splice_info* pdata)
{
        struct dma_splice_desc* rval = 0;
	unsigned curr = pdata->desc_fill;
	unsigned next = (curr+1)%NDESCRIPTORS;
	if (next != pdata->desc_drain) {
		pdata->desc_fill = next;
		rval = &pdata->desc[curr];
	}
	return rval;
}

static void dummy_release(struct splice_pipe_desc *spd, unsigned int i)
{
}

/* Device access routines. The device major is the one of misc drivers
 * and if the system has udev it will be created automatically
 */

static struct miscdevice dma_splice_dev;

static int dma_splice_dev_open     (struct inode *inode, struct file *filp);
static int dma_splice_dev_release  (struct inode *inode, struct file *filp);
static int dma_splice_dev_ioctl    (struct inode *inode, struct file *filp, 
				    unsigned int cmd, unsigned long arg);
static int dma_splice_dev_compat_ioctl( struct file *filp, 
				    unsigned int cmd, unsigned long arg);
static ssize_t dma_splice_dev_read (struct file *filp, loff_t *ppos,
				    struct pipe_inode_info *pipe, size_t len, 
				    unsigned int flags);

static struct file_operations dma_splice_fops = {
	.owner        = THIS_MODULE,
	.open         = dma_splice_dev_open,
	.release      = dma_splice_dev_release,
	.ioctl        = dma_splice_dev_ioctl,
	.compat_ioctl = dma_splice_dev_compat_ioctl,
	.splice_read  = dma_splice_dev_read,
};

static void get_fragment(struct pipe_inode_info *pipe, 
			 struct pipe_buffer *buf)
{
  struct dma_splice_release* rel_info = (struct dma_splice_release*)buf->private;
  atomic_inc(&rel_info->_count);
}

static void release_fragment(struct pipe_inode_info *pipe,
			     struct pipe_buffer *buf)
{
         struct dma_splice_release* rel_info = (struct dma_splice_release*)buf->private;
	 if (atomic_dec_and_test(&rel_info->_count))
	         rel_info_release( rel_info );
}

static void *map_fragment(struct pipe_inode_info *pipe,
			  struct pipe_buffer *buf, int atomic)
{
	if (atomic) {
		buf->flags |= PIPE_BUF_FLAG_ATOMIC;
		return kmap_atomic(buf->page, KM_USER0);
	}

	return kmap(buf->page);
}

static void unmap_fragment(struct pipe_inode_info *pipe,
			   struct pipe_buffer *buf, void *map_data)
{
	if (buf->flags & PIPE_BUF_FLAG_ATOMIC) {
		buf->flags &= ~PIPE_BUF_FLAG_ATOMIC;
		kunmap_atomic(map_data, KM_USER0);
	} else
		kunmap(buf->page);
}

int steal_fragment(struct pipe_inode_info *pipe,
		   struct pipe_buffer *buf)
{
	return 1;
}

int confirm_fragment(struct pipe_inode_info *info,
		     struct pipe_buffer *buf)
{
	return 0;
}

static const struct pipe_buf_operations dma_splice_pbuf_ops = {
	.can_merge = 0,
	.map     = map_fragment,
	.unmap   = unmap_fragment,
	.confirm = confirm_fragment,
	.release = release_fragment,
	.steal   = steal_fragment,
	.get     = get_fragment,
};

static int dma_splice_dev_open(struct inode *inode, struct file *filp)
{
	struct dma_splice_info *pdata;

        printk( KERN_INFO "dma_splice_dev_open() called.\n" );

	/* Allocate and initialize open handle data structure */
	pdata = kmalloc(sizeof(struct dma_splice_info), GFP_KERNEL);
	if (pdata == NULL) {
		printk(KERN_ERR "%s: could not allocate %d bytes of memory.\n",
			DRIVER_NAME, sizeof(struct dma_splice_info));
		return -ENOMEM;
	}
	pdata->baseaddr      = 0;
	pdata->npages        = 0;
	pdata->desc_fill     = 0;
	pdata->desc_drain    = 0;
	pdata->notify.fill   = 0;
	pdata->notify.drain  = 0;

	{
	        int rel_info_queue_sz = NRELEASE_DESCRIPTORS*sizeof(struct dma_splice_release);
		unsigned i;
                struct dma_splice_release* rel
		  = kmalloc(rel_info_queue_sz, GFP_KERNEL);
		if (rel == NULL) {
		        printk(KERN_ERR "%s: could not allocate %d bytes of memory.\n",
			       DRIVER_NAME, rel_info_queue_sz);
			kfree( pdata );
			return -ENOMEM;
		}

		pdata->rel_fill  = 0;
		pdata->rel_drain = 0;
		for(i=0; i<NRELEASE_DESCRIPTORS; i++, rel++) {
		        rel->pdata = pdata;
			pdata->rel_info_queue[i] = rel;
		}

	}

	init_MUTEX       ( &pdata->release_mutex );
	init_MUTEX_LOCKED( &pdata->notify_mutex );

	filp->private_data = pdata;

	return 0;
}

static int dma_splice_dev_release(struct inode *inode, struct file *filp)
{
	struct dma_splice_info *pdata = filp->private_data;
	unsigned i = pdata->desc_drain;
	struct page *pbegin, *pend;

	/* Free any descriptor still owned by this driver */
	while(i != pdata->desc_fill) {
		rel_info_release( pdata->desc[i].rel_info );
	        i = (i+1)%NDESCRIPTORS;
	}

	kfree( pdata->rel_info_queue[0] );

	pbegin = 0;
	pend   = 0;
	for(i=0; i<pdata->npages; i++) {
		{
		  int count = atomic_read(&pdata->pages[i]->_count);
		  if (count == 1 && pbegin == 0)
		    pbegin = pdata->pages[i];
		  if (count != 1 && pbegin != 0) {
		    printk(KERN_ALERT "dma_splice_release freed pages %p-%p\n",
			   pbegin, pend);
		    pbegin = 0;
		  }
		  pend = pdata->pages[i];
		}
	        page_cache_release(pdata->pages[i]);
	}

	if (filp->private_data!=NULL)
		kfree(filp->private_data);
	
	return 0;
}

static int dma_splice_dev_compat_ioctl( struct file *filp, 
                                        unsigned int cmd,
                                        unsigned long arg)
{
   struct inode* inode = filp->f_dentry->d_inode;

   printk( KERN_INFO "dma_splice_dev_compat_ioctl() called:\n" );
   printk( KERN_INFO "\tfilp = 0x%016x (struct file*)\n", filp );
   printk( KERN_INFO "\tcmd  = %u (unsigned int)\n", cmd );
   printk( KERN_INFO "\targ  = %lu (unsigned long)\n", filp );

   return 0;
}


static int dma_splice_dev_ioctl(struct inode *inode, struct file *filp, 
				unsigned int cmd, unsigned long arg)
{
	struct dma_splice_info *pdata = filp->private_data;

        printk( KERN_INFO "dma_splic_dev_ioctl() called.\n" );
	if (cmd==DMA_SPLICE_IOCTL_INIT) {
	        int i,n,len;
		struct dma_splice_ioctl_desc desc;
	        struct dma_splice_ioctl_desc* idesc = (struct dma_splice_ioctl_desc*)arg;
		if (copy_from_user(&desc,idesc,sizeof(*idesc)))
		       return -EINVAL;

		len = ((desc.end-1) >> PAGE_SHIFT) -
		  (desc.addr >> PAGE_SHIFT) + 1;

		for(i=0; i<pdata->npages; i++) {
		       {
			 int count = atomic_read(&pdata->pages[i]->_count);
			 if (count == 1)
			   printk(KERN_ALERT "dma_splice_release frees page %p/%d\n",
				  pdata->pages[i],i);
		       }
		       page_cache_release(pdata->pages[i]);
		}
		pdata->npages = 0;

		if (len > NPAGES) {
		       printk(KERN_ALERT "dma_splice limited to %ld bytes (%ld:%ld)\n",
			      NPAGES * PAGE_SIZE, desc.addr, desc.end);
		       return -ENOMEM;
		}
		down_read(&current->mm->mmap_sem);
		n = __get_user_pages(current, current->mm, 
				     desc.addr & PAGE_MASK, len,
				     pdata->pages);
		up_read(&current->mm->mmap_sem);
		if (n == len) {
		       pdata->baseaddr = desc.addr & PAGE_MASK;
		       pdata->npages   = n;

		       /*  Make all the pages cacheable */
		       for(i=0; i<pdata->npages; i++) {
			       /* set_pages_wb(pdata->pages[i],1); */
			       /* set_memory_ro(page_address(pdata->pages[i]),1); */
		       }
		}
		else
		       printk(KERN_ALERT "dma_splice_init get_user_pages failed %d\n",n);
	}
	else if (cmd==DMA_SPLICE_IOCTL_QUEUE) {
	        struct dma_splice_desc* idesc = (struct dma_splice_desc*)arg;
		struct dma_splice_desc* desc = desc_dequeue(pdata);
		if (desc) {
		       if (copy_from_user(desc,idesc,sizeof(*idesc)))
			       return -EINVAL;
		       desc->rel_info = rel_info_get(pdata,desc->arg);
		}
		else {
		       printk(KERN_ALERT "dma_splice_queue buffer full\n");
		}
	}
	else if (cmd==DMA_SPLICE_IOCTL_NOTIFY) {
           printk( KERN_INFO "Notify ioctl called.\n" );
	        unsigned long i = pdata->notify.drain;
		while( pdata->notify.fill == i )
		       if (down_interruptible(&pdata->notify_mutex))
                       {
                          printk( KERN_INFO "down_interruptible() != 0\n" );
			        return -EINTR;
                       }
		pdata->notify.drain = (i+1)%NNOTIFY_DESCRIPTORS;
		if (copy_to_user( (void __user*)arg, &pdata->notify.arg[i], sizeof(unsigned long) )) 
                {
                   printk( KERN_INFO "copy_to_user() != 0\n" );
		       return -EINVAL;
                }
	}
	return 0;
}

static ssize_t dma_splice_dev_read(struct file *filp, loff_t *ppos, 
				   struct pipe_inode_info *pipe, size_t len, unsigned int flags)
{
	struct dma_splice_info *pdata = filp->private_data;
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages = 0,
		.ops = &dma_splice_pbuf_ops,
		.flags = flags,
		.spd_release = dummy_release,
	};

	int npbuf = PIPE_BUFFERS - pipe->nrbufs;
	unsigned i = pdata->desc_drain;

	while((len > 0) && (spd.nr_pages < npbuf) && i!=pdata->desc_fill) {
		struct dma_splice_desc *d = &pdata->desc[i];
		unsigned ipage  = (d->addr - pdata->baseaddr)>>PAGE_SHIFT;
		unsigned offset = d->addr & (PAGE_SIZE-1);
		unsigned dlen   = PAGE_SIZE - offset;
		pages  [spd.nr_pages]         = pdata->pages[ipage];
		partial[spd.nr_pages].offset  = offset;
		partial[spd.nr_pages].private = (unsigned long)d->rel_info;

		if (dlen + d->addr > d->end) 
		        dlen = d->end - d->addr;
		if (len < dlen) {
		        printk(KERN_ALERT "dma_splice_read buffer truncated %d/%d\n", len,dlen);
			dlen = len;
		}

		partial[spd.nr_pages].len = dlen;
		d->addr += dlen;
		len     -= dlen;

		/*
		** take a reference for each but the last page 
		** (last page's reference was taken in "queue" method)
		*/
		if (d->addr == d->end) {
		  /*        if (len)
			    printk(KERN_ALERT "dma_splice_read incrementing with %d bytes remaining\n", len); */
		        i = (i+1)%NDESCRIPTORS;    /* desc_enqueue */
		}
		else
			atomic_inc(&d->rel_info->_count);

		spd.nr_pages++;
	}
	pdata->desc_drain = i;
	if (spd.nr_pages == 0)
		return 0;
	return ssplice_to_pipe(pipe, &spd);
}

static int dma_splice_dev_init(void)
{
	int err;

	/* Initialize the device structure */
	dma_splice_dev.minor = MISC_DYNAMIC_MINOR;
	dma_splice_dev.name = DRIVER_NAME;
	dma_splice_dev.fops = &dma_splice_fops;

	/* Register this driver as a miscellaneous driver */
	err=misc_register(&dma_splice_dev);
	if (err < 0)
		goto err_miscdev_register;

	/* Set the driver internal data pointer */
	dev_set_drvdata(dma_splice_dev.this_device, &dma_splice_dev);

	printk(KERN_INFO "driver %s initialized successfully.\n", DRIVER_NAME);
	return 0;

	misc_deregister(&dma_splice_dev);
err_miscdev_register:
	printk(KERN_ERR "%s: Initialization failed with error %d.\n", 
		DRIVER_NAME, err);
	return err;
}
module_init(dma_splice_dev_init);

static void dma_splice_dev_exit(void)
{
	misc_deregister(&dma_splice_dev);
	printk(KERN_INFO "driver %s unregistered successfully.\n", DRIVER_NAME);
}
module_exit(dma_splice_dev_exit);
