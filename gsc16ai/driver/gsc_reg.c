// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_reg.c $
// $Rev: 3175 $
// $Date$

#include "gsc_main.h"
#include "main.h"



// data types	***************************************************************

typedef struct
{
	unsigned long		reg;
	unsigned long		type;	// Alt, PCI, PLX, GSC
	unsigned long		size;	// 1, 2, 3 or 4 bytes
	unsigned long		offset;	// Allign per size
	VADDR_T				vaddr;	// Virtual Address
	const gsc_bar_t*	bar;
} _reg_t;



// variables	***************************************************************

static const gsc_bar_t	_pci_region	=
{
	/* index		*/	0,	// not needed for PCI registers
	/* offset		*/	0,	// not needed for PCI registers
	/* reg			*/	0,	// not needed for PCI registers
	/* flags		*/	0,	// not needed for PCI registers
	/* io_mapped	*/	0,	// not needed for PCI registers
	/* phys_adrs	*/	0,	// not needed for PCI registers
	/* size			*/	256,
	/* requested	*/	0,	// not needed for PCI registers
	/* vaddr		*/	0	// not needed for PCI registers
} ;



/******************************************************************************
*
*	Function:	_reg_decode
*
*	Purpose:
*
*		Decode a register id.
*
*	Arguments:
*
*		bar		This is the BAR region where the register resides.
*
*		reg		This is the register of interest.
*
*		rt		The decoded register definition.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _reg_decode(
	const gsc_bar_t*	bar,
	unsigned long		reg,
	_reg_t*				rt)
{
	rt->reg		= reg;
	rt->bar		= bar;
	rt->type	= GSC_REG_TYPE(reg);
	rt->offset	= GSC_REG_OFFSET(reg);
	rt->size	= GSC_REG_SIZE(reg);
	rt->vaddr	= (VADDR_T) rt->offset;
	rt->vaddr	+= (unsigned long) bar->vaddr;
}



/******************************************************************************
*
*	Function:	_reg_mem_mod
*
*	Purpose:
*
*		Perform a read-modify-write operation on a memory mapped register.
*
*	Arguments:
*
*		rt		The decoded register definition.
*
*		value	The value bits to apply.
*
*		mask	The mask of bits to modify.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _reg_mem_mod(_reg_t* rt, u32 value, u32 mask)
{
	u32	v;

	switch (rt->size)
	{
		default:
		case 1:	mask	&= 0xFF;
				value	&= 0xFF & mask;
				v		= readb(rt->vaddr);
				v		&= ~mask;
				v		|= value;
				writeb(v, rt->vaddr);
				break;

		case 2:	mask	&= 0xFFFF;
				value	&= 0xFFFF & mask;
				v		= readw(rt->vaddr);
				v		&= ~mask;
				v		|= value;
				writew(v, rt->vaddr);
				break;

		case 4:	mask	&= 0xFFFFFFFF;
				value	&= 0xFFFFFFFF & mask;
				v		= readl(rt->vaddr);
				v		&= ~mask;
				v		|= value;
				writel(v, rt->vaddr);
				break;
	}
}



/******************************************************************************
*
*	Function:	_reg_mem_read
*
*	Purpose:
*
*		Read a value from a memory mapped register.
*
*	Arguments:
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _reg_mem_read(_reg_t* rt, u32* value)
{
	switch (rt->size)
	{
		default:
		case 1:	value[0]	= readb(rt->vaddr);
				break;

		case 2:	value[0]	= readw(rt->vaddr);
				break;

		case 4:	value[0]	= readl(rt->vaddr);
				break;
	}
}



/******************************************************************************
*
*	Function:	_reg_mem_write
*
*	Purpose:
*
*		Write a value to a memory mapped register.
*
*	Arguments:
*
*		rt		The decoded register definition.
*
*		value	The value to write to the register.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _reg_mem_write(_reg_t* rt, u32 value)
{
	switch (rt->size)
	{
		default:
		case 1:	writeb(value, rt->vaddr);
				break;

		case 2:	writew(value, rt->vaddr);
				break;

		case 4:	writel(value, rt->vaddr);
				break;
	}
}



/******************************************************************************
*
*	Function:	_reg_pci_read_1
*
*	Purpose:
*
*		Read a PCI register that is one byte long.
*
*	Arguments:
*
*		pci		The PCI structure for the device to access.
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		0		All went well.
*		< 0		An error code for the problem seen.
*
******************************************************************************/

static int _reg_pci_read_1(struct pci_dev* pci, _reg_t* rt, u32* value)
{
	int	ret;
	u8	v;

	if (rt->offset <= 255)
	{
		pci_read_config_byte(pci, rt->offset, &v);
		value[0]	= v;
		ret			= 0;
	}
	else
	{
		value[0]	= 0;
		ret			= -EINVAL;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	_reg_pci_read_2
*
*	Purpose:
*
*		Read a PCI register that is two bytes long. Any alignment is supported.
*
*	Arguments:
*
*		pci		The PCI structure for the device to access.
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		0		All went well.
*		< 0		An error code for the problem seen.
*
******************************************************************************/

static int _reg_pci_read_2(struct pci_dev* pci, _reg_t* rt, u32* value)
{
	int	ret;
	u16	v1;
	u16	v2;

	if (((rt->offset & 1) == 0) && (rt->offset <= 254))
	{
		pci_read_config_word(pci, rt->offset, &v1);
		value[0]	= v1;
		ret			= 0;
	}
	else if (((rt->offset & 1) == 1) && (rt->offset <= 253))
	{
		pci_read_config_word(pci, rt->offset - 1, &v1);
		pci_read_config_word(pci, rt->offset + 1, &v2);
		value[0]	= ((v2 & 0x00FF) << 8) | ((v1 & 0xFF00) >> 8);
		ret			= 0;
	}
	else
	{
		value[0]	= 0;
		ret			= -EINVAL;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	_reg_pci_read_3
*
*	Purpose:
*
*		Read a PCI register that is three bytes long. Any alignment is
*		supported.
*
*	Arguments:
*
*		pci		The PCI structure for the device to access.
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		0		All went well.
*		< 0		An error code for the problem seen.
*
******************************************************************************/

static int _reg_pci_read_3(struct pci_dev* pci, _reg_t* rt, u32* value)
{
	int	ret;
	u32	v1;
	u32	v2;

	if (((rt->offset & 3) == 0) && (rt->offset <= 252))
	{
		pci_read_config_dword(pci, rt->offset, &v1);
		value[0]	= v1 & 0x00FFFFFF;
		ret			= 0;
	}
	else if (((rt->offset & 3) == 1) && (rt->offset <= 252))
	{
		pci_read_config_dword(pci, rt->offset - 1, &v1);
		value[0]	= (v1 & 0xFFFFFF00) >> 8;
		ret			= 0;
	}
	else if (((rt->offset & 3) == 2) && (rt->offset <= 248))
	{
		pci_read_config_dword(pci, rt->offset - 2, &v1);
		pci_read_config_dword(pci, rt->offset + 2, &v2);
		value[0]	= ((v1 & 0xFFFF0000) >> 16) | ((v2 & 0x000000FF) << 16);
		ret			= 0;
	}
	else if (((rt->offset & 3) == 3) && (rt->offset <= 248))
	{
		pci_read_config_dword(pci, rt->offset - 3, &v1);
		pci_read_config_dword(pci, rt->offset + 1, &v2);
		value[0]	= ((v1 & 0xFF000000) >> 24) | ((v2 & 0x0000FFFF) << 8);
		ret			= 0;
	}
	else
	{
		value[0]	= 0;
		ret			= -EINVAL;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	_reg_pci_read_4
*
*	Purpose:
*
*		Read a PCI register that is four bytes long. Any alignment is
*		supported.
*
*	Arguments:
*
*		pci		The PCI structure for the device to access.
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		0		All went well.
*		< 0		An error code for the problem seen.
*
******************************************************************************/

static int _reg_pci_read_4(struct pci_dev* pci, _reg_t* rt, u32* value)
{
	int	ret;
	u32	v1;
	u32	v2;

	if (((rt->offset & 3) == 0) && (rt->offset <= 252))
	{
		pci_read_config_dword(pci, rt->offset,&v1);
		value[0]	= v1;
		ret			= 0;
	}
	else if (((rt->offset & 3) == 1) && (rt->offset <= 248))
	{
		pci_read_config_dword(pci, rt->offset - 1, &v1);
		pci_read_config_dword(pci, rt->offset + 3, &v2);
		value[0]	= ((v1 & 0xFFFFFF00) >> 8) | ((v2 & 0x000000FF) << 24);
		ret			= 0;
	}
	else if (((rt->offset & 3) == 2) && (rt->offset <= 248))
	{
		pci_read_config_dword(pci, rt->offset - 2, &v1);
		pci_read_config_dword(pci, rt->offset + 2, &v2);
		value[0]	= ((v1 & 0xFFFF0000) >> 16) | ((v2 & 0x0000FFFF) << 16);
		ret			= 0;
	}
	else if (((rt->offset & 3) == 3) && (rt->offset <= 248))
	{
		pci_read_config_dword(pci, rt->offset - 3, &v1);
		pci_read_config_dword(pci, rt->offset + 1, &v2);
		value[0]	= ((v1 & 0xFF000000) >> 24) | ((v2 & 0x00FFFFFF) << 8);
		ret			= 0;
	}
	else
	{
		value[0]	= 0;
		ret			= -EINVAL;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	_reg_pci_read
*
*	Purpose:
*
*		Read a value from a PCI register.
*
*	Arguments:
*
*		pci		The PCI structure for the device to access.
*
*		rt		The decoded register definition.
*
*		value	The value read is recorded here.
*
*	Returned:
*
*		0		All went well.
*		< 0		The code for the problem encounterred.
*
******************************************************************************/

static int _reg_pci_read(struct pci_dev* pci, _reg_t* rt, u32* value)
{
	int	ret;

	switch (rt->size)
	{
		default:	ret		= -EINVAL;
					break;

		case 1:		ret	= _reg_pci_read_1(pci, rt, value);	break;
		case 2:		ret	= _reg_pci_read_2(pci, rt, value);	break;
		case 3:		ret	= _reg_pci_read_3(pci, rt, value);	break;
		case 4:		ret	= _reg_pci_read_4(pci, rt, value);	break;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	_reg_validate
*
*	Purpose:
*
*		Verify that a regiter id is valid.
*
*	Arguments:
*
*		rt		The decoded register definition.
*
*	Returned:
*
*		>= 0	The number of errors seen.
*
******************************************************************************/

static int _reg_validate(_reg_t* rt)
{
	int				errs	= 1;
	unsigned long	ul;

	for (;;)	// We'll use a loop for convenience.
	{
		// Are there extranious bits in the register id?
		ul	= GSC_REG_ENCODE(rt->type, rt->size, rt->offset);

		if (ul != rt->reg)
			break;

		// Does the register extend past the end of the region?
		ul	= rt->offset + rt->size - 1;

		if (ul >= rt->bar->size)
			break;

		// Is the register's size valid?

		if (strchr("\001\002\003\004", (int) rt->size) == NULL)
			break;

		// Is the register properly aligned?

		if ((rt->size == 2) && (rt->offset & 0x1))
			break;
		else if ((rt->size == 4) && (rt->offset & 0x3))
			break;

		//	We don't test the "type" since that is validated
		//	before the register is decoded.

		errs	= 0;
		break;
	}

	return(errs);
}



/******************************************************************************
*
*	Function:	gsc_reg_mod_ioctl
*
*	Purpose:
*
*		Implement the Register Modify (read-modify-write) IOCTL service.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		arg		The IOCTL service's required argument.
*
*	Returned:
*
*		0		All went well.
*		< 0		There was a problem that the value reflects.
*
******************************************************************************/

int gsc_reg_mod_ioctl(dev_data_t* dev, gsc_reg_t* arg)
{
	_reg_t			rt;
	int				errs;
	int				ret;
	unsigned long	type	= GSC_REG_TYPE(arg->reg);


	switch (type)
	{
		default:
		case GSC_REG_PCI:	// READ ONLY!
		case GSC_REG_PLX:	// READ ONLY!

			ret	= -EINVAL;
			break;

		case GSC_REG_ALT:

			ret	= dev_reg_mod_alt(dev, arg);
			break;

		case GSC_REG_GSC:

			_reg_decode(&dev->gsc, arg->reg, &rt);
			errs	= _reg_validate(&rt);

			if (errs == 0)
				_reg_mem_mod(&rt, arg->value, arg->mask);

			ret	= errs ? -EINVAL : 0;
			break;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_reg_read_ioctl
*
*	Purpose:
*
*		Implement the Register Read IOCTL service.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		arg		The IOCTL service's required argument.
*
*	Returned:
*
*		0		All went well.
*		< 0		There was a problem that the value reflects.
*
******************************************************************************/

int gsc_reg_read_ioctl(dev_data_t* dev, gsc_reg_t* arg)
{
	_reg_t			rt;
	int				errs;
	int				ret;
	unsigned long	type	= GSC_REG_TYPE(arg->reg);

	switch (type)
	{
		default:

			ret	= -EINVAL;
			break;

		case GSC_REG_ALT:

			ret	= dev_reg_read_alt(dev, arg);
			break;

		case GSC_REG_GSC:

			_reg_decode(&dev->gsc, arg->reg, &rt);
			errs	= _reg_validate(&rt);

			if (errs == 0)
				_reg_mem_read(&rt, &arg->value);

			ret	= errs ? -EINVAL : 0;
			break;

		case GSC_REG_PCI:

			_reg_decode(&_pci_region, arg->reg, &rt);
			errs	= _reg_validate(&rt);

			if (errs == 0)
				errs	= _reg_pci_read(dev->pci, &rt, &arg->value);

			ret	= errs ? -EINVAL : 0;
			break;

		case GSC_REG_PLX:

			_reg_decode(&dev->plx, arg->reg, &rt);
			errs	= _reg_validate(&rt);

			if (errs == 0)
				_reg_mem_read(&rt, &arg->value);

			ret	= errs ? -EINVAL : 0;
			break;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_reg_write_ioctl
*
*	Purpose:
*
*		Implement the Register Write IOCTL service.
*
*	Arguments:
*
*		dev	The data for the device of interest.
*
*		arg	The IOCTL service's required argument.
*
*	Returned:
*
*		0	All went well.
*		< 0	There was a problem that the value reflects.
*
******************************************************************************/

int gsc_reg_write_ioctl(dev_data_t* dev, gsc_reg_t* arg)
{
	_reg_t			rt;
	int				errs;
	int				ret;
	unsigned long	type	= GSC_REG_TYPE(arg->reg);

	switch (type)
	{
		default:
		case GSC_REG_PCI:	// READ ONLY!
		case GSC_REG_PLX:	// READ ONLY!

			ret	= -EINVAL;
			break;

		case GSC_REG_ALT:

			ret	= dev_reg_write_alt(dev, arg);
			break;

		case GSC_REG_GSC:

			_reg_decode(&dev->gsc, arg->reg, &rt);
			errs	= _reg_validate(&rt);

			if (errs == 0)
				_reg_mem_write(&rt, arg->value);

			ret	= errs ? -EINVAL : 0;
			break;
	}

	return(ret);
}


