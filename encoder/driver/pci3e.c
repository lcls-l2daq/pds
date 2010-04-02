/**********************************************************************
 *  pci3e.c -- US Digital Corp PCI-3E PCI Card Linux Driver
 *
 *  Copyright (C) 2005 US Digitial Corporation
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it 
 *  and/or modify it under the terms of the GNU General Public
 *  License as published bythe Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later
 *  version.
 *
 *  This program is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330,
 *  Boston, MA 02111-1307, USA.
 *  
 *  Author: Stephen Smith <steve@usdigital.com>
 *********************************************************************/

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include "pci3e.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define OLD_KERNEL 1
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
#define FILE_GET_UID(file) (file->f_uid)
#else
#define FILE_GET_UID(file) (file->f_cred->uid)
#endif

#ifdef OLD_KERNEL
#define miminor(i) MINOR((i)->i_rdev)
#else
#include <linux/device.h>
#endif

MODULE_AUTHOR("Stephen Smith <steve@usdigital.com>");
MODULE_DESCRIPTION("US Digital Generic PCI card driver");
MODULE_LICENSE("GPL");

#define PCI_VENDOR_ID_USDIGITAL (0x1892)
#define PCI_DEVICE_ID_PCI3E     (0x6294)
#define PCI_DEVICE_ID_PCI4E     (0x5747)
#define PCI3E_MODULE_NAME       "pci3e"
#define PCI3E_FIRST_MINOR       (0)

#define PCI3E_DBG(fmt...) if (debug > 0) { printk(KERN_DEBUG PCI3E_MODULE_NAME ": " fmt) ; }

#define PCI3E_TYPE(inode) ((MINOR((inode)->i_rdev)) & 0x0f)
#define PCI3E_DEV(inode)  ((MINOR((inode)->i_rdev)) >> 4)
#define PCI3E_NUM_DEVS    (4)
#define PCI3E_NO_DEV      (MKDEV(0, 0))

#define REG_TO_ADDR(reg) (pci3e->ioaddr + REG_TO_OFFSET(reg))

#define FILE_SET_DEV_NUM(file, num) (file->private_data = (void*) num)
#define FILE_GET_DEV_NUM(file)      ((int) file->private_data)

////////////////////////////////////////////////////////////////////////
// Function declarations.  All are internal to this file.

static int     pci3e_ioctl(   struct inode *inode,
                              struct file *file,
                              unsigned int cmd,
                              unsigned long arg );
static int     pci3e_open(    struct inode *inode,
                              struct file *file );
static int     pci3e_release( struct inode *inode,
                              struct file *file );
static ssize_t pci3e_read(    struct file *file,
                              char *buf,
                              size_t count,
                              loff_t *ppos );
static int     pci3e_fasync(  int fd,
                              struct file *file,
                              int mode );
static unsigned int pci3e_poll( struct file* file,
                                poll_table*  wait );

static int  __init pci3e_pci_probe(  struct pci_dev *pci_dev, 
                                     const struct pci_device_id *pci_id );
static void __exit pci3e_pci_remove( struct pci_dev *pci_dev );

static int  __init pci3e_init( void );
static void __exit pci3e_exit( void );

////////////////////////////////////////////////////////////////////////
// Structure declarations.

struct pci3e_struct {
   struct cdev cdev;
   unsigned id;
   const char *name;
   struct pci_dev *pci_dev;
   unsigned int irq;
   void __iomem *ioaddr;
   unsigned num;
   unsigned used;
   unsigned started;
   unsigned status;
   spinlock_t lock;
   wait_queue_head_t wait;
   struct fasync_struct* async_queue;
   struct pci3e_io_struct ios;
   int waitingForInterrupt;
   int interruptcount;
};

////////////////////////////////////////////////////////////////////////
// Global variables

static int debug = 1;
static dev_t dev = MKDEV(0, 0);

module_param(debug,int, 0);
MODULE_PARM_DESC(debug,"debug level: 0-3 (default=0)");

module_init(pci3e_init);
module_exit(pci3e_exit);

static struct pci_device_id pci3e_pci_tbl [] __devinitdata = {
   {PCI_VENDOR_ID_USDIGITAL, PCI_DEVICE_ID_PCI3E,  /* 1892:6294 */
    PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
   {0,}
};

MODULE_DEVICE_TABLE (pci, pci3e_pci_tbl);

static const char *card_names[] = {
   "pci3e"
};

static struct pci3e_struct *pci3e_devices[PCI3E_NUM_DEVS] = {};

DECLARE_WAIT_QUEUE_HEAD(pci3e_wait);

static struct file_operations pci3e_fops = {
   .owner   = THIS_MODULE,
   .ioctl   = pci3e_ioctl,
   .open    = pci3e_open,
   .release = pci3e_release,
   .read    = pci3e_read,
   .fasync  = pci3e_fasync,
   .poll    = pci3e_poll,
};

static struct pci_driver pci3e_pci_driver = {
   .name     = PCI3E_MODULE_NAME,
   .id_table = pci3e_pci_tbl,
   .probe    = pci3e_pci_probe,
   .remove   = pci3e_pci_remove,
};

// ---------------------------------------------------------------------
// Debug functions
// ---------------------------------------------------------------------

static int pci3e_vprintf(const char *fmt, va_list args)
{
   static char temp[4096];
   struct timeval tv;
   unsigned long len;
   char *p;
   int ret;
   if (!fmt || !(len = strlen(fmt)))
      return -1;
   do_gettimeofday(&tv);
   while (*fmt == '\n' || *fmt == '\r' || *fmt == '\t' || *fmt == ' ')
      fmt++;
   p = temp;
   len = sprintf(p, "<%c%03ld.%06ld> ",
                 in_interrupt() ? 'i' : 'p',
                 (tv.tv_sec % 1000), tv.tv_usec);
   p += len;
   ret = vsprintf(p, fmt, args);
   len += ret;
   if(temp[len-1] != '\n') {
      temp[len++] = '\n';
      temp[len]   = '\0';
   }
   return printk(KERN_DEBUG "%s",temp);
}

asmlinkage int pci3e_debug_printf(const char *fmt, ...)
{
   int ret = 0;
   va_list args;
   if(debug <= 0) return 0;
   va_start(args, fmt);
   ret = pci3e_vprintf(fmt,args);
   va_end(args);
   return ret;
}

// ---------------------------------------------------------------------
// Character drive implementation
// ---------------------------------------------------------------------

int pci3e_ioctl( struct inode *inode,
                 struct file *file,
                 unsigned int cmd,
                 unsigned long arg )
{
   int retval = 0;
   unsigned int devnum = PCI3E_DEV(inode);
   struct pci3e_struct *pci3e = NULL;
   uint32_t fifo_entry[FIFO_ENTRY_LEN];
   int i;
   unsigned int addr;

   if(devnum >= PCI3E_NUM_DEVS || !pci3e_devices[devnum])
      return -ENODEV;
		
   pci3e = pci3e_devices[devnum];

   switch ( cmd ) {
      case IOCREAD:/* for writing data to arg */
         if( copy_from_user( &pci3e->ios, (int*) arg, sizeof(pci3e->ios) ) )
            return -EFAULT;

         pci3e->ios.value = readl(pci3e->ioaddr + pci3e->ios.offset);
         PCI3E_DBG( "ioctl IOCREAD: ioaddr = 0x%p, off = %u, val = 0x%08x\n", 
                    pci3e->ioaddr,
                    (unsigned int) pci3e->ios.offset,
                    (unsigned int) pci3e->ios.value );

         if( copy_to_user( (int*) arg, &pci3e->ios, sizeof(pci3e->ios) ) )
            return -EFAULT;
         break;

      case IOCWRITE:/* for reading data from arg */
         if (copy_from_user(&pci3e->ios, (int *) arg, sizeof(pci3e->ios)))
            return -EFAULT;

         PCI3E_DBG( "ioctl IOCWRITE: ioaddr = 0x%p, off = %u, val = 0x%08x\n", 
                    pci3e->ioaddr,
                    (unsigned int) pci3e->ios.offset,
                    (unsigned int) pci3e->ios.value );

         writel(pci3e->ios.value, pci3e->ioaddr + pci3e->ios.offset);
         break;

      case IOCDISABLEIRQ:
         pci3e->waitingForInterrupt = 0;
         break;

      case IOCREADFIFO:
         PCI3E_DBG( "ioctl IOCREADFIFO.\n" );
         addr = (uint32_t) ( pci3e->ioaddr + REG_TO_OFFSET(REG_FIFO_READ) );
         for( i = 0; i < FIFO_ENTRY_LEN; ++i )
         {
            writel( JUNK, (void*) addr );
            fifo_entry[i] = readl( (void*) addr );
         }

         if( copy_to_user( (int*) arg,
                           fifo_entry,
                           sizeof(fifo_entry) ) )
            return -EFAULT;
         break;

      default:
         retval = -EINVAL;
   }

   return retval;
}

int pci3e_open( struct inode *inode,
                struct file *file )
{
   unsigned int devnum = PCI3E_DEV(inode);
   struct pci3e_struct *pci3e = NULL;

   if(devnum >= PCI3E_NUM_DEVS || !pci3e_devices[devnum])
      return -ENODEV;
		
   pci3e = pci3e_devices[devnum];

   PCI3E_DBG("open(), num=%d, used=%d, started=%d, pci3e=%p...\n",
             pci3e->num, pci3e->used, pci3e->started, pci3e);

   if(pci3e->used)
      return -EBUSY;

   FILE_SET_DEV_NUM(file, devnum);
   pci3e->started = 1;
   pci3e->used++;
   init_waitqueue_head(&pci3e->wait);
	
   return 0;
}

int pci3e_release( struct inode *inode,
                   struct file *file )
{
   unsigned int devnum = PCI3E_DEV(inode);
   struct pci3e_struct *pci3e = NULL;
    
   if(devnum >= PCI3E_NUM_DEVS || !pci3e_devices[devnum])
      return -ENODEV;

   pci3e = pci3e_devices[devnum];

   PCI3E_DBG("release, num = %d, used = %d, started = %d, pci3e->ioaddr = %p...\n",
             pci3e->num, pci3e->used, pci3e->started, pci3e->ioaddr);

   // Remove our asynchronous notification entry.
   pci3e_fasync( -1, file, 0 );

   pci3e->started = 0;
   pci3e->used--;

   FILE_SET_DEV_NUM(file, 0);
    
   return 0;
}

ssize_t pci3e_read( struct file *file,
                    char *buf,
                    size_t count,
                    loff_t *ppos )
{
   struct pci3e_struct *pci3e = NULL;

   PCI3E_DBG( "read.\n" );

   pci3e = pci3e_devices[FILE_GET_DEV_NUM(file)];
   if(!pci3e)
      return -ENODEV;

   // check if we have data - if not, sleep wake up in
   // interrupt_handler.
   // 0 = not handling interrupts
   // 1 = waitingForInterrupt
   // 2 = interrupt recieved

   if( pci3e->waitingForInterrupt == 2 )
   {
      if (copy_to_user(buf, &pci3e->ios.value, 4))
         return -EFAULT;
      return 4;
   }

   pci3e->waitingForInterrupt = 1; 

   if( file->f_flags & O_NONBLOCK )
   {
      return -EAGAIN;
   }
   
   wait_event_interruptible(pci3e_wait, pci3e->waitingForInterrupt != 1);
   pci3e->waitingForInterrupt = 0;

   if (copy_to_user(buf, &pci3e->ios.value, 4))
      return -EFAULT;

   return 4;
}

 int pci3e_fasync( int fd,
                   struct file *file,
                   int mode )
{
   int ret;
   struct pci3e_struct *pci3e = NULL;

   PCI3E_DBG( "pci3e: pci3e_fasync( %d, %p, %d ).\n", fd, file, mode );

   pci3e = pci3e_devices[FILE_GET_DEV_NUM(file)];
   if(!pci3e)
      return -ENODEV;

   ret = fasync_helper( fd, file, mode, &pci3e->async_queue );

   return ret;
}


// This will be called *twice* by select().  Once to add our wait
// queue to the poll table's wait structure, and again after our wait
// queue was notified (by an interrupt) to determine if we're actually
// readable.
unsigned int pci3e_poll( struct file* file,
                         poll_table* wait )
{
   struct pci3e_struct* pci3e = NULL;
   unsigned int poll_mask = 0;  // Not readable or writable.
   unsigned long fifo_stat = 0;

   pci3e = pci3e_devices[FILE_GET_DEV_NUM(file)];

   PCI3E_DBG( "pci3e_poll() called, waitingForInterrupt=%d.\n",
              pci3e->waitingForInterrupt );

   poll_wait( file, &pci3e_wait, wait );

   // If FIFO enabled, we are readable only if the FIFO isn't empty.
   // Else, we're readable after an interrupt latches an encoder.
   fifo_stat = readl( REG_TO_ADDR( REG_FIFO_ENABLE ) );
   if( fifo_stat )
   {
      fifo_stat = readl( REG_TO_ADDR( REG_FIFO_STAT_CTRL ) );
      if( ! ( fifo_stat & mask(FIFO_STAT_EMPTY) ) )
      {
         poll_mask |= ( POLLIN | POLLRDNORM );
      }
   }
   else
   {
      if( pci3e->waitingForInterrupt == 0 )
      {
         pci3e->waitingForInterrupt = 1;
      }
      else if( pci3e->waitingForInterrupt == 2 )
      {
         poll_mask |= ( POLLIN | POLLRDNORM );
         pci3e->waitingForInterrupt = 0;
      }
   }
   
   return poll_mask;
}


// ---------------------------------------------------------------------
// PCI interface implementation
// ---------------------------------------------------------------------

static irqreturn_t pci3e_pci_interrupt(int irq, void *dev_id)
{
   struct pci3e_struct *pci3e = (struct pci3e_struct *)dev_id;
   unsigned long ulVal = 0;

   PCI3E_DBG( "pci3e: Interrupt!  irq=%d, dev_id=0x%p\n", irq, dev_id );

   if (pci3e == NULL) {
      PCI3E_DBG( "pci3e: Unknown device, dev_id=0x%p.\n", dev_id );
      return IRQ_HANDLED;
   }

   // Increment interrupt counter.                
   pci3e->interruptcount++;

   // Determine if this is really our interrupt.
   ulVal = readl( pci3e->ioaddr + REG_TO_OFFSET( REG_INT_STAT ) );

   if( ulVal & (mask(STAT_INT_DETECTED) | mask(STAT_FIFO_HALF_FULL)) ) {
      // Clear an interrupt.
      writel( mask(STAT_INT_DETECTED),
              REG_TO_ADDR( REG_INT_STAT ) );
      ulVal = readl( REG_TO_ADDR( REG_INT_STAT ) );
      PCI3E_DBG( "pci3e: Aft clear: REG_INT_STAT=0x%08x\n",
                 (unsigned) ulVal );

      pci3e->waitingForInterrupt = 2;

      // If we have any asynchronous waiters, signal them.
      if( pci3e->async_queue ) {
         kill_fasync( &pci3e->async_queue, SIGIO, POLL_IN );
      }

      // Wakes up any blocked readers or select-/poll-ers.
      wake_up_interruptible(&pci3e_wait);
   }

   return IRQ_HANDLED;
}

int pci3e_pci_probe( struct pci_dev* pci_dev, 
                     const struct pci_device_id* pci_id )
{
   struct pci3e_struct *pci3e;
   unsigned long mem_start = 0;
   unsigned long mem_len = 0;
   int i, ret = 0;

   printk(KERN_INFO "pci3e: probe %04x:%04x irq = %d"
                         ", pci_id->driver_data = %d, %s...\n",
          pci_id->vendor, 
          pci_id->device, 
          pci_dev->irq,
          (int) pci_id->driver_data, 
          card_names[pci_id->driver_data]);

   if ( pci_enable_device(pci_dev) || pci_dev->irq == 0 )
      return -ENODEV;

   if( dev == PCI3E_NO_DEV )
      return -ENODEV;

   pci3e = kmalloc(sizeof(*pci3e), GFP_KERNEL);
   if (!pci3e) {
      ret = -ENOMEM;
      printk(KERN_NOTICE "pci3e: failed to allocate memory\n");
      goto device_error;
   }
	
   memset(pci3e, 0, sizeof(*pci3e));

   pci3e->cdev.dev = PCI3E_NO_DEV;
   cdev_init( &pci3e->cdev, &pci3e_fops );
   pci3e->cdev.owner = THIS_MODULE;
   pci3e->cdev.ops = &pci3e_fops;
   ret = cdev_add( &pci3e->cdev, dev, 1 );
   if( ret )
   {
      printk( KERN_NOTICE "Error adding cdev, err=%d.\n", ret );
      return ret;
   }

   pci3e->id      = pci_id->driver_data;
   pci3e->name    = card_names[pci3e->id];
   pci3e->pci_dev = pci_dev;
   pci3e->irq     = pci_dev->irq;
   pci3e->started = 0;
   pci3e->used    = 0;
   pci3e->async_queue = NULL;

   mem_start = pci_resource_start(pci_dev, 0);
   mem_len   = pci_resource_len(pci_dev, 0);

   spin_lock_init(&pci3e->lock);
   init_waitqueue_head(&pci3e->wait);
   pci_set_master(pci_dev);
    
   PCI3E_DBG( "pci3e: pci_request_regions, %s\n", pci3e->name );
   ret = pci_request_regions(pci_dev,(char*)pci3e->name);
   if(ret) {
      printk(KERN_ERR "pci3e: failed request regions, ret = %d.\n", ret);
      goto kmalloc_error;
   }

   if(mem_len) {
      pci3e->ioaddr = (void*)mem_start;
      PCI3E_DBG( "pci3e: before ioport_map ioaddr = 0x%p, mem_len = %u\n", 
                 pci3e->ioaddr, (unsigned int)mem_len);

      pci3e->ioaddr = pci_iomap(pci_dev, 0, mem_len);
      PCI3E_DBG( "pci3e: after ioport_map ioaddr = 0x%p\n", 
                 pci3e->ioaddr);

      if(!pci3e->ioaddr) {
         printk(KERN_ERR "pci3e: failed request_irq\n");
         ret = -EIO;
         goto release_error;
      }
   }

   ret = request_irq(pci3e->irq,
                     (irq_handler_t) &pci3e_pci_interrupt,
                     IRQF_SHARED,
                     pci3e->name, pci3e);
   if(ret) {
      printk(KERN_ERR "pci3e: failed request_irq\n");
      goto release_error;
   }

   for(i = 0 ; i < PCI3E_NUM_DEVS ; i++) {
      if(pci3e_devices[i] == NULL) {
         pci3e->num = i;
         pci3e_devices[i] = pci3e;
         break;
      }
   }
   if( i == PCI3E_NUM_DEVS ) {
      ret = -EBUSY;
      goto irq_error;
   }

   // Clear interrupt.
   writel(0x80000000, pci3e->ioaddr + 4 * 35);

   PCI3E_DBG("pci3e_pci_probe: %d pci3e->ioaddr is %p\n",
             pci3e->num, pci3e->ioaddr);
    
   pci_set_drvdata(pci_dev, pci3e);
   return 0;

   irq_error:
   free_irq(pci3e->irq, pci3e);

   release_error:
   pci_release_regions(pci_dev);

   kmalloc_error:
   kfree(pci3e);

   device_error:
   pci_disable_device(pci_dev);

   return ret;
}

void pci3e_pci_remove( struct pci_dev *pci_dev )
{
   struct pci3e_struct *pci3e = pci_get_drvdata(pci_dev);
   PCI3E_DBG("pci3e: remove %p...\n", pci3e->ioaddr);

   pci3e_devices[pci3e->num] = NULL;
   free_irq(pci3e->irq, pci3e);
   pci_release_regions(pci_dev);
   pci_disable_device(pci_dev);
   pci_set_drvdata(pci_dev, NULL);
   kfree(pci3e);

   return;
}

// ---------------------------------------------------------------------
// Linux loadable module interface.
// ---------------------------------------------------------------------

int pci3e_init(void)
{
   int err;

   printk(KERN_INFO PCI3E_MODULE_NAME ": " "US DIGITAL CORP. \n");

   // Request that the system assign us an unused major number.
   err = alloc_chrdev_region( &dev,
                              PCI3E_FIRST_MINOR,
                              PCI3E_NUM_DEVS,
                              PCI3E_MODULE_NAME );
   if( err < 0 )
   {
      printk( KERN_ERR "Unable to allocate a device major number, err=%d.\n",
                       err );
      dev = PCI3E_NO_DEV;
      return err;
   }

   if ((err = pci_register_driver(&pci3e_pci_driver)) < 0)
   {
      printk( KERN_NOTICE "pci3e: Fail: pci_register_driver() = %d.\n",
                          err );
      dev = PCI3E_NO_DEV;
      pci_unregister_driver(&pci3e_pci_driver);
      return err;
   }

   PCI3E_DBG( "%s: Assigned major number '%d'.\n",
              PCI3E_MODULE_NAME, MAJOR(dev) );

   return 0;
}

void pci3e_exit( void )
{
   int d;

   printk( KERN_INFO "pci3e: exit...\n" );

   // If we have a device number, release it to the system.
   if( dev != PCI3E_NO_DEV )
   {
      PCI3E_DBG("pci3e: Releasing device major number.\n");
      unregister_chrdev_region( dev, PCI3E_NUM_DEVS );
   }
   
   for( d = 0; d < PCI3E_NUM_DEVS; ++d )
   {
      if(    pci3e_devices[d]
          && pci3e_devices[d]->cdev.dev != PCI3E_NO_DEV )
      {
         PCI3E_DBG("pci3e: Releasing char device.\n");
         cdev_del( &pci3e_devices[d]->cdev );
      }
   }

   PCI3E_DBG("pci3e: Unregistering PCI driver.\n");
   pci_unregister_driver(&pci3e_pci_driver);
}
