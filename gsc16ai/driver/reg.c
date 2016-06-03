// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/reg.c $
// $Rev$
// $Date$

#include "main.h"



//*****************************************************************************
int dev_reg_mod_alt(dev_data_t* dev, gsc_reg_t* arg)
{
	// No alternate register encodings are defined for this driver.
	return(-EINVAL);
}



//*****************************************************************************
int dev_reg_read_alt(dev_data_t* dev, gsc_reg_t* arg)
{
	// No alternate register encodings are defined for this driver.
	return(-EINVAL);
}



//*****************************************************************************
int dev_reg_write_alt(dev_data_t* dev, gsc_reg_t* arg)
{
	// No alternate register encodings are defined for this driver.
	return(-EINVAL);
}



//*****************************************************************************
void reg_mem32_mod(VADDR_T vaddr, u32 value, u32 mask)
{
	u32	v;

	v	= readl(vaddr);
	v	&= ~mask;
	v	|= (value & mask);
	writel(v, vaddr);
}


