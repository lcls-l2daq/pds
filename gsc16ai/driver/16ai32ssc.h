// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/16ai32ssc.h $
// $Rev: 5987 $
// $Date$

#ifndef __16AI32SSC_H__
#define __16AI32SSC_H__

#include <asm/types.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#include "gsc_common.h"
#include "gsc_pci9056.h"



// #defines	*******************************************************************

#define	AI32SSC_BASE_NAME							"16ai32ssc"
#define	AI32SSC_DEV_BASE_NAME						"/dev/" AI32SSC_BASE_NAME



// IOCTL command codes
#define	AI32SSC_IOCTL_REG_READ			_IOWR(GSC_IOCTL,   0, gsc_reg_t)
#define	AI32SSC_IOCTL_REG_WRITE			_IOWR(GSC_IOCTL,   1, gsc_reg_t)
#define	AI32SSC_IOCTL_REG_MOD			_IOWR(GSC_IOCTL,   2, gsc_reg_t)
#define AI32SSC_IOCTL_QUERY				_IOWR(GSC_IOCTL,   3, __s32)
#define AI32SSC_IOCTL_INITIALIZE		_IO  (GSC_IOCTL,   4)
#define AI32SSC_IOCTL_AUTO_CALIBRATE	_IO  (GSC_IOCTL,   5)
#define AI32SSC_IOCTL_RX_IO_MODE		_IOWR(GSC_IOCTL,   6, __s32)
#define AI32SSC_IOCTL_RX_IO_OVERFLOW	_IOWR(GSC_IOCTL,   7, __s32)
#define AI32SSC_IOCTL_RX_IO_TIMEOUT		_IOWR(GSC_IOCTL,   8, __s32)
#define AI32SSC_IOCTL_RX_IO_UNDERFLOW	_IOWR(GSC_IOCTL,   9, __s32)
#define AI32SSC_IOCTL_ADC_CLK_SRC		_IOWR(GSC_IOCTL,  10, __s32)
#define AI32SSC_IOCTL_ADC_ENABLE		_IOWR(GSC_IOCTL,  11, __s32)
#define AI32SSC_IOCTL_AIN_BUF_CLEAR		_IO  (GSC_IOCTL,  12)
#define AI32SSC_IOCTL_AIN_BUF_LEVEL		_IOR (GSC_IOCTL,  13, __s32)
#define AI32SSC_IOCTL_AIN_BUF_OVERFLOW	_IOWR(GSC_IOCTL,  14, __s32)
#define AI32SSC_IOCTL_AIN_BUF_THR_LVL	_IOWR(GSC_IOCTL,  15, __s32)
#define AI32SSC_IOCTL_AIN_BUF_THR_STS	_IOR (GSC_IOCTL,  16, __s32)
#define AI32SSC_IOCTL_AIN_BUF_UNDERFLOW	_IOWR(GSC_IOCTL,  17, __s32)
#define AI32SSC_IOCTL_AIN_MODE			_IOWR(GSC_IOCTL,  18, __s32)
#define AI32SSC_IOCTL_AIN_RANGE			_IOWR(GSC_IOCTL,  19, __s32)
#define AI32SSC_IOCTL_BURST_BUSY		_IOR (GSC_IOCTL,  20, __s32)
#define AI32SSC_IOCTL_BURST_SIZE		_IOWR(GSC_IOCTL,  21, __s32)
#define AI32SSC_IOCTL_BURST_SYNC		_IOWR(GSC_IOCTL,  22, __s32)
#define AI32SSC_IOCTL_CHAN_ACTIVE		_IOWR(GSC_IOCTL,  23, __s32)
#define AI32SSC_IOCTL_CHAN_FIRST		_IOWR(GSC_IOCTL,  24, __s32)
#define AI32SSC_IOCTL_CHAN_LAST			_IOWR(GSC_IOCTL,  25, __s32)
#define AI32SSC_IOCTL_CHAN_SINGLE		_IOWR(GSC_IOCTL,  26, __s32)
#define AI32SSC_IOCTL_DATA_FORMAT		_IOWR(GSC_IOCTL,  27, __s32)
#define AI32SSC_IOCTL_DATA_PACKING		_IOWR(GSC_IOCTL,  28, __s32)
#define AI32SSC_IOCTL_INPUT_SYNC		_IO  (GSC_IOCTL,  29)
#define AI32SSC_IOCTL_IO_INV			_IOWR(GSC_IOCTL,  30, __s32)
#define AI32SSC_IOCTL_IRQ0_SEL			_IOWR(GSC_IOCTL,  31, __s32)
#define AI32SSC_IOCTL_IRQ1_SEL			_IOWR(GSC_IOCTL,  32, __s32)
#define AI32SSC_IOCTL_RAG_ENABLE		_IOWR(GSC_IOCTL,  33, __s32)
#define AI32SSC_IOCTL_RAG_NRATE			_IOWR(GSC_IOCTL,  34, __s32)
#define AI32SSC_IOCTL_RBG_CLK_SRC		_IOWR(GSC_IOCTL,  35, __s32)
#define AI32SSC_IOCTL_RBG_ENABLE		_IOWR(GSC_IOCTL,  36, __s32)
#define AI32SSC_IOCTL_RBG_NRATE			_IOWR(GSC_IOCTL,  37, __s32)
#define AI32SSC_IOCTL_SCAN_MARKER		_IOWR(GSC_IOCTL,  38, __s32)
#define AI32SSC_IOCTL_AUX_CLK_MODE		_IOWR(GSC_IOCTL,  39, __s32)
#define AI32SSC_IOCTL_AUX_IN_POL		_IOWR(GSC_IOCTL,  40, __s32)
#define AI32SSC_IOCTL_AUX_NOISE			_IOWR(GSC_IOCTL,  41, __s32)
#define AI32SSC_IOCTL_AUX_OUT_POL		_IOWR(GSC_IOCTL,  42, __s32)
#define AI32SSC_IOCTL_AUX_SYNC_MODE		_IOWR(GSC_IOCTL,  43, __s32)
#define AI32SSC_IOCTL_TT_ADC_CLK_SRC	_IOWR(GSC_IOCTL,  44, __s32)
#define AI32SSC_IOCTL_TT_ADC_ENABLE		_IOWR(GSC_IOCTL,  45, __s32)
#define AI32SSC_IOCTL_TT_CHAN_MASK		_IOW (GSC_IOCTL,  46, __u32)
#define AI32SSC_IOCTL_TT_ENABLE			_IOWR(GSC_IOCTL,  47, __s32)
#define AI32SSC_IOCTL_TT_LOG_MODE		_IOWR(GSC_IOCTL,  48, __s32)
#define AI32SSC_IOCTL_TT_REF_CLK_SRC	_IOWR(GSC_IOCTL,  49, __s32)
#define AI32SSC_IOCTL_TT_REF_VAL_MODE	_IOWR(GSC_IOCTL,  50, __s32)
#define AI32SSC_IOCTL_TT_RESET			_IO  (GSC_IOCTL,  51)
#define AI32SSC_IOCTL_TT_RESET_EXT		_IOWR(GSC_IOCTL,  52, __s32)
#define AI32SSC_IOCTL_TT_TAGGING		_IOWR(GSC_IOCTL,  53, __s32)
#define AI32SSC_IOCTL_TT_TRIG_MODE		_IOWR(GSC_IOCTL,  54, __s32)
#define AI32SSC_IOCTL_TT_REF_00			_IOWR(GSC_IOCTL,  55, __s32)
#define AI32SSC_IOCTL_TT_REF_01			_IOWR(GSC_IOCTL,  56, __s32)
#define AI32SSC_IOCTL_TT_REF_02			_IOWR(GSC_IOCTL,  57, __s32)
#define AI32SSC_IOCTL_TT_REF_03			_IOWR(GSC_IOCTL,  58, __s32)
#define AI32SSC_IOCTL_TT_REF_04			_IOWR(GSC_IOCTL,  59, __s32)
#define AI32SSC_IOCTL_TT_REF_05			_IOWR(GSC_IOCTL,  60, __s32)
#define AI32SSC_IOCTL_TT_REF_06			_IOWR(GSC_IOCTL,  61, __s32)
#define AI32SSC_IOCTL_TT_REF_07			_IOWR(GSC_IOCTL,  62, __s32)
#define AI32SSC_IOCTL_TT_REF_08			_IOWR(GSC_IOCTL,  63, __s32)
#define AI32SSC_IOCTL_TT_REF_09			_IOWR(GSC_IOCTL,  64, __s32)
#define AI32SSC_IOCTL_TT_REF_10			_IOWR(GSC_IOCTL,  65, __s32)
#define AI32SSC_IOCTL_TT_REF_11			_IOWR(GSC_IOCTL,  66, __s32)
#define AI32SSC_IOCTL_TT_REF_12			_IOWR(GSC_IOCTL,  67, __s32)
#define AI32SSC_IOCTL_TT_REF_13			_IOWR(GSC_IOCTL,  68, __s32)
#define AI32SSC_IOCTL_TT_REF_14			_IOWR(GSC_IOCTL,  69, __s32)
#define AI32SSC_IOCTL_TT_REF_15			_IOWR(GSC_IOCTL,  70, __s32)
#define AI32SSC_IOCTL_TT_REF_16			_IOWR(GSC_IOCTL,  71, __s32)
#define AI32SSC_IOCTL_TT_REF_17			_IOWR(GSC_IOCTL,  72, __s32)
#define AI32SSC_IOCTL_TT_REF_18			_IOWR(GSC_IOCTL,  73, __s32)
#define AI32SSC_IOCTL_TT_REF_19			_IOWR(GSC_IOCTL,  74, __s32)
#define AI32SSC_IOCTL_TT_REF_20			_IOWR(GSC_IOCTL,  75, __s32)
#define AI32SSC_IOCTL_TT_REF_21			_IOWR(GSC_IOCTL,  76, __s32)
#define AI32SSC_IOCTL_TT_REF_22			_IOWR(GSC_IOCTL,  77, __s32)
#define AI32SSC_IOCTL_TT_REF_23			_IOWR(GSC_IOCTL,  78, __s32)
#define AI32SSC_IOCTL_TT_REF_24			_IOWR(GSC_IOCTL,  79, __s32)
#define AI32SSC_IOCTL_TT_REF_25			_IOWR(GSC_IOCTL,  80, __s32)
#define AI32SSC_IOCTL_TT_REF_26			_IOWR(GSC_IOCTL,  81, __s32)
#define AI32SSC_IOCTL_TT_REF_27			_IOWR(GSC_IOCTL,  82, __s32)
#define AI32SSC_IOCTL_TT_REF_28			_IOWR(GSC_IOCTL,  83, __s32)
#define AI32SSC_IOCTL_TT_REF_29			_IOWR(GSC_IOCTL,  84, __s32)
#define AI32SSC_IOCTL_TT_REF_30			_IOWR(GSC_IOCTL,  85, __s32)
#define AI32SSC_IOCTL_TT_REF_31			_IOWR(GSC_IOCTL,  86, __s32)
#define AI32SSC_IOCTL_TT_THR_00			_IOWR(GSC_IOCTL,  87, __s32)
#define AI32SSC_IOCTL_TT_THR_01			_IOWR(GSC_IOCTL,  88, __s32)
#define AI32SSC_IOCTL_TT_THR_02			_IOWR(GSC_IOCTL,  89, __s32)
#define AI32SSC_IOCTL_TT_THR_03			_IOWR(GSC_IOCTL,  90, __s32)
#define AI32SSC_IOCTL_TT_THR_04			_IOWR(GSC_IOCTL,  91, __s32)
#define AI32SSC_IOCTL_TT_THR_05			_IOWR(GSC_IOCTL,  92, __s32)
#define AI32SSC_IOCTL_TT_THR_06			_IOWR(GSC_IOCTL,  93, __s32)
#define AI32SSC_IOCTL_TT_THR_07			_IOWR(GSC_IOCTL,  94, __s32)
#define AI32SSC_IOCTL_TT_THR_08			_IOWR(GSC_IOCTL,  95, __s32)
#define AI32SSC_IOCTL_TT_THR_09			_IOWR(GSC_IOCTL,  96, __s32)
#define AI32SSC_IOCTL_TT_THR_10			_IOWR(GSC_IOCTL,  97, __s32)
#define AI32SSC_IOCTL_TT_THR_11			_IOWR(GSC_IOCTL,  98, __s32)
#define AI32SSC_IOCTL_TT_THR_12			_IOWR(GSC_IOCTL,  99, __s32)
#define AI32SSC_IOCTL_TT_THR_13			_IOWR(GSC_IOCTL, 100, __s32)
#define AI32SSC_IOCTL_TT_THR_14			_IOWR(GSC_IOCTL, 101, __s32)
#define AI32SSC_IOCTL_TT_THR_15			_IOWR(GSC_IOCTL, 102, __s32)
#define AI32SSC_IOCTL_TT_THR_16			_IOWR(GSC_IOCTL, 103, __s32)
#define AI32SSC_IOCTL_TT_THR_17			_IOWR(GSC_IOCTL, 104, __s32)
#define AI32SSC_IOCTL_TT_THR_18			_IOWR(GSC_IOCTL, 105, __s32)
#define AI32SSC_IOCTL_TT_THR_19			_IOWR(GSC_IOCTL, 106, __s32)
#define AI32SSC_IOCTL_TT_THR_20			_IOWR(GSC_IOCTL, 107, __s32)
#define AI32SSC_IOCTL_TT_THR_21			_IOWR(GSC_IOCTL, 108, __s32)
#define AI32SSC_IOCTL_TT_THR_22			_IOWR(GSC_IOCTL, 109, __s32)
#define AI32SSC_IOCTL_TT_THR_23			_IOWR(GSC_IOCTL, 110, __s32)
#define AI32SSC_IOCTL_TT_THR_24			_IOWR(GSC_IOCTL, 111, __s32)
#define AI32SSC_IOCTL_TT_THR_25			_IOWR(GSC_IOCTL, 112, __s32)
#define AI32SSC_IOCTL_TT_THR_26			_IOWR(GSC_IOCTL, 113, __s32)
#define AI32SSC_IOCTL_TT_THR_27			_IOWR(GSC_IOCTL, 114, __s32)
#define AI32SSC_IOCTL_TT_THR_28			_IOWR(GSC_IOCTL, 115, __s32)
#define AI32SSC_IOCTL_TT_THR_29			_IOWR(GSC_IOCTL, 116, __s32)
#define AI32SSC_IOCTL_TT_THR_30			_IOWR(GSC_IOCTL, 117, __s32)
#define AI32SSC_IOCTL_TT_THR_31			_IOWR(GSC_IOCTL, 118, __s32)
#define AI32SSC_IOCTL_TT_NRATE			_IOWR(GSC_IOCTL, 119, __s32)
#define AI32SSC_IOCTL_RX_IO_ABORT		_IOR (GSC_IOCTL, 120, __s32)
#define	AI32SSC_IOCTL_WAIT_EVENT		_IOWR(GSC_IOCTL, 121, gsc_wait_t)
#define	AI32SSC_IOCTL_WAIT_CANCEL		_IOWR(GSC_IOCTL, 122, gsc_wait_t)
#define	AI32SSC_IOCTL_WAIT_STATUS		_IOWR(GSC_IOCTL, 123, gsc_wait_t)
#define AI32SSC_IOCTL_SCAN_MARKER_VAL	_IOWR(GSC_IOCTL, 124, __u32)


//*****************************************************************************
// AI32SSC_IOCTL_REG_READ
// AI32SSC_IOCTL_REG_WRITE
// AI32SSC_IOCTL_REG_MOD
//
// Parameter:	gsc_reg_t*
#define AI32SSC_GSC_BCTLR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x00)	// Board Control Register
#define AI32SSC_GSC_ICR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x04)	// Interrupt Control Register
#define AI32SSC_GSC_IBDR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x08)	// Input Buffer Data Register
#define AI32SSC_GSC_IBCR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x0C)	// Input Buffer Control Register
#define AI32SSC_GSC_RAGR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x10)	// Rate-A Generator Register
#define AI32SSC_GSC_RBGR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x14)	// Rate-B Generator Register
#define AI32SSC_GSC_BUFSR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x18)	// Buffer Size Register
#define AI32SSC_GSC_BRSTSR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x1C)	// Burst Size Register
#define AI32SSC_GSC_SSCR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x20)	// Scan & Sync Control Register
#define AI32SSC_GSC_ACAR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x24)	// Active Channel Assignment Register
#define AI32SSC_GSC_BCFGR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x28)	// Board Configuration Register
#define AI32SSC_GSC_AVR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x2C)	// Auto Cal Values Register
#define AI32SSC_GSC_AWRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x30)	// Auxiliary R/W Register
#define AI32SSC_GSC_ASIOCR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x34)	// Auxiliary Sync I/O Control Register
#define AI32SSC_GSC_SMUWR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x38)	// Scan Marker Upper Word Register
#define AI32SSC_GSC_SMLWR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x3C)	// Scan Marker Lower Word Register

// These are the Time Tag registers
#define AI32SSC_GSC_TTCR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x50)	// Time Tag Cinfiguration Register
#define AI32SSC_GSC_TTACMR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x54)	// Time Tag Active Channel Mask Register
#define AI32SSC_GSC_TTCLR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x58)	// Time Tag Counter Lower Register
#define AI32SSC_GSC_TTCUR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x5C)	// Time Tag Counter Upper Register
#define AI32SSC_GSC_TTRDR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x60)	// Time Tag Rate Divider Register
#define AI32SSC_GSC_TTBSR		GSC_REG_ENCODE(GSC_REG_GSC,4,0x64)	// Time Tag Burst Size Register

#define AI32SSC_GSC_TTC00TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x80)	// Time Tag Channel 00 Threshold/Reference Register
#define AI32SSC_GSC_TTC01TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x84)	// Time Tag Channel 01 Threshold/Reference Register
#define AI32SSC_GSC_TTC02TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x88)	// Time Tag Channel 02 Threshold/Reference Register
#define AI32SSC_GSC_TTC03TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x8C)	// Time Tag Channel 03 Threshold/Reference Register
#define AI32SSC_GSC_TTC04TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x90)	// Time Tag Channel 04 Threshold/Reference Register
#define AI32SSC_GSC_TTC05TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x94)	// Time Tag Channel 05 Threshold/Reference Register
#define AI32SSC_GSC_TTC06TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x98)	// Time Tag Channel 06 Threshold/Reference Register
#define AI32SSC_GSC_TTC07TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0x9C)	// Time Tag Channel 07 Threshold/Reference Register
#define AI32SSC_GSC_TTC08TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xA0)	// Time Tag Channel 08 Threshold/Reference Register
#define AI32SSC_GSC_TTC09TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xA4)	// Time Tag Channel 09 Threshold/Reference Register

#define AI32SSC_GSC_TTC10TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xA8)	// Time Tag Channel 10 Threshold/Reference Register
#define AI32SSC_GSC_TTC11TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xAC)	// Time Tag Channel 11 Threshold/Reference Register
#define AI32SSC_GSC_TTC12TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xB0)	// Time Tag Channel 12 Threshold/Reference Register
#define AI32SSC_GSC_TTC13TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xB4)	// Time Tag Channel 13 Threshold/Reference Register
#define AI32SSC_GSC_TTC14TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xB8)	// Time Tag Channel 14 Threshold/Reference Register
#define AI32SSC_GSC_TTC15TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xBC)	// Time Tag Channel 15 Threshold/Reference Register
#define AI32SSC_GSC_TTC16TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xC0)	// Time Tag Channel 16 Threshold/Reference Register
#define AI32SSC_GSC_TTC17TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xC4)	// Time Tag Channel 17 Threshold/Reference Register
#define AI32SSC_GSC_TTC18TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xC8)	// Time Tag Channel 18 Threshold/Reference Register
#define AI32SSC_GSC_TTC19TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xCC)	// Time Tag Channel 19 Threshold/Reference Register

#define AI32SSC_GSC_TTC20TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xD0)	// Time Tag Channel 20 Threshold/Reference Register
#define AI32SSC_GSC_TTC21TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xD4)	// Time Tag Channel 21 Threshold/Reference Register
#define AI32SSC_GSC_TTC22TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xD8)	// Time Tag Channel 22 Threshold/Reference Register
#define AI32SSC_GSC_TTC23TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xDC)	// Time Tag Channel 23 Threshold/Reference Register
#define AI32SSC_GSC_TTC24TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xE0)	// Time Tag Channel 24 Threshold/Reference Register
#define AI32SSC_GSC_TTC25TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xE4)	// Time Tag Channel 25 Threshold/Reference Register
#define AI32SSC_GSC_TTC26TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xE8)	// Time Tag Channel 26 Threshold/Reference Register
#define AI32SSC_GSC_TTC27TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xEC)	// Time Tag Channel 27 Threshold/Reference Register
#define AI32SSC_GSC_TTC28TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xF0)	// Time Tag Channel 28 Threshold/Reference Register
#define AI32SSC_GSC_TTC29TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xF4)	// Time Tag Channel 29 Threshold/Reference Register

#define AI32SSC_GSC_TTC30TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xF8)	// Time Tag Channel 30 Threshold/Reference Register
#define AI32SSC_GSC_TTC31TRR	GSC_REG_ENCODE(GSC_REG_GSC,4,0xFC)	// Time Tag Channel 31 Threshold/Reference Register

//*****************************************************************************
// AI32SSC_IOCTL_QUERY
//
//	Parameter:	s32
//		Pass in a value from the list below.
//		The value returned is the answer to the query.

typedef enum
{
	AI32SSC_QUERY_AUTO_CAL_MS,		// Max auto-cal period in ms.
	AI32SSC_QUERY_CHANNEL_MAX,		// Maximum number of channels supported.
	AI32SSC_QUERY_CHANNEL_QTY,		// The number of A/D channels.
	AI32SSC_QUERY_COUNT,			// How many query options are supported?
	AI32SSC_QUERY_DEVICE_TYPE,		// Value from gsc_dev_type_t
	AI32SSC_QUERY_FGEN_MAX,			// Rate Generator maximum output rate.
	AI32SSC_QUERY_FGEN_MIN,			// Rate Generator minimum output rate.
	AI32SSC_QUERY_FIFO_SIZE,		// FIFO depth in 32-bit samples
	AI32SSC_QUERY_FSAMP_MAX,		// The maximum sample rate per channel.
	AI32SSC_QUERY_FSAMP_MIN,		// The minimum sample rate per channel.
	AI32SSC_QUERY_INIT_MS,			// Max initialize period in ms.
	AI32SSC_QUERY_MASTER_CLOCK,		// Master clock frequency
	AI32SSC_QUERY_NRATE_MASK,		// Mask of valid Nrate bits.
	AI32SSC_QUERY_NRATE_MAX,		// Maximum rate generator Nrate value.
	AI32SSC_QUERY_NRATE_MIN,		// Maximum rate generator Nrate value.
	AI32SSC_QUERY_RATE_GEN_QTY,		// Number of Rate Generatorts.
	AI32SSC_QUERY_TIME_TAG			// Is Time Tagging supported?

} ai32ssc_query_t;

#define	AI32SSC_IOCTL_QUERY_LAST			AI32SSC_QUERY_TIME_TAG
#define	AI32SSC_IOCTL_QUERY_ERROR			(-1)

//*****************************************************************************
// AI32SSC_IOCTL_INITIALIZE					BCTLR D15
//
//	Parameter:	None

//*****************************************************************************
// AI32SSC_IOCTL_AUTO_CALIBRATE				BCTLR D13
//
//	Parameter:	None

//*****************************************************************************
// AI32SSC_IOCTL_RX_IO_MODE
//
//	Parameter:	__s32*
//		Pass in any of the gsc_io_mode_t options, or
//		-1 to read the current setting.
#define	AI32SSC_IO_MODE_DEFAULT				GSC_IO_MODE_PIO

//*****************************************************************************
// AI32SSC_IOCTL_RX_IO_OVERFLOW
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IO_OVERFLOW_DEFAULT			AI32SSC_IO_OVERFLOW_CHECK
#define	AI32SSC_IO_OVERFLOW_IGNORE			0
#define	AI32SSC_IO_OVERFLOW_CHECK			1

//*****************************************************************************
// AI32SSC_IOCTL_RX_IO_TIMEOUT (in seconds)
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IO_TIMEOUT_DEFAULT			10
#define	AI32SSC_IO_TIMEOUT_NO_SLEEP			0
#define	AI32SSC_IO_TIMEOUT_MIN				0
#define	AI32SSC_IO_TIMEOUT_MAX				3600	// 1 hour

//*****************************************************************************
// AI32SSC_IOCTL_RX_IO_UNDERFLOW
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IO_UNDERFLOW_DEFAULT		AI32SSC_IO_UNDERFLOW_CHECK
#define	AI32SSC_IO_UNDERFLOW_IGNORE			0
#define	AI32SSC_IO_UNDERFLOW_CHECK			1

//*****************************************************************************
// AI32SSC_IOCTL_ADC_CLK_SRC				SSCR D3-D4
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_ADC_CLK_SRC_EXT				0
#define	AI32SSC_ADC_CLK_SRC_RAG				1
#define	AI32SSC_ADC_CLK_SRC_RBG				2
#define	AI32SSC_ADC_CLK_SRC_BCR				3

//*****************************************************************************
// AI32SSC_IOCTL_ADC_ENABLE					SSCR D5
//
//	Parameter:	__s32*
//		Pass in any valid option below, or
//		-1 to read the current setting.
#define	AI32SSC_ADC_ENABLE_NO				0	// Disable ADC clocking.
#define	AI32SSC_ADC_ENABLE_YES				1	// Enable ADC clocking.

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_CLEAR				IBCR D18
//
//	Parameter:	None.

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_LEVEL				BUFSR D0-D18
//
//	Parameter:	__s32*
//		The value returned is the current input buffer fill level.

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_OVERFLOW			BCTLR D17
//
//	Parameter:	__s32*
//		Pass in any valid option below, or
//		-1 to read the current setting.
#define	AI32SSC_AIN_BUF_OVERFLOW_CLEAR		0
#define	AI32SSC_AIN_BUF_OVERFLOW_IGNORE		1

// For queries the following values are returned.
#define	AI32SSC_AIN_BUF_OVERFLOW_NO			0
#define	AI32SSC_AIN_BUF_OVERFLOW_YES		1

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_THR_LVL			IBCR D0-D17
//
//	Parameter:	__s32*
//		Pass in any valid value from 0x0 to 0x3FFFF, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_THR_STS			IBCR D19
//
//	Parameter:	__s32*
//		The value returned is one of the following.
#define AI32SSC_AIN_BUF_THR_STS_CLEAR		0
#define AI32SSC_AIN_BUF_THR_STS_SET			1

//*****************************************************************************
// AI32SSC_IOCTL_AIN_BUF_UNDERFLOW			BCTLR D16
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AIN_BUF_UNDERFLOW_CLEAR		0
#define	AI32SSC_AIN_BUF_UNDERFLOW_IGNORE	1

// For queries the following values are returned.
#define	AI32SSC_AIN_BUF_UNDERFLOW_NO		0
#define	AI32SSC_AIN_BUF_UNDERFLOW_YES		1

//*****************************************************************************
// AI32SSC_IOCTL_AIN_MODE					BCTLR D0-D2
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AIN_MODE_DIFF				0	// Differential
#define	AI32SSC_AIN_MODE_ZERO				2	// Zero test
#define	AI32SSC_AIN_MODE_VREF				3	// Vref test

//*****************************************************************************
// AI32SSC_IOCTL_AIN_RANGE					BCTLR D4-D5
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AIN_RANGE_2_5V				0	// +- 2.5 volts
#define	AI32SSC_AIN_RANGE_5V				1	// +- 5 volts
#define	AI32SSC_AIN_RANGE_10V				2	// +- 10 volts

//*****************************************************************************
// AI32SSC_IOCTL_BURST_BUSY					BCTLR D12
//
//	Parameter:	__s32*
//		The value returned is one of the following.
#define AI32SSC_BURST_BUSY_IDLE				0
#define AI32SSC_BURST_BUSY_ACTIVE			1

//*****************************************************************************
// AI32SSC_IOCTL_BURST_SIZE					BRSTSR D0-D19
//
//	Parameter:	__s32*
//		Pass in any value between the below inclusive limits, or
//		-1 to read the current setting.
//		At this time the valid range is 0-0xFFFFF.

//*****************************************************************************
// AI32SSC_IOCTL_BURST_SYNC					SSCR D8-D9
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_BURST_SYNC_DISABLE			0
#define	AI32SSC_BURST_SYNC_RBG				1
#define	AI32SSC_BURST_SYNC_EXT				2
#define	AI32SSC_BURST_SYNC_BCR				3

//*****************************************************************************
// AI32SSC_IOCTL_CHAN_ACTIVE				SSCR D0-D2
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_CHAN_ACTIVE_SINGLE			0
#define	AI32SSC_CHAN_ACTIVE_0_1				1
#define	AI32SSC_CHAN_ACTIVE_0_3				2
#define	AI32SSC_CHAN_ACTIVE_0_7				3
#define	AI32SSC_CHAN_ACTIVE_0_15			4
#define	AI32SSC_CHAN_ACTIVE_0_31			5
#define	AI32SSC_CHAN_ACTIVE_RANGE			7

//*****************************************************************************
// AI32SSC_IOCTL_CHAN_FIRST					ACAR D0-D7
//
//	Parameter:	__s32*
//		Pass in any valid value from zero to the index of the last channel,
//		but not more than the LAST channel selection, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_CHAN_LAST					ACAR D8-D15
//
//	Parameter:	__s32*
//		Pass in any valid value from zero to the index of the last channel,
//		but not less than the FIRST channel selection, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_CHAN_SINGLE				SSCR D12-D17
//
//	Parameter:	__s32*
//		Pass in any valid value from zero to the index of the last channel, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_DATA_FORMAT				BCTLR D6
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_DATA_FORMAT_2S_COMP			0	// Twos Compliment
#define	AI32SSC_DATA_FORMAT_OFF_BIN			1	// Offset Binary

//*****************************************************************************
// AI32SSC_IOCTL_DATA_PACKING				BCTLR D18
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_DATA_PACKING_DISABLE		0
#define	AI32SSC_DATA_PACKING_ENABLE			1

//*****************************************************************************
// AI32SSC_IOCTL_RAG_ENABLE					RAGR D16
// AI32SSC_IOCTL_RBG_ENABLE					RBGR D16
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_GEN_ENABLE_NO				1
#define	AI32SSC_GEN_ENABLE_YES				0

//*****************************************************************************
// AI32SSC_IOCTL_RAG_NRATE					RAGR D0-D15
// AI32SSC_IOCTL_RBG_NRATE					RBGR D0-D15
//	Parameter:	__s32*
//		Pass in any value between the below inclusive limits, or
//		-1 to read the current setting.
//		At this time the valid range is 250-0xFFFF.

//*****************************************************************************
// AI32SSC_IOCTL_RBG_CLK_SRC				SSCR D10
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_RBG_CLK_SRC_MASTER			0
#define	AI32SSC_RBG_CLK_SRC_RAG				1

//*****************************************************************************
// AI32SSC_IOCTL_INPUT_SYNC					BCTLR D12
//
//	Parameter:	None.

//*****************************************************************************
// AI32SSC_IOCTL_IO_INV						SSCR D11
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IO_INV_LOW					0	// assert LOW
#define	AI32SSC_IO_INV_HIGH					1	// assert HI

//*****************************************************************************
// AI32SSC_IOCTL_IRQ0_SEL					ICR D0-D2
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IRQ0_INIT_DONE				0
#define	AI32SSC_IRQ0_AUTO_CAL_DONE			1
#define	AI32SSC_IRQ0_SYNC_START				2
#define	AI32SSC_IRQ0_SYNC_DONE				3
#define	AI32SSC_IRQ0_BURST_START			4
#define	AI32SSC_IRQ0_BURST_DONE				5

//*****************************************************************************
// AI32SSC_IOCTL_IRQ1_SEL					ICR D4-D6
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_IRQ1_NONE					0
#define	AI32SSC_IRQ1_IN_BUF_THR_L2H			1
#define	AI32SSC_IRQ1_IN_BUF_THR_H2L			2
#define	AI32SSC_IRQ1_IN_BUF_OVR_UNDR		3

//*****************************************************************************
// AI32SSC_IOCTL_SCAN_MARKER				BCTLR D11
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_SCAN_MARKER_DISABLE			1
#define	AI32SSC_SCAN_MARKER_ENABLE			0

//*****************************************************************************
// AI32SSC_IOCTL_AUX_CLK_MODE				ASIOCR D0-D1
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AUX_CLK_MODE_DISABLE		0
#define	AI32SSC_AUX_CLK_MODE_INPUT			1
#define	AI32SSC_AUX_CLK_MODE_OUTPUT			2

//*****************************************************************************
// AI32SSC_IOCTL_AUX_IN_POL					ASIOCR D8
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AUX_IN_POL_LO_2_HI			0
#define	AI32SSC_AUX_IN_POL_HI_2_LO			1

//*****************************************************************************
// AI32SSC_IOCTL_AUX_NOISE					ASIOCR D10
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AUX_NOISE_LOW				0
#define	AI32SSC_AUX_NOISE_HIGH				1

//*****************************************************************************
// AI32SSC_IOCTL_AUX_OUT_POL				ASIOCR D9
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AUX_OUT_POL_HI_PULSE		0
#define	AI32SSC_AUX_OUT_POL_LOW_PULSE		1

//*****************************************************************************
// AI32SSC_IOCTL_AUX_SYNC_MODE				ASIOCR D2-D3
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_AUX_SYNC_MODE_DISABLE		0
#define	AI32SSC_AUX_SYNC_MODE_INPUT			1
#define	AI32SSC_AUX_SYNC_MODE_OUTPUT		2

//*****************************************************************************
// AI32SSC_IOCTL_TT_ADC_CLK_SRC				TTCR D0-D1
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_ADC_CLK_SRC_RAG			0
#define	AI32SSC_TT_ADC_CLK_SRC_EXT_SAMP		1
#define	AI32SSC_TT_ADC_CLK_SRC_EXT_REF		2

//*****************************************************************************
// AI32SSC_IOCTL_TT_ADC_ENABLE				TTCR D2
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_ADC_ENABLE_NO			0
#define	AI32SSC_TT_ADC_ENABLE_YES			1

//*****************************************************************************
// AI32SSC_IOCTL_TT_CHAN_MASK				TTACMR D0-D31
//
//	Parameter:	__u32*
//		Pass in any bit-wise combination of bits to enable/disable the desired
//		channels. A "1" enables and "0" disables each respective channel.

//*****************************************************************************
// AI32SSC_IOCTL_TT_ENABLE					BCTLR D20
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_ENABLE_NO				0
#define	AI32SSC_TT_ENABLE_YES				1

//*****************************************************************************
// AI32SSC_IOCTL_TT_LOG_MODE				TTCR D6
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_LOG_MODE_TRIG			0
#define	AI32SSC_TT_LOG_MODE_ALL				1

//*****************************************************************************
// AI32SSC_IOCTL_TT_REF_CLK_SRC				TTCR D8
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_REF_CLK_SRC_INT			0
#define	AI32SSC_TT_REF_CLK_SRC_EXT			1

//*****************************************************************************
// AI32SSC_IOCTL_TT_REF_VAL_MODE			TTCR D5
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_REF_VAL_MODE_AUTO		0
#define	AI32SSC_TT_REF_VAL_MODE_CONST		1

//*****************************************************************************
// AI32SSC_IOCTL_TT_RESET					TTCR D9
//
//	Parameter:	None.

//*****************************************************************************
// AI32SSC_IOCTL_TT_RESET_EXT				TTCR D10
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_RESET_EXT_DISABLE		0
#define	AI32SSC_TT_RESET_EXT_ENABLE			1

//*****************************************************************************
// AI32SSC_IOCTL_TT_TAGGING					TTCR D11
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_TAGGING_DISABLE			0
#define	AI32SSC_TT_TAGGING_ENABLE			1

//*****************************************************************************
// AI32SSC_IOCTL_TT_TRIG_MODE				TTCR D4
//
//	Parameter:	__s32*
//		Pass in any of the below options, or
//		-1 to read the current setting.
#define	AI32SSC_TT_TRIG_MODE_CONTIN			0
#define	AI32SSC_TT_TRIG_MODE_REF_TRIG		1

//*****************************************************************************
// AI32SSC_IOCTL_TT_REF_00					TTC00TRR D0-D15
// AI32SSC_IOCTL_TT_REF_01					TTC01TRR D0-D15
// AI32SSC_IOCTL_TT_REF_02					TTC02TRR D0-D15
// AI32SSC_IOCTL_TT_REF_03					TTC03TRR D0-D15
// AI32SSC_IOCTL_TT_REF_04					TTC04TRR D0-D15
// AI32SSC_IOCTL_TT_REF_05					TTC05TRR D0-D15
// AI32SSC_IOCTL_TT_REF_06					TTC06TRR D0-D15
// AI32SSC_IOCTL_TT_REF_07					TTC07TRR D0-D15
// AI32SSC_IOCTL_TT_REF_08					TTC08TRR D0-D15
// AI32SSC_IOCTL_TT_REF_09					TTC09TRR D0-D15
// AI32SSC_IOCTL_TT_REF_10					TTC10TRR D0-D15
// AI32SSC_IOCTL_TT_REF_11					TTC11TRR D0-D15
// AI32SSC_IOCTL_TT_REF_12					TTC12TRR D0-D15
// AI32SSC_IOCTL_TT_REF_13					TTC13TRR D0-D15
// AI32SSC_IOCTL_TT_REF_14					TTC14TRR D0-D15
// AI32SSC_IOCTL_TT_REF_15					TTC15TRR D0-D15
// AI32SSC_IOCTL_TT_REF_16					TTC16TRR D0-D15
// AI32SSC_IOCTL_TT_REF_17					TTC17TRR D0-D15
// AI32SSC_IOCTL_TT_REF_18					TTC18TRR D0-D15
// AI32SSC_IOCTL_TT_REF_19					TTC19TRR D0-D15
// AI32SSC_IOCTL_TT_REF_20					TTC20TRR D0-D15
// AI32SSC_IOCTL_TT_REF_21					TTC21TRR D0-D15
// AI32SSC_IOCTL_TT_REF_22					TTC22TRR D0-D15
// AI32SSC_IOCTL_TT_REF_23					TTC23TRR D0-D15
// AI32SSC_IOCTL_TT_REF_24					TTC24TRR D0-D15
// AI32SSC_IOCTL_TT_REF_25					TTC25TRR D0-D15
// AI32SSC_IOCTL_TT_REF_26					TTC26TRR D0-D15
// AI32SSC_IOCTL_TT_REF_27					TTC27TRR D0-D15
// AI32SSC_IOCTL_TT_REF_28					TTC28TRR D0-D15
// AI32SSC_IOCTL_TT_REF_29					TTC29TRR D0-D15
// AI32SSC_IOCTL_TT_REF_30					TTC30TRR D0-D15
// AI32SSC_IOCTL_TT_REF_31					TTC31TRR D0-D15
//
//	Parameter:	__s32*
//		Pass in any value from 0x0 to 0xFFFF, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_TT_THR_00					TTC00TRR D16-D31
// AI32SSC_IOCTL_TT_THR_01					TTC01TRR D16-D31
// AI32SSC_IOCTL_TT_THR_02					TTC02TRR D16-D31
// AI32SSC_IOCTL_TT_THR_03					TTC03TRR D16-D31
// AI32SSC_IOCTL_TT_THR_04					TTC04TRR D16-D31
// AI32SSC_IOCTL_TT_THR_05					TTC05TRR D16-D31
// AI32SSC_IOCTL_TT_THR_06					TTC06TRR D16-D31
// AI32SSC_IOCTL_TT_THR_07					TTC07TRR D16-D31
// AI32SSC_IOCTL_TT_THR_08					TTC08TRR D16-D31
// AI32SSC_IOCTL_TT_THR_09					TTC09TRR D16-D31
// AI32SSC_IOCTL_TT_THR_10					TTC10TRR D16-D31
// AI32SSC_IOCTL_TT_THR_11					TTC11TRR D16-D31
// AI32SSC_IOCTL_TT_THR_12					TTC12TRR D16-D31
// AI32SSC_IOCTL_TT_THR_13					TTC13TRR D16-D31
// AI32SSC_IOCTL_TT_THR_14					TTC14TRR D16-D31
// AI32SSC_IOCTL_TT_THR_15					TTC15TRR D16-D31
// AI32SSC_IOCTL_TT_THR_16					TTC16TRR D16-D31
// AI32SSC_IOCTL_TT_THR_17					TTC17TRR D16-D31
// AI32SSC_IOCTL_TT_THR_18					TTC18TRR D16-D31
// AI32SSC_IOCTL_TT_THR_19					TTC19TRR D16-D31
// AI32SSC_IOCTL_TT_THR_20					TTC20TRR D16-D31
// AI32SSC_IOCTL_TT_THR_21					TTC21TRR D16-D31
// AI32SSC_IOCTL_TT_THR_22					TTC22TRR D16-D31
// AI32SSC_IOCTL_TT_THR_23					TTC23TRR D16-D31
// AI32SSC_IOCTL_TT_THR_24					TTC24TRR D16-D31
// AI32SSC_IOCTL_TT_THR_25					TTC25TRR D16-D31
// AI32SSC_IOCTL_TT_THR_26					TTC26TRR D16-D31
// AI32SSC_IOCTL_TT_THR_27					TTC27TRR D16-D31
// AI32SSC_IOCTL_TT_THR_28					TTC28TRR D16-D31
// AI32SSC_IOCTL_TT_THR_29					TTC29TRR D16-D31
// AI32SSC_IOCTL_TT_THR_30					TTC30TRR D16-D31
// AI32SSC_IOCTL_TT_THR_31					TTC31TRR D16-D31
//
//	Parameter:	__s32*
//		Pass in any value from 0x0 to 0xFFFF, or
//		-1 to read the current setting.

//*****************************************************************************
// AI32SSC_IOCTL_TT_NRATE					TTRDR D0-D19
//	Parameter:	__s32*
//		Pass in any value between the below inclusive limits, or
//		-1 to read the current setting.
//		At this time the valid range is 2-0xFFFFF.

//*****************************************************************************
// AI32SSC_IOCTL_RX_IO_ABORT
//
//	Parameter:	__s32*
//		The returned value is one of the below options.
#define	AI32SSC_IO_ABORT_NO					0
#define	AI32SSC_IO_ABORT_YES				1

//*****************************************************************************
// AI32SSC_IOCTL_WAIT_EVENT					all fields must be valid
// AI32SSC_IOCTL_WAIT_CANCEL				fields need not be valid
// AI32SSC_IOCTL_WAIT_STATUS				fields need not be valid
//
//	Parameter:	gsc_wait_t*
// gsc_wait_t.flags - see gsc_common.h
// gsc_wait_t.main - see gsc_common.h
// gsc_wait_t.gsc
#define	AI32SSC_WAIT_GSC_INIT_DONE			0x0001
#define	AI32SSC_WAIT_GSC_AUTO_CAL_DONE		0x0002
#define	AI32SSC_WAIT_GSC_SYNC_START			0x0004
#define	AI32SSC_WAIT_GSC_SYNC_DONE			0x0008
#define	AI32SSC_WAIT_GSC_BURST_START		0x0010
#define	AI32SSC_WAIT_GSC_BURST_DONE			0x0020
#define	AI32SSC_WAIT_GSC_IN_BUF_THR_L2H		0x0040
#define	AI32SSC_WAIT_GSC_IN_BUF_THR_H2L		0x0080
#define	AI32SSC_WAIT_GSC_IN_BUF_OVR_UNDR	0x0100
#define	AI32SSC_WAIT_GSC_ALL				0x01FF
// gsc_wait_t.alt flags
#define	AI32SSC_WAIT_ALT_ALL				0x0000
// gsc_wait_t.io - see gsc_common.h

//*****************************************************************************
// AI32SSC_IOCTL_SCAN_MARKER_VAL			SMUWR D0-D15, SMLWR D0-D15
//
//	Parameter:	__u32*
//		Pass in any value between the range 0 to 0xFFFFFFFF, or
//		-1 to read the current setting.

#endif
