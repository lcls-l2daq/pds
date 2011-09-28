// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_kernel_2_6.h $
// $Rev: 5920 $
// $Date$

//	Provide special handling for things specific to the 2.6 kernel.

#ifndef __GSC_KERNEL_2_6_H__
#define __GSC_KERNEL_2_6_H__

#if	(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) && \
	(LINUX_VERSION_CODE <  KERNEL_VERSION(2,7,0))

#include <asm/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>



// #defines	*******************************************************************

#define	EVENT_RESUME_IRQ(q,c)		c = 1; wake_up_interruptible(q)
#define	EVENT_WAIT_IRQ_TO(q,c,t)	wait_event_interruptible_timeout((q)[0],(c),(t))
#define	JIFFIES_TIMEOUT(end)		(time_after(jiffies,(end)))
#define	MEM_ALLOC_LIMIT				(2L * 1024L * 1024)
#undef	MOD_DEC_USE_COUNT
#define	MOD_DEC_USE_COUNT
#undef	MOD_INC_USE_COUNT
#define	MOD_INC_USE_COUNT
#define	PAGE_RESERVE(vpa)			SetPageReserved(virt_to_page((vpa)))
#define	PAGE_UNRESERVE(vpa)			ClearPageReserved(virt_to_page((vpa)))

#define	PCI_DEVICE_LOOP(p)	\
	for ((p) = NULL; ((p) = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, (p))); )

#define	PCI_BAR_ADDRESS(p,b)		pci_resource_start((p),(b))
#define	PCI_BAR_FLAGS(p,b)			(pci_resource_flags((p),(b)))
#define	PCI_BAR_SIZE(p,b)			pci_resource_len((p),(b))
#define	PCI_DEVICE_SUB_SYSTEM(p)	((p)->subsystem_device)
#define	PCI_DEVICE_SUB_VENDOR(p)	((p)->subsystem_vendor)
#define	PCI_ENABLE_DEVICE(pci)		pci_enable_device(pci)
#define	PCI_DISABLE_DEVICE(p)		pci_disable_device((p))
#define	REGION_IO_CHECK(a,s)		0
#define	REGION_IO_RELEASE(a,s)		release_region(a,s)
#define	REGION_IO_REQUEST(a,s,n)	request_region(a,s,n)
#define	REGION_MEM_CHECK(a,s)		check_mem_region(a,s)
#define	REGION_MEM_RELEASE(a,s)		release_mem_region(a,s)
#define	REGION_MEM_REQUEST(a,s,n)	request_mem_region(a,s,n)
#define	REGION_TYPE_IO_BIT			IORESOURCE_IO
#define	SET_CURRENT_STATE(s)		current->state = (s)
#define	SET_MODULE_OWNER(p)
#define	VADDR_T						void*
#define	WAIT_QUEUE_ENTRY_INIT(w,c)	init_waitqueue_entry((w),(c))
#define	WAIT_QUEUE_ENTRY_T			wait_queue_t
#define	WAIT_QUEUE_HEAD_INIT(q)		init_waitqueue_head((q))
#define	WAIT_QUEUE_HEAD_T			wait_queue_head_t

#define  copy_from_user_ret(to, from, size, retval) {	   \
  if(access_ok(VERIFY_READ, from, size)) {		   \
    __copy_from_user(to, from, size);			   \
  }							   \
  else {						   \
    return(retval);					   \
  }							   \
}

#define  copy_to_user_ret(to, from, size, retval) {	   \
  if(access_ok(VERIFY_WRITE, to, size)) {		   \
	return(__copy_to_user(to, from, size));			   \
  }							   \
  else {						   \
    return(retval);					   \
  }							   \
}

#define put_user_ret(val, to, retval) { 		   \
  if(access_ok(VERIFY_WRITE, to, sizeof(*(to)))) {	   \
    __put_user(val, to);				   \
  }							   \
  else {						   \
    return(retval);					   \
  }							   \
 }

#define get_user_ret(val, from, retval) {		   \
  if(access_ok(VERIFY_READ, from, sizeof(*(from)))) {	   \
    __get_user(val, from);				   \
  }							   \
  else {						   \
    return(retval);					   \
  }							   \
}

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19))
	#define	IRQ_REQUEST(dv,fnc)		request_irq((dv)->pci->irq,	\
												(fnc),			\
												SA_SHIRQ,		\
												DEV_NAME,		\
												(dv))
#else
	#define	IRQ_REQUEST(dv,fnc)		request_irq((dv)->pci->irq,	\
												(fnc),			\
												IRQF_SHARED,	\
												DEV_NAME,		\
												(dv))
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18))
	#define	ISR_ARG3				, struct pt_regs* regs
#else
	#define	ISR_ARG3
#endif

#if HAVE_COMPAT_IOCTL
	#define	IOCTL_SET_COMPAT(_ptr,_func)	(_ptr)->compat_ioctl = (_func)
#else
	#define	IOCTL_SET_COMPAT(_ptr,_func)
	#include "linux/ioctl32.h"
#endif

#if HAVE_UNLOCKED_IOCTL
	#define	IOCTL_SET_UNLOCKED(_ptr,_func)	(_ptr)->unlocked_ioctl = (_func)
#else
	#define	IOCTL_SET_UNLOCKED(_ptr,_func)
#endif

#if (BITS_PER_LONG == 32)
	#define	IOCTL_32BIT_SUPPORT				GSC_IOCTL_32BIT_NATIVE
#elif (HAVE_COMPAT_IOCTL) // 64-bit OS
	#define	IOCTL_32BIT_SUPPORT				GSC_IOCTL_32BIT_COMPAT
#elif defined(CONFIG_COMPAT) // !HAVE_COMPAT_IOCTL, 64-bit OS
	#define	IOCTL_32BIT_SUPPORT				GSC_IOCTL_32BIT_TRANSLATE
#else // (!HAVE_COMPAT_IOCTL) // 64-bit OS
	#define	IOCTL_32BIT_SUPPORT				GSC_IOCTL_32BIT_NONE
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,25))
	#define	PROC_GET_INFO_SUPPORTED			1
	#define	PROC_GET_INFO(ptr,func)			(ptr)->get_info = (void*) (func)
#else
	#define	PROC_GET_INFO_SUPPORTED			0
	#define	PROC_GET_INFO(ptr,func)
#endif



// prototypes	***************************************************************

irqreturn_t		gsc_irq_isr(int irq, void* dev_id   ISR_ARG3);
int				gsc_proc_get_info(char* page, char** start, off_t offset, int count);



#endif
#endif
