// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/open.c $
// $Rev: 6276 $
// $Date$

#include "main.h"



//*****************************************************************************
int dev_open(dev_data_t* dev)
{
	int	ret;

	ret	= io_open(dev);

	if (ret == 0)
		ret	= gsc_dma_open(dev);

	if (ret == 0)
		ret	= gsc_irq_open(dev);

	if (ret == 0)
		ret	= initialize_ioctl(dev, NULL);

	if (ret)
		dev_close(dev);

	return(ret);
}


