// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_time.c $
// $Rev$
// $Date$

#include "main.h"



/******************************************************************************
*
*	Function:	gsc_time_delta
*
*	Purpose:
*
*		Return the different in time between the two values provided. This
*		function is intended to work with jiffies, but may work with other time
*		units as well. This function is intended to work ONLY over a relatively
*		short span of time.
*
*	Arguments:
*
*		t1		This is the reference time.
*
*		t2		This is the time whose difference from t1 is desired.
*
*	Returned:
*
*		> 0		t2 is after t1 by the reported amount.
*		0		t2 equals t1.
*		< 0		t2 is before t1 by the reported amount.
*
******************************************************************************/

long gsc_time_delta(unsigned long t1, unsigned long t2)
{
	long	delta;

	// If the jiffies wraparound mechanism changes,
	// then this may need changing as well.

	if (t2 >= t1)
		delta	= (long) (t2 - t1);
	else
		delta	= (long) ((long) t2 - (long) t1);

	return(delta);
}


