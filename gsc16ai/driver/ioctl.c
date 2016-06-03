// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/ioctl.c $
// $Rev$
// $Date$

#include "main.h"



//*****************************************************************************
static int _arg_s32_list_reg(
	dev_data_t*	dev,
	s32*		arg,
	const s32*	list,
	u32			reg,
	int			begin,
	int			end)
{
	int			i;
	u32			mask;
	u16			offset;
	int			ret		= -EINVAL;
	u32			v;
	VADDR_T		vaddr;

	mask	= GSC_FIELD_ENCODE(0xFFFFFFFF, begin, end);
	offset	= GSC_REG_OFFSET(reg);
	vaddr	= GSC_VADDR(dev, offset);

	if (arg[0] == -1)
	{
		ret	= 0;
		v	= readl(vaddr);
		v	&= mask;
		v	>>= end;

		for (i = 0; list[i] >= 0; i++)
		{
			if (list[i] == v)
			{
				arg[0]	= v;
				break;
			}
		}
	}
	else
	{
		for (i = 0; list[i] >= 0; i++)
		{
			if (list[i] == arg[0])
			{
				ret	= 0;
				v	= readl(vaddr);
				v	&= ~mask;
				v	|= (list[i] << end);
				writel(v, vaddr);
				break;
			}
		}
	}

	return(ret);
}



//*****************************************************************************
static int _arg_s32_range_reg(
	dev_data_t*	dev,
	s32*		arg,
	s32			min,
	s32			max,
	u32			reg,
	int			begin,
	int			end)
{
	u32			mask;
	u16			offset;
	int			ret		= -EINVAL;
	u32			v;
	VADDR_T		vaddr;

	mask	= GSC_FIELD_ENCODE(0xFFFFFFFF, begin, end);
	offset	= GSC_REG_OFFSET(reg);
	vaddr	= GSC_VADDR(dev, offset);

	if (arg[0] == -1)
	{
		ret		= 0;
		arg[0]	= readl(vaddr);
		arg[0]	&= mask;
		arg[0]	>>= end;
	}
	else if ((arg[0] >= min) && (arg[0] <= max))
	{
		ret	= 0;
		v	= readl(vaddr);
		v	&= ~mask;
		v	|= (arg[0] << end);
		writel(v, vaddr);
	}

	return(ret);
}



//*****************************************************************************
static void _icr_mod(dev_data_t* dev, u32 value, u32 mask)
{
	u32	icr;
	int	status;

	status	= gsc_irq_access_lock(dev, 0);
	icr		= readl(dev->vaddr.gsc_icr_32);
	icr		= (icr & ~mask) | (value & mask);
	writel(icr, dev->vaddr.gsc_icr_32);

	if (status == 0)
		gsc_irq_access_unlock(dev, 0);
}



//*****************************************************************************
static void _wait_ms(long ms_limit)
{
	ssize_t	limit	= jiffies + MS_TO_JIFFIES(ms_limit);

	for (;;)
	{
		if (jiffies >= limit)
		{
			// We've waited long enough.
			break;
		}

		// Wait one timer tick before checking again.
		SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
		SET_CURRENT_STATE(TASK_RUNNING);
	}
}



//*****************************************************************************
static int _wait_for_reg_field_value(
	dev_data_t*	dev,
	long		ms_limit,
	VADDR_T		vaddr,
	u32			mask,
	u32			value)
{
	ssize_t	limit;
	u32		reg;
	int		ret;

	limit	= jiffies + MS_TO_JIFFIES(ms_limit);

	for (;;)
	{
		reg	= readl(vaddr);

		if ((reg & mask) == value)
		{
			ret	= 0;
			break;
		}

		if (jiffies >= limit)
		{
			// We've waited long enough.
			ret	= -EIO;
			break;
		}

		// Wait one timer tick before checking again.
		SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
		SET_CURRENT_STATE(TASK_RUNNING);
	}

	return(ret);
}



//*****************************************************************************
static int _query(dev_data_t* dev, s32* arg)
{
	switch (arg[0])
	{
		default:							arg[0]	= AI32SSC_IOCTL_QUERY_ERROR;		break;
		case AI32SSC_QUERY_AUTO_CAL_MS:		arg[0]	= dev->cache.auto_cal_ms;			break;
		case AI32SSC_QUERY_CHANNEL_MAX:		arg[0]	= dev->cache.channels_max;			break;
		case AI32SSC_QUERY_CHANNEL_QTY:		arg[0]	= dev->cache.channel_qty;			break;
		case AI32SSC_QUERY_COUNT:			arg[0]	= AI32SSC_IOCTL_QUERY_LAST + 1;		break;
		case AI32SSC_QUERY_DEVICE_TYPE:		arg[0]	= dev->board_type;					break;
		case AI32SSC_QUERY_FGEN_MAX:		arg[0]	= dev->cache.rate_gen_fgen_max;		break;
		case AI32SSC_QUERY_FGEN_MIN:		arg[0]	= dev->cache.rate_gen_fgen_min;		break;
		case AI32SSC_QUERY_FIFO_SIZE:		arg[0]	= dev->cache.fifo_size;				break;
		case AI32SSC_QUERY_FSAMP_MAX:		arg[0]	= dev->cache.fsamp_max;				break;
		case AI32SSC_QUERY_FSAMP_MIN:		arg[0]	= dev->cache.fsamp_min;				break;
		case AI32SSC_QUERY_INIT_MS:			arg[0]	= dev->cache.initialize_ms;			break;
		case AI32SSC_QUERY_MASTER_CLOCK:	arg[0]	= dev->cache.master_clock;			break;
		case AI32SSC_QUERY_NRATE_MASK:		arg[0]	= dev->cache.rate_gen_nrate_mask;	break;
		case AI32SSC_QUERY_NRATE_MAX:		arg[0]	= dev->cache.rate_gen_nrate_max;	break;
		case AI32SSC_QUERY_NRATE_MIN:		arg[0]	= dev->cache.rate_gen_nrate_min;	break;
		case AI32SSC_QUERY_RATE_GEN_QTY:	arg[0]	= dev->cache.rate_gen_qty;			break;
		case AI32SSC_QUERY_TIME_TAG:		arg[0]	= dev->cache.time_tag_support;		break;
	}

	return(0);
}



//*****************************************************************************
int initialize_ioctl(dev_data_t* dev, void* arg)
{
	#define	BCTLR_INIT_BIT	0x8000

	long	ms;
	int		ret;

	// Manually wait for completion to insure that initialization completes
	// even if we get a signal, such as ^c.

	reg_mem32_mod(dev->vaddr.gsc_bctlr_32, BCTLR_INIT_BIT, BCTLR_INIT_BIT);
	ms	= dev->cache.initialize_ms + 5000;
	ret	= _wait_for_reg_field_value(
			dev,
			ms,
			dev->vaddr.gsc_bctlr_32,
			BCTLR_INIT_BIT,
			0);

	if (ret)
		printk("%s: INITIALIZATION TIMED OUT: %ld ms\n", dev->model, ms);

	// Initialize the software settings.
	dev->rx.bytes_per_sample	= 4;
	dev->rx.io_mode				= AI32SSC_IO_MODE_DEFAULT;
	dev->rx.overflow_check		= AI32SSC_IO_OVERFLOW_DEFAULT;
	dev->rx.pio_threshold		= 32;
	dev->rx.timeout_s			= AI32SSC_IO_TIMEOUT_DEFAULT;
	dev->rx.underflow_check		= AI32SSC_IO_UNDERFLOW_DEFAULT;
	return(ret);
}



//*****************************************************************************
static int _auto_cal_start(dev_data_t* dev, unsigned long arg)
{
	#define	AUTO_CAL_START	0x2000

	// Initiate auto-calibration.
	reg_mem32_mod(dev->vaddr.gsc_bctlr_32, AUTO_CAL_START, AUTO_CAL_START);
	return(0);
}



//*****************************************************************************
static int _auto_calibrate(dev_data_t* dev, void* arg)
{
	#define	ICR_IRQ0_AUTO_CAL_DONE	0x0001
	#define	ICR_IRQ0_MASK			0x0007
	#define	ICR_IRQ0_ACTIVE			0x0008
	#define	AUTO_CAL_PASS			0x4000

	long		ms;
	u32			reg;
	int			ret;
	gsc_sem_t	sem;
	gsc_wait_t	wt;

	// Safely select the Auto-Cal Done interrupt.
	_icr_mod(dev, ICR_IRQ0_AUTO_CAL_DONE, ICR_IRQ0_MASK);
	_icr_mod(dev, 0, ICR_IRQ0_ACTIVE);

	// Wait for the local interrupt.
	gsc_sem_create(&sem);	// dummy, required for wait operations.
	ms				= dev->cache.auto_cal_ms + 5000;
	memset(&wt, 0, sizeof(wt));
	wt.flags		= GSC_WAIT_FLAG_INTERNAL;
	wt.gsc			= AI32SSC_WAIT_GSC_AUTO_CAL_DONE;
	wt.timeout_ms	= ms;
	ret				= gsc_wait_event(dev, &wt, _auto_cal_start, (unsigned long) NULL, &sem);
	gsc_sem_destroy(&sem);

	if (wt.flags & GSC_WAIT_FLAG_TIMEOUT)
	{
		ret	= ret ? ret : -ETIMEDOUT;
		printk(	"%s: AUTO-CALIBRATION TIMED OUT (%ld ms).\n",
				dev->model,
				(long) ms);
	}

	// Check the completion status.
	reg	= readl(dev->vaddr.gsc_bctlr_32);

	if (ret)
	{
	}
	else if (reg & AUTO_CAL_START)
	{
		ret	= ret ? ret : -EIO;
		printk(	"%s: AUTO-CALIBRATION DID NOT COMPLETE (%ld ms)\n",
				dev->model,
				(long) ms);
	}
	else if ((reg & AUTO_CAL_PASS) == 0)
	{
		ret	= ret ? ret : -EIO;
		printk(	"%s: AUTO-CALIBRATION FAILED (%ld ms)\n",
				dev->model,
				(long) ms);
	}
	else
	{
		// Wait for settling.
		_wait_ms(100);
	}

	return(ret);
}



//*****************************************************************************
static int _rx_io_mode(dev_data_t* dev, void* arg)
{
	s32*	parm	= (s32*) arg;
	int		ret		= 0;

	switch (parm[0])
	{
		default:

			ret	= -EINVAL;
			break;

		case -1:

			parm[0]	= dev->rx.io_mode;
			break;

		case GSC_IO_MODE_PIO:
		case GSC_IO_MODE_DMA:
		case GSC_IO_MODE_DMDMA:

			dev->rx.io_mode	= parm[0];
			break;
	}

	return(ret);
}



//*****************************************************************************
static int _rx_io_overflow(dev_data_t* dev, void* arg)
{
	s32*	parm	= (s32*) arg;
	int		ret		= 0;

	switch (parm[0])
	{
		default:

			ret	= -EINVAL;
			break;

		case -1:

			parm[0]	= dev->rx.overflow_check;
			break;

		case AI32SSC_IO_OVERFLOW_IGNORE:
		case AI32SSC_IO_OVERFLOW_CHECK:

			dev->rx.overflow_check	= parm[0];
			break;
	}

	return(ret);
}



//*****************************************************************************
static int _rx_io_timeout(dev_data_t* dev, void* arg)
{
	s32*	parm	= (s32*) arg;
	int		ret		= 0;

	if (parm[0] == -1)
	{
		parm[0]	= dev->rx.timeout_s;
	}
	else if (	(parm[0] >= AI32SSC_IO_TIMEOUT_MIN) &&
				(parm[0] <= AI32SSC_IO_TIMEOUT_MAX))
	{
		dev->rx.timeout_s	= parm[0];
	}
	else
	{
		ret	= -EINVAL;
	}

	return(ret);
}



//*****************************************************************************
static int _rx_io_underflow(dev_data_t* dev, void* arg)
{
	s32*	parm	= (s32*) arg;
	int		ret		= 0;

	switch (parm[0])
	{
		default:

			ret	= -EINVAL;
			break;

		case -1:

			parm[0]	= dev->rx.underflow_check;
			break;

		case AI32SSC_IO_UNDERFLOW_IGNORE:
		case AI32SSC_IO_UNDERFLOW_CHECK:

			dev->rx.underflow_check	= parm[0];
			break;
	}

	return(ret);
}



//*****************************************************************************
static int _adc_clk_src(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_ADC_CLK_SRC_EXT,
		AI32SSC_ADC_CLK_SRC_RAG,
		AI32SSC_ADC_CLK_SRC_RBG,
		AI32SSC_ADC_CLK_SRC_BCR,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 4, 3);
	return(ret);
}



//*****************************************************************************
static int _adc_enable(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_ADC_ENABLE_NO,
		AI32SSC_ADC_ENABLE_YES,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 5, 5);
	return(ret);
}



//*****************************************************************************
static int _ain_buf_clear(dev_data_t* dev, void* arg)
{
	#undef	MASK
	#undef	SHIFT
	#define	MASK	(0x1L << SHIFT)
	#define	SHIFT	18

	int	ret;

	// This action clears the buffer and the over and under run flags.
	reg_mem32_mod(dev->vaddr.gsc_ibcr_32, MASK, MASK);

	// The bit should clear within 200ns.
	ret	= _wait_for_reg_field_value(dev, 2, dev->vaddr.gsc_ibcr_32, MASK, 0);
	return(ret);
}



//*****************************************************************************
static int _ain_buf_level(dev_data_t* dev, void* arg)
{
	u32*	ptr	= arg;

	ptr[0]	= readl(dev->vaddr.gsc_bufsr_32);
	ptr[0]	&= 0x7FFFF;
	return(0);
}



//*****************************************************************************
static int _ain_buf_overflow(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AIN_BUF_OVERFLOW_IGNORE,
		AI32SSC_AIN_BUF_OVERFLOW_CLEAR,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 17, 17);
	return(ret);
}



//*****************************************************************************
static int _ain_buf_thr_lvl(dev_data_t* dev, void* arg)
{
	int	ret;

	ret	= _arg_s32_range_reg(dev, arg, 0, 0x3FFFF, AI32SSC_GSC_IBCR, 17, 0);
	return(ret);
}



//*****************************************************************************
static int _ain_buf_thr_sts(dev_data_t* dev, s32* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AIN_BUF_THR_STS_CLEAR,
		AI32SSC_AIN_BUF_THR_STS_SET,
		-1	// terminate list
	};

	int	ret;

	arg[0]	= -1;
	ret		= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_IBCR, 19, 19);
	return(ret);
}



//*****************************************************************************
static int _ain_buf_underflow(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AIN_BUF_UNDERFLOW_IGNORE,
		AI32SSC_AIN_BUF_UNDERFLOW_CLEAR,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 16, 16);
	return(ret);
}



//*****************************************************************************
static int _ain_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AIN_MODE_DIFF,
		AI32SSC_AIN_MODE_ZERO,
		AI32SSC_AIN_MODE_VREF,
		-1	// terminate list
	};

	int	ret;
	s32	v	= ((s32*) arg)[0];

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 2, 0);

	if ((v == -1) && (ret == 0))
		_wait_ms(100);

	return(ret);
}



//*****************************************************************************
static int _ain_range(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AIN_RANGE_2_5V,
		AI32SSC_AIN_RANGE_5V,
		AI32SSC_AIN_RANGE_10V,
		-1	// terminate list
	};

	int	ret;
	s32	v	= ((s32*) arg)[0];

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 5, 4);

	if ((v == -1) && (ret == 0))
		_wait_ms(100);

	return(ret);
}



//*****************************************************************************
static int _burst_busy(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_BURST_BUSY_IDLE,
		AI32SSC_BURST_BUSY_ACTIVE,
		-1	// terminate list
	};

	int		ret;
	s32*	val	= (s32*) arg;

	val[0]	= -1;
	ret		= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 12, 12);
	return(ret);
}



//*****************************************************************************
static int _burst_size(dev_data_t* dev, void* arg)
{
	int	ret;

	ret	= _arg_s32_range_reg(dev, arg, 0, 0xFFFFF, AI32SSC_GSC_BRSTSR, 19, 0);
	return(ret);
}



//*****************************************************************************
static int _burst_sync(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_BURST_SYNC_DISABLE,
		AI32SSC_BURST_SYNC_RBG,
		AI32SSC_BURST_SYNC_EXT,
		AI32SSC_BURST_SYNC_BCR,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 9, 8);
	return(ret);
}



//*****************************************************************************
static int _chan_active(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_CHAN_ACTIVE_SINGLE,
		AI32SSC_CHAN_ACTIVE_0_1,
		AI32SSC_CHAN_ACTIVE_0_3,
		AI32SSC_CHAN_ACTIVE_0_7,
		AI32SSC_CHAN_ACTIVE_0_15,
		AI32SSC_CHAN_ACTIVE_0_31,
		AI32SSC_CHAN_ACTIVE_RANGE,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 2, 0);
	return(ret);
}



//*****************************************************************************
static int _chan_first(dev_data_t* dev, void* arg)
{
	u32	last;
	u32	reg;
	int	ret;

	reg		= readl(dev->vaddr.gsc_acar_32);
	last	= GSC_FIELD_DECODE(reg, 15, 8);
	ret		= _arg_s32_range_reg(dev, arg, 0, last, AI32SSC_GSC_ACAR, 7, 0);
	return(ret);
}



//*****************************************************************************
static int _chan_last(dev_data_t* dev, void* arg)
{
	u32	first;
	u32	last;
	u32	reg;
	int	ret;

	last	= dev->cache.channel_qty - 1;
	reg		= readl(dev->vaddr.gsc_acar_32);
	first	= GSC_FIELD_DECODE(reg, 7, 0);
	ret		= _arg_s32_range_reg(dev, arg, first, last, AI32SSC_GSC_ACAR, 15, 8);
	return(ret);
}



//*****************************************************************************
static int _chan_single(dev_data_t* dev, void* arg)
{
	u32	last;
	int	ret;

	last	= dev->cache.channel_qty - 1;
	ret		= _arg_s32_range_reg(dev, arg, 0, last, AI32SSC_GSC_SSCR, 17, 12);
	return(ret);
}



//*****************************************************************************
static int _data_format(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_DATA_FORMAT_2S_COMP,
		AI32SSC_DATA_FORMAT_OFF_BIN,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 6, 6);
	return(ret);
}



//*****************************************************************************
static int _data_packing(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_DATA_PACKING_DISABLE,
		AI32SSC_DATA_PACKING_ENABLE,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 18, 18);
	return(ret);
}



//*****************************************************************************
static int _input_sync(dev_data_t* dev, void* arg)
{
	#undef	MASK
	#undef	SHIFT
	#define	MASK	(0x1L << SHIFT)
	#define	SHIFT	12

	unsigned long	ms_limit;
	int				ret;

	reg_mem32_mod(dev->vaddr.gsc_bctlr_32, MASK, MASK);

	// Wait for the operation to complete.

	if (dev->rx.timeout_s)
		ms_limit	= dev->rx.timeout_s * 1000;
	else
		ms_limit	= 1000;

	ret	= _wait_for_reg_field_value(dev, ms_limit, dev->vaddr.gsc_bctlr_32, MASK, 0);
	return(ret);
}



//*****************************************************************************
static int _io_inv(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_IO_INV_LOW,
		AI32SSC_IO_INV_HIGH,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 11, 11);
	return(ret);
}



//*****************************************************************************
static int _irq0_sel(dev_data_t* dev, s32* arg)
{
	int	i;
	u32	reg;
	int	ret	= 0;

	i	= gsc_irq_local_disable(dev);
	reg	= readl(dev->vaddr.gsc_icr_32);

	if (arg[0] == -1)
	{
		// Retrieve the current setting.
		arg[0]	= GSC_FIELD_DECODE(reg, 2, 0);
	}
	else
	{
		// Validate the option value passed in.

		switch (arg[0])
		{
			default:

				ret	= -EINVAL;
				break;

			case AI32SSC_IRQ0_INIT_DONE:
			case AI32SSC_IRQ0_AUTO_CAL_DONE:
			case AI32SSC_IRQ0_SYNC_START:
			case AI32SSC_IRQ0_SYNC_DONE:
			case AI32SSC_IRQ0_BURST_START:
			case AI32SSC_IRQ0_BURST_DONE:

				break;
		}

		if (ret == 0)
		{
			// Clear the IRQ0 status bit and apply the new setting.

			if (dev->cache.fw_ver >= 0x005)
				reg	= (reg & 0xFFFFFFF0) | arg[0] | 0x80;
			else
				reg	= (reg & 0xFFFFFF70) | arg[0];

			writel(reg, dev->vaddr.gsc_icr_32);
		}
	}

	if (i == 0)
		ret	= gsc_irq_local_enable(dev);

	ret	= ret ? ret : i;
	return(ret);
}



//*****************************************************************************
static int _irq1_sel(dev_data_t* dev, s32* arg)
{
	int	i;
	u32	reg;
	int	ret	= 0;

	i	= gsc_irq_local_disable(dev);
	reg	= readl(dev->vaddr.gsc_icr_32);

	if (arg[0] == -1)
	{
		// Retrieve the current setting.
		arg[0]	= GSC_FIELD_DECODE(reg, 6, 4);
	}
	else
	{
		// Validate the option value passed in.

		switch (arg[0])
		{
			default:

				ret	= -EINVAL;
				break;

			case AI32SSC_IRQ1_NONE:
			case AI32SSC_IRQ1_IN_BUF_THR_L2H:
			case AI32SSC_IRQ1_IN_BUF_THR_H2L:
			case AI32SSC_IRQ1_IN_BUF_OVR_UNDR:

				break;
		}

		if (ret == 0)
		{
			// Clear the IRQ1 status bit and apply the new setting.

			if (dev->cache.fw_ver >= 0x005)
				reg	= (reg & 0xFFFFFF0F) | (arg[0] << 4) | 0x08;
			else
				reg	= (reg & 0xFFFFFF07) | (arg[0] << 4);

			writel(reg, dev->vaddr.gsc_icr_32);
		}
	}

	if (i == 0)
		ret	= gsc_irq_local_enable(dev);

	ret	= ret ? ret : i;
	return(ret);
}



//*****************************************************************************
static int _rag_enable(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_GEN_ENABLE_NO,
		AI32SSC_GEN_ENABLE_YES,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_RAGR, 16, 16);
	return(ret);
}



//*****************************************************************************
static int _rag_nrate(dev_data_t* dev, void* arg)
{
	int	ret;
	s32	v	= ((s32*) arg)[0];

	ret	= _arg_s32_range_reg(dev, arg, 250, 0xFFFF, AI32SSC_GSC_RAGR, 15, 0);

	if ((v == -1) && (ret == 0))
		_wait_ms(100);

	return(ret);
}



//*****************************************************************************
static int _rbg_clk_src(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_RBG_CLK_SRC_MASTER,
		AI32SSC_RBG_CLK_SRC_RAG,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_SSCR, 10, 10);
	return(ret);
}



//*****************************************************************************
static int _rbg_enable(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_GEN_ENABLE_NO,
		AI32SSC_GEN_ENABLE_YES,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_RBGR, 16, 16);
	return(ret);
}



//*****************************************************************************
static int _rbg_nrate(dev_data_t* dev, void* arg)
{
	int	ret;
	s32	v	= ((s32*) arg)[0];

	ret	= _arg_s32_range_reg(dev, arg, 250, 0xFFFF, AI32SSC_GSC_RBGR, 15, 0);

	if ((v == -1) && (ret == 0))
		_wait_ms(100);

	return(ret);
}



//*****************************************************************************
static int _scan_marker(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_SCAN_MARKER_DISABLE,
		AI32SSC_SCAN_MARKER_ENABLE,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 11, 11);
	return(ret);
}



//*****************************************************************************
static int _scan_marker_val(dev_data_t* dev, u32* arg)
{
	u32	lwr;
	u32	upr;

	if ((s32) arg[0] == (s32) -1)
	{
		upr	= readl(dev->vaddr.gsc_smuwr_32);
		lwr	= readl(dev->vaddr.gsc_smlwr_32);
		arg[0]	= ((upr & 0xFFFF) << 16) | (lwr & 0xFFFF);
	}
	else
	{
		lwr	= arg[0] & 0xFFFF;
		upr	= arg[0] >> 16;
		reg_mem32_mod(dev->vaddr.gsc_smuwr_32, upr, 0xFFFF);
		reg_mem32_mod(dev->vaddr.gsc_smlwr_32, lwr, 0xFFFF);
	}

	return(0);
}



//*****************************************************************************
static int _aux_clk_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AUX_CLK_MODE_DISABLE,
		AI32SSC_AUX_CLK_MODE_INPUT,
		AI32SSC_AUX_CLK_MODE_OUTPUT,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_ASIOCR, 1, 0);
	return(ret);
}



//*****************************************************************************
static int _aux_in_pol(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AUX_IN_POL_LO_2_HI,
		AI32SSC_AUX_IN_POL_HI_2_LO,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_ASIOCR, 8, 8);
	return(ret);
}



//*****************************************************************************
static int _aux_noise(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AUX_NOISE_LOW,
		AI32SSC_AUX_NOISE_HIGH,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_ASIOCR, 10, 10);
	return(ret);
}



//*****************************************************************************
static int _aux_out_pol(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AUX_OUT_POL_HI_PULSE,
		AI32SSC_AUX_OUT_POL_LOW_PULSE,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_ASIOCR, 9, 9);
	return(ret);
}



//*****************************************************************************
static int _aux_sync_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_AUX_SYNC_MODE_DISABLE,
		AI32SSC_AUX_SYNC_MODE_INPUT,
		AI32SSC_AUX_SYNC_MODE_OUTPUT,
		-1	// terminate list
	};

	int	ret;

	ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_ASIOCR, 3, 2);
	return(ret);
}



//*****************************************************************************
static int _tt_adc_clk_src(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_ADC_CLK_SRC_RAG,
		AI32SSC_TT_ADC_CLK_SRC_EXT_SAMP,
		AI32SSC_TT_ADC_CLK_SRC_EXT_REF,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 1, 0);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_adc_enable(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_ADC_ENABLE_NO,
		AI32SSC_TT_ADC_ENABLE_YES,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 2, 2);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_chan_mask(dev_data_t* dev, void* arg)
{
	u32		mask;
	u32*	ptr	= arg;
	int		ret;

	if (dev->cache.time_tag_support == 0)
		mask	= 0xFFFFFFFF;
	else if (dev->cache.channel_qty == 32)
		mask	= 0x00000000;
	else
		mask	= 0xFFFF0000;

	if (ptr[0] & mask)
	{
		ret	= -EINVAL;
	}
	else
	{
		ret	= 0;
		writel(ptr[0], dev->vaddr.gsc_ttacmr_32);
	}

	return(ret);
}



//*****************************************************************************
static int _tt_enable(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_ENABLE_NO,
		AI32SSC_TT_ENABLE_YES,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_BCTLR, 20, 20);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_log_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_LOG_MODE_TRIG,
		AI32SSC_TT_LOG_MODE_ALL,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 6, 6);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_ref_clk_src(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_REF_CLK_SRC_INT,
		AI32SSC_TT_REF_CLK_SRC_EXT,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 8, 8);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_ref_val_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_REF_VAL_MODE_AUTO,
		AI32SSC_TT_REF_VAL_MODE_CONST,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 5, 5);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_reset(dev_data_t* dev, void* arg)
{
	#undef	MASK
	#undef	SHIFT
	#define	MASK	(0x1L << SHIFT)
	#define	SHIFT	9

	int	ret;
	u32	v;

	if (dev->cache.time_tag_support)
	{
		ret	= 0;

		// Set the bit to do the reset.
		v	= readl(dev->vaddr.gsc_ttcr_32);
		v	&= ~MASK;
		v	|= MASK;
		writel(v, dev->vaddr.gsc_ttcr_32);

		// Clear the bit to release the reset.
		v	&= ~MASK;
		writel(v, dev->vaddr.gsc_ttcr_32);

		// Wait for settling.
		_wait_ms(100);
	}
	else
	{
		ret	= -EINVAL;
	}

	return(ret);
}



//*****************************************************************************
static int _tt_reset_ext(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_RESET_EXT_DISABLE,
		AI32SSC_TT_RESET_EXT_ENABLE,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 10, 10);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_tagging(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_TAGGING_DISABLE,
		AI32SSC_TT_TAGGING_ENABLE,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 11, 11);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_trig_mode(dev_data_t* dev, void* arg)
{
	static const s32	options[]	=
	{
		AI32SSC_TT_TRIG_MODE_CONTIN,
		AI32SSC_TT_TRIG_MODE_REF_TRIG,
		-1	// terminate list
	};

	int	ret;

	if (dev->cache.time_tag_support)
		ret	= _arg_s32_list_reg(dev, arg, options, AI32SSC_GSC_TTCR, 4, 4);
	else
		ret	= -EINVAL;

	return(ret);
}



//*****************************************************************************
static int _tt_ref_xx(dev_data_t* dev, s32* arg, int index)
{
	u32	reg;
	int	ret;

	if ((dev->cache.time_tag_support == 0)	||
		(index < 0)							||
		(index >= dev->cache.channel_qty))
	{
		ret	= -EINVAL;
	}
	else
	{
		reg	= GSC_REG_ENCODE(GSC_REG_GSC,4,0x80 + 4 * index);
		ret	= _arg_s32_range_reg(dev, arg, 0, 0xFFFF, reg, 15, 0);
	}

	return(ret);
}



//*****************************************************************************
static int _tt_ref_00(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 0);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_01(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 1);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_02(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 2);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_03(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 3);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_04(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 4);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_05(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 5);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_06(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 6);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_07(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 7);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_08(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 8);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_09(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 9);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_10(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 10);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_11(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 11);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_12(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 12);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_13(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 13);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_14(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 14);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_15(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 15);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_16(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 16);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_17(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 17);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_18(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 18);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_19(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 19);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_20(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 20);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_21(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 21);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_22(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 22);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_23(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 23);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_24(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 24);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_25(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 25);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_26(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 26);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_27(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 27);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_28(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 28);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_29(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 29);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_30(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 30);
	return(ret);
}



//*****************************************************************************
static int _tt_ref_31(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_ref_xx(dev, arg, 31);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_xx(dev_data_t* dev, s32* arg, int index)
{
	u32	reg;
	int	ret;

	if ((dev->cache.time_tag_support == 0)	||
		(index < 0)							||
		(index >= dev->cache.channel_qty))
	{
		ret	= -EINVAL;
	}
	else
	{
		reg	= GSC_REG_ENCODE(GSC_REG_GSC,4,0x80 + 4 * index);
		ret	= _arg_s32_range_reg(dev, arg, 0, 0xFFFF, reg, 31, 16);
	}

	return(ret);
}



//*****************************************************************************
static int _tt_thr_00(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 0);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_01(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 1);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_02(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 2);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_03(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 3);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_04(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 4);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_05(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 5);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_06(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 6);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_07(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 7);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_08(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 8);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_09(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 9);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_10(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 10);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_11(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 11);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_12(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 12);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_13(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 13);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_14(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 14);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_15(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 15);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_16(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 16);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_17(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 17);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_18(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 18);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_19(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 19);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_20(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 20);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_21(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 21);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_22(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 22);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_23(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 23);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_24(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 24);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_25(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 25);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_26(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 26);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_27(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 27);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_28(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 28);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_29(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 29);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_30(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 30);
	return(ret);
}



//*****************************************************************************
static int _tt_thr_31(dev_data_t* dev, void* arg)
{	int	ret;

	ret	= _tt_thr_xx(dev, arg, 31);
	return(ret);
}



//*****************************************************************************
static int _tt_nrate(dev_data_t* dev, s32* arg)
{
	int	ret;

	if (dev->cache.time_tag_support)
	{
		ret	= _arg_s32_range_reg(dev, arg, 2, 0xFFFFF, AI32SSC_GSC_TTRDR, 19, 0);
	}
	else
	{
		ret	= -EINVAL;
	}

	return(ret);
}



//*****************************************************************************
static int _rx_io_abort(dev_data_t* dev, s32* arg)
{
	arg[0]	= gsc_read_abort_active_xfer(dev);
	return(0);
}



// variables	***************************************************************

const gsc_ioctl_t	dev_ioctl_list[]	=
{
	{ AI32SSC_IOCTL_REG_READ,			(void*) gsc_reg_read_ioctl		},
	{ AI32SSC_IOCTL_REG_WRITE,			(void*) gsc_reg_write_ioctl		},
	{ AI32SSC_IOCTL_REG_MOD,			(void*) gsc_reg_mod_ioctl		},
	{ AI32SSC_IOCTL_QUERY,				(void*) _query					},
	{ AI32SSC_IOCTL_INITIALIZE,			(void*) initialize_ioctl		},
	{ AI32SSC_IOCTL_AUTO_CALIBRATE,		(void*) _auto_calibrate			},
	{ AI32SSC_IOCTL_RX_IO_MODE,			(void*) _rx_io_mode				},
	{ AI32SSC_IOCTL_RX_IO_OVERFLOW,		(void*) _rx_io_overflow			},
	{ AI32SSC_IOCTL_RX_IO_TIMEOUT,		(void*) _rx_io_timeout			},
	{ AI32SSC_IOCTL_RX_IO_UNDERFLOW,	(void*) _rx_io_underflow		},
	{ AI32SSC_IOCTL_ADC_CLK_SRC,		(void*) _adc_clk_src			},
	{ AI32SSC_IOCTL_ADC_ENABLE,			(void*) _adc_enable				},
	{ AI32SSC_IOCTL_AIN_BUF_CLEAR,		(void*) _ain_buf_clear			},
	{ AI32SSC_IOCTL_AIN_BUF_LEVEL,		(void*) _ain_buf_level			},
	{ AI32SSC_IOCTL_AIN_BUF_OVERFLOW,	(void*) _ain_buf_overflow		},
	{ AI32SSC_IOCTL_AIN_BUF_THR_LVL,	(void*) _ain_buf_thr_lvl		},
	{ AI32SSC_IOCTL_AIN_BUF_THR_STS,	(void*) _ain_buf_thr_sts		},
	{ AI32SSC_IOCTL_AIN_BUF_UNDERFLOW,	(void*) _ain_buf_underflow		},
	{ AI32SSC_IOCTL_AIN_MODE,			(void*) _ain_mode				},
	{ AI32SSC_IOCTL_AIN_RANGE,			(void*) _ain_range				},
	{ AI32SSC_IOCTL_BURST_BUSY,			(void*) _burst_busy				},
	{ AI32SSC_IOCTL_BURST_SIZE,			(void*) _burst_size				},
	{ AI32SSC_IOCTL_BURST_SYNC,			(void*) _burst_sync				},
	{ AI32SSC_IOCTL_CHAN_ACTIVE,		(void*) _chan_active			},
	{ AI32SSC_IOCTL_CHAN_FIRST,			(void*) _chan_first				},
	{ AI32SSC_IOCTL_CHAN_LAST,			(void*) _chan_last				},
	{ AI32SSC_IOCTL_CHAN_SINGLE,		(void*) _chan_single			},
	{ AI32SSC_IOCTL_DATA_FORMAT,		(void*) _data_format			},
	{ AI32SSC_IOCTL_DATA_PACKING,		(void*) _data_packing			},
	{ AI32SSC_IOCTL_INPUT_SYNC,			(void*) _input_sync				},
	{ AI32SSC_IOCTL_IO_INV,				(void*) _io_inv					},
	{ AI32SSC_IOCTL_IRQ0_SEL,			(void*) _irq0_sel				},
	{ AI32SSC_IOCTL_IRQ1_SEL,			(void*) _irq1_sel				},
	{ AI32SSC_IOCTL_RAG_ENABLE,			(void*) _rag_enable				},
	{ AI32SSC_IOCTL_RAG_NRATE,			(void*) _rag_nrate				},
	{ AI32SSC_IOCTL_RBG_CLK_SRC,		(void*) _rbg_clk_src			},
	{ AI32SSC_IOCTL_RBG_ENABLE,			(void*) _rbg_enable				},
	{ AI32SSC_IOCTL_RBG_NRATE,			(void*) _rbg_nrate				},
	{ AI32SSC_IOCTL_SCAN_MARKER,		(void*) _scan_marker			},
	{ AI32SSC_IOCTL_AUX_CLK_MODE,		(void*) _aux_clk_mode			},
	{ AI32SSC_IOCTL_AUX_IN_POL,			(void*) _aux_in_pol				},
	{ AI32SSC_IOCTL_AUX_NOISE,			(void*) _aux_noise				},
	{ AI32SSC_IOCTL_AUX_OUT_POL,		(void*) _aux_out_pol			},
	{ AI32SSC_IOCTL_AUX_SYNC_MODE,		(void*) _aux_sync_mode			},
	{ AI32SSC_IOCTL_TT_ADC_CLK_SRC,		(void*) _tt_adc_clk_src			},
	{ AI32SSC_IOCTL_TT_ADC_ENABLE,		(void*) _tt_adc_enable			},
	{ AI32SSC_IOCTL_TT_CHAN_MASK,		(void*) _tt_chan_mask			},
	{ AI32SSC_IOCTL_TT_ENABLE,			(void*) _tt_enable				},
	{ AI32SSC_IOCTL_TT_LOG_MODE,		(void*) _tt_log_mode			},
	{ AI32SSC_IOCTL_TT_REF_CLK_SRC,		(void*) _tt_ref_clk_src			},
	{ AI32SSC_IOCTL_TT_REF_VAL_MODE,	(void*) _tt_ref_val_mode		},
	{ AI32SSC_IOCTL_TT_RESET,			(void*) _tt_reset				},
	{ AI32SSC_IOCTL_TT_RESET_EXT,		(void*) _tt_reset_ext			},
	{ AI32SSC_IOCTL_TT_TAGGING,			(void*) _tt_tagging				},
	{ AI32SSC_IOCTL_TT_TRIG_MODE,		(void*) _tt_trig_mode			},
	{ AI32SSC_IOCTL_TT_REF_00,			(void*) _tt_ref_00				},
	{ AI32SSC_IOCTL_TT_REF_01,			(void*) _tt_ref_01				},
	{ AI32SSC_IOCTL_TT_REF_02,			(void*) _tt_ref_02				},
	{ AI32SSC_IOCTL_TT_REF_03,			(void*) _tt_ref_03				},
	{ AI32SSC_IOCTL_TT_REF_04,			(void*) _tt_ref_04				},
	{ AI32SSC_IOCTL_TT_REF_05,			(void*) _tt_ref_05				},
	{ AI32SSC_IOCTL_TT_REF_06,			(void*) _tt_ref_06				},
	{ AI32SSC_IOCTL_TT_REF_07,			(void*) _tt_ref_07				},
	{ AI32SSC_IOCTL_TT_REF_08,			(void*) _tt_ref_08				},
	{ AI32SSC_IOCTL_TT_REF_09,			(void*) _tt_ref_09				},
	{ AI32SSC_IOCTL_TT_REF_10,			(void*) _tt_ref_10				},
	{ AI32SSC_IOCTL_TT_REF_11,			(void*) _tt_ref_11				},
	{ AI32SSC_IOCTL_TT_REF_12,			(void*) _tt_ref_12				},
	{ AI32SSC_IOCTL_TT_REF_13,			(void*) _tt_ref_13				},
	{ AI32SSC_IOCTL_TT_REF_14,			(void*) _tt_ref_14				},
	{ AI32SSC_IOCTL_TT_REF_15,			(void*) _tt_ref_15				},
	{ AI32SSC_IOCTL_TT_REF_16,			(void*) _tt_ref_16				},
	{ AI32SSC_IOCTL_TT_REF_17,			(void*) _tt_ref_17				},
	{ AI32SSC_IOCTL_TT_REF_18,			(void*) _tt_ref_18				},
	{ AI32SSC_IOCTL_TT_REF_19,			(void*) _tt_ref_19				},
	{ AI32SSC_IOCTL_TT_REF_20,			(void*) _tt_ref_20				},
	{ AI32SSC_IOCTL_TT_REF_21,			(void*) _tt_ref_21				},
	{ AI32SSC_IOCTL_TT_REF_22,			(void*) _tt_ref_22				},
	{ AI32SSC_IOCTL_TT_REF_23,			(void*) _tt_ref_23				},
	{ AI32SSC_IOCTL_TT_REF_24,			(void*) _tt_ref_24				},
	{ AI32SSC_IOCTL_TT_REF_25,			(void*) _tt_ref_25				},
	{ AI32SSC_IOCTL_TT_REF_26,			(void*) _tt_ref_26				},
	{ AI32SSC_IOCTL_TT_REF_27,			(void*) _tt_ref_27				},
	{ AI32SSC_IOCTL_TT_REF_28,			(void*) _tt_ref_28				},
	{ AI32SSC_IOCTL_TT_REF_29,			(void*) _tt_ref_29				},
	{ AI32SSC_IOCTL_TT_REF_30,			(void*) _tt_ref_30				},
	{ AI32SSC_IOCTL_TT_REF_31,			(void*) _tt_ref_31				},
	{ AI32SSC_IOCTL_TT_THR_00,			(void*) _tt_thr_00				},
	{ AI32SSC_IOCTL_TT_THR_01,			(void*) _tt_thr_01				},
	{ AI32SSC_IOCTL_TT_THR_02,			(void*) _tt_thr_02				},
	{ AI32SSC_IOCTL_TT_THR_03,			(void*) _tt_thr_03				},
	{ AI32SSC_IOCTL_TT_THR_04,			(void*) _tt_thr_04				},
	{ AI32SSC_IOCTL_TT_THR_05,			(void*) _tt_thr_05				},
	{ AI32SSC_IOCTL_TT_THR_06,			(void*) _tt_thr_06				},
	{ AI32SSC_IOCTL_TT_THR_07,			(void*) _tt_thr_07				},
	{ AI32SSC_IOCTL_TT_THR_08,			(void*) _tt_thr_08				},
	{ AI32SSC_IOCTL_TT_THR_09,			(void*) _tt_thr_09				},
	{ AI32SSC_IOCTL_TT_THR_10,			(void*) _tt_thr_10				},
	{ AI32SSC_IOCTL_TT_THR_11,			(void*) _tt_thr_11				},
	{ AI32SSC_IOCTL_TT_THR_12,			(void*) _tt_thr_12				},
	{ AI32SSC_IOCTL_TT_THR_13,			(void*) _tt_thr_13				},
	{ AI32SSC_IOCTL_TT_THR_14,			(void*) _tt_thr_14				},
	{ AI32SSC_IOCTL_TT_THR_15,			(void*) _tt_thr_15				},
	{ AI32SSC_IOCTL_TT_THR_16,			(void*) _tt_thr_16				},
	{ AI32SSC_IOCTL_TT_THR_17,			(void*) _tt_thr_17				},
	{ AI32SSC_IOCTL_TT_THR_18,			(void*) _tt_thr_18				},
	{ AI32SSC_IOCTL_TT_THR_19,			(void*) _tt_thr_19				},
	{ AI32SSC_IOCTL_TT_THR_20,			(void*) _tt_thr_20				},
	{ AI32SSC_IOCTL_TT_THR_21,			(void*) _tt_thr_21				},
	{ AI32SSC_IOCTL_TT_THR_22,			(void*) _tt_thr_22				},
	{ AI32SSC_IOCTL_TT_THR_23,			(void*) _tt_thr_23				},
	{ AI32SSC_IOCTL_TT_THR_24,			(void*) _tt_thr_24				},
	{ AI32SSC_IOCTL_TT_THR_25,			(void*) _tt_thr_25				},
	{ AI32SSC_IOCTL_TT_THR_26,			(void*) _tt_thr_26				},
	{ AI32SSC_IOCTL_TT_THR_27,			(void*) _tt_thr_27				},
	{ AI32SSC_IOCTL_TT_THR_28,			(void*) _tt_thr_28				},
	{ AI32SSC_IOCTL_TT_THR_29,			(void*) _tt_thr_29				},
	{ AI32SSC_IOCTL_TT_THR_30,			(void*) _tt_thr_30				},
	{ AI32SSC_IOCTL_TT_THR_31,			(void*) _tt_thr_31				},
	{ AI32SSC_IOCTL_TT_NRATE,			(void*) _tt_nrate				},
	{ AI32SSC_IOCTL_RX_IO_ABORT,		(void*) _rx_io_abort			},
	{ AI32SSC_IOCTL_WAIT_EVENT,			(void*) gsc_wait_event_ioctl	},
	{ AI32SSC_IOCTL_WAIT_CANCEL,		(void*) gsc_wait_cancel_ioctl	},
	{ AI32SSC_IOCTL_WAIT_STATUS,		(void*) gsc_wait_status_ioctl	},
	{ AI32SSC_IOCTL_SCAN_MARKER_VAL,	(void*) _scan_marker_val		},
	{ -1, NULL }
};


