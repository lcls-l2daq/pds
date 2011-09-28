// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_sem.c $
// $Rev: 3175 $
// $Date$

#include "gsc_main.h"



/******************************************************************************
*
*	Function:	gsc_sem_create
*
*	Purpose:
*
*		Create a semaphore.
*
*	Arguments:
*
*		sem		The OS specific semaphore data is stored here.
*
*		count	The initial count for the semaphore.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_sem_create(gsc_sem_t* sem)
{
	sema_init(sem, 1);
}



/******************************************************************************
*
*	Function:	gsc_sem_destroy
*
*	Purpose:
*
*		Destroy a semaphore.
*
*	Arguments:
*
*		sem		The OS specific semaphore data is stored here.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_sem_destroy(gsc_sem_t* sem)
{
	if (sem)
		memset(sem, 0, sizeof(gsc_sem_t));
}



/******************************************************************************
*
*	Function:	gsc_sem_lock
*
*	Purpose:
*
*		Acquire a semaphore.
*
*	Arguments:
*
*		sem		The OS specific semaphore data is stored here.
*
*	Returned:
*
*		0		All went well.
*		< 0		The error for the problem.
*
******************************************************************************/

int gsc_sem_lock(gsc_sem_t* sem)
{
	int	ret;

	ret	= down_interruptible(sem);

	if (ret)
		ret	= -ERESTARTSYS;

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_sem_unlock
*
*	Purpose:
*
*		Release a semaphore.
*
*	Arguments:
*
*		sem		The semaphore to acquire.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_sem_unlock(gsc_sem_t* sem)
{
	up(sem);
}


