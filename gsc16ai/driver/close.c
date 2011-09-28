// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/close.c $
// $Rev: 5857 $
// $Date$

#include "main.h"



//*****************************************************************************
int dev_close(dev_data_t* dev)
{
	// Clear and disable all interrupts.
	writel(0, dev->vaddr.gsc_icr_32);

	gsc_dma_close(dev);
	io_close(dev);
	gsc_irq_close(dev);
	return(0);
}



