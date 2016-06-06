// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_kernel_2_2.h $
// $Rev$
// $Date$

//	Provide special handling for things specific to the 2.2 kernel.

#ifndef __GSC_KERNEL_2_2_H__
#define __GSC_KERNEL_2_2_H__

#if	(LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)) && \
	(LINUX_VERSION_CODE <  KERNEL_VERSION(2,3,0))

#include <asm/spinlock.h>
#include <linux/malloc.h>
#include <linux/wrapper.h>



// #defines	*******************************************************************

#define	del_timer_sync(t)			del_timer((t))
#define	min(a,b)					(((a) < (b)) ? (a) : (b))

#define	__GFP_NOWARN				0
#define	EVENT_RESUME_IRQ(q,c)		wake_up_interruptible(q)
#define	EVENT_WAIT_IRQ_TO(q,c,t)	interruptible_sleep_on_timeout(q,t)
#define	IOCTL_32BIT_SUPPORT			GSC_IOCTL_32BIT_NATIVE
#define	IOCTL_SET_COMPAT(p,f)
#define	IOCTL_SET_UNLOCKED(p,f)
#define	JIFFIES_TIMEOUT(end)		(time_after(jiffies,(end)))
#define	MEM_ALLOC_LIMIT				(2L * 1024L * 1024)
#define	PAGE_RESERVE(vpa)			mem_map_reserve(MAP_NR((vpa)))
#define	PAGE_UNRESERVE(vpa)			mem_map_unreserve(MAP_NR((vpa)))
#define	PCI_BAR_ADDRESS(p,b)		(((p)->base_address[(int)(b)]) & ~0xFL)
#define	PCI_BAR_FLAGS(p,b)			(gsc_reg_pci_read_u16((p),0x10 + (4*(b))) & 1)
#define	PCI_BAR_SIZE(p,b)			(gsc_bar_size((p),(b)))
#define	PCI_DEVICE_LOOP(p)			for (p=pci_devices;p;p=p->next)
#define	PCI_DEVICE_SUB_SYSTEM(p)	gsc_reg_pci_read_u16((p),0x2E)
#define	PCI_DEVICE_SUB_VENDOR(p)	gsc_reg_pci_read_u16((p),0x2C)
#define	PCI_DISABLE_DEVICE(p)
#define	PCI_ENABLE_DEVICE(pci)		0
#define	PROC_GET_INFO(ptr,func)		(ptr)->get_info = (void*) (func)
#define	REGION_IO_CHECK(a,s)		check_region(a,s)
#define	REGION_IO_RELEASE(a,s)		release_region(a,s)
#define	REGION_IO_REQUEST(a,s,n)	(void*) (request_region(a,s,n), 1)
#define	REGION_MEM_CHECK(a,s)		0
#define	REGION_MEM_RELEASE(a,s)
#define	REGION_MEM_REQUEST(a,s,n)	(void*) 1
#define	REGION_TYPE_IO_BIT			1
#define	SET_CURRENT_STATE(s)		current->state = (s); mb()
#define	SET_MODULE_OWNER(p)
#define	VADDR_T						unsigned long
#define	WAIT_QUEUE_ENTRY_INIT(w,c)	(w)->task = (c)
#define	WAIT_QUEUE_ENTRY_T			struct wait_queue
#define	WAIT_QUEUE_HEAD_INIT(q)		init_waitqueue((q))
#define	WAIT_QUEUE_HEAD_T			struct wait_queue*

#define	IRQ_REQUEST(dv,fnc)			request_irq((dv)->pci->irq,	\
												(fnc),			\
												SA_SHIRQ,		\
												DEV_NAME,		\
												(dv))



// prototypes	***************************************************************

u32		gsc_bar_size(struct pci_dev* pci, u8 bar);
void	gsc_irq_isr(int irq, void* dev_id, struct pt_regs* regs);
int		gsc_proc_get_info(char* page, char** start, off_t offset, int count, int dummy);
u16		gsc_reg_pci_read_u16(struct pci_dev* pci, u8 offset);



#endif
#endif

