// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/irq.c $
// $Rev$
// $Date$

#include "main.h"



//*****************************************************************************
void  dev_irq_isr_local_handler(dev_data_t* dev)
{
	#define	ICR_IRQ0_REQUEST	0x08
	#define	ICR_IRQ1_REQUEST	0x80

	u32	icr;
	u32	irq;

	icr	= readl(dev->vaddr.gsc_icr_32);

	if (icr & ICR_IRQ0_REQUEST)
	{
		// Clear the interrupt.

		if (dev->cache.fw_ver >= 0x005)
			icr	= (icr & ~ICR_IRQ0_REQUEST) | ICR_IRQ1_REQUEST;
		else
			icr	= icr & ~(ICR_IRQ0_REQUEST | ICR_IRQ1_REQUEST);

		writel(icr, dev->vaddr.gsc_icr_32);

		// Resume any blocked threads.
		irq	= GSC_FIELD_DECODE(icr, 2, 0);

		switch (irq)
		{
			default:

				gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_SPURIOUS);
				break;

			case AI32SSC_IRQ0_INIT_DONE:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_INIT_DONE);
				break;

			case AI32SSC_IRQ0_AUTO_CAL_DONE:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_AUTO_CAL_DONE);
				break;

			case AI32SSC_IRQ0_SYNC_START:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_SYNC_START);
				break;

			case AI32SSC_IRQ0_SYNC_DONE:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_SYNC_DONE);
				break;

			case AI32SSC_IRQ0_BURST_START:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_BURST_START);
				break;

			case AI32SSC_IRQ0_BURST_DONE:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_BURST_DONE);
				break;
		}
	}
	else if (icr & ICR_IRQ1_REQUEST)
	{

		if (dev->cache.fw_ver >= 0x005)
			icr	= (icr & ~ICR_IRQ1_REQUEST) | ICR_IRQ0_REQUEST;
		else
			icr	= icr & ~(ICR_IRQ1_REQUEST | ICR_IRQ0_REQUEST);

		writel(icr, dev->vaddr.gsc_icr_32);

		// Resume any blocked threads.
		irq	= GSC_FIELD_DECODE(icr, 6, 4);

		switch (irq)
		{
			default:

				gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_SPURIOUS);
				break;

			case AI32SSC_IRQ1_IN_BUF_THR_L2H:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_IN_BUF_THR_L2H);
				break;

			case AI32SSC_IRQ1_IN_BUF_THR_H2L:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_IN_BUF_THR_H2L);
				break;

			case AI32SSC_IRQ1_IN_BUF_OVR_UNDR:

				gsc_wait_resume_irq_gsc(dev, AI32SSC_WAIT_GSC_IN_BUF_OVR_UNDR);
				break;
		}
	}
	else
	{
		// We don't know the source of the interrupt.
		gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_SPURIOUS);
	}
}


