/* kmemory.c
 * Linux kernel module that allow user space application
 * to allocate and use memory areas located inside the 
 * kernel memory space. The memory can then be sent to
 * other kernel subsystems using splice system call.
 * The main reason behind this module vs using vmsplice
 * is that the kernel will know when the memory area send
 * to splice can be freed thus avoiding the classic
 * memory leak associated with vmsplice.
 * 
 * Author: Remi Machet <rmachet@slac.stanford.edu>
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
#include <linux/mm.h>
#include <linux/splice.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/mman.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include "kmemory.h"

#define DRIVER_NAME KMEMORY_DEVNAME
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Remi Machet");
MODULE_DESCRIPTION("kernel memory allocator for user space");
MODULE_VERSION("1.0");

#define KMEMORY_FULL		(1<<16)
#define KMEMORY_SPLICEDIN	(1<<17)
#define KMEMORY_SPLICEDOUT	(1<<18)		/* obsolete */
#define KMEMORY_MAPPED		(1<<19)

#define NDESCRIPTORS	(256)
struct kmemory_descriptor {
	struct page *page;
	int offset;
	int offset_map;
	size_t len;
	void __user *useraddr;
	int flags;
};

struct kmemory_fd_data {
	struct kmemory_descriptor desc[NDESCRIPTORS];
	int next_desc;
	struct semaphore desc_mutex;
};

static struct miscdevice kmemory_dev;

static int kmemory_open(struct inode *inode, struct file *filp);
static int kmemory_release(struct inode *inode, struct file *filp);
static int kmemory_ioctl(struct inode *inode, struct file *filp, 
	unsigned int cmd, unsigned long arg);
static ssize_t kmemory_splice_write(struct pipe_inode_info *pipe,
	struct file *filp, loff_t *ppos, size_t len, unsigned int flags);
static ssize_t kmemory_splice_read(struct file *filp, loff_t *ppos,
	struct pipe_inode_info *pipe, size_t len, unsigned int flags);


/* The kmemory_pipe_buf_* are copies of the generic_pipe_buf* APIs 
 * in fs/pipe.c. See this file for details about those APIs.
 */
void *kmemory_pipe_buf_map(struct pipe_inode_info *pipe,
			   struct pipe_buffer *buf, int atomic)
{
	if (atomic) {
		buf->flags |= PIPE_BUF_FLAG_ATOMIC;
		return kmap_atomic(buf->page, KM_USER0);
	}

	return kmap(buf->page);
}

void kmemory_pipe_buf_unmap(struct pipe_inode_info *pipe,
			    struct pipe_buffer *buf, void *map_data)
{
	if (buf->flags & PIPE_BUF_FLAG_ATOMIC) {
		buf->flags &= ~PIPE_BUF_FLAG_ATOMIC;
		kunmap_atomic(map_data, KM_USER0);
	} else
		kunmap(buf->page);
}

int kmemory_pipe_buf_steal(struct pipe_inode_info *pipe,
			   struct pipe_buffer *buf)
{
	struct page *page = buf->page;

	/*
	 * A reference of one is golden, that means that the owner of this
	 * page is the only one holding a reference to it. lock the page
	 * and return OK.
	 */
	if (page_count(page) == 1) {
		lock_page(page);
		return 0;
	}

	return 1;
}

void kmemory_pipe_buf_get(struct pipe_inode_info *pipe, struct pipe_buffer *buf)
{
	page_cache_get(buf->page);
}

int kmemory_pipe_buf_confirm(struct pipe_inode_info *info,
			     struct pipe_buffer *buf)
{
	return 0;
}

static void kmemory_pipe_buf_release(struct pipe_inode_info *pipe,
					struct pipe_buffer *buf)
{
	page_cache_release(buf->page);
}

static const struct pipe_buf_operations kmemory_pipe_buf_ops = {
	.can_merge = 0,
	.map = kmemory_pipe_buf_map,
	.unmap = kmemory_pipe_buf_unmap,
	.confirm = kmemory_pipe_buf_confirm,
	.release = kmemory_pipe_buf_release,
	.steal = kmemory_pipe_buf_steal,
	.get = kmemory_pipe_buf_get,
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

static void __user *kmemory_map(struct page *page, size_t size)
{	
	unsigned long vaddr;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	int ret, i;

	/* Get a virtual address range */
	down_write(&mm->mmap_sem);
	vaddr = do_mmap_pgoff(NULL, 0, size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, 0);
	if (vaddr == 0)
		goto failed_vaddr;
	/* Get the memory region for later */
	vma = find_vma(mm, vaddr);
	if(vma == NULL)
		goto failed_vma;
	vma->vm_page_prot = __pgprot(pgprot_val(vma->vm_page_prot) | _PAGE_RW);
	/* split_page cannot be used in modules, if the page has a order>1
	   but isn't a compound page, we cannot handle it */
	BUG_ON((!PageCompound(page)) && (size>PAGE_SIZE));
	/* Map the virtual addresses with the pages */
	for(i = 0; i < size; i+=PAGE_SIZE, page++) {
		ret = vm_insert_page(vma, vaddr+i, page);
		if (ret < 0)
			goto failed_insert;
	}
	up_write(&mm->mmap_sem);
	return (void __user *)vaddr;

failed_insert:
failed_vma:
	do_munmap(mm, vaddr, size);
failed_vaddr:
	up_write(&mm->mmap_sem);	
	printk(KERN_ERR "Unable to map virtual memory in the current " \
			"process address space (%u bytes requested).\n", size);
	return NULL;
}

static int kmemory_unmap(void __user *useraddr, struct page *page, size_t size)
{
	struct mm_struct *mm = current->mm;
	unsigned long vaddr = (unsigned long)useraddr;
	int ret = 0;
	
	down_write(&mm->mmap_sem);
	ret = do_munmap(mm, vaddr, size);
	up_write(&mm->mmap_sem);
	return ret;
}

static int kmemory_alloc_iov(struct kmemory_fd_data *pdata, int flags,
							struct iovec *iovec)
{
	unsigned long size = iovec->iov_len;
	int order = get_order(size);
	struct page *page;

	/* Allocate memory and assign the next buffer desc to it */
	if (pdata->desc[pdata->next_desc].flags & KMEMORY_FULL) {
		printk(KERN_ERR "%s: Next buffer is not empty,extend buffer " \
			"ring size (currently %d).\n", 
			DRIVER_NAME, NDESCRIPTORS);
		return -ENOBUFS;
	}
	page = alloc_pages(GFP_KERNEL | __GFP_COMP, order);
	iovec->iov_base = kmemory_map(page, size);
	if(iovec->iov_base == NULL)
		return -ENOMEM;
	pdata->desc[pdata->next_desc].page = page;
	pdata->desc[pdata->next_desc].offset = 0;
	pdata->desc[pdata->next_desc].len = size;
	pdata->desc[pdata->next_desc].flags = flags 
						| KMEMORY_FULL | KMEMORY_MAPPED;
	pdata->desc[pdata->next_desc].useraddr = iovec->iov_base;
	pdata->desc[pdata->next_desc].offset_map = 0;
	pdata->next_desc++;
	if (pdata->next_desc == NDESCRIPTORS)
		pdata->next_desc = 0;
	return 0;
}

/* Look for the next non-empty, not-yet-mapped buffer with KMEMORY_SPLICEDIN 
	set */
static int kmemory_map_iov(struct kmemory_fd_data *pdata, int flags,
							struct iovec *iovec)
{
	int i = pdata->next_desc;
	/* We do not use KMEMORY_READ flag instead of KMEMORY_SPLICEDIN
		because RDWR buffers that are unmapped before being sent
		could cause the buffer to be immediately remapped */
	while((pdata->desc[i].flags 
			& (KMEMORY_FULL | KMEMORY_SPLICEDIN | KMEMORY_MAPPED))
			!= (KMEMORY_FULL | KMEMORY_SPLICEDIN)) {
		i++;
		if (i == NDESCRIPTORS)
			i = 0;
		if (i == pdata->next_desc)
			return -ENOBUFS;
	}
	/* map its address into user space */
	pdata->desc[i].useraddr =
		kmemory_map(pdata->desc[i].page, pdata->desc[i].len);
	if(pdata->desc[i].useraddr == NULL)
		return -ENOMEM;
	pdata->desc[i].offset_map = pdata->desc[i].offset;
	pdata->desc[i].flags |= KMEMORY_MAPPED | flags;
	iovec->iov_base = pdata->desc[i].useraddr + pdata->desc[i].offset_map;
	iovec->iov_len = pdata->desc[i].len;
	return 0;
}

/* Device access routines. The device major is the one of misc drivers
 * and if the system has udev it will be created automatically
 */

/* File operations of the misc char device */
static struct file_operations kmemory_fops = {
	.owner = THIS_MODULE,
	.open = kmemory_open,
	.release = kmemory_release,
	.ioctl = kmemory_ioctl,
	.splice_read = kmemory_splice_read,
	.splice_write = kmemory_splice_write,
};

static int kmemory_open(struct inode *inode, struct file *filp)
{
	struct kmemory_fd_data *pdata;
	int i;

	/* Allocate and initialize open handle data structure */
	pdata = kmalloc(sizeof(struct kmemory_fd_data), GFP_KERNEL);
	if (pdata == NULL) {
		printk(KERN_ERR "%s: could not allocate %d bytes of memory.\n",
			DRIVER_NAME, sizeof(struct kmemory_fd_data));
		return -ENOMEM;
	}
	init_MUTEX(&pdata->desc_mutex);
	for (i=0; i<NDESCRIPTORS; i++)
		pdata->desc[i].flags = 0;
	pdata->next_desc = 0;
	filp->private_data = pdata;

	return 0;
}

static int kmemory_release(struct inode *inode, struct file *filp)
{
	struct kmemory_fd_data *pdata = filp->private_data;
	int i;

	if(down_interruptible(&pdata->desc_mutex))
		printk(KERN_ERR "got signal while waiting for mutex in %s, " \
			"continue anyway ...\n",
			__func__);
	/* Free any descriptor still owned by this driver */
	for (i = 0; i < NDESCRIPTORS; i++) {
		if(pdata->desc[i].flags & KMEMORY_MAPPED)
			/* we do the check bellow in case the user space
				memory has already been cleaned up.
				This can happen if release is called because
				the process was killed  */
			if(page_mapcount(pdata->desc[i].page))
				kmemory_unmap(pdata->desc[i].useraddr,
					pdata->desc[i].page,
					pdata->desc[i].len);
		if(pdata->desc[i].flags & KMEMORY_FULL)
			page_cache_release(pdata->desc[i].page);
	}
	if (filp->private_data!=NULL)
		kfree(filp->private_data);
	
	return 0;
}

static int kmemory_ioctl(struct inode *inode, struct file *filp, 
	unsigned int cmd, unsigned long arg)
{
	struct kmemory_fd_data *pdata = filp->private_data;
	int err = -ENOTSUPP;

	if(down_interruptible(&pdata->desc_mutex)) {
		printk(KERN_ERR "got signal while waiting for mutex in %s.\n",
			__func__);
		return -EINTR;
	}
	switch(cmd) {
	case KMEMORY_IOCTL_MAP:
	{	struct kmemory_map_data map_data;
		int i;
		struct iovec __user *user_piovec_ptr;

		/* Fetch the user data:
		 * We first get the kmemory_map_data structure.
		 * Next we allocate enough memory to store the maximum
		 * number of struct iovec that can be returned.
		 * Last we copy the list of user space struct iovec into the 
		 * allocated space if necessary (KMEMORY_ALLOC flag present).
		 */
		err = copy_from_user(&map_data, (void *)arg, sizeof(map_data));
		if(err) {
			err = -EINVAL;
			break;
		}
		user_piovec_ptr = map_data.piovec_table;
		map_data.piovec_table = (struct iovec *)kmalloc(
				sizeof(struct iovec)*map_data.nelems,
				GFP_KERNEL);
		if (map_data.piovec_table == NULL) {
			err = -ENOMEM;
			break;
		}
		if (map_data.flags & KMEMORY_ALLOC) {
			err = copy_from_user(map_data.piovec_table, 
					user_piovec_ptr,
					sizeof(struct iovec)*map_data.nelems
				);
			if(err) {
				err = -EINVAL;
				kfree(map_data.piovec_table);
				break;
			}
		}
		/* Check flags, here are the valid combinations:
		 * KMEMORY_READ
		 * KMEMORY_READ | KMEMORY_WRITE
		 * KMEMORY_ALLOC | KMEMORY_WRITE
		 */
		if ((map_data.flags & KMEMORY_ALLOC)
			&& (map_data.flags & KMEMORY_READ)) {
			err = -EINVAL;
			kfree(map_data.piovec_table);
			break;
		}
		/* Now we fill the buffers. If KMEMORY_ALLOC is set 
		 * we allocate them and add them to the buffer list,
		 * otherwise we find a buffer that was received using
		 * splice_write
		 */
		for (i = 0, err = 0; (i < map_data.nelems) && (!err); i++) {
			if (map_data.flags & KMEMORY_ALLOC) {
				err = kmemory_alloc_iov(pdata, map_data.flags,
						&map_data.piovec_table[i]);
			} else {
				err = kmemory_map_iov(pdata, map_data.flags,
						&map_data.piovec_table[i]);
				if (err == -ENOBUFS) {
					/* No more buffers available, this is
						not an error but we must stop */
					err = 0;
					break;
				}
			}	
		}
		if (err < 0) {
			kfree(map_data.piovec_table);
			break;
		}
		map_data.nelems = i;	/* Number of buffers processed */
		if(!err) {
			err = copy_to_user((void *)user_piovec_ptr, 
				map_data.piovec_table,
				sizeof(struct iovec)*map_data.nelems
			);
		}
		kfree(map_data.piovec_table);
		err = map_data.nelems;
		break;
	}
	case KMEMORY_IOCTL_UNMAP:
	{	struct kmemory_map_data map_data;
		int i, nelems, start;
		struct iovec __user *user_piovec_ptr;

		/* Fetch the user data:
		 * We first get the kmemory_map_data structure.
		 * Next we allocate enough memory to store the maximum
		 * number of struct iovec that can be returned.
		 * Last we copy the list of user space struct iovec into the 
		 * allocated space.
		 */
		err = copy_from_user(&map_data, (void *)arg, sizeof(map_data));
		if(err) {
			err = -EINVAL;
			break;
		}
		user_piovec_ptr = map_data.piovec_table;
		map_data.piovec_table = (struct iovec *)kmalloc(
				sizeof(struct iovec)*map_data.nelems,
				GFP_KERNEL);
		if (map_data.piovec_table == NULL) {
			err = -ENOMEM;
			break;
		}
		err = copy_from_user(map_data.piovec_table, 
				user_piovec_ptr,
				sizeof(struct iovec)*map_data.nelems
			);
		if(err) {
			err = -EINVAL;
			kfree(map_data.piovec_table);
			break;
		}
		/* For each element, locate the descriptor, unmap
		 * the area and free the descriptor and page if applicable
		 */
		i = pdata->next_desc;
		for(nelems = 0, err = 0; nelems < map_data.nelems; nelems++) {
			start = i;
			while((pdata->desc[i].useraddr + pdata->desc[i].offset_map
				!= map_data.piovec_table[nelems].iov_base)
				|| (!(pdata->desc[i].flags & KMEMORY_MAPPED))) {
				i++;
				if (i == NDESCRIPTORS)
					i = 0;
				if (i == start) {
					err = -EFAULT;
					break;
				}
			}
			if (err)
				break;
			/* Unmap the area */
			err = kmemory_unmap(pdata->desc[i].useraddr,
					pdata->desc[i].page,
					pdata->desc[i].len);
			if (err)
				break;
			/* If the write flag is not set we free the buffer */
			if (!(pdata->desc[i].flags & KMEMORY_WRITE)) {
				page_cache_release(pdata->desc[i].page);
				pdata->desc[i].flags &= 
					~(KMEMORY_MAPPED | KMEMORY_FULL);
			} else {
				pdata->desc[i].flags &= ~KMEMORY_MAPPED;
			}
		}
		kfree(map_data.piovec_table);
		if (!err)
			err = nelems;
		break;
	}
	}
	up(&pdata->desc_mutex);
	return err;
}

static int pipe_to_kmemory(struct pipe_inode_info *pipe,
			    struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *filp = sd->u.file;
	struct kmemory_fd_data *pdata = filp->private_data;

	if(down_interruptible(&pdata->desc_mutex)) {
		printk(KERN_ERR "got signal while waiting for mutex in %s.\n",
			__func__);
		return -EINTR;
	}
	if (pdata->desc[pdata->next_desc].flags & KMEMORY_FULL) {
		printk(KERN_ERR "%s: Next buffer is not empty, extend buffer " \
			"ring size (currently %d).\n", 
			DRIVER_NAME, NDESCRIPTORS);
		up(&pdata->desc_mutex);
		return -ENOBUFS;
	}
	page_cache_get(buf->page);
	pdata->desc[pdata->next_desc].page = buf->page;
	pdata->desc[pdata->next_desc].offset = buf->offset;
	pdata->desc[pdata->next_desc].len = sd->len;
	pdata->desc[pdata->next_desc].flags = KMEMORY_FULL 
						| KMEMORY_SPLICEDIN 
						| KMEMORY_READ;
	pdata->next_desc++;
	if (pdata->next_desc == NDESCRIPTORS)
		pdata->next_desc = 0;
	up(&pdata->desc_mutex);
	return sd->len;
}

static ssize_t kmemory_splice_write(struct pipe_inode_info *pipe, 
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
	ret = __splice_from_pipe(pipe, &sd, pipe_to_kmemory);
	inode_double_unlock(inode, pipe->inode);

	return ret;
}

static ssize_t kmemory_splice_read(struct file *filp, loff_t *ppos, 
		struct pipe_inode_info *pipe, size_t len, unsigned int flags)
{
	struct kmemory_fd_data *pdata = filp->private_data;
	struct page *pages[PIPE_BUFFERS];
	struct partial_page partial[PIPE_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages = 0,
		.flags = flags,
		.ops = &kmemory_pipe_buf_ops,
		.spd_release = spd_release_page,
	};
	int i = pdata->next_desc;

	if(down_interruptible(&pdata->desc_mutex)) {
		printk(KERN_ERR "got signal while waiting for mutex in %s.\n",
			__func__);
		return -EINTR;
	}
	while((len > 0) && (spd.nr_pages < PIPE_BUFFERS)) {
		if ((pdata->desc[i].flags & (KMEMORY_FULL | KMEMORY_WRITE))
				== (KMEMORY_FULL | KMEMORY_WRITE)) {
			/* If the buffer is in use by user space we refuse
				to pass it on */
			if (pdata->desc[i].flags & KMEMORY_MAPPED) {
				up(&pdata->desc_mutex);
				return -EBUSY;
			}
			pages[spd.nr_pages] = pdata->desc[i].page;
			partial[spd.nr_pages].offset = pdata->desc[i].offset;
			if (len < pdata->desc[i].len) {
				partial[spd.nr_pages].len = len;
				pdata->desc[i].offset += len;
				pdata->desc[i].len -= len;
				/* we need to tell the system we still own
					part of this page */
				page_cache_get(pdata->desc[i].page);
				len = 0;
			} else {
				partial[spd.nr_pages].len = pdata->desc[i].len;
				len -= pdata->desc[i].len;
				/* This descriptor does not need to
					reference the page anymore */
				pdata->desc[i].flags &= ~KMEMORY_FULL;
			}
			spd.nr_pages++;
		}
		i++;
		if (i == NDESCRIPTORS)
			i = 0;
		if (i == pdata->next_desc)
			break;
	}
	up(&pdata->desc_mutex);
	if (spd.nr_pages == 0)
		return 0;
	return splice_to_pipe(pipe, &spd);
}

static int kmemory_init(void)
{
	int err;

	/* Initialize the device structure */
	kmemory_dev.minor = MISC_DYNAMIC_MINOR;
	kmemory_dev.name = KMEMORY_DEVNAME;
	kmemory_dev.fops = &kmemory_fops;

	/* Register this driver as a miscellaneous driver */
	err=misc_register(&kmemory_dev);
	if (err < 0)
		goto err_miscdev_register;

	/* Set the driver internal data pointer */
	dev_set_drvdata(kmemory_dev.this_device, &kmemory_dev);

	printk(KERN_INFO "driver %s initialized successfully.\n", DRIVER_NAME);
	return 0;

	misc_deregister(&kmemory_dev);
err_miscdev_register:
	printk(KERN_ERR "%s: Initialization failed with error %d.\n", 
		DRIVER_NAME, err);
	return err;
}
module_init(kmemory_init);

static void kmemory_exit(void)
{
	misc_deregister(&kmemory_dev);
	printk(KERN_INFO "driver %s unregistered successfully.\n", DRIVER_NAME);
}
module_exit(kmemory_exit);
